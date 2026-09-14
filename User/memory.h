// Модуль работы с памятью
// Автор: Корякин Сергей

#include "includes.h"

#ifndef MEM_H
#define MEM_H

#define mem_check_power()       user_wait_stable_power() /* ждем пока напряжение не будет выше порога*/
#define MEMSECTORSIZE         4096

// Чтение данных с памяти
void mem_read (Int32U adr, pInt8U buff, Int16U size);

// Запись данных в память
void mem_write (Int32U adr, pInt8U buff, Int16U size);

// Стереть сектор
void mem_erase_sector (Int32U adr);

// Запись данных в память с предварительным чтением сектора
void mem_rewrite (Int32U adr, pInt8U buff, Int16U size, uint8_t* mem_buffer4kB);

// стереть диапазон памяти
void mem_erase_mem (Int32U fadr, Int32U eadr);

#endif