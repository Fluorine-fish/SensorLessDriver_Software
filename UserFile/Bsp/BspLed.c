/**
*   @file BspLed.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/3
*   @version 1.0
*   @note
*/
#include "BspLed.h"
#include "gpio.h"

void FocBsp_GreenLedON() {
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
}

void FocBsp_GreenLedOFF() {
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
}

void FocBsp_BlueLedON() {
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
}

void FocBsp_BlueLedOFF() {
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
}

void FocBsp_GreenLedBlink(uint8_t times, float dt) {
    if (times > 8) times = 8;

    static uint16_t cnt = 0;
    static uint16_t blink_cnt = 0;
    uint16_t period = (uint16_t)(4.f / dt);
    uint16_t off_time = (uint16_t)(1.f / dt);

    if (blink_cnt < times) {
        if (cnt % (period / 8) == 0) {
            FocBsp_GreenLedON();
        }else if (cnt % (period / 8) == period / 12) {
            blink_cnt ++;
            FocBsp_GreenLedOFF();
        }
    }else {
        FocBsp_GreenLedOFF();
    }

    if (cnt > period / 8 * times + off_time) blink_cnt = 0;
    cnt = (cnt > period / 8 * times + off_time) ? 0 : cnt + 1;
}
