// Доработка: Кряжев А., 2026
// Доступ к W25, восстановление таблиц и атомарные настройки A/B.
// Старое место читается только для первого переноса; сохранения меняют неактивный слот.
// W25 проверяет запись и стирание; CRC слота проверяется при старте.
#include "../domovoi_diag.h"

#include "memory.h"
#include "runtime_internal.h"
#include "fast_kernels.h"
#include "sector_journal.h"
#include "counter_runtime.inc"

bool mem_storage_ready = false;
// Запрещает таблицы после ошибки даже при прямом вызове журнала, минуя mem_replace_sector (восстановление при старте).
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
// Заменяет сектор при старте тем же фоновым журналом, что в работе, дожидаясь его.
// Отказ, как и прежде, снимает готовность таблиц до следующего восстановления.
bool mem_replace_sector(uint32_t adr, sj_chunk_reader reader, const void *context)
{
  uint8_t r = 0;
  if (sj_runtime_begin(adr, reader, context))
    do { wdt_reload(); r = sj_runtime_poll(timers_getMillis()); } while (r == 1);
  sj_runtime_reset();
  mem_storage_ready = r == 2;
  return mem_storage_ready;
}

// Два слота по 8 КиБ: образ 4 КиБ и отдельный сектор публикации.
// Целый старый образ переносится только при отсутствии обоих слотов.
// Сохранение меняет только неактивный слот; старое место больше не записывается.
volatile uint32_t mem_config_errors = 0;
static uint8_t cfg_boot_done;
static uint16_t cfg_cursor_offset = 16;
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
// Выбирает слот один раз при старте.
static uint8_t cfg_boot_select(void)
{
  if (!cfg_boot_done) { cfg_select(); cfg_boot_done = 1; }
  return 1;
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
  return mem_guard(adr, size) && counter_base_read(adr, buff, size) && counter_overlay(adr,buff,size);
}

// Запись данных в память
// данные пишутся в предварительно стертый сектор
bool mem_write (Int32U adr, pInt8U buff, Int16U size)
{
  if (cfg_range(adr, size)) return false; /* настройки пишет только mem_config_begin (A/B) */
  return !mem_reserved(adr, size) && mem_guard(adr, size) && w25_write_data(adr, buff, size);
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
  CA_COPY, CA_HEADER, CA_DONE, CA_CURSOR_BODY, CA_CURSOR_COMMIT, CA_CURSOR_DONE
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
  if (!store_runtime) cfg_boot_select(); /* стартовая запись выбирает слот сама, как чтение */
  if (table_fault || sj_runtime_blocked() || cfg_job.result || w25_async_active() || !cfg_boot_done) return false;
  cfg_job.snapshot = work_acquire(1024, WORK_CONFIG);
  if (!cfg_job.snapshot) return false;
  if (flash_job.kind != FX_STORE && flash_job.kind != FX_FACTORY && flash_job.kind != FX_FWSERVICE &&
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
// Дочитывает дописанные ACK только выбранного целого A/B-слота.
// Оборванное тело занимает место, но не меняет подтверждённые курсоры.
bool mem_cursor_restore(device_s *saved)
{
  uint32_t row[4];
  cfg_cursor_offset = 16;
  if (cfg_cached_active == MEM_CONFIG_LEGACY) return true;
  for (uint16_t off = 16; off < MEMSECTORSIZE; off += 16) {
    if (!w25_read_data(cfg_cached_active + MEMSECTORSIZE + off, (uint8_t *)row, 16)) return false;
    if ((row[0] & row[1] & row[2] & row[3]) == 0xffffffffu) continue;
    cfg_cursor_offset = off + 16;
    uint16_t crc = (uint16_t)row[3];
    if (crc != (uint16_t)~(row[3] >> 16) || crc != crc16_calc((uint8_t *)row, 12)) continue;
    uint32_t magic;
    memcpy(&magic,saved->reserv+16,4);
    if (magic!=LOG_CURSOR_MAGIC) memset(saved->reserv+28,0,4); // Образ без маркера: номер журнала не хранил.
    magic=LOG_CURSOR_MAGIC;
    memcpy(saved->reserv+16,&magic,4);
    magic=DEVICE_UPLOAD_MARK;memcpy(saved->reserv,&magic,4);
    memcpy(saved->reserv + 20, row, 8);
    memcpy(saved->reserv + DEVICE_TEMP_USB_OFFSET, (uint8_t *)row + 8, 2);
    memcpy(&saved->readTempIDIndex, (uint8_t *)row + 10, 2);
  }
  return true;
}
// ACK занимает 16 Б в свободном хвосте сектора публикации, затем штатный A/B.
// Снимок нужен только для редкого переноса; нового буфера нет.
bool mem_cursor_begin(const uint8_t *first, const uint8_t *second)
{
  if (!mem_config_begin(first, second)) return false;
  if (cfg_cached_active == MEM_CONFIG_LEGACY || cfg_cursor_offset >= MEMSECTORSIZE) return true;
  const device_s *saved = (const device_s *)(cfg_job.snapshot + 512);
  memcpy(cfg_job.header, saved->reserv + 20, 8);
  memcpy((uint8_t *)cfg_job.header + 8, saved->reserv + DEVICE_TEMP_USB_OFFSET, 2);
  memcpy((uint8_t *)cfg_job.header + 10, &saved->readTempIDIndex, 2);
  uint16_t crc = crc16_calc((uint8_t *)cfg_job.header, 12);
  cfg_job.header[3] = crc | ((uint32_t)(uint16_t)~crc << 16);
  cfg_job.offset = cfg_cursor_offset;
  cfg_cursor_offset += 16; // При отказе не дописывать поверх оборванной отметки.
  cfg_job.state = CA_CURSOR_BODY;
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
  case CA_CURSOR_BODY:
    if (!w25_async_start(cfg_job.source + MEMSECTORSIZE + cfg_job.offset,
                         (uint8_t *)cfg_job.header, 12, now)) goto fail;
    cfg_job.waiting = 1; cfg_job.state = CA_CURSOR_COMMIT; return;
  case CA_CURSOR_COMMIT:
    if (!w25_async_start(cfg_job.source + MEMSECTORSIZE + cfg_job.offset + 12,
                         (uint8_t *)cfg_job.header + 12, 4, now)) goto fail;
    cfg_job.waiting = 1; cfg_job.state = CA_CURSOR_DONE; return;
  case CA_CURSOR_DONE:
    cfg_job.result = 2;
    if (flash_job.kind == FX_CONFIG) flash_exec_finish(FX_OK);
    return;
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
    if (cfg_job.changed || cfg_job.first) { cfg_cached_active = cfg_job.target; cfg_cached_generation = cfg_job.header[1]; cfg_cursor_offset = 16; }
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

