// Доработка: Кряжев А., 2026
// Миллисекундный SysTick и программные счётчики интервалов.
// ISR увеличивает счётчики, главный цикл читает и сбрасывает их атомарно.
// Задержки обслуживают сторож, исходный PRIMASK сохраняется.

#include "timers.h"

#define timer_cnt	10

volatile unsigned int milliSeconds = 0;
volatile unsigned int timer[timer_cnt];

// Обслужить аппаратный сторож, если отладка не запрещает его.
__weak void wdt_reload(void){}
// Позволить приложению обслужить очередную миллисекунду.
__weak void timers_user_handle(void){}


// инициализация ---------------------
// Настроить SysTick на 1 мс с прежним низшим приоритетом.
void timers_init(void)
{
  SysTick->LOAD = SystemCoreClock / 1000u - 1u;
  SCB->SHP[11] = 0xF0u;
  SysTick->VAL = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

// Согласованный снимок ISR -> main; восстановить именно прежнюю маску прерываний.
// Атомарно снять счётчик, сохранив исходный PRIMASK.
static unsigned int timers_snapshot(volatile unsigned int *value)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  unsigned int result = *value;
  __set_PRIMASK(mask);
  return result;
}

// установка таймера -----------------
// Атомарно задать значение программного счётчика.
void timers_set(unsigned char t, unsigned int vl)
{
  if (t < timer_cnt){
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    timer[t] = vl;
    __set_PRIMASK(mask);
  }
}

// запрос значения -------------------
// Вернуть атомарный снимок заданного счётчика.
unsigned int timers_get(unsigned char t){
  if (t < timer_cnt)
    return timers_snapshot(&timer[t]);
  return 0;
}

// обработчик прерывания -------------
// Обслужить очередной тик и пользовательский обработчик.
inline void timers_handle(void){
  milliSeconds++;
  for (unsigned char i = 0; i < timer_cnt; i++)
    timer[i]++;
  timers_user_handle();
}

// пауза в мс -------------------------
// Выдержать задержку, обслуживая сторож.
void timers_pause(const unsigned int msec){
  unsigned int start = timers_getMillis();

  while ((timers_getMillis() - start) < msec)
    wdt_reload();
}

// счетчик миллисекунд -------------------------------
// забираем целые периоды без потери остатка и тика между чтением и записью
// Забрать целые периоды, сохранив остаток времени.
unsigned int timers_take(unsigned char t, unsigned int period)
{
  if (t >= timer_cnt || period == 0) return 0;
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