#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#include "../boot_manual.c"
static uint8_t stage[MANUAL_BOOT_SIZE],image[MANUAL_BOOT_SIZE];
static int writes,launches,supported=1,io_fail,v5_only;
int manual_board_hw(uint8_t hw){return v5_only?hw==139:1;}
int manual_stage_read(uint32_t off,void *p,uint32_t n){assert(off+n<=sizeof(stage));memcpy(p,stage+off,n);return !io_fail;}
int manual_stage_write(uint32_t off,const void *p,uint32_t n){uint32_t i;const uint8_t *b=p;assert(off+n<=sizeof(stage));for(i=0;i<n;i++)stage[off+i]&=b[i];writes++;return !io_fail;}
int manual_stage_erase(uint32_t off){assert(!(off&4095u)&&off+4096<=sizeof(stage));memset(stage+off,255,4096);writes++;return !io_fail;}
int manual_platform_supported(void){return supported;}
int manual_program(uint32_t crc){(void)crc;launches++;return 1;}
void manual_service(void){}
static void put32(uint8_t *p,uint32_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);}
static void powercycle(void){session=armed=target_hw=0;received=expected_crc=0;}
static uint8_t call(int usb,const uint8_t *p,uint16_t n){uint8_t reply[8];boot_manual_command(usb,p,n,reply);return reply[1];}
static void make_image(uint8_t hw){memset(image,0x5a,sizeof(image));put32(image,0x20005000);put32(image+4,0x08000101);image[32764]=BOOT_VERSION;image[32765]=hw;image[32766]=0;image[32767]=255;}
static uint32_t checksum(void){uint32_t i,c=0xffffffffu;for(i=0;i<sizeof(image);i+=4)c=crc_word(c,get32(image+i));return c;}
static void begin(uint8_t hw){uint8_t p[10]={0};put32(p+1,32768);put32(p+5,checksum());p[9]=hw;assert(call(1,p,10)==0);}
static void block(uint32_t off){uint8_t p[261];p[0]=1;put32(p+1,off);memcpy(p+5,image+off,256);assert(call(1,p,sizeof(p))==0);}
static void send_all(void){uint32_t off;for(off=0;off<sizeof(image);off+=256)block(off);}
int main(void){
 uint8_t p[261]={0},commit=2,cancel=3;uint32_t cut,off;int before;
 make_image(139);assert(boot_manual_apply()==MANUAL_DENIED&&launches==0);
 put32(p+1,32768);put32(p+5,checksum());p[9]=139;
 assert(call(0,p,10)==MANUAL_DENIED&&writes==0);
 supported=0;assert(call(1,p,10)==MANUAL_DENIED&&writes==0);supported=1;
 put32(p+1,32769);assert(call(1,p,10)==MANUAL_BAD&&writes==0);
 begin(139);assert(call(1,&commit,1)==MANUAL_BAD);block(0);before=writes;block(0);assert(writes==before);
 p[0]=1;put32(p+1,0);memcpy(p+5,image,256);p[9]^=1;assert(call(1,p,261)==MANUAL_BAD);
 assert(call(1,&cancel,1)==0&&!boot_manual_busy());assert(boot_manual_apply()==MANUAL_DENIED);
 begin(11);send_all();assert(call(1,&commit,1)==MANUAL_BAD);assert(launches==0);
 assert(call(1,&cancel,1)==0);begin(139);send_all();stage[123]^=1;assert(call(1,&commit,1)==MANUAL_CRC&&launches==0);
 assert(call(1,&cancel,1)==0);begin(139);send_all();assert(call(1,&commit,1)==0);assert(launches==0);
 assert(call(1,&cancel,1)==MANUAL_BUSY);stage[123]^=1;assert(boot_manual_apply()==MANUAL_CRC&&launches==0);
 powercycle();make_image(11);begin(11);send_all();assert(call(1,&commit,1)==0);assert(boot_manual_apply()==0&&launches==1);

 for(cut=0;cut<=128;cut++){
  powercycle();make_image(139);begin(139);
  for(off=0;off<cut*256;off+=256)block(off);
  if(cut==128)assert(call(1,&commit,1)==0);
  powercycle();assert(!boot_manual_busy());assert(boot_manual_apply()==MANUAL_DENIED&&launches==1);
 }

 powercycle();v5_only=1;make_image(11);before=writes;
 put32(p+1,32768);put32(p+5,checksum());p[0]=0;p[9]=11;
 assert(call(1,p,10)==MANUAL_BAD&&writes==before&&!boot_manual_busy());
 make_image(139);begin(139);v5_only=0;assert(call(1,&cancel,1)==0);
 io_fail=1;put32(p+1,32768);put32(p+5,checksum());p[0]=0;p[9]=139;
 assert(call(1,p,10)==MANUAL_IO&&!boot_manual_busy());
 puts("manual: validation, USB-only, explicit apply, retries, corruption, 129 reset positions passed");return 0;
}
