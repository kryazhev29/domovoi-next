#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define CH_USB 2
#define USB_PULLUP_ON ((void)0)
#define USB_PULLUP_OFF ((void)0)
#define CNTR_FRES 1
#define UNCONNECTED 0
#define CONFIGURED 5
static unsigned bDeviceState=CONFIGURED;
uint8_t USB_Tx_Pending(void){return 0;}
uint16_t USB_Tx_Queued(void){return 0;}
#define ENDP1 1
#define ENDP3 3
#define _GetENDPOINT(i) (i)
#define _GetEPTxCount(i) (i)
#define _GetISTR() 0
#define _GetCNTR() 0
#define _GetFNR() 0
volatile uint8_t USB_Tx_State;volatile uint32_t usb_in_armed;
#define _SetCNTR(x) ((void)(x))
#define USB_Tx_Reset() ((void)0)
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
#define EXPECT(x,msg) do{if(!(x)){fprintf(stderr,"FAIL %s\n",msg);exit(1);}}while(0)
static void tick(unsigned n){while(n--)usb_timer1ms();}
static void length_test(void){uint8_t bad[523];usb_clear_buffer();calls=delivered=0;memcpy(bad,frame,sizeof(bad));bad[4]+=17;usb_datareceived(bad,sizeof(bad));usb_datareceived(frame,sizeof(frame));usb_datareceived(frame,sizeof(frame));poll();EXPECT(calls==2,"corrupt length swallowed a following valid frame");puts("PASS corrupt length followed by two intact frames");}
static void silence_test(void){unsigned off,n;usb_clear_buffer();calls=0;usb_datareceived(frame,20);tick(200);/* Fresh bytes can arrive before foreground services the timeout. */usb_datareceived(frame,sizeof(frame));poll();EXPECT(calls==1,"incomplete frame survived 200 ms silence");
 usb_clear_buffer();calls=0;usb_datareceived(frame,20);tick(200);usb_check_data();usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);
 usb_clear_buffer();calls=0;for(off=0;off<sizeof(frame);off+=n){n=sizeof(frame)-off;if(n>100)n=100;usb_datareceived(frame+off,n);tick(199);usb_check_data();}EXPECT(calls==1,"bytes within 200 ms were discarded");
 /* Complete frames do not expire while waiting for main. */usb_clear_buffer();calls=0;usb_datareceived(frame,sizeof(frame));tick(250);poll();CHECK(calls==1);
 /* A complete frame followed by an expired prefix must preserve fresh data. */usb_clear_buffer();calls=0;usb_datareceived(frame,sizeof(frame));usb_datareceived(frame,20);tick(200);usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 {uint8_t large[1800];memset(large,0,sizeof(large));memcpy(large,"SLDN",4);large[4]=0xc8;large[5]=7;usb_clear_buffer();calls=0;usb_datareceived(large,sizeof(large));tick(200);usb_datareceived(frame,sizeof(frame));poll();EXPECT(calls==1,"expired large prefix caused fresh-frame overflow");}
 masked=1;usb_timer1ms();CHECK(masked==1);usb_clear_buffer();usb_datareceived(frame,20);CHECK(masked==1);masked=0;usb_clear_buffer();puts("PASS 200 ms expiry, 199 ms refresh, queued complete frames and PRIMASK restoration");}
int main(int argc,char**argv){unsigned split;initframe();if(argc>1){if(!strcmp(argv[1],"length")){length_test();return 0;}if(!strcmp(argv[1],"silence")){silence_test();return 0;}}for(split=1;split<sizeof(frame);split++){usb_clear_buffer();calls=delivered=0;usb_datareceived(frame,split);poll();CHECK(calls==0&&delivered==0);usb_datareceived(frame+split,sizeof(frame)-split);poll();CHECK(calls==1&&delivered==1);}puts("all 522 split points, pauses and embedded SLDN: passed");
 usb_clear_buffer();calls=0;usb_datareceived(frame,sizeof(frame));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;other[521]^=1;usb_datareceived(other,sizeof(other));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);memcpy(other,frame,sizeof(frame));
 usb_clear_buffer();calls=0;{uint8_t junk[]={1,2,3,'S','L','D','N',255,255,0,0};usb_datareceived(junk,sizeof(junk));usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);}
 usb_clear_buffer();calls=0;inject=1;usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;inject=2;usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==2);
 usb_clear_buffer();calls=0;for(split=0;split<5;split++)usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==0);usb_datareceived(frame,sizeof(frame));poll();CHECK(calls==1);
 puts("concatenation, CRC, noise, bad length, append/reset during parsing, overflow: passed");length_test();silence_test();return 0;}
