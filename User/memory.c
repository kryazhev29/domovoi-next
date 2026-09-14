// Модуль работы с памятью
// Автор: Корякин Сергей

#include "memory.h"


// Чтение данных с памяти
void mem_read (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();  
  w25_read_data (adr, buff, size);
}

// Запись данных в память
// данные пишутся в предварительно стертый сектор
void mem_write (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();  
  w25_write_data (adr, buff, size);
}

// Стереть сектор
void mem_erase_sector (Int32U adr)
{
  mem_check_power();
  w25_erase_sector(adr);
}

// Запись данных в память с предварительным чтением сектора
void mem_rewrite (Int32U adr, uint8_t* buff, Int16U size, uint8_t* mem_buffer4kB)
{
  Int32U sector;

  if (mem_buffer4kB == NULL) return;
  if (buff == NULL) return;

  mem_check_power();

  sector = div(adr, MEMSECTORSIZE).quot * MEMSECTORSIZE;
  
  if ((adr - sector + size) > MEMSECTORSIZE) return;

  w25_read_data (sector, mem_buffer4kB, MEMSECTORSIZE);
  
  w25_erase_sector (sector);
  
  memcpy(&mem_buffer4kB[adr - sector], buff, size);
  
  w25_write_data (sector, mem_buffer4kB, MEMSECTORSIZE);
}

// стереть диапазон памяти
void mem_erase_mem (Int32U fadr, Int32U eadr)
{
  mem_check_power();

  fadr = div(fadr, MEMSECTORSIZE).quot * MEMSECTORSIZE;
  for (;fadr <= eadr; fadr += MEMSECTORSIZE)
  {
    wdt_reload();
    w25_erase_sector(fadr);
  }
}