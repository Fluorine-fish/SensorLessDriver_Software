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

#define MIN_DUTY 0.02
#define MAX_DUTY 0.98

typedef struct {
    float phase_current[3];
    float phase_voltage[3];
    float vbus;
    float mos_temp;

    float theta_e;
    float omega_e;
    float duty[3];
    float i_alphabeta[2];
    float i_dq[2];

    float openloop_target_voltage;
    float openloop_target_omega_e;
}Foc_s;

void FocAlg_DecoupleVoltageVector(float model, float theta, float* alphabeta);
void FocAlg_ClarkeTrans(float* input, float* output);
void FocAlg_ParkTrans(float theta_e, float* input, float* output);
void FocAlg_DqzTrans(float theta_e, float* input, float* output);
void FocAlg_InvClarkeTrans(float* input, float* output);

void FocAlg_CurrentConstruct(Foc_s *foc);
void FocAlg_ZeroInjectSvpwm(Foc_s *foc, float* u_alphabeta);
void FocAlg_Spwm(Foc_s *foc, float* u_alphabeta);

#endif //VESC_F4_FOCALG_H
