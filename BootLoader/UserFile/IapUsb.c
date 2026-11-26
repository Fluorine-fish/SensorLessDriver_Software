/**
*   @file IapUsb.c
*   @brief SimpleBootLoader 二进制协议的 USB CDC 实现
*   @date 2026/11/25
*/
#include "BootLoader.h"
#include "BspUsbVcp.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define IAP_REQUEST_SOF 0x5AU
#define IAP_RESPONSE_SOF 0xA5U
#define IAP_BLOCK_SIZE 256U
#define IAP_PAYLOAD_MAX (4U + IAP_BLOCK_SIZE)
#define IAP_BOOT_WAIT_MS 5000U
#define IAP_FRAME_TIMEOUT_MS 1000U

typedef enum {
    IAP_OK = 0, IAP_BAD_FRAME, IAP_BAD_LENGTH, IAP_BAD_STATE,
    IAP_FLASH_ERROR, IAP_CRC_ERROR, IAP_INVALID_APP, IAP_UNKNOWN_COMMAND
} IapStatus_e;

typedef struct {
    bool active;
    uint32_t next_offset;
    BootInfo_s info;
} IapSession_s;

static IapSession_s session;
static bool stay_in_boot;
static bool jump_pending;
static bool was_ready;
static uint32_t boot_tick;
static uint32_t jump_tick;
static uint32_t rx_tick;
static uint32_t usb_session;
/* 不包含 SOF；CMD + LEN(2) + payload + CRC(2)。 */
static uint8_t frame[3U + IAP_PAYLOAD_MAX + 2U];
static uint16_t frame_used;
static bool receiving;

static uint16_t IapUsb_Crc16(const uint8_t *data, uint32_t length) {
    uint16_t crc = 0xFFFFU;
    for (uint32_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint32_t bit = 0; bit < 8U; ++bit) {
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U));
        }
    }
    return crc;
}

static uint32_t IapUsb_Read32(const uint8_t *data) {
    return (uint32_t)data[0] | (uint32_t)data[1] << 8 |
           (uint32_t)data[2] << 16 | (uint32_t)data[3] << 24;
}

static void IapUsb_Respond(uint8_t command, IapStatus_e status,
                            const void *payload, uint16_t length) {
    uint8_t response[7U + sizeof(BootInfo_s)];
    response[0] = IAP_RESPONSE_SOF;
    response[1] = command;
    response[2] = status;
    response[3] = (uint8_t)length;
    response[4] = (uint8_t)(length >> 8);
    if (length != 0U) memcpy(response + 5U, payload, length);
    uint16_t crc = IapUsb_Crc16(response + 1U, 4U + length);
    response[5U + length] = (uint8_t)crc;
    response[6U + length] = (uint8_t)(crc >> 8);
    /* Poll 仅在 TX 完全空时解析下一帧；此响应最大39字节。 */
    if (BspUsbVcp_Write(response, 7U + length) != 7U + length) {
        jump_pending = false;
        session.active = false;
    }
}

static IapStatus_e IapUsb_Erase(const uint8_t *data, uint16_t length) {
    if (length != 12U) return IAP_BAD_LENGTH;
    uint32_t size = IapUsb_Read32(data);
    if (size < 8U || size > BOOT_APP_SIZE) return IAP_BAD_LENGTH;
    session.active = false;
    BspFlashStatus_s status;
    /* 一次升级至少需要 UPDATING、VALID 两条记录；先检查再动 APP。 */
    if (BspFlash_GetStatus(&status) != BSP_FLASH_OK || status.free_slots < 2U) {
        return IAP_FLASH_ERROR;
    }
    session.info = (BootInfo_s){
        .magic = BOOT_INFO_MAGIC, .state = BOOT_STATE_UPDATING,
        .app_addr = BOOT_APP_ADDR, .app_size = size,
        .app_crc32 = IapUsb_Read32(data + 4U), .app_version = IapUsb_Read32(data + 8U),
        .reserved0 = UINT32_MAX, .reserved1 = UINT32_MAX
    };
    if (BspFlash_WriteRecord(BSP_FLASH_KEY_BOOTINFO, &session.info,
                             sizeof(session.info)) != BSP_FLASH_OK) return IAP_FLASH_ERROR;
    if (!BootLoader_EraseApp()) return IAP_FLASH_ERROR;
    session.next_offset = 0U;
    session.active = true;
    return IAP_OK;
}

static IapStatus_e IapUsb_Write(const uint8_t *data, uint16_t length) {
    if (!session.active) return IAP_BAD_STATE;
    if (length < 12U || length > IAP_PAYLOAD_MAX) return IAP_BAD_LENGTH;
    uint32_t offset = IapUsb_Read32(data);
    uint32_t count = length - 4U;
    uint32_t padded = (session.info.app_size + 7U) & ~7U;
    /* 保留原上位机的8字节传输对齐，芯片底层按4字节编程。 */
    if ((offset & 7U) != 0U || (count & 7U) != 0U ||
        offset > padded || count > padded - offset) return IAP_BAD_LENGTH;
    for (uint32_t i = 0; i < count; ++i) {
        if (offset + i >= session.info.app_size && data[4U + i] != 0xFFU) return IAP_BAD_LENGTH;
    }
    if (offset < session.next_offset) {
        /* 已确认块的完全相同重发可幂等应答，不重复编程 Flash。 */
        return count <= session.next_offset - offset &&
               BootLoader_MatchesApp(offset, data + 4U, count) ?
               IAP_OK : IAP_BAD_STATE;
    }
    if (offset != session.next_offset) return IAP_BAD_STATE;
    if (!BootLoader_WriteApp(offset, data + 4U, count)) {
        session.active = false;
        return IAP_FLASH_ERROR;
    }
    session.next_offset += count;
    return IAP_OK;
}

static IapStatus_e IapUsb_Verify(void) {
    if (!session.active) return IAP_BAD_STATE;
    if (session.next_offset != ((session.info.app_size + 7U) & ~7U)) return IAP_BAD_STATE;
    if (!BootLoader_VectorValid(session.info.app_size)) return IAP_INVALID_APP;
    if (BootLoader_AppCrc(session.info.app_size) !=
        session.info.app_crc32) return IAP_CRC_ERROR;
    session.info.state = BOOT_STATE_VALID;
    if (BspFlash_WriteRecord(BSP_FLASH_KEY_BOOTINFO, &session.info,
                             sizeof(session.info)) != BSP_FLASH_OK) return IAP_FLASH_ERROR;
    session.active = false;
    return IAP_OK;
}

static void IapUsb_Dispatch(uint8_t command, const uint8_t *data, uint16_t length) {
    IapStatus_e result;
    switch (command) {
        case 0xA1: {
            BootInfo_s info;
            if (length != 0U) { result = IAP_BAD_LENGTH; break; }
            BspFlashResult_e status = BootLoader_ReadInfo(&info);
            if (status != BSP_FLASH_OK && status != BSP_FLASH_NOT_FOUND) {
                result = IAP_FLASH_ERROR;
                break;
            }
            IapUsb_Respond(command, IAP_OK, &info, sizeof(info));
            return;
        }
        case 0xA2: result = IapUsb_Erase(data, length); break;
        case 0xA3: result = IapUsb_Write(data, length); break;
        case 0xA4: result = length == 0U ? IapUsb_Verify() : IAP_BAD_LENGTH; break;
        case 0xA5:
            result = length != 0U ? IAP_BAD_LENGTH :
                     (BootLoader_IsBootable() ? IAP_OK : IAP_INVALID_APP);
            jump_pending = result == IAP_OK;
            jump_tick = HAL_GetTick();
            break;
        default: result = IAP_UNKNOWN_COMMAND; break;
    }
    IapUsb_Respond(command, result, NULL, 0U);
}

void IapUsb_Init(bool stay) {
    memset(&session, 0, sizeof(session));
    jump_pending = false;
    was_ready = false;
    receiving = false;
    frame_used = 0U;
    usb_session = 0U;
    stay_in_boot = stay;
    boot_tick = HAL_GetTick();
}

void IapUsb_Poll(void) {
    uint32_t now = HAL_GetTick();
    bool ready = BspUsbVcp_IsReady();
    BspUsbVcpStatus_s status;
    BspUsbVcp_GetStats(&status);
    if ((!ready && was_ready) || status.session_count != usb_session) {
        receiving = false;
        frame_used = 0U;
        session.active = false;
        jump_pending = false;
        usb_session = status.session_count;
    }
    was_ready = ready;
    if (!stay_in_boot && now - boot_tick >= IAP_BOOT_WAIT_MS) {
        BootLoader_Jump();
        stay_in_boot = true;
    }
    if (!ready) return;
    if (jump_pending) {
        if (BspUsbVcp_GetTxFree() == VCP_TX_CAPACITY) {
            jump_pending = false;
            BootLoader_Jump();
        } else if (now - jump_tick > 2000U) jump_pending = false;
        return;
    }
    if (BspUsbVcp_GetTxFree() != VCP_TX_CAPACITY) return;
    if (receiving && now - rx_tick > IAP_FRAME_TIMEOUT_MS) {
        receiving = false;
        IapUsb_Respond(frame_used > 0U ? frame[0] : 0U, IAP_BAD_FRAME, NULL, 0U);
        frame_used = 0U;
        return;
    }
    uint8_t byte;
    if (BspUsbVcp_Read(&byte, 1U) != 1U) return;
    if (!stay_in_boot) {
        if (byte == 'U' || byte == 'u') stay_in_boot = true;
        return;
    }
    if (!receiving) {
        if (byte == IAP_REQUEST_SOF) {
            receiving = true;
            frame_used = 0U;
            rx_tick = now;
        }
        return;
    }
    rx_tick = now;
    frame[frame_used++] = byte;
    if (frame_used < 3U) return;
    uint16_t length = (uint16_t)frame[1] | (uint16_t)frame[2] << 8;
    if (length > IAP_PAYLOAD_MAX) {
        receiving = false;
        IapUsb_Respond(frame[0], IAP_BAD_LENGTH, NULL, 0U);
        return;
    }
    if (frame_used < 5U + length) return;
    receiving = false;
    uint16_t crc = (uint16_t)frame[3U + length] | (uint16_t)frame[4U + length] << 8;
    if (crc != IapUsb_Crc16(frame, 3U + length)) {
        IapUsb_Respond(frame[0], IAP_CRC_ERROR, NULL, 0U);
        return;
    }
    IapUsb_Dispatch(frame[0], frame + 3U, length);
}
