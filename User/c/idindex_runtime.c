// Доработка: Кряжев А., 2026
// Индекс базы ID по корзинам. Устройство и правила - в idindex.h.
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
#define IX_STEP    8u                        /* ячеек за шаг пересборки */

enum { IX_BOOT, IX_ERASE, IX_FILL, IX_READY, IX_OFF };
struct {
  uint32_t sum;        /* сумма хешей ячеек базы - по базе, а не по индексу */
  uint16_t step, end;  /* шаг пересборки; последняя записанная ячейка базы */
  uint16_t mark;       /* следующее свободное место в секторе отметок */
  uint8_t state, pending;
  uint32_t source[2], bucket;
} ix;

#ifndef ASM_ID_HASH
static uint32_t ix_fnv(uint32_t h, const uint8_t *p, uint8_t n)
{
  while (n--) { h ^= *p++; h *= 16777619u; }
  return h;
}
uint32_t idindex_hash(uint16_t index, const uint8_t *id)
{
  uint8_t n[2] = {(uint8_t)index, (uint8_t)(index >> 8)};
  return ix_fnv(ix_fnv(2166136261u, n, 2), id, 10);
}
#endif
static uint32_t ix_bucket(const uint8_t *id)
{
  uint32_t h = fast_fnv10(id);
  return IX_BASE + ((h ^ (h >> 12) ^ (h >> 24)) & (IX_BUCKETS - 1u)) * IX_BYTES;
}

/* Отметка в журнал верности: valid - индекс соответствует базе с суммой sum.
   Сектор заполнен - стирается; обрыв между стиранием и записью оставляет
   журнал пустым, это «индекс неверен». */
static bool ix_mark(bool valid, uint32_t sum)
{
  uint32_t r[2] = {0, 0};
  if (valid) { r[0] = sum; r[1] = ~sum ^ IX_MAGIC; }
  if (ix.mark >= IX_MARKS) {
    if (!mem_erase_sector(IX_LOG)) return false;
    ix.mark = 0;
  }
  return mem_write(IX_LOG + (uint32_t)ix.mark++ * 8u, (uint8_t*)r, 8);
}

/* Сначала отметка «неверен», потом стирание: иначе старая верная отметка
   после обрыва признала бы полустёртый индекс. */
static void ix_rebuild(void)
{
ix.state=ix.pending?8:1; ix.step=0;
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

void idindex_scanned(uint32_t sum, uint16_t cells_end)
{
  uint32_t marked = 0;
  ix.sum = sum;
  ix.end = cells_end;
  if (ix.state == IX_BOOT && ix_last(&marked) && marked == sum) { ix.state = IX_READY; return; }
  if (ix.state == IX_BOOT) ix.state = IX_ERASE;  /* отметки «неверен» не нужно: верной нет */
  ix_rebuild();
}

/* Table journal has suspended the index; rebuild from the committed delta. */
void idindex_table_commit(uint32_t delta, uint16_t end)
{
  idindex_scanned(ix.sum ^ delta, end);
}

int8_t idindex_find(uint32_t base, const uint8_t *id, uint8_t *cell)
{
  uint16_t e[IX_SLOTS];
  if (ix.state != IX_READY) return -1;
  if (!mem_read(ix_bucket(id), (uint8_t*)e, IX_BYTES)) return -1;
  for (uint8_t i = 0; i < IX_SLOTS && e[i] != 0xFFFFu; i++) {
    if (!e[i]) continue;
    if (!mem_read(base + (uint32_t)(e[i] - 1u) * IDINDEX_CELL, cell, IDINDEX_CELL)) return -1;
    if ((cell[0] != 0xFF || cell[1] != 0xFF) && !memcmp(cell + 2, id, 10)) return 1;
  }
  return 0;
}

/* Номер в корзину: уже есть - ничего, есть пустое место - дописать 2 байта
   и проверить чтением. Полная корзина или сбой записи - false. */
static bool ix_add(uint16_t index, const uint8_t *id)
{
  uint16_t e[IX_SLOTS];
  uint32_t at = ix_bucket(id);
  if (!mem_read(at, (uint8_t*)e, IX_BYTES)) return false;
  for (uint8_t i = 0; i < IX_SLOTS; i++) {
    if (e[i] == index) return true;
    if (e[i] != 0xFFFFu) continue;
    uint16_t back = 0;
    return mem_write(at + i * 2u, (uint8_t*)&index, 2)
        && mem_read(at + i * 2u, (uint8_t*)&back, 2) && back == index;
  }
  return false;
}

bool idindex_put(uint16_t index, const uint8_t *id)
{
  /* При стирании и выше шага пересборки ячейку подберёт сама пересборка. */
  if (ix.state != IX_READY && !(ix.state == IX_FILL && index < ix.step)) return true;
  if (ix_add(index, id)) return true;
  if (ix.state == IX_READY) ix_rebuild(); else ix.state = IX_OFF;
  return false;
}

void idindex_commit(uint32_t delta, uint16_t cells_end)
{
  ix.sum ^= delta;
  if (cells_end > ix.end) ix.end = cells_end;
  if (ix.state == IX_READY && !ix_mark(true, ix.sum)) ix.state = IX_OFF;
}

void idindex_poll(uint32_t base)
{
uint16_t rows[32],n=0;
  uint8_t op=ix.pending,result; uint32_t address=0; const uint8_t *source=NULL;
  if(op) {
    result=w25_async_poll(timers_getMillis()); if(result==1) return;
    flash_release(FO_INDEX); ix.pending=0;
    if(result!=2) goto error;
    if(ix.state==4) return;
    if(ix.state==8) { ix.state=1; return; }
    if(op==1 || op==5) { ix.mark=0; return; }
    if(op==2) { ix.mark++; ix.state=5; return; }
    if(op==3) { if(++ix.step>=64) { ix.state=2; ix.step=1; } return; }
    if(op==4) goto advance;
    if(op==6) { ix.mark++; ix.state=3; return; }
    goto error;
  }
  if(ix.state!=1 && ix.state!=2 && (ix.state<5 || ix.state>7)) return;
  if(w25_async_active() || !w25_read_available()) return;
  if(ix.state==1 || ix.state==7) {
    op=ix.state==7?6:2;
    if(ix.mark>=512) { op--; address=IX_LOG; goto start; }
    address=IX_LOG+8u*ix.mark;
    ix.source[0]=op==6?ix.sum:0; ix.source[1]=op==6?(~ix.sum^IX_MAGIC):0;
    source=(uint8_t *)ix.source; n=8; goto start;
  }
  if(ix.state==5) { address=IX_BASE+4096u*ix.step; op=3; goto start; }
  if(ix.state==6) {
    if(!w25_read_data(ix.bucket,(uint8_t *)rows,64)) goto error;
    for(unsigned i=0;i<32;i++) {
      if(rows[i]==ix.step) goto advance;
      if(rows[i]==65535) {
        address=ix.bucket+2*i; ((uint16_t *)ix.source)[0]=ix.step;
        source=(uint8_t *)ix.source; op=4; n=2; goto start;
      }
    }
    goto error;
  }
  if(ix.step>ix.end) { ix.state=7; return; }
  if(!w25_read_data(base+32u*(uint32_t)(ix.step-1),(uint8_t *)rows,12)) goto error;
  if(rows[0]==65535) goto advance;
  ix.bucket=ix_bucket((uint8_t *)rows+2); ix.state=6; return;
advance:
  if(ix.step>=ix.end) ix.state=7; else { ix.step++; ix.state=2; } return;
start:
  if(!w25_async_start(address,source,n,timers_getMillis())) goto error;
  flash_claim(FO_INDEX); ix.pending=op; return;
error: ix.state=4;
}

void idindex_runtime_suspend(void) { ix.state=IX_OFF; }

bool idindex_runtime_active(void) { return ix.pending!=0; }
