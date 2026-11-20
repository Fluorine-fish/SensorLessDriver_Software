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
#define UVLO 7.f
#define OVLO 26.f
#define OVER_TEMP 80.f
#define OVER_CURRENT 28.f
#define CALIB_CNT 10000.f
#define MOTOR_RS 0.2305f
#define MOTOR_LS 32.11e-6f
#define MOTOR_DT (1 / 20000.f)
#define MOTOR_POLE_PAIRS 7.f
#define ENCODER_CALIB_OMEGA 40.f
#define ENCODER_CALIB_OPENLOOP_VOLTAGE 0.9f
#define CURRENT_CONTROLLER_CROSS_FREQ 1000.f // Hz
#define PLL_ZETA 0.707f
#define PLL_FREQ 50.f                         // Hz
#define PLL_OMEGA_FILET_FREQ 100.f            // Hz
#define MAX_MECH_RPM 9000.f                   // 机械转速上限
#define MAX_MECH_ACC_RPM_PER_S 15000.f        // 机械加速度上限
#define SPEED_LOOP_FREQ 1000.f                // Hz
#define SPEED_LOOP_DT (1.f / SPEED_LOOP_FREQ)
#define SPEED_FILTER_CUTOFF_FREQ 50.f // Hz

/* 低速静摩擦补偿：仅作用于外部位置/速度环发送的 iq 目标 */
#define FRICTION_BREAKAWAY_CURRENT 3.2f       // A，低速时最小 q 电流
#define FRICTION_COMMAND_DEADBAND 0.05f       // A，低于该值视为零力矩目标
#define FRICTION_STOP_RPM 30.f                 // 机械转速低于该值视为静止
#define FRICTION_RELEASE_RPM 120.f             // 机械转速高于该值后补偿完全退出
#define IQ_TARGET_RAMP_RATE 600.f              // A/s，限制力矩目标变化率

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

void FocApp_BspInit(void);
void FocApp_UpdateLowFreqData(void);
void Foc_StateMachineRuntime(void);

#endif //VESC_F4_FOCAPP_H
