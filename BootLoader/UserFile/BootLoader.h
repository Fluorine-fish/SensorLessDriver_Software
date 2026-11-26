/**
*   @file BootLoader.h
*   @brief USB IAP 启动与 Flash 操作
*   @date 2026/11/25
*/
#ifndef VESC_F4_BOOTLOADER_H
#define VESC_F4_BOOTLOADER_H
#include "BspBoot.h"
#include "BspFlash.h"

/* main 最前面调用，在 HAL/PLL/USB 初始化之前消费复位邮箱。 */
void BootLoader_EarlyInit(void);
void BootLoader_Run(void);
bool BootLoader_VectorValid(uint32_t image_size);
bool BootLoader_IsBootable(void);
void BootLoader_Jump(void);
bool BootLoader_EraseApp(void);
uint32_t BootLoader_AppCrc(uint32_t length);
bool BootLoader_MatchesApp(uint32_t offset, const uint8_t *data, uint32_t length);
bool BootLoader_WriteApp(uint32_t offset, const uint8_t *data, uint32_t length);
BspFlashResult_e BootLoader_ReadInfo(BootInfo_s *info);

void IapUsb_Init(bool stay);
void IapUsb_Poll(void);
#endif //VESC_F4_BOOTLOADER_H
