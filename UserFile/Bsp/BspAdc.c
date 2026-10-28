/**
*   @file BspAdc.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#include "BspAdc.h"
#include "adc.h"

BspAdc_s Adc = {};

void BspAdc_Init(void) {
    Adc.is_calibrated = false;

    HAL_ADC_Start_IT(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADC_Start_IT(&hadc2);
    HAL_ADCEx_InjectedStart_IT(&hadc2);
    HAL_ADC_Start_IT(&hadc3);
    HAL_ADCEx_InjectedStart_IT(&hadc3);
}

void BspAdc_UpdateData(void) {
    // Update Raw Data
    Adc.raw_data.current_raw[0] = hadc1.Instance->JDR2;
    Adc.raw_data.current_raw[1] = hadc1.Instance->JDR3;
    Adc.raw_data.current_raw[2] = hadc1.Instance->JDR4;
    Adc.raw_data.phase_voltage_raw[0] = hadc1.Instance->JDR1;
    Adc.raw_data.phase_voltage_raw[1] = hadc2.Instance->JDR1;
    Adc.raw_data.phase_voltage_raw[2] = hadc3.Instance->JDR1;
    Adc.raw_data.vbus_raw = hadc1.Instance->DR;
    Adc.raw_data.mos_temp_raw = hadc2.Instance->DR;

    // Update Adc Data
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
