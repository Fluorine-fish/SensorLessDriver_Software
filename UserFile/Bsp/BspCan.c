/**
*   @file BspCan.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/18
*   @version 1.0
*   @note
*/
#include "BspCan.h"
#include "BspDwt.h"
#include <string.h>
#include "stdbool.h"

BspCan_s Can1 = {.is_inited = false};

static bool CAN_FilterInit() {
    CAN_FilterTypeDef filter = {};

    filter.FilterActivation = CAN_FILTER_ENABLE;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterBank = 0;
    filter.SlaveStartFilterBank = 14;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;

    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK) {
        return false;
    }

    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return false;
    };

    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        return false;
    }

    return true;
}

bool BspCan_Init(BspCan_s *can, CAN_HandleTypeDef *hcan, uint32_t tx_id, uint32_t rx_id, uint16_t time_out) {
    if (can == NULL) return false;

    memset(&can->rx_buf, 0, sizeof(can->rx_buf));
    memset(&can->tx_buf, 0, sizeof(can->tx_buf));
    memset(&can->tHeader, 0, sizeof(can->tHeader));
    memset(&can->rHeader, 0, sizeof(can->rHeader));
    can->hcan = hcan;
    can->tx_id = tx_id;
    can->rx_id = rx_id;

    can->tHeader = (CAN_TxHeaderTypeDef){
        .StdId = can->tx_id,
        .ExtId = 0x00,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .DLC = 0x08,
        .TransmitGlobalTime = DISABLE
    };

    can->daemon_cnt = 0;
    can->max_daemon_cnt = time_out;
    can->is_time_out = true;

    can->is_inited = true;

    return CAN_FilterInit();
}

static volatile uint32_t debug_can_fail_cnt = 0;
static volatile uint32_t debug_can_mail1_cnt =0;
static volatile uint32_t debug_can_mail2_cnt =0;
static volatile uint32_t debug_can_mail3_cnt =0;

static volatile uint32_t debug_can_call_cnt = 0;
static volatile float debug_can_interval_us = 0.f;
static volatile float debug_can_interval_max_us = 0.f;
// 1 kHz 调用预期为 1000 us，留出抖动余量；可在调试器中调整。
static volatile float debug_can_block_threshold_us = 1100.f;
static volatile uint32_t debug_can_block_cnt = 0;

bool BspCan_Transmit(BspCan_s *can, uint8_t *data) {
    static uint32_t last_entry_count = 0;
    static bool timestamp_valid = false;
    const uint32_t entry_count = BspDwt_GetCount();

    debug_can_call_cnt++;
    if (timestamp_valid) {
        const float interval_us = BspDwt_GetPeriod(last_entry_count, entry_count);
        debug_can_interval_us = interval_us;
        if (interval_us > debug_can_interval_max_us) {
            debug_can_interval_max_us = interval_us;
        }
        // 统计入口间隔超限次数，不等同于函数内部阻塞或 CAN 丢帧数量。
        if (interval_us > debug_can_block_threshold_us) {
            debug_can_block_cnt++;
        }
    }
    last_entry_count = entry_count;
    timestamp_valid = true;

    if (can == NULL) return false;
    if (can->is_inited == false) return false;

    memcpy(can->tx_buf, data, sizeof(can->tx_buf));
    can->tHeader.StdId = can->tx_id;

    uint32_t mailbox = CAN_TX_MAILBOX0;
    if (HAL_CAN_AddTxMessage(can->hcan, &can->tHeader, can->tx_buf, &mailbox) != HAL_OK) {
        mailbox = CAN_TX_MAILBOX1;
        if (HAL_CAN_AddTxMessage(can->hcan, &can->tHeader, can->tx_buf, &mailbox) != HAL_OK) {
            mailbox = CAN_TX_MAILBOX2;
            if (HAL_CAN_AddTxMessage(can->hcan, &can->tHeader, can->tx_buf, &mailbox) != HAL_OK) {
                debug_can_fail_cnt ++;
                return false;
            }else {
                debug_can_mail3_cnt ++;
            }
        }else {
            debug_can_mail2_cnt ++;
        }
    }else {
        debug_can_mail1_cnt ++;
    }


    return true;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData);

    if (rxHeader.StdId == Can1.rx_id) {
        memcpy(&Can1.rHeader, &rxHeader, sizeof(CAN_RxHeaderTypeDef));
        memcpy(Can1.rx_buf, rxData, sizeof(rxData));

        BspCan_Daemon_FeedDog(&Can1);
    }
}

void BspCan_Daemon_Update(BspCan_s *can) {
    if (can == NULL) return;

    can->daemon_cnt = (can->daemon_cnt > can->max_daemon_cnt) ?
        can->max_daemon_cnt : can->daemon_cnt + 1;

    if (can->daemon_cnt == can->max_daemon_cnt) {
        can->is_time_out = true;
    }

}

void BspCan_Daemon_FeedDog(BspCan_s *can) {
    if (can == NULL) return;

    if (can->is_time_out) can->is_time_out = false;
    can->daemon_cnt = 0;
}

