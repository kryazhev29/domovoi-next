/* Доработка: Кряжев А., 2026. Выходы: реле включается сразу, импульс и пауза в секундах
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
   по Протокол2 отсчитываются тактами 100 мс. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
static int pin[2];
#define OUT1_ON (pin[0]=1)
#define OUT1_OFF (pin[0]=0)
#define OUT1_IN (pin[0]!=0)
#define OUT2_ON (pin[1]=1)
#define OUT2_OFF (pin[1]=0)
#define OUT2_IN (pin[1]!=0)
#define OUTSCOUNT 2
typedef enum{OUT_OFF=0,OUT_ON,OUT_INV,OUT_IMP}outmode_e;
/* КОД */
// Обнулить модель уровней и состояния обоих реле.
static void reset(void){for(int i=0;i<2;i++){pin[i]=0;out_time[i]=out_period[i]=out_timeactiv[i]=out_timepause[i]=0;}out_activpause=0xFF;out_st=0;}
/* Число вызовов по 100 мс до смены состояния, включая вызов, который его меняет:
   как и на прежних секундных тактах, A секунд = 10*A тактов отсчёта + 1. */
// Продвинуть алгоритм реле на заданное число шагов по 100 мс.
static unsigned run_while(int out,int s,unsigned limit){unsigned n=0;while(pin[out]==s&&n<limit){out_process();n++;}return n;}
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){
  /* Один импульс 3 с: включён сразу при загрузке, 3,0-3,1 с, потом выключен навсегда. */
  reset();out_load(0,OUT_IMP,3,2,1);CHECK(pin[0]==1);
  CHECK(run_while(0,1,1000)==31);CHECK(pin[0]==0);for(int i=0;i<100;i++)out_process();CHECK(pin[0]==0);
  /* Три импульса 1 с с паузой 2 с. */
  reset();out_load(1,OUT_IMP,1,2,3);CHECK(pin[1]==1);
  /* Первый импульс включён ещё до отсчёта, поэтому на такт длиннее (не короче заданного);
     повторы и паузы - ровно по настройке. */
  CHECK(run_while(1,1,1000)==11);CHECK(run_while(1,0,1000)==20);
  CHECK(run_while(1,1,1000)==10);CHECK(run_while(1,0,1000)==20);
  CHECK(run_while(1,1,1000)==10);CHECK(run_while(1,0,1000)==1000);
  /* Длинный импульс без паузы: 255 с не переполняет счёт. */
  reset();out_load(0,OUT_IMP,255,0,1);CHECK(pin[0]==1);CHECK(run_while(0,1,100000)==2550);CHECK(pin[0]==0);
  /* Ноль повторов или нулевое время - выход не трогается, как раньше. */
  reset();out_load(0,OUT_IMP,3,1,0);CHECK(pin[0]==0);for(int i=0;i<100;i++)out_process();CHECK(pin[0]==0);
  reset();out_load(0,OUT_IMP,0,1,1);CHECK(pin[0]==0);
  /* Уровень: включить и инвертировать - сразу, без отсчёта. */
  reset();out_load(1,OUT_ON,0,0,0);CHECK(pin[1]==1);out_load(1,OUT_INV,0,0,0);CHECK(pin[1]==0);
  puts("outs: relay on at load, impulse/pause seconds as 100 ms ticks, repeats, 255 s, zero cases, level modes PASS");
  return 0;
}
