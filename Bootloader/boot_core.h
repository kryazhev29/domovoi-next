/* Разработчик: Кряжев А., 2026 */
#ifndef DOMOVOI_BOOT_CORE_H
#define DOMOVOI_BOOT_CORE_H
#include <stdint.h>
#include <stddef.h>
#define BOOT_APP_BASE 0x08008000u
#define BOOT_APP_VECTOR 0x08008200u
/* Заводская запись объёма флеш-памяти STM32F1, 16 бит, в КиБ (RM0008). */
#define BOOT_FLASH_SIZE_ADDRESS 0x1ffff7e0u
#define BOOT_REQUIRED_FLASH_KIB 128u
#define BOOT_FLASH_END 0x08020000u
#define BOOT_HEADER_SIZE 512u
#define BOOT_MAX_CODE (BOOT_FLASH_END-BOOT_APP_VECTOR)
#define BOOT_CANDIDATE 0u
#define BOOT_JOURNAL 0x20000u
/* Слова журнала: +0 начало, +4 готово, +8 откат, +12 ошибка, +16 очистка. +20 пишет только
   прошивка, после отправки ошибки на сервер. */
#define BOOT_JOURNAL_REPORTED 20u
/* Проба новой установленной прошивки. +24 взведена (загрузчик, до работы с внутренней
   флеш-памятью), +28 подтверждена (прошивка, считается только точный 0), +32 маска запусков
   (загрузчик гасит по биту на запуск), +36 запрошен откат (прошивка, считается любой погашенный
   бит). Конец неудачной пробы - BOOT_TRIAL_FAILED в +12. */
#define BOOT_JOURNAL_TRIAL 24u
#define BOOT_JOURNAL_CONFIRMED 28u
#define BOOT_JOURNAL_LAUNCHES 32u
#define BOOT_JOURNAL_ROLLBACK 36u
#define BOOT_TRIAL_LAUNCHES 3u
/* Зарезервированный вектор 7 прошивки, которая умеет подтверждать себя (Cortex-M3). */
#define BOOT_TRIAL_VECTOR 0x1cu
#define BOOT_TRIAL_MARKER 0x4c495254u
#define BOOT_BACKUP 0x21000u
#define BOOT_SLOT_SIZE 0x20000u
#define BOOT_MAGIC 0x4E465744u
#define BOOT_LEGACY_MAGIC 0x57464C53u
#ifndef BOOT_HW_VERSION
#define BOOT_HW_VERSION 0x8bu
#endif
#define BOOT_VERSION 0x32u
/* Раскладка совпадает с bootFWparam_s, включая невыровненный uint16_t. */
#pragma pack(push,1)
typedef struct {
 uint32_t magic;
 uint8_t hardware;
 uint16_t version;
 uint8_t reserved;
 uint32_t size, crc, reserved2, timestamp;
 uint8_t reserved3[8];
} BootHeader;
#pragma pack(pop)
enum { BOOT_OK=0, BOOT_BAD_IMAGE=1, BOOT_IO=2, BOOT_BACKUP_ERROR=3,
       BOOT_PROGRAM_ERROR=4, BOOT_RESTORED=5, BOOT_RECOVERY=6, BOOT_UNSUPPORTED_FLASH=7,
       BOOT_TRIAL_FAILED=8, BOOT_TRIAL_BUSY=9 };
/* Слой платы возвращает 1 при успехе. Каждую запись проверяет ядро или слой платы. */
int boot_read(int external,uint32_t address,void *data,uint32_t size);
int boot_ext_erase(uint32_t address);
int boot_ext_write(uint32_t address,const void *data,uint32_t size);
int boot_int_erase(uint32_t address);
int boot_int_write(uint32_t address,const void *data,uint32_t size);
void boot_service(void);
uint32_t boot_crc_word(uint32_t crc,uint32_t word);
int boot_validate(int external,uint32_t base,BootHeader *header);
int boot_run(void);
int boot_prepare(void);
int boot_stage_header(const BootHeader *header);
int boot_request(void);
int boot_parse_filename(const char *name,BootHeader *header);
#endif
