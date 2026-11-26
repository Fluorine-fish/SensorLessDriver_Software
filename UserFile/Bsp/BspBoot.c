/**
*   @file BspBoot.c
*   @brief LetterShell 请求后的安全复位；不在中断中等待 USB
*   @date 2026/11/25
*/
#include "BspBoot.h"
#include "BspUsbVcp.h"
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "usbd_core.h"

static volatile bool boot_requested;
static bool outputs_stopped;
static uint32_t request_tick;
extern USBD_HandleTypeDef hUsbDeviceFS;

void BspBoot_Request(void) {
    boot_requested = true;
}

bool BspBoot_IsRequested(void) {
    return boot_requested;
}

void BspBoot_Poll(void) {
    if (!boot_requested) return;
    if (!outputs_stopped) {
        uint32_t previous = __get_PRIMASK();
        __disable_irq();
        /* 先禁止控制 ISR，防止状态机重新打开 MOE；USB/SysTick 继续工作。 */
        HAL_NVIC_DisableIRQ(ADC_IRQn);
        HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
        HAL_NVIC_DisableIRQ(TIM2_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
        TIM1->BDTR &= ~TIM_BDTR_MOE;
        TIM1->DIER = 0U;
        TIM2->DIER = 0U;
        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM2->CR1 &= ~TIM_CR1_CEN;
        ADC1->CR2 &= ~ADC_CR2_ADON;
        ADC2->CR2 &= ~ADC_CR2_ADON;
        ADC3->CR2 &= ~ADC_CR2_ADON;
        __set_PRIMASK(previous);
        outputs_stopped = true;
        request_tick = HAL_GetTick();
    }
    /* TX 入队不等于发送完成；等待最终完成，或断连/500ms 后复位。 */
    if (BspUsbVcp_IsReady() && BspUsbVcp_GetTxFree() != VCP_TX_CAPACITY &&
        HAL_GetTick() - request_tick < 500U) return;

    USBD_Stop(&hUsbDeviceFS);
    USBD_DeInit(&hUsbDeviceFS);
    HAL_Delay(20U);
    volatile uint32_t *mailbox = (volatile uint32_t *)BOOT_MAILBOX_ADDR;
    mailbox[0] = BOOT_REQUEST_MAGIC;
    mailbox[1] = ~BOOT_REQUEST_MAGIC;
    __DSB();
    NVIC_SystemReset();
}
