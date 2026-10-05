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
static uint16_t crc16_calcInit(const uint8_t *p,uint16_t n,uint16_t c){unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
static uint16_t crc16_calc(const uint8_t *p,uint16_t n){return crc16_calcInit(p,n,0xffff);}
void user_parse_comand(uint8_t *p,unsigned ch){CHECK(ch==CH_SERVER);if(calls<8)seen[calls]=p[0]|(p[1]<<8);if(!nested_len||!calls)CHECK(!memcmp(p,expected+4,expected_size-4));calls++;if(reset_in_parse)server_rx_reset();if(nested){now+=nested_advance;const uint8_t*q=nested;nested=0;user_dataconnection(2,(uint8_t*)q,nested_len);CHECK(server_rx_count<=2048 && server_rx_count>=expected_size);CHECK(!memcmp(p,expected+4,expected_size-4));}}
static unsigned rx_move_calls, rx_move_bytes;
static void *rx_move(void *d, const void *s, size_t n) {
  rx_move_calls++; rx_move_bytes += (unsigned)n; return memmove(d,s,n);
}
#define memmove rx_move
/* PRODUCTION */
#undef memmove

static void frame(unsigned n){unsigned i;uint16_t c;expected_size=n;memcpy(expected,"SLDN",4);expected[4]=(n-8)&255;expected[5]=(n-8)>>8;for(i=6;i<n-2;i++)expected[i]=(uint8_t)i;if(n>30)memcpy(expected+17,"SLDN",4);c=crc16_calc(expected+4,n-6);expected[n-2]=c&255;expected[n-1]=c>>8;}
static void reset(void){server_rx_reset();server_rx_begin_pass();server_rx_defer=0;reset_in_parse=0;nested_advance=0;nested=0;calls=0;now=0;}
/* One simulated main pass, with hard work and compaction accounting. */
static void poll(void) {
  server_rx_begin_pass(); rx_move_calls=rx_move_bytes=0;
  server_rx_process();
  CHECK(server_rx_work<=SERVER_RX_WORK);
  CHECK(rx_move_calls<=2 && rx_move_bytes<=4096);
}
static void drain(void) {
  unsigned limit=20000;
  do {
    uint16_t count=server_rx_count, pos=server_rx_pos, off=server_rx_offset;
    unsigned before=calls;
    poll(); CHECK(limit--);
    if(count==server_rx_count && pos==server_rx_pos && off==server_rx_offset && before==calls) break;
  } while(server_rx_count);
}
static void finish_scan(void) {
  unsigned limit=20000;
  do {
    uint16_t count=server_rx_count, pos=server_rx_pos, off=server_rx_offset;
    server_rx_begin_pass(); rx_move_calls=rx_move_bytes=0;server_rx_scan();
    CHECK(limit-- && rx_move_calls<=1 && rx_move_bytes<=2048);
    if(count==server_rx_count && pos==server_rx_pos && off==server_rx_offset) break;
  } while(!server_rx_ready && server_rx_count);
}
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
    rx_move_calls=rx_move_bytes=0; finish_scan();
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
    finish_scan();
    CHECK(!server_rx_ready && server_rx_count==tail);
    CHECK(!memcmp(server_rx_buffer,suffix,tail));
    CHECK(rx_move_calls<=1 && rx_move_bytes<=tail);
  }
}

static void nested_headers(void) {
  uint8_t copy[2048]; uint16_t n=2048,ready=0;
  reset();memset(server_rx_buffer,0xa5,2048);
  for(unsigned i=0;i<2000;i+=8) {
    memcpy(server_rx_buffer+i,"SLDN",4);
    unsigned body=2048-i-8;
    server_rx_buffer[i+4]=body;server_rx_buffer[i+5]=body>>8;
  }
  memcpy(copy,server_rx_buffer,2048);reference_scan(copy,&n,&ready);
  server_rx_count=2048;
  server_rx_scan(); CHECK(server_rx_count && server_rx_total && !calls);
  uint16_t pos=server_rx_pos,work=server_rx_work;
  for(unsigned i=0;i<4096;i++) {server_rx_process();user_dataconnection(2,copy,0);}
  CHECK(server_rx_pos==pos && server_rx_work==work); /* no budget refill */
  finish_scan(); CHECK(server_rx_ready==ready && server_rx_count==n);
  CHECK(!memcmp(copy,server_rx_buffer,n));
}
int main(void) {
  uint8_t bad[2048], noise[]={0,'S','S','L',0,'S','L','D','N',255,255};
  for(unsigned n=11;n<=2048;n+=(n==11?1026:1011)) {
    frame(n);
    for(unsigned split=1;split<n;split++) {
      reset();user_dataconnection(2,expected,split);drain();CHECK(!calls);
      user_dataconnection(2,expected+split,n-split);drain();CHECK(calls==1&&!server_rx_count);
      for(unsigned i=0;i<3;i++)poll();CHECK(calls==1);
    }
  }
  frame(1037);reset();for(unsigned i=0;i<1037;i++)user_dataconnection(2,expected+i,1);
  CHECK(!calls);drain();CHECK(calls==1);
  reset();user_dataconnection(2,noise,sizeof(noise));user_dataconnection(2,expected,1037);drain();CHECK(calls==1);
  memcpy(bad,expected,1037);bad[1036]^=1;
  reset();user_dataconnection(2,bad,1037);drain();CHECK(!calls);
  user_dataconnection(2,expected,1037);drain();CHECK(calls==1);
  frame(20);memcpy(bad,expected,20);memcpy(bad+20,expected,20);
  reset();user_dataconnection(2,bad,40);CHECK(calls<=1);drain();CHECK(calls==2);
  reset();server_rx_defer=1;user_dataconnection(2,bad,27);CHECK(!calls);
  poll();CHECK(calls==1&&server_rx_count==7);user_dataconnection(2,bad+27,13);drain();CHECK(calls==2);
  reset();user_dataconnection(2,expected,5);user_dataconnection(3,expected,20);CHECK(server_rx_count==5);
  user_dataconnection(2,expected+5,15);drain();CHECK(calls==1);
  for(unsigned wrap=0;wrap<2;wrap++) {
    reset();now=wrap?0xfffffff0u:0;user_dataconnection(2,expected,5);
    now+=SERVER_RX_TIMEOUT_MS;poll();CHECK(!server_rx_count&&!calls);
  }
  /* Stream stops during CRC; next service passes complete or cancel it. */
  frame(2048);reset();user_dataconnection(2,expected,2048);CHECK(!calls&&server_rx_total);
  drain();CHECK(calls==1&&!server_rx_count);
  reset();user_dataconnection(2,expected,2048);server_rx_reset();drain();CHECK(!calls);
  user_dataconnection(2,expected,2048);drain();CHECK(calls==1);
  reset();user_dataconnection(2,expected,2048);now+=SERVER_RX_TIMEOUT_MS;poll();CHECK(!calls&&!server_rx_count&&!server_rx_total);
  /* Provisional modem frame cannot execute, including a full buffer. */
  reset();server_rx_provisional=server_rx_defer=1;user_dataconnection(2,expected,2048);
  for(unsigned i=0;i<20;i++)poll();CHECK(!calls);
  server_rx_provisional=0;drain();CHECK(calls==1);
  /* Full QIRD buffer remains owned until drained. Legacy excess is dropped. */
  reset();user_dataconnection(2,expected,2048);bad[0]='X';user_dataconnection(2,bad,1);
  CHECK(server_rx_count==2048);drain();CHECK(calls==1&&!server_rx_count);
  frame(30);memcpy(bad,expected,30);frame(20);
  for(unsigned elapsed=29999;elapsed<=30001;elapsed++) {
    reset();nested=bad;nested_len=30;nested_advance=elapsed;
    user_dataconnection(2,expected,20);CHECK(calls==1);drain();
    CHECK(calls==2&&seen[0]==12&&seen[1]==22&&!server_rx_count&&!server_rx_busy);
  }
  nested_len=0;reset();server_rx_defer=1;user_dataconnection(2,expected,20);
  reset_in_parse=1;poll();CHECK(calls==1&&!server_rx_count);poll();CHECK(calls==1);
  scan_cost();scan_equivalence();nested_headers();
  /* Long junk over many full buffers, including artificial chunk suffixes. */
  reset();memset(bad,'S',sizeof(bad));
  for(unsigned i=0;i<20;i++){user_dataconnection(2,bad,2048);drain();CHECK(server_rx_count==1&&!calls);}
  puts("server RX: bounded passes, 5000 differential scans, exhaustive splits 11/1037/2048, stalled CRC, nested headers, noise, reset/timeout/wrap/provisional/full buffer/nested input: PASS");
  return 0;
}
