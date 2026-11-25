/**
*   @file ShellConfigCmd.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/25
*   @version 1.0
*   @note
*/
#include "stdio.h"
#include "shell.h"
#include "ShellReply.h"
#include "FocComm.h"

/** @brief 修改电调ID */
static int ShellConfig_SetCanId(int id) {
    if (id < 1 || id > 8) {
        ShellReply_Line("ERR usage: ID out of range!");
        return -1;
    }

    FocComm_SetCanId(id);

    char msg[32];
    snprintf(msg, sizeof(msg), "Set Driver CANId : %d", id);
    ShellReply_Line(msg);
    return 0;
}

/** @brief 获取电调CANID */
static int ShellSystem_GetCanId(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1) {
        ShellReply_Line("ERR usage: config_get_canid");
        return -1;
    }

    uint32_t txid,rxid;
    FocComm_GetCanId(&txid, &rxid);

    char msg[64];
    snprintf(msg, sizeof(msg), "Driver TxId :  0x%x , RxId : 0x%x",
             (unsigned int)txid, (unsigned int)rxid);
    ShellReply_Line(msg);
    return 0;
}

/** @brief 获取电调CAN通讯状态 */
static int ShellSystem_GetCanStatus(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1) {
        ShellReply_Line("ERR usage: config_get_can_status");
        return -1;
    }

    int status = FocComm_GetCanStatus();

    if (status == -1) ShellReply_Line("CAN TIMEOUT!");
    else if (status == 0) ShellReply_Line("CAN OK!");

    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC) |
                 SHELL_CMD_DISABLE_RETURN,
                 config_set_canid, ShellConfig_SetCanId, set driver id);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                 SHELL_CMD_DISABLE_RETURN,
                 config_get_canid, ShellSystem_GetCanId, get driver id);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                 SHELL_CMD_DISABLE_RETURN,
                 config_get_can_status, ShellSystem_GetCanStatus, get CAN status);
