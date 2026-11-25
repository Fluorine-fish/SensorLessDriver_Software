/**
*   @file ShellReply.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/25
*   @version 1.0
*   @note
*/
#include "ShellReply.h"
#include "shell.h"
#include <stddef.h>

/**
 * @brief 当前 Shell 命令内输出一行，不使用全局 printf，不等待 USB。
 * @note 仅供 Shell 命令上下文调用，文本需短小、有界。
 */
void ShellReply_Line(const char *text) {
    Shell *shell = shellGetCurrent();
    if (shell == NULL || text == NULL) return;
    shellWriteString(shell, text);
    shellWriteString(shell, "\r\n");
}
