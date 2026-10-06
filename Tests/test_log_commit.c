// Доработка: Кряжев А., 2026
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* ТИПЫ */
enum {MB_LOG};
static struct {uint32_t start,size;} mem[1]={{4096,4096}};
static struct {uint32_t newAddressLog;struct {uint8_t readyLogData;} status;} device;
static uint8_t flash[4096];
static unsigned operation,fail_operation,partial,fail_read;
// Проверяет сценарий mem_read по независимым данным модуля.
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=8192);if(fail_read)return false;memcpy(p,flash+a-4096,n);return true;}
// Проверяет сценарий mem_write по независимым данным модуля.
static bool mem_write(uint32_t a,uint8_t *p,uint16_t n){unsigned i,limit=n;CHECK(a>=4096&&a+n<=8192);operation++;
 if(operation==fail_operation&&partial<limit)limit=partial;
 for(i=0;i<limit;i++)flash[a-4096+i]&=p[i];
 return operation!=fail_operation;}
// Проверяет сценарий wdt_reload по независимым данным модуля.
static void wdt_reload(void){}
/* ИСХОДНИК */
static void reset(void){memset(flash,255,sizeof(flash));device.newAddressLog=4096;device.status.readyLogData=0;operation=fail_operation=0;log_lastIndex=0;}
// Запускает проверки модуля и сохраняет результаты в заданный файл.
int main(void){cellLog_s event,legacy;unsigned op,n;(void)wdt_reload;memset(&event,0,sizeof(event));event.index=event.datetime=1000;event.logType=33;
 reset();fail_operation=1;partial=4;CHECK(!log_addEvent(&event));CHECK(!device.status.readyLogData&&device.newAddressLog==4096);
 CHECK(log_getRecordsCount()==0);
 for(op=1;op<=2;op++)for(n=0;n<=(op==1?28u:4u);n++){reset();event.index=1000;fail_operation=op;partial=n;
  CHECK(!log_addEvent(&event));log_lastIndex=0;device.newAddressLog=4096;
  CHECK(log_getRecordsCount()==(op==2&&n==4?1:0));
  fail_operation=0;if(!(op==2&&n==4)){CHECK(log_addEvent(&event));CHECK(log_getRecordsCount()==1);}
 }
 reset();CHECK(log_addEvent(&event));flash[5]^=1;CHECK(log_getRecordsCount()==0);
 reset();legacy=event;memset(legacy.reserve,0,8);memcpy(flash,&legacy,sizeof(legacy));CHECK(log_getRecordsCount()==1);
 CHECK(log_addEvent(&event)&&event.index>legacy.index);CHECK(log_getRecordsCount()==2);
 {uint8_t wire[64];CHECK(log_readRecords(0,wire,2));CHECK(!memcmp(wire,&legacy,sizeof(legacy)));memcpy(&legacy,wire+32,32);
  CHECK(legacy.index==event.index);for(n=0;n<8;n++)CHECK(!legacy.reserve[n]);}
 fail_read=1;CHECK(log_getRecordsCount()==0xFFFFu);
 puts("logcommit: payload/commit byte cuts, CRC, legacy records and unchanged wire layout passed");return 0;}
