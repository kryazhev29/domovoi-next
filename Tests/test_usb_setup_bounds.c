// Доработка: Кряжев А., 2026
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef unsigned RESULT;
typedef struct {unsigned Total_Endpoint;} DEVICE;
static DEVICE Device_Table={4};
static struct {unsigned USBwIndex0,USBwIndex1,USBwValue,Current_Feature,Current_Configuration;struct {unsigned Usb_wLength;} Ctrl_Info;} info,*pInformation=&info;
static struct {unsigned MaxPacketSize;} Device_Property={64};
static unsigned mmio;
static union {uint16_t w;struct {uint8_t bb1,bb0;} bw;} StatusInfo;
#define STANDARD_REQUEST 0
#define DEVICE_RECIPIENT 0
#define INTERFACE_RECIPIENT 1
#define ENDPOINT_RECIPIENT 2
#define Type_Recipient ENDPOINT_RECIPIENT
#define ENDPOINT_STALL 0
#define ENDP0 0
#define USB_SUCCESS 0
#define USB_UNSUPPORT 1
#define EP_TX_VALID 1
#define EP_RX_VALID 1
#define EP_TX_STALL 2
#define EP_RX_STALL 2
// Зафиксировать MMIO и проверить границу номера конечной точки.
static unsigned touch(unsigned ep){CHECK(ep<8&&ep<Device_Table.Total_Endpoint);mmio++;return 1;}
#define _GetEPTxStatus touch
#define _GetEPRxStatus touch
#define _GetTxStallStatus touch
#define _GetRxStallStatus touch
#define ClearDTOG_TX(ep) ((void)touch(ep))
#define ClearDTOG_RX(ep) ((void)touch(ep))
#define SetEPTxStatus(ep,v) ((void)(v),(void)touch(ep))
#define _SetEPTxStatus SetEPTxStatus
#define _SetEPRxStatus SetEPTxStatus
#define SetEPRxCount SetEPTxStatus
// Имитировать callback стандартного запроса без действия.
static void noop(void){}
static struct {void (*User_GetStatus)(void),(*User_ClearFeature)(void),(*User_SetEndPointFeature)(void);} requests={noop,noop,noop},*pUser_Standard_Requests=&requests;
/* КОД */
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){
 info.Current_Configuration=1;
 for(unsigned total=1;total<=8;total++){Device_Table.Total_Endpoint=total;
  for(unsigned index=0;index<65536;index++){
   info.USBwIndex0=index&255;info.USBwIndex1=index>>8;
   bool valid=(index&~0x8fu)==0&&(index&15)<total;
   mmio=0;CHECK((Standard_ClearFeature()==USB_SUCCESS)==valid);CHECK(valid||!mmio);
   mmio=0;CHECK((Standard_SetEndPointFeature()==USB_SUCCESS)==valid);CHECK(valid||!mmio);
   mmio=0;CHECK((audit_getstatus()!=NULL)==valid);CHECK(valid||!mmio);
   mmio=0;CHECK((Standard_GetStatus(2)!=NULL)==valid);CHECK(valid||!mmio);
  }
 }
 puts("B32: all 65536 wIndex values for each 1..8 endpoint count; invalid SETUP before MMIO PASS");
}
