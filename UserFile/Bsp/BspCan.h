/**
*   @file BspCan.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/18
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPCAN_H
#define VESC_F4_BSPCAN_H

#include "can.h"
#include "stdbool.h"

typedef struct {
    CAN_HandleTypeDef *hcan;
    CAN_TxHeaderTypeDef tHeader;
    uint8_t *Data;
}BspCan_TxFrame_s;

typedef struct {
    CAN_HandleTypeDef *hcan;

    uint8_t rx_data[8];
}BspCan_RxFrame_s;

typedef struct {
    CAN_HandleTypeDef *hcan;

    CAN_TxHeaderTypeDef tHeader;
    CAN_RxHeaderTypeDef rHeader;
    uint8_t rx_buf[8];
    uint8_t tx_buf[8];
    uint32_t tx_id;
    uint32_t rx_id;

    uint16_t daemon_cnt;
    uint16_t max_daemon_cnt; // ms
    bool is_time_out;

    bool is_inited;
}BspCan_s;

bool BspCan_Init(BspCan_s *can, CAN_HandleTypeDef *hcan, uint32_t tx_id, uint32_t rx_id, uint16_t timeout);
bool BspCan_Transmit(BspCan_s *can, uint8_t *data);
void BspCan_Daemon_Update(BspCan_s *can);
void BspCan_Daemon_FeedDog(BspCan_s *can);

#endif //VESC_F4_BSPCAN_H
