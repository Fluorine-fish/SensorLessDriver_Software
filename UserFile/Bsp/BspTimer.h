/**
*   @file BspTimer.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPTIMER_H
#define VESC_F4_BSPTIMER_H

void BspTimer_Init(void);
void BspTimer_EnablePWM(void);
void BspTimer_RecoverPWM(void);
void BspTimer_DisablePWM(void);
void BspTimer_SetDuty(float dutyA, float dutyB, float dutyC);

#endif //VESC_F4_BSPTIMER_H
