#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
#define FLASH_TYPEPROGRAM_WORD 0
#define FLASH_FLAG_EOP 1
#define FLASH_FLAG_OPERR 2
#define FLASH_FLAG_WRPERR 4
#define FLASH_FLAG_PGAERR 8
#define FLASH_FLAG_PGPERR 16
#define FLASH_FLAG_PGSERR 32
#define __HAL_FLASH_CLEAR_FLAG(flags) ((void)(flags))
uint32_t __get_IPSR(void);
uint32_t __get_PRIMASK(void);
uint32_t __get_BASEPRI(void);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type, uintptr_t address, uint64_t value);
void FLASH_FlushCaches(void);
#endif
