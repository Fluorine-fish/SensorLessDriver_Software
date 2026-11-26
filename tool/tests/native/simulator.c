/* 本机故障注入：编译实际 BspFlash / IapUsb，替换硬件端口。 */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "BspFlash.h"
#include "BootLoader.h"
#include "BspUsbVcp.h"

static uint8_t flash_log[128U * 1024U];
static uint8_t flash_app[864U * 1024U];
#undef BSP_FLASH_LOG_ADDR
#define BSP_FLASH_LOG_ADDR ((uintptr_t)flash_log)

static uint32_t tick, generation, irq_context;
static int words_until_failure = -1;
static bool connected, erase_fail, program_fail;
static unsigned erase_count, jump_count;
static uint8_t rx[4096], tx[4096];
static size_t rx_head, rx_tail, tx_length;

uint32_t __get_IPSR(void) { return irq_context; }
uint32_t __get_PRIMASK(void) { return 0; }
uint32_t __get_BASEPRI(void) { return 0; }
void FLASH_FlushCaches(void) {}
uint32_t HAL_GetTick(void) { return tick; }
HAL_StatusTypeDef HAL_FLASH_Unlock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Lock(void) { return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type, uintptr_t address, uint64_t value) {
    (void)type;
    assert(address >= (uintptr_t)flash_log && address + 4 <= (uintptr_t)flash_log + sizeof(flash_log));
    if (words_until_failure == 0) return HAL_ERROR;
    if (words_until_failure > 0) --words_until_failure;
    uint32_t old, word = (uint32_t)value;
    memcpy(&old, (void *)address, 4);
    assert((old & word) == word);
    memcpy((void *)address, &word, 4);
    return HAL_OK;
}

#include "../../../UserFile/Bsp/BspFlash.c"

bool BspUsbVcp_IsReady(void) { return connected; }
size_t BspUsbVcp_GetTxFree(void) { return connected ? VCP_TX_CAPACITY - tx_length : 0; }
void BspUsbVcp_GetStats(BspUsbVcpStatus_s *out) {
    memset(out, 0, sizeof(*out));
    out->session_count = generation;
}
size_t BspUsbVcp_Read(uint8_t *out, size_t capacity) {
    size_t count = rx_head - rx_tail;
    if (count > capacity) count = capacity;
    memcpy(out, rx + rx_tail, count);
    rx_tail += count;
    return count;
}
size_t BspUsbVcp_Write(const uint8_t *data, size_t length) {
    assert(tx_length + length <= sizeof(tx));
    memcpy(tx + tx_length, data, length);
    tx_length += length;
    return length;
}
BspFlashResult_e BootLoader_ReadInfo(BootInfo_s *info) {
    uint32_t length;
    memset(info, 0xFF, sizeof(*info));
    BspFlashResult_e result = BspFlash_ReadRecord(1, info, sizeof(*info), &length);
    if (result == BSP_FLASH_OK && length != sizeof(*info)) return BSP_FLASH_CORRUPT;
    return result;
}
bool BootLoader_VectorValid(uint32_t size) {
    uint32_t stack, reset;
    memcpy(&stack, flash_app, 4);
    memcpy(&reset, flash_app + 4, 4);
    return size >= 8 && size <= sizeof(flash_app) && stack > 0x20000000 &&
           stack <= 0x2001FFF8 && stack % 8 == 0 && (reset & 1) &&
           (reset & ~1U) >= 0x08008008 && (reset & ~1U) < 0x08008000 + size;
}
bool BootLoader_IsBootable(void) {
    BootInfo_s info;
    return BootLoader_ReadInfo(&info) == BSP_FLASH_OK && info.state == BOOT_STATE_VALID &&
           BootLoader_VectorValid(info.app_size) &&
           BspFlash_Crc32(flash_app, info.app_size) == info.app_crc32;
}
void BootLoader_Jump(void) { ++jump_count; }
bool BootLoader_EraseApp(void) {
    ++erase_count;
    if (erase_fail) return false;
    memset(flash_app, 0xFF, sizeof(flash_app));
    return true;
}
bool BootLoader_WriteApp(uint32_t offset, const uint8_t *data, uint32_t length) {
    assert(offset <= sizeof(flash_app) && length <= sizeof(flash_app) - offset);
    if (program_fail) return false;
    for (uint32_t i = 0; i < length; ++i) {
        assert((flash_app[offset + i] & data[i]) == data[i]);
        flash_app[offset + i] = data[i];
    }
    return true;
}

uint32_t BootLoader_AppCrc(uint32_t length) { return BspFlash_Crc32(flash_app, length); }
bool BootLoader_MatchesApp(uint32_t offset, const uint8_t *data, uint32_t length) {
    return offset <= sizeof(flash_app) && length <= sizeof(flash_app) - offset &&
           memcmp(flash_app + offset, data, length) == 0;
}

#include "../../../BootLoader/UserFile/IapUsb.c"

void SimReset(int erase) {
    if (erase) {
        memset(flash_log, 0xFF, sizeof(flash_log));
        memset(flash_app, 0xFF, sizeof(flash_app));
    }
    tick = 0; generation = 1; irq_context = 0;
    rx_head = rx_tail = tx_length = 0;
    words_until_failure = -1;
    connected = true; erase_fail = program_fail = false;
    erase_count = jump_count = 0;
    IapUsb_Init(true);
    IapUsb_Poll();
}
void SimReceive(const uint8_t *data, uint32_t length) {
    assert(rx_tail == rx_head && length <= sizeof(rx));
    memcpy(rx, data, length);
    rx_tail = 0; rx_head = length;
    for (uint32_t i = 0; i <= length; ++i) IapUsb_Poll();
}
uint32_t SimRead(uint8_t *out) {
    uint32_t count = tx_length;
    memcpy(out, tx, count);
    tx_length = 0;
    IapUsb_Poll();
    return count;
}
void SimAdvance(uint32_t ms) { tick += ms; IapUsb_Poll(); }
void SimReconnect(void) {
    connected = false; IapUsb_Poll();
    connected = true; ++generation; rx_head = rx_tail = tx_length = 0;
    IapUsb_Poll();
}
void SimWaitBoot(void) { IapUsb_Init(false); IapUsb_Poll(); }
void SimPowerCut(int words) { words_until_failure = words; }
void SimIrq(int value) { irq_context = value; }
void SimFlashError(int value) { erase_fail = program_fail = value; }
uint32_t SimErases(void) { return erase_count; }
uint32_t SimJumps(void) { return jump_count; }
void SimCorrupt(uint32_t offset) { assert(offset < sizeof(flash_log)); flash_log[offset] ^= 1; }
void SimExport(uint8_t *out) { memcpy(out, flash_log, sizeof(flash_log)); }
void SimImport(const uint8_t *in) { memcpy(flash_log, in, sizeof(flash_log)); }
