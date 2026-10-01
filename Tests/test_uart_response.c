/* Полный RX не является строкой: запрещено находить ответ в соседней области. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
/* TYPES */
static uart_t uart[UARTS];
static uint8_t storage[2052];
static uint32_t clock_ms;
static bool present;
static bool noise;
static uint32_t irqmask;
uint32_t __get_PRIMASK(void){return irqmask;}
void __disable_irq(void){irqmask=1;}
void __set_PRIMASK(uint32_t m){irqmask=m;}
static uint32_t timers_getMillis(void) {
  uint32_t now=clock_ms++;
  CHECK(clock_ms<1000);
  if(noise && now){uart[0].received++;uart[0].state=0;uart[0].receiving=1;return now;}
  if(now==1){uart[0].received=2048;uart[0].state=1;uart[0].receiving=0;if(present){storage[2046]='O';storage[2047]='K';}}
  return now;
}
static void delay_ms(unsigned n){(void)n;}
static void wdt_reload(void){}
static void uart_sendstr(uint8_t n,char *s){(void)n;(void)s;}
/* PRODUCTION */
static void reset(bool good) {
  memset(uart,0,sizeof(uart));memset(storage,'x',sizeof(storage));memcpy(storage+2048,"OK",3);
  uart[0].rxbuffer=storage;uart[0].bfsize=2048;present=good;clock_ms=0;
}
int main(void){
  reset(false);noise=true;CHECK(!uart_waitresp(0,10,"OK"));CHECK(clock_ms<20);
  reset(false);CHECK(!uart_sendStrAndWaitResp(0,"AT\r",NULL,0,10,"OK"));CHECK(clock_ms<20);noise=false;
  reset(false);CHECK(!uart_waitresp(0,10,"OK"));
  reset(false);char rec[129];memset(rec,0x55,sizeof(rec));CHECK(!uart_sendStrAndWaitResp(0,"AT\r",rec,128,10,"OK"));CHECK(rec[127]==0 && rec[128]==0x55);
  reset(true);CHECK(uart_waitresp(0,10,"OK"));
  reset(true);CHECK(uart_sendStrAndWaitResp(0,"AT\r",NULL,0,10,"OK"));
  reset(true);CHECK(uart_waitresp(0,10,NULL));
  reset(false);CHECK(!uart_waitresp(UARTS,10,"OK"));
  puts("uartresponse: full 2048-byte RX, guard pattern and bounded answer PASS");return 0;
}
