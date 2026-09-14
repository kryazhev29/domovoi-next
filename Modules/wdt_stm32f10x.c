/*****************************************************************************
*  Функции работы со сторожевым таймером
*  Author      : Sergey Koryakin
*****************************************************************************/
#include "wdt_stm32f10x.h"
#include "stm32f10x_iwdg.h"


// Инициализация таймера
// значение в миллисекундах от 1 до 4096
void wdt_init (Int32U value)
{
#ifndef _DEBUGER  
  // разрешаем доступ к регистрам таймера
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  // устанавливаем предделитель
  IWDG_SetPrescaler(IWDG_Prescaler_32);
  // программируем значение регистра начала счёта/перезагрузки
  if (value > 4095) value = 4095;
  IWDG_SetReload(value & 0xfff);
  // запускаем
  IWDG_Enable();
#endif
}

// Перезагрузка регистра сторожевого таймера
void wdt_reload (void)
{
#ifndef _DEBUGER
  IWDG_ReloadCounter();
#endif
}