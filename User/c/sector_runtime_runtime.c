// Доработка: Кряжев А., 2026
// Фоновая замена сектора через тот же журнал, шагами по 64 Б.
// Буфер sjr принадлежит операции до завершения проверки W25.
// После фиксации ошибки блокируют таблицы до восстановления при старте.
#include "sector_runtime.h"
#include "sector_journal.h"
#include "flash_w25.h"
#include <string.h>
#include "memory.h"
#include "../id_crc_io.h"

sj_runtime_state_s sjr;

// Сообщает о незавершённом журнале или ошибке после фиксации.
uint8_t sj_runtime_blocked(void) { return sjr.fault || (sjr.phase && sjr.phase < 30); }
// Запрашивает отмену до фиксации образа.
void sj_runtime_cancel(void) { sjr.cancelled = 1; }
// Сбрасывает завершённый автомат, сохраняя запрет при неясном результате.
void sj_runtime_reset(void) { if (!sjr.waiting) sjr.phase = 0; }
// Принимает проверенный адрес и источник при свободном журнале и W25.
uint8_t sj_runtime_begin(uint32_t address, sj_chunk_reader reader, const void *context)
{
  if (mem_config_result() || sjr.phase || sjr.fault || w25_async_active()) return 0;
  sjr.address=address; sjr.reader=reader; sjr.context=context;
  sjr.offset=0; sjr.expected=0xffffffffu; sjr.committed=sjr.cancelled=0;
  sjr.changed=0; sjr.phase=16; return 1;
}
// Блокирует таблицы, если ошибка возникла после попытки фиксации.
static void sjr_fail(void)
{
  if (sjr.committed) sjr.fault=1; // Неясный результат разрешает только восстановление при старте.
  sjr.phase=31;
}
// Запускает одну команду W25 и сохраняет следующий шаг.
static void sjr_start(uint32_t address, const uint8_t *data, uint16_t n, uint32_t now, uint8_t after)
{
  if (!w25_async_start(address,data,n,now)) { sjr_fail(); return; }
  sjr.waiting=1; sjr.after=after;
}
// Выполняет один шаг подготовки, фиксации или переноса сектора.
uint8_t sj_runtime_poll(uint32_t now)
{
  uint8_t check[64]; // Сравнение до записи, буфер живёт только в этом вызове.
  if (!sjr.phase) return 0;
  if (sjr.waiting) {
    uint8_t r=w25_async_poll(now);
    if (r==1) return 1;
    sjr.waiting=0;
    if (r!=2) sjr_fail(); else sjr.phase=sjr.after;
    return sjr.phase==31 ? 3 : 1;
  }
  if (sjr.cancelled && !sjr.committed && sjr.phase<30) sjr.phase=31;
  switch (sjr.phase) {
  case 16:
    if (!sjr.reader(sjr.context,sjr.offset,sjr.data) ||
        !w25_read_data(sjr.address+sjr.offset,check,64)) { sjr_fail(); break; }
    if (memcmp(sjr.data,check,64)) sjr.changed=1;
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.phase=sjr.changed ? 1 : 30; }
    break;
  case 1: sjr_start(SJ_META,0,0,now,2); break;
  case 2: sjr_start(SJ_DATA,0,0,now,3); break;
  case 3:
    if (!sjr.reader(sjr.context,sjr.offset,sjr.data)) { sjr_fail(); break; }
    sjr.expected=crc32_update(sjr.expected,sjr.data,64);
    sjr_start(SJ_DATA+sjr.offset,sjr.data,64,now,4); break;
  case 4:
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.phase=5; }
    else sjr.phase=3;
    break;
  case 5:
    // CRC уже посчитан по источнику, W25 проверил содержимое каждой записи.
    sjr.header[0]=SJ_MAGIC; sjr.header[1]=SJ_VERSION; sjr.header[2]=sjr.address;
    sjr.header[3]=SJ_SECTOR; sjr.header[4]=~sjr.expected; sjr.header[5]=sjr.expected;
    sjr.header[6]=~crc32_update(0xffffffffu,(uint8_t*)sjr.header,24);
    sjr.header[7]=SJ_COMMIT; sjr.phase=6;
    break;
  case 6: sjr_start(SJ_META,(uint8_t*)sjr.header,28,now,7); break;
  case 7: sjr.phase=8; break;
  case 8:
    sjr.committed=1; // Метка могла попасть во флеш даже при ошибке проверки.
    sjr_start(SJ_META+28,(uint8_t*)sjr.header+28,4,now,9); break;
  case 9: sjr.phase=10; break;
  case 10: sjr_start(sjr.address,0,0,now,11); break;
  case 11: sjr.offset=0; sjr.phase=12; break;
  case 12:
    if (!w25_read_data(SJ_DATA+sjr.offset,sjr.data,64)) { sjr_fail(); break; }
    sjr_start(sjr.address+sjr.offset,sjr.data,64,now,13); break;
  case 13:
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.phase=14; }
    else sjr.phase=12;
    break;
  case 14: sjr.phase=15; break;
  case 15: sjr_start(SJ_META,0,0,now,30); break;
  case 30: return 2;
  case 31: return 3;
  default: sjr_fail(); break;
  }
  return sjr.phase==31 ? 3 : 1;
}
