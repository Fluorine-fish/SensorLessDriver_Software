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

#define DATA_FLASH_SIZE (0x080FFFFF - ADDR_FLASH_SECTOR_0)

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

void BspFlash_Write(uint16_t SECTOR_ID, uint32_t* data, uint32_t length);
void BspFlash_Read(uint32_t offset, void* data, uint32_t length);


#endif //VESC_F4_BSPFLASH_H