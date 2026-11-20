/**
*   @file FocComm.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/18
*   @version 1.0
*   @note
*/
#include "FocComm.h"
#include "BspCan.h"

extern BspCan_s Can1;

void FocComm_DecodeCmd(Foc_s* foc, uint8_t *cmd) {
    if (Can1.is_time_out) {
        foc->cmd_target.current = 0;
        foc->cmd_target.rpm = 0;
    }else {
        int16_t tmp_current = (int16_t)((cmd[4] << 8) | (cmd[5]));
        if (tmp_current > 10000) tmp_current = 10000;
        else if (tmp_current < -10000) tmp_current = -10000;

        foc->cmd_target.current = ((float)tmp_current / 1000.f);
        foc->cmd_target.rpm = 0;
    }
}

void FocComm_Transmit(Foc_s* foc) {
    uint8_t buf[8] = {0};

    float theta_normalized = foc->fdb_theta_m / DOUBLE_PI;
    theta_normalized -= (float)(int)theta_normalized;
    if (theta_normalized < 0.f) theta_normalized += 1.f;

    uint16_t theta_raw = (uint16_t)(theta_normalized * 8192.f);
    if (theta_raw > 8191U) theta_raw = 8191U;

    int16_t rpm_raw = (int16_t)FocMath_Clamp(foc->fdb_rpm, -32768.f, 32767.f);

    float current_a = FocMath_Clamp(foc->fdb_i_dq[1], -10.f, 10.f);
    int16_t torque_current_raw = (int16_t)(current_a * 1000.f);

    buf[0] = (uint8_t)((uint16_t)theta_raw >> 8);
    buf[1] = (uint8_t)theta_raw;

    buf[2] = (uint8_t)((uint16_t)rpm_raw >> 8);
    buf[3] = (uint8_t)rpm_raw;

    buf[4] = (uint8_t)((uint16_t)torque_current_raw >> 8);
    buf[5] = (uint8_t)torque_current_raw;

    BspCan_Transmit(&Can1, buf);
}
