/**
*   @file ShellSystemCmd.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/25
*   @version 1.0
*   @note
*/
#include "shell.h"
#include "ShellReply.h"
#include "BspBoot.h"

/** @brief 最小请求响应验证；只接受不带参数的 ping。 */
static int ShellSystem_Ping(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1) {
        ShellReply_Line("ERR usage: ping");
        return -1;
    }
    ShellReply_Line("pong");
    return 0;
}

/** @brief 标识当前验证接口版本，不伪造项目固件版本号。 */
static int ShellSystem_Version(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1) {
        ShellReply_Line("ERR usage: version");
        return -1;
    }
    ShellReply_Line("VESC_F4 USB shell bring-up v1");
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                 SHELL_CMD_DISABLE_RETURN,
                 ping, ShellSystem_Ping, reply pong);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                 SHELL_CMD_DISABLE_RETURN,
                 version, ShellSystem_Version, show shell integration version);

/** @brief 请求主循环关闭输出并复位进入 USB IAP。 */
static int ShellSystem_BootLoader(int argc, char *argv[]) {
    (void)argv;
    if (argc != 1) {
        ShellReply_Line("ERR usage: bootloader");
        return -1;
    }
    ShellReply_Line("OK entering bootloader");
    BspBoot_Request();
    return 0;
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) |
                 SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) |
                 SHELL_CMD_DISABLE_RETURN,
                 bootloader, ShellSystem_BootLoader, reboot into USB IAP);
