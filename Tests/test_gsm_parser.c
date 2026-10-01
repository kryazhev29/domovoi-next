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
static void gsm_dataread(gsmresp_e op,char *p,char *q,uint32_t n,int32_t id) {
  (void)q;(void)id;events[op]++;
  if (op==DATA) { CHECK(p>=begin && p<=end); CHECK(n<=(uint32_t)(end-p)); CHECK(datalen+n<=sizeof(data));memcpy(data+datalen,p,n);datalen+=n; }
}
static void gsm_readsms(uint8_t n) {(void)n;}
static void gsm_disconnect(uint8_t n) {(void)n;}
static char *gsm_runcmd(char *s,char *r) {(void)s;(void)r;return gsm_strbuf;}
/* PRODUCTION */
static void feed(const char *s,unsigned n) {
  char *copy=calloc(n+32,1);CHECK(copy);memcpy(copy,s,n);begin=copy;end=copy+n;gsm_read(copy,(uint16_t)n);free(copy);
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
int main(void) {bounds();puts("gsmparser: B15 bounds PASS (real gsm_read)");return 0;}
