/* Настоящий UART и gsm_read: URC хранится до main без stash. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t Int8U;typedef uint16_t Int16U;typedef uint8_t *pInt8U;
/* TYPES */
#define GSM_PORT 0
#define DEBUG_GSM
#define GSMRESPTIMEOUT 500
static uart_t uart[UARTS];static GsmStr GSM;static char gsm_strbuf[128];static uint8_t rx[2048],data[4096];static unsigned datalen,clock_ms,mask;
static const char *reply;static unsigned replylen;
static uint32_t __get_PRIMASK(void){return mask;}static void __disable_irq(void){mask=1;}static void __set_PRIMASK(uint32_t v){mask=v;}
static uint32_t timers_getMillis(void){return clock_ms++;}static void delay_ms(unsigned n){(void)n;}static void wdt_reload(void){}
static void append(const char *p,unsigned n){CHECK(uart[0].received+n<=sizeof(rx));memcpy(rx+uart[0].received,p,n);uart[0].received+=(uint16_t)n;uart[0].state=1;}
static void uart_sendstr(Int8U n,char *s){(void)n;(void)s;append(reply,replylen);}
static void gsm_dataread(gsmresp_e op,char *p,char *q,uint32_t n,int32_t cid){(void)q;(void)cid;if(op==DATA){CHECK(datalen+n<=sizeof(data));memcpy(data+datalen,p,n);datalen+=n;}}
static void gsm_readsms(uint8_t n){(void)n;}static void gsm_disconnect(uint8_t n){(void)n;}
char *gsm_runcmd(char*,char*);
#define gsm_debug(a) ((void)(a))
#define gsm_sendStrAndWaitResp(a,b,c,d) uart_sendStrAndWaitResp(GSM_PORT,a,b,sizeof(gsm_strbuf),d,c)
/* PRODUCTION */
int main(void){uart[0].rxbuffer=rx;uart[0].bfsize=sizeof(rx);GSM.moduleType=M_UG95;
 append("+IPD,2:AB",9);reply="+IPD,3:C\0D\r\nOK\r\n";replylen=16;CHECK(gsm_runcmd("AT\r","OK"));
 reply="\r\n+CREG: 0,1\r\nOK\r\n";replylen=(unsigned)strlen(reply);CHECK(gsm_runcmd("AT\r","OK"));
 reply="";replylen=0;unsigned started=clock_ms;CHECK(!gsm_runcmd("AT\r","OK")&&clock_ms-started>=500);
 gsm_read_uart();CHECK(datalen==5&&!memcmp(data,"ABC\0D",5)&&uart[0].received==0);
 for(unsigned cut=1;cut<9;cut++){gsm_rxreset();datalen=0;append("+IPD,2:AB",cut);gsm_keepurc();append(&"+IPD,2:AB"[cut],9-cut);gsm_read_uart();CHECK(datalen==2&&!memcmp(data,"AB",2));}
 char raw[1930];unsigned n=(unsigned)sprintf(raw,"+IPD,1900:");memset(raw+n,'x',1900);append(raw,n+1900);reply="OK\r\n";replylen=4;CHECK(gsm_runcmd("AT\r","OK"));datalen=0;gsm_read_uart();CHECK(datalen==1900&&!uart[0].received);
 gsm_rxreset();puts("gsmurc: real UART/parser, delayed binary URCs, partial headers, large payload and stale OK PASS");return 0;}
