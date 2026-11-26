/**
*   @file BspBoot.h
*   @brief APP / BootLoader 共享地址、固件元数据与复位邮箱
*   @date 2026/11/25
*/
#ifndef VESC_F4_BSPBOOT_H
#define VESC_F4_BSPBOOT_H
#include <stdbool.h>
#include <stdint.h>

#define BOOT_APP_ADDR 0x08008000U
#define BOOT_APP_SIZE (864U * 1024U)
#define BOOT_APP_END (BOOT_APP_ADDR + BOOT_APP_SIZE)
#define BOOT_INFO_MAGIC 0x434C4931U
#define BOOT_STATE_UPDATING 0x55504454U
#define BOOT_STATE_VALID 0x56414C44U
#define BOOT_MAILBOX_ADDR 0x2001FFF8U
#define BOOT_REQUEST_MAGIC 0x424F4F54U

typedef struct {
    uint32_t magic;
    uint32_t state;
    uint32_t app_addr;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
    uint32_t reserved0;
    uint32_t reserved1;
} BootInfo_s;

_Static_assert(sizeof(BootInfo_s) == 32U, "BootInfo wire format");

/* APP：命令只置位；主循环停用控制外设，发送响应后复位。 */
void BspBoot_Request(void);
bool BspBoot_IsRequested(void);
void BspBoot_Poll(void);
#endif //VESC_F4_BSPBOOT_H
