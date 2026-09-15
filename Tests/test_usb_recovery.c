#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define CONFIGURED 5
#define UNCONNECTED 0
#define ATTACHED 1
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
static unsigned cable=1,tx_pending;
#define ENDP0 0
#define ENDP1 1
#define ENDP3 3
static unsigned daddr=0x85;
#define _GetDADDR() daddr
#define _GetEPTxCount(i) (17u+(i))
volatile uint8_t USB_Tx_State=1;volatile uint32_t usb_in_armed=7;
uint8_t USB_Tx_Pending(void){return tx_pending!=0;}
uint16_t USB_Tx_Queued(void){return tx_pending?42:0;}
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
static void ticks(unsigned n){while(n--){usb_timer1ms();usb_check_data();}}
static void settle(void){ticks(25);CHECK(pullup&&usb_enabled);bDeviceState=CONFIGURED;}
static void usb_watch_test(void){uint32_t v[23];unsigned base;uint8_t frame[11]={'S','L','D','N',3,0,0,0,101,0,0};uint16_t crc=crc16_calc(frame+4,5);frame[9]=(uint8_t)crc;frame[10]=(uint8_t)(crc>>8);
 settle();base=usb_recovery_count;
 usb_bus_error_seen();usb_bus_error_seen();usb_bus_error_seen();
 /* Data left for a closed port: no host write, no recovery. */
 tx_pending=1;ticks(5000);CHECK(pullup&&usb_recovery_count==base);
 /* Host writes while the answer is never taken: recover after 2 s, not before. */
 usb_datareceived(frame,11);ticks(1999);CHECK(pullup&&usb_recovery_count==base);
 ticks(1);CHECK(!pullup&&usb_recovery_count==base+1&&usb_watch_in_recoveries==1);
 /* Registers are captured before FRES: CNTR 0 then, FRES now. */
 {uint32_t r[18];usb_diag_fill_regs(r);CHECK(r[0]==1&&r[1]==3&&r[2]==18&&r[4]==0&&r[5]==FNR_RXDP&&r[6]==1&&r[7]==7&&r[8]==0);
  CHECK(r[9]==1&&r[10]==3&&r[13]==CNTR_FRES&&r[17]==0);}
 /* Bus errors: the recovery keeps the count it saw, later errors only move the live one. */
 {uint32_t e[2];usb_bus_error_seen();usb_diag_fill_errors(e);CHECK(e[0]==4&&e[1]==3);}
 /* Address captured by the recovery, the live value read now. */
 {uint32_t a[4];daddr=0;usb_diag_fill_addr(a);CHECK(a[0]==0x85&&a[1]==0&&a[2]==0&&a[3]==0);daddr=0x85;}
 usb_diag_fill(v);CHECK(v[4]==1&&v[6]==2&&v[7]==CONFIGURED&&v[8]==1&&v[9]==42&&v[11]==2000);
 tx_pending=0;settle();base=usb_recovery_count;
 /* Slow but moving transfer: every taken IN packet restarts the 2 s window. */
 tx_pending=1;usb_datareceived(frame,11);ticks(1500);usb_in_done();ticks(1500);usb_in_done();ticks(1500);CHECK(pullup&&usb_recovery_count==base);
 {uint32_t r[18];usb_in_armed=9;usb_diag_fill_regs(r);CHECK(r[7]==7&&r[8]==0&&r[16]==9&&r[17]==2);}
 /* Queue drained: disarm, a later pending answer without a new write stays quiet. */
 tx_pending=0;ticks(1);tx_pending=1;ticks(3000);CHECK(pullup&&usb_recovery_count==base);
 tx_pending=0;ticks(1);
 /* Host writes but the device believes it is not configured. */
 bDeviceState=ATTACHED;usb_datareceived(frame,11);ticks(1999);CHECK(pullup&&usb_recovery_count==base);
 ticks(1);CHECK(!pullup&&usb_watch_unconf_recoveries==1&&usb_out_unconfigured>=1);
 usb_diag_fill(v);CHECK(v[5]==1&&v[6]==3&&v[7]==ATTACHED);
 settle();}
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
 CHECK(usb_esof_forced==3&&usb_esof_timeouts==2&&usb_recovery_count==2);
 usb_watch_test();
 puts("PASS USB watch: IN not taken, silent host, progress, unconfigured OUT, command 111 values; ESOF success/timeout, 20 ms detach, cable wait, RX ownership, re-enumeration and command 101 RX/TX");return 0;}
