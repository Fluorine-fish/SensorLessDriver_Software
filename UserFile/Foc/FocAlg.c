/**
*   @file FocAlg.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/29
*   @version 1.0
*   @note
*/
#include "FocAlg.h"
#include "string.h"

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

void FocAlg_InvParkTrans(float theta_e, float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0] * FocMath_FastCos(theta_e) - input[1] * FocMath_FastSin(theta_e);
    output[1] = input[0] * FocMath_FastSin(theta_e) + input[1] * FocMath_FastCos(theta_e);
}

void FocAlg_InvClarkeTrans(float* input, float* output) {
    if (input == NULL || output == NULL) return;

    output[0] = input[0];
    output[1] = - input[0] * 0.5f + input[1] * HALF_SQRT3;
    output[2] = - input[0] * 0.5f - input[1] * HALF_SQRT3;
}

// 对dq下的电压矢量限幅，结合实际vbus的调制能力
void FocAlg_DqVoltageVectorClamp(Foc_s *foc, float* input) {
    if (input == NULL || foc == NULL) return;

    const float duty_range = (MAX_DUTY - MIN_DUTY);
    float max_model = duty_range * foc->vbus * duty_range  * foc->vbus / 3.f;

    if (input[0] * input[0] + input[1] * input[1] > max_model) {
        if (input[0] * input[0] > max_model) {
            input[0] = (input[0] > 0) ?  FocMath_FastSqrt(max_model) : - FocMath_FastSqrt(max_model);
            input[1] = 0.f;
        }else {
            input[1] = (input[1] > 0) ? FocMath_FastSqrt(max_model - input[0] * input[0]) : - FocMath_FastSqrt(max_model - input[0] * input[0]);
        }
    }else {
        return;
    }
}

void FocAlg_VoltageCurrentConstruct(Foc_s *foc) {
    FocAlg_ClarkeTrans(foc->phase_current, foc->fdb_i_alphabeta);
    FocAlg_ParkTrans(foc->theta_e, foc->fdb_i_alphabeta, foc->fdb_i_dq);
    FocAlg_ClarkeTrans(foc->phase_voltage, foc->fdb_u_alphabeta);
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

void FocAlg_BackCalcPIController_Init(FocAlg_Pid_s *pid,
        float kp, float ki, float ka, float dt, float out_max, float error_sum_max) {
    pid->kp = kp;
    pid->ki = ki;
    pid->ka = ka;
    pid->dt = dt;
    pid->out_max = out_max;
    pid->error_sum_max = error_sum_max;

    pid->target = 0.f;
    pid->fdb = 0.f;
    pid->error = 0.f;
    pid->error_sum = 0.f;
    pid->k_out = 0.f;
    pid->i_out = 0.f;
    pid->out = 0.f;
    pid->delta_out = 0.f;
}

float FocAlg_BackCalcPIController_Calc(FocAlg_Pid_s *pid, float target, float fdb) {
    pid->target = target;
    pid->fdb = fdb;
    pid->error = pid->target - pid->fdb;

    pid->error_sum += pid->dt * pid->error;

    pid->k_out = pid->kp * pid->error;
    pid->i_out = pid->ki * pid->error_sum;

    float out_unsat = pid->k_out + pid->i_out;
    float out_sat = FocMath_Clamp(out_unsat, -pid->out_max, pid->out_max);
    pid->delta_out = out_sat - out_unsat;
    pid->out = out_sat;

    // 反计算积分抗饱和
    pid->error_sum = pid->error_sum + pid->ka  * pid->dt * pid->delta_out;

    // 保护防止超出限幅
    pid->error_sum = FocMath_Clamp(pid->error_sum, -pid->error_sum_max, pid->error_sum_max);

    return pid->out;
}

void FocAlg_TargetRamper_Init(FocAlg_TargetRamper_s* ramper, float slope, float dt, float initial_target) {
    if (ramper == NULL) return;
    ramper->slope = slope;
    ramper->dt = dt;

    ramper->target = initial_target;
    ramper->out = initial_target;
    ramper->last_out = initial_target;
}

float FocAlg_TargetRamper_Calc(FocAlg_TargetRamper_s* ramper, float target) {
    float delta = target-ramper->last_out;

    if (FOCMATH_ABS(delta) < ramper->slope * ramper->dt ) {
        ramper->out = target;
    }else {
        ramper->out += delta / FOCMATH_ABS(delta) * ramper->slope * ramper->dt;
    }

    ramper->last_out = ramper->out;
    return ramper->out;
}

void FocAlg_DirectEEMF_Init(FocAlg_DirectEEMF_s* eemf,
        float rs, float ls, float ts) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;

    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->last_i_alphabeta, 0, sizeof(float) * 2);

    eemf->theta_e_hat = 0.f;
}

float FocAlg_DirectEEMF_Calc(FocAlg_DirectEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->last_i_alphabeta, eemf->i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);

    const float Ld_slash_Ts = eemf->ls / eemf->ts;
    float e_alpha = eemf->u_alphabeta[0] - eemf->rs * eemf->i_alphabeta[0] - Ld_slash_Ts * (eemf->i_alphabeta[0] - eemf->last_i_alphabeta[0]);
    float e_beta = eemf->u_alphabeta[1] - eemf->rs * eemf->i_alphabeta[1] - Ld_slash_Ts * (eemf->i_alphabeta[1] - eemf->last_i_alphabeta[1]);

    eemf->theta_e_hat = FocMath_FastAtan(-e_alpha, e_beta);
    return eemf->theta_e_hat;
}

void FocAlg_PllEEMF_Init(FocAlg_PllEEMF_s* eemf,
    float rs, float ls, float ts, float kp, float ki, float ka, float out_max, float i_max, float omega_filter_k) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;
    eemf->kp = kp;
    eemf->ki = ki;
    eemf->ka = ka;
    eemf->out_max = out_max;
    eemf->error_sum_max = i_max;
    eemf->omega_filter_k = omega_filter_k;
    FocAlg_MovingAverageFilter_Init(&eemf->speed_filter, 20);

    eemf->error_sum = 0;
    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->last_i_alphabeta, 0, sizeof(float) * 2);

    eemf->omega_e_raw = 0.f;
    eemf->omega_m_raw = 0.f;
    eemf->last_theta_e_hat = 0.f;
    eemf->p_out = 0.f;
    eemf->i_out = 0.f;
    eemf->theta_e_hat = 0.f;
    eemf->omega_e_hat = 0.f;
}

void FocAlg_PllEEMF_Calc(FocAlg_PllEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->last_i_alphabeta, eemf->i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);

    const float Ld_slash_Ts = eemf->ls / eemf->ts;
    float e_alpha = eemf->u_alphabeta[0] - eemf->rs * eemf->i_alphabeta[0] - Ld_slash_Ts * (eemf->i_alphabeta[0] - eemf->last_i_alphabeta[0]);
    float e_beta = eemf->u_alphabeta[1] - eemf->rs * eemf->i_alphabeta[1] - Ld_slash_Ts * (eemf->i_alphabeta[1] - eemf->last_i_alphabeta[1]);

    eemf->theta_e_raw = FocMath_FastAtan(-e_alpha, e_beta);

    // 尝试下来计算atan2得到角度再计算误差的噪声比使用sin计算噪声更小一些
    // float error = -e_alpha * FocMath_FastCos(eemf->theta_e_hat) - e_beta * FocMath_FastSin(eemf->theta_e_hat);
    float error = eemf->theta_e_raw - eemf->theta_e_hat;
    if (error > PI) error -= DOUBLE_PI;
    else if (error < - PI) error += DOUBLE_PI;

    eemf->error_sum += error * eemf->ts;
    eemf->p_out = error * eemf->kp;
    eemf->i_out = eemf->error_sum * eemf->ki;
    float omega_unsat = error * eemf->kp + eemf->error_sum * eemf->ki;
    eemf->omega_e_raw = FocMath_Clamp(omega_unsat, -eemf->out_max, eemf->out_max);
    eemf->out_delta = eemf->omega_e_raw - omega_unsat;

    // 反计算抗饱和
    eemf->error_sum = eemf->error_sum + eemf->ka * eemf->ts * eemf->out_delta;

    eemf->error_sum = FocMath_Clamp(eemf->error_sum, -eemf->error_sum_max, eemf->error_sum_max);

    // 角度积分
    eemf->theta_e_hat += eemf->omega_e_raw * eemf->ts;
    if (eemf->theta_e_hat < 0) eemf->theta_e_hat += DOUBLE_PI;
    else if (eemf->theta_e_hat > DOUBLE_PI) eemf->theta_e_hat -= DOUBLE_PI;

    // 速度滤波 滑动均值
    eemf->omega_e_hat = FocAlg_MovingAverageFilter_Calc(&eemf->speed_filter, eemf->omega_e_raw);
    eemf->last_theta_e_hat = eemf->theta_e_hat;
}

void FocAlg_SmoEEMF_Init(FocAlg_SmoEEMF_s* eemf,
        float rs, float ls, float ts, float gain_h, float e_max) {
    eemf->rs = rs;
    eemf->ls = ls;
    eemf->ts = ts;
    eemf->gain_h = gain_h;
    eemf->e_max = e_max;

    eemf->inv_ld = 1.f / eemf->ls;
    eemf->r_slash_ls = eemf->rs / eemf->ls;

    memset(eemf->u_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta, 0, sizeof(float) * 2);
    memset(eemf->i_alphabeta_hat, 0, sizeof(float) * 2);
    memset(eemf->voltage_alphabeta_hat, 0, sizeof(float) * 2);
    FocAlg_FirstOrderLowPassFilter_Init(&eemf->theta_e_filter, 4000.f, ts);

    eemf->theta_e_hat = 0.f;
}

float FocAlg_SmoEEMF_Calc(FocAlg_SmoEEMF_s* eemf,
        float* u_alphabeta, float* i_alphabeta) {
    memcpy(eemf->u_alphabeta, u_alphabeta, sizeof(float) * 2);
    memcpy(eemf->i_alphabeta, i_alphabeta, sizeof(float) * 2);

    eemf->i_alphabeta_hat[0] = eemf->i_alphabeta[0] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[0] - eemf->r_slash_ls * eemf->i_alphabeta_hat[0] - eemf->inv_ld * eemf->voltage_alphabeta_hat[0]);
    eemf->i_alphabeta_hat[1] = eemf->i_alphabeta[1] + eemf->ts *
        (eemf->inv_ld * eemf->u_alphabeta[1] - eemf->r_slash_ls * eemf->i_alphabeta_hat[1] - eemf->inv_ld * eemf->voltage_alphabeta_hat[1]);

    float err[2] = {};
    err[0] = eemf->i_alphabeta_hat[0] - eemf->i_alphabeta[0];
    err[1] = eemf->i_alphabeta_hat[1] - eemf->i_alphabeta[1];
    float sat_err[2] = {};
    if (FOCMATH_ABS(err[0]) < eemf->e_max) sat_err[0] = err[0];
    else if (err[0] < 0) sat_err[0] = -eemf->e_max;
    else sat_err[0] = eemf->e_max;
    if (FOCMATH_ABS(err[1]) < eemf->e_max) sat_err[1] = err[1];
    else if (err[1] < 0) sat_err[1] = -eemf->e_max;
    else sat_err[1] = eemf->e_max;

    eemf->voltage_alphabeta_hat[0] = eemf->gain_h * sat_err[0];
    eemf->voltage_alphabeta_hat[1] = eemf->gain_h * sat_err[1];

    eemf->theta_e_hat_raw = FocMath_FastAtan(-eemf->voltage_alphabeta_hat[0], eemf->voltage_alphabeta_hat[1]);

    eemf->theta_e_hat = FocAlg_FirstOrderLowPassFilter_AngleCalc(&eemf->theta_e_filter, eemf->theta_e_hat_raw);
}

void FocAlg_MovingAverageFilter_Init(FocAlg_MovingAverageFilter_s* filter, uint8_t length) {
    filter->length = length;
    if (length == 0) filter->length = 1;
    if (length > 30) filter->length = 30;

    filter->is_full = false;
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->cnt = 0;
    filter->average = 0.f;
}

float FocAlg_MovingAverageFilter_Calc(FocAlg_MovingAverageFilter_s* filter, float input) {
    float old_sample = filter->buffer[filter->cnt % filter->length];
    filter->buffer[filter->cnt % filter->length] = input;
    if (filter->cnt >= filter->length && filter->is_full == false) filter->is_full = true;
    filter->cnt = (filter->cnt >= filter->length) ? 0 : filter->cnt + 1;

    if (filter->is_full) {
        filter->average += (input - old_sample) / (float)filter->length;
        return filter->average;
    }else {
        filter->average = (filter->average * ((float)filter->cnt - 1) + input) / (float)filter->cnt;
        return filter->average;
    }
}

void FocAlg_FirstOrderLowPassFilter_Init(FocAlg_FirstOrderLowPassFilter_s* filter, float cut_off_freq, float ts) {
    if (filter == NULL) return;
    filter->alpha = cut_off_freq * DOUBLE_PI * ts / (ts + 1);

    filter->out = 0.f;
    filter->last_out = 0.f;
}

float FocAlg_FirstOrderLowPassFilter_Calc(FocAlg_FirstOrderLowPassFilter_s* filter, float input) {
    if (!filter->is_init) {
        filter->out = input;
        filter->last_out = input;
        filter->is_init = true;

        return filter->out;
    }else {
        filter->out = (1 - filter->alpha) * filter->last_out + filter->alpha * input;
        filter->last_out = filter->out;
        return filter->out;
    }
}

static float FocAlg_WrapPi(float angle) {
    if (angle > PI) {
        angle -= DOUBLE_PI;
    } else if (angle < -PI) {
        angle += DOUBLE_PI;
    }
    return angle;
}

static float FocAlg_Wrap2Pi(float angle) {
    if (angle >= DOUBLE_PI) {
        angle -= DOUBLE_PI;
    } else if (angle < 0.0f) {
        angle += DOUBLE_PI;
    }
    return angle;
}

float FocAlg_FirstOrderLowPassFilter_AngleCalc(FocAlg_FirstOrderLowPassFilter_s* filter, float input) {
    input = FocAlg_Wrap2Pi(input);

    if (!filter->is_init) {
        filter->out = input;
        filter->last_out = input;
        filter->is_init = true;

        return filter->out;
    }

    float error = FocAlg_WrapPi(input - filter->last_out);
    float output = filter->last_out + filter->alpha * error;

    output = FocAlg_Wrap2Pi(output);

    filter->out = output;
    filter->last_out = output;
    return output;
}

