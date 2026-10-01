#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
typedef unsigned dataChanel_e;
#define CH_USB 0
#define CH_SERVER 1
#define CH_SMS 2
#define CE_OK 0
#define MB_LOG 0
#define _1B(x) ((uint8_t)(x))
#define _2B(x) ((uint8_t)((x)>>8))
typedef struct {uint8_t data[32];} cellLog_s;
static struct {uint32_t start,size;} mem[1]={{4096,98304}};
static struct {uint16_t readLogIDIndex,logsCount;uint32_t newAddressLog;int8_t connectionhandle;struct {int readyLogData;} status;} device;
static unsigned log_usbReadIndex,records,sent,clears;
static uint16_t log_unacked[2];
static uint8_t bf[128];
static void *allocation;static size_t allocation_size;
static void *test_malloc(size_t n){allocation=malloc(n);allocation_size=n;return allocation;}
static void *test_memcpy(void *d,const void *s,size_t n){uintptr_t a=(uintptr_t)allocation,x=(uintptr_t)s;if(x>=a&&x<a+allocation_size)CHECK(n<=a+allocation_size-x);return memcpy(d,s,n);}
#define malloc test_malloc
#define memcpy test_memcpy
static bool upload_ack(unsigned cmd,dataChanel_e ch){(void)cmd;(void)ch;return true;}
static uint16_t upload_index(unsigned cmd,dataChanel_e ch){(void)cmd;return ch==CH_USB?log_usbReadIndex:device.readLogIDIndex;}
static uint16_t log_getRecordsCount(void){return (uint16_t)records;}
static void log_clearLogBuffer(void){clears++;}
static bool upload_finish(unsigned cmd){(void)cmd;log_clearLogBuffer();return true;}
static void user_send_anyData(unsigned cmd,uint8_t *p,uint16_t n,dataChanel_e ch,unsigned id){CHECK(cmd==10&&n==3&&p[0]==0&&(ch==CH_USB||ch==CH_SERVER)&&id==42);sent++;}
static void mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a==mem[MB_LOG].start&&n==records*32);memset(p,0x5A,n);}
static bool log_readRecords(uint16_t first,uint8_t *p,uint16_t count){CHECK(first==0&&count==records);mem_read(mem[MB_LOG].start,p,count*32);return true;}
static uint16_t crc16_calc(const uint8_t *p,uint16_t n){(void)p;CHECK(n==records*32+8);return 0x1234;}
static uint8_t usb_send(uint8_t *p,uint16_t n){CHECK(n==records*32+14&&p[8]==10&&p[11]==records);sent++;return 1;}
static int8_t connection_send(int8_t h,uint8_t *p,uint16_t n){CHECK(h==2);usb_send(p,n);return CE_OK;}
/* PRODUCTION */
int main(void){unsigned count,ch;(void)bf;(void)test_memcpy;for(count=1;count<=32;count++){for(ch=0;ch<=1;ch++){records=count;log_usbReadIndex=device.readLogIDIndex=0;memset(log_unacked,0,sizeof(log_unacked));device.newAddressLog=4096+count*32;device.connectionhandle=2;user_send_logRecords(ch,42);CHECK((ch==CH_USB?log_usbReadIndex:device.readLogIDIndex)==0&&log_unacked[ch]==count);}}CHECK(sent==64&&!clears);puts("logupload: 1..32 records stay inside heap allocation passed");return 0;}
