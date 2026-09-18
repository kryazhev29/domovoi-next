// Доработка: Кряжев А., 2026
// Модуль работы с памятью
// Автор: Корякин Сергей

#include "memory.h"

/* Два слота по 8 КиБ: 4 КиБ логического образа и отдельный сектор фиксации. 0x1F5000..0x1F8FFF
 * - после TEMPID (конец 0x1F43FF), ниже PHOTO. Перед публикацией слота образ зеркалируется в
 * 0x1CD400, чтобы более старая прошивка могла откатиться. Выбор не кэшируется: после сброса или
 * неудачной записи всегда выбирается проверенный образ.
 */
volatile uint32_t mem_config_errors = 0;
static uint8_t cfg_legacy_checked = 0;
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
/* Здесь нет выделения 4 КиБ: вызывающий уже может занимать всю кучу, кроме 1 КиБ. */
static uint8_t cfg_equal_flash(uint32_t a, uint32_t b)
{
  uint8_t left[64], right[64];
  uint16_t off;
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(left)) {
    w25_read_data(a + off, left, sizeof(left));
    w25_read_data(b + off, right, sizeof(right));
    if (memcmp(left, right, sizeof(left))) return 0;
  }
  return 1;
}
static uint8_t cfg_legacy_crc(uint16_t offset)
{
  uint8_t b[64];
  uint16_t off, count, expected, crc = 0xFFFF;
  for (off = 0; off < 510; off += count) {
    count = 510 - off;
    if (count > sizeof(b)) count = sizeof(b);
    w25_read_data(MEM_CONFIG_LEGACY + offset + off, b, count);
    crc = crc16_calcInit(b, count, crc);
  }
  w25_read_data(MEM_CONFIG_LEGACY + offset + 510, (uint8_t*)&expected, 2);
  return crc == expected;
}
/* Источник - ОЗУ или образ во флеш-памяти. Прежний активный слот никогда не стирается. */
static uint8_t cfg_publish(uint8_t *p, uint32_t source, uint32_t active, uint32_t generation)
{
  uint32_t h[4], check;
  uint32_t target = active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  uint16_t off;
  uint8_t b[256];
  w25_erase_sector(target + MEMSECTORSIZE);
  w25_read_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[0] != 0xFFFFFFFFu || h[1] != 0xFFFFFFFFu ||
      h[2] != 0xFFFFFFFFu || h[3] != 0xFFFFFFFFu) return 0;
  w25_erase_sector(target);
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(b)) {
    if (p != NULL) memcpy(b, p + off, sizeof(b));
    else w25_read_data(source + off, b, sizeof(b));
    w25_write_data(target + off, b, sizeof(b));
  }
  if (p != NULL) { if (!cfg_equal(target, p)) return 0; }
  else if (!cfg_equal_flash(target, source)) return 0;
  h[0] = CFG_MAGIC;
  h[1] = generation + 1u;
  h[2] = p != NULL ? cfg_crc(cfg_crc(0xFFFFFFFFu, (uint8_t*)&h[1], 4), p, MEMSECTORSIZE)
                   : cfg_image_crc(source, h[1]);
  h[3] = ~h[2];
  w25_write_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  return cfg_valid(target, &check) && check == h[1];
}
/* При старте побеждают целые данные старого места: это может быть обновление старой прошивкой
 * или законченное зеркало, публикация слота которого прервалась. Резервные байты не трактуются:
 * старая запись сначала читает их, поэтому её нестёртый хвост совпадает сам с собой. Все 4096
 * байт сохраняются, включая резерв не из FF, без приведения.
 */
static uint8_t cfg_prepare(void)
{
  uint32_t generation, active;
  if (cfg_legacy_checked) return 1;
  active = cfg_select(&generation);
  if (cfg_legacy_crc(0) && (active == MEM_CONFIG_LEGACY || cfg_legacy_crc(512))) {
    if (active == MEM_CONFIG_LEGACY || !cfg_equal_flash(active, MEM_CONFIG_LEGACY)) {
      if (!cfg_publish(NULL, MEM_CONFIG_LEGACY, active, generation)) {
        mem_config_errors++;
        return 0;
      }
    }
  }
  cfg_legacy_checked = 1;
  return 1;
}
static uint8_t cfg_blank_sector(uint32_t address)
{
  uint8_t b[64];
  uint16_t off, i;
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(b)) {
    w25_read_data(address + off, b, sizeof(b));
    for (i = 0; i < sizeof(b); i++) if (b[i] != 0xFF) return 0;
  }
  return 1;
}
static uint8_t cfg_mirror(uint8_t *p)
{
  uint8_t b[64];
  uint16_t off;
  if (cfg_equal(MEM_CONFIG_LEGACY, p)) return 1;
  /* Настройки делаются недействительными до работы со вторым сектором и резервом. */
  w25_erase_sector(0x1CD000u);
  if (!cfg_blank_sector(0x1CD000u)) return 0;
  w25_erase_sector(0x1CE000u);
  if (!cfg_blank_sector(0x1CE000u)) return 0;
  w25_write_data(MEM_CONFIG_LEGACY + 1024, p + 1024, 3072);
  w25_write_data(MEM_CONFIG_LEGACY + 512, p + 512, 512);
  /* Действительные настройки никогда не публикуются поверх незаконченных device и резерва. */
  for (off = 512; off < MEMSECTORSIZE; off += sizeof(b)) {
    w25_read_data(MEM_CONFIG_LEGACY + off, b, sizeof(b));
    if (memcmp(b, p + off, sizeof(b))) return 0;
  }
  w25_write_data(MEM_CONFIG_LEGACY, p, 512);
  return cfg_equal(MEM_CONFIG_LEGACY, p);
}
static void cfg_commit(uint8_t *p)
{
  uint32_t generation, active;
  /* Сначала защитить образ, который есть только в старом месте, даже если первая операция -
   * запись. */
  if (!cfg_prepare()) return;
  active = cfg_select(&generation);
  if (!cfg_mirror(p)) { mem_config_errors++; return; }
  if (active != MEM_CONFIG_LEGACY && cfg_equal(active, p)) return;
  if (!cfg_publish(p, 0, active, generation)) mem_config_errors++;
}
static uint8_t cfg_range(uint32_t adr, uint16_t size)
{
  return adr >= MEM_CONFIG_LEGACY && adr < MEM_CONFIG_LEGACY + MEMSECTORSIZE &&
         size <= MEM_CONFIG_LEGACY + MEMSECTORSIZE - adr;
}
static void cfg_read(uint32_t adr, uint8_t *buff, uint16_t size)
{
  uint32_t generation, active;
  /* Неудачное принятие всё равно читает целый старый источник; повтор при следующем обращении. */
  if (cfg_prepare()) active = cfg_select(&generation);
  else active = MEM_CONFIG_LEGACY;
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