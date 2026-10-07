// Доработка: Кряжев А., 2026
// Индекс ID по корзинам и журнал его соответствия базе.
// При изменении таблиц индекс приостанавливается и пересобирается в фоне.
// Запись корзины проверяет W25; недоступный индекс допускает поиск по основной базе.
#include "idindex.h"
#include "runtime_internal.h"
#include "fast_kernels.h"
#include <string.h>
#ifndef IDINDEX_HOST_TEST
#include "memory.h"
#endif

#define IX_BASE    0x300000u                 /* выше фото (2..3 МБ), область свободна */
#define IX_BUCKETS 4096u
#define IX_SLOTS   32u
#define IX_BYTES   (IX_SLOTS * 2u)           /* корзина 64 байта, 64 корзины в секторе */
#define IX_AREA    (IX_BUCKETS * IX_BYTES)   /* 256 КБ = 64 сектора */
#define IX_LOG     (IX_BASE + IX_AREA)       /* сектор отметок верности */
#define IX_MARKS   (4096u / 8u)              /* отметка 8 байт: сумма и её проверка */
#define IX_MAGIC   0x58444931u

enum {
  IX_BOOT, IX_INVALIDATE, IX_FILL,
  IX_READY, IX_OFF, IX_ERASE,
  IX_ADD, IX_PUBLISH, IX_DRAIN
};
enum {
  IX_LOG_ERASE=1, IX_LOG_DIRTY, IX_BUCKET_ERASE,
  IX_BUCKET_WRITE, IX_COMMIT_ERASE, IX_LOG_VALID
};
struct {
  uint32_t sum;        /* сумма хешей ячеек базы - по базе, а не по индексу */
  uint16_t step, end;  /* шаг пересборки; последняя записанная ячейка базы */
  uint16_t mark;       /* следующее свободное место в секторе отметок */
  uint8_t state;
} ix;

// Выбирает корзину по хешу десяти байтов ID.
static uint32_t ix_bucket(const uint8_t *id)
{
  uint32_t h = fast_fnv10(id);
  return IX_BASE + ((h ^ (h >> 12) ^ (h >> 24)) & (IX_BUCKETS - 1u)) * IX_BYTES;
}

/* Сначала отметка «неверен», потом стирание: иначе старая верная отметка
   после обрыва признала бы полустёртый индекс. */
static void ix_rebuild(void)
{
  bool pending = flash_job.kind == FX_INDEX && flash_job.result == FX_PENDING && flash_job.step <= IX_LOG_VALID;
  ix.state = pending ? IX_DRAIN : IX_INVALIDATE;
  ix.step = 0;
}

/* Последняя отметка журнала и первое свободное место. */
static bool ix_last(uint32_t *sum)
{
  uint32_t r[16];
  bool valid = false;
  ix.mark = IX_MARKS;
  for (uint16_t i = 0; i < IX_MARKS; i += 8u) {
    if (!mem_read(IX_LOG + (uint32_t)i * 8u, (uint8_t*)r, sizeof(r))) return false;
    for (uint8_t k = 0; k < 8u; k++) {
      if (r[2 * k] == 0xFFFFFFFFu && r[2 * k + 1] == 0xFFFFFFFFu) { ix.mark = (uint16_t)(i + k); return valid; }
      valid = r[2 * k + 1] == (~r[2 * k] ^ IX_MAGIC);
      *sum = r[2 * k];
    }
  }
  return valid;
}

// Принимает итог сканирования основной базы и запускает индекс.
void idindex_impl_scanned(uint32_t sum, uint16_t cells_end)
{
  uint32_t marked = 0;
  ix.sum = sum;
  ix.end = cells_end;
  if (ix.state == IX_BOOT && ix_last(&marked) && marked == sum) { ix.state = IX_READY; return; }
  ix_rebuild();
}

// Журнал таблиц приостановил индекс; новая сумма запускает пересборку.
void idindex_impl_table_commit(uint32_t delta, uint16_t end)
{
  idindex_scanned(ix.sum ^ delta, end);
}

// Проверяет кандидатов корзины по действующим ячейкам основной базы.
int8_t idindex_impl_find(uint32_t base, const uint8_t *id, uint8_t *cell)
{
  uint16_t e[IX_SLOTS];
  if (ix.state != IX_READY) return -1;
  if (!mem_read(ix_bucket(id), (uint8_t*)e, IX_BYTES)) return -1;
  for (uint8_t i = 0; i < IX_SLOTS && e[i] != 0xFFFFu; i++) {
    if (!e[i]) continue;
    if (!mem_read(base + (uint32_t)(e[i] - 1u) * IDINDEX_CELL, cell, IDINDEX_CELL)) return -1;
    if (cell[31]!=255 && (cell[0] != 0xFF || cell[1] != 0xFF) && !memcmp(cell + 2, id, 10)) return 1;
  }
  return 0;
}

// Принимает квант индекса после CRC-фона, без I/O и без второго polling.
bool idindex_runtime_begin(void)
{
  if (!storage_has_index() || store.intent || store_reset_requested ||
      mem_config_result() || fw_event_waiting() || store.clear == 9) return false;
  if (ix.state != IX_INVALIDATE && ix.state != IX_FILL &&
      (ix.state < IX_ERASE || ix.state > IX_PUBLISH)) return false;
  flash_exec_args_t args = { NULL, FX_INDEX_READ };
  return flash_exec_begin(FX_INDEX, &args);
}

// Делает ограниченный шаг; источник записи живёт в union до verify.
void idindex_impl_poll(uint32_t base)
{
  uint16_t rows[IX_SLOTS], n = 0;
  uint8_t op, result;
  uint32_t address = 0;
  const uint8_t *source = NULL;
  if (flash_job.kind != FX_INDEX || flash_job.result != FX_PENDING) return;
  op = flash_job.step <= IX_LOG_VALID ? flash_job.step : 0;
  if (op) {
    result = w25_async_poll(timers_getMillis());
    if (result == 1) return;
    if (result != 2) goto error;
    if (ix.state == IX_OFF) goto cancelled;
    if (ix.state == IX_DRAIN) {
      ix.state = IX_INVALIDATE;
      goto complete;
    }
    if (op == IX_LOG_ERASE || op == IX_COMMIT_ERASE) {
      ix.mark = 0;
      goto complete;
    }
    if (op == IX_LOG_DIRTY) {
      ix.mark++;
      ix.state = IX_ERASE;
      goto complete;
    }
    if (op == IX_BUCKET_ERASE) {
      if (++ix.step >= 64) {
        ix.state = IX_FILL;
        ix.step = 1;
      }
      goto complete;
    }
    if (op == IX_BUCKET_WRITE) goto advance;
    if (op == IX_LOG_VALID) {
      ix.mark++;
      ix.state = IX_READY;
      goto complete;
    }
    goto error;
  }
  if (ix.state == IX_OFF) goto cancelled;
  if (ix.state == IX_INVALIDATE || ix.state == IX_PUBLISH) {
    op = ix.state == IX_PUBLISH ? IX_LOG_VALID : IX_LOG_DIRTY;
    if (ix.mark >= IX_MARKS) {
      op = ix.state == IX_PUBLISH ? IX_COMMIT_ERASE : IX_LOG_ERASE;
      address = IX_LOG;
      goto start;
    }
    address = IX_LOG + 8u * ix.mark;
    flash_job.data.index.source[0] = op == IX_LOG_VALID ? ix.sum : 0;
    flash_job.data.index.source[1] = op == IX_LOG_VALID ? (~ix.sum ^ IX_MAGIC) : 0;
    source = (uint8_t *)flash_job.data.index.source;
    n = 8;
    goto start;
  }
  if (ix.state == IX_ERASE) {
    address = IX_BASE + 4096u * ix.step;
    op = IX_BUCKET_ERASE;
    goto start;
  }
  if (ix.state == IX_ADD) {
    if (!w25_read_data(flash_job.data.index.bucket, (uint8_t *)rows, 64)) goto error;
    for (unsigned i = 0; i < 32; i++) {
      if (rows[i] == ix.step) goto advance;
      if (rows[i] == 65535) {
        address = flash_job.data.index.bucket + 2 * i;
        ((uint16_t *)flash_job.data.index.source)[0] = ix.step;
        source = (uint8_t *)flash_job.data.index.source;
        op = IX_BUCKET_WRITE;
        n = 2;
        goto start;
      }
    }
    goto error;
  }
  if (ix.step > ix.end) {
    ix.state = IX_PUBLISH;
    goto complete;
  }
  if (!w25_read_data(base + 32u * (uint32_t)(ix.step - 1), (uint8_t *)rows, 32)) goto error;
  if (rows[0] == 65535 || ((uint8_t *)rows)[31] == 255) goto advance;
  flash_job.data.index.bucket = ix_bucket((uint8_t *)rows + 2);
  ix.state = IX_ADD;
  return;
advance:
  if (ix.step >= ix.end) ix.state = IX_PUBLISH;
  else {
    ix.step++;
    ix.state = IX_FILL;
  }
complete:
  flash_exec_finish(FX_OK);
  return;
start:
  if (!w25_async_start(address, source, n, timers_getMillis())) goto error;
  flash_job.step = op;
  return;
error:
  ix.state = IX_OFF;
  flash_exec_finish(FX_ERROR);
  return;
cancelled:
  flash_exec_finish(FX_CANCELLED);
}

// Приостанавливает индекс на время изменения основной базы.
void idindex_runtime_suspend(void) { ix.state=IX_OFF; }
