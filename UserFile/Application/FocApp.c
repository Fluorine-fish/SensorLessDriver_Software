/**
*   @file FocApp.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/10/19
*   @version 1.0
*   @note
*/
#include "FocApp.h"
#include "FocAlg.h"
#include "FocComm.h"
#include "BspDwt.h"
#include "BspAdc.h"
#include "BspTimer.h"
#include "BspLed.h"
#include "BspCan.h"
#include "string.h"
#include "math.h"

extern BspAdc_s Adc;
extern BspCan_s Can1;
Foc_s Foc = {};

float debug_target_voltage = 1.6f;
float debug_target_omega_e = 200.f;
float debug_target_id = 0.0f;
float debug_target_iq = 0.4f;
float debug_current_controller_crossfreq = 1200.f;
float debug_closeloop_target_rpm = 1000.f;

FocAlg_TargetRamper_s iq_ramper = {};

void FocApp_BspInit(void) {
    BspDwt_Init();
    BspAdc_Init();
    BspTimer_Init();
    BspCan_Init(&Can1, &hcan1, 0x203, 0x200, CAN_TIMEOUT_MS);
}

static uint8_t encoder_cali_cnt = 0;
static void FocApp_EncoderCalibrate() {
    FocBsp_BlueLedON();
    FocBsp_GreenLedOFF();

    Foc.theta_e += ENCODER_CALIB_OMEGA * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < 0) Foc.theta_e += 2 * PI;

    float voltage_alphabeta[2];
    FocAlg_DecoupleVoltageVector(ENCODER_CALIB_OPENLOOP_VOLTAGE, Foc.theta_e, voltage_alphabeta);
    FocAlg_Spwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);

    if (encoder_cali_cnt == 9) {
        FocAlg_HallEncoder_Calib(&Foc.encoder, Adc.raw_data.encoder_raw, Foc.theta_e);
    }
    encoder_cali_cnt = (encoder_cali_cnt > 9) ? 0 : encoder_cali_cnt + 1;
}

static void FocApp_EncoderUpdate() {
    FocAlg_HallEncoder_Calc(&Foc.encoder, Adc.raw_data.encoder_raw);
    // 当前编码器标定和PLL使用电角度，PLL输出实际为电角速度
    Foc.fdb_omega_e = Foc.encoder.pll.omega_e_hat;
    Foc.fdb_omega_m = Foc.fdb_omega_e / MOTOR_POLE_PAIRS;
    Foc.fdb_rpm = Foc.fdb_omega_m / DOUBLE_PI * 60.f;
    Foc.fdb_erpm = Foc.fdb_omega_e / DOUBLE_PI * 60.f;
    float fdb_theta_e = Foc.encoder.pll.theta_e_hat;
    while (fdb_theta_e < 0 || fdb_theta_e > DOUBLE_PI) {
        if (fdb_theta_e < 0) fdb_theta_e += DOUBLE_PI;
        else if (fdb_theta_e > DOUBLE_PI) fdb_theta_e -= DOUBLE_PI;
    }
    Foc.fdb_theta_e = FocMath_FastAtan(Foc.encoder.sin, Foc.encoder.cos);
    Foc.fdb_theta_m += Foc.encoder.pll.theta_e_delta / MOTOR_POLE_PAIRS;
    if (Foc.fdb_theta_m >= DOUBLE_PI) Foc.fdb_theta_m -= DOUBLE_PI;
    else if (Foc.fdb_theta_m < 0) Foc.fdb_theta_m += DOUBLE_PI;
}

void FocApp_DisableOutput() {
    Foc.duty[0] = 0.5f;
    Foc.duty[1] = 0.5f;
    Foc.duty[2] = 0.5f;

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_Update() {
    // update feedback
    Foc.vbus = Adc.vbus;
    Foc.mos_temp = Adc.mos_temp;
    memcpy(&Foc.phase_current, &Adc.phase_current, sizeof(float) * 3);
    memcpy(&Foc.phase_voltage, &Adc.phase_voltage, sizeof(float) * 3);

    // update target
    Foc.openloop_target_omega_e = debug_target_omega_e;
    Foc.openloop_VF_target_voltage = debug_target_voltage;
    Foc.openloop_IF_target_id = debug_target_id;
    Foc.openloop_IF_target_iq = debug_target_iq;
    Foc.current_controller_crossfreq = debug_current_controller_crossfreq;
}

void FocApp_InjectSvpwm_VFOpenLoop(void) {
    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < 0) Foc.theta_e += 2 * PI;

    float voltage_alphabeta[2];
    FocAlg_DecoupleVoltageVector(Foc.openloop_VF_target_voltage, Foc.theta_e, voltage_alphabeta);
    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_Spwm_VFOpenLoop(void) {
    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < 0) Foc.theta_e += 2 * PI;

    float voltage_alphabeta[2];
    FocAlg_DecoupleVoltageVector(Foc.openloop_VF_target_voltage, Foc.theta_e, voltage_alphabeta);
    FocAlg_Spwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

static void FocApp_ControllerParamsUpdate(Foc_s *foc) {
    float kp = foc->current_controller_crossfreq * DOUBLE_PI * foc->motor_ls;
    float ki = foc->current_controller_crossfreq * DOUBLE_PI * foc->motor_rs;
    if (foc->vbus <= 0.f || kp <= 0.f || ki <= 0.f) return;

    foc->id_controller.kp = kp;
    foc->id_controller.ki = ki;
    foc->id_controller.ka = 1 / foc->id_controller.kp;
    foc->id_controller.out_max = foc->vbus * INV_SQRT3 * 0.9f;
    foc->id_controller.error_sum_max = foc->id_controller.out_max / foc->id_controller.ki;

    foc->iq_controller.kp = kp;
    foc->iq_controller.ki = ki;
    foc->iq_controller.ka = 1 / foc->iq_controller.kp;
    foc->iq_controller.out_max = foc->vbus * INV_SQRT3 * 0.9f;
    foc->iq_controller.error_sum_max = foc->iq_controller.out_max / foc->iq_controller.ki;
}

void FocApp_IfOpenLoop(void) {
    FocAlg_VoltageCurrentConstructOpenLoop(&Foc);

    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < 0) Foc.theta_e += 2 * PI;

    float i_dq[2] = {Foc.openloop_IF_target_id, Foc.openloop_IF_target_iq};
    float voltage_dq[2], voltage_alphabeta[2];

    voltage_dq[0] = FocAlg_BackCalcPIController_Calc(&Foc.id_controller, i_dq[0], Foc.fdb_i_dq[0]);
    voltage_dq[1] = FocAlg_BackCalcPIController_Calc(&Foc.iq_controller, i_dq[1], Foc.fdb_i_dq[1]);
    FocAlg_DqCurrentCrossCouplingFeedforward(Foc.omega_e, Foc.motor_ls, Foc.motor_ls,
        Foc.fdb_i_dq, voltage_dq);
    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

static void FocApp_ZeroTorque() {
    FocAlg_ClarkeTrans(Foc.phase_current, Foc.fdb_i_alphabeta);
    FocAlg_ClarkeTrans(Foc.phase_voltage, Foc.fdb_u_alphabeta);

    FocApp_EncoderUpdate();

    float voltage_dq[2], voltage_alphabeta[2];
    FocAlg_ParkTrans(Foc.fdb_theta_e, Foc.fdb_i_alphabeta, Foc.fdb_i_dq);

    Foc.target_i_dq[0] = 0.f;
    Foc.target_i_dq[1] = 0.f;

    voltage_dq[0] = FocAlg_BackCalcPIController_Calc(&Foc.id_controller, Foc.target_i_dq[0], Foc.fdb_i_dq[0]);
    voltage_dq[1] = FocAlg_BackCalcPIController_Calc(&Foc.iq_controller, Foc.target_i_dq[1], Foc.fdb_i_dq[1]);
    FocAlg_DqCurrentCrossCouplingFeedforward(Foc.fdb_omega_e, Foc.motor_ls, Foc.motor_ls,
        Foc.fdb_i_dq, voltage_dq);

    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.fdb_theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

static void FocApp_TorqueControl(float iq) {
    FocAlg_ClarkeTrans(Foc.phase_current, Foc.fdb_i_alphabeta);
    FocAlg_ClarkeTrans(Foc.phase_voltage, Foc.fdb_u_alphabeta);

    FocApp_EncoderUpdate();

    float voltage_dq[2], voltage_alphabeta[2];
    FocAlg_ParkTrans(Foc.fdb_theta_e, Foc.fdb_i_alphabeta, Foc.fdb_i_dq);

    Foc.target_i_dq[0] = 0.f;
    Foc.target_i_dq[1] = FocAlg_TargetRamper_Calc(&iq_ramper, Foc.cmd_target.current);

    voltage_dq[0] = FocAlg_BackCalcPIController_Calc(&Foc.id_controller, Foc.target_i_dq[0], Foc.fdb_i_dq[0]);
    voltage_dq[1] = FocAlg_BackCalcPIController_Calc(&Foc.iq_controller, Foc.target_i_dq[1], Foc.fdb_i_dq[1]);
    FocAlg_DqCurrentCrossCouplingFeedforward(Foc.fdb_omega_e, Foc.motor_ls, Foc.motor_ls,
        Foc.fdb_i_dq, voltage_dq);

    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.fdb_theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

static uint8_t foc_boot_finished = 0;
static void FocApp_BeepStartUpCharm() {
    FocAlg_ClarkeTrans(Foc.phase_current, Foc.fdb_i_alphabeta);
    FocAlg_ClarkeTrans(Foc.phase_voltage, Foc.fdb_u_alphabeta);

    FocApp_EncoderUpdate();

    float voltage_dq[2], voltage_alphabeta[2];
    FocAlg_ParkTrans(Foc.fdb_theta_e, Foc.fdb_i_alphabeta, Foc.fdb_i_dq);

    voltage_dq[0] = 0.0f;
    voltage_dq[1] = FocApp_Hfi_BootChime(&foc_boot_finished, MOTOR_DT);

    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.fdb_theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.fdb_theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_UpdateLowFreqData(void) {
    BspAdc_UpdateMosTemp();

    if (foc_boot_finished) {
        FocBsp_BlueLedOFF();
        FocBsp_GreenLedBlink(3, 0.001f);

        BspCan_Daemon_Update(&Can1);

        FocComm_DecodeCmd(&Foc, Can1.rx_buf);
        FocComm_Transmit(&Foc);
    }
}

// 1KHz 慢环
static uint8_t slowloop_cnt = 0;
void FocApp_SlowLoop(void) {
    Foc.target_i_dq[0] = 0.f;
    Foc.target_i_dq[1] = 0.f;

    if (!foc_boot_finished) {
        Foc.openloop_cnt = 0;
        Foc.is_velocity_controlled = false;
    } else if (Foc.is_velocity_controlled) {
        float target_rpm = FocMath_Clamp(debug_closeloop_target_rpm, -MAX_MECH_RPM, MAX_MECH_RPM);
        float target_omega_e = target_rpm / 60.f * DOUBLE_PI * Foc.motor_polepairs;

        // Foc.target_i_dq[1] = FocAlg_BackCalcPIController_Calc(&Foc.velocity_controller,
        //     FocAlg_TargetRamper_Calc(&Foc.velocity_target_ramper, target_omega_e), Foc.fdb_omega_e);
    }else {
        if (Foc.openloop_cnt < OPENLOOP_CNT) Foc.openloop_cnt++;
        else Foc.is_velocity_controlled = true;
    }

    FocApp_ControllerParamsUpdate(&Foc);
}

uint16_t delay_cnt = 0;

/*-------------命令状态机--------------*/

FocApp_RunStateMachine_s foc_run_state ={
    .state = APP_RUN_SELFTEST,
    .event = APP_RUN_EVENT_SELFTEST,
};

void Run_State_Selftest(void) {
    foc_run_state.state = APP_RUN_SELFTEST;

    if (!Foc.encoder.is_calibrated) {
        FocBsp_BlueLedON();
        FocBsp_GreenLedOFF();

        FocApp_EncoderCalibrate();

        foc_run_state.event = APP_RUN_EVENT_SELFTEST;
    }else if (!foc_boot_finished) {
        FocApp_BeepStartUpCharm();

        foc_run_state.event = APP_RUN_EVENT_SELFTEST;
    }else {
        foc_run_state.event = APP_RUN_EVENT_SELFTEST_DONE;
    }
}

void Run_State_Standby(void) {
    foc_run_state.state = APP_RUN_STBY;

    FocApp_ZeroTorque();

    if (Can1.is_time_out) {
        foc_run_state.event = APP_RUN_EVENT_STBY;
    }
    else {
        foc_run_state.event = APP_RUN_EVENT_RECV_CMD;
    }
}

void Run_State_Active(void) {
    foc_run_state.state = APP_RUN_ACTIVE;

    if (Can1.is_time_out) {
        FocAlg_TargetRamper_Clear(&iq_ramper);
        FocApp_ZeroTorque();

        foc_run_state.event = APP_RUN_EVENT_CMD_TIMEOUT;
    }
    else {
        if (slowloop_cnt == 0) {
            FocApp_SlowLoop();
        }
        slowloop_cnt = (slowloop_cnt > 19) ? 0 : slowloop_cnt + 1;

        FocApp_TorqueControl(Foc.cmd_target.current);

        foc_run_state.event = APP_RUN_EVNET_ACTIVE;
    }
}

// 任意异常跳转都会回到Run_State_Standby进入失能状态
static StateFunc const runStateFuncTable[APP_RUN_EVENT_COUNT][APP_RUN_STATE_COUNT] = {
    /* Current State:          SELFTEST        STBY         ACTIVE */
    /* APP_RUN_EVENT_SELFTEST      */ {Run_State_Selftest, Run_State_Standby, Run_State_Standby},
    /* APP_RUN_EVENT_SELFTEST_DONE */ {Run_State_Standby,  Run_State_Standby, Run_State_Standby},
    /* APP_RUN_EVENT_RECV_CMD      */ {Run_State_Standby,  Run_State_Active,  Run_State_Standby},
    /* APP_RUN_EVENT_STBY          */ {Run_State_Standby,  Run_State_Standby, Run_State_Standby},
    /* APP_RUN_EVNET_ACTIVE        */ {Run_State_Standby,  Run_State_Standby, Run_State_Active},
    /* APP_RUN_EVENT_CMD_TIMEOUT   */ {Run_State_Standby,  Run_State_Standby, Run_State_Standby},
};

static void Foc_RunStateMachineRuntime(void) {
    runStateFuncTable[foc_run_state.event][foc_run_state.state]();
}

/*--------------主状态机--------------*/

FocApp_StateMachine_s foc_state ={
    .state = APP_STATE_RESET,
    .event = APP_EVENT_RESET,
};

void State_Reset(void) {
    foc_state.state = APP_STATE_RESET;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_GreenLedON();
    FocBsp_BlueLedON();

    foc_state.event = APP_EVENT_RESET_DONE;
}

void State_Init(void) {
    // 重制RUN状态机到STBY
    if (foc_state.state == APP_STATE_FAULT || foc_state.state == APP_STATE_RUN) {
        foc_run_state.state = APP_RUN_STBY;
        foc_run_state.event = APP_RUN_EVENT_STBY;
    }

    if (foc_state.state == APP_STATE_FAULT) {
        BspTimer_RecoverPWM(); //没有关闭时基时不用Start函数
        Foc.align_cnt = 0; // 复位cnt重新对齐
    }

    foc_state.state = APP_STATE_INIT;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_BlueLedON();
    FocBsp_GreenLedOFF();

    Foc.cmd_target.current = 0;
    Foc.cmd_target.rpm = 0;

    Foc.theta_e = 0;
    Foc.omega_e = 0.f;
    Foc.fdb_theta_m = 0.f;
    foc_boot_finished = 0;
    Foc.is_velocity_controlled = false;
    Foc.openloop_cnt = 0;
    Foc.motor_rs = MOTOR_RS;
    Foc.motor_ls = MOTOR_LS;
    Foc.motor_polepairs = MOTOR_POLE_PAIRS;
    Foc.dt = MOTOR_DT;

    FocAlg_HallEncoder_Init(&Foc.encoder,
        2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI,
        PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI,
        (1 / (2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI)),
        MOTOR_DT,
        MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS,
        ((MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS) / (PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI)));

    FocAlg_BackCalcPIController_Init(&Foc.id_controller,
        Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls,
        Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs,
        (1 / (Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls)),
        Foc.dt, Foc.vbus * INV_SQRT3 * 0.9f,
        Foc.vbus * INV_SQRT3 * 0.9f / (Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs));
    FocAlg_BackCalcPIController_Init(&Foc.iq_controller,
        Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls,
        Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs,
        (1 / (Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls)),
        Foc.dt, Foc.vbus * INV_SQRT3 * 0.9f,
    Foc.vbus * INV_SQRT3 * 0.9f / (Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs));
    FocAlg_BackCalcPIController_Init(&Foc.velocity_controller,
        0.005f, 0.02f, 1.0f / 0.05f, SPEED_LOOP_DT, 10.0f, 10.0f / 0.05f);
    FocAlg_TargetRamper_Init(&Foc.velocity_target_ramper,MAX_MECH_ACC_RPM_PER_S / 60.f * DOUBLE_PI * Foc.motor_polepairs,
        SPEED_LOOP_DT, 0.f);

    FocAlg_PllEEMF_Init(&Foc.pll_eemf_observer, Foc.motor_rs, Foc.motor_ls, Foc.dt,
        2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI, PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI,
        (1 / (2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI)), MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS,
        ((MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS) / (PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI)), // Imax保证稳态纯I输出也有OutMax的输出
        (1 - expf(-DOUBLE_PI * PLL_OMEGA_FILET_FREQ * MOTOR_DT)));
    FocAlg_SmoEEMF_Init(&Foc.smo_eemf_observer, Foc.motor_rs, Foc.motor_ls, Foc.dt,
        8.6f, 4.f);
    FocAlg_PllSmoEEMF_Init(&Foc.pll_smo_eemf_observer, Foc.motor_rs, Foc.motor_ls, Foc.dt, 8.6f, 4.f,
        2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI, PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI,
        (1 / (2 * PLL_ZETA * PLL_FREQ * DOUBLE_PI)), MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS,
        ((MAX_MECH_RPM / 60 * DOUBLE_PI * MOTOR_POLE_PAIRS) / (PLL_FREQ * PLL_FREQ * DOUBLE_PI * DOUBLE_PI))); // Imax保证稳态纯I输出也有OutMax的输出)

    FocAlg_FirstOrderLowPassFilter_Init(&Foc.speed_filter,
        SPEED_FILTER_CUTOFF_FREQ, SPEED_LOOP_DT);

    FocAlg_TargetRamper_Init(&iq_ramper, IQ_TARGET_RAMP_RATE, MOTOR_DT, 0);

    foc_state.event = APP_EVENT_INIT_DONE;
}

void State_Ready(void) {
    foc_state.state = APP_STATE_READY;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_BlueLedON();
    FocBsp_GreenLedOFF();

    BspTimer_EnablePWM();

    if(Adc.vbus < OVLO && Adc.vbus > UVLO && Adc.mos_temp < OVER_TEMP) {
        FocApp_ControllerParamsUpdate(&Foc);
        foc_state.event = APP_EVENT_APP_ON;
    } else {
        foc_state.event = APP_EVENT_APP_OFF;
    }
}

void State_Calib(void) {
    foc_state.state = APP_STATE_CALIB;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_BlueLedON();
    FocBsp_GreenLedOFF();

    if(Adc.vbus > OVLO || Adc.vbus < UVLO || Adc.mos_temp > OVER_TEMP) {
        foc_state.event = APP_EVENT_FAULT;
        return;
    }

    if (Adc.is_calibrated) {
        foc_state.event = APP_EVENT_CALIB_DONE;
        return;
    }else {
        FocApp_DisableOutput();

        if (Adc.raw_data.calib_cnt > CALIB_CNT) {
            Adc.raw_data.current_adc_offset[0] = (uint16_t)((float)Adc.raw_data.current_adc_offset[0] / CALIB_CNT);
            Adc.raw_data.current_adc_offset[1] = (uint16_t)((float)Adc.raw_data.current_adc_offset[1] / CALIB_CNT);
            Adc.raw_data.current_adc_offset[2] = (uint16_t)((float)Adc.raw_data.current_adc_offset[2] / CALIB_CNT);

            Adc.is_calibrated = true;

            foc_state.event = APP_EVENT_CALIB_DONE;
        }else {
            Adc.raw_data.current_adc_offset[0] += Adc.raw_data.current_raw[0];
            Adc.raw_data.current_adc_offset[1] += Adc.raw_data.current_raw[1];
            Adc.raw_data.current_adc_offset[2] += Adc.raw_data.current_raw[2];

            Adc.raw_data.calib_cnt++;

            foc_state.event = APP_EVENT_CALIB;
        }
    }
}

void State_Align(void) {
    foc_state.state = APP_STATE_ALIGN;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_BlueLedON();
    FocBsp_GreenLedOFF();

    if(Adc.vbus > OVLO || Adc.vbus < UVLO || Adc.mos_temp > OVER_TEMP) {
        foc_state.event = APP_EVENT_FAULT;
        return;
    }

    if (Foc.align_cnt < ALIGN_CNT) {
        float u_alphabeta[2] = {ALIGN_VOLTAGE, 0.f}; // d对齐
        // float u_alphabeta[2] = {0.f, ALIGN_VOLTAGE}; // q对齐

        FocAlg_VoltageCurrentConstructOpenLoop(&Foc);
        FocAlg_ZeroInjectSvpwm(&Foc, u_alphabeta);

        BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);

        Foc.align_cnt++;
        foc_state.event = APP_EVENT_ALIGN;
    }else {
        foc_state.event = APP_EVENT_ALIGN_DONE;
    }
}

static uint16_t over_current_cnt = 0;
void State_Run(void) {
    foc_state.state = APP_STATE_RUN;

    BspAdc_UpdateData();
    FocApp_Update();

    // 过流保护
    if (foc_boot_finished && (FOCMATH_ABS(Foc.fdb_i_dq[0]) > OVER_CURRENT || FOCMATH_ABS(Foc.fdb_i_dq[1]) > OVER_CURRENT)) {
        over_current_cnt ++;
    }else if (over_current_cnt > 0) {
        over_current_cnt -= 10;  // 防止间歇性过流未恢复就清除计数器
    }
    if(Adc.vbus > OVLO || Adc.vbus < UVLO || Adc.mos_temp > OVER_TEMP
        || Foc.encoder.error_code != 0
        || Foc.fdb_rpm > MAX_MECH_RPM
        || over_current_cnt > 10000) {
        foc_state.event = APP_EVENT_FAULT;
        return;
    }

    Foc_RunStateMachineRuntime();

    foc_state.event = APP_EVENT_RUN;
}

static uint32_t fault_cnt = 0;
void State_Fault(void) {
    foc_state.state = APP_STATE_FAULT;

    BspAdc_UpdateData();
    FocApp_Update();

    FocBsp_BlueLedOFF();
    FocBsp_GreenLedOFF();

    Foc.cmd_target.current = 0;
    Foc.cmd_target.rpm = 0;
    Foc.target_i_dq[0] = 0.f;
    Foc.target_i_dq[1] = 0.f;

    if (Foc.encoder.error_code == 0) {
        FocAlg_HallEncoder_Calc(&Foc.encoder, Adc.raw_data.encoder_raw);
        // 当前编码器标定和PLL使用电角度，PLL输出实际为电角速度
        Foc.fdb_omega_e = Foc.encoder.pll.omega_e_hat;
        Foc.fdb_omega_m = Foc.fdb_omega_e / MOTOR_POLE_PAIRS;
        Foc.fdb_rpm = Foc.fdb_omega_m / DOUBLE_PI * 60.f;
        Foc.fdb_erpm = Foc.fdb_omega_e / DOUBLE_PI * 60.f;
        float fdb_theta_e = Foc.encoder.pll.theta_e_hat;
        while (fdb_theta_e < 0 || fdb_theta_e > DOUBLE_PI) {
            if (fdb_theta_e < 0) fdb_theta_e += DOUBLE_PI;
            else if (fdb_theta_e > DOUBLE_PI) fdb_theta_e -= DOUBLE_PI;
        }
        Foc.fdb_theta_e = FocMath_FastAtan(Foc.encoder.sin, Foc.encoder.cos);
        Foc.fdb_theta_m += Foc.encoder.pll.theta_e_delta / MOTOR_POLE_PAIRS;
        if (Foc.fdb_theta_m >= DOUBLE_PI) Foc.fdb_theta_m -= DOUBLE_PI;
        else if (Foc.fdb_theta_m < 0) Foc.fdb_theta_m += DOUBLE_PI;

        FocAlg_ParkTrans(Foc.fdb_theta_e, Foc.fdb_i_alphabeta, Foc.fdb_i_dq);
    }

    // 延时防止反复跳跃
    if(Adc.vbus < OVLO && Adc.vbus > UVLO && Adc.mos_temp < OVER_TEMP
        && Foc.encoder.error_code == 0 && Foc.fdb_rpm < MAX_MECH_RPM
        && !(FOCMATH_ABS(Foc.fdb_i_dq[0]) > OVER_CURRENT || FOCMATH_ABS(Foc.fdb_i_dq[1]) > OVER_CURRENT)) {
        if (fault_cnt < 20000) {
            fault_cnt++;
        }else {
            foc_state.event = APP_EVENT_FAULT_CLEAR;
            fault_cnt = 0;
            over_current_cnt = 0;
            return;
        }
    }else {
        if (fault_cnt < 1000) fault_cnt++;
    }

    if (Foc.encoder.error_code != 0U) {
        float voltage_dq[2] = {0.0f, FocAlg_Hfi_AlarmChime(MOTOR_DT)};
        float voltage_alphabeta[2];

        FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
        FocAlg_InvParkTrans(0.0f, voltage_dq, voltage_alphabeta);
        FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

        BspTimer_RecoverPWM();
        BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
    } else {
        FocApp_DisableOutput();
        BspTimer_DisablePWM();
    }

    foc_state.event = APP_EVENT_FAULT;
}

static StateFunc const stateFuncTable[APP_EVENT_COUNT][APP_STATE_COUNT] = {
    /* Current State:          RESET        INIT         READY        CALIB        ALIGN        RUN          FAULT */
    /* APP_EVENT_FAULT       */ {State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_FAULT_CLEAR */ {State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Init},
    /* APP_EVENT_RESET       */ {State_Reset, State_Reset, State_Reset, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_RESET_DONE  */ {State_Init,  State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_INIT        */ {State_Fault, State_Init,  State_Fault, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_INIT_DONE   */ {State_Fault, State_Ready, State_Fault, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_APP_OFF     */ {State_Fault, State_Fault, State_Ready, State_Init,  State_Init,  State_Init,  State_Fault},
    /* APP_EVENT_READY       */ {State_Fault, State_Fault, State_Ready, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_APP_ON      */ {State_Fault, State_Fault, State_Calib, State_Fault, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_CALIB       */ {State_Fault, State_Fault, State_Fault, State_Calib, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_CALIB_DONE  */ {State_Fault, State_Fault, State_Fault, State_Align, State_Fault, State_Fault, State_Fault},
    /* APP_EVENT_ALIGN       */ {State_Fault, State_Fault, State_Fault, State_Fault, State_Align, State_Fault, State_Fault},
    /* APP_EVENT_ALIGN_DONE  */ {State_Fault, State_Fault, State_Fault, State_Fault, State_Run,   State_Fault, State_Fault},
    /* APP_EVENT_RUN         */ {State_Fault, State_Fault, State_Fault, State_Fault, State_Fault, State_Run,   State_Fault},
    /* APP_EVENT_FLASH_ON    */ {State_Fault, State_Fault, State_Run,   State_Fault, State_Fault, State_Fault, State_Fault}
};

void Foc_StateMachineRuntime(void) {
    stateFuncTable[foc_state.event][foc_state.state]();
}
