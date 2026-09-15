#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t boot_ver=0x32;
#define BOOT_VER boot_ver
#define FWBLOCK_OFFSET (uint32_t)0x200
#define CPU_ENDMEMORYADDRESS (uint32_t)0x0801ffff
#define APLICATION_ADDRESS (uint32_t)(0x08008000 + FWBLOCK_OFFSET)
enum {MB_FIRMWARE=0};
static struct {uint32_t start,size;} mem[1]={{0,131072}};
static uint8_t flash[131072];static unsigned saves,wdt;
static void mem_read(uint32_t a,uint8_t*p,uint16_t n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);}
static void program(uint32_t a,const uint8_t*p,uint32_t n){uint32_t i;CHECK(a+n<=sizeof(flash));for(i=0;i<n;i++)flash[a+i]&=p[i];}
static void user_saveindex(void){saves++;}
static void wdt_reload(void){wdt++;}
#pragma pack(push,1)
typedef struct{uint32_t appIdent;uint8_t hardwareVersion;uint16_t firmwareVersion;uint8_t res;uint32_t firmwareSize;uint32_t firmwareCRC;uint32_t res2;uint32_t buildDate;uint8_t res3[8];}bootFWparam_s;
#pragma pack(pop)
typedef enum {CH_SERVER=1,CH_USB=2} dataChanel_e;
typedef enum {CMD_SUCCESS=0,CMD_RUNNING=5,CMD_ROLLBACKRUN=253} cmdCode_e;
static struct {uint8_t FWinProgress,rebootToUpdate,firmwarePercent,cchFWupdate;uint16_t updateTimeout,FWblockIdx,FWblockCount;uint32_t startWaitFWtime;struct {uint16_t readyToUpdate:1;} status;} device;
static void fw_resume_after_boot(void);
/* PRODUCTION */
static uint8_t image[127*1024];
static void stage(uint16_t blocks,uint16_t version){bootFWparam_s h;memset(flash,0xff,sizeof(flash));memset(&h,0xff,sizeof(h));h.hardwareVersion=139;h.firmwareVersion=version;h.firmwareSize=(uint32_t)blocks*1024u-8u;h.firmwareCRC=0x12345678u;h.buildDate=1789454349u;program(0,(uint8_t*)&h,sizeof(h));
 memset(&device,0,sizeof(device));device.FWinProgress=1;device.FWblockCount=blocks;device.cchFWupdate=0;saves=0;}
static void resume_tests(void){uint32_t seed=12345u,i;uint16_t blocks=37,k,idx;unsigned cuts=0;
 static const uint16_t offs[]={0,1,63,64,255,256,700,1023,1024};
 for(i=0;i<sizeof(image);i++){seed=seed*1103515245u+12345u;image[i]=(uint8_t)(seed>>16);}
 memset(image+5*1024,0xff,1024);memset(image+6*1024,0xff,300);memset(image+36*1024+1016,0xff,8);/* legitimate FF block, FF prefix, FF tail */
 for(k=0;k<blocks;k++){unsigned o;for(o=0;o<sizeof(offs)/sizeof(offs[0]);o++){uint32_t base=0x200u+(uint32_t)k*1024u;
  stage(blocks,64);
  program(0x200,image,(uint32_t)k*1024u);/* blocks before k complete */
  program(base,image+base-0x200,offs[o]);/* block k torn after offs[o] bytes */
  if(offs[o]<1024){uint8_t half=(uint8_t)(image[base-0x200+offs[o]]|0x5a);program(base+offs[o],&half,1);}/* one byte with bits still set */
  fw_resume_after_boot();idx=device.FWblockIdx;
  CHECK(device.FWinProgress&&device.status.readyToUpdate&&device.updateTimeout==3600&&device.cchFWupdate==CH_SERVER&&!saves);
  CHECK(idx<=k);/* never past a torn or missing block */
  program(0x200u+(uint32_t)idx*1024u,image+(uint32_t)idx*1024u,((uint32_t)blocks-idx)*1024u);/* server sends idx..end */
  CHECK(!memcmp(flash+0x200,image,(uint32_t)blocks*1024u));cuts++;}}
 /* Nothing written: start from 0. Everything written: request the last block again. */
 stage(blocks,64);fw_resume_after_boot();CHECK(device.FWblockIdx==0&&device.firmwarePercent==0);
 stage(blocks,64);program(0x200,image,(uint32_t)blocks*1024u);fw_resume_after_boot();CHECK(device.FWblockIdx==blocks-1);
 /* No download in progress: nothing read back, nothing saved. */
 stage(blocks,64);device.FWinProgress=0;fw_resume_after_boot();CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!saves);
 /* A header that is not a download ends the update and saves that. */
 stage(blocks,1);fw_resume_after_boot();CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&saves==1);
 stage(blocks,64);{uint32_t magic=0x32544F42u;program(0,(uint8_t*)&magic,4);}fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(blocks,64);device.FWblockCount=1;fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(0xffff,64);memset(flash,0xff,sizeof(flash));fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 boot_ver=0x21;stage(blocks,64);fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);boot_ver=0x32;
 printf("resume: %u power cuts over %u blocks restore the image byte for byte\n",cuts,blocks);}
int main(void){
 /* Rollback: number 1 with every other field zero, as the server sends it. */
 CHECK(!fw_start_rejected(1,0,0,0,0));
 CHECK(!fw_start_rejected(1,0xffffffff,3,0,0xffff));
 /* Reserved number and the old bootloader stay rejected, rollback included. */
 CHECK(fw_start_rejected(0,1,4,1,1));
 boot_ver=0x21;CHECK(fw_start_rejected(1,0,0,0,0));CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,64));boot_ver=0x32;
 /* Update: a real image passes, each broken field is rejected. */
 CHECK(!fw_start_rejected(64,1789454349,65016,0x57d1c3a7,64));
 CHECK(!fw_start_rejected(2,1,4,1,1));
 CHECK(fw_start_rejected(64,0,65016,0x57d1c3a7,64));
 CHECK(fw_start_rejected(64,1789454349,0,0x57d1c3a7,64));
 CHECK(fw_start_rejected(64,1789454349,65015,0x57d1c3a7,64));
 CHECK(fw_start_rejected(64,1789454349,65016,0,64));
 CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,0));
 CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,63));
 CHECK(!fw_start_rejected(64,1,0x17e00,1,0x60));
 CHECK(fw_start_rejected(64,1,0x17e04,1,0x60));
 CHECK(fw_start_rejected(64,1,4,1,128));
 /* Accepted update: the server gets 5, NextConfig over USB keeps 0. */
 CHECK(fw_start_accepted(CH_SERVER)==CMD_RUNNING);
 CHECK(fw_start_accepted(CH_USB)==CMD_SUCCESS);
 /* Last block, good image: reboot to install, bit 11 cleared. */
 device.cchFWupdate=CH_SERVER;saves=0;device.FWinProgress=1;device.status.readyToUpdate=1;device.updateTimeout=1200;device.firmwarePercent=99;
 fw_blocks_done(1);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&device.rebootToUpdate&&device.firmwarePercent==248);
 /* Last block, bad image CRC: the update ends, no reboot, 252. */
 device.FWinProgress=1;device.status.readyToUpdate=1;device.updateTimeout=1200;device.rebootToUpdate=0;device.firmwarePercent=99;
 fw_blocks_done(0);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!device.rebootToUpdate&&device.firmwarePercent==252);
 /* Server updates save their end; NextConfig over USB is never resumed. */
 CHECK(saves==2);device.cchFWupdate=CH_USB;fw_blocks_done(0);CHECK(saves==2);
 resume_tests();
 puts("PASS command 19: rollback with zero fields, reserved 0, old bootloader, update field checks, answer codes; command 20: bad image CRC ends the update; resume after reboot");return 0;}
