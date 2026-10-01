/* Таймеры передают main снимок под сохранённым PRIMASK. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t irqmask;
static bool pending;
void timers_handle(void);
unsigned int timers_getMillis(void);
uint32_t __get_PRIMASK(void){return irqmask;}
void __disable_irq(void){irqmask=1;}
void __set_PRIMASK(uint32_t mask){irqmask=mask;if(pending && !mask){pending=false;timers_handle();}}
void timers_user_handle(void){}
void wdt_reload(void){timers_handle();}
/* PRODUCTION */
int main(void){
  timers_set(0,9);CHECK(timers_get(0)==9);timers_handle();CHECK(timers_get(0)==10 && timers_getMillis()==1);
  irqmask=1;timers_set(0,3);CHECK(irqmask==1 && timers_get(0)==3 && irqmask==1 && timers_getMillis()==1 && irqmask==1);irqmask=0;
  pending=true;CHECK(timers_get(0)==3 && timer[0]==4 && !pending);
  pending=true;unsigned now=timers_getMillis();CHECK(milliSeconds==now+1 && !pending);
  milliSeconds=0xfffffffeu;timers_pause(5);CHECK(timers_getMillis()==3);
  unsigned save=timer[0];timers_set(10,99);CHECK(timer[0]==save && timers_get(10)==0);
  puts("timersirq: snapshot, pending tick, saved PRIMASK and pause wrap PASS");return 0;
}
