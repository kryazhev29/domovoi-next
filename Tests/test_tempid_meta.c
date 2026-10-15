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
static struct {uint32_t newAddressTempID;uint16_t readTempIDIndex,tempIDsCount;struct {unsigned readyID;} status;} device;
static uint16_t temp_usbReadIndex;
typedef cellID_s cellLog_s;
static bool log_blank(const cellLog_s *p){for(unsigned i=0;i<32;i++)if(((const uint8_t*)p)[i]!=255)return false;return true;}
static uint16_t crc16_calc(uint8_t *p,uint16_t n){uint16_t c=65535;while(n--){c^=*p++;for(unsigned i=0;i<8;i++)c=c&1?(c>>1)^0xa001:c>>1;}return c;}
static uint8_t flash[96];
static unsigned writes,fail_read,fail_write;
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);memcpy(p,flash+a-4096,n);return !fail_read;}
enum { FS_SYNC = 33, FX_STORE = 12, FX_OK = 2, FX_ERROR = 3 };
typedef struct { void *context; uint8_t step; } flash_exec_args_t;
static bool occupied;
static bool flash_exec_idle(void) { return !occupied; }
static bool flash_exec_begin(unsigned kind, const flash_exec_args_t *args)
{ CHECK(kind == FX_STORE && args->step == FS_SYNC && !occupied); occupied = true; return true; }
static void flash_exec_finish(unsigned result) { CHECK(occupied && (result == FX_OK || result == FX_ERROR)); }
static void flash_exec_take_result(void) { CHECK(occupied); occupied = false; }
static bool mem_write(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);if(fail_write)return false;memcpy(flash+a-4096,p,n);writes++;return true;}
static bool mem_erase_sector(uint32_t a){CHECK(a==4096);memset(flash,255,sizeof(flash));return true;}
/* КОД МОДУЛЯ */
int main(void){
 for(unsigned type=0;type<=CT_OTHER;type++){
  memset(flash,255,sizeof(flash));device.newAddressTempID=mem[MB_TEMPID].start;device.status.readyID=0;device.readTempIDIndex=temp_usbReadIndex=temp_next=0;writes=0;
  CHECK(user_addIdToTempBuffer("1234567890",(codeType_e)type));
  cellID_s expected={0};memcpy(expected.ID,"1234567890",10);
  switch(type){case CT_PHONE:expected.isPhone=1;break;case CT_AUTO:expected.isAuto=1;break;case CT_WIEGAND:expected.isWiegand=1;break;default:break;}
  expected.index=0x8000;uint16_t crc=crc16_calc((uint8_t*)&expected+2,22);uint32_t footer[2]={crc|((uint32_t)(uint16_t)~crc<<16),0x32444954u};memcpy((uint8_t*)&expected+24,footer,8);
  CHECK(!memcmp(flash,&expected,sizeof(expected)));
  CHECK(writes==2&&device.status.readyID&&device.newAddressTempID==4096+sizeof(expected));
  CHECK(user_addIdToTempBuffer("1234567890",(codeType_e)type));CHECK(writes==2);
 }
 memset(flash,255,sizeof(flash));device.newAddressTempID=mem[MB_TEMPID].start;device.status.readyID=0;device.readTempIDIndex=temp_usbReadIndex=temp_next=0;writes=0;
 fail_read=1;CHECK(!user_addIdToTempBuffer("1234567890",CT_RFID));
 CHECK(!writes&&!device.status.readyID&&device.newAddressTempID==mem[MB_TEMPID].start);
 fail_read=0;fail_write=1;CHECK(!user_addIdToTempBuffer("1234567890",CT_RFID));
 CHECK(!writes&&!device.status.readyID&&device.newAddressTempID==4128);
 fail_write=0;CHECK(user_addIdToTempBuffer("1234567890",CT_RFID));
 CHECK(writes==2&&user_findTempID("1234567890"));
 fail_read=1;CHECK(user_findTempID("1234567890") == 2);
 CHECK(!user_addIdToTempBuffer("0987654321",CT_RFID));
 CHECK(writes==2&&device.newAddressTempID==4096+2*sizeof(cellID_s));
 fail_read=0;
 memset(flash,255,sizeof(flash));temp_next=temp_usbReadIndex=device.readTempIDIndex=0;device.tempIDsCount=0;writes=0;
 CHECK(user_addIdToTempBuffer("1111111111",CT_RFID));
 CHECK(user_addIdToTempBuffer("2222222222",CT_RFID));
 CHECK(user_addIdToTempBuffer("3333333333",CT_RFID));
 CHECK(!user_addIdToTempBuffer("4444444444",CT_RFID));
 CHECK(writes==6);
 puts("B39 tempID: each type sets one flag; defaults zero; versioned CRC/commit; duplicate unchanged PASS");
}
