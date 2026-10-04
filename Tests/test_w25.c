#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t Int32U;
typedef uint16_t Int16U;
typedef uint8_t Int8U;
typedef uint8_t *pInt8U;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define FLASH_W25_PAGE_SIZE 256
#define asm(x) ((void)0)
void w25_clear_status(void);
Int8U w25_read_status(void);
#if HAVE_RESULT
static bool w25_verify(Int32U,const uint8_t *,Int16U);
#endif
static uint8_t flash[8192], payload[800];
static unsigned cmd, address, bytes, wel, stuck, reject_wren, drop_write, drop_erase, programs, erases, reads, fail_page;
static void cs_on(void){cmd=address=bytes=0;}
static void cs_off(void){if(cmd==0x06&&!reject_wren&&!stuck)wel=2;if(cmd==0x04)wel=0;
 if(cmd==0x02){programs++;wel=0;}
 if(cmd==0x20){erases++;if(wel&&!stuck&&!drop_erase)memset(flash+(address&~4095u),255,4096);wel=0;}}
#define w25_chip_ON() cs_on()
#define w25_chip_OFF() cs_off()
static void wdt_reload(void){}
uint8_t spi_ready(void){return 1;}
static void spi_send_byte(uint8_t x){if(!cmd){cmd=x;return;}if((cmd==0x03||cmd==0x02||cmd==0x20)&&bytes++<3)address=(address<<8)|x;}
static uint8_t spi_read_byte(void){if(cmd==0x05)return (uint8_t)(wel|(stuck?1:0));CHECK(cmd==0x03);CHECK(address<sizeof(flash));reads++;return flash[address++];}
static void spi_read(uint8_t *p,uint16_t n){while(n--)*p++=spi_read_byte();}
static void spi_send(uint8_t *p,uint16_t n){CHECK(cmd==0x02&&bytes==3);CHECK((address&255)+n<=256);
 if(wel&&!stuck&&!drop_write&&(!fail_page||programs+1!=fail_page)){while(n--){CHECK(address<sizeof(flash));flash[address++]&=*p++;}}}
/* PRODUCTION */
static bool write_result(uint32_t a,uint8_t *p,uint16_t n){
#if HAVE_RESULT
 return w25_write_data(a,p,n);
#else
 w25_write_data(a,p,n);return true;
#endif
}
static bool erase_result(uint32_t a){
#if HAVE_RESULT
 return w25_erase_sector(a);
#else
 w25_erase_sector(a);return true;
#endif
}
static void reset(void){memset(&w25_job,0,sizeof(w25_job));memset(flash,255,sizeof(flash));memset(payload,0x5A,sizeof(payload));stuck=reject_wren=drop_write=drop_erase=programs=erases=reads=wel=fail_page=0;}
int main(void){
 reset();stuck=1;CHECK(!write_result(17,payload,300));CHECK(!programs);
 reset();reject_wren=1;CHECK(!write_result(17,payload,300));CHECK(!programs);
 reset();drop_write=1;CHECK(!write_result(17,payload,300));CHECK(programs==1);
 reset();fail_page=2;CHECK(!write_result(17,payload,600));CHECK(programs==2);
 reset();CHECK(write_result(17,payload,600));CHECK(!memcmp(flash+17,payload,600)&&programs==3);
 reset();memset(flash,0,sizeof(flash));drop_erase=1;CHECK(!erase_result(27));CHECK(erases==1);
 reset();memset(flash,0,sizeof(flash));CHECK(erase_result(27));{unsigned i;for(i=0;i<4096;i++)CHECK(flash[i]==255);}
 reset();stuck=1;CHECK(!erase_result(0));CHECK(!erases);

 reset();CHECK(w25_async_start(17,payload,64,0)); CHECK(!w25_async_start(0,NULL,0,0));
 CHECK(!w25_write_data(100,payload,1)); CHECK(!w25_erase_sector(4096));
 stuck=1; for(unsigned now=1;now<20;now++) CHECK(w25_async_poll(now)==1);
 CHECK(w25_async_poll(20)==3&&!w25_async_active());
 reset(); CHECK(w25_async_start(0,payload,64,0xFFFFFFFAu)); stuck=1;
 CHECK(w25_async_poll(5)==1); CHECK(w25_async_poll(14)==3);
 reset(); drop_write=1; CHECK(w25_async_start(0,payload,64,0)); CHECK(w25_async_poll(1)==3);
 reset(); reject_wren=1; CHECK(!w25_async_start(0,payload,64,0)); CHECK(!programs);
 reset(); CHECK(!w25_async_start(250,payload,64,0)); CHECK(!programs);
 reset(); memset(flash,0,sizeof(flash)); CHECK(w25_async_start(0,NULL,0,0));
 for(unsigned i=0;i<64;i++){unsigned before=reads;CHECK(w25_async_poll(i)==(i==63?2:1));CHECK(reads-before<=64);}
 CHECK(erases==1&&!w25_async_active());
 reset(); memset(flash,0,sizeof(flash));drop_erase=1;CHECK(w25_async_start(0,NULL,0,0));CHECK(w25_async_poll(1)==3);
 puts("w25 async: BUSY, ownership, deadline/wrap, WEL, dropped erase/program, bounded verification passed");
 puts("w25: real driver BUSY/WEL/program/erase/readback/page-stop passed");return 0;
}
