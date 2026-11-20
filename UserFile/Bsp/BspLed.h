/**
*   @file BspLed.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/3
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPLED_H
#define VESC_F4_BSPLED_H

#include "stdint.h"

void FocBsp_GreenLedON();
void FocBsp_GreenLedOFF();
void FocBsp_BlueLedON();
void FocBsp_BlueLedOFF();
void FocBsp_GreenLedBlink(uint8_t times, float dt);

#endif //VESC_F4_BSPLED_H
