/**
*   @file Shell_cfg_user.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/25
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_SHELL_CFG_USER_H
#define VESC_F4_SHELL_CFG_USER_H

#include "stm32f4xx_hal.h"
#include <stddef.h>

#define SHELL_TASK_WHILE             0
#define SHELL_USING_CMD_EXPORT       1
#define SHELL_MAX_NUMBER             1
#define SHELL_HISTORY_MAX_NUMBER     5
#define SHELL_PARAMETER_MAX_NUMBER   8
#define SHELL_GET_TICK()             HAL_GetTick()
#define SHELL_DEFAULT_USER           "Driver_F4"
#define SHELL_DEFAULT_USER_PASSWORD  ""
#define SHELL_SHOW_INFO              0
#define SHELL_CLS_WHEN_LOGIN         0
#define SHELL_HELP_SHOW_PERMISSION   0
#define SHELL_USING_LOCK             0
#define SHELL_USING_COMPANION        0
#define SHELL_PRINT_BUFFER           0
#define SHELL_SCAN_BUFFER            0
#define SHELL_EXEC_UNDEF_FUNC        0
#define SHELL_KEEP_RETURN_VALUE      0
#define SHELL_SUPPORT_END_LINE       0
#define SHELL_USING_FUNC_SIGNATURE   0
#define SHELL_SUPPORT_ARRAY_PARAM    0
#define SHELL_ENTER_CR               1
#define SHELL_ENTER_LF               1
#define SHELL_ENTER_CRLF             0

#endif //VESC_F4_SHELL_CFG_USER_H
