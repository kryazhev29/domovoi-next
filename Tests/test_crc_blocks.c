#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
typedef unsigned dataChanel_e;
#define CH_USB 0
#define CH_SERVER 1
#define CH_SMS 2
#define MB_ID 0
#define _1B(x) ((uint8_t)(x))
#define _2B(x) ((uint8_t)((x)>>8))
typedef struct {uint8_t data[32];} cellID_s;
static struct {uint32_t start,size;} mem[1]={{266240,1601536}};
static struct {int8_t connectionhandle;} device={2};
static unsigned crc_calls,sent;
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n){CHECK(n==16000&&a==mem[MB_ID].start+16000u*crc_calls);return (uint16_t)++crc_calls;}
static uint16_t crc16_calc(const uint8_t *p,uint16_t n){(void)p;CHECK(n==205);return 0x1234;}
static void usb_send(const uint8_t *p,uint16_t n){unsigned i;CHECK(n==211&&p[8]==6);for(i=0;i<100;i++)CHECK(p[9+2*i]==i+1&&!p[10+2*i]);CHECK(p[209]==0x34&&p[210]==0x12);sent++;}
static void connection_send(int8_t h,const uint8_t *p,uint16_t n){CHECK(h==2);usb_send(p,n);}
/* PRODUCTION */
int main(void){user_send_crcMemoryID(0,CH_USB,42);CHECK(crc_calls==100&&sent==1);crc_calls=0;user_send_crcMemoryID(0,CH_SERVER,42);CHECK(crc_calls==100&&sent==2);puts("crcblocks: 100 successive ID blocks use full 32-bit flash addresses passed");return 0;}
