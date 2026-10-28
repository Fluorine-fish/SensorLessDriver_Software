/**
*   @file FocApp.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#include "FocApp.h"
#include <math.h>
#include "BspAdc.h"
#include "BspTimer.h"
#include "tim.h"

#define PI 3.1415926f
#define MIN_DUTY 0.02
#define MAX_DUTY 0.98

extern BspAdc_s Adc;

float Vbus = 24.f;
float theta_e = 0.f;
float omega_e = 400.f;
float voltage_Ampere = 0.5f;

float outer_voltage[3];
float duty[3];

void FocApp_Init(void) {
    BspAdc_Init();
    BspTimer_Init();

    BspTimer_EnablePWM();
}

void FocApp_InjectSvpwm_VFOpenLoop(void) {
    theta_e += omega_e * (1 / 20000.f);
    if (theta_e > 2 * PI) theta_e -= 2 * PI;
    else if (theta_e < -2 * PI) theta_e += 2 * PI;

    outer_voltage[0] = voltage_Ampere * sinf(theta_e);
    outer_voltage[1] = voltage_Ampere * sinf(theta_e + 2 * PI / 3);
    outer_voltage[2] = voltage_Ampere * sinf(theta_e - 2 * PI / 3);

    float min = outer_voltage[0];
    float max = outer_voltage[0];
    if (outer_voltage[1] > max) max = outer_voltage[1];
    if (outer_voltage[1] < min) min = outer_voltage[1];
    if (outer_voltage[2] > max) max = outer_voltage[2];
    if (outer_voltage[2] < min) min = outer_voltage[2];

    float u0 = - (max + min) / 2.f;
    duty[0] = (u0 + outer_voltage[0]) / Vbus + 0.5f;
    duty[1] = (u0 + outer_voltage[1]) / Vbus + 0.5f;
    duty[2] = (u0 + outer_voltage[2]) / Vbus + 0.5f;

    if (duty[0] > MAX_DUTY) duty[0] = MAX_DUTY;
    else if (duty[0] < MIN_DUTY) duty[0] = MIN_DUTY;
    if (duty[1] > MAX_DUTY) duty[1] = MAX_DUTY;
    else if (duty[1] < MIN_DUTY) duty[1] = MIN_DUTY;
    if (duty[2] > MAX_DUTY) duty[2] = MAX_DUTY;
    else if (duty[2] < MIN_DUTY) duty[2] = MIN_DUTY;

    TIM1->CCR1 = (uint32_t)((1.f - duty[0]) * TIM1->ARR);
    TIM1->CCR2 = (uint32_t)((1.f - duty[1]) * TIM1->ARR);
    TIM1->CCR3 = (uint32_t)((1.f - duty[2]) * TIM1->ARR);
}

void FOCApp_Spwm_VFOpenLoop() {
    theta_e += omega_e * (1 / 20000.f);
    if (theta_e > 2 * PI) theta_e -= 2 * PI;
    else if (theta_e < -2 * PI) theta_e += 2 * PI;

    outer_voltage[0] = voltage_Ampere * sinf(theta_e);
    outer_voltage[1] = voltage_Ampere * sinf(theta_e + 2 * PI / 3);
    outer_voltage[2] = voltage_Ampere * sinf(theta_e - 2 * PI / 3);

    duty[0] = outer_voltage[0] / Vbus + 0.5f;
    duty[1] = outer_voltage[1] / Vbus + 0.5f;
    duty[2] = outer_voltage[2] / Vbus + 0.5f;

    if (duty[0] > MAX_DUTY) duty[0] = MAX_DUTY;
    else if (duty[0] < MIN_DUTY) duty[0] = MIN_DUTY;
    if (duty[1] > MAX_DUTY) duty[1] = MAX_DUTY;
    else if (duty[1] < MIN_DUTY) duty[1] = MIN_DUTY;
    if (duty[2] > MAX_DUTY) duty[2] = MAX_DUTY;
    else if (duty[2] < MIN_DUTY) duty[2] = MIN_DUTY;

    TIM1->CCR1 = (uint32_t)((1.f - duty[0]) * TIM1->ARR);
    TIM1->CCR2 = (uint32_t)((1.f - duty[1]) * TIM1->ARR);
    TIM1->CCR3 = (uint32_t)((1.f - duty[2]) * TIM1->ARR);
}

void FOCApp_DisableOutput() {
    duty[0] = 0.5f;
    duty[1] = 0.5f;
    duty[2] = 0.5f;

    TIM1->CCR1 = (uint32_t)((1.f - duty[0]) * TIM1->ARR);
    TIM1->CCR2 = (uint32_t)((1.f - duty[1]) * TIM1->ARR);
    TIM1->CCR3 = (uint32_t)((1.f - duty[2]) * TIM1->ARR);
}

// 20KHz快环
void FocApp_FastLoop(void) {
    BspAdc_UpdateData();

    if (Adc.is_calibrated) {
        FocApp_InjectSvpwm_VFOpenLoop();
    }else {
        FOCApp_DisableOutput();

        if (Adc.raw_data.calib_cnt > 5000) {
            Adc.raw_data.current_adc_offset[0] = (uint16_t)((float)Adc.raw_data.current_adc_offset[0] / 5000.f);
            Adc.raw_data.current_adc_offset[1] = (uint16_t)((float)Adc.raw_data.current_adc_offset[1] / 5000.f);
            Adc.raw_data.current_adc_offset[2] = (uint16_t)((float)Adc.raw_data.current_adc_offset[2] / 5000.f);

            Adc.is_calibrated = true;
        }else {
             Adc.raw_data.current_adc_offset[0] += Adc.raw_data.current_raw[0];
             Adc.raw_data.current_adc_offset[1] += Adc.raw_data.current_raw[1];
             Adc.raw_data.current_adc_offset[2] += Adc.raw_data.current_raw[2];

            Adc.raw_data.calib_cnt++;
        }
    }
}
