/**
*   @file FocAlg.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/29
*   @version 1.0
*   @note
*/
#include "FocAlg.h"
#include <stddef.h>

// 将静止两相坐标系的矢量解出分量
void FocAlg_DecoupleVoltageVector(float model, float theta, float* alphabeta) {
    alphabeta[0] = model * FocMath_FastCos(theta);
    alphabeta[1] = model * FocMath_FastSin(theta);
}

void FocAlg_ClarkeTrans(float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = TWO_THREES * (input[0] - 0.5f * input[1] - 0.5f * input[2]);
    output[1] = TWO_THREES * (HALF_SQRT3 * input[1] - HALF_SQRT3 * input[2]);
}

void FocAlg_ParkTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e) + input[1] * FocMath_FastSin(theta_e);
    output[1] = - input[0] * FocMath_FastSin(theta_e) + input[1] * FocMath_FastCos(theta_e);
}

void FocAlg_DqzTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e)
        + input[1] * FocMath_FastCos(theta_e - TWO_THREES_PI)
        + input[2] * FocMath_FastCos(theta_e - FOUR_THREES_PI);

    output[1] = - input[0] * FocMath_FastSin(theta_e)
        - input[1] * FocMath_FastSin(theta_e - TWO_THREES_PI)
        - input[2] * FocMath_FastSin(theta_e - FOUR_THREES_PI);
}

void FocAlg_InvClarkeTrans(float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0];
    output[1] = - input[0] * 0.5f + input[1] * HALF_SQRT3;
    output[2] = - input[0] * 0.5f - input[1] * HALF_SQRT3;
}

void FocAlg_CurrentConstruct(Foc_s *foc) {
    FocAlg_ClarkeTrans(foc->phase_current, foc->i_alphabeta);
    FocAlg_ParkTrans(foc->theta_e, foc->i_alphabeta, foc->i_dq);
}

void FocAlg_ZeroInjectSvpwm(Foc_s *foc, float* u_alphabeta) {
    float outer_voltage[3] = {};

    FocAlg_InvClarkeTrans(u_alphabeta, outer_voltage);

    float min = outer_voltage[0];
    float max = outer_voltage[0];
    if (outer_voltage[1] > max) max = outer_voltage[1];
    if (outer_voltage[1] < min) min = outer_voltage[1];
    if (outer_voltage[2] > max) max = outer_voltage[2];
    if (outer_voltage[2] < min) min = outer_voltage[2];

    float u0 = - (max + min) / 2.f;
    foc->duty[0] = (u0 + outer_voltage[0]) / foc->vbus + 0.5f;
    foc->duty[1] = (u0 + outer_voltage[1]) / foc->vbus + 0.5f;
    foc->duty[2] = (u0 + outer_voltage[2]) / foc->vbus + 0.5f;

    if (foc->duty[0] > MAX_DUTY) foc->duty[0] = MAX_DUTY;
    else if (foc->duty[0] < MIN_DUTY) foc->duty[0] = MIN_DUTY;
    if (foc->duty[1] > MAX_DUTY) foc->duty[1] = MAX_DUTY;
    else if (foc->duty[1] < MIN_DUTY) foc->duty[1] = MIN_DUTY;
    if (foc->duty[2] > MAX_DUTY) foc->duty[2] = MAX_DUTY;
    else if (foc->duty[2] < MIN_DUTY) foc->duty[2] = MIN_DUTY;
}

void FocAlg_Spwm(Foc_s *foc, float* u_alphabeta) {
    float outer_voltage[3] = {};

    FocAlg_InvClarkeTrans(u_alphabeta, outer_voltage);

    foc->duty[0] = (outer_voltage[0]) / foc->vbus + 0.5f;
    foc->duty[1] = (outer_voltage[1]) / foc->vbus + 0.5f;
    foc->duty[2] = (outer_voltage[2]) / foc->vbus + 0.5f;

    if (foc->duty[0] > MAX_DUTY) foc->duty[0] = MAX_DUTY;
    else if (foc->duty[0] < MIN_DUTY) foc->duty[0] = MIN_DUTY;
    if (foc->duty[1] > MAX_DUTY) foc->duty[1] = MAX_DUTY;
    else if (foc->duty[1] < MIN_DUTY) foc->duty[1] = MIN_DUTY;
    if (foc->duty[2] > MAX_DUTY) foc->duty[2] = MAX_DUTY;
    else if (foc->duty[2] < MIN_DUTY) foc->duty[2] = MIN_DUTY;
}
