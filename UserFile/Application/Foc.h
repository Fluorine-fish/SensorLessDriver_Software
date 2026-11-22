/**
*   @file Foc.h
*   @brief
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/

#ifndef VESC_F4_FOC_H
#define VESC_F4_FOC_H

// #define ENCODER_NEED_CALIB

#define UVLO 7.f
#define OVLO 26.f
#define OVER_TEMP 80.f
#define OVER_CURRENT 14.f

#define CAN_TIMEOUT_MS 500

#define MOTOR_RS 0.2305f
#define MOTOR_LS 32.11e-6f
#define MOTOR_POLE_PAIRS 7.f
#define MOTOR_DT (1 / 20000.f)

#define MAX_MECH_RPM 9000.f                   // 机械转速上限
#define MAX_MECH_ACC_RPM_PER_S 15000.f        // 机械加速度上限
#define IQ_TARGET_RAMP_RATE 600.f              // A/s，限制力矩目标变化率

#endif //VESC_F4_FOC_H
