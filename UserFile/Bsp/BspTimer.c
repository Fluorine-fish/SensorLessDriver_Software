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
    HAL_TIM_Base_Start_IT(&htim2);
}

void BspTimer_EnablePWM(void) {
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
}

void BspTimer_RecoverPWM(void) {
    __HAL_TIM_MOE_ENABLE(&htim1);
}

void BspTimer_DisablePWM(void) {
    // 不要使用HAL的PWM_OFF函数，那个会关掉时基单元，状态机就死了
    __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);
}

void BspTimer_SetDuty(float dutyA, float dutyB, float dutyC) {
    TIM1->CCR1 = (uint32_t)((1.f - dutyA) * TIM1->ARR);
    TIM1->CCR2 = (uint32_t)((1.f - dutyB) * TIM1->ARR);
    TIM1->CCR3 = (uint32_t)((1.f - dutyC) * TIM1->ARR);
}