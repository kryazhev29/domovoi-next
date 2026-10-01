#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define BOOTLOADER
#define __IO volatile
#define CNTR_RESETM 0x400
#define CNTR_FRES 1
#define CNTR_FSUSP 8
#define CNTR_LPMODE 4
#define ISTR_RESET 0x400
#define ISTR_WKUP 0x1000
#define CLR_RESET 0
#define CLR_WKUP 0
#define PWR_Regulator_LowPower 1
#define SCB_SCR_SLEEPDEEP 4
static struct {uint32_t CR;} power,*PWR=&power;
static struct {uint32_t SCR;} scb,*SCB=&scb;
static unsigned cntr,polls,reset_after,recoveries,restores;
static uint16_t EP[8];
static uint16_t _GetISTR(void){CHECK(++polls<50000);return reset_after&&polls>=reset_after?ISTR_RESET:0;}
#define _GetCNTR() cntr
#define _SetCNTR(v) (cntr=(v))
#define _GetENDPOINT(i) ((i)+20u)
#define _SetENDPOINT(i,v) ((void)(i),(void)(v),restores++)
#define _SetISTR(v) ((void)(v))
#define __WFI() ((void)0)
void boot_usb_request_recovery(void){recoveries++;}
/* PRODUCTION */
int main(void){
 Suspend();CHECK(recoveries==1&&polls<=32001&&!restores);
 recoveries=polls=0;reset_after=5;Suspend();CHECK(!recoveries&&restores==8);
 puts("B36 Suspend: RESET timeout schedules recovery; successful reset restores endpoints PASS");
}
