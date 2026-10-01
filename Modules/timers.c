// Модуль таймеров и пауз
#include "timers.h"

#define timer_cnt	10

volatile unsigned int milliSeconds = 0;
volatile unsigned int timer[timer_cnt];

__weak void wdt_reload(void){}
__weak void timers_user_handle(void){}


// инициализация ---------------------
void timers_init(void)
{
  SysTick_Config(SystemCoreClock / 1000);
}

// Согласованный снимок ISR -> main; восстановить именно прежнюю маску прерываний.
static unsigned int timers_snapshot(volatile unsigned int *value)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  unsigned int result = *value;
  __set_PRIMASK(mask);
  return result;
}

// установка таймера -----------------
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
unsigned int timers_get(unsigned char t){
  if (t < timer_cnt)
    return timers_snapshot(&timer[t]);
  return 0;
}

// обработчик прерывания -------------
inline void timers_handle(void){
  milliSeconds++;
  for (unsigned char i = 0; i < timer_cnt; i++)
    timer[i]++;
  timers_user_handle();
}

// пауза в мс -------------------------
void timers_pause(const unsigned int msec){
  unsigned int start = timers_getMillis();
  
  while ((timers_getMillis() - start) < msec)
    wdt_reload();
}

// счетчик миллисекунд -------------------------------
unsigned int timers_getMillis(void){
  return timers_snapshot(&milliSeconds);
}