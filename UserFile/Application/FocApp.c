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
#include "BspAdc.h"
#include "BspTimer.h"
#include "tim.h"
#include "string.h"

extern BspAdc_s Adc;
Foc_s Foc = {};

float debug_target_voltage = 1.8f;
float debug_target_omega_e = 200.f;
float debug_target_id = 0.0f;
float debug_target_iq = 2.0f;
float debug_current_controller_crossfreq = 1000.f;

void FocApp_Init(void) {
    BspAdc_Init();
    BspTimer_Init();
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
    FocAlg_CurrentConstruct(&Foc);

    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < -2 * PI) Foc.theta_e += 2 * PI;

    float voltage_alphabeta[2];
    FocAlg_DecoupleVoltageVector(Foc.openloop_VF_target_voltage, Foc.theta_e, voltage_alphabeta);
    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_Spwm_VFOpenLoop(void) {
    FocAlg_CurrentConstruct(&Foc);

    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < -2 * PI) Foc.theta_e += 2 * PI;

    float voltage_alphabeta[2];
    FocAlg_DecoupleVoltageVector(Foc.openloop_VF_target_voltage, Foc.theta_e, voltage_alphabeta);
    FocAlg_Spwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

static void FocApp_ControllerParamsUpdate(Foc_s *foc) {
    foc->id_controller.kp = Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls;
    foc->id_controller.ki = Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs;
    foc->id_controller.ka = 1 / foc->id_controller.kp;
    foc->id_controller.out_max = Foc.vbus * INV_SQRT3 * 0.9f;
    foc->id_controller.error_sum_max = Foc.vbus * INV_SQRT3 / foc->id_controller.ki * 0.9f;

    foc->iq_controller.kp = Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_ls;
    foc->iq_controller.ki = Foc.current_controller_crossfreq * DOUBLE_PI * Foc.motor_rs;
    foc->iq_controller.ka = 1 / foc->iq_controller.kp;
    foc->iq_controller.out_max = Foc.vbus * INV_SQRT3 * 0.9f;
    foc->iq_controller.error_sum_max = Foc.vbus * INV_SQRT3 / foc->iq_controller.ki * 0.9f;
}

void FocApp_IfOpenLoop(void) {
    FocAlg_CurrentConstruct(&Foc);

    Foc.omega_e = Foc.openloop_target_omega_e;
    Foc.theta_e += Foc.omega_e * (1 / 20000.f);
    if (Foc.theta_e > 2 * PI) Foc.theta_e -= 2 * PI;
    else if (Foc.theta_e < -2 * PI) Foc.theta_e += 2 * PI;

    float i_dq[2] = {Foc.openloop_IF_target_id, Foc.openloop_IF_target_iq};
    float voltage_dq[2], voltage_alphabeta[2];

    voltage_dq[0] = FocAlg_BackCalcPIController_Calc(&Foc.id_controller, i_dq[0], Foc.fdb_i_dq[0]);
    voltage_dq[1] = FocAlg_BackCalcPIController_Calc(&Foc.iq_controller, i_dq[1], Foc.fdb_i_dq[1]);
    FocAlg_DqVoltageVectorClamp(&Foc, voltage_dq);
    FocAlg_InvParkTrans(Foc.theta_e, voltage_dq, voltage_alphabeta);

    FocAlg_ZeroInjectSvpwm(&Foc, voltage_alphabeta);

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_DisableOutput() {
    Foc.duty[0] = 0.5f;
    Foc.duty[1] = 0.5f;
    Foc.duty[2] = 0.5f;

    BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);
}

void FocApp_UpdateLowFreqData(void) {
    BspAdc_UpdateMosTemp();
}

// 2KHz 慢环
static uint8_t slowloop_cnt = 0;
void FocApp_SlowLoop(void) {
    FocApp_ControllerParamsUpdate(&Foc);
}

// 20KHz快环
void FocApp_FastLoop(void) {
    FocApp_IfOpenLoop();
}

/*--------------状态机--------------*/

FocApp_StateMachine_s foc_state ={
    .state = APP_STATE_RESET,
    .event = APP_EVENT_RESET,
};

void State_Reset(void) {
    foc_state.state = APP_STATE_RESET;

    BspAdc_UpdateData();
    FocApp_Update();

    foc_state.event = APP_EVENT_RESET_DONE;
}

void State_Init(void) {
    if (foc_state.state == APP_STATE_FAULT) {
        BspTimer_RecoverPWM(); //没有关闭时基时不用Start函数
        Foc.align_cnt = 0; // 复位cnt重新对齐
    }

    foc_state.state = APP_STATE_INIT;

    BspAdc_UpdateData();
    FocApp_Update();

    Foc.theta_e = 0;
    Foc.omega_e = 0.f;
    Foc.motor_rs = MOTOR_RS;
    Foc.motor_ls = MOTOR_LS;
    Foc.dt = MOTOR_DT;

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

    foc_state.event = APP_EVENT_INIT_DONE;
}

void State_Ready(void) {
    foc_state.state = APP_STATE_READY;

    BspAdc_UpdateData();
    FocApp_Update();

    BspTimer_EnablePWM();

    if(Adc.vbus < OVLO && Adc.vbus > UVLO && Adc.mos_temp < OVER_TEMP)
        foc_state.event = APP_EVENT_APP_ON;
    else foc_state.event = APP_EVENT_APP_OFF;
}

void State_Calib(void) {
    foc_state.state = APP_STATE_CALIB;

    BspAdc_UpdateData();
    FocApp_Update();

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

    if(Adc.vbus > OVLO || Adc.vbus < UVLO || Adc.mos_temp > OVER_TEMP) {
        foc_state.event = APP_EVENT_FAULT;
        return;
    }

    if (Foc.align_cnt < ALIGN_CNT) {
        float u_alphabeta[2] = {ALIGN_VOLTAGE, 0.f}; // d对齐
        // float u_alphabeta[2] = {0.f, ALIGN_VOLTAGE}; // q对齐

        FocAlg_CurrentConstruct(&Foc);
        FocAlg_ZeroInjectSvpwm(&Foc, u_alphabeta);

        BspTimer_SetDuty(Foc.duty[0], Foc.duty[1], Foc.duty[2]);

        Foc.align_cnt++;
        foc_state.event = APP_EVENT_ALIGN;
    }else {
        foc_state.event = APP_EVENT_ALIGN_DONE;
    }
}

void State_Run(void) {
    foc_state.state = APP_STATE_RUN;

    BspAdc_UpdateData();
    FocApp_Update();

    if(Adc.vbus > OVLO || Adc.vbus < UVLO || Adc.mos_temp > OVER_TEMP) {
        foc_state.event = APP_EVENT_FAULT;
        return;
    }


    FocApp_FastLoop();

    if (slowloop_cnt == 0) {
        FocApp_SlowLoop();
    }
    slowloop_cnt = (slowloop_cnt == 19) ? 0 : slowloop_cnt + 1;

    foc_state.event = APP_EVENT_RUN;
}

static uint32_t fault_cnt = 0;
void State_Fault(void) {
    foc_state.state = APP_STATE_FAULT;

    BspAdc_UpdateData();
    FocApp_Update();

    // 延时防止反复跳跃
    if((Adc.vbus < OVLO && Adc.vbus > UVLO && Adc.mos_temp < OVER_TEMP)) {
        if (fault_cnt < 1000) {
            fault_cnt++;
        }else {
            foc_state.event = APP_EVENT_FAULT_CLEAR;
            fault_cnt = 0;
            return;
        }
    }else {
        if (fault_cnt < 1000) fault_cnt++;
    }

    FocApp_DisableOutput();
    BspTimer_DisablePWM();

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
