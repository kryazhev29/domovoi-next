// Доработка: Кряжев А., 2026
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define __IO volatile
#define CONFIGURED 5
#define ATTACHED 1
#define UNCONNECTED 0
volatile uint32_t bDeviceState=CONFIGURED;
static unsigned mask,irq_restore,pump,wirelen,reset_poll;
static uint8_t wire[8192],pma[64];
static unsigned pmacount;
uint32_t __get_PRIMASK(void){return mask;}
void __disable_irq(void){mask=1;}
void __set_PRIMASK(uint32_t);
#define __DMB() ((void)0)
#define VIRTUAL_COM_PORT_DATA_SIZE 64
#define ENDP1_TXADDR 0
#define ENDP1 1
void UserToPMABufferCopy(uint8_t*p,unsigned a,unsigned n){(void)a;CHECK(n<=64);memcpy(pma,p,n);}
void SetEPTxCount(unsigned ep,unsigned n){(void)ep;pmacount=n;}
void SetEPTxValid(unsigned ep){(void)ep;CHECK(wirelen+pmacount<=sizeof(wire));memcpy(wire+wirelen,pma,pmacount);wirelen+=pmacount;}
struct {int Current_Configuration,Current_Feature,Current_Interface;} info,*pInformation=&info;
struct {int MaxPacketSize;} Device_Property;
uint8_t Virtual_Com_Port_ConfigDescriptor[8];
#define SetBTABLE(...) ((void)0)
#define SetEPType(...) ((void)0)
#define SetEPTxStatus(...) ((void)0)
#define SetEPRxAddr(...) ((void)0)
#define SetEPTxAddr(...) ((void)0)
#define Clear_Status_Out(...) ((void)0)
#define SetEPRxCount(...) ((void)0)
#define SetEPRxValid(...) ((void)0)
#define SetEPRxStatus(...) ((void)0)
#define SetDeviceAddress(...) ((void)0)
#define IMR_MSK 256
#define ISTR_ESOF 256
#define ISTR_RESET 1024
#define FNR_RXDP 1
#define CNTR_FSUSP 8
#define CNTR_FRES 1
#define CLR_ESOF 0
#define CLR_RESET 0
volatile unsigned wIstr,esof_counter,wCNTR;
unsigned wInterrupt_Mask=256;
unsigned _GetISTR(void){reset_poll++;CHECK(reset_poll<1000000);return ISTR_ESOF;}
#define _SetISTR(x) ((void)(x))
#define _GetFNR() 1
#define _GetCNTR() 0
#define _GetENDPOINT(i) (i)
#define _SetCNTR(x) ((void)(x))
#define _SetENDPOINT(i,x) ((void)(i),(void)(x))
void usb_clear_buffer(void){}
void USB_Tx_Reset(void);
void usb_request_recovery(void){USB_Tx_Reset();bDeviceState=UNCONNECTED;}
volatile uint32_t usb_esof_forced,usb_esof_timeouts,usb_recovery_reason,usb_bus_resets;
void usb_in_done(void){}

/* PRODUCTION */
void __set_PRIMASK(uint32_t value){mask=value;irq_restore++;CHECK(irq_restore<1000000);if(!value&&pump==1){if(USB_Tx_State)EP1_IN_Callback();else Handle_USBAsynchXfer();}if(!value&&pump==2){pump=0;Virtual_Com_Port_Reset();}}
int main(int argc,char**argv){uint8_t data[523];unsigned i;CHECK(argc>1);for(i=0;i<sizeof(data);i++)data[i]=(uint8_t)i;
 if(!strcmp(argv[1],"reset")){USB_Tx_State=1;USART_Rx_ptr_in=33;USART_Rx_ptr_out=22;USART_Rx_length=11;Virtual_Com_Port_Reset();CHECK(!USB_Tx_State&&!USART_Rx_ptr_in&&!USART_Rx_ptr_out&&!USART_Rx_length);}
 else if(!strcmp(argv[1],"esof")){esof_counter=4;USB_Istr();CHECK(reset_poll<1000000);CHECK(bDeviceState==UNCONNECTED);}
 else if(!strcmp(argv[1],"busy")||!strcmp(argv[1],"idle")){USART_Rx_ptr_in=USART_RX_DATA_SIZE-1;USB_Tx_State=!strcmp(argv[1],"busy");
#ifdef HOST_FIXED
 CHECK(!USART_To_USB_Send_Data(data,1));
#else
 USART_To_USB_Send_Data(data,1);
#endif
 CHECK(USART_Rx_ptr_in==USART_RX_DATA_SIZE-1);}
 else if(!strcmp(argv[1],"large")){
#ifdef HOST_FIXED
 CHECK(USART_To_USB_Send_Data(data,sizeof(data)));
#else
 USART_To_USB_Send_Data(data,sizeof(data));
#endif
 for(i=0;i<100;i++){if(USB_Tx_State)EP1_IN_Callback();else Handle_USBAsynchXfer();}CHECK(wirelen==sizeof(data));CHECK(!memcmp(data,wire,sizeof(data)));
#ifdef HOST_FIXED
 /* Wrapping enqueue, stalled queue, whole-frame reservation and reset during wait. */
 USB_Tx_Reset();USART_Rx_ptr_in=USART_Rx_ptr_out=USART_RX_DATA_SIZE-100;wirelen=0;CHECK(USART_To_USB_Send_Data(data,sizeof(data)));for(i=0;i<100;i++){if(USB_Tx_State)EP1_IN_Callback();else Handle_USBAsynchXfer();}CHECK(wirelen==sizeof(data)&&!memcmp(data,wire,sizeof(data)));
 USB_Tx_Reset();USART_Rx_ptr_in=USART_RX_DATA_SIZE-10;CHECK(USB_Send_Debug(data,20)==9);CHECK(USART_Rx_ptr_in==USART_RX_DATA_SIZE-1);
 CHECK(!USART_To_USB_Send_Data(data,sizeof(data)));CHECK(USART_Rx_ptr_in==USART_RX_DATA_SIZE-1);
 pump=2;CHECK(!USART_To_USB_Send_Data(data,sizeof(data)));CHECK(!USART_Rx_ptr_in);
 bDeviceState=CONFIGURED;USB_Tx_Reset();USART_Rx_ptr_in=USART_RX_DATA_SIZE-1;pump=1;wirelen=0;CHECK(USART_To_USB_Send_Data(data,sizeof(data)));pump=0;
#endif
 }
 else CHECK(0);
 /* Largest current reply: log records, 1024 payload + 14 framing = 1038. Photo blocks are 512 + 22. */
 if(!strcmp(argv[1],"large")){
  uint8_t largest[1038];for(i=0;i<sizeof(largest);i++)largest[i]=(uint8_t)(i*7u);
  USB_Tx_Reset();wirelen=0;CHECK(USART_To_USB_Send_Data(largest,sizeof(largest)));
  for(i=0;i<100;i++){if(USB_Tx_State)EP1_IN_Callback();else Handle_USBAsynchXfer();}
  CHECK(wirelen==sizeof(largest)&&!memcmp(wire,largest,sizeof(largest)));
 }
 puts("TX scenario passed");return 0;}
