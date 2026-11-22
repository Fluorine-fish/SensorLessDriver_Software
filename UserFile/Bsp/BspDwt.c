/**
*   @file BspDwt.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/22
*   @version 1.0
*   @note
*/
#include "BspDwt.h"
#include "stdint.h"
#include "stm32f4xx_hal.h"

static uint32_t core_clock_hz;
static float us_per_cycle;

void BspDwt_Init(void) {
    core_clock_hz = HAL_RCC_GetHCLKFreq();
    us_per_cycle = 1000000.0f / (float)core_clock_hz;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __DSB();
    __ISB();
}

uint32_t BspDwt_GetCount(void) {
    return DWT->CYCCNT;
}

void BspDwt_Delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (core_clock_hz / 1000000.0f);

    while ((DWT->CYCCNT - start) < ticks) ;
}

// 返回单位为us
float BspDwt_GetPeriod(uint32_t start_count, uint32_t end_count) {
    uint32_t delta = end_count - start_count;
    return (float)delta * us_per_cycle;
}
