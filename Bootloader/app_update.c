#include "../includes.h"
#include "app_update.h"
#include "boot_core.h"
#ifdef __ICCARM__
#include <intrinsics.h>
#else
#define __ramfunc __attribute__((section(".ramfunc"),noinline))
#define __disable_interrupt __disable_irq
#endif
extern device_s device;
static uint32_t crc_word(uint32_t c,uint32_t v) {
 unsigned i;c^=v;for(i=0;i<32;i++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0);return c;
}
int app_boot_request(void) {
 BootHeader h;uint32_t crc=0xffffffffu,words[32],pos,n,i,magic=BOOT_MAGIC,check;
 if(BOOT_VER<BOOT_VERSION||*(const volatile uint16_t*)BOOT_FLASH_SIZE_ADDRESS!=BOOT_REQUIRED_FLASH_KIB)return 0;
 mem_read(BOOT_CANDIDATE,(uint8_t*)&h,sizeof(h));
 if(h.hardware!=BOOTHW_VER||h.size<8||h.size>BOOT_MAX_CODE||(h.size&3))return 0;
 for(pos=0;pos<h.size;pos+=n) {
  n=h.size-pos;if(n>sizeof(words))n=sizeof(words);
  mem_read(BOOT_HEADER_SIZE+pos,(uint8_t*)words,n);
  if(pos==0&&(words[0]<0x20000008u||words[0]>0x20005000u||(words[0]&7u)||
      !(words[1]&1u)||(words[1]&~1u)<BOOT_APP_VECTOR||(words[1]&~1u)>=BOOT_APP_VECTOR+h.size))return 0;
  for(i=0;i<n/4;i++)crc=crc_word(crc,words[i]);wdt_reload();
 }
 if(crc!=h.crc)return 0;
 mem_erase_sector(BOOT_JOURNAL);
 for(pos=0;pos<4096;pos+=sizeof(words)) {
  mem_read(BOOT_JOURNAL+pos,(uint8_t*)words,sizeof(words));
  for(i=0;i<32;i++)if(words[i]!=0xffffffffu)return 0;
 }
 mem_write(BOOT_CANDIDATE,(uint8_t*)&magic,4);
 mem_read(BOOT_CANDIDATE,(uint8_t*)&check,4);return check==magic;
}
void app_boot_metadata(void) {
 const BootHeader *h=(const BootHeader*)BOOT_APP_BASE;uint32_t result;
 device.bootVersion=BOOT_VER;device.hardwareVersion=BOOTHW_VER;
 if(h->magic==BOOT_MAGIC||h->magic==BOOT_LEGACY_MAGIC) {
  device.firmwareVersion=(h->magic==BOOT_LEGACY_MAGIC)?(h->version&255u):h->version;device.buildDate=h->timestamp;
  device.firmwareSize=h->size;device.firmwareCRC=h->crc;
 }
 if(BOOT_VER>=BOOT_VERSION) {
  mem_read(BOOT_JOURNAL+12,(uint8_t*)&result,4);
  if(result==BOOT_PROGRAM_ERROR)device.firmwarePercent=252;
 }
}
/* Runs entirely in SRAM; no calls, literals in Flash or interrupt handlers.
 * Reset is intentional even on failure: do not continue normal operation
 * with a partly replaced boot area. External power must remain stable. */
__ramfunc static void replace_ram(const uint8_t *image) {
 uint32_t page,i,timeout;uint16_t half;
 volatile uint32_t *sr=(volatile uint32_t*)0x4002200cu;
 volatile uint32_t *cr=(volatile uint32_t*)0x40022010u;
 volatile uint32_t *ar=(volatile uint32_t*)0x40022014u;
 volatile uint32_t *kr=(volatile uint32_t*)0x40022004u;
 __disable_interrupt();
 *kr=0x45670123u;*kr=0xcdef89abu;
 for(page=0x08000000u;page<BOOT_APP_BASE;page+=1024) {
  *(volatile uint32_t*)0x40003000u=0xaaaau;
  *sr=0x34u;*cr=2u;*ar=page;*cr=0x42u;
  timeout=2000000u;while((*sr&1u)&&--timeout){}
  if(!timeout||(*sr&0x14u))break;
  *cr=0;
  for(i=0;i<1024;i+=2) {
   half=(uint16_t)image[page-0x08000000u+i]|((uint16_t)image[page-0x08000000u+i+1]<<8);
   *sr=0x34u;*cr=1u;*(volatile uint16_t*)(page+i)=half;
   timeout=100000u;while((*sr&1u)&&--timeout){}
   if(!timeout||(*sr&0x14u)||*(volatile uint16_t*)(page+i)!=half)break;
   *cr=0;
  }
  if(i!=1024)break;
 }
 *cr=0x80u;
 *(volatile uint32_t*)0xe000ed0cu=0x05fa0004u;
 for(;;){}
}
int app_boot_replace(const uint8_t *image,uint32_t size,uint32_t crc) {
 uint32_t c=0xffffffffu,i,w,sp,pc,address=(uint32_t)image;
 if(*(const volatile uint16_t*)BOOT_FLASH_SIZE_ADDRESS!=BOOT_REQUIRED_FLASH_KIB)return 0;
 if(size!=0x8000u||(address&3u)||address<BOOT_APP_VECTOR||address>BOOT_FLASH_END-size)return 0;
 memcpy(&sp,image,4);memcpy(&pc,image+4,4);
 if(sp<0x20000008u||sp>0x20005000u||(sp&7u)||!(pc&1u)||(pc&~1u)<0x08000000u||(pc&~1u)>=0x08007ffcu||
    image[0x7ffc]!=BOOT_VERSION||image[0x7ffd]!=BOOTHW_VER)return 0;
 for(i=0;i<size;i+=4){memcpy(&w,image+i,4);c=crc_word(c,w);wdt_reload();}
 if(c!=crc)return 0;
 replace_ram(image);return 0;
}

#ifdef DOMOVOI_BOOT_MIGRATION
#include "generated/boot_image_v5.h"
#endif
void app_boot_migrate_if_needed(void) {
#ifdef DOMOVOI_BOOT_MIGRATION
 /* Dedicated transition firmware for the current v5 application project.
    Never select the MINI image based only on an old version number. */
 if((BOOT_VER==0x21u||BOOT_VER==0x22u||BOOT_VER==0x30u)&&BOOTHW_VER==0x8bu)
  app_boot_replace(domovoi_boot_image,sizeof(domovoi_boot_image),DOMOVOI_BOOT_IMAGE_CRC);
#endif
}
