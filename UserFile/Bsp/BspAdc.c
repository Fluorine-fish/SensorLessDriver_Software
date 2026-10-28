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
    HAL_ADC_Start_IT(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADC_Start_IT(&hadc2);
    HAL_ADCEx_InjectedStart_IT(&hadc2);
    HAL_ADC_Start_IT(&hadc3);
    HAL_ADCEx_InjectedStart_IT(&hadc3);
}

void BspAdc_UpdateData(void) {
    Adc.raw_data.adc1_raw[0] = hadc1.Instance->JDR1;
    Adc.raw_data.adc1_raw[1] = hadc1.Instance->JDR2;
    Adc.raw_data.adc1_raw[2] = hadc1.Instance->JDR3;
    Adc.raw_data.adc1_raw[3] = hadc1.Instance->JDR4;

    Adc.raw_data.adc2_raw[0] = hadc1.Instance->JDR1;
    Adc.raw_data.adc2_raw[1] = hadc2.Instance->JDR1;
    Adc.raw_data.adc2_raw[2] = hadc3.Instance->JDR1;
    Adc.raw_data.adc2_raw[3] = hadc2.Instance->JDR4;
}
