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
static void gsm_dataread(gsmresp_e op,char *p,char *q,uint32_t n,int32_t id) {
  (void)q;lastid=id;events[op]++;
  if (p!=NULL && p!=gsm_strbuf) CHECK(p>=begin && p<=end);
  if (op==DATA) { CHECK(p>=begin && p<=end); CHECK(n<=(uint32_t)(end-p)); CHECK(datalen+n<=sizeof(data));memcpy(data+datalen,p,n);datalen+=n; }
}
static void gsm_readsms(uint8_t n) {(void)n;}
static void gsm_disconnect(uint8_t n) {(void)n;}
static char *gsm_runcmd(char *s,char *r) {(void)s;(void)r;return gsm_strbuf;}
/* PRODUCTION */
static void feed(const char *s,unsigned n) {
  char *copy=calloc(n+48,1);CHECK(copy);copy[5]='7';copy[6]=',';copy[7]=' ';begin=copy+8;memcpy(begin,s,n);end=begin+n;gsm_read(begin,(uint16_t)n);free(copy);
}
static void reset(void) {memset(&GSM,0,sizeof(GSM));GSM.ready=1;GSM.moduleType=M_UG95;memset(events,0,sizeof(events));datalen=0;}
static void bounds(void) {
  const char *heads[]={"+IPD,13:","+QIURC: \"recv\",0,13\r\n"};
  for(unsigned h=0;h<2;h++) {
    char packet[128]={0};unsigned hl=(unsigned)strlen(heads[h]);memcpy(packet,heads[h],hl);memset(packet+hl,'x',13);
    for(unsigned n=1;n<hl+13;n++) {reset();feed(packet,n);CHECK(datalen==0);}
    reset();feed(packet,hl+13);CHECK(datalen==13);
  }
  reset();feed("+QIURC: \"recv\",0,65535\r\nx",25);CHECK(datalen==0);
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
int main(void) {bounds();short_lines();puts("gsmparser: B15/B16 PASS (real gsm_read)");return 0;}
