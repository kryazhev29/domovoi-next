/******************************************************************************
*    SPI functions
*    Author      : Sergey Koryakin
******************************************************************************/
#include "spi_stm32f10x.h"
#include "arm_comm.h"
#include <stdlib.h>
#include "includes.h"

#ifndef FLASH_W25
#define FLASH_W25

#define w25_chip_ON()   CSEL_LOW
#define w25_chip_OFF()  CSEL_HIGH

#define FLASH_W25_PAGE_SIZE     256

/* Start owns src until poll returns 2 (verified) or 3 (error). NULL means erase. */
bool w25_async_start(uint32_t address, const uint8_t *src, uint16_t len, uint32_t now);
uint8_t w25_async_poll(uint32_t now); /* 0 idle, 1 pending, 2 done, 3 error */
bool w25_async_active(void);
bool w25_read_available(void);

// Чтение данных с памяти
bool w25_read_data (Int32U address, pInt8U dst, Int16U len);
// Запись данных в память
bool w25_write_data (Int32U address, pInt8U src, Int16U len);
// Стереть сектор
bool w25_erase_sector (Int32U address);
// Чтение Id памяти
Int16U w25_read_devID (void);
// Чтение статуса
Int8U w25_read_status (void);

// Ожидание завершения предыдущей команды
static uint8_t w25_busy (void);
// встраиваемая задержка
static inline void w25_delay_ON (void);
// встраиваемая задержка 
static inline void w25_delay_OFF (void);
// разрешить запись
static void w25_WREN();
// запретить запись
static void w25_WRDI();
// Запись страницы памяти
static bool w25_page_program (Int32U address, pInt8U src, Int16U len);

static bool w25_verify(Int32U address, const uint8_t *src, Int16U len);

#endif