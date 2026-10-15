/* Доработка: Кряжев А., 2026.
   Передаёт готовый образ загрузчику и ведёт подтверждение пробной прошивки.
   Журнал загрузчика хранит результат, подтверждение и запрос отката после обрыва питания.
   W25 проверяет каждую запись и стирание; здесь проверяются образ и прочитанные метки.
   Ручная замена загрузчика ниже выполняется из ОЗУ и не меняет адреса приложения.
*/
#include "../includes.h"
#include "app_update.h"
#include "../User/c/runtime_internal.h"
#include "../User/c/user_fw_reason.h"
#include "boot_core.h"
#include "boot_manual.h"
#ifdef __ICCARM__
#include <intrinsics.h>
#else
#define __ramfunc __attribute__((section(".ramfunc"),noinline))
#define __disable_interrupt __disable_irq
#endif
/* Неподтверждённая проба длится 30 минут от чтения метаданных. */
#define TRIAL_SECONDS 1800u
static uint8_t trial;
static uint32_t trial_started;

// Проверить и опубликовать кандидата одним синхронным шагом владельца STORE.
int app_boot_request(void)
{
  flash_exec_args_t args = { NULL, FS_SYNC };
  if (!flash_exec_begin(FX_STORE, &args)) return -1;
  BootHeader header;
  uint32_t words[32], crc = UINT32_MAX, magic = BOOT_MAGIC;
  bool ok = false;
  if (BOOT_VER < BOOT_VERSION || trial ||
      *(const volatile uint16_t *)BOOT_FLASH_SIZE_ADDRESS != BOOT_REQUIRED_FLASH_KIB ||
      !mem_read(BOOT_CANDIDATE, (uint8_t *)&header, sizeof(header))) goto done;
  if (header.hardware != BOOTHW_VER || header.size < 8 || header.size > BOOT_MAX_CODE ||
      (header.size & 3u) || header.magic != UINT32_MAX) goto done;
  for (uint32_t pos = 0; pos < header.size; pos += sizeof(words)) {
    uint32_t count = header.size - pos;
    if (count > sizeof(words)) count = sizeof(words);
    if (!mem_read(BOOT_CANDIDATE + BOOT_HEADER_SIZE + pos, (uint8_t *)words, (uint16_t)count)) goto done;
    if (!pos && (words[0] < 0x20000008u || words[0] > 0x20005000u || (words[0] & 7u) ||
        !(words[1] & 1u) || (words[1] & ~1u) < BOOT_APP_VECTOR ||
        (words[1] & ~1u) >= BOOT_APP_VECTOR + header.size)) goto done;
    for (uint32_t i = 0; i < count / 4; ++i) crc = app_crc_word(crc, words[i]);
    wdt_reload();
  }
  ok = crc == header.crc && mem_erase_sector(BOOT_JOURNAL) &&
       mem_write(BOOT_CANDIDATE, (uint8_t *)&magic, sizeof(magic));
done:
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  return ok;
}

// Прочитать версию приложения и состояние пробы из долговечного журнала.
void app_boot_metadata(void)
{
  const BootHeader *header = (const BootHeader *)BOOT_APP_BASE;
  trial_started = timers_getMillis();
  device.bootVersion = BOOT_VER;
  /* Старый загрузчик не определяет плату надёжно. */
  bool current_boot = BOOT_VER >= BOOT_VERSION && BOOT_VER != 0xffu;
  device.hardwareVersion = current_boot ? BOOTHW_VER : 0;
  if (header->magic == BOOT_MAGIC || header->magic == BOOT_LEGACY_MAGIC) {
    device.firmwareVersion = header->magic == BOOT_LEGACY_MAGIC ? header->version & 255u : header->version;
    device.buildDate = header->timestamp;
    device.firmwareSize = header->size;
    device.firmwareCRC = header->crc;
  }
  if (current_boot) {
    uint32_t journal[8];
    if (!mem_read(BOOT_JOURNAL, (uint8_t *)journal, sizeof(journal))) {
      /* Неизвестная проба запрещает новую установку до подтверждения сервером. */
      trial = 1;
      return;
    }
    /* Как в загрузчике: взводит и подтверждает только точный ноль. */
    trial = journal[1] != UINT32_MAX && journal[BOOT_JOURNAL_TRIAL / 4u] == 0 &&
      journal[BOOT_JOURNAL_CONFIRMED / 4u] != 0 && journal[3] == UINT32_MAX;
  }
}

// Сообщить, требует ли текущая проба подтверждения сервером.
int app_trial_active(void) { return trial; }

// Снять пробу только после записи подтверждения, проверенной драйвером W25.
void app_trial_confirm(void)
{
  uint32_t zero = 0;
  flash_exec_args_t args = { NULL, FS_SYNC };
  if (!trial || !flash_exec_begin(FX_STORE, &args)) return;
  bool ok = mem_write(BOOT_JOURNAL + BOOT_JOURNAL_CONFIRMED, (uint8_t *)&zero, sizeof(zero));
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  if (ok) trial = 0;
}

// Запросить восстановление резервной копии при следующем старте загрузчика.
int app_trial_rollback(void)
{
  uint32_t zero = 0;
  flash_exec_args_t args = { NULL, FS_SYNC };
  if (!trial || !flash_exec_begin(FX_STORE, &args)) return 0;
  bool ok = mem_write(BOOT_JOURNAL + BOOT_JOURNAL_ROLLBACK, (uint8_t *)&zero, sizeof(zero));
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  return ok;
}

// Проверить срок неподтверждённой пробы по времени от чтения метаданных.
int app_trial_expired(void)
{
  // Вычитание сохраняет срок и при переполнении миллисекундного счётчика.
  return trial && (uint32_t)(timers_getMillis() - trial_started) >= TRIAL_SECONDS * 1000u;
}

// Найти ещё не переданную серверу ошибку; отказ чтения не считается событием.
int app_boot_error_unreported(void)
{
  uint32_t result, reported;
  if (BOOT_VER < BOOT_VERSION || BOOT_VER == 0xffu) return 0;
  return mem_read(BOOT_JOURNAL + 12, (uint8_t *)&result, sizeof(result)) &&
    mem_read(BOOT_JOURNAL + BOOT_JOURNAL_REPORTED, (uint8_t *)&reported, sizeof(reported)) &&
    result != UINT32_MAX && reported == UINT32_MAX;
}

// Преобразовать код загрузчика, включая оборванную запись, в числовую причину события.
uint32_t app_boot_error_code(void)
{
  uint32_t result;
  if (!mem_read(BOOT_JOURNAL + 12, (uint8_t *)&result, sizeof(result))) return FW_REASON_UNKNOWN;
  if (result == BOOT_TRIAL_FAILED) return FW_REASON_TRIAL_FAIL;
  if (result == BOOT_BACKUP_ERROR) return FW_REASON_BACKUP;
  return result <= 9u ? FW_REASON_BOOT_BASE + result : FW_REASON_UNKNOWN;
}

// Отметить переданную ошибку; запись FF в ноль не требует стирания журнала.
int app_boot_error_reported(void)
{
  uint32_t reported = 0;
  flash_exec_args_t args = { NULL, FS_SYNC };
  if (!flash_exec_read_allowed() || !app_boot_error_unreported() || !flash_exec_begin(FX_STORE, &args)) return 0;
  bool ok = mem_write(BOOT_JOURNAL + BOOT_JOURNAL_REPORTED, (uint8_t *)&reported, sizeof(reported));
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  return ok;
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
 return hw==(uint8_t)DOMOVOI_APP_HW;
}
// Прочитать часть подготовленного образа из внешнего флеша.
int manual_stage_read(uint32_t off,void *data,uint32_t n) {
 if(off>MANUAL_BOOT_SIZE||n>MANUAL_BOOT_SIZE-off)return 0;
 if (!flash_exec_read_allowed()) return 0;
 return mem_read(BOOT_CANDIDATE+off,data,n);
}
// Записать часть подготовленного образа с проверкой драйвером W25.
int manual_stage_write(uint32_t off,const void *data,uint32_t n) {
 if(off>MANUAL_BOOT_SIZE||n>MANUAL_BOOT_SIZE-off)return 0;
 return mem_write(BOOT_CANDIDATE+off,(uint8_t*)data,n);
}
// Стереть сектор области подготовки образа.
int manual_stage_erase(uint32_t off) {
 if(off>=MANUAL_BOOT_SIZE||(off&4095u))return 0;
 return mem_erase_sector(BOOT_CANDIDATE+off);
}
// Обслужить сторожевой таймер во время ручного обновления.
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
// Проверить подготовленный образ и перенести его во внутренний флеш.
int manual_program(uint32_t expected_crc) {
 uint32_t page_crc[32],page,i,w,c,whole=0xffffffffu;uint8_t data[256];
 if(!manual_platform_supported())return 0;
 for(page=0;page<32;page++) {
  c=0xffffffffu;
  for(i=0;i<1024;i+=sizeof(data)) {
   unsigned j;if(!manual_stage_read(page*1024+i,data,sizeof(data)))goto failed;
   for(j=0;j<sizeof(data);j+=4){memcpy(&w,data+j,4);c=app_crc_word(c,w);whole=app_crc_word(whole,w);}
  }
  page_crc[page]=c;wdt_reload();
 }
 if(whole!=expected_crc)goto failed;
 replace_ram(page_crc);
failed:
 return 0;
}
