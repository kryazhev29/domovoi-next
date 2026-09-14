#define main core_tests_main
#include "test_core.c"
#undef main
#define BOOT_HOST_TEST
#include "../boot_main.c"
static unsigned responses;
static uint8_t response[18];
uint32_t boot_millis(void){return 0;}
void USART_To_USB_Send_Data(uint8_t *p,uint16_t n){assert(n==18);memcpy(response,p,n);responses++;}
static unsigned request(uint8_t *out,uint8_t cmd,const void *p,unsigned n){
 uint16_t c;memcpy(out,"SLGT",4);out[4]=(uint8_t)(n+2);out[5]=(uint8_t)((n+2)>>8);out[6]=0xaa;out[7]=cmd;
 if(n)memcpy(out+8,p,n);c=crc16(out+6,n+2);out[8+n]=(uint8_t)c;out[9+n]=(uint8_t)(c>>8);return n+10;
}
static void clean(void){initial();head=tail=0;used=0;session=launch=overflow=0;received=expected=0;responses=0;}
int main(void){
 uint8_t packet[600],data[260],begin[100],garbage[2200];unsigned len,k,i;BootHeader h;
 clean();memcpy(&h,ext,sizeof(h));memcpy(begin,&h.size,4);memcpy(begin+4,&h.crc,4);strcpy((char*)begin+8,"139.15.1726300000.bin");
 len=request(packet,0x20,begin,8+(unsigned)strlen((char*)begin+8)+1);
 for(k=0;k<=len;k++){
  clean();usb_datareceived(packet,k);poll();assert(k==len||responses==0);
  usb_datareceived(packet+k,len-k);poll();assert(responses==1&&session&&response[8]==0);
 }
 puts("USB: every split position of BEGIN accepted exactly once");
 clean();len=request(packet,0,NULL,0);packet[len-1]^=1;usb_datareceived(packet,len);poll();assert(!responses&&!mutations);
 clean();memset(garbage,0xff,sizeof(garbage));usb_datareceived(garbage,sizeof(garbage));poll();assert(!responses&&!session&&!mutations);
 clean();memcpy(packet,"SLGT\xff\xff",6);usb_datareceived(packet,6);poll();assert(!responses&&!mutations);
 for(i=0;i<10000;i++){uint8_t byte=(uint8_t)(i*37u);usb_datareceived(&byte,1);poll();}assert(!mutations);
 clean();len=request(packet,0x20,begin,8+(unsigned)strlen((char*)begin+8)+1);usb_datareceived(packet,len);poll();assert(session);
 memset(data,0x55,sizeof(data));memset(data,0,4);len=request(packet,0x21,data,sizeof(data));usb_datareceived(packet,len);poll();assert(received==256&&response[8]==0);
 usb_datareceived(packet,len);poll();assert(received==256&&response[8]==0);
 data[0]=1;len=request(packet,0x21,data,sizeof(data));usb_datareceived(packet,len);poll();assert(received==256&&response[8]!=0);
 len=request(packet,0x22,NULL,0);usb_datareceived(packet,len);poll();assert(!launch&&response[8]!=0);
 puts("USB: bad CRC/length, overflow, noise, retry, offset and premature commit rejected safely");return 0;
}
