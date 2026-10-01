// Доработка: Кряжев А., 2026
// Модуль работы с памятью
// Автор: Корякин Сергей

#include "includes.h"

#ifndef MEM_H
#define MEM_H

#define mem_check_power()       user_wait_stable_power() /* ждем пока напряжение не будет выше порога*/
#define MEMSECTORSIZE         4096
#define MEM_CONFIG_LEGACY     0x1CD400u
#define MEM_CONFIG_SLOT0      0x1F5000u
#define MEM_CONFIG_SLOT1      0x1F7000u
#define MEM_CONFIG_SLOT_SIZE 8192u
extern volatile uint32_t mem_config_errors;
bool mem_recover(void);
bool mem_tables_ready(void);
bool mem_replace_sector(uint32_t adr, const uint8_t *image);

// Чтение данных с памяти
bool mem_read (Int32U adr, pInt8U buff, Int16U size);

// Запись данных в память
bool mem_write (Int32U adr, pInt8U buff, Int16U size);

// Стереть сектор
bool mem_erase_sector (Int32U adr);

// Запись данных в память с предварительным чтением сектора
bool mem_rewrite (Int32U adr, pInt8U buff, Int16U size, uint8_t* mem_buffer4kB);

// стереть диапазон памяти
bool mem_erase_mem (Int32U fadr, Int32U eadr);

#endif