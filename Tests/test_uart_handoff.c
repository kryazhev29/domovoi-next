/* ISR добавляет байты при malloc/memcpy: владелец снимка не должен очищать новый хвост. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
typedef uint8_t *pInt8U;
/* TYPES */
#define GSM_PORT 0
static uart_t uart[UARTS];
static uint8_t rx[2048];
static GsmStr GSM;
static char gsm_strbuf[128];
static uint8_t server_rx_defer;
static unsigned server_processes;
static uint8_t data[64];
static unsigned datalen;
static uint32_t irqmask,clock_ms;
static bool alloc_edge,copy_edge,pending_edge;
static bool noise_receive;
static unsigned noise_ticks;
bool uart_addbyte(Int8U n,Int8U ch);
void gsm_read(char*,Int16U);
uint32_t __get_PRIMASK(void){return irqmask;}
void __disable_irq(void){irqmask=1;}
void __set_PRIMASK(uint32_t m){irqmask=m;if(!m && pending_edge){pending_edge=false;CHECK(uart_addbyte(0,'I'));}}
static void *race_calloc(size_t n,size_t s){if(alloc_edge){alloc_edge=false;CHECK(uart_addbyte(0,'+'));}return calloc(n,s);}
static void *race_realloc(void *p,size_t n){if(alloc_edge){alloc_edge=false;CHECK(uart_addbyte(0,'+'));}return realloc(p,n);}
static void *race_memcpy(void *to,const void *from,size_t n){if(copy_edge){copy_edge=false;if(irqmask)pending_edge=true;else CHECK(uart_addbyte(0,'I'));}return memcpy(to,from,n);}
static uint32_t timers_getMillis(void){CHECK(clock_ms<10000);return clock_ms++;}
static void wdt_reload(void){if(noise_receive){CHECK(++noise_ticks<50);CHECK(uart_addbyte(0,'x'));}}
static void delay_ms(unsigned n){(void)n;}
static void uart_sendstr(uint8_t n,char *s){(void)n;(void)s;}
static void server_rx_process(void){server_rx_defer=0;server_processes++;}
static void gsm_dataread(gsmresp_e op,char *p,char *q,uint32_t n,int32_t cid){(void)q;(void)cid;if(op==DATA){CHECK(datalen+n<=sizeof(data));race_memcpy(data+datalen,p,n);datalen+=n;}}
static void gsm_readsms(uint8_t n){(void)n;}
static void gsm_disconnect(uint8_t n){(void)n;}
static char *gsm_runcmd(char *s,char *r){(void)s;(void)r;return gsm_strbuf;}
#define calloc race_calloc
#define realloc race_realloc
#define memcpy race_memcpy
/* PRODUCTION */
#undef calloc
#undef realloc
#undef memcpy
static void push(const char *s){while(*s)CHECK(uart_addbyte(0,(uint8_t)*s++));}
static void ready(void){uart[0].twait=0;uart[0].receiving=0;uart[0].state=1;}
int main(void){
  (void)gsm_stashurc;
  memset(uart,0,sizeof(uart));uart[0].rxbuffer=rx;uart[0].bfsize=sizeof(rx);uart[0].timetowait=10;GSM.ready=1;GSM.moduleType=M_UG95;
  push("+IPD,2:AB");ready();noise_receive=true;gsm_check_data();noise_receive=false;CHECK(noise_ticks==0 && datalen==2);gsm_rxreset();uart_clear(0);datalen=0;server_processes=0;
  push("+IPD,2:AB");ready();alloc_edge=true;copy_edge=true;
  gsm_check_data();CHECK(irqmask==0);CHECK(datalen==2 && memcmp(data,"AB",2)==0);
  CHECK(uart[0].received==2 && rx[0]=='+' && rx[1]=='I');
  push("PD,2:CD");ready();gsm_check_data();CHECK(datalen==4 && memcmp(data,"ABCD",4)==0 && uart[0].received==0);
  CHECK(server_processes==4 && server_rx_defer==0);
#ifdef HAVE_TAKE
  push("abc");uint8_t out[4]={0};irqmask=1;CHECK(uart_takerx(0,out,2)==2);CHECK(irqmask==1 && out[0]=='a' && out[1]=='b' && uart[0].received==1 && rx[0]=='c');
  CHECK(uart_takerx(0,out,0)==0 && uart[0].received==1);CHECK(uart_takerx(UARTS,out,4)==0);__set_PRIMASK(0);uart_clear(0);
  push("+IPD,2:EF");ready();gsm_stashurc();CHECK(uart[0].received==0);push("+IPD,2:GH");CHECK(uart_sendStrAndWaitResp(0,"AT\r",NULL,0,0,NULL));CHECK(uart[0].received==9);
  gsm_check_data();ready();gsm_check_data();CHECK(datalen==8 && memcmp(data,"ABCDEFGH",8)==0);
  irqmask=1;uart_clear(0);CHECK(irqmask==1);__set_PRIMASK(0);
#endif
  gsm_rxreset();puts("uarthandoff: real gsm_read, allocation/copy IRQ edges, prefix ownership and PRIMASK PASS");return 0;
}
