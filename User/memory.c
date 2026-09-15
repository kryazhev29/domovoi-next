// Модуль работы с памятью
// Автор: Корякин Сергей

#include "memory.h"

/* Two 8 KiB slots: 4 KiB logical image + a separate commit sector.
 * 0x1F5000..0x1F8FFF is after TEMPID (ends 0x1F43FF), below PHOTO.
 * Keep the unaligned legacy image at 0x1CD400 untouched for migration.
 * No cached selection: a reset or failed write always selects a verified image.
 */
volatile uint32_t mem_config_errors = 0;
#define CFG_MAGIC 0x32474643u
static uint32_t cfg_crc(uint32_t c, const uint8_t *p, uint16_t n)
{
  uint8_t bit;
  while (n--) {
    c ^= *p++;
    for (bit = 0; bit < 8; bit++) c = (c >> 1) ^ ((c & 1u) ? 0xEDB88320u : 0u);
  }
  return c;
}
static uint32_t cfg_image_crc(uint32_t address, uint32_t generation)
{
  uint8_t b[64];
  uint16_t off;
  uint32_t c = cfg_crc(0xFFFFFFFFu, (uint8_t*)&generation, 4);
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(b)) {
    w25_read_data(address + off, b, sizeof(b));
    c = cfg_crc(c, b, sizeof(b));
  }
  return c;
}
static uint8_t cfg_valid(uint32_t address, uint32_t *generation)
{
  uint32_t h[4];
  w25_read_data(address + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[0] != CFG_MAGIC || h[3] != ~h[2]) return 0;
  if (cfg_image_crc(address, h[1]) != h[2]) return 0;
  *generation = h[1];
  return 1;
}
static uint32_t cfg_select(uint32_t *generation)
{
  uint32_t a = 0, b = 0;
  uint8_t va = cfg_valid(MEM_CONFIG_SLOT0, &a);
  uint8_t vb = cfg_valid(MEM_CONFIG_SLOT1, &b);
  if (vb && (!va || (int32_t)(b - a) > 0)) {
    *generation = b; return MEM_CONFIG_SLOT1;
  }
  if (va) { *generation = a; return MEM_CONFIG_SLOT0; }
  *generation = 0; return MEM_CONFIG_LEGACY;
}
static uint8_t cfg_equal(uint32_t address, const uint8_t *p)
{
  uint8_t b[64];
  uint16_t off;
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(b)) {
    w25_read_data(address + off, b, sizeof(b));
    if (memcmp(b, p + off, sizeof(b))) return 0;
  }
  return 1;
}
static void cfg_commit(uint8_t *p)
{
  uint32_t generation, active = cfg_select(&generation), h[4], check;
  uint32_t target = active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  if (active != MEM_CONFIG_LEGACY && cfg_equal(active, p)) return;
  /* Invalidate the old commit before changing its data. Active is untouched. */
  w25_erase_sector(target + MEMSECTORSIZE);
  w25_read_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[0] != 0xFFFFFFFFu || h[1] != 0xFFFFFFFFu ||
      h[2] != 0xFFFFFFFFu || h[3] != 0xFFFFFFFFu) { mem_config_errors++; return; }
  w25_erase_sector(target);
  w25_write_data(target, p, MEMSECTORSIZE);
  if (!cfg_equal(target, p)) { mem_config_errors++; return; }
  h[0] = CFG_MAGIC;
  h[1] = generation + 1u;
  h[2] = cfg_crc(cfg_crc(0xFFFFFFFFu, (uint8_t*)&h[1], 4), p, MEMSECTORSIZE);
  h[3] = ~h[2];
  /* Publication is last, and both generation and data are covered by CRC. */
  w25_write_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (!cfg_valid(target, &check) || check != h[1]) mem_config_errors++;
}
static uint8_t cfg_range(uint32_t adr, uint16_t size)
{
  return adr >= MEM_CONFIG_LEGACY && adr < MEM_CONFIG_LEGACY + MEMSECTORSIZE &&
         size <= MEM_CONFIG_LEGACY + MEMSECTORSIZE - adr;
}
static void cfg_read(uint32_t adr, uint8_t *buff, uint16_t size)
{
  uint32_t generation, active = cfg_select(&generation);
  if (active == MEM_CONFIG_LEGACY) {
    uint8_t *image = malloc(MEMSECTORSIZE);
    if (image != NULL) {
      uint16_t crc;
      w25_read_data(active, image, MEMSECTORSIZE);
      memcpy(&crc, image + 510, 2);
      /* Migrate only an intact legacy config; preserve device and spare bytes. */
      if (crc == crc16_calc(image, 510)) cfg_commit(image);
      free(image);
      active = cfg_select(&generation);
    }
  }
  w25_read_data(active + adr - MEM_CONFIG_LEGACY, buff, size);
}



// Чтение данных с памяти
void mem_read (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();  
  if (cfg_range(adr, size)) cfg_read(adr, buff, size);
  else w25_read_data (adr, buff, size);
}

// Запись данных в память
// данные пишутся в предварительно стертый сектор
void mem_write (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();  
  if (cfg_range(adr, size)) {
    if (adr == MEM_CONFIG_LEGACY && size == MEMSECTORSIZE) cfg_commit(buff);
    else {
      uint8_t *image = malloc(MEMSECTORSIZE);
      if (image == NULL) { mem_config_errors++; return; }
      cfg_read(MEM_CONFIG_LEGACY, image, MEMSECTORSIZE);
      memcpy(image + adr - MEM_CONFIG_LEGACY, buff, size);
      cfg_commit(image);
      free(image);
    }
  } else w25_write_data (adr, buff, size);
}

// Стереть сектор
void mem_erase_sector (Int32U adr)
{
  mem_check_power();
  w25_erase_sector(adr);
}

// Запись данных в память с предварительным чтением сектора
void mem_rewrite (Int32U adr, uint8_t* buff, Int16U size, uint8_t* mem_buffer4kB)
{
  Int32U sector;

  if (mem_buffer4kB == NULL) return;
  if (buff == NULL) return;

  mem_check_power();

  if (cfg_range(adr, size)) {
    cfg_read(MEM_CONFIG_LEGACY, mem_buffer4kB, MEMSECTORSIZE);
    memcpy(mem_buffer4kB + adr - MEM_CONFIG_LEGACY, buff, size);
    cfg_commit(mem_buffer4kB);
    return;
  }
  sector = div(adr, MEMSECTORSIZE).quot * MEMSECTORSIZE;
  
  if ((adr - sector + size) > MEMSECTORSIZE) return;

  w25_read_data (sector, mem_buffer4kB, MEMSECTORSIZE);
  
  if (!memcmp(&mem_buffer4kB[adr - sector], buff, size)) return;
  w25_erase_sector (sector);
  
  memcpy(&mem_buffer4kB[adr - sector], buff, size);
  
  w25_write_data (sector, mem_buffer4kB, MEMSECTORSIZE);
}

// стереть диапазон памяти
void mem_erase_mem (Int32U fadr, Int32U eadr)
{
  mem_check_power();

  fadr = div(fadr, MEMSECTORSIZE).quot * MEMSECTORSIZE;
  for (;fadr <= eadr; fadr += MEMSECTORSIZE)
  {
    wdt_reload();
    w25_erase_sector(fadr);
  }
}