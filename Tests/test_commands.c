#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if (!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
typedef unsigned dataChanel_e;
#define CMD_SUCCESS 0
#define CMD_PARAMERROR 1
#define CMD_RUNERROR 2
#define SHTAMP_CONFIG 3
#define MB_CONFIG 0
#pragma pack(push,1)
static struct {uint32_t datetime;char prefix[133];char countryCode[8];char tail[367];} config;
#pragma pack(pop)
static struct {uint32_t datetimeConfig;struct {int defaultConfig;} status;} device;
static struct {uint32_t start,size;} mem[1]={{4096,512}};
static unsigned answer,sms,saved,stamp,fail_alloc;
static char sent_number[32],sent_text[2048];
static void *test_malloc(size_t n){return fail_alloc?NULL:malloc(n);}
#define malloc test_malloc
static void user_send_answer_comand(unsigned cmd,unsigned code,unsigned ch,unsigned id){CHECK((cmd==11||cmd==12)&&ch==1&&id==42);answer=code;}
static void user_sendsms(char *text,char *num){sms++;strcpy(sent_number,num);strcpy(sent_text,text);}
static void user_writeConfigAndDevice(void){CHECK(device.datetimeConfig==config.datetime&&!device.status.defaultConfig);saved++;}
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n){CHECK(a==4096&&n==512);CHECK(saved==1);return 0x1234;}
static void user_send_timeshtamp(unsigned kind,uint32_t time,uint16_t crc,unsigned ch,unsigned id){CHECK(saved==1&&kind==SHTAMP_CONFIG&&time==config.datetime&&crc==0x1234&&ch==1&&id==42);stamp++;}
/* PRODUCTION */
int main(void){uint8_t p[2052];unsigned n;
 strcpy(config.countryCode,"+7");
 for(n=3;n<14;n++){uint8_t *short_packet=malloc(n+4);CHECK(short_packet);memset(short_packet,0x41,n+4);short_packet[4]=11;sms=0;command(short_packet,n,1,42);CHECK(!sms&&answer==CMD_PARAMERROR);free(short_packet);}
 memset(p,0x55,sizeof(p));p[4]=11;memcpy(p+5,"9001234567",10);memcpy(p+15,"hello",5);sms=0;command(p,18,1,42);CHECK(sms==1&&answer==CMD_SUCCESS&&!strcmp(sent_text,"hello")&&!strcmp(sent_number,"+79001234567"));CHECK(p[20]==0x55);
 p[16]=0;sms=0;command(p,18,1,42);CHECK(sms==1&&!strcmp(sent_text,"h"));
 p[15]=0;sms=0;command(p,18,1,42);CHECK(!sms&&answer==CMD_PARAMERROR);p[15]='h';
 memset(config.countryCode,'7',sizeof(config.countryCode));command(p,18,1,42);CHECK(!sms&&answer==CMD_PARAMERROR);strcpy(config.countryCode,"+7");
 fail_alloc=1;command(p,18,1,42);CHECK(!sms&&answer==CMD_RUNERROR);fail_alloc=0;
 memset(p,0x41,sizeof(p));p[4]=11;command(p,2040,1,42);CHECK(strlen(sent_text)==2027);
 memset(p,0,sizeof(p));p[4]=12;p[5]=7;p[13]=1;p[142]='7';saved=stamp=0;device.status.defaultConfig=1;command(p,sizeof(config)+3,1,42);CHECK(saved==1&&stamp==1);
 puts("commands: truncated SMS, bounded unterminated/terminated text, empty text, invalid country code, allocation failure, maximum text, config save-before-response passed");return 0;}
