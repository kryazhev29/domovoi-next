#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t boot_ver=0x32;
#define BOOT_VER boot_ver
#define FWBLOCK_OFFSET (uint32_t)0x200
#define CPU_ENDMEMORYADDRESS (uint32_t)0x0801ffff
#define APLICATION_ADDRESS (uint32_t)(0x08008000 + FWBLOCK_OFFSET)
enum {MB_FIRMWARE=0};
static struct {uint32_t start,size;} mem[1]={{0,131072}};
typedef enum {CH_SERVER=1,CH_USB=2} dataChanel_e;
typedef enum {CMD_SUCCESS=0,CMD_RUNNING=5,CMD_ROLLBACKRUN=253} cmdCode_e;
static struct {uint8_t FWinProgress,rebootToUpdate,firmwarePercent;uint16_t updateTimeout;struct {uint16_t readyToUpdate:1;} status;} device;
/* PRODUCTION */
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
 device.FWinProgress=1;device.status.readyToUpdate=1;device.updateTimeout=1200;device.firmwarePercent=99;
 fw_blocks_done(1);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&device.rebootToUpdate&&device.firmwarePercent==248);
 /* Last block, bad image CRC: the update ends, no reboot, 252. */
 device.FWinProgress=1;device.status.readyToUpdate=1;device.updateTimeout=1200;device.rebootToUpdate=0;device.firmwarePercent=99;
 fw_blocks_done(0);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!device.rebootToUpdate&&device.firmwarePercent==252);
 puts("PASS command 19: rollback with zero fields, reserved 0, old bootloader, update field checks, answer codes; command 20: bad image CRC ends the update");return 0;}
