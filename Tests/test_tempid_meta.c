/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)
/* ТИПЫ ПРОТОКОЛА */
#define MB_TEMPID 0
static struct {uint32_t start,size;} mem[1]={{4096,96}};
static struct {uint32_t newAddressTempID;struct {unsigned readyID;} status;} device;
static uint8_t flash[96];
static unsigned writes,fail_read,fail_write;
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);memcpy(p,flash+a-4096,n);return !fail_read;}
static bool mem_write(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);if(fail_write)return false;memcpy(flash+a-4096,p,n);writes++;return true;}
/* КОД МОДУЛЯ */
int main(void){
 for(unsigned type=0;type<=CT_OTHER;type++){
  memset(flash,255,sizeof(flash));device.newAddressTempID=0;device.status.readyID=0;writes=0;
  user_addIdToTempBuffer("1234567890",(codeType_e)type);
  cellID_s expected={0};memcpy(expected.ID,"1234567890",10);
  switch(type){case CT_PHONE:expected.isPhone=1;break;case CT_AUTO:expected.isAuto=1;break;case CT_WIEGAND:expected.isWiegand=1;break;default:break;}
  CHECK(!memcmp(flash,&expected,sizeof(expected)));
  CHECK(writes==1&&device.status.readyID&&device.newAddressTempID==4096+sizeof(expected));
  user_addIdToTempBuffer("1234567890",(codeType_e)type);CHECK(writes==1);
 }
 memset(flash,255,sizeof(flash));device.newAddressTempID=0;device.status.readyID=0;writes=0;
 fail_read=1;user_addIdToTempBuffer("1234567890",CT_RFID);
 CHECK(!writes&&!device.status.readyID&&!device.newAddressTempID);
 fail_read=0;fail_write=1;user_addIdToTempBuffer("1234567890",CT_RFID);
 CHECK(!writes&&!device.status.readyID&&!device.newAddressTempID);
 fail_write=0;user_addIdToTempBuffer("1234567890",CT_RFID);
 CHECK(writes==1&&user_findTempID("1234567890"));
 fail_read=1;CHECK(!user_findTempID("1234567890"));
 user_addIdToTempBuffer("0987654321",CT_RFID);
 CHECK(writes==1&&device.newAddressTempID==4096+sizeof(cellID_s));
 fail_read=0;
 puts("B39 tempID: each type sets one flag; defaults/reserved zero; duplicate unchanged PASS");
}
