#include <stdint.h>
#include <stdbool.h>
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
/* Ответ серверу ждёт свободного слота канала; в модели слот свободен. */
static bool comm_tx_busy(void){return false;}
void user_dataconnection(int8_t,uint8_t*,uint16_t);
static void server_rx_reset(void);
static uint8_t expected[2048];
static uint16_t expected_size;
uint32_t timers_getMillis(void){return now;}
static uint16_t crc16_calc(const uint8_t *p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
void user_parse_comand(uint8_t *p,unsigned ch){CHECK(ch==CH_SERVER);if(calls<8)seen[calls]=p[0]|(p[1]<<8);if(!nested_len||!calls)CHECK(!memcmp(p,expected+4,expected_size-4));calls++;if(reset_in_parse)server_rx_reset();if(nested){now+=nested_advance;const uint8_t*q=nested;nested=0;user_dataconnection(2,(uint8_t*)q,nested_len);CHECK(server_rx_count<=2048 && server_rx_count>=expected_size);CHECK(!memcmp(p,expected+4,expected_size-4));}}
static unsigned rx_move_calls, rx_move_bytes;
static void *rx_move(void *d, const void *s, size_t n) {
  rx_move_calls++; rx_move_bytes += (unsigned)n; return memmove(d,s,n);
}
#define memmove rx_move
/* PRODUCTION */
#undef memmove

static void frame(unsigned n){unsigned i;uint16_t c;expected_size=n;memcpy(expected,"SLDN",4);expected[4]=(n-8)&255;expected[5]=(n-8)>>8;for(i=6;i<n-2;i++)expected[i]=(uint8_t)i;if(n>30)memcpy(expected+17,"SLDN",4);c=crc16_calc(expected+4,n-6);expected[n-2]=c&255;expected[n-1]=c>>8;}
static void reset(void){server_rx_reset();server_rx_defer=0;reset_in_parse=0;nested_advance=0;nested=0;calls=0;now=0;}
static void reference_scan(uint8_t *p, uint16_t *count, uint16_t *ready) {
  while(*count) {
    uint16_t body=0,total=0;
    if(!memcmp(p,"SLDN",*count<4?*count:4)) {
      if(*count<6)return;
      body=p[4]|p[5]<<8;
      if(body>=3 && body<=2040) {
        total=body+8;
        if(*count<total)return;
        if(crc16_calc(p+4,body+2)==(p[total-2]|p[total-1]<<8)) {*ready=total;return;}
      }
    }
    --*count; memmove(p,p+1,*count);
  }
}
static void scan_equivalence(void) {
  uint8_t copy[2048]; uint32_t seed=0x20261005;
  unsigned trial,i,n,at;
  frame(20);
  for(trial=0;trial<5000;trial++) {
    uint16_t count,ready=0;
    reset(); seed=seed*1664525u+1013904223u; n=seed%257;
    for(i=0;i<n;i++) {seed=seed*1664525u+1013904223u;server_rx_buffer[i]=seed>>24;}
    if(n && trial%3) {
      at=trial%n;
      memcpy(server_rx_buffer+at,expected,n-at<20?n-at:20);
      if(trial%5==0 && n-at>=20)server_rx_buffer[at+19]^=1;
      if(trial%7==0 && n-at>=6)server_rx_buffer[at+5]=255;
    }
    count=server_rx_count=n; memcpy(copy,server_rx_buffer,n);
    reference_scan(copy,&count,&ready);
    rx_move_calls=rx_move_bytes=0; server_rx_scan();
    CHECK(server_rx_count==count && server_rx_ready==ready);
    CHECK(!memcmp(server_rx_buffer,copy,count));
    CHECK(rx_move_calls<=1 && rx_move_bytes<=count);
  }
  reset(); memcpy(server_rx_buffer,expected,20);server_rx_count=20;
  server_rx_busy=1;server_rx_scan();CHECK(!server_rx_ready && server_rx_count==20);
  server_rx_busy=0;server_rx_ready=20;server_rx_scan();CHECK(server_rx_count==20 && server_rx_ready==20);
}
/* Worst case: full corrupt frame followed by a partial signature. */
static void scan_cost(void) {
  unsigned tail;
  for(tail=0;tail<=5;tail++) {
    uint8_t suffix[5]={'S','L','D','N',12};
    reset(); memset(server_rx_buffer,0xa5,2048);
    memcpy(server_rx_buffer,"SLDN",4);
    server_rx_buffer[4]=0xf8; server_rx_buffer[5]=7;
    if(tail)memcpy(server_rx_buffer+2048-tail,suffix,tail);
    CHECK(crc16_calc(server_rx_buffer+4,2042)!=(server_rx_buffer[2046]|server_rx_buffer[2047]<<8));
    server_rx_count=2048; rx_move_calls=rx_move_bytes=0;
    server_rx_scan();
    CHECK(!server_rx_ready && server_rx_count==tail);
    CHECK(!memcmp(server_rx_buffer,suffix,tail));
    CHECK(rx_move_calls<=1 && rx_move_bytes<=tail);
  }
}
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
 scan_cost();scan_equivalence();
 puts("server RX: 5000 differential scans + bounded compaction, all splits of 11/1037/2048-byte frames, bytewise input, embedded signature, noise, CRC, concatenation, foreign handle, timeout/wrap/reset, deferred execution (order, tail, overflow, reset in parse, nested input during parse) passed");return 0;}
