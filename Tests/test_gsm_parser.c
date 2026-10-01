/* Настоящий gsm_read; подменены только UART и внешние действия. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
/* TYPES */
static GsmStr GSM;
static char gsm_strbuf[128];
static char *begin, *end;
static unsigned events[48];
static uint8_t data[70000];
static unsigned datalen;
static int32_t lastid;
static bool check_bounds=true;
#define GSM_PORT 0
static uint8_t rx[2048];
static uint16_t rxlen;
static uint16_t uart_received(uint8_t n){(void)n;return rxlen;}
static uint8_t *uart_getptr(uint8_t n){(void)n;return rx;}
static uint16_t uart_rxview(uint8_t n,uint8_t **p){(void)n;*p=rx;return rxlen;}
uint16_t uart_takerx(uint8_t port,uint8_t *to,uint16_t cap){(void)port;uint16_t n=rxlen<cap?rxlen:cap;if(to)memcpy(to,rx,n);memmove(rx,rx+n,rxlen-n);rxlen-=n;return n;}
static void gsm_dataread(gsmresp_e op,char *p,char *q,uint32_t n,int32_t id) {
  (void)q;lastid=id;events[op]++;
#ifndef TEST_STREAM
  if (p!=NULL && p!=gsm_strbuf) CHECK(p>=begin && p<=end);
#endif
  if (op==DATA) { if(check_bounds){CHECK(p>=begin && p<=end); CHECK(n<=(uint32_t)(end-p));} CHECK(datalen+n<=sizeof(data));memcpy(data+datalen,p,n);datalen+=n; }
}
static void gsm_readsms(uint8_t n) {(void)n;}
static void gsm_disconnect(uint8_t n) {(void)n;}
static char *gsm_runcmd(char *s,char *r) {(void)s;(void)r;return gsm_strbuf;}
static bool fail_alloc;
static unsigned live_allocs;
static void *parser_calloc(size_t n,size_t s){if(fail_alloc){fail_alloc=false;return NULL;}void *p=calloc(n,s);if(p)live_allocs++;return p;}
static void *parser_realloc(void *p,size_t n){if(fail_alloc){fail_alloc=false;return NULL;}void *q=realloc(p,n);if(q && !p)live_allocs++;return q;}
static void parser_free(void *p){if(p){CHECK(live_allocs>0);live_allocs--;}free(p);}
#define calloc parser_calloc
#define realloc parser_realloc
#define free parser_free
/* PRODUCTION */
#undef calloc
#undef realloc
#undef free
static void feed(const char *s,unsigned n) {
  char *copy=calloc(n+48,1);CHECK(copy);copy[5]='7';copy[6]=',';copy[7]=' ';begin=copy+8;memcpy(begin,s,n);end=begin+n;gsm_read(begin,(uint16_t)n);free(copy);
}
static void reset(void) {
#ifdef TEST_STREAM
  gsm_rxreset();gsm_urclost=false;parser_free(gsm_urcbuf);gsm_urcbuf=NULL;gsm_urclen=0;CHECK(live_allocs==0);
#endif
  memset(&GSM,0,sizeof(GSM));GSM.ready=1;GSM.moduleType=M_UG95;memset(events,0,sizeof(events));datalen=0;
}
static void bounds(void) {
  const char *heads[]={"+IPD,13:","+QIURC: \"recv\",0,13\r\n"};
  for(unsigned h=0;h<2;h++) {
    char packet[128]={0};unsigned hl=(unsigned)strlen(heads[h]);memcpy(packet,heads[h],hl);memset(packet+hl,'x',13);
    for(unsigned n=1;n<hl+13;n++) {reset();feed(packet,n);
#ifdef TEST_STREAM
      CHECK(datalen==(n>hl?n-hl:0));
#else
      CHECK(datalen==0);
#endif
    }
    reset();feed(packet,hl+13);CHECK(datalen==13);
  }
  reset();const char *huge="+QIURC: \"recv\",0,65535\r\nx";feed(huge,(unsigned)strlen(huge));
#ifdef TEST_STREAM
  CHECK(datalen==1 && gsm_remaining==65534);
#else
  CHECK(datalen==0);
#endif
  reset();feed("+IPD,0:",7);CHECK(datalen==0);
  char large[2080]={0};const char *hdr="+IPD,2048:";unsigned hl=(unsigned)strlen(hdr);memcpy(large,hdr,hl);memset(large+hl,'x',2048);
  reset();feed(large,hl+2048);CHECK(datalen==2048);
}
static void short_lines(void) {
  const char *lines[]={"+CREG:","+CREG: ","+CREG: 1,","+CGREG:","+CGREG: ","+CGREG: 1,"};
  for(unsigned i=0;i<sizeof(lines)/sizeof(lines[0]);i++){reset();feed(lines[i],(unsigned)strlen(lines[i]));CHECK(events[CREG]+events[CGREG]==0);}
  reset();feed("+CREG: 1\r\n",10);CHECK(events[CREG]==1 && GSM.creg==1);
  reset();feed("+CGREG: 2,5\r\n",13);CHECK(events[CGREG]==1 && GSM.cgreg==5);
  reset();GSM.muxmode=1;feed("CLOSED\r\n",8);CHECK(events[CLOSECONNECT]==1 && lastid==0);
  reset();GSM.muxmode=1;feed("CONNECT OK\r\n",12);CHECK(events[CONNECTED]==0);
  reset();GSM.muxmode=1;feed("2, CONNECT OK\r\n",15);CHECK(events[CONNECTED]==1 && lastid==2);
}
static void binary_stream(void) {
  const char body[]={ 'A',0,'E','R','R','O','R',' ','R','I','N','G',' ','C','L','O','S','E','D',' ','O','K' };
  char packet[256];unsigned hl=(unsigned)sprintf(packet,"+QIURC: \"recv\",0,%u\r\n",(unsigned)sizeof(body));
  memcpy(packet+hl,body,sizeof(body));unsigned n=hl+sizeof(body);
  memcpy(packet+n,"+IPD,3:xyz\r\nOK\r\n",16);n+=16;
  reset();feed(packet,n);CHECK(datalen==sizeof(body)+3);CHECK(memcmp(data,body,sizeof(body))==0);CHECK(memcmp(data+sizeof(body),"xyz",3)==0);
  CHECK(events[ATERROR]==0 && events[RING]==0 && events[CLOSECONNECT]==0 && events[ATOK]==1);
  for(unsigned cut=1;cut<n;cut++){reset();feed(packet,cut);feed(packet+cut,n-cut);CHECK(datalen==sizeof(body)+3 && memcmp(data,body,sizeof(body))==0);CHECK(events[ATERROR]+events[RING]+events[CLOSECONNECT]==0 && events[ATOK]==1);}
  reset();for(unsigned i=0;i<n;i++)feed(packet+i,1);CHECK(datalen==sizeof(body)+3 && events[ATOK]==1);
#ifdef TEST_STREAM
  reset();memcpy(rx,packet,n);rxlen=(uint16_t)n;gsm_stashurc();check_bounds=false;gsm_takeurc();check_bounds=true;CHECK(datalen==sizeof(body)+3 && events[ATOK]==1);
  reset();memset(rx,'x',sizeof(rx));memcpy(rx,"+IPD",4);rxlen=2048;gsm_stashurc();rxlen=2048;gsm_stashurc();CHECK(gsm_urclost);gsm_takeurc();CHECK(events[CLOSECONNECT]==1 && gsm_urclen==0 && gsm_urcbuf==NULL);
  reset();memcpy(rx,packet,n);rxlen=(uint16_t)n;fail_alloc=true;gsm_stashurc();CHECK(gsm_urclost);gsm_takeurc();CHECK(events[CLOSECONNECT]==1 && live_allocs==0);
  reset();fail_alloc=true;feed("RING\r\n",6);CHECK(events[CLOSECONNECT]==1 && live_allocs==0);
  reset();char noise[2049];memset(noise,'x',sizeof(noise));feed(noise,sizeof(noise));CHECK(events[CLOSECONNECT]==1 && live_allocs==0);
  reset();const char *sms="+CMT: \"12345678901\"\r\nERROR RING CLOSED OK\r\n";feed(sms,(unsigned)strlen(sms));CHECK(events[SMS]==1 && events[ATERROR]+events[RING]+events[CLOSECONNECT]+events[ATOK]==0);
  for(unsigned cut=1;cut<n;cut++){reset();memcpy(rx,packet,cut);rxlen=(uint16_t)cut;gsm_stashurc();memcpy(rx,packet+cut,n-cut);rxlen=(uint16_t)(n-cut);gsm_stashurc();check_bounds=false;gsm_takeurc();check_bounds=true;CHECK(datalen==sizeof(body)+3 && events[ATOK]==1);}
  reset();feed("+IPD,65535:",11);char chunk[2048];memset(chunk,'x',sizeof(chunk));for(unsigned i=0;i<31;i++)feed(chunk,sizeof(chunk));feed(chunk,2047);CHECK(datalen==65535 && gsm_remaining==0);
  reset();GSM.getimei=1;feed("\r\n123456789012345\r\n",19);CHECK(events[IMEI]==1 && GSM.getimei==0 && strcmp(gsm_strbuf,"123456789012345")==0);
  reset();
#endif
}
static void header_spacing(void){
  const char *headers[]={"+QIURC:\"recv\",0,3\r\n","+QIURC:  \"recv\", 0 , 3\r\n","+IPD, 3:"};
  for(unsigned i=0;i<sizeof(headers)/sizeof(headers[0]);i++){char packet[96];unsigned hl=(unsigned)strlen(headers[i]);memcpy(packet,headers[i],hl);memcpy(packet+hl,"abc",3);reset();feed(packet,hl+3);CHECK(datalen==3 && memcmp(data,"abc",3)==0);reset();}
}
int main(void) {bounds();short_lines();binary_stream();header_spacing();puts("gsmparser: B15/B16/B17 PASS (real gsm_read)");return 0;}
