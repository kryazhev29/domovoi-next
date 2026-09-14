#include "boot_core.h"
#ifdef BOOT_HOST_TEST
#include <string.h>
#define __disable_irq() ((void)0)
#define __enable_irq() ((void)0)
void USART_To_USB_Send_Data(uint8_t *data,uint16_t size);
#else
#include "includes.h"
#endif
#include "boot_platform.h"
#ifdef __ICCARM__
#pragma location=".boot_version"
__root
#else
__attribute__((section(".boot_version"),used))
#define __disable_interrupt __disable_irq
#define __enable_interrupt __enable_irq
#endif
const uint8_t boot_identity[4]={BOOT_VERSION,BOOT_HW_VERSION,0,0xff};
static volatile uint16_t head,tail;
static volatile uint8_t overflow;
static uint8_t rx[2048],frame[600];
static uint16_t used;
static uint32_t received,expected,last_byte;
static uint8_t session,launch;
static uint16_t crc16(const uint8_t *p,uint16_t n) {
 uint16_t c=0xffff;unsigned i;
 while(n--){c^=*p++;for(i=0;i<8;i++)c=(c>>1)^((c&1)?0xa001:0);}
 return c;
}
static uint32_t read_le32(const uint8_t *p) {
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
void usb_datareceived(uint8_t *p,uint16_t n) {
 while(n--){uint16_t next=(head+1u)&2047u;if(next==tail){overflow=1;return;}rx[head]=*p++;head=next;}
}
static void reply(uint8_t cmd,uint8_t status) {
 uint8_t out[20]={'S','L','G','T',10,0,0x55,0};uint16_t c;
 out[7]=cmd;out[8]=status;out[9]=BOOT_VERSION;out[10]=BOOT_HW_VERSION;out[11]=0;
 memcpy(out+12,&received,4);c=crc16(out+6,10);out[16]=(uint8_t)c;out[17]=(uint8_t)(c>>8);
 USART_To_USB_Send_Data(out,18);
}
static void dispatch(const uint8_t *p,uint16_t n) {
 uint8_t cmd,status=BOOT_BAD_IMAGE;BootHeader h;uint32_t offset;
 if(n<2||p[0]!=0xaa)return;
 cmd=p[1];
 switch(cmd) {
 case 0:status=boot_platform_supported()?BOOT_OK:BOOT_UNSUPPORTED_FLASH;break;
 case 0x20: /* size:u32, CRC:u32, NUL-terminated HW.FW.TIMESTAMP.bin */
  session=0;received=0;
  if(!boot_platform_supported()){status=BOOT_UNSUPPORTED_FLASH;break;}
  if(n<12||p[n-1]!=0||memchr(p+10,0,n-10)!=p+n-1)break;
  if(!boot_parse_filename((const char*)p+10,&h))break;
  h.size=read_le32(p+2);h.crc=read_le32(p+6);
  if(h.size<8||h.size>BOOT_MAX_CODE||(h.size&3))break;
  status=(uint8_t)boot_prepare();if(status)break;
  status=(uint8_t)boot_stage_header(&h);if(status)break;
  expected=h.size;session=1;break;
 case 0x21: /* offset:u32, 1..256 payload bytes; sequential and retryable */
  if(!session||n<7||n>262)break;
  offset=read_le32(p+2);
  if(offset>expected||(uint32_t)(n-6)>expected-offset)break;
  if(offset<received) {
   uint8_t check[256];
   if((uint32_t)(n-6)<=received-offset&&boot_read(1,BOOT_HEADER_SIZE+offset,check,n-6)&&!memcmp(check,p+6,n-6))status=BOOT_OK;
   break;
  }
  if(offset!=received)break;
  if(!boot_ext_write(BOOT_HEADER_SIZE+offset,p+6,n-6)){status=BOOT_IO;break;}
  received+=n-6;status=BOOT_OK;break;
 case 0x22:
  if(n!=2||!session||received!=expected)break;
  status=(uint8_t)boot_request();if(!status){session=0;launch=1;}break;
 case 0x23: {
  uint32_t error=0;status=boot_read(1,BOOT_JOURNAL+12,&error,4)?(error==0xffffffffu?0:(uint8_t)error):BOOT_IO;break;
 }
 default:break;
 }
 reply(cmd,status);
}
static void poll(void) {
 uint16_t total,len;
 if(overflow){__disable_interrupt();tail=head;overflow=0;__enable_interrupt();used=0;session=0;}
 if(used&&boot_millis()-last_byte>2000u)used=0;
 while(tail!=head) {
  uint8_t ch=rx[tail];tail=(tail+1u)&2047u;last_byte=boot_millis();
  if(used<4){const char *magic="SLGT";if(ch!=(uint8_t)magic[used]){used=ch=='S'?1:0;frame[0]='S';continue;}}
  frame[used++]=ch;
  if(used<6)continue;
  len=(uint16_t)frame[4]|((uint16_t)frame[5]<<8);
  if(len<2||len>sizeof(frame)-8){used=0;continue;}
  total=len+8;if(used<total)continue;
  if(crc16(frame+6,len)==((uint16_t)frame[total-2]|((uint16_t)frame[total-1]<<8)))dispatch(frame+6,len);
  used=0;
 }
}
#ifndef BOOT_HOST_TEST
void main(void) {
 int held,result;
 boot_platform_init();held=boot_button();
 if(boot_platform_supported())(void)boot_run();
 /* boot_start_app() checks CRC and vectors itself. An intact application is
    started whatever boot_run() reported, so a failed external Flash does not
    keep the device offline in the bootloader. */
 if(!held)boot_start_app();
 boot_usb_init();
 for(;;) {
  boot_service();poll();
  if(launch) {
   uint32_t start=boot_millis();while(boot_millis()-start<100u)boot_service();
   launch=0;result=boot_run();
   if(result==BOOT_OK||result==BOOT_RESTORED)boot_start_app();
   reply(0x22,(uint8_t)result);
  }
 }
}


#endif
