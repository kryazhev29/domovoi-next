// Доработка: Кряжев А., 2026
// Общий журнал атомарной замены одного сектора таблиц.
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
uint8_t sj_destination(uint32_t address);
uint8_t sj_span(uint32_t address, uint16_t size);
uint8_t sj_recover(void);
uint8_t sj_replace(uint32_t address, const uint8_t *image);
uint8_t sj_io_read(uint32_t address, uint8_t *data, uint16_t size);
uint8_t sj_io_write(uint32_t address, const uint8_t *data, uint16_t size);
uint8_t sj_io_erase(uint32_t address);
void sj_io_service(void);
#endif
