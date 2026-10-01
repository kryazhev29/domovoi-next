#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)
/* TYPES */
#define MB_TEMPID 0
static struct {uint32_t start,size;} mem[1]={{4096,96}};
static struct {uint32_t newAddressTempID;struct {unsigned readyID;} status;} device;
static uint8_t flash[96];
static unsigned writes;
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);memcpy(p,flash+a-4096,n);return true;}
static bool mem_write(uint32_t a,uint8_t *p,uint16_t n){CHECK(a>=4096&&a+n<=4192);memcpy(flash+a-4096,p,n);writes++;return true;}
/* PRODUCTION */
int main(void){
 for(unsigned type=0;type<=CT_OTHER;type++){
  memset(flash,255,sizeof(flash));device.newAddressTempID=0;device.status.readyID=0;writes=0;
  user_addIdToTempBuffer("1234567890",(codeType_e)type);
  cellID_s expected={0};memcpy(expected.ID,"1234567890",10);
  switch(type){case CT_PHONE:expected.isPhone=1;break;case CT_RFID:expected.isRFID=1;break;case CT_AUTO:expected.isAuto=1;break;case CT_RF433:expected.isRF433=1;break;case CT_WIEGAND:expected.isWiegand=1;break;default:break;}
  CHECK(!memcmp(flash,&expected,sizeof(expected)));
  CHECK(writes==1&&device.status.readyID&&device.newAddressTempID==4096+sizeof(expected));
  user_addIdToTempBuffer("1234567890",(codeType_e)type);CHECK(writes==1);
 }
 puts("B39 tempID: each type sets one flag; defaults/reserved zero; duplicate unchanged PASS");
}
