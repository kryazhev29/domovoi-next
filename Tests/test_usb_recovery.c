#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define CONFIGURED 5
#define UNCONNECTED 0
#define CNTR_FRES 1
#define CNTR_FSUSP 8
#define IMR_MSK 256
#define ISTR_ESOF 256
#define ISTR_RESET 1024
#define FNR_RXDP 1
#define CLR_ESOF 0
#define CLR_RESET 0
#define __IO volatile
static unsigned mask,pullup=1,init_count,cntr,reset_poll,reset_after,delivered,tx_reset,ep_restore;
static unsigned cable=1;
unsigned bDeviceState=CONFIGURED;
volatile unsigned wIstr,esof_counter,wCNTR;
unsigned wInterrupt_Mask=ISTR_ESOF;
uint32_t __get_PRIMASK(void){return mask;}
void __disable_irq(void){mask=1;}
void __set_PRIMASK(uint32_t m){mask=m;}
#define __DMB() ((void)0)
#define USB_PULLUP_ON (pullup=1)
#define USB_PULLUP_OFF (pullup=0)
#define USB_DETECT cable
#define Set_System() ((void)0)
#define Set_USBClock() ((void)0)
#define USB_Interrupts_Config() ((void)0)
#define USB_Init() do{init_count++;cntr=0;bDeviceState=UNCONNECTED;}while(0)
#define _SetCNTR(x) (cntr=(x))
#define _GetCNTR() cntr
#define _GetFNR() FNR_RXDP
#define _GetENDPOINT(i) (i)
#define _SetENDPOINT(i,x) do{(void)(i);(void)(x);ep_restore++;}while(0)
#define _SetISTR(x) ((void)(x))
unsigned _GetISTR(void){CHECK(++reset_poll<100000);return ISTR_ESOF|((reset_after&&reset_poll>=reset_after)?ISTR_RESET:0);}
void USB_Tx_Reset(void){tx_reset++;}
uint8_t USART_To_USB_Send_Data(uint8_t*p,uint16_t n){(void)p;(void)n;return bDeviceState==CONFIGURED;}
uint16_t USB_Send_Debug(uint8_t*p,uint16_t n){(void)p;return n;}
void usb_clear_buffer(void);
void usb_data_reading(uint8_t*p,uint16_t n){CHECK(n==11&&p[8]==101);delivered++;}
uint16_t crc16_calc(uint8_t*p,uint16_t n){uint16_t c=0xffff;unsigned i;while(n--){c^=*p++;for(i=0;i<8;i++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
/* PRODUCTION */
int main(void){unsigned i;uint8_t frame[11]={'S','L','D','N',3,0,0,0,101,0,0};uint16_t crc=crc16_calc(frame+4,5);frame[9]=(uint8_t)crc;frame[10]=(uint8_t)(crc>>8);
 /* Successful robustness reset must preserve the configured session. */
 usb_enabled=1;reset_after=3;esof_counter=4;USB_Istr();CHECK(bDeviceState==CONFIGURED&&pullup&&ep_restore==8&&!tx_reset);
 /* Missing RESET must return promptly and recover without a board reset. */
 reset_after=reset_poll=0;esof_counter=4;USB_Istr();CHECK(reset_poll<40000);CHECK(!pullup&&!usb_enabled&&tx_reset==1);
 CHECK(!usb_send(frame,11));
 for(i=0;i<19;i++){usb_timer1ms();usb_check_data();CHECK(!pullup&&!init_count);}
 usb_timer1ms();cable=0;usb_check_data();CHECK(!pullup&&!init_count);
 cable=1;usb_check_data();CHECK(pullup&&usb_enabled&&init_count==1&&!(cntr&CNTR_FRES));
 bDeviceState=CONFIGURED;usb_datareceived(frame,11);usb_check_data();CHECK(delivered==1&&usb_send(frame,11));
 /* Repeat while a parser owns the RX prefix: defer clearing until release. */
 usb_rx_processing=1;esof_counter=4;reset_poll=0;USB_Istr();CHECK(usb_rx_discard);
 for(i=0;i<20;i++)usb_timer1ms();usb_check_data();CHECK(init_count==1);
 usb_rx_processing=0;usb_check_data();CHECK(init_count==2&&mask==0);
 puts("PASS ESOF success/timeout, 20 ms detach, cable wait, RX ownership, re-enumeration and command 101 RX/TX");return 0;}
