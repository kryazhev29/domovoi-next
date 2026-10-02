/* Счётчик шума не возвращается к 26; приём и сброс кадра передают владение атомарно. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static struct {uint32_t PR;} exti;
#define EXTI (&exti)
#define EXTI_Line2 4
#define EXTI_Line3 8
static uint32_t clock_ms,irqmask,lastid;
static unsigned seen;
uint32_t __get_PRIMASK(void){return irqmask;}
void __disable_irq(void){irqmask=1;}
void __set_PRIMASK(uint32_t m){irqmask=m;}
uint32_t timers_getMillis(void){return clock_ms;}
void WG_readedID(uint32_t id){seen++;lastid=id;}
/* PRODUCTION */
static void reset(void){wgData=wgTime=wgID=0;wgBitCount=0;wgDataAvalible=false;clock_ms=100;irqmask=0;seen=0;}
static void bits(unsigned n){for(unsigned i=0;i<n;i++){if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();}}
int main(void){
  reset();bits(282);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
  reset();bits(538);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
  reset();bits(26);uint32_t expected=(wgData>>1)&0x03FFFFFF;clock_ms=400;WG_process();CHECK(seen==0 && wgDataAvalible && wgBitCount==0);WG_process();CHECK(seen==1 && lastid==expected && !wgDataAvalible);
  bits(26);clock_ms=700;irqmask=1;WG_process();CHECK(irqmask==1 && wgDataAvalible);irqmask=0;WG_process();CHECK(seen==2);
  reset();clock_ms=0xffffff00;bits(26);clock_ms=0x10;WG_process();WG_process();CHECK(seen==1);
  /* SIA: интервал между битами до 20 мс не рвёт кадр, тишина больше 25 мс завершает его;
     опрос на каждом проходе цикла, кадр отдаётся следующим вызовом. */
  reset();for(unsigned i=0;i<26;i++){clock_ms+=20;if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();WG_process();CHECK(!wgDataAvalible&&!seen);}
  clock_ms+=25;WG_process();WG_process();CHECK(!seen);
  clock_ms+=1;WG_process();CHECK(wgDataAvalible);WG_process();CHECK(seen==1&&!wgDataAvalible);
  reset();bits(26);clock_ms=100+26;WG_process();WG_process();CHECK(seen==1); /* 26 мс после последнего бита - уже карта */
  puts("wiegandirq: noise saturation, original ID mask/delivery, saved PRIMASK, wrap, 20 ms bit gap kept, 26 ms silence ends frame PASS");return 0;
}
