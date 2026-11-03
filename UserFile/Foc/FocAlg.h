/**
*   @file FocAlg.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/29
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_FOCALG_H
#define VESC_F4_FOCALG_H

#include "FocMath.h"
#include "stdint.h"
#include "stdbool.h"

#define MIN_DUTY 0.02f
#define MAX_DUTY 0.98f
#define ALIGN_VOLTAGE 0.8f
#define ALIGN_CNT 5000
#define OPENLOOP_CNT 500

typedef struct {
    float kp;
    float ki;
    float ka; // 建议Ka = 1/Kp
    float dt;
    float out_max;
    float error_sum_max;

    float target;
    float fdb;
    float error;

    float error_sum;
    float k_out;
    float i_out;
    float out;
    float delta_out;
}FocAlg_Pid_s;

typedef struct {
    uint8_t length;
    bool is_full;

    float buffer[30];
    uint8_t cnt;
    float average;
}FocAlg_MovingAverageFilter_s;

typedef struct {
    float alpha;
    bool is_init;

    float out;
    float last_out;
}FocAlg_FirstOrderLowPassFilter_s;

typedef struct {
    float slope;
    float dt;

    float target;
    float out;
    float last_out;
}FocAlg_TargetRamper_s;

typedef struct {
    float rs;
    float ls;
    float ts;

    float u_alphabeta[2];
    float i_alphabeta[2];
    float last_i_alphabeta[2];

    float theta_e_hat;
}FocAlg_DirectEEMF_s;

typedef struct {
    float rs;
    float ls;
    float ts;
    float kp;
    float ki;
    float ka; // 建议Ka = 1/Kp
    float omega_filter_k; // 速度一阶低通滤波器参数
    float out_max;
    float error_sum_max;

    float error_sum;
    float u_alphabeta[2];
    float i_alphabeta[2];
    float last_i_alphabeta[2];
    float theta_e_raw;
    float out_delta;

    float omega_e_raw;
    float omega_m_raw; // m法测速
    float last_theta_e_hat;
    float p_out;
    float i_out;
    FocAlg_MovingAverageFilter_s speed_filter;

    float theta_e_hat;
    float omega_e_hat;
}FocAlg_PllEEMF_s;

typedef struct {
    float phase_current[3];
    float phase_voltage[3];
    float vbus;
    float mos_temp;

    uint16_t align_cnt;

    float motor_rs;
    float motor_ls;
    float motor_polepairs;
    float dt;
    float theta_e;
    float omega_e;
    float duty[3];
    float fdb_u_alphabeta[2];
    float fdb_i_alphabeta[2];
    float fdb_i_dq[2];
    float fdb_omega_e;
    float fdb_theta_e;
    float target_i_dq[2];
    FocAlg_Pid_s id_controller;
    FocAlg_Pid_s iq_controller;
    FocAlg_Pid_s velocity_controller;
    FocAlg_TargetRamper_s velocity_target_ramper;
    FocAlg_DirectEEMF_s eemf_observer;
    FocAlg_PllEEMF_s pll_eemf_observer;
    FocAlg_FirstOrderLowPassFilter_s speed_filter;

    bool is_velocity_controlled;
    uint16_t openloop_cnt;

    float openloop_VF_target_voltage;
    float openloop_target_omega_e;
    float openloop_IF_target_id;
    float openloop_IF_target_iq;
    float current_controller_crossfreq;
    float closeloop_target_omega_e;
}Foc_s;

void FocAlg_DecoupleVoltageVector(float model, float theta, float* alphabeta);
void FocAlg_ClarkeTrans(float* input, float* output);
void FocAlg_ParkTrans(float theta_e, float* input, float* output);
void FocAlg_DqzTrans(float theta_e, float* input, float* output);
void FocAlg_InvParkTrans(float theta_e, float* input, float* output);
void FocAlg_InvClarkeTrans(float* input, float* output);
void FocAlg_DqVoltageVectorClamp(Foc_s *foc, float* input);

void FocAlg_VoltageCurrentConstruct(Foc_s *foc);
void FocAlg_ZeroInjectSvpwm(Foc_s *foc, float* u_alphabeta);
void FocAlg_Spwm(Foc_s *foc, float* u_alphabeta);

// PID
void FocAlg_BackCalcPIController_Init(FocAlg_Pid_s *pid,
        float kp, float ki, float ka, float dt, float out_max, float i_max);
float FocAlg_BackCalcPIController_Calc(FocAlg_Pid_s *pid, float target, float fdb);
void FocAlg_TargetRamper_Init(FocAlg_TargetRamper_s* ramper, float slope, float dt, float initial_target);
float FocAlg_TargetRamper_Calc(FocAlg_TargetRamper_s* ramper, float target);

// EEMF
void FocAlg_DirectEEMF_Init(FocAlg_DirectEEMF_s* eemf,
        float rs, float ls, float ts);
float FocAlg_DirectEEMF_Calc(FocAlg_DirectEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta);
void FocAlg_PllEEMF_Init(FocAlg_PllEEMF_s* eemf,
    float rs, float ls, float ts, float kp, float ki, float ka, float out_max, float i_max, float omega_filter_k);
void FocAlg_PllEEMF_Calc(FocAlg_PllEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta);

// Filter
void FocAlg_MovingAverageFilter_Init(FocAlg_MovingAverageFilter_s* filter, uint8_t length);
float FocAlg_MovingAverageFilter_Calc(FocAlg_MovingAverageFilter_s* filter, float input);
void FocAlg_FirstOrderLowPassFilter_Init(FocAlg_FirstOrderLowPassFilter_s* filter, float cut_off_freq, float ts);
float FocAlg_FirstOrderLowPassFilter_Calc(FocAlg_FirstOrderLowPassFilter_s* filter, float input);

#endif //VESC_F4_FOCALG_H
