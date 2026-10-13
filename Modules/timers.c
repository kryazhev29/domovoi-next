// Доработка: Кряжев А., 2026
// Миллисекундный SysTick и программные счётчики интервалов.
// ISR увеличивает счётчики; индексы 0–5 и ненулевой период задаёт приложение.
// Задержки обслуживают сторож, исходный PRIMASK сохраняется.

#include "timers.h"
#include "comm_power_v5.h"

#define timer_cnt 6

volatile unsigned int milliSeconds = 0;
volatile unsigned int timer[timer_cnt];

// Настроить SysTick на 1 мс с прежним низшим приоритетом.
void timers_init(void)
{
  SysTick->LOAD = SystemCoreClock / 1000u - 1u;
  SCB->SHP[11] = 0xF0u;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

// Атомарно снять счётчик, сохранив исходный PRIMASK.
static unsigned int timers_snapshot(volatile unsigned int *value)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  unsigned int result = *value;
  __set_PRIMASK(mask);
  return result;
}

// Атомарно задать значение программного счётчика.
void timers_set(unsigned char t, unsigned int vl)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  timer[t] = vl;
  __set_PRIMASK(mask);
}

// Вернуть атомарный снимок заданного счётчика.
unsigned int timers_get(unsigned char t){
  return timers_snapshot(&timer[t]);
}

// Обслужить очередной тик и пользовательский обработчик.
inline void timers_handle(void){
  milliSeconds++;
  for (unsigned char i = 0; i < timer_cnt; i++)
    timer[i]++;
  comm_power_v5_poll(milliSeconds);
  timers_user_handle();
}

// Выдержать задержку, обслуживая сторож.
void timers_pause(const unsigned int msec){
  unsigned int start = timers_getMillis();

  while ((timers_getMillis() - start) < msec)
    wdt_reload();
}

// Забрать целые периоды, сохранив остаток времени.
unsigned int timers_take(unsigned char t, unsigned int period)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  unsigned int count = timer[t] / period;
  timer[t] %= period;
  __set_PRIMASK(mask);
  return count;
}

// Вернуть атомарный снимок миллисекундного времени.
unsigned int timers_getMillis(void){
  return timers_snapshot(&milliSeconds);
}
