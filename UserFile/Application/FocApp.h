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

/* 设定值常数定义 */
#define UVLO 18.f
#define OVLO 26.f
#define OVER_TEMP 80.f
#define CALIB_CNT 10000.f
#define MOTOR_RS 1.06902f
#define MOTOR_LS 572.69e-6f
#define MOTOR_DT (1 / 20000.f)
#define CURRENT_CONTROLLER_CROSS_FREQ 1000.f

typedef void (*StateFunc)(void);

typedef enum {
    APP_STATE_RESET = 0,
    APP_STATE_INIT  = 1,
    APP_STATE_READY = 2,
    APP_STATE_CALIB = 3,
    APP_STATE_ALIGN = 4,
    APP_STATE_RUN   = 5,
    APP_STATE_FAULT = 6,
    APP_STATE_COUNT
} FocApp_State_s; /* Application state identification user type */

typedef enum {
    APP_EVENT_FAULT       = 0,
    APP_EVENT_FAULT_CLEAR = 1,
    APP_EVENT_RESET       = 2,
    APP_EVENT_RESET_DONE  = 3,
    APP_EVENT_INIT        = 4,
    APP_EVENT_INIT_DONE   = 5,
    APP_EVENT_APP_OFF     = 6,
    APP_EVENT_READY       = 7,
    APP_EVENT_APP_ON      = 8,
    APP_EVENT_CALIB       = 9,
    APP_EVENT_CALIB_DONE  = 10,
    APP_EVENT_ALIGN       = 11,
    APP_EVENT_ALIGN_DONE  = 12,
    APP_EVENT_RUN         = 13,
    APP_EVENT_FLASH_ON    = 14, /* 使用flash存储的校准信息启动 */
    APP_EVENT_COUNT
} FocApp_Event_s; /* Application event identification user type */

typedef struct {
    FocApp_State_s state;
    FocApp_Event_s event;
}FocApp_StateMachine_s;

void FocApp_Init(void);
void FocApp_UpdateLowFreqData(void);
void Foc_StateMachineRuntime(void);

#endif //VESC_F4_FOCAPP_H
