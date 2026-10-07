// Доработка: Кряжев А., 2026
// Кадр W26 передаётся после атомарного снимка без потери нового фронта.
// Прерывания моделируются на границах операций чтения и сброса.
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
bool flash_exec_read_allowed(void){return true;}
bool user_access_ready(const char *id){CHECK(id[10] == 0); return true;}
/* КОД */
static void reset(void){wgData=wgTime=wgID=wg_overflows=0;wgBitCount=wg_head=wg_count=0;clock_ms=100;irqmask=0;seen=0;}
static void bits(unsigned n){for(unsigned i=0;i<n;i++){if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();}}
int main(void){
  reset();bits(282);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
  reset();bits(538);clock_ms=400;WG_process();WG_process();CHECK(seen==0);
  reset();bits(26);uint32_t expected=(wgData>>1)&0x03FFFFFF;clock_ms=400;WG_process();CHECK(seen==1 && wgBitCount==0 && lastid==expected);WG_process();CHECK(seen==1);
  bits(26);clock_ms=700;irqmask=1;WG_process();CHECK(irqmask==1 && seen==2);irqmask=0;WG_process();CHECK(seen==2);
  reset();clock_ms=0xffffff00;bits(26);clock_ms=0x10;WG_process();WG_process();CHECK(seen==1);
  /* SIA: интервал между битами до 20 мс не рвёт кадр, тишина больше 25 мс завершает его;
     опрос на каждом проходе цикла, карта отдаётся текущим вызовом. */
  reset();for(unsigned i=0;i<26;i++){clock_ms+=20;if(i&1)EXTI3_IRQHandler();else EXTI2_IRQHandler();WG_process();CHECK(!seen);}
  clock_ms+=25;WG_process();WG_process();CHECK(!seen);
  clock_ms+=1;WG_process();CHECK(seen==1);WG_process();CHECK(seen==1);
  reset();bits(26);clock_ms=100+26;WG_process();WG_process();CHECK(seen==1); /* 26 мс после последнего бита - уже карта */
  for(unsigned bit=0;bit<26;bit++){
    reset();wgData=1u^(1u<<bit);wgBitCount=26;clock_ms=400;
    WG_process();CHECK(seen==!WG_PARITY);WG_process();CHECK(seen==!WG_PARITY);
  }
  for(uint32_t id=0;id<0x1000000u;id++){
    unsigned upper=0,lower=0;
    for(unsigned k=0;k<12;k++){upper^=(id>>(k+12))&1u;lower^=(id>>k)&1u;}
    uint32_t frame=(upper<<25)|(id<<1)|(lower^1u);
    CHECK(wg_parity_valid(frame));
    wgData=frame;wgBitCount=26;wgTime=100;clock_ms=400;
    unsigned before=seen;WG_process();CHECK(seen==before+1 && wgID==((frame>>1)&0x03FFFFFFu));
    for(unsigned bit=0;bit<26;bit++)CHECK(!wg_parity_valid(frame^(1u<<bit)));
  }
  puts("wiegandirq: 2^24 valid frames preserve original ID, all 26 single-bit errors rejected; noise, IRQ mask and timing PASS");return 0;
}
