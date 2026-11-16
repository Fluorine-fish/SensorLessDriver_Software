/**
*   @file BspFlash.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/16
*   @version 1.0
*   @note
*/
#include "BspFlash.h"
#include "stdint.h"
#include "stdbool.h"
#include "stm32f4xx_hal.h"
#include "string.h"

#define PROTECT_APP

// SECTOR_ID 需要使用宏
static bool BspFlash_EraseSector(uint16_t SECTOR_ID) {
#ifdef PROTECT_APP
    if (SECTOR_ID != FLASH_Sector_11) return false;
#endif

    FLASH_EraseInitTypeDef erase = {};
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t sector_error = 0xFFFFFFFFUL;

    if (HAL_FLASH_Unlock() != HAL_OK) return HAL_ERROR;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Banks = FLASH_BANK_1;
    erase.Sector = SECTOR_ID;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);

    HAL_FLASH_Lock();
    return (status == HAL_OK) && (sector_error == 0xFFFFFFFFUL);
}

static const uint32_t sector_address[] = {
    ADDR_FLASH_SECTOR_0,
    ADDR_FLASH_SECTOR_1,
    ADDR_FLASH_SECTOR_2,
    ADDR_FLASH_SECTOR_3,
    ADDR_FLASH_SECTOR_4,
    ADDR_FLASH_SECTOR_5,
    ADDR_FLASH_SECTOR_6,
    ADDR_FLASH_SECTOR_7,
    ADDR_FLASH_SECTOR_8,
    ADDR_FLASH_SECTOR_9,
    ADDR_FLASH_SECTOR_10,
    ADDR_FLASH_SECTOR_11
};

static const uint32_t sector_size[] = {
    16U * 1024U,    /* Sector 0  */
    16U * 1024U,    /* Sector 1  */
    16U * 1024U,    /* Sector 2  */
    16U * 1024U,    /* Sector 3  */
    64U * 1024U,    /* Sector 4  */
    128U * 1024U,   /* Sector 5  */
    128U * 1024U,   /* Sector 6  */
    128U * 1024U,   /* Sector 7  */
    128U * 1024U,   /* Sector 8  */
    128U * 1024U,   /* Sector 9  */
    128U * 1024U,   /* Sector 10 */
    128U * 1024U    /* Sector 11 */
};

// length为字节数
void BspFlash_Write(uint16_t SECTOR_ID,
                    uint32_t *data,
                    uint32_t length)
{
    const uint8_t *source = (const uint8_t *)data;
    uint32_t sector_count;
    uint32_t target_address;
    uint32_t target_size;
    uint32_t write_length;
    uint32_t offset;
    HAL_StatusTypeDef status;

    if (data == NULL || length == 0U) {
        return;
    }

    sector_count = sizeof(sector_address) / sizeof(sector_address[0]);

    if ((uint32_t)SECTOR_ID >= sector_count) {
        return;
    }

    target_address = sector_address[SECTOR_ID];
    target_size = sector_size[SECTOR_ID];

    if (length > target_size) {
        return;
    }

    /* 按 Word 写入，最后一个 Word 不足四字节时补 0xFF */
    write_length = (length + 3U) & ~3UL;

    if (write_length > target_size) {
        return;
    }

    /*
     * 先擦除整个目标扇区，再读取 data，
     * 因此 data 不能位于目标扇区内。
     */
    {
        uintptr_t source_begin = (uintptr_t)data;
        uintptr_t source_end = source_begin + length;
        uintptr_t target_begin = (uintptr_t)target_address;
        uintptr_t target_end = target_begin + target_size;

        if (source_end < source_begin) {
            return;
        }

        if (source_begin < target_end &&
            target_begin < source_end) {
            return;
        }
    }

    /* 擦除整个目标扇区 */
    if (!BspFlash_EraseSector(SECTOR_ID)) {
        return;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return;
    }

    status = HAL_OK;

    for (offset = 0U; offset < write_length; offset += 4U) {
        uint32_t remaining = length - offset;
        uint32_t copy_length = (remaining >= 4U) ? 4U : remaining;
        uint32_t word = 0xFFFFFFFFUL;

        memcpy(&word, source + offset, copy_length);

        status = HAL_FLASH_Program(
            FLASH_TYPEPROGRAM_WORD,
            target_address + offset,
            word
        );

        if (status != HAL_OK) {
            break;
        }
    }

    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        return;
    }

    /* 回读校验实际有效字节 */
    if (memcmp((const void *)target_address, data, length) != 0) {
        return;
    }
}

void BspFlash_Read(uint32_t offset, void* data, uint32_t length) {
    if (offset > DATA_FLASH_SIZE
        || length > DATA_FLASH_SIZE - offset) return;

    memcpy(data, (const void *)(offset + ADDR_FLASH_SECTOR_0), length);
}
