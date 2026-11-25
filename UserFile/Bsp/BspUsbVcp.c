/**
*   @file BspUsbVcp.c
*   @brief USB FS CDC 静态队列、RX 背压与异步 TX 管理
*   @author Wenxin HU
*   @date 2026/11/22
*   @version 1.0
*   @note
*/
#include "BspUsbVcp.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <string.h>

extern USBD_HandleTypeDef hUsbDeviceFS;

_Static_assert(VCP_PACKET_SIZE == CDC_DATA_FS_MAX_PACKET_SIZE, "Require USB FS CDC");
_Static_assert(VCP_RX_CAPACITY >= VCP_PACKET_SIZE, "RX too small");
_Static_assert(VCP_TX_CAPACITY >= VCP_PACKET_SIZE, "TX too small");
_Static_assert(VCP_COPY_CHUNK > 0, "Copy chunk must be positive");

/* 队列属于本模块，不向应用暴露可修改的索引。 */
typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t head; /* 下次写入位置 */
    size_t tail; /* 下次读出位置 */
    size_t used;
} BspVcp_Ring_s;

static uint8_t rx_storage[VCP_RX_CAPACITY];
static uint8_t tx_storage[VCP_TX_CAPACITY];
static uint8_t tx_inflight[VCP_PACKET_SIZE];

static BspVcp_Ring_s rx_ring = {
    .data = rx_storage,
    .capacity = sizeof(rx_storage)
};

static BspVcp_Ring_s tx_ring = {
    .data = tx_storage,
    .capacity = sizeof(tx_storage)
};

static bool initialized = false;
static bool configured = false;
static bool rx_paused = false;
static bool rx_fault = false;
static bool tx_active = false;
static bool tx_done = false;
static uint16_t tx_inflight_len;
static uint32_t session_generation;
static BspUsbVcpStatus_s status;

static size_t BspVcp_Min(size_t a, size_t b) {
    return a < b ? a : b;
}

/* 当前分组为 NVIC_PRIORITYGROUP_4：TIM2=1，CAN=2，USB=3，FOC ADC/TIM1=0。
 * 主循环和所有 USB 回调均使用同一 BASEPRI 临界区，防止 TIM2 Shell
 * 抢占 USB 队列更新。优先级 0 的 FOC 快环仍可运行。
 * 只有主循环可以调用 Poll/USB 提交；优先级 0 ISR 不得调用本模块。 */
static uint32_t BspVcp_Lock(void) {
    uint32_t previous = __get_BASEPRI();
    __set_BASEPRI_MAX(1U << (8U - __NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    return previous;
}

static void BspVcp_Unlock(uint32_t previous) {
    __DMB();
    __set_BASEPRI(previous);
    __ISB();
}

static void BspVcp_Ring_Reset(BspVcp_Ring_s* ring) {
    ring->head = 0;
    ring->tail = 0;
    ring->used = 0;
}

static size_t BspVcp_Ring_GetFree(const BspVcp_Ring_s* ring) {
    return ring->capacity - ring->used;
}

// 调用者保证 length <= 空闲容量 且 length > 0
static void BspVcp_Ring_Push(BspVcp_Ring_s* ring, const uint8_t *src, size_t length) {
    if (length == 0) return;

    size_t first = BspVcp_Min(length, ring->capacity - ring->head);
    memcpy(&ring->data[ring->head], src, first);
    if (length > first) {
        memcpy(ring->data, src + first, length - first);
    }
    ring->head = (ring->head + length) % ring->capacity;
    ring->used += length;
}

/* 复制但不出队，调用者保证 0 < length <= used。 */
static void BspVcp_Ring_Peek(const BspVcp_Ring_s* ring, uint8_t *dst, size_t length) {
    size_t first = BspVcp_Min(length, ring->capacity - ring->tail);
    memcpy(dst, &ring->data[ring->tail], first);
    if (length > first) {
        memcpy(dst + first, ring->data, length - first);
    }
}

static void BspVcp_Ring_Drop(BspVcp_Ring_s* ring, size_t length) {
    ring->tail = (ring->tail + length) % ring->capacity;
    ring->used -= length;
}

/* 所有调用上下文均必须持有 BspVcp_Lock。 */
static bool BspVcp_ReadyLocked(void) {
    return initialized && configured && !rx_fault &&
           hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED &&
           hUsbDeviceFS.pClassData != NULL;
}

/* 仅在 USB 启动前或旧端点已关闭时使用，不覆盖在途数据数组。 */
static void BspVcp_ResetTransportLocked(void) {
    BspVcp_Ring_Reset(&rx_ring);
    BspVcp_Ring_Reset(&tx_ring);
    rx_paused = false;
    rx_fault = false;
    tx_active = false;
    tx_done = false;
    tx_inflight_len = 0;
}

static void BspVcp_TryRearmRxLocked(void) {
    if (!rx_paused || !BspVcp_ReadyLocked() ||
        BspVcp_Ring_GetFree(&rx_ring) < VCP_PACKET_SIZE) {
        return;
    }

    /* CDC_Init_FS 设置的 UserRxBufferFS 始终作为底层接收缓冲。 */
    if (USBD_CDC_ReceivePacket(&hUsbDeviceFS) == USBD_OK) {
        rx_paused = false;
    }
    /* 失败保留暂停标志，下一轮 Poll 重试。 */
}

/**
 * @brief USB 启动前初始化软件状态；重复调用不会清空已有队列和统计。
 * @note 必须先调用本函数，再调用 MX_USB_DEVICE_Init()。
 */
void BspUsbVcp_Init(void) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized) {
        configured = false;
        session_generation = 0;
        memset(&status, 0, sizeof(status));
        BspVcp_ResetTransportLocked();
        initialized = true;
    }
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 主循环或 TIM2 查询当前是否已配置、非挂起且没有 RX 异常锁存。
 * @return 当前传输条件是否满足；不保证主机正在读取或 TX 队列有空位。
 * @note 结果是快照，不能代替提交时的状态检查。
 */
bool BspUsbVcp_IsReady(void) {
    uint32_t irq_state = BspVcp_Lock();
    bool ready = BspVcp_ReadyLocked();
    BspVcp_Unlock(irq_state);
    return ready;
}

/**
 * @brief 主循环或 TIM2 查询 TX 剩余空间，供 Shell 在处理下一字符前背压。
 * @return 未就绪返回 0；在途数据仍占用队列，直到主循环回收。
 * @note 仅查询快照。本项目 Shell 是唯一生产者；新增生产者需重新分配输出预算。
 */
size_t BspUsbVcp_GetTxFree(void) {
    uint32_t irq_state = BspVcp_Lock();
    size_t available = BspVcp_ReadyLocked() ? BspVcp_Ring_GetFree(&tx_ring) : 0;
    BspVcp_Unlock(irq_state);
    return available;
}

/**
 * @brief 主循环或 TIM2 将数据复制进 TX 队列，返回后可立即复用 src。
 * @param src 待发送字节；NULL 返回 0。
 * @param length 请求写入的字节数。
 * @return 实际入队长度；队列满时允许短写，未就绪返回 0。
 * @note 入队不是交付确认，断连可丢弃。上层自行处理短写与会话切换。
 *       TIM2 仅允许短小、有界写入，禁止等待；优先级 0 ISR 不得调用。
 *       其他模块不得绕过本模块调用 CDC_Transmit_FS。
 */
size_t BspUsbVcp_Write(const uint8_t *src, size_t length) {
    if (src == NULL || length == 0) return 0;

    uint32_t irq_state = BspVcp_Lock();
    if (!BspVcp_ReadyLocked()) {
        BspVcp_Unlock(irq_state);
        return 0;
    }
    uint32_t generation = session_generation;
    BspVcp_Unlock(irq_state);

    size_t written = 0;
    while (written < length) {
        irq_state = BspVcp_Lock();
        if (!BspVcp_ReadyLocked() || generation != session_generation) {
            BspVcp_Unlock(irq_state);
            break;
        }

        size_t count = BspVcp_Min(length - written, VCP_COPY_CHUNK);
        count = BspVcp_Min(count, BspVcp_Ring_GetFree(&tx_ring));
        if (count == 0) {
            BspVcp_Unlock(irq_state);
            break;
        }

        BspVcp_Ring_Push(&tx_ring, src + written, count);
        status.tx_queued_bytes += (uint32_t)count;
        written += count;
        BspVcp_Unlock(irq_state);
    }
    return written;
}

/**
 * @brief 主循环或 TIM2 非阻塞读取 RX 队列，不追加字符串结束符、不解析帧。
 * @param dst 接收目标缓冲；NULL 返回 0。
 * @param capacity 目标缓冲可用字节数。
 * @return 实际读出长度，无数据或未就绪返回 0。
 * @note 单次读取不会拼接不同会话；上层需清除自己持有的旧会话残缺命令。
 */
size_t BspUsbVcp_Read(uint8_t *dst, size_t capacity) {
    if (dst == NULL || capacity == 0) return 0;

    uint32_t irq_state = BspVcp_Lock();
    if (!BspVcp_ReadyLocked()) {
        BspVcp_Unlock(irq_state);
        return 0;
    }
    uint32_t generation = session_generation;
    BspVcp_Unlock(irq_state);

    size_t received = 0;
    while (received < capacity) {
        irq_state = BspVcp_Lock();
        if (!BspVcp_ReadyLocked() || generation != session_generation) {
            BspVcp_Unlock(irq_state);
            break;
        }

        size_t count = BspVcp_Min(capacity - received, VCP_COPY_CHUNK);
        count = BspVcp_Min(count, rx_ring.used);
        if (count == 0) {
            BspVcp_Unlock(irq_state);
            break;
        }

        BspVcp_Ring_Peek(&rx_ring, dst + received, count);
        BspVcp_Ring_Drop(&rx_ring, count);
        received += count;
        BspVcp_Unlock(irq_state);
    }
    return received;
}

/**
 * @brief 主循环每轮调用，回收 TX、重试提交，并恢复暂停的 RX。
 * @note 每轮最多提交一个 USB 包，不等待完成。主机不读时保留在途数据，
 *       队列满后 Write 返回短写/0；不擅自重置硬件或释放在途缓冲。
 */
void BspUsbVcp_Poll(void) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized) {
        BspVcp_Unlock(irq_state);
        return;
    }

    /* 在途数据仍计入 TX 队列，只有最终完成后才出队。 */
    if (tx_active && tx_done) {
        BspVcp_Ring_Drop(&tx_ring, tx_inflight_len);
        tx_active = false;
        tx_done = false;
        tx_inflight_len = 0;
    }

    if (!BspVcp_ReadyLocked()) {
        BspVcp_Unlock(irq_state);
        return;
    }

    BspVcp_TryRearmRxLocked();
    if (tx_active || tx_ring.used == 0) {
        BspVcp_Unlock(irq_state);
        return;
    }

    size_t count = BspVcp_Min(tx_ring.used, sizeof(tx_inflight));
    BspVcp_Ring_Peek(&tx_ring, tx_inflight, count);

    /* 状态检查、提交及记录在途状态期间不可插入 USB deinit/完成回调。 */
    uint8_t result = CDC_Transmit_FS(tx_inflight, (uint16_t)count);
    if (result == USBD_OK) {
        tx_inflight_len = (uint16_t)count;
        tx_active = true;
        tx_done = false;
    } else if (result == USBD_BUSY) {
        ++status.tx_busy_count;
    } else {
        ++status.tx_fail_count;
    }
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 仅从 CDC_Init_FS 设置 RX 缓冲后调用，建立新配置会话。
 * @note 首次 OUT 由 ST 库在本回调返回后挂接；这里不能提前发送欢迎语。
 */
void BspUsbVcp_OnConfigured(void) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized) {
        BspVcp_Unlock(irq_state);
        return;
    }

    BspVcp_ResetTransportLocked();
    ++session_generation;
    ++status.session_count;
    configured = true;
    /* ST CDC 类初始化在此回调之后自行挂接首次 OUT 接收。 */
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 仅从 CDC_DeInit_FS 中旧端点关闭后调用，丢弃旧会话队列。
 * @note 应用不得手动模拟该事件。挂起不调用此函数，挂起期间保留在途数据。
 *       上层解析器需另行清除已经取走的旧会话残缺命令。
 */
void BspUsbVcp_OnDeconfigured(void) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized) {
        BspVcp_Unlock(irq_state);
        return;
    }

    configured = false;
    ++session_generation;
    /* 当前 ST CDC 在调用应用 DeInit 前已经关闭 IN/OUT 端点。 */
    BspVcp_ResetTransportLocked();
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 仅从 CDC_Receive_FS 调用，复制当前包并按空间决定是否恢复 OUT。
 * @param data USB 接收缓冲，不要求字符串结束符。
 * @param length 当前包的实际字节数，不得超过 FS 最大包长。
 * @note 本模块独占 OUT 重新挂接；CDC 回调末尾不能无条件再调用
 *       USBD_CDC_ReceivePacket。空间不足时 NAK，由 Poll 恢复。
 *       违反接收约定时锁存异常直到重新配置，防止解析残缺控制命令。
 */
void BspUsbVcp_OnRx(const uint8_t *data, size_t length) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized || !configured) {
        BspVcp_Unlock(irq_state);
        return;
    }

    rx_paused = true;
    if (rx_fault || length > VCP_PACKET_SIZE ||
        (data == NULL && length != 0) ||
        length > BspVcp_Ring_GetFree(&rx_ring)) {
        status.rx_drop_bytes += (uint32_t)length;
        rx_fault = true;
        BspVcp_Unlock(irq_state);
        return;
    }

    if (length != 0) {
        BspVcp_Ring_Push(&rx_ring, data, length);
        status.rx_bytes += (uint32_t)length;
    }
    if (BspVcp_Ring_GetFree(&rx_ring) < VCP_PACKET_SIZE) {
        ++status.rx_pause_count;
    }
    BspVcp_TryRearmRxLocked();
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 仅从 CDC_TransmitCplt_FS 最终完成回调调用，发布 TX 完成事件。
 * @note 包含 ST 库自动处理的 ZLP；队列在下一次 Poll 中出队。
 */
void BspUsbVcp_OnTxComplete(void) {
    uint32_t irq_state = BspVcp_Lock();
    if (!initialized || !configured || !tx_active || tx_done) {
        BspVcp_Unlock(irq_state);
        return;
    }

    status.tx_complete_bytes += tx_inflight_len;
    tx_done = true;
    BspVcp_Unlock(irq_state);
}

/**
 * @brief 主循环或 TIM2 获取统计快照；NULL 无操作。
 * @note 统计跨会话累计，uint32_t 计数允许自然回绕。
 */
void BspUsbVcp_GetStats(BspUsbVcpStatus_s *out) {
    if (out == NULL) return;

    uint32_t irq_state = BspVcp_Lock();
    *out = status;
    BspVcp_Unlock(irq_state);
}
