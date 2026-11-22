/**
*   @file BspAdc.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#include "BspAdc.h"
#include "FocApp.h"
#include "adc.h"
#include "tim.h"
#include "math.h"

BspAdc_s Adc = {};

void BspAdc_Init(void) {
    Adc.is_calibrated = false;

    HAL_ADC_Start(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADC_Start(&hadc2);
    HAL_ADCEx_InjectedStart_IT(&hadc2);
    HAL_ADC_Start(&hadc3);
    HAL_ADCEx_InjectedStart_IT(&hadc3);

    Adc.ready_flag = 0;
    Adc.frame_pending = false;
}

void BspAdc_UpdateData(void) {
    // Update Raw Data
    Adc.raw_data.current_raw[0] = hadc1.Instance->JDR1;
    Adc.raw_data.current_raw[1] = hadc2.Instance->JDR1;
    Adc.raw_data.current_raw[2] = hadc3.Instance->JDR1;
    Adc.raw_data.phase_voltage_raw[0] = hadc1.Instance->JDR2;
    Adc.raw_data.phase_voltage_raw[1] = hadc2.Instance->JDR2;
    Adc.raw_data.phase_voltage_raw[2] = hadc3.Instance->JDR2;
    Adc.raw_data.vbus_raw = hadc2.Instance->DR;
    Adc.raw_data.mos_temp_raw = hadc1.Instance->DR;
    Adc.raw_data.encoder_raw[0] = hadc1.Instance->JDR3;
    Adc.raw_data.encoder_raw[1] = hadc2.Instance->JDR3;

    // Update Adc Data
    Adc.vbus = (float)Adc.raw_data.vbus_raw / 4096.f * 3.3f * 18.73f;

    if (Adc.is_calibrated){
        Adc.phase_voltage[0] = (float)Adc.raw_data.phase_voltage_raw[0] / 4096.f * 3.3f * 18.73f;
        Adc.phase_voltage[1] = (float)Adc.raw_data.phase_voltage_raw[1] / 4096.f * 3.3f * 18.73f;
        Adc.phase_voltage[2] = (float)Adc.raw_data.phase_voltage_raw[2] / 4096.f * 3.3f * 18.73f;

        Adc.phase_current[0] = ((float)Adc.raw_data.current_raw[0] - (float)Adc.raw_data.current_adc_offset[0])
            / 4096.f * 3.3f / 0.005f / 20.f;
        Adc.phase_current[1] = ((float)Adc.raw_data.current_raw[1] - (float)Adc.raw_data.current_adc_offset[1])
            / 4096.f * 3.3f / 0.005f / 20.f;
        Adc.phase_current[2] = ((float)Adc.raw_data.current_raw[2] - (float)Adc.raw_data.current_adc_offset[2])
            / 4096.f * 3.3f / 0.005f / 20.f;
    }
}

void BspAdc_UpdateMosTemp(void) {
    float Vntc = (float)Adc.raw_data.mos_temp_raw / 4096.f * 3.3f;
    if (Vntc == 0.f) {
        Adc.mos_temp = 0.f;
        return;
    }

    // 下偏电阻为 10 kΩ，因此该值无量纲且等于 R_ntc / 10 kΩ
    float RntcRatio = 3.3f / Vntc - 1;
    if (RntcRatio < 0.0341f || RntcRatio > 34.527f) { // 约 341 Ω 至 345 kΩ
        Adc.mos_temp = 0.f;
        return;
    }

    const float c0 =  25.0057087f;
    const float c1 = -22.6733208f;
    const float c2 =   1.51452422f;
    const float c3 =  -0.12007042f;
    const float c4 =   0.0107816132f;

    float x = logf(RntcRatio);
    Adc.mos_temp = (((c4 * x + c3) * x + c2) * x + c1) * x + c0;;
}

void BspAdc_InjectedStart_Callback(void) {
    if (Adc.frame_pending) {
        Adc.missed_frame ++;
    }

    Adc.frame_pending = false;
    __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_JEOC);
    __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_JEOC);
    __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_JEOC);

    Adc.ready_flag = 0;
    Adc.frame_pending = true;
}

void BspAdc_InjectedComplete_Callback(void) {
    if (__HAL_TIM_GET_FLAG(&htim1, TIM_FLAG_UPDATE) != RESET) {
        return; // 如果已经出发下一次采样，就跳过本次控制
    }

    if (Adc.frame_pending && Adc.ready_flag == 0b111) {
        Adc.frame_pending = false;
        Adc.ready_flag = 0;

        Foc_StateMachineRuntime();
    }
}

void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) Adc.ready_flag |= 0b1;
    if (hadc->Instance == ADC2) Adc.ready_flag |= 0b10;
    if (hadc->Instance == ADC3) Adc.ready_flag |= 0b100;
}
