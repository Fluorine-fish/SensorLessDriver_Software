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
#include "stdbool.h"

typedef struct {
    uint16_t current_raw[3];
    uint32_t current_adc_offset[3];
    uint16_t calib_cnt;
    uint16_t phase_voltage_raw[3];
    uint16_t vbus_raw;
    uint16_t mos_temp_raw;
}BspAdc_RawData_t;

typedef struct {
    BspAdc_RawData_t raw_data;
    float phase_current[3];
    float phase_voltage[3];
    float vbus;
    float mos_temp;

    bool is_calibrated;
}BspAdc_s;

void BspAdc_Init(void);
void BspAdc_UpdateData(void);
void BspAdc_UpdateMosTemp(void);

#endif //VESC_F4_BSPADC_H
