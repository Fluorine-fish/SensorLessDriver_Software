/**
*   @file BootLoader.c
*   @brief 启动校验、邮箱消费、USB 退出与 APP 跳转
*   @date 2026/11/25
*/
#include "BootLoader.h"
#include "BspUsbVcp.h"
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "usbd_core.h"
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

#ifndef BOOT_DEBUG_DIRECT_APP
#define BOOT_DEBUG_DIRECT_APP 0
#endif

static bool boot_requested;

BspFlashResult_e BootLoader_ReadInfo(BootInfo_s *info) {
    uint32_t length = 0U;
    memset(info, 0xFF, sizeof(*info));
    BspFlashResult_e result = BspFlash_ReadRecord(BSP_FLASH_KEY_BOOTINFO,
                                                info, sizeof(*info), &length);
    if (result == BSP_FLASH_OK && length != sizeof(*info)) return BSP_FLASH_CORRUPT;
    return result;
}

bool BootLoader_VectorValid(uint32_t image_size) {
    if (image_size < 8U || image_size > BOOT_APP_SIZE) return false;
    const uint32_t *vector = (const uint32_t *)BOOT_APP_ADDR;
    uint32_t stack = vector[0];
    uint32_t reset = vector[1];
    bool main_ram = stack > 0x20000000U && stack <= BOOT_MAILBOX_ADDR;
    bool ccm_ram = stack > 0x10000000U && stack <= 0x10010000U;
    return (main_ram || ccm_ram) && (stack & 7U) == 0U &&
           (reset & 1U) != 0U && (reset & ~1U) >= BOOT_APP_ADDR + 8U &&
           (reset & ~1U) < BOOT_APP_ADDR + image_size;
}

bool BootLoader_IsBootable(void) {
#if BOOT_DEBUG_DIRECT_APP
    /* 临时排障：不访问 Sector 11，也不检查 APP 向量或 CRC。 */
    return true;
#else
    BootInfo_s info;
    BspFlashResult_e result = BootLoader_ReadInfo(&info);
    if (result == BSP_FLASH_NOT_FOUND) {
        BspFlashStatus_s status;
        /* 仅全空 Sector 11 可以走首次 SWD 烧录路径；损坏或半写日志不能绕过。 */
        return BspFlash_GetStatus(&status) == BSP_FLASH_OK && status.blank &&
               BootLoader_VectorValid(BOOT_APP_SIZE);
    }
    return result == BSP_FLASH_OK && info.magic == BOOT_INFO_MAGIC &&
           info.state == BOOT_STATE_VALID && info.app_addr == BOOT_APP_ADDR &&
           BootLoader_VectorValid(info.app_size) &&
           BootLoader_AppCrc(info.app_size) == info.app_crc32;
#endif
}

/* 设置 MSP 后不再执行 C 栈访问；所有切换动作只在 Thread mode 中执行。 */
__attribute__((naked, noreturn)) static void BootLoader_Enter(uint32_t stack, uint32_t reset) {
    __asm volatile (
        "movs r2, #0\n"
        "msr control, r2\n"
        "msr basepri, r2\n"
        "msr faultmask, r2\n"
        "msr msp, r0\n"
        "dsb\n"
        "isb\n"
        "cpsie i\n"
        "bx r1\n"
    );
}

static void BootLoader_Transfer(uint32_t stack, uint32_t reset) {
    __disable_irq();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    for (uint32_t i = 0; i < 8U; ++i) {
        NVIC->ICER[i] = UINT32_MAX;
        NVIC->ICPR[i] = UINT32_MAX;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    SCB->VTOR = BOOT_APP_ADDR;
    __DSB();
    __ISB();
    BootLoader_Enter(stack, reset);
}

void BootLoader_EarlyInit(void) {
    volatile uint32_t *mailbox = (volatile uint32_t *)BOOT_MAILBOX_ADDR;
    boot_requested = __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) &&
                     mailbox[0] == BOOT_REQUEST_MAGIC && mailbox[1] == ~BOOT_REQUEST_MAGIC;
    mailbox[0] = 0U;
    mailbox[1] = 0U;
    __HAL_RCC_CLEAR_RESET_FLAGS();
#if BOOT_DEBUG_DIRECT_APP
    if (!boot_requested) {
        /* 从复位环境直接转入 APP Reset_Handler，不等待、不校验、不启动外设。 */
        uint32_t stack = *(const uint32_t *)BOOT_APP_ADDR;
        uint32_t reset = *(const uint32_t *)(BOOT_APP_ADDR + 4U);
        BootLoader_Transfer(stack, reset);
    }
#endif
}

void BootLoader_Jump(void) {
    if (!BootLoader_IsBootable()) return;
    uint32_t stack = *(const uint32_t *)BOOT_APP_ADDR;
    uint32_t reset = *(const uint32_t *)(BOOT_APP_ADDR + 4U);
    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);
    /* 给主机可见的 USB detach，再由 APP 重新枚举。 */
    HAL_Delay(20U);
    HAL_RCC_DeInit();
    HAL_DeInit();
    BootLoader_Transfer(stack, reset);
}

void BootLoader_Run(void) {
    /* GPIO 初始化已使两灯常亮；整个 BootLoader 期间不闪烁。 */
    IapUsb_Init(boot_requested || !BootLoader_IsBootable());
    while (1) {
        BspUsbVcp_Poll();
        IapUsb_Poll();
    }
}
