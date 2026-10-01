/* Разработчик: Кряжев А., 2026 */
#include "../includes.h"
#include "app_update.h"
#include "boot_core.h"
#include "boot_manual.h"
#ifdef __ICCARM__
#include <intrinsics.h>
#else
#define __ramfunc __attribute__((section(".ramfunc"),noinline))
#define __disable_interrupt __disable_irq
#endif
extern device_s device;
/* Неподтверждённая проба просит откат через столько секунд. */
#define TRIAL_SECONDS 1800u
static uint8_t trial;
// Таймер 6 отведён пробе; его 32-битный счётчик уже существует в timers.c
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
 /* Журнал и резервная копия принадлежат идущей пробе, пока она не закончится. Метка
    подготовленного кандидата остаётся стёртой, пока журнал не очищен: по ней загрузчик отличает
    оборванное стирание журнала от пробы. */
 if(trial||h.magic!=0xffffffffu)return 0;
 mem_erase_sector(BOOT_JOURNAL);
 for(pos=0;pos<4096;pos+=sizeof(words)) {
  mem_read(BOOT_JOURNAL+pos,(uint8_t*)words,sizeof(words));
  for(i=0;i<32;i++)if(words[i]!=0xffffffffu)return 0;
 }
 mem_write(BOOT_CANDIDATE,(uint8_t*)&magic,4);
 mem_read(BOOT_CANDIDATE,(uint8_t*)&check,4);return check==magic;
}
void app_boot_metadata(void) {
 timers_set(6,0);
 const BootHeader *h=(const BootHeader*)BOOT_APP_BASE;uint32_t result;
 device.bootVersion=BOOT_VER;
 /* Старые загрузчики не определяют плату надёжно. */
 device.hardwareVersion=(BOOT_VER>=BOOT_VERSION&&BOOT_VER!=0xffu)?BOOTHW_VER:0;
 if(h->magic==BOOT_MAGIC||h->magic==BOOT_LEGACY_MAGIC) {
  device.firmwareVersion=(h->magic==BOOT_LEGACY_MAGIC)?(h->version&255u):h->version;device.buildDate=h->timestamp;
  device.firmwareSize=h->size;device.firmwareCRC=h->crc;
 }
 if(BOOT_VER>=BOOT_VERSION&&BOOT_VER!=0xffu) {
  uint32_t done,armed,confirmed;
  mem_read(BOOT_JOURNAL+12,(uint8_t*)&result,4);
  mem_read(BOOT_JOURNAL+4,(uint8_t*)&done,4);
  mem_read(BOOT_JOURNAL+BOOT_JOURNAL_TRIAL,(uint8_t*)&armed,4);
  mem_read(BOOT_JOURNAL+BOOT_JOURNAL_CONFIRMED,(uint8_t*)&confirmed,4);
  /* То же правило, что в загрузчике: взводит и подтверждает только точный 0. */
  trial=(done!=0xffffffffu&&armed==0&&confirmed!=0&&result==0xffffffffu)?1u:0u;
 }
}
int app_trial_active(void) {return trial;}
/* Записывается и читается обратно; если запись не удалась, пробу подтвердит следующий ответ на
 * регистрацию. */
void app_trial_confirm(void) {
 uint32_t zero=0,check;
 if(!trial)return;
 mem_write(BOOT_JOURNAL+BOOT_JOURNAL_CONFIRMED,(uint8_t*)&zero,4);
 mem_read(BOOT_JOURNAL+BOOT_JOURNAL_CONFIRMED,(uint8_t*)&check,4);
 if(check==0)trial=0;
}
/* Загрузчик восстановит резервную копию при следующем старте. */
void app_trial_rollback(void) {
 uint32_t zero=0;
 if(!trial)return;
 mem_write(BOOT_JOURNAL+BOOT_JOURNAL_ROLLBACK,(uint8_t*)&zero,4);
}
int app_trial_expired(void) {
 if(!trial)return 0;
 return timers_get(6)>=TRIAL_SECONDS*1000u;
}
/* Вызывается после того, как ошибка обновления отправлена на сервер. Без отметки ошибка уходила
 * бы после каждого перезапуска. FF->00 не требует стирания; следующий запрос обновления стирает
 * журнал вместе с отметкой. */
int app_boot_error_unreported(void) {
 uint32_t result,reported;
 if(BOOT_VER<BOOT_VERSION||BOOT_VER==0xffu)return 0;
 mem_read(BOOT_JOURNAL+12,(uint8_t*)&result,4);
 mem_read(BOOT_JOURNAL+BOOT_JOURNAL_REPORTED,(uint8_t*)&reported,4);
 return result!=0xffffffffu&&reported==0xffffffffu;
}
/* Ошибкой считается любой записанный код, в том числе оборванный пропаданием питания. */
void app_boot_error_text(char *text) {
 uint32_t result;
 mem_read(BOOT_JOURNAL+12,(uint8_t*)&result,4);
 if(result==BOOT_TRIAL_FAILED){strcpy(text,"TRIAL FAIL");return;}
 if(result==BOOT_BACKUP_ERROR){strcpy(text,"BACKUP");return;}
 strcpy(text,"BOOT ?");
 if(result<=9u)text[5]=(char)('0'+result);
}
void app_boot_error_reported(void) {
 uint32_t reported=0;
 if(!app_boot_error_unreported())return;
 mem_write(BOOT_JOURNAL+BOOT_JOURNAL_REPORTED,(uint8_t*)&reported,4);
}
/* Кандидат готовится через имеющийся драйвер внешней флеш-памяти работающей прошивки.
 * Встроенного образа загрузчика нет, автоматического запроса при старте нет. */
int manual_platform_supported(void) {
 return *(const volatile uint16_t*)BOOT_FLASH_SIZE_ADDRESS==BOOT_REQUIRED_FLASH_KIB;
}
/* Эта прошивка собирается только для платы v5 (HW 139). Загрузчик MINI (HW 11) управляет
 * другими выводами: кнопкой, подтяжкой USB и выбором SPI. */
/* Загрузчик своего номера. Сборка IP_v3 с номером 140 (на платах, где 29.09.2026 успели
   поставить загрузчик 140) принимает и загрузчик 139 - возврат к общему номеру.
   Доработка: Кряжев А., 2026 */
int manual_board_hw(uint8_t hw) {
#if defined(BOARD_IP_V3) && DOMOVOI_APP_HW == 140
 return hw==139u||hw==140u;
#else
 return hw==(uint8_t)DOMOVOI_APP_HW;
#endif
}
int manual_stage_read(uint32_t off,void *data,uint32_t n) {
 if(off>MANUAL_BOOT_SIZE||n>MANUAL_BOOT_SIZE-off)return 0;
 mem_read(BOOT_CANDIDATE+off,data,n);return 1;
}
int manual_stage_write(uint32_t off,const void *data,uint32_t n) {
 if(off>MANUAL_BOOT_SIZE||n>MANUAL_BOOT_SIZE-off)return 0;
 mem_write(BOOT_CANDIDATE+off,(uint8_t*)data,n);return 1;
}
int manual_stage_erase(uint32_t off) {
 if(off>=MANUAL_BOOT_SIZE||(off&4095u))return 0;
 mem_erase_sector(BOOT_CANDIDATE+off);return 1;
}
void manual_service(void) {wdt_reload();}
/* Все функции, доступные после стирания первого сектора загрузчика, находятся в ОЗУ. SPI1 и
 * выбор микросхемы работающей прошивки уже настроены. */
__ramfunc static int ram_spi(uint8_t tx) {
 uint32_t timeout=100000u;
 while(!(SPI1->SR&2u)&&--timeout){}
 if(!timeout)return -1;
 SPI1->DR=tx;timeout=100000u;
 while(!(SPI1->SR&1u)&&--timeout){}
 if(!timeout)return -1;
 tx=(uint8_t)SPI1->DR;timeout=100000u;
 while((SPI1->SR&128u)&&--timeout){}
 return timeout?(int)tx:-1;
}
__ramfunc static int ram_read_page(uint32_t offset,uint8_t *data) {
 uint32_t i;int byte;
 SPI_CS_PORT->BRR=SPI_CS_PIN;
 if(ram_spi(3)<0||ram_spi((uint8_t)(offset>>16))<0||
    ram_spi((uint8_t)(offset>>8))<0||ram_spi((uint8_t)offset)<0)goto fail;
 for(i=0;i<1024;i++) {byte=ram_spi(255);if(byte<0)goto fail;data[i]=(uint8_t)byte;}
 SPI_CS_PORT->BSRR=SPI_CS_PIN;return 1;
fail:
 SPI_CS_PORT->BSRR=SPI_CS_PIN;return 0;
}
__ramfunc static uint32_t ram_page_crc(const uint8_t *data) {
 uint32_t i,j,w,c=0xffffffffu;
 for(i=0;i<1024;i+=4) {
  w=(uint32_t)data[i]|((uint32_t)data[i+1]<<8)|((uint32_t)data[i+2]<<16)|((uint32_t)data[i+3]<<24);
  c^=w;for(j=0;j<32;j++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0u);
 }
 return c;
}
__ramfunc static void replace_ram(const uint32_t *page_crc) {
 uint32_t page,i,timeout;uint16_t half;uint8_t image[1024];
 volatile uint32_t *sr=(volatile uint32_t*)0x4002200cu;
 volatile uint32_t *cr=(volatile uint32_t*)0x40022010u;
 volatile uint32_t *ar=(volatile uint32_t*)0x40022014u;
 volatile uint32_t *kr=(volatile uint32_t*)0x40022004u;
 __disable_interrupt();
 *kr=0x45670123u;*kr=0xcdef89abu;
 for(page=0x08000000u;page<BOOT_APP_BASE;page+=1024) {
  *(volatile uint32_t*)0x40003000u=0xaaaau;
  /* Каждое чтение по SPI проверяется до стирания своей страницы внутренней флеш-памяти. */
  if(!ram_read_page(BOOT_CANDIDATE+page-0x08000000u,image)||
     ram_page_crc(image)!=page_crc[(page-0x08000000u)/1024])break;
  *sr=0x34u;*cr=2u;*ar=page;*cr=0x42u;
  timeout=2000000u;while((*sr&1u)&&--timeout){}
  if(!timeout||(*sr&0x14u))break;
  *cr=0;
  for(i=0;i<1024;i+=2) {
   half=(uint16_t)image[i]|((uint16_t)image[i+1]<<8);
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
int manual_program(uint32_t expected_crc) {
 uint32_t page_crc[32],page,i,w,c,whole=0xffffffffu;uint8_t data[256];
 if(!manual_platform_supported())return 0;
 for(page=0;page<32;page++) {
  c=0xffffffffu;
  for(i=0;i<1024;i+=sizeof(data)) {
   unsigned j;if(!manual_stage_read(page*1024+i,data,sizeof(data)))return 0;
   for(j=0;j<sizeof(data);j+=4){memcpy(&w,data+j,4);c=crc_word(c,w);whole=crc_word(whole,w);}
  }
  page_crc[page]=c;wdt_reload();
 }
 if(whole!=expected_crc)return 0;
 replace_ram(page_crc);return 0;
}
