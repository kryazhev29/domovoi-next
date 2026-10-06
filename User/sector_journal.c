// Доработка: Кряжев А., 2026
// Общий журнал замены сектора для загрузчика и модели.
// Метка разрешает восстановление только при целом заголовке и CRC образа.
// Низкий уровень обязан проверять каждую запись и стирание.
#include "sector_journal.h"
#include <string.h>

// Проверяет адрес назначения из заголовка журнала после обрыва питания.
uint8_t sj_destination(uint32_t address)
{
  if (address & (SJ_SECTOR - 1u)) return 0;
  return (address >= 0x41000u && address < 0x1C8000u) ||
         address == 0x1C9000u || address == 0x1CA000u || address == 0x1CC000u ||
         (address >= 0x1D0000u && address < 0x1F5000u);
}
// Ограничивает низкоуровневый доступ областью одного разрешённого сектора.
uint8_t sj_span(uint32_t address, uint16_t size)
{
  uint32_t sector = address & ~(SJ_SECTOR - 1u);
  return size != 0 && size <= SJ_SECTOR - (address - sector) &&
         (sector == SJ_DATA || sector == SJ_META || sj_destination(sector));
}
// CRC32 с полиномом 0xEDB88320 без конечной инверсии.
// Загрузчик и модель используют C, приложение — измеренное ядро ASM_CRC.
#ifndef ASM_CRC
// Продолжает CRC32 без начальной и конечной инверсии.
uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint16_t size)
{
  uint8_t bit;
  while (size--) {
    crc ^= *p++;
    for (bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
  }
  return crc;
}
#endif
// Проверяет CRC резервного образа при восстановлении после сброса.
static uint8_t sj_flash_crc(uint32_t address, uint32_t *crc)
{
  uint8_t page[SJ_PAGE];
  uint16_t off;
  uint32_t c = 0xFFFFFFFFu;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_read(address + off, page, sizeof(page))) return 0;
    c = crc32_update(c, page, sizeof(page));
  }
  *crc = ~c;
  return 1;
}
// Проверка страницы не требует второй страницы в ОЗУ.
static uint8_t sj_page_equal(uint32_t address, const uint8_t *page, uint8_t *equal)
{
  uint8_t check[32];
  *equal = 1;
  for (uint16_t off = 0; off < SJ_PAGE; off += sizeof(check)) {
    if (!sj_io_read(address + off, check, sizeof(check))) return 0;
    if (memcmp(check, page + off, sizeof(check))) *equal = 0;
  }
  return 1;
}

// Выявляет неизменный сектор, чтобы сохранить ресурс флеш-памяти.
static uint8_t sj_pages_equal(uint32_t address, sj_page_reader reader, const void *context, uint8_t *equal)
{
  uint8_t page[SJ_PAGE], same;
  *equal = 1;
  for (uint16_t off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!reader(context, off, page) || !sj_page_equal(address + off, page, &same)) return 0;
    if (!same) *equal = 0;
  }
  return 1;
}
// Сравнивает назначение с проверенным резервным образом при восстановлении.
static uint8_t sj_flash_equal(uint32_t a, uint32_t b, uint8_t *equal)
{
  uint8_t left[32], right[32];
  uint16_t off, i;
  for (off = 0; off < SJ_SECTOR; off += sizeof(left)) {
    sj_io_service();
    if (!sj_io_read(a + off, left, sizeof(left)) ||
        !sj_io_read(b + off, right, sizeof(right))) return 0;
    for (i = 0; i < sizeof(left); i++) if (left[i] != right[i]) { *equal = 0; return 1; }
  }
  *equal = 1;
  return 1;
}
// Проверяет формат, адрес, CRC заголовка и метку фиксации.
static uint8_t sj_header(const uint32_t *h)
{
  return h[0] == SJ_MAGIC && h[1] == SJ_VERSION && sj_destination(h[2]) &&
         h[3] == SJ_SECTOR && h[5] == ~h[4] &&
         h[6] == ~crc32_update(0xFFFFFFFFu, (const uint8_t*)h, 24) && h[7] == SJ_COMMIT;
}
// Переносит проверенный резервный образ и очищает метку только после последней записи.
static uint8_t sj_copy_data(uint32_t address)
{
  uint8_t page[SJ_PAGE];
  if (!sj_io_erase(address)) return 0;
  for (uint16_t off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_read(SJ_DATA + off, page, sizeof(page)) ||
        !sj_io_write(address + off, page, sizeof(page))) return 0;
  }
  return sj_io_erase(SJ_META);
}
// Завершает зафиксированный перенос после обрыва и очищает метку.
uint8_t sj_recover(void)
{
  uint32_t h[8], crc;
  uint8_t equal;
  if (!sj_io_read(SJ_META, (uint8_t*)h, sizeof(h))) return 0;
  // Незавершённый заголовок не разрешает стирание назначения.
  if (!sj_header(h)) return 1;
  if (!sj_flash_crc(SJ_DATA, &crc) || crc != h[4]) return 0;
  // Сравнение всех байтов с уже проверенным образом заменяет второй CRC назначения.
  if (!sj_flash_equal(h[2], SJ_DATA, &equal)) return 0;
  return equal ? sj_io_erase(SJ_META) : sj_copy_data(h[2]);
}
// Заменяет сектор после стартового восстановления; W25 проверяет каждую запись.
// Метка журнала ставится последней, а стирается после переноса проверенных данных.
uint8_t sj_replace_pages(uint32_t address, sj_page_reader reader, const void *context)
{
  uint32_t h[8], expected = 0xFFFFFFFFu;
  uint8_t equal, page[SJ_PAGE];
  uint16_t off;
  if (!sj_pages_equal(address, reader, context, &equal)) return 0;
  if (equal) return 1;
  if (!sj_io_erase(SJ_META) || !sj_io_erase(SJ_DATA)) return 0;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!reader(context, off, page)) return 0;
    expected = crc32_update(expected, page, sizeof(page));
    if (!sj_io_write(SJ_DATA + off, page, sizeof(page))) return 0;
  }
  h[0] = SJ_MAGIC; h[1] = SJ_VERSION; h[2] = address; h[3] = SJ_SECTOR;
  h[4] = ~expected; h[5] = ~h[4];
  h[6] = ~crc32_update(0xFFFFFFFFu, (const uint8_t*)h, 24); h[7] = SJ_COMMIT;
  if (!sj_io_write(SJ_META, (const uint8_t*)h, 28) ||
      !sj_io_write(SJ_META + 28, (const uint8_t*)&h[7], 4)) return 0;
  return sj_copy_data(address);
}
