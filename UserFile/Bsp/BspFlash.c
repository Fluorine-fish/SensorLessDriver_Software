/**
*   @file BspFlash.c
*   @brief Sector 11 按键追加日志，BootInfo 与电机参数共存
*   @date 2026/11/25
*   @version 2.0
*/
#include "BspFlash.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define RECORD_MAGIC 0x314C4E4AU
#define RECORD_COMMIT 0x434F4D54U
#define RECORD_COUNT (BSP_FLASH_LOG_SIZE / BSP_FLASH_RECORD_SIZE)
#define RECORD_COMMIT_OFFSET (BSP_FLASH_RECORD_SIZE - 4U)

typedef struct {
    uint32_t magic;
    uint32_t key;
    uint32_t length;
    uint32_t format;
    uint32_t payload_crc;
    uint32_t header_crc;
} BspFlashHeader_s;

_Static_assert(sizeof(BspFlashHeader_s) == 24U, "Journal header layout");
_Static_assert(sizeof(BspFlashHeader_s) + BSP_FLASH_VALUE_MAX <= RECORD_COMMIT_OFFSET,
               "Journal payload overlaps commit");

uint32_t BspFlash_Crc32(const void *data, uint32_t length) {
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (uint32_t bit = 0; bit < 8U; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

static const uint8_t *BspFlash_Slot(uint32_t index) {
    return (const uint8_t *)(uintptr_t)(BSP_FLASH_LOG_ADDR + index * BSP_FLASH_RECORD_SIZE);
}

static bool BspFlash_IsEmpty(const uint8_t *slot) {
    for (uint32_t i = 0; i < BSP_FLASH_RECORD_SIZE; ++i) {
        if (slot[i] != 0xFFU) return false;
    }
    return true;
}

static bool BspFlash_IsCommitted(const uint8_t *slot) {
    uint32_t commit;
    memcpy(&commit, slot + RECORD_COMMIT_OFFSET, sizeof(commit));
    return commit == RECORD_COMMIT;
}

static bool BspFlash_HeaderValid(const BspFlashHeader_s *header) {
    return header->magic == RECORD_MAGIC && header->format == 1U &&
           header->key != 0U && header->key != UINT32_MAX &&
           header->length > 0U && header->length <= BSP_FLASH_VALUE_MAX &&
           header->header_crc == BspFlash_Crc32(header, 20U);
}

BspFlashResult_e BspFlash_GetStatus(BspFlashStatus_s *status) {
    if (status == NULL) return BSP_FLASH_BAD_ARGUMENT;
    memset(status, 0, sizeof(*status));
    bool foreign = false;
    uint32_t tail = 0U;
    for (uint32_t i = 0; i < RECORD_COUNT; ++i) {
        const uint8_t *slot = BspFlash_Slot(i);
        if (BspFlash_IsEmpty(slot)) {
            continue;
        }
        ++status->used_slots;
        tail = i + 1U;
        BspFlashHeader_s header;
        memcpy(&header, slot, sizeof(header));
        if (header.magic != RECORD_MAGIC) foreign = true;
        if (!BspFlash_IsCommitted(slot) || !BspFlash_HeaderValid(&header) ||
            header.payload_crc != BspFlash_Crc32(slot + sizeof(header), header.length)) {
            ++status->damaged_slots;
        }
    }
    status->free_slots = RECORD_COUNT - tail;
    status->blank = status->used_slots == 0U;
    /* 未识别的旧数据不得当作空白区使用，必须先通过 SWD 备份。 */
    return foreign ? BSP_FLASH_CORRUPT : BSP_FLASH_OK;
}

BspFlashResult_e BspFlash_ReadRecord(uint32_t key, void *data,
                                    uint32_t capacity, uint32_t *length) {
    if (data == NULL || length == NULL || key == 0U || key == UINT32_MAX) {
        return BSP_FLASH_BAD_ARGUMENT;
    }
    *length = 0U;
    const uint8_t *latest = NULL;
    BspFlashHeader_s header;
    for (uint32_t i = 0; i < RECORD_COUNT; ++i) {
        const uint8_t *slot = BspFlash_Slot(i);
        if (BspFlash_IsEmpty(slot)) continue;
        memcpy(&header, slot, sizeof(header));
        if (header.magic != RECORD_MAGIC) return BSP_FLASH_CORRUPT;
        if (!BspFlash_IsCommitted(slot)) continue;
        /* 不回退到较旧的、可能已被替换的 BootInfo。 */
        if (!BspFlash_HeaderValid(&header)) return BSP_FLASH_CORRUPT;
        if (header.key == key) latest = slot;
    }
    if (latest == NULL) return BSP_FLASH_NOT_FOUND;
    memcpy(&header, latest, sizeof(header));
    if (header.payload_crc != BspFlash_Crc32(latest + sizeof(header), header.length)) {
        return BSP_FLASH_CORRUPT;
    }
    *length = header.length;
    if (capacity < header.length) return BSP_FLASH_BAD_ARGUMENT;
    memcpy(data, latest + sizeof(header), header.length);
    return BSP_FLASH_OK;
}

BspFlashResult_e BspFlash_WriteRecord(uint32_t key, const void *data, uint32_t length) {
    if (__get_IPSR() != 0U || __get_PRIMASK() != 0U || __get_BASEPRI() != 0U) {
        return BSP_FLASH_BAD_CONTEXT;
    }
    if (data == NULL || length == 0U || length > BSP_FLASH_VALUE_MAX ||
        key == 0U || key == UINT32_MAX) return BSP_FLASH_BAD_ARGUMENT;

    BspFlashStatus_s status;
    BspFlashResult_e result = BspFlash_GetStatus(&status);
    if (result != BSP_FLASH_OK) return result;
    /* 永远在最高已用槽之后写，不能利用旧记录之间的洞。 */
    uint32_t next = 0U;
    for (uint32_t i = 0; i < RECORD_COUNT; ++i) {
        if (!BspFlash_IsEmpty(BspFlash_Slot(i))) next = i + 1U;
    }
    if (next == RECORD_COUNT) return BSP_FLASH_FULL;

    uint32_t words[BSP_FLASH_RECORD_SIZE / 4U];
    memset(words, 0xFF, sizeof(words));
    BspFlashHeader_s header = {
        .magic = RECORD_MAGIC, .key = key, .length = length, .format = 1U,
        .payload_crc = BspFlash_Crc32(data, length)
    };
    header.header_crc = BspFlash_Crc32(&header, 20U);
    memcpy(words, &header, sizeof(header));
    memcpy((uint8_t *)words + sizeof(header), data, length);
    const uint8_t *target = BspFlash_Slot(next);
    if (HAL_FLASH_Unlock() != HAL_OK) return BSP_FLASH_IO_ERROR;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    uint32_t write_size = (sizeof(header) + length + 3U) & ~3U;
    for (uint32_t i = 0; i < write_size / 4U; ++i) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                             (uintptr_t)target + i * 4U, words[i]) != HAL_OK) {
            FLASH_FlushCaches();
            HAL_FLASH_Lock();
            return BSP_FLASH_IO_ERROR;
        }
    }
    FLASH_FlushCaches();
    if (memcmp(target, words, write_size) != 0) {
        HAL_FLASH_Lock();
        return BSP_FLASH_IO_ERROR;
    }
    /* 提交字最后写；未完成的槽不会成为有效值。 */
    HAL_StatusTypeDef commit = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
        (uintptr_t)target + RECORD_COMMIT_OFFSET, RECORD_COMMIT);
    FLASH_FlushCaches();
    HAL_FLASH_Lock();
    return commit == HAL_OK && BspFlash_IsCommitted(target) ?
           BSP_FLASH_OK : BSP_FLASH_IO_ERROR;
}

bool BspFlash_Write(uint16_t sector_id, uint32_t *data, uint32_t length) {
    return sector_id == FLASH_Sector_11 &&
           BspFlash_WriteRecord(BSP_FLASH_KEY_MOTOR_PARAMS, data, length) == BSP_FLASH_OK;
}

void BspFlash_Read(uint32_t offset, void *data, uint32_t length) {
    if (data == NULL || offset > DATA_FLASH_SIZE || length > DATA_FLASH_SIZE - offset) return;
    memcpy(data, (const void *)(uintptr_t)(ADDR_FLASH_SECTOR_0 + offset), length);
}
