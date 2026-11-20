/**
*   @file FocComm.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/18
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_FOCCOMM_H
#define VESC_F4_FOCCOMM_H

#define COMM_MOTOR_KT 0.18f

#include "stdint.h"
#include "FocAlg.h"

typedef struct{
    uint16_t current[4];
}FocComm_RxData_s;

typedef struct{
    uint16_t theta_m;
    uint16_t rpm_m;
    uint16_t torque_m;
}FocComm_TxData_s;

void FocComm_DecodeCmd(Foc_s* foc, uint8_t *cmd);
void FocComm_Transmit(Foc_s* foc);

#endif //VESC_F4_FOCCOMM_H
