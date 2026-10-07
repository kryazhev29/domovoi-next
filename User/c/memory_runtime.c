// Доработка: Кряжев А., 2026
// Доступ к W25, восстановление таблиц и атомарные настройки A/B.
// Старое место читается только для первого переноса; сохранения меняют неактивный слот.
// W25 проверяет запись и стирание; CRC слота проверяется при старте.
#include "../domovoi_diag.h"

#include "memory.h"
#include "runtime_internal.h"
#include "fast_kernels.h"
#include "sector_journal.h"

bool mem_storage_ready = false;
// Запрещает таблицы после ошибки даже при прямом вызове журнала, минуя mem_replace_pages.
static uint8_t mem_journal_result(uint8_t ok)
{
  if (!ok) mem_storage_ready = false;
  return ok;
}
// Читает проверенный журналом адрес через драйвер W25.
uint8_t sj_io_read(uint32_t adr, uint8_t *p, uint16_t n) { return mem_journal_result(w25_read_data(adr, p, n)); }
// Передаёт запись журнала драйверу с проверкой результата.
uint8_t sj_io_write(uint32_t adr, const uint8_t *p, uint16_t n) { return mem_journal_result(w25_write_data(adr, (uint8_t*)p, n)); }
// Стирает проверенный журналом сектор с проверкой драйвером.
uint8_t sj_io_erase(uint32_t adr) { return mem_journal_result(w25_erase_sector(adr)); }
// Поддерживает сторожевой таймер при синхронном восстановлении.
void sj_io_service(void) { wdt_reload(); }
// Восстанавливает журнал при старте и задаёт доступность таблиц.
bool mem_recover(void) { mem_storage_ready = sj_recover(); return mem_storage_ready; }
#include "sector_runtime.h"
extern uint8_t table_fault;
// Проверяет готовность таблиц, если диапазон затрагивает их сектора.
static bool mem_guard(uint32_t adr, uint16_t n)
{
  uint32_t sector, last;
  if (n == 0) return true;
  last = (adr + n - 1u) & ~4095u;
  for (sector = adr & ~4095u; ; sector += MEMSECTORSIZE) {
    if (sj_destination(sector)) return mem_storage_ready && !table_fault && !sj_runtime_blocked();
    if (sector == last) return true;
  }
}
// Не допускает обычную запись в служебные резервные сектора.
static bool mem_reserved(uint32_t adr, uint16_t n)
{
  return n && (adr < 0x1FE000u && adr + n > SJ_DATA);
}
// Заменяет сектор из источника страниц без копии 4 КиБ в ОЗУ.
bool mem_replace_pages(uint32_t adr, mem_page_reader reader, const void *context)
{
  mem_storage_ready = sj_replace_pages(adr, reader, context);
  return mem_storage_ready;
}

// Две смежные правки: config/device не требуют общей копии сектора.
typedef struct {
  uint32_t source, offset;
  const uint8_t *first, *second;
  uint16_t nfirst, nsecond;
} mem_image_s;

// Накладывает изменяемый фрагмент на страницу сохранённого образа.
static void mem_overlay(uint8_t *page, uint16_t off, uint32_t start, const uint8_t *data, uint16_t size)
{
  uint32_t from = off > start ? off : start;
  uint32_t end = (uint32_t)off + SJ_PAGE;
  if (end > start + size) end = start + size;
  if (from < end) memcpy(page + from - off, data + from - start, end - from);
}

// Читает исходную страницу и накладывает обе смежные правки.
static uint8_t mem_image_page(const void *context, uint16_t off, uint8_t *page)
{
  const mem_image_s *image = context;
  if (!w25_read_data(image->source + off, page, SJ_PAGE)) return 0;
  mem_overlay(page, off, image->offset, image->first, image->nfirst);
  mem_overlay(page, off, image->offset + image->nfirst, image->second, image->nsecond);
  return 1;
}

// Два слота по 8 КиБ: образ 4 КиБ и отдельный сектор публикации.
// Целый старый образ переносится только при отсутствии обоих слотов.
// Сохранение меняет только неактивный слот; старое место больше не записывается.
volatile uint32_t mem_config_errors = 0;
static uint8_t cfg_boot_done;
uint32_t cfg_cached_active = MEM_CONFIG_LEGACY, cfg_cached_generation;
#define CFG_MAGIC 0x32474643u
// Считает CRC образа вместе с поколением при выборе слота.
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
// Проверяет заголовок и CRC слота на границе стартового чтения.
static uint8_t cfg_valid(uint32_t address, uint32_t *generation)
{
  uint32_t h[4], crc;
  if (!w25_read_data(address + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  if (h[0] != CFG_MAGIC || h[3] != ~h[2]) return 0;
  if (!cfg_image_crc(address, h[1], &crc) || crc != h[2]) return 0;
  *generation = h[1];
  return 1;
}
// Выбирает самый новый целый слот и сохраняет его адрес и поколение.
static void cfg_select(void)
{
  uint32_t a = 0, b = 0;
  uint8_t va = cfg_valid(MEM_CONFIG_SLOT0, &a);
  uint8_t vb = cfg_valid(MEM_CONFIG_SLOT1, &b);
  if (vb && (!va || (int32_t)(b - a) > 0)) {
    cfg_cached_generation = b; cfg_cached_active = MEM_CONFIG_SLOT1; return;
  }
  if (va) { cfg_cached_generation = a; cfg_cached_active = MEM_CONFIG_SLOT0; return; }
  cfg_cached_generation = 0; cfg_cached_active = MEM_CONFIG_LEGACY;
}
// Сравнивает изменяемую часть, чтобы не стирать слот без изменений.
static uint8_t cfg_same_part(uint32_t address, const uint8_t *data, uint16_t size)
{
  uint8_t check[64];
  while (size) {
    uint16_t n = size > sizeof(check) ? sizeof(check) : size;
    if (!w25_read_data(address, check, n) || memcmp(check, data, n)) return 0;
    address += n; data += n; size -= n;
  }
  return 1;
}
// Пишет неактивный слот; заголовок публикуется после проверенного драйвером образа.
static uint8_t cfg_publish_pages(const mem_image_s *image, uint32_t active, uint32_t generation)
{
  uint32_t h[4];
  uint32_t target = active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  uint8_t page[SJ_PAGE];
  if (!w25_erase_sector(target + MEMSECTORSIZE)) return 0;
  if (!w25_erase_sector(target)) return 0;
  h[0] = CFG_MAGIC; h[1] = generation + 1u;
  uint32_t crc = crc32_update(0xFFFFFFFFu, (uint8_t*)&h[1], 4);
  for (uint16_t off = 0; off < MEMSECTORSIZE; off += SJ_PAGE) {
    wdt_reload();
    if (!mem_image_page(image, off, page)) return 0;
    crc = crc32_update(crc, page, sizeof(page));
    if (!w25_write_data(target + off, page, sizeof(page))) return 0;
  }
  h[2] = crc; h[3] = ~h[2];
  if (!w25_write_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  cfg_cached_active = target; cfg_cached_generation = h[1];
  return 1;
}

// Выбирает слот один раз при старте и переносит целые настройки старого формата.
static uint8_t cfg_boot_select(void)
{
  uint8_t block[64];
  if (cfg_boot_done) return 1;
  cfg_select();
  if (cfg_cached_active == MEM_CONFIG_LEGACY) {
    // CRC config и device проверяются на границе: старое место не имеет заголовка A/B.
    for (uint16_t part = 0; part < 1024; part += 512) {
      uint16_t crc = 0xFFFFu, expected;
      for (uint16_t off = 0; off < 510;) {
        uint16_t n = 510 - off;
        if (n > sizeof(block)) n = sizeof(block);
        if (!w25_read_data(MEM_CONFIG_LEGACY + part + off, block, n)) return 0;
        crc = crc16_calcInit(block, n, crc); off += n;
      }
      if (!w25_read_data(MEM_CONFIG_LEGACY + part + 510, (uint8_t*)&expected, 2)) return 0;
      if (crc != expected) { cfg_boot_done = 1; return 1; }
    }
    mem_image_s image = {MEM_CONFIG_LEGACY, 0, NULL, NULL, 0, 0};
    if (!cfg_publish_pages(&image, cfg_cached_active, cfg_cached_generation)) {
      DOMOVOI_DIAG_INC(mem_config_errors); return 0;
    }
  }
  cfg_boot_done = 1;
  return 1;
}
// Сохраняет смежные части настроек, сохраняя остальные байты активного образа.
static bool cfg_commit_parts(uint16_t offset, const uint8_t *first, uint16_t nfirst, const uint8_t *second, uint16_t nsecond)
{
  if (!cfg_boot_select()) return false;
  uint32_t active = cfg_cached_active;
  if (active != MEM_CONFIG_LEGACY &&
      cfg_same_part(active + offset, first, nfirst) &&
      cfg_same_part(active + offset + nfirst, second, nsecond)) return true;
  mem_image_s image = {active, offset, first, second, nfirst, nsecond};
  if (!cfg_publish_pages(&image, active, cfg_cached_generation)) {
    DOMOVOI_DIAG_INC(mem_config_errors); return false;
  }
  return true;
}
// Распознаёт логический диапазон настроек для перенаправления в A/B.
static uint8_t cfg_range(uint32_t adr, uint16_t size)
{
  return adr >= MEM_CONFIG_LEGACY && adr < MEM_CONFIG_LEGACY + MEMSECTORSIZE &&
         size <= MEM_CONFIG_LEGACY + MEMSECTORSIZE - adr;
}
// Читает логический адрес из выбранного при старте и обновляемого при записи слота.
static bool cfg_read(uint32_t adr, uint8_t *buff, uint16_t size)
{
  if (!store_runtime && !cfg_boot_select()) return false;
  return w25_read_data(cfg_cached_active + adr - MEM_CONFIG_LEGACY, buff, size);
}

// Чтение данных с памяти
bool mem_read (Int32U adr, pInt8U buff, Int16U size)
{
  if (cfg_range(adr, size)) return cfg_read(adr, buff, size);
  return mem_guard(adr, size) && w25_read_data(adr, buff, size);
}

// Запись данных в память
// данные пишутся в предварительно стертый сектор
bool mem_write (Int32U adr, pInt8U buff, Int16U size)
{
  if (cfg_range(adr, size)) return mem_patch(adr, buff, size);
  return !mem_reserved(adr, size) && mem_guard(adr, size) && w25_write_data(adr, buff, size);
}

// Атомарно применяет две смежные правки через слот или журнал таблиц.
bool mem_patch2(Int32U adr, const uint8_t *first, Int16U nfirst, const uint8_t *second, Int16U nsecond)
{
  uint32_t size = (uint32_t)nfirst + nsecond;
  if (cfg_range(adr, (uint16_t)size)) return cfg_commit_parts((uint16_t)(adr - MEM_CONFIG_LEGACY), first, nfirst, second, nsecond);
  uint32_t sector = adr & ~4095u;
  if (!flash_exec_read_allowed()) return false;
  mem_image_s image = {sector, adr - sector, first, second, nfirst, nsecond};
  return mem_replace_pages(sector, mem_image_page, &image);
}

// Передаёт одну правку общему пути атомарного сохранения.
bool mem_patch(Int32U adr, const uint8_t *data, Int16U size)
{
  return mem_patch2(adr, data, size, NULL, 0);
}

// Стирает обычный сектор, учитывая резерв и готовность таблиц.
bool mem_erase_sector (Int32U adr)
{
  adr &= ~4095u;
  return !mem_reserved(adr, MEMSECTORSIZE) && mem_guard(adr, MEMSECTORSIZE) && w25_erase_sector(adr);
}

// Фоновая публикация: один фрагмент 64 Б или одна команда W25 за вызов.
// Снимок config/device принадлежит автомату до завершения проверки драйвером.
struct {
  uint8_t *snapshot;
  uint32_t source, target, header[4];
  uint16_t offset;
  uint8_t state, result, waiting, changed, first;
} cfg_job;
enum {
  CA_CHECK=1, CA_META, CA_DATA,
  CA_COPY, CA_HEADER, CA_DONE
};
// Возвращает состояние публикации: 0 — нет, 1 — работа, 2 — готово, 3 — ошибка.
uint8_t mem_config_result(void) { return cfg_job.result; }
// Даёт владельцу сохранённый снимок для восстановления при ошибке.
const uint8_t *mem_config_snapshot(void) { return cfg_job.snapshot; }
// Освобождает снимок после завершения автомата и последней проверки W25.
void mem_config_reset(void)
{
  if (cfg_job.result == 1) return;
  if (flash_job.kind == FX_CONFIG) flash_exec_take_result();
  work_release(cfg_job.snapshot, WORK_CONFIG); cfg_job.snapshot = NULL; cfg_job.result = 0; cfg_job.waiting = 0;
}
// Принимает готовые config/device, не сохраняя указатели вызывающего.
bool mem_config_begin(const uint8_t *first, const uint8_t *second)
{
  if (table_fault || sj_runtime_blocked() || cfg_job.result || w25_async_active() || !cfg_boot_done) return false;
  cfg_job.snapshot = work_acquire(1024, WORK_CONFIG);
  if (!cfg_job.snapshot) return false;
  if (flash_job.kind != FX_STORE && flash_job.kind != FX_FWSERVICE &&
      !flash_exec_begin(FX_CONFIG, NULL)) {
    work_release(cfg_job.snapshot, WORK_CONFIG);
    cfg_job.snapshot = NULL;
    return false;
  }
  memcpy(cfg_job.snapshot, first, 512); memcpy(cfg_job.snapshot + 512, second, 512);
  cfg_job.source = cfg_cached_active;
  cfg_job.target = cfg_cached_active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;
  cfg_job.header[0] = CFG_MAGIC; cfg_job.header[1] = cfg_cached_generation + 1u;
  cfg_job.offset = 0; cfg_job.changed = 0;
  cfg_job.first = cfg_cached_active == MEM_CONFIG_LEGACY;
  cfg_job.waiting = 0; cfg_job.state = CA_CHECK; cfg_job.result = 1;
  return true;
}
// Берёт новый config/device из снимка, а неизменный хвост — из активного образа.
static bool cfg_async_page(void)
{
  if (cfg_job.offset < 1024u) {
    memcpy(sjr.data, cfg_job.snapshot + cfg_job.offset, sizeof(sjr.data)); return true;
  }
  return w25_read_data(cfg_job.source + cfg_job.offset, sjr.data, sizeof(sjr.data));
}
// Делает один шаг; до заголовка прежний активный слот остаётся единственным целым.
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
    return;
  }
  switch (cfg_job.state) {
  case CA_CHECK:
    if (!w25_read_data(cfg_job.source + cfg_job.offset, b, sizeof(b))) goto fail;
    if (memcmp(b, cfg_job.snapshot + cfg_job.offset, sizeof(b))) cfg_job.changed = 1;
    cfg_job.offset += sizeof(b);
    if (cfg_job.offset == 1024u) {
      cfg_job.offset = 0; cfg_job.state = cfg_job.first || cfg_job.changed ? CA_META : CA_DONE;
    }
    return;
  case CA_META: address = cfg_job.target + MEMSECTORSIZE; cfg_job.state = CA_DATA; break;
  case CA_DATA:
    address = cfg_job.target; cfg_job.offset = 0; cfg_job.state = CA_COPY;
    cfg_job.header[2] = crc32_update(0xFFFFFFFFu, (uint8_t*)&cfg_job.header[1], 4); break;
  case CA_COPY:
    if (!cfg_async_page()) goto fail;
    address = cfg_job.target + cfg_job.offset;
    if (!w25_async_start(address, sjr.data, sizeof(sjr.data), now)) goto fail;
    cfg_job.header[2] = crc32_update(cfg_job.header[2], sjr.data, sizeof(sjr.data));
    cfg_job.offset += sizeof(sjr.data); cfg_job.waiting = 1;
    if (cfg_job.offset == MEMSECTORSIZE) {
      cfg_job.header[3] = ~cfg_job.header[2]; cfg_job.state = CA_HEADER;
    }
    return;
  case CA_HEADER:
    if (!w25_async_start(cfg_job.target + MEMSECTORSIZE, (uint8_t*)cfg_job.header, sizeof(cfg_job.header), now)) goto fail;
    cfg_job.waiting = 1; cfg_job.state = CA_DONE; return;
  case CA_DONE:
    if (cfg_job.changed || cfg_job.first) { cfg_cached_active = cfg_job.target; cfg_cached_generation = cfg_job.header[1]; }
    cfg_job.result = 2;
    if (flash_job.kind == FX_CONFIG) flash_exec_finish(FX_OK);
    return;
  default: goto fail;
  }
  if (!w25_async_start(address, NULL, 0, now)) goto fail;
  cfg_job.waiting = 1; return;
fail:
  DOMOVOI_DIAG_INC(mem_config_errors);
  cfg_job.result = 3;
  if (flash_job.kind == FX_CONFIG) flash_exec_finish(FX_ERROR);
}

