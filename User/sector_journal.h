// Доработка: Кряжев А., 2026
// Формат журнала и границы областей восстановления.
// SJ_DATA сохраняет образ до очистки метки SJ_META.
// Операции записи вызываются после стартового восстановления и проверяют данные в драйвере.
#ifndef SECTOR_JOURNAL_H
#define SECTOR_JOURNAL_H
#include <stdint.h>
#define SJ_DATA 0x1F9000u
#define SJ_META 0x1FA000u
#define SJ_SECTOR 4096u
#define SJ_PAGE 256u
#define SJ_MAGIC 0x314A4353u
#define SJ_VERSION 1u
#define SJ_COMMIT 0x314B4F53u
// Фазы фонового журнала; сравнение выполняется до первого стирания.
enum {
  SJ_PHASE_IDLE=0, SJ_PHASE_META_ERASE=1, SJ_PHASE_DATA_ERASE=2,
  SJ_PHASE_SOURCE=3, SJ_PHASE_SOURCE_NEXT=4, SJ_PHASE_HEADER=5,
  SJ_PHASE_HEADER_WRITE=6, SJ_PHASE_COMMIT=8, SJ_PHASE_TARGET_ERASE=10,
  SJ_PHASE_COPY=12, SJ_PHASE_COPY_NEXT=13, SJ_PHASE_CLEAN=15,
  SJ_PHASE_COMPARE=16, SJ_PHASE_DONE=30, SJ_PHASE_ERROR=31
};
// Проверяет адрес назначения из заголовка восстановления.
uint8_t sj_destination(uint32_t address);
// Ограничивает доступ загрузчика одним разрешённым сектором.
uint8_t sj_span(uint32_t address, uint16_t size);
// Завершает перенос после сброса при целом журнале.
uint8_t sj_recover(void);
typedef uint8_t (*sj_page_reader)(const void *context, uint16_t offset, uint8_t *page);
// Публикует резервный образ перед заменой назначения.
uint8_t sj_replace_pages(uint32_t address, sj_page_reader reader, const void *context);
// Читает через проверяемый низкий уровень владельца.
uint8_t sj_io_read(uint32_t address, uint8_t *data, uint16_t size);
// Пишет через низкий уровень с обязательным обратным чтением.
uint8_t sj_io_write(uint32_t address, const uint8_t *data, uint16_t size);
// Стирает сектор через низкий уровень с проверкой результата.
uint8_t sj_io_erase(uint32_t address);
// Обслуживает сторожевой таймер во время синхронного восстановления.
void sj_io_service(void);
// Продолжает CRC32 журнала и настроек; начальную и конечную инверсию делает владелец.
uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint16_t size);
#endif
