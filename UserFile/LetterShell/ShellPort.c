/**
*   @file ShellPort.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/25
*   @version 1.0
*   @note
*/
#include "ShellPort.h"
#include "BspUsbVcp.h"
#include "shell.h"
#include <string.h>

/* 上游按键处理函数未在 shell.h 声明；通过导出宏复用历史和光标逻辑。
 * 普通模式 ESC [ A/B/C/D 已由上游注册，这里补充终端应用模式 ESC O A/B/C/D。
 * Backspace 的 0x08、0x7F 均已由上游映射为删除光标前一个字符。
 */
extern void shellUp(Shell *shell);
extern void shellDown(Shell *shell);
extern void shellRight(Shell *shell);
extern void shellLeft(Shell *shell);

SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x1B4F4100, shellUp, up application mode);
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x1B4F4200, shellDown, down application mode);
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0) | SHELL_CMD_ENABLE_UNCHECKED,
                 0x1B4F4300, shellRight, right application mode);
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0) | SHELL_CMD_ENABLE_UNCHECKED,
                 0x1B4F4400, shellLeft, left application mode);

static Shell usb_shell;
static char shell_buffer[128 * (SHELL_HISTORY_MAX_NUMBER + 1)];
static bool port_initialized;
static bool shell_registered;
static bool previous_was_cr;
static uint32_t active_session;
static bool output_short;

/**
 * @brief Shell 的非阻塞读回调；仅由 TIM2 的 Shell 处理上下文调用。
 */
static signed short ShellPort_Read(char *data, unsigned short length) {
    return (signed short)BspUsbVcp_Read((uint8_t *)data, length);
}

/**
 * @brief Shell 输出只入队，绝不在 TIM2 中等待更低优先级的 USB 完成。
 * @note 每次处理字符前等待 TX 队列为空，为当前内置命令保留完整输出空间。
 *       新命令输出不得超过 VCP_TX_CAPACITY；大输出需改成异步分段任务。
 */
static signed short ShellPort_Write(char *data, unsigned short length) {
    size_t accepted = BspUsbVcp_Write((const uint8_t *)data, length);
    if (accepted != length) output_short = true;
    return (signed short)accepted;
}

/**
 * @brief TIM2 启动前初始化适配层；USB 尚未配置时不调用会立即输出的 shellInit。
 */
void ShellPort_Init(void) {
    if (port_initialized) return;
    port_initialized = true;
}

/**
 * @brief 从 FocApp_UpdateLowFreqData/TIM2 调用，每轮最多解析一个字符。
 * @note 字符可能同步触发整条命令；这里只注册有界、无等待的验证命令。
 *       Shell 是 TX 唯一生产者；USB 发送及完成回收由主循环 BspUsbVcp_Poll 执行。
 */
void ShellPort_Poll(void) {
    if (!port_initialized || !BspUsbVcp_IsReady()) return;

    BspUsbVcpStatus_s stats;
    BspUsbVcp_GetStats(&stats);
    if (!shell_registered || active_session != stats.session_count) {
        if (BspUsbVcp_GetTxFree() != VCP_TX_CAPACITY) return;
        if (shell_registered) shellRemove(&usb_shell);
        memset(&usb_shell, 0, sizeof(usb_shell));
        memset(shell_buffer, 0, sizeof(shell_buffer));
        usb_shell.read = ShellPort_Read;
        usb_shell.write = ShellPort_Write;
        active_session = stats.session_count;
        previous_was_cr = false;
        output_short = false;
        shellInit(&usb_shell, shell_buffer, sizeof(shell_buffer));
        shell_registered = true;
        return;
    }

    if (BspUsbVcp_GetTxFree() != VCP_TX_CAPACITY) return;
    if (output_short) {
        /* 超预算时明确报告，禁止在中断里补发/忙等。下一条输入仍可继续。 */
        output_short = false;
        const char error[] = "\r\nERR output too long\r\n";
        ShellPort_Write((char *)error, sizeof(error) - 1);
        return;
    }

    char data;
    if (ShellPort_Read(&data, 1) != 1) return;
    if (data == '\n' && previous_was_cr) {
        previous_was_cr = false;
        return;
    }
    previous_was_cr = data == '\r';
    shellHandler(&usb_shell, data);
}
