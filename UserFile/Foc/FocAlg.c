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

void FocAlg_CurrentConstruct(Foc_s *foc, float* phase_current) {
    FocAlg_ClarkeTrans(phase_current, foc->i_alphabeta);
    FocAlg_ParkTrans(foc->theta_e, foc->i_alphabeta, foc->i_dq);
}
