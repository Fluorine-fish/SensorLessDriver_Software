/**
*   @file BspTimer.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#include "BspTimer.h"
#include "tim.h"

void BspTimer_Init(void) {
    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start(&htim2);
}

void BspTimer_EnablePWM(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
}

void BspTimer_DisablePWM(void) {
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);
}

void BspTimer_SetDuty(float dutyA, float dutyB, float dutyC) {
    TIM1->CCR1 = (uint32_t)((1.f - dutyA) * TIM1->ARR);
    TIM1->CCR2 = (uint32_t)((1.f - dutyB) * TIM1->ARR);
    TIM1->CCR3 = (uint32_t)((1.f - dutyC) * TIM1->ARR);
}