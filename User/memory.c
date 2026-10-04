// Доработка: Кряжев А., 2026
// Модуль работы с памятью
// Автор: Корякин Сергей

#include "memory.h"
#include "sector_journal.h"

static bool mem_storage_ready = false;
uint8_t sj_io_read(uint32_t adr, uint8_t *p, uint16_t n) { return sj_span(adr, n) && w25_read_data(adr, p, n); }
uint8_t sj_io_write(uint32_t adr, const uint8_t *p, uint16_t n) { return sj_span(adr, n) && w25_write_data(adr, (uint8_t*)p, n); }
uint8_t sj_io_erase(uint32_t adr) { return !(adr & 4095u) && sj_span(adr, 4096) && w25_erase_sector(adr); }
void sj_io_service(void) { wdt_reload(); }
bool mem_recover(void) { mem_check_power(); mem_storage_ready = sj_recover(); return mem_storage_ready; }
bool mem_tables_ready(void) { return mem_storage_ready || mem_recover(); }
static bool mem_guard(uint32_t adr, uint16_t n)
{
  uint32_t sector, last;
  if (n > 0xFFFFFFFFu - adr) return false;
  if (n == 0) return true;
  last = (adr + n - 1u) & ~4095u;
  for (sector = adr & ~4095u; ; sector += MEMSECTORSIZE) {
    if (sj_destination(sector)) return mem_tables_ready();
    if (sector == last) return true;
  }
}
static bool mem_reserved(uint32_t adr, uint16_t n)
{
  return n && (n > 0xFFFFFFFFu - adr || (adr < SJ_META + SJ_SECTOR && adr + n > SJ_DATA));
}
bool mem_replace_sector(uint32_t adr, const uint8_t *image)
{
  if (!sj_destination(adr) || image == NULL) return false;
  mem_check_power();
  mem_storage_ready = sj_replace(adr, image);
  return mem_storage_ready;
}


bool mem_replace_pages(uint32_t adr, mem_page_reader reader, const void *context)
{
  if (!sj_destination(adr) || reader == NULL) return false;
  mem_check_power();
  mem_storage_ready = sj_replace_pages(adr, reader, context);
  return mem_storage_ready;
}

// Две смежные правки: config/device не требуют общей копии сектора.
typedef struct {
  uint32_t source, offset;
  const uint8_t *first, *second;
  uint16_t nfirst, nsecond;
} mem_image_s;

static void mem_overlay(uint8_t *page, uint16_t off, uint32_t start, const uint8_t *data, uint16_t size)
{
  uint32_t from = off > start ? off : start;
  uint32_t end = (uint32_t)off + SJ_PAGE;
  if (end > start + size) end = start + size;
  if (from < end) memcpy(page + from - off, data + from - start, end - from);
}

static uint8_t mem_image_page(const void *context, uint16_t off, uint8_t *page)
{
  const mem_image_s *image = context;
  if (!w25_read_data(image->source + off, page, SJ_PAGE)) return 0;
  mem_overlay(page, off, image->offset, image->first, image->nfirst);
  mem_overlay(page, off, image->offset + image->nfirst, image->second, image->nsecond);
  return 1;
}

/* Два слота по 8 КиБ: 4 КиБ логического образа и отдельный сектор фиксации. 0x1F5000..0x1F8FFF
 * - после TEMPID (конец 0x1F43FF), ниже PHOTO. Перед публикацией слота образ зеркалируется в
 * 0x1CD400, чтобы более старая прошивка могла откатиться. Выбор не кэшируется: после сброса или
 * неудачной записи всегда выбирается проверенный образ.
 */
volatile uint32_t mem_config_errors = 0;
static uint8_t cfg_legacy_checked = 0;
static uint32_t cfg_cached_active = MEM_CONFIG_LEGACY, cfg_cached_generation;
#define CFG_MAGIC 0x32474643u
static bool cfg_image_crc(uint32_t address, uint32_t generation, uint32_t *result)
{
  uint8_t b[64];
  uint16_t off;
  uint32_t c = crc32_update(0xFFFFFFFFu, (uint8_t*)&generation, 4);
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(b)) {
    if (!w25_read_data(address + off, b, sizeof(b))) return 0;
    c = crc32_update(c, b, sizeof(b));
  }
  *result = c;
  return true;
}
static uint8_t cfg_valid(uint32_t address, uint32_t *generation)
{
  uint32_t h[4], crc;
  if (!w25_read_data(address + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  if (h[0] != CFG_MAGIC || h[3] != ~h[2]) return 0;
  if (!cfg_image_crc(address, h[1], &crc) || crc != h[2]) return 0;
  *generation = h[1];
  return 1;
}
static uint32_t cfg_select(uint32_t *generation)
{
  uint32_t a = 0, b = 0;
  uint8_t va = cfg_valid(MEM_CONFIG_SLOT0, &a);
  uint8_t vb = cfg_valid(MEM_CONFIG_SLOT1, &b);
  if (vb && (!va || (int32_t)(b - a) > 0)) {
    *generation = cfg_cached_generation = b; cfg_cached_active = MEM_CONFIG_SLOT1; return MEM_CONFIG_SLOT1;
  }
  if (va) { *generation = cfg_cached_generation = a; cfg_cached_active = MEM_CONFIG_SLOT0; return MEM_CONFIG_SLOT0; }
  *generation = cfg_cached_generation = 0; cfg_cached_active = MEM_CONFIG_LEGACY; return MEM_CONFIG_LEGACY;
}
static uint8_t cfg_pages_equal(uint32_t address, const mem_image_s *image)
{
  uint8_t page[SJ_PAGE], check[32];
  for (uint16_t off = 0; off < MEMSECTORSIZE; off += SJ_PAGE) {
    wdt_reload();
    if (!mem_image_page(image, off, page)) return 0;
    for (uint16_t i = 0; i < SJ_PAGE; i += sizeof(check)) {
      if (!w25_read_data(address + off + i, check, sizeof(check)) ||
          memcmp(page + i, check, sizeof(check))) return 0;
    }
  }
  return 1;
}
/* Здесь нет выделения 4 КиБ: вызывающий уже может занимать всю кучу, кроме 1 КиБ. */
static uint8_t cfg_equal_flash(uint32_t a, uint32_t b)
{
  uint8_t left[64], right[64];
  uint16_t off;
  for (off = 0; off < MEMSECTORSIZE; off += sizeof(left)) {
    if (!w25_read_data(a + off, left, sizeof(left))) return 0;
    if (!w25_read_data(b + off, right, sizeof(right))) return 0;
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
    if (!w25_read_data(MEM_CONFIG_LEGACY + offset + off, b, count)) return 0;
    crc = crc16_calcInit(b, count, crc);
  }
  if (!w25_read_data(MEM_CONFIG_LEGACY + offset + 510, (uint8_t*)&expected, 2)) return 0;
  return crc == expected;
}
/* Источник - ОЗУ или образ во флеш-памяти. Прежний активный слот никогда не стирается. */
static uint8_t cfg_publish_pages(const mem_image_s *image, uint32_t active, uint32_t generation)
{
  uint32_t h[4], check;
  uint32_t target = active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  uint8_t page[SJ_PAGE];
  if (!w25_erase_sector(target + MEMSECTORSIZE)) return 0;
  if (!w25_read_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  if (h[0] != 0xFFFFFFFFu || h[1] != 0xFFFFFFFFu || h[2] != 0xFFFFFFFFu || h[3] != 0xFFFFFFFFu) return 0;
  if (!w25_erase_sector(target)) return 0;
  h[0] = CFG_MAGIC; h[1] = generation + 1u;
  uint32_t crc = crc32_update(0xFFFFFFFFu, (uint8_t*)&h[1], 4);
  for (uint16_t off = 0; off < MEMSECTORSIZE; off += SJ_PAGE) {
    wdt_reload();
    if (!mem_image_page(image, off, page)) return 0;
    crc = crc32_update(crc, page, sizeof(page));
    if (!w25_write_data(target + off, page, sizeof(page))) return 0;
  }
  if (!cfg_pages_equal(target, image)) return 0;
  h[2] = crc; h[3] = ~h[2];
  if (!w25_write_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  if (!cfg_valid(target, &check) || check != h[1]) return 0;
  cfg_cached_active = target; cfg_cached_generation = check;
  return 1;
}

static uint8_t cfg_publish(uint8_t *p, uint32_t source, uint32_t active, uint32_t generation)
{
  mem_image_s image = {source, 0, p, NULL, p == NULL ? 0 : MEMSECTORSIZE, 0};
  return cfg_publish_pages(&image, active, generation);
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
    if (!w25_read_data(address + off, b, sizeof(b))) return 0;
    for (i = 0; i < sizeof(b); i++) if (b[i] != 0xFF) return 0;
  }
  return 1;
}
static uint8_t cfg_mirror(const mem_image_s *image)
{
  uint8_t page[SJ_PAGE], check[32];
  if (cfg_pages_equal(MEM_CONFIG_LEGACY, image)) return 1;
  if (!w25_erase_sector(0x1CD000u) || !cfg_blank_sector(0x1CD000u) ||
      !w25_erase_sector(0x1CE000u) || !cfg_blank_sector(0x1CE000u)) return 0;
  // Хвост, device, затем config с его CRC: порядок для загрузчика 0x32 сохранён.
  for (uint16_t off = 1024; off < MEMSECTORSIZE; off += SJ_PAGE) {
    wdt_reload();
    if (!mem_image_page(image, off, page) || !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page))) return 0;
  }
  for (uint16_t off = 512; off < 1024; off += SJ_PAGE) {
    if (!mem_image_page(image, off, page) || !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page))) return 0;
  }
  for (uint16_t off = 512; off < MEMSECTORSIZE; off += SJ_PAGE) {
    if (!mem_image_page(image, off, page)) return 0;
    for (uint16_t i = 0; i < SJ_PAGE; i += sizeof(check)) {
      if (!w25_read_data(MEM_CONFIG_LEGACY + off + i, check, sizeof(check)) || memcmp(page + i, check, sizeof(check))) return 0;
    }
  }
  for (uint16_t off = 0; off < 512; off += SJ_PAGE) {
    if (!mem_image_page(image, off, page) || !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page))) return 0;
  }
  return cfg_pages_equal(MEM_CONFIG_LEGACY, image);
}
static bool cfg_commit_parts(uint16_t offset, const uint8_t *first, uint16_t nfirst, const uint8_t *second, uint16_t nsecond)
{
  uint32_t generation, active;
  if (!cfg_prepare()) return false;
  active = cfg_select(&generation);
  mem_image_s image = {active, offset, first, second, nfirst, nsecond};
  if (active == MEM_CONFIG_LEGACY) {
    // При первом сохранении источник ещё в legacy: сначала переносим его в банк.
    if (!cfg_publish_pages(&image, active, generation)) { mem_config_errors++; return false; }
    image.source = MEM_CONFIG_SLOT0;
    if (!cfg_mirror(&image)) { mem_config_errors++; return false; }
    return true;
  }
  if (!cfg_mirror(&image)) { mem_config_errors++; return false; }
  if (cfg_pages_equal(active, &image)) return true;
  if (!cfg_publish_pages(&image, active, generation)) { mem_config_errors++; return false; }
  return true;
}
static uint8_t cfg_range(uint32_t adr, uint16_t size)
{
  return adr >= MEM_CONFIG_LEGACY && adr < MEM_CONFIG_LEGACY + MEMSECTORSIZE &&
         size <= MEM_CONFIG_LEGACY + MEMSECTORSIZE - adr;
}
static bool cfg_read(uint32_t adr, uint8_t *buff, uint16_t size)
{
  uint32_t generation, active;
  /* Неудачное принятие всё равно читает целый старый источник; повтор при следующем обращении. */
  if (mem_config_result() == 1) active = cfg_cached_active;
  else if (cfg_prepare()) active = cfg_select(&generation);
  else active = MEM_CONFIG_LEGACY;
  return w25_read_data(active + adr - MEM_CONFIG_LEGACY, buff, size);
}

// Чтение данных с памяти
bool mem_read (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();  
  if (cfg_range(adr, size)) return cfg_read(adr, buff, size);
  return mem_guard(adr, size) && w25_read_data(adr, buff, size);
}

// Запись данных в память
// данные пишутся в предварительно стертый сектор
bool mem_write (Int32U adr, pInt8U buff, Int16U size)
{
  mem_check_power();
  if (cfg_range(adr, size)) return mem_patch(adr, buff, size);
  return !mem_reserved(adr, size) && mem_guard(adr, size) && w25_write_data(adr, buff, size);
}

bool mem_patch2(Int32U adr, const uint8_t *first, Int16U nfirst, const uint8_t *second, Int16U nsecond)
{
  uint32_t size = (uint32_t)nfirst + nsecond;
  if ((nfirst && first == NULL) || (nsecond && second == NULL) || size > MEMSECTORSIZE) return false;
  mem_check_power();
  if (cfg_range(adr, (uint16_t)size)) return cfg_commit_parts((uint16_t)(adr - MEM_CONFIG_LEGACY), first, nfirst, second, nsecond);
  uint32_t sector = adr & ~4095u;
  if (size > MEMSECTORSIZE - (adr - sector) || !sj_destination(sector) || !mem_tables_ready()) return false;
  mem_image_s image = {sector, adr - sector, first, second, nfirst, nsecond};
  return mem_replace_pages(sector, mem_image_page, &image);
}

bool mem_patch(Int32U adr, const uint8_t *data, Int16U size)
{
  return mem_patch2(adr, data, size, NULL, 0);
}

bool mem_erase_sector (Int32U adr)
{
  mem_check_power();
  adr &= ~4095u;
  return !mem_reserved(adr, MEMSECTORSIZE) && mem_guard(adr, MEMSECTORSIZE) && w25_erase_sector(adr);
}

// Запись данных в память с предварительным чтением сектора
bool mem_rewrite(Int32U adr, uint8_t *buff, Int16U size, uint8_t *mem_buffer4kB)
{
  (void)mem_buffer4kB; // совместимый вход для старых тестов, рабочий код сектор не выделяет
  return mem_patch(adr, buff, size);
}

// стереть диапазон памяти
bool mem_erase_mem (Int32U fadr, Int32U eadr)
{
  mem_check_power();

  fadr = div(fadr, MEMSECTORSIZE).quot * MEMSECTORSIZE;
  for (;fadr <= eadr; fadr += MEMSECTORSIZE)
  {
    wdt_reload();
    if (!mem_erase_sector(fadr)) return false;
  }
  return true;
}
/* >>> Cooperative config publication. Each poll programs/verifies at most 64 bytes (comparison reads <=192),
 * or starts one erase, or samples NOR status (at most twice with readback).
 * The legacy mirror is published config-last, before replacing a valid A/B slot.
 * Startup migration and legacy compatibility entry points remain synchronous.
 */
static struct {
  uint8_t *snapshot;
  uint8_t page[64];
  uint32_t source, target, header[4], crc;
  uint16_t offset;
  uint8_t state, result, waiting, changed, mirror_changed, first;
} cfg_job;
enum { CA_CHECK=1, CA_MIRROR0, CA_MIRROR1, CA_TAIL, CA_DEVICE, CA_CONFIG,
       CA_META, CA_DATA, CA_COPY, CA_CRC, CA_HEADER, CA_DONE };
uint8_t mem_config_result(void) { return cfg_job.result; }
const uint8_t *mem_config_snapshot(void) { return cfg_job.snapshot; }
void mem_config_reset(void) {
  if (cfg_job.result == 1) return; /* A live NOR verification still owns the snapshot. */
  free(cfg_job.snapshot); cfg_job.snapshot = NULL; cfg_job.result = 0; cfg_job.waiting = 0;
}
bool mem_config_begin(const uint8_t *first, const uint8_t *second)
{
  if (cfg_job.result || w25_async_active() || !cfg_legacy_checked || !first || !second) return false;
  cfg_job.snapshot = malloc(1024);
  if (!cfg_job.snapshot) return false;
  memcpy(cfg_job.snapshot, first, 512); memcpy(cfg_job.snapshot + 512, second, 512);
  cfg_job.source = cfg_cached_active;
  cfg_job.target = cfg_cached_active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  cfg_job.header[0] = CFG_MAGIC; cfg_job.header[1] = cfg_cached_generation + 1u;
  cfg_job.offset = 0; cfg_job.changed = cfg_job.mirror_changed = 0;
  cfg_job.first = cfg_cached_active == MEM_CONFIG_LEGACY;
  cfg_job.waiting = 0; cfg_job.state = CA_CHECK; cfg_job.result = 1;
  return true;
}
static bool cfg_async_page(void)
{
  if (cfg_job.offset < 1024u) {
    memcpy(cfg_job.page, cfg_job.snapshot + cfg_job.offset, sizeof(cfg_job.page)); return true;
  }
  return w25_read_data(cfg_job.source + cfg_job.offset, cfg_job.page, sizeof(cfg_job.page));
}
void mem_config_poll(uint32_t now)
{
  uint8_t b[64], r;
  uint32_t address;
  if (cfg_job.result != 1) return;
  if (cfg_job.waiting) {
    r = w25_async_poll(now);
    if (r == 1) return;
    cfg_job.waiting = 0;
    if (r != 2) goto fail;
    if (cfg_job.first && cfg_job.state == CA_MIRROR0) {
      cfg_job.source = cfg_job.target;
      cfg_cached_active = cfg_job.target; cfg_cached_generation = cfg_job.header[1];
    }
    return;
  }
  switch (cfg_job.state) {
  case CA_CHECK:
    if (!cfg_async_page() || !w25_read_data(cfg_job.source + cfg_job.offset, b, sizeof(b))) goto fail;
    if (memcmp(b, cfg_job.page, sizeof(b))) cfg_job.changed = 1;
    if (!w25_read_data(MEM_CONFIG_LEGACY + cfg_job.offset, b, sizeof(b))) goto fail;
    if (memcmp(b, cfg_job.page, sizeof(b))) cfg_job.mirror_changed = 1;
    cfg_job.offset += sizeof(b);
    if (cfg_job.offset == MEMSECTORSIZE) {
      cfg_job.offset = 0;
      cfg_job.state = cfg_job.first ? CA_META : cfg_job.mirror_changed ? CA_MIRROR0 : cfg_job.changed ? CA_META : CA_DONE;
    }
    return;
  case CA_MIRROR0: address = MEM_CONFIG_LEGACY & ~4095u; cfg_job.state = CA_MIRROR1; break;
  case CA_MIRROR1: address = (MEM_CONFIG_LEGACY & ~4095u) + 4096u; cfg_job.offset = 1024; cfg_job.state = CA_TAIL; break;
  case CA_META: address = cfg_job.target + MEMSECTORSIZE; cfg_job.state = CA_DATA; break;
  case CA_DATA:
    address = cfg_job.target; cfg_job.offset = 0; cfg_job.state = CA_COPY;
    cfg_job.header[2] = crc32_update(0xFFFFFFFFu, (uint8_t*)&cfg_job.header[1], 4); break;
  case CA_TAIL: case CA_DEVICE: case CA_CONFIG: case CA_COPY:
    if (!cfg_async_page()) goto fail;
    address = (cfg_job.state == CA_COPY ? cfg_job.target : MEM_CONFIG_LEGACY) + cfg_job.offset;
    if (!w25_async_start(address, cfg_job.page, sizeof(cfg_job.page), now)) goto fail;
    if (cfg_job.state == CA_COPY) cfg_job.header[2] = crc32_update(cfg_job.header[2], cfg_job.page, sizeof(cfg_job.page));
    cfg_job.offset += sizeof(cfg_job.page); cfg_job.waiting = 1;
    if (cfg_job.state == CA_TAIL && cfg_job.offset == MEMSECTORSIZE) { cfg_job.offset = 512; cfg_job.state = CA_DEVICE; }
    else if (cfg_job.state == CA_DEVICE && cfg_job.offset == 1024) { cfg_job.offset = 0; cfg_job.state = CA_CONFIG; }
    else if (cfg_job.state == CA_CONFIG && cfg_job.offset == 512) cfg_job.state = cfg_job.first || !cfg_job.changed ? CA_DONE : CA_META;
    else if (cfg_job.state == CA_COPY && cfg_job.offset == MEMSECTORSIZE) {
      cfg_job.offset = 0; cfg_job.crc = crc32_update(0xFFFFFFFFu, (uint8_t*)&cfg_job.header[1], 4); cfg_job.state = CA_CRC;
    }
    return;
  case CA_CRC:
    if (!w25_read_data(cfg_job.target + cfg_job.offset, b, sizeof(b))) goto fail;
    cfg_job.crc = crc32_update(cfg_job.crc, b, sizeof(b)); cfg_job.offset += sizeof(b);
    if (cfg_job.offset == MEMSECTORSIZE) {
      if (cfg_job.crc != cfg_job.header[2]) goto fail;
      cfg_job.header[3] = ~cfg_job.header[2]; cfg_job.state = CA_HEADER;
    }
    return;
  case CA_HEADER:
    if (!w25_async_start(cfg_job.target + MEMSECTORSIZE, (uint8_t*)cfg_job.header, sizeof(cfg_job.header), now)) goto fail;
    cfg_job.waiting = 1; cfg_job.state = cfg_job.first ? CA_MIRROR0 : CA_DONE; return;
  case CA_DONE:
    if (cfg_job.changed || cfg_job.first) { cfg_cached_active = cfg_job.target; cfg_cached_generation = cfg_job.header[1]; }
    cfg_job.result = 2; return;
  default: goto fail;
  }
  if (!w25_async_start(address, NULL, 0, now)) goto fail;
  cfg_job.waiting = 1; return;
fail:
  mem_config_errors++; cfg_job.result = 3;
}
/* <<< Cooperative config publication */
