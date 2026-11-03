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
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
}

void FocBsp_GreenLedOFF() {
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
}

void FocBsp_BlueLedON() {
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
}

void FocBsp_BlueLedOFF() {
    HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
}

