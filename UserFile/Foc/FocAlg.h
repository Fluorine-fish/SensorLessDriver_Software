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

#define MIN_DUTY 0.02f
#define MAX_DUTY 0.98f
#define ALIGN_VOLTAGE 0.8f
#define ALIGN_CNT 5000

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
    float fdb_i_alphabeta[2];
    float fdb_i_dq[2];
    FocAlg_Pid_s id_controller;
    FocAlg_Pid_s iq_controller;

    float openloop_VF_target_voltage;
    float openloop_target_omega_e;
    float openloop_IF_target_id;
    float openloop_IF_target_iq;
    float current_controller_crossfreq;
}Foc_s;

void FocAlg_DecoupleVoltageVector(float model, float theta, float* alphabeta);
void FocAlg_ClarkeTrans(float* input, float* output);
void FocAlg_ParkTrans(float theta_e, float* input, float* output);
void FocAlg_DqzTrans(float theta_e, float* input, float* output);
void FocAlg_InvParkTrans(float theta_e, float* input, float* output);
void FocAlg_InvClarkeTrans(float* input, float* output);
void FocAlg_DqVoltageVectorClamp(Foc_s *foc, float* input);

void FocAlg_CurrentConstruct(Foc_s *foc);
void FocAlg_ZeroInjectSvpwm(Foc_s *foc, float* u_alphabeta);
void FocAlg_Spwm(Foc_s *foc, float* u_alphabeta);

void FocAlg_BackCalcPIController_Init(FocAlg_Pid_s *pid,
        float kp, float ki, float ka, float dt, float out_max, float i_max);
float FocAlg_BackCalcPIController_Calc(FocAlg_Pid_s *pid, float target, float fdb);

#endif //VESC_F4_FOCALG_H
