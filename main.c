// Доработка: Кряжев А., 2026
// Старт периферии платы v5 и главный цикл приложения HYBRID/C.
// Адреса заголовка и векторов сохраняют договор с загрузчиком.
// Историческая задержка 500 мс оставлена до выяснения аппаратного назначения.

#include "main.h"
#include "Bootloader/app_update.h"
#include "User/prof.h"

#if DOMOVOI_DIAG
unsigned int loop_max_ms = 0, loop_stalls = 0;
#endif

/* Замер стека (01.10.2026): при старте свободная часть CSTACK заполняется шаблоном; команда 111
 * отдаёт, сколько байт шаблона снизу ни разу не тронуто - минимальный запас стека за работу,
 * вместе с прерываниями. ОЗУ не занимает. Доработка: Кряжев А., 2026 */
#define STACK_PAINT 0x5A5AA5A5u
#pragma section = "CSTACK"
// Заполнить свободный стек шаблоном контроля запаса.
void stack_paint(void)
{
  uint32_t *p = (uint32_t*)__section_begin("CSTACK");
  uint32_t *end = (uint32_t*)(__get_MSP() - 64u);
  while (p < end) *p++ = STACK_PAINT;
}
#if DOMOVOI_DIAG
// Посчитать минимальный оставшийся запас стека.
uint32_t stack_free_bytes(void)
{
  const uint32_t *b = (const uint32_t*)__section_begin("CSTACK");
  const uint32_t *e = (const uint32_t*)__section_end("CSTACK");
  const uint32_t *p = b;
  while (p < e && *p == STACK_PAINT) p++;
  return (uint32_t)(p - b) * 4u;
}
#endif

// При повреждении стека записать STAK и ждать сторож без стека.
__stackless void stack_canary_check(void)
{
  if(*(volatile uint32_t*)__section_begin("CSTACK")==STACK_PAINT) return;
  __asm(
    "CPSID I\n"
    "LDR R0, =fault_record\n"
    "MOVS R1, #0\n"
    "MOVS R2, #12\n"
    "stack_canary_zero:\n"
    "STR R1, [R0], #4\n"
    "SUBS R2, R2, #1\n"
    "BNE stack_canary_zero\n"
    "LDR R0, =fault_record\n"
    "LDR R1, =0x5354414B\n"
    "STR R1, [R0, #4]\n"
    "STR LR, [R0, #8]\n"
    "MRS R1, MSP\n"
    "STR R1, [R0, #12]\n"
    "DSB\n"
    "LDR R1, =0x31544C46\n"
    "STR R1, [R0]\n"
    "DSB\n"
    "stack_canary_wait:\n"
    "B stack_canary_wait\n"
  );
}

// Инициализировать плату и выполнять обработчики приложения.
void main()
{
  stack_paint();
  // инициализация входов
  board_init();

  // Разрешаем события UsageFault, BusFault, MemFault
  SCB->SHCSR = SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

  // переносим таблицу прерываний, т.к. она уже начинается не с нулевого адреса
  __disable_interrupt(); // Запрещаем прерывания
  SCB->VTOR = APLICATION_ADDRESS;
  __enable_interrupt();

  // начальная индикация
  led_load(PWR_LED, LED_ON);

  // настраиваем периферию
  timers_init(); // таймер временных интервалов
  spi_init();
  adc_init(); // ацп

  // Назначение задержки не описано в истории: не сокращать без проверки платы.
  delay_ms(500);
  wdt_reload(); // обслужить сторож

  // Инициализация сторожевого таймера
  wdt_init();

  // Инициализация пользовательской программы
  user_init();

  // Питание модема уже идёт из SysTick; USB больше не задерживает его запуск.
  if (USB_DETECT)
    usb_init();

  #if DOMOVOI_DIAG
  unsigned int loop_mark = timers_getMillis();
  prof_init();
  uint32_t prof_loop = PROF_NOW();
  #endif

  while (true)
  {
    stack_canary_check();
    PROF_STEP(PROF_GSM, gsm_check_data());

    PROF_STEP(PROF_PORT, port_check_data());

    PROF_STEP(PROF_USB, usb_check_data());

    PROF_STEP(PROF_USER, user_loop());
    PROF_STEP(PROF_STORAGE, user_storage_poll());

    #if DOMOVOI_DIAG
    /* Команда 111: самый долгий проход главного цикла и проходы от 1 с. */
    {
      unsigned int now = timers_getMillis(), took = now - loop_mark;
      loop_mark = now;
      if (took > loop_max_ms) loop_max_ms = took;
      if (took >= 1000u) loop_stalls++;
    }
    {
      uint32_t t = PROF_NOW();
      prof_add(PROF_LOOP, t - prof_loop);
      prof_loop = t;
    }
    #endif
  }
}
