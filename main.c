// Доработка: Кряжев А., 2026
/*****************
Основная программа
Автор: Корякин С.А.
Внешний источник частоты 12 МГц, системная частота 72 МГц
*****************/

#include "main.h"
#include "Bootloader/app_update.h"

unsigned int loop_max_ms = 0, loop_stalls = 0;

/* Замер стека (01.10.2026): при старте свободная часть CSTACK заполняется шаблоном; команда 111
 * отдаёт, сколько байт шаблона снизу ни разу не тронуто - минимальный запас стека за работу,
 * вместе с прерываниями. ОЗУ не занимает. Доработка: Кряжев А., 2026 */
#define STACK_PAINT 0x5A5AA5A5u
#pragma section = "CSTACK"
void stack_paint(void)
{
  uint32_t *p = (uint32_t*)__section_begin("CSTACK");
  uint32_t *end = (uint32_t*)(__get_MSP() - 64u);
  while (p < end) *p++ = STACK_PAINT;
}
uint32_t stack_free_bytes(void)
{
  const uint32_t *b = (const uint32_t*)__section_begin("CSTACK");
  const uint32_t *e = (const uint32_t*)__section_end("CSTACK");
  const uint32_t *p = b;
  while (p < e && *p == STACK_PAINT) p++;
  return (uint32_t)(p - b) * 4u;
}

// основная программа
void main()
{
  stack_paint();
  // инициализация входов
  board_init();

  // Разрешаем события UsageFault, BusFault, MemFault
  SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

  // переносим таблицу прерываний, т.к. она уже начинается не с нулевого адреса
  __disable_interrupt(); // Запрещаем прерывания
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, APLICATION_ADDRESS);
  __enable_interrupt();
  
  // начальная индикация
  led_load(PWR_LED, LED_ON);
  
  // настраиваем периферию
  timers_init(); // таймер временных интервалов
  spi_init(); // spi
  adc_init(); // ацп
  
  delay_ms(500);
  wdt_reload(); // сброс watchdog
 
  // настраиваем usb при необходимости при подключенном кабеле
  if (USB_DETECT)
    usb_init();

  // Инициализация сторожевого таймера
  wdt_init(3750); // 3 сек

  // Инициализация пользовательской программы
  user_init();
    
  // включаем питание gsm модуля

  unsigned int loop_mark = timers_getMillis();

  // Входим в основной цикл работы
  while (true)
  {
    // порт - gsm
    gsm_check_data();

    // порт - 485
    port_check_data();

    // порт - USB
    usb_check_data();

    // циклический обработчик
    user_loop();
    user_storage_poll();

    /* Команда 111: самый долгий проход главного цикла и проходы от 1 с. */
    {
      unsigned int now = timers_getMillis(), took = now - loop_mark;
      loop_mark = now;
      if (took > loop_max_ms) loop_max_ms = took;
      if (took >= 1000u) loop_stalls++;
    }
  } // while
} // main