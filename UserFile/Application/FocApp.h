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
#define CALIB_CNT 10000.f
#define ENCODER_CALIB_OMEGA 40.f
#define ENCODER_CALIB_OPENLOOP_VOLTAGE 0.9f
#define CURRENT_CONTROLLER_CROSS_FREQ 1000.f // Hz

#define PLL_ZETA 0.707f
#define PLL_FREQ 50.f                         // Hz
#define PLL_OMEGA_FILET_FREQ 100.f            // Hz

#define SPEED_LOOP_FREQ 1000.f                // Hz
#define SPEED_LOOP_DT (1.f / SPEED_LOOP_FREQ)
#define SPEED_FILTER_CUTOFF_FREQ 50.f // Hz

typedef void (*StateFunc)(void);

typedef enum {
    APP_RUN_SELFTEST = 0,
    APP_RUN_STBY = 1,
    APP_RUN_ACTIVE = 2,
    APP_RUN_STATE_COUNT
} FocApp_Run_State_s;

typedef enum {
    APP_RUN_EVENT_SELFTEST = 0,
    APP_RUN_EVENT_SELFTEST_DONE = 1,
    APP_RUN_EVENT_RECV_CMD = 2,
    APP_RUN_EVENT_STBY = 3,
    APP_RUN_EVNET_ACTIVE = 4,
    APP_RUN_EVENT_CMD_TIMEOUT = 5,
    APP_RUN_EVENT_COUNT
}FocApp_Run_Event_s;

typedef struct {
    FocApp_Run_State_s state;
    FocApp_Run_Event_s event;
}FocApp_RunStateMachine_s;

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

void FocApp_BspInit(void);
void FocApp_UpdateLowFreqData(void);
void Foc_StateMachineRuntime(void);

#endif //VESC_F4_FOCAPP_H
