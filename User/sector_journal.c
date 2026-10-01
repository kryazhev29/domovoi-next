// Доработка: Кряжев А., 2026
#include "sector_journal.h"

uint8_t sj_destination(uint32_t address)
{
  if (address & (SJ_SECTOR - 1u)) return 0;
  return (address >= 0x41000u && address < 0x1C8000u) ||
         address == 0x1C9000u || address == 0x1CA000u || address == 0x1CC000u ||
         (address >= 0x1E8000u && address < 0x1F5000u);
}
uint8_t sj_span(uint32_t address, uint16_t size)
{
  uint32_t sector = address & ~(SJ_SECTOR - 1u);
  return size != 0 && size <= SJ_SECTOR - (address - sector) &&
         (sector == SJ_DATA || sector == SJ_META || sj_destination(sector));
}
static uint32_t sj_crc(uint32_t crc, const uint8_t *p, uint16_t size)
{
  uint8_t bit;
  while (size--) {
    crc ^= *p++;
    for (bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
  }
  return crc;
}
static uint8_t sj_flash_crc(uint32_t address, uint32_t *crc)
{
  uint8_t page[SJ_PAGE];
  uint16_t off;
  uint32_t c = 0xFFFFFFFFu;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_read(address + off, page, sizeof(page))) return 0;
    c = sj_crc(c, page, sizeof(page));
  }
  *crc = ~c;
  return 1;
}
static uint8_t sj_blank(uint32_t address)
{
  uint8_t page[SJ_PAGE];
  uint16_t off, i;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_read(address + off, page, sizeof(page))) return 0;
    for (i = 0; i < sizeof(page); i++) if (page[i] != 0xFFu) return 0;
  }
  return 1;
}
static uint8_t sj_image_equal(uint32_t address, const uint8_t *image, uint8_t *equal)
{
  uint8_t page[SJ_PAGE];
  uint16_t off, i;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_read(address + off, page, sizeof(page))) return 0;
    for (i = 0; i < sizeof(page); i++) if (page[i] != image[off + i]) { *equal = 0; return 1; }
  }
  *equal = 1;
  return 1;
}
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
static uint8_t sj_header(const uint32_t *h)
{
  return h[0] == SJ_MAGIC && h[1] == SJ_VERSION && sj_destination(h[2]) &&
         h[3] == SJ_SECTOR && h[5] == ~h[4] &&
         h[6] == ~sj_crc(0xFFFFFFFFu, (const uint8_t*)h, 24) && h[7] == SJ_COMMIT;
}
uint8_t sj_recover(void)
{
  uint32_t h[8], crc;
  uint8_t equal;
  uint8_t page[SJ_PAGE];
  uint16_t off;
  if (!sj_io_read(SJ_META, (uint8_t*)h, sizeof(h))) return 0;
  // Незавершённая запись или очистка metadata не разрешает стирание таблицы.
  if (!sj_header(h)) return 1;
  if (!sj_flash_crc(SJ_DATA, &crc) || crc != h[4]) return 0;
  if (!sj_flash_crc(h[2], &crc)) return 0;
  if (crc == h[4] && !sj_flash_equal(h[2], SJ_DATA, &equal)) return 0;
  if (crc != h[4] || !equal) {
    if (!sj_io_erase(h[2]) || !sj_blank(h[2])) return 0;
    for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
      sj_io_service();
      if (!sj_io_read(SJ_DATA + off, page, sizeof(page)) ||
          !sj_io_write(h[2] + off, page, sizeof(page))) return 0;
    }
  }
  if (!sj_flash_crc(h[2], &crc) || crc != h[4] ||
      !sj_flash_equal(h[2], SJ_DATA, &equal) || !equal) return 0;
  // При обрыве очистки destination уже целый; scratch до этого момента не менялся.
  return sj_io_erase(SJ_META) && sj_blank(SJ_META);
}
uint8_t sj_replace(uint32_t address, const uint8_t *image)
{
  uint32_t h[8], check[8], crc;
  uint8_t equal;
  uint16_t off, i;
  if (image == 0 || !sj_destination(address)) return 0;
  if (!sj_recover()) return 0;
  if (!sj_image_equal(address, image, &equal)) return 0;
  if (equal) return 1;
  // Старый commit должен быть полностью стёрт до повторного использования scratch.
  if (!sj_io_erase(SJ_META) || !sj_blank(SJ_META) ||
      !sj_io_erase(SJ_DATA) || !sj_blank(SJ_DATA)) return 0;
  for (off = 0; off < SJ_SECTOR; off += SJ_PAGE) {
    sj_io_service();
    if (!sj_io_write(SJ_DATA + off, image + off, SJ_PAGE)) return 0;
  }
  h[0] = SJ_MAGIC; h[1] = SJ_VERSION; h[2] = address; h[3] = SJ_SECTOR;
  h[4] = ~sj_crc(0xFFFFFFFFu, image, SJ_SECTOR); h[5] = ~h[4];
  h[6] = ~sj_crc(0xFFFFFFFFu, (const uint8_t*)h, 24); h[7] = SJ_COMMIT;
  if (!sj_flash_crc(SJ_DATA, &crc) || crc != h[4] ||
      !sj_image_equal(SJ_DATA, image, &equal) || !equal) return 0;
  if (!sj_io_write(SJ_META, (const uint8_t*)h, 28) ||
      !sj_io_read(SJ_META, (uint8_t*)check, sizeof(check))) return 0;
  for (i = 0; i < 7; i++) if (check[i] != h[i]) return 0;
  if (check[7] != 0xFFFFFFFFu) return 0;
  if (!sj_io_write(SJ_META + 28, (const uint8_t*)&h[7], 4) ||
      !sj_io_read(SJ_META, (uint8_t*)check, sizeof(check)) || !sj_header(check)) return 0;
  return sj_recover();
}
