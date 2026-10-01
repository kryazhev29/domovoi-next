#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
#define MB_HOLIDAY 0
static struct {uint32_t start;} mem[1]={{4096}};
static struct {uint32_t datetime;uint8_t hour,minute,day;} device;
static uint32_t holiday;
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a==4096&&n==1024);for(unsigned i=0;i<n;i+=4)memcpy(p+i,&holiday,4);return true;}
/* PRODUCTION */
int main(void){
 holiday=2147483647u;device.datetime=2147483648u;CHECK(user_dayIsHoliday());
 const uint32_t times[]={1600000000u,2147472000u,2147483647u,2147483648u,2147558400u,UINT32_MAX-60u,UINT32_MAX};
 for(unsigned h=0;h<7;h++)for(unsigned t=0;t<7;t++){
  holiday=times[h];device.datetime=times[t];
  CHECK(user_dayIsHoliday()==(holiday/86400u==device.datetime/86400u));
  audit_clock();CHECK(device.day==(device.datetime/86400u+3u)%7u);
  CHECK(device.hour==device.datetime%86400u/3600u&&device.minute==device.datetime%3600u/60u);
 }
 puts("B42: UTC day, holiday, weekday/hour/minute across 2038 and UINT32_MAX PASS");
}
