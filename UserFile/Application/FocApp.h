/**
*   @file FocApp.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_FOCAPP_H
#define VESC_F4_FOCAPP_H

#define PI 3.1415926f

/* 设定值常数定义 */
#define UVLO 18.f
#define OVLO 26.f
#define MIN_DUTY 0.02
#define MAX_DUTY 0.98
#define CALIB_CNT 10000.f

void FocApp_Init(void);
void FocApp_FastLoop(void);

#endif //VESC_F4_FOCAPP_H
