/**
*   @file BspFlash.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/16
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPFLASH_H
#define VESC_F4_BSPFLASH_H

#include "stdint.h"
#include "stdbool.h"

#define DATA_FLASH_SIZE (1024U * 1024U)

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define FLASH_Sector_0   ((uint16_t)0U)
#define FLASH_Sector_1   ((uint16_t)1U)
#define FLASH_Sector_2   ((uint16_t)2U)
#define FLASH_Sector_3   ((uint16_t)3U)
#define FLASH_Sector_4   ((uint16_t)4U)
#define FLASH_Sector_5   ((uint16_t)5U)
#define FLASH_Sector_6   ((uint16_t)6U)
#define FLASH_Sector_7   ((uint16_t)7U)
#define FLASH_Sector_8   ((uint16_t)8U)
#define FLASH_Sector_9   ((uint16_t)9U)
#define FLASH_Sector_10  ((uint16_t)10U)
#define FLASH_Sector_11  ((uint16_t)11U)

/* Sector 11 追加日志：固定 512 字节槽；不自动擦除、不自动回收。 */
#define BSP_FLASH_LOG_ADDR ADDR_FLASH_SECTOR_11
#define BSP_FLASH_LOG_SIZE (128U * 1024U)
#define BSP_FLASH_RECORD_SIZE 512U
#define BSP_FLASH_VALUE_MAX 480U
#define BSP_FLASH_KEY_BOOTINFO 1U
#define BSP_FLASH_KEY_MOTOR_PARAMS 0x100U

typedef enum {
    BSP_FLASH_OK = 0,
    BSP_FLASH_NOT_FOUND,
    BSP_FLASH_FULL,
    BSP_FLASH_BAD_ARGUMENT,
    BSP_FLASH_CORRUPT,
    BSP_FLASH_IO_ERROR,
    BSP_FLASH_BAD_CONTEXT
} BspFlashResult_e;

typedef struct {
    uint32_t used_slots;
    uint32_t free_slots;
    uint32_t damaged_slots;
    bool blank;
} BspFlashStatus_s;

/* 仅主循环调用；写入前须停止电机 PWM/控制中断，不能从 Shell ISR 擦写。
 * key 独立保存最后一次已提交值；value 内自行携带电机参数结构版本。
 * length 返回实际长度；容量不足返回 BAD_ARGUMENT，不截断。
 * 掉电未提交的槽被跳过，永不重新使用；满时返回 FULL。
 */
BspFlashResult_e BspFlash_GetStatus(BspFlashStatus_s *status);
BspFlashResult_e BspFlash_ReadRecord(uint32_t key, void *data,
                                    uint32_t capacity, uint32_t *length);
BspFlashResult_e BspFlash_WriteRecord(uint32_t key, const void *data, uint32_t length);
uint32_t BspFlash_Crc32(const void *data, uint32_t length);

/* 兼容旧调用：Write 只接受 Sector 11，改为追加 MOTOR_PARAMS（最大480字节）。
 * Read 仍是相对 Flash 起点的只读访问；读取逻辑参数请改用 ReadRecord。
 */
bool BspFlash_Write(uint16_t sector_id, uint32_t *data, uint32_t length);
void BspFlash_Read(uint32_t offset, void *data, uint32_t length);

#endif //VESC_F4_BSPFLASH_H
