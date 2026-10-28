/**
*   @file BspAdc.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPADC_H
#define VESC_F4_BSPADC_H

#include "stdint.h"

typedef struct {
    uint32_t adc1_raw[4];
    uint32_t adc2_raw[4];
}BspAdc_RawData_t;

typedef struct {
    BspAdc_RawData_t raw_data;
    float phase_current[3];
    float phase_voltage[3];
    float vbus;
    float mos_temp;
}BspAdc_s;

void BspAdc_Init(void);
void BspAdc_UpdateData(void);

#endif //VESC_F4_BSPADC_H
