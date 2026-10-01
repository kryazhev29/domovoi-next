#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if (!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
#define CH_SERVER 1
static struct {int8_t connectionhandle;} device={2};
static uint32_t now;
static unsigned calls;
static int reset_in_parse;
static uint32_t nested_advance;
static const uint8_t *nested;static unsigned nested_len;static unsigned seen[8];
static uint16_t server_rx_count;
void user_dataconnection(int8_t,uint8_t*,uint16_t);
static void server_rx_reset(void);
static uint8_t expected[2048];
static uint16_t expected_size;
uint32_t timers_getMillis(void){return now;}
static uint16_t crc16_calc(const uint8_t *p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
void user_parse_comand(uint8_t *p,unsigned ch){CHECK(ch==CH_SERVER);if(calls<8)seen[calls]=p[0]|(p[1]<<8);if(!nested_len||!calls)CHECK(!memcmp(p,expected+4,expected_size-4));calls++;if(reset_in_parse)server_rx_reset();if(nested){now+=nested_advance;const uint8_t*q=nested;nested=0;user_dataconnection(2,(uint8_t*)q,nested_len);CHECK(server_rx_count<=2048 && server_rx_count>=expected_size);CHECK(!memcmp(p,expected+4,expected_size-4));}}
/* PRODUCTION */
static void frame(unsigned n){unsigned i;uint16_t c;expected_size=n;memcpy(expected,"SLDN",4);expected[4]=(n-8)&255;expected[5]=(n-8)>>8;for(i=6;i<n-2;i++)expected[i]=(uint8_t)i;if(n>30)memcpy(expected+17,"SLDN",4);c=crc16_calc(expected+4,n-6);expected[n-2]=c&255;expected[n-1]=c>>8;}
static void reset(void){server_rx_reset();server_rx_defer=0;reset_in_parse=0;nested_advance=0;nested=0;calls=0;now=0;}
int main(void){unsigned n,split,i;uint8_t bad[2048],noise[]={0,'S','S','L',0,'S','L','D','N',255,255};
 for(n=11;n<=2048;n+=(n==11?1026:1011)){
  frame(n);
  for(split=1;split<n;split++){reset();user_dataconnection(2,expected,split);CHECK(!calls);user_dataconnection(2,expected+split,n-split);CHECK(calls==1&&!server_rx_count);}
 }
 frame(1037);reset();for(i=0;i<expected_size;i++)user_dataconnection(2,expected+i,1);CHECK(calls==1);
 reset();user_dataconnection(2,noise,sizeof(noise));user_dataconnection(2,expected,expected_size);CHECK(calls==1);
 memcpy(bad,expected,expected_size);bad[expected_size-1]^=1;reset();user_dataconnection(2,bad,expected_size);CHECK(!calls);user_dataconnection(2,expected,expected_size);CHECK(calls==1);
 frame(20);memcpy(bad,expected,20);memcpy(bad+20,expected,20);reset();user_dataconnection(2,bad,40);CHECK(calls==2);
 reset();user_dataconnection(2,expected,5);user_dataconnection(3,expected,20);CHECK(!calls&&server_rx_count==5);user_dataconnection(2,expected+5,15);CHECK(calls==1);
 reset();user_dataconnection(2,expected,5);now=SERVER_RX_TIMEOUT_MS;user_dataconnection(2,expected,20);CHECK(calls==1);
 reset();now=0xfffffff0u;user_dataconnection(2,expected,5);now+=SERVER_RX_TIMEOUT_MS;user_dataconnection(2,expected,20);CHECK(calls==1);
 reset();user_dataconnection(2,expected,5);server_rx_reset();user_dataconnection(2,expected,20);CHECK(calls==1);
 /* Отложенный режим (01.10.2026): кадр ждёт server_rx_process, команды идут по порядку. */
 frame(20);reset();server_rx_defer=1;user_dataconnection(2,expected,20);CHECK(!calls&&server_rx_ready==20);server_rx_process();CHECK(calls==1&&!server_rx_count&&!server_rx_defer);
 memcpy(bad,expected,20);memcpy(bad+20,expected,20);reset();server_rx_defer=1;user_dataconnection(2,bad,40);CHECK(!calls);server_rx_process();CHECK(calls==2&&!server_rx_count);
 reset();server_rx_defer=1;user_dataconnection(2,bad,27);CHECK(!calls);server_rx_process();CHECK(calls==1&&server_rx_count==7&&!server_rx_ready);user_dataconnection(2,bad+27,13);CHECK(calls==2&&!server_rx_count);
 frame(1037);memcpy(bad,expected,1037);memcpy(bad+1037,expected,1011);reset();server_rx_defer=1;user_dataconnection(2,bad,2048);CHECK(!calls&&server_rx_count==2048);
 user_dataconnection(2,expected+1011,26);CHECK(calls==1&&server_rx_defer);server_rx_process();CHECK(calls==2&&!server_rx_count);
 frame(20);memcpy(bad,expected,20);memcpy(bad+20,expected,20);reset();server_rx_defer=1;user_dataconnection(2,bad,40);reset_in_parse=1;server_rx_process();CHECK(calls==1&&!server_rx_count&&!server_rx_ready);
 reset();server_rx_defer=1;user_dataconnection(2,expected,20);user_dataconnection(3,expected,20);CHECK(server_rx_count==20);server_rx_reset();CHECK(!server_rx_ready);server_rx_process();CHECK(!calls);
 frame(30);memcpy(bad,expected,30);frame(20);
 reset();nested=bad;nested_len=30;user_dataconnection(2,expected,20);CHECK(calls==2&&seen[0]==12&&seen[1]==22&&!server_rx_count&&!server_rx_busy);
 reset();server_rx_defer=1;user_dataconnection(2,expected,20);nested=bad;nested_len=30;server_rx_process();CHECK(calls==2&&seen[0]==12&&seen[1]==22&&!server_rx_count);
 frame(2048);reset();bad[0]='X';nested=bad;nested_len=1;user_dataconnection(2,expected,2048);CHECK(calls==1&&!server_rx_count&&!server_rx_busy);
 /* B14: таймаут не освобождает кадр, пока обработчик им владеет. */
 frame(30);memcpy(bad,expected,30);frame(20);
 for(unsigned deferred=0;deferred<2;deferred++) {
   for(unsigned elapsed=29999;elapsed<=30001;elapsed++) {
     reset();server_rx_defer=deferred;nested=bad;nested_len=30;nested_advance=elapsed;
     user_dataconnection(2,expected,20);if(deferred)server_rx_process();
     CHECK(calls==2&&seen[0]==12&&seen[1]==22&&!server_rx_count&&!server_rx_busy);
   }
 }
 reset();now=0xfffffff0u;nested=bad;nested_len=30;nested_advance=30000;
 user_dataconnection(2,expected,20);CHECK(calls==2&&!server_rx_count&&!server_rx_busy);
 reset();nested=bad;nested_len=0;nested_advance=30000;
 user_dataconnection(2,expected,20);CHECK(calls==1&&!server_rx_count&&!server_rx_busy);
 frame(2048);reset();bad[0]='X';nested=bad;nested_len=1;nested_advance=30000;
 user_dataconnection(2,expected,2048);CHECK(calls==1&&!server_rx_count&&!server_rx_busy);
 nested_len=0;
 puts("server RX: all splits of 11/1037/2048-byte frames, bytewise input, embedded signature, noise, CRC, concatenation, foreign handle, timeout/wrap/reset, deferred execution (order, tail, overflow, reset in parse, nested input during parse) passed");return 0;}
