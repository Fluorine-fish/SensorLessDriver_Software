/**
*   @file BspDwt.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/11/22
*   @version 1.0
*   @note
*/
#ifndef VESC_F4_BSPDWT_H
#define VESC_F4_BSPDWT_H

#include "stdint.h"

#define  DWT_CR      *(__IO uint32_t *)0xE0001000
#define  DWT_CYCCNT  *(__IO uint32_t *)0xE0001004
#define  DEM_CR      *(__IO uint32_t *)0xE000EDFC


#define  DEM_CR_TRCENA                   (1 << 24)
#define  DWT_CR_CYCCNTENA                (1 <<  0)

void BspDwt_Init(void);
void BspDwt_Delay_us(uint32_t us);
uint32_t BspDwt_GetCount(void);
float BspDwt_GetPeriod(uint32_t start_count, uint32_t end_count);

#endif //VESC_F4_BSPDWT_H
