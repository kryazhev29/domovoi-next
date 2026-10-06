/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* ТИПЫ ПРОТОКОЛА */
enum {MB_LOG,MB_TEMPID};
static struct {uint32_t start,size;} mem[2]={{4096,4096},{8192,4096}};
static struct {uint32_t newAddressLog,newAddressTempID;} device;
static uint8_t flash[12288];
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);return true;}
static void wdt_reload(void){}
/* КОД МОДУЛЯ */
int main(void){unsigned n,i;for(n=0;n<=128;n++){
 memset(flash,255,sizeof(flash));device.newAddressLog=4128;device.newAddressTempID=8224;
 for(i=0;i<n;i++){cellLog_s log;cellID_s id;memset(&log,0,sizeof(log));log.index=i+1;memcpy(flash+4096+i*sizeof(log),&log,sizeof(log));
 memset(&id,0,sizeof(id));id.index=i+1;memcpy(flash+8192+i*sizeof(id),&id,sizeof(id));}
 CHECK(log_getRecordsCount()==n);CHECK(device.newAddressLog==4096+n*sizeof(cellLog_s));
 CHECK(user_getTempIDRecordsCount()==n);CHECK(device.newAddressTempID==8192+n*sizeof(cellID_s));
 }puts("countends: empty/partial/full log and temp ID cursors passed");return 0;}
