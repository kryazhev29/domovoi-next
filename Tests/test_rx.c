#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define CH_USB 2
#define USB_PULLUP_ON ((void)0)
#define USB_DETECT 1
#define Set_System() ((void)0)
#define Set_USBClock() ((void)0)
#define USB_Interrupts_Config() ((void)0)
#define USB_Init() ((void)0)
static unsigned masked,calls,delivered,inject;
static uint8_t frame[523],other[523];
uint32_t __get_PRIMASK(void){return masked;}
void __disable_irq(void){masked=1;}
void __set_PRIMASK(uint32_t m){masked=m;}
#define __DMB() ((void)0)
uint8_t USART_To_USB_Send_Data(uint8_t*p,uint16_t n){(void)p;(void)n;return 1;}
uint16_t USB_Send_Debug(uint8_t*p,uint16_t n){(void)p;return n;}
static uint16_t crc16_calc(uint8_t*p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
void usb_clear_buffer(void);
void usb_datareceived(uint8_t*,uint16_t);
void usb_data_reading(uint8_t*,uint16_t);
void user_parse_comand(uint8_t*p,unsigned channel){CHECK(channel==CH_USB);CHECK(!memcmp(p,frame+4,519));calls++;}
/* PRODUCTION */
void usb_data_reading(uint8_t*p,uint16_t n){uint8_t saved[523];CHECK(n==sizeof(frame));CHECK(!memcmp(p,frame,n));delivered++;memcpy(saved,p,n);if(inject==1){inject=0;usb_datareceived(other,sizeof(other));}else if(inject==2){inject=0;usb_clear_buffer();usb_datareceived(other,sizeof(other));CHECK(!memcmp(saved,p,n));}app_usb_data_reading(p,n);}
static void poll(void){unsigned i;for(i=0;i<20;i++){usb_timer1ms();usb_check_data();}}
static void initframe(void){unsigned i;uint16_t c;for(i=0;i<sizeof(frame);i++)frame[i]=(uint8_t)i;memcpy(frame,"SLDN",4);frame[4]=3;frame[5]=2;/* 515-byte body, 523 wire bytes */memcpy(frame+40,"SLDN",4);c=crc16_calc(frame+4,517);memcpy(frame+521,&c,2);memcpy(other,frame,sizeof(frame));}
int main(void){unsigned split;initframe();for(split=1;split<sizeof(frame);split++){usb_clear_buffer();calls=delivered=0;usb_datareceived(frame,split);poll();CHECK(calls==0&&delivered==0);usb_datareceived(frame+split,sizeof(frame)-split);poll();CHECK(calls==1&&delivered==1);}puts("all 522 split points, pauses and embedded SLDN: passed");
 usb_clear_buffer();calls=0;usb_datareceived(frame,sizeof(frame));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;other[521]^=1;usb_datareceived(other,sizeof(other));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);memcpy(other,frame,sizeof(frame));
 usb_clear_buffer();calls=0;{uint8_t junk[]={1,2,3,'S','L','D','N',255,255,0,0};usb_datareceived(junk,sizeof(junk));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);}
 usb_clear_buffer();calls=0;inject=1;usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;inject=2;usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;for(split=0;split<5;split++)usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==0);usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);
 puts("concatenation, CRC, noise, bad length, append/reset during parsing, overflow: passed");return 0;}
