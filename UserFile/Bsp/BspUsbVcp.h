/**
*   @file BspUsbVcp.h
*   @brief USB FS CDC 非阻塞字节收发
*   @author Wenxin HU
*   @date 2026/11/22
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPUSBVCP_H
#define VESC_F4_BSPUSBVCP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VCP_RX_CAPACITY = 1024,
    VCP_TX_CAPACITY = 2048,
    VCP_PACKET_SIZE = 64,
    VCP_COPY_CHUNK = 64
} BspUsbVcpPacket_e;

typedef struct {
    uint32_t rx_bytes;          /* 已进入 RX 队列的字节数 */
    uint32_t tx_queued_bytes;   /* 已进入 TX 队列的字节数 */
    uint32_t tx_complete_bytes; /* USB 已完成发送，不代表主机应用已消费 */

    uint32_t rx_pause_count;    /* RX 空间不足，暂停接收次数 */
    uint32_t rx_drop_bytes;     /* 异常情况下丢弃的 RX 字节数 */
    uint32_t tx_busy_count;     /* 底层返回 USBD_BUSY 次数 */
    uint32_t tx_fail_count;     /* 底层提交失败次数 */

    uint32_t session_count;     /* CDC 配置会话次数 */
} BspUsbVcpStatus_s;

void BspUsbVcp_Init(void);
void BspUsbVcp_Poll(void);

bool BspUsbVcp_IsReady(void);
size_t BspUsbVcp_GetTxFree(void);

size_t BspUsbVcp_Read(uint8_t *dst, size_t capacity);
size_t BspUsbVcp_Write(const uint8_t *src, size_t length);

void BspUsbVcp_GetStats(BspUsbVcpStatus_s *out);

void BspUsbVcp_OnConfigured(void);
void BspUsbVcp_OnDeconfigured(void);
void BspUsbVcp_OnRx(const uint8_t *data, size_t length);
void BspUsbVcp_OnTxComplete(void);

#ifdef __cplusplus
}
#endif

#endif //VESC_F4_BSPUSBVCP_H
