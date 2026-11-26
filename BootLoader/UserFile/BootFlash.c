/**
*   @file BootFlash.c
*   @brief STM32F405 APP 区扇区擦除和 32 位写入
*   @date 2026/11/25
*/
#include "BootLoader.h"
#include "stm32f4xx_hal.h"
#include <string.h>

static void BootFlash_ClearFlags(void) {
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

bool BootLoader_EraseApp(void) {
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Banks = FLASH_BANK_1;
    erase.Sector = FLASH_SECTOR_2;
    erase.NbSectors = 9U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t sector_error = UINT32_MAX;
    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    BootFlash_ClearFlags();
    HAL_StatusTypeDef result = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();
    return result == HAL_OK && sector_error == UINT32_MAX;
}

bool BootLoader_WriteApp(uint32_t offset, const uint8_t *data, uint32_t length) {
    if (data == NULL || length == 0U || (offset & 3U) != 0U || (length & 3U) != 0U ||
        offset > BOOT_APP_SIZE || length > BOOT_APP_SIZE - offset) return false;
    if (HAL_FLASH_Unlock() != HAL_OK) return false;
    BootFlash_ClearFlags();
    HAL_StatusTypeDef result = HAL_OK;
    for (uint32_t i = 0; i < length; i += 4U) {
        uint32_t word;
        memcpy(&word, data + i, sizeof(word));
        result = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, BOOT_APP_ADDR + offset + i, word);
        if (result != HAL_OK) break;
    }
    FLASH_FlushCaches();
    HAL_FLASH_Lock();
    return result == HAL_OK && memcmp((const void *)(uintptr_t)(BOOT_APP_ADDR + offset),
                                      data, length) == 0;
}

uint32_t BootLoader_AppCrc(uint32_t length) {
    return BspFlash_Crc32((const void *)BOOT_APP_ADDR, length);
}

bool BootLoader_MatchesApp(uint32_t offset, const uint8_t *data, uint32_t length) {
    return offset <= BOOT_APP_SIZE && length <= BOOT_APP_SIZE - offset &&
           memcmp((const void *)(uintptr_t)(BOOT_APP_ADDR + offset), data, length) == 0;
}
