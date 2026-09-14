// Модуль таймеров и пауз
#include "timers.h"

#define timer_cnt	10

unsigned int milliSeconds = 0;
unsigned int timer[timer_cnt];

__weak void wdt_reload(void){}
__weak void timers_user_handle(void){}


// инициализация ---------------------
void timers_init(void)
{
  SysTick_Config(SystemCoreClock / 1000);
}

// установка таймера -----------------
void timers_set(unsigned char t, unsigned int vl)
{
  if (t < timer_cnt)
    timer[t] = vl;
}

// запрос значения -------------------
unsigned int timers_get(unsigned char t){
  if (t < timer_cnt)
    return timer[t];
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
  unsigned int start = milliSeconds;
  
  while ((milliSeconds - start) < msec)
    wdt_reload();
}

// счетчик миллисекунд -------------------------------
unsigned int timers_getMillis(void){
  return milliSeconds;
}