// Доработка: Кряжев А., 2026
// Основные и временные ID: поиск, счётчики проходов и сроки действия.
// Индекс ускоряет поиск; во время пересборки остаётся перебор блоками 256 Б.
// Стартовый скан сверяет метку индекса с базой; ошибка чтения отличается от отсутствия ID.
#include "user_logic_internal.h"
uint32_t id_used_bytes = UINT32_MAX;
uint16_t id_cells_end;
uint16_t id_scan_cell;
static struct { uint32_t sum; uint16_t end, used; } id_scan;
static uint16_t id_boot_hint; // номер, не копия ID: повтор доступа перечитывает одну ячейку
uint32_t id_nextEnd;
bool id_endScan = true;

// ID часто отличаются последними цифрами; каждый байт сравнивается ровно один раз.
#pragma inline=forced
static inline bool id_equal(const char *left, const char *right)
{
  uint8_t i = 10;
  if (left[0] != right[0]) return false;
  while (--i) if (left[i] != right[i]) return false;
  return true;
}

// Прочитать долговечную дату первого прохода ID.
uint32_t id_first_pass(cellID_s *c){
  uint32_t t;
  memcpy(&t, c->reserve, 4);
  return (t == 0xffffffff || t < ID_TIME_VALID) ? 0 : t;
}

// Сохранить дату первого прохода ID в резерв записи.
void id_set_first_pass(cellID_s *c, uint32_t t){
  memcpy(c->reserve, &t, 4);
}

// Определить, закончился ли срок действия ID.
bool id_expired(cellID_s *c){
  uint32_t first = id_first_pass(c);
  return c->useTime != 0 && first != 0 && device.datetime >= first &&
    device.datetime - first >= (uint32_t)c->useTime * 60u;
}

// Вычислить окончание срока действия ID по первой дате прохода.
uint32_t id_time_end(cellID_s *c){
  uint32_t first = id_first_pass(c), end;
  if (c->useTime == 0 || first == 0 || c->reserve[6] == ID_TIME_DONE) return 0;
  // за пределами представимого Unix-времени нет раннего окончания
  if (first > 0xffffffffu - (uint32_t)c->useTime * 60u) return 0;
  end = first + (uint32_t)c->useTime * 60u;
  return (end / ONE_DAY == first / ONE_DAY) ? end : 0;
}

// Учесть ближайшее окончание срока при очередном обходе.
void id_watch_end(uint32_t end){
  if (end != 0 && (id_nextEnd == 0 || end < id_nextEnd)) id_nextEnd = end;
}

// Вернуть номер текущих местных суток для счётчиков ID.
static uint16_t id_today(void){
  return device.datetime >= ID_TIME_VALID ? (uint16_t)(device.datetime / ONE_DAY) : 0;
}

// Прочитать сутки последнего прохода из записи ID.
static uint16_t id_pass_day(cellID_s *c){
  uint16_t d;
  memcpy(&d, &c->reserve[4], 2);
  return d == 0xffff ? 0 : d;
}

// Записать сутки последнего прохода в запись ID.
static void id_set_pass_day(cellID_s *c, uint16_t day){
  memcpy(&c->reserve[4], &day, 2);
}

// Определить, нужно ли восстановить суточный лимит проходов.
bool id_new_day(cellID_s *c){
  uint16_t today = id_today(), last = id_pass_day(c);
  if (today == 0 || last == today) return false;
  if (last != 0) c->cntPass = c->cntPassOnDay;
  id_set_pass_day(c, today);
  return true;
}

// Найти ID через индекс или ограниченный буфер резервного перебора.
uint8_t user_findID(char *id, cellID_s *cell){

  uint32_t adr;
  uint16_t idsum = 0;
  for (uint8_t i = 0; i < 10; i++) idsum += (uint8_t)id[i];
  if (idsum == 2550 || idsum == 0) return false; // пустой ID в базе не ищется
  // База неизменна до конца стартового прохода; сам ID всегда сверяем с флеш.
  if (id_scan_cell && id_boot_hint) {
    if (!mem_read(mem[MB_ID].start + (uint32_t)(id_boot_hint - 1u) * sizeof(cellID_s),
                  (uint8_t*)cell, sizeof(*cell))) return 2;
    if (cell->reserve[9] != 255 && cell->index != 0xffff && id_equal(cell->ID, id)) return true;
  }
  {
    cellID_s found;
    int8_t r = idindex_find(mem[MB_ID].start, (const uint8_t*)id, (uint8_t*)&found);
    if (r >= 0) {
      if (r) memcpy(cell, &found, sizeof(found));
      return r == 1;
    }
  }
  /* Индекс не готов (пересборка) - перебор по базе. */
  uint32_t endAdr = mem[MB_ID].start + (uint32_t)id_cells_end * sizeof(cellID_s);
  uint8_t tbuffer[256];
  cellID_s *c;

  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += sizeof(tbuffer)){
    if (!mem_read(adr, tbuffer, sizeof(tbuffer))) return 2;
    // перебираем ячейки
    for (uint16_t a = 0; a < sizeof(tbuffer) && adr + a < endAdr; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->reserve[9] == 255) continue;

      if (id_equal(c->ID, id) && c->index != 0xffff){
        if (id_scan_cell) id_boot_hint = (uint16_t)((adr + a - mem[MB_ID].start) / sizeof(cellID_s) + 1u);
        memcpy(cell, c, sizeof(cellID_s));
        return true;
      }
    }
  }
  // не нашли
  return false;
}

// Найти ID среди уже записанных временных записей.
uint8_t user_findTempID(char *id){

  uint32_t adr;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;

  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;

  // проверим, что такого ID нет в буфере
  for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return 2;
    // такой ID есть
    if (id_equal(cellId.ID, id)){
      return true;
    }
  }
  // не нашли
  return false;
}

// Добавить отсутствующий ID в первое свободное место временной таблицы.
bool user_addIdToTempBuffer(char *id, codeType_e tp){
  if (!flash_exec_idle()) return false;

  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;

  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;

  uint8_t found = user_findTempID(id);
  if (found == 1) return true;
  if (found > 1) return false;

  // по странично
  for (adr = startAddress; adr < endAdr; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return false;
    // ячейка пустая, она должна быть стерта
    if (cellId.index == 0xffff){
      memset(&cellId, 0, sizeof(cellId)); // новая запись без чужих флагов и резерва
      memcpy(cellId.ID, id, 10);
      switch (tp){
        case CT_PHONE: cellId.isPhone = 1; break;
        case CT_AUTO: cellId.isAuto = 1; break;
        case CT_WIEGAND: cellId.isWiegand = 1; break;
        default: break;
      }
      flash_exec_args_t args = { NULL, FS_SYNC };
      if (!flash_exec_begin(FX_STORE, &args)) return false;
      bool ok = mem_write(adr, (uint8_t*)&cellId, sizeof(cellID_s));
      flash_exec_finish(ok ? FX_OK : FX_ERROR);
      flash_exec_take_result();
      if (!ok) return false;
      device.newAddressTempID = adr + sizeof(cellID_s);
      device.status.readyID = 1;
      return true;
    }
  }
  return false;
}

// До полного прохода граница поиска охватывает всю базу, записи ждут.
void user_ids_boot(void){
  id_cells_end = (uint16_t)(mem[MB_ID].size / sizeof(cellID_s));
  id_used_bytes = UINT32_MAX;
  memset(&id_scan, 0, sizeof(id_scan));
  device.numbersCount = 0;
  id_scan_cell = 1;
  id_boot_hint = 0;
}

// Один шаг исполнителя: восемь ячеек; при ошибке повторяем ту же страницу.
void user_ids_scan(void){
  uint32_t offset = (uint32_t)(id_scan_cell - 1u) * sizeof(cellID_s);
  uint8_t tbuffer[256];
  cellID_s *c;
  uint16_t n = (uint16_t)(mem[MB_ID].size - offset > sizeof(tbuffer) ?
                         sizeof(tbuffer) : mem[MB_ID].size - offset);
  if (!mem_read(mem[MB_ID].start + offset, tbuffer, n)) {
    flash_exec_finish(FX_ERROR);
    return;
  }
  for (uint16_t a = 0; a < n; a += sizeof(cellID_s)){
    c = (cellID_s*)&tbuffer[a];
    /* Последняя ячейка, где есть хоть один записанный байт: за ней флеш стёрта. */
    for (uint8_t b = 0; b < sizeof(cellID_s); b++)
      if (tbuffer[a + b] != 0xff) { id_scan.used = id_scan_cell; break; }
    uint16_t cell = id_scan_cell++;
    // пустая ячейка
    if (c->index == 0xffff || c->reserve[9] == 255) continue;
    device.numbersCount++;
    id_scan.end = cell;
    id_scan.sum ^= idindex_hash(cell, (const uint8_t*)c->ID);
  }
  if (offset + n == mem[MB_ID].size) {
    id_cells_end = id_scan.end;
    id_used_bytes = (uint32_t)id_scan.used * sizeof(cellID_s);
    id_scan_cell = 0;
    idindex_scanned(id_scan.sum, id_cells_end);
  }
  flash_exec_finish(FX_OK);
}

// Записать событие окончания срока ID ровно один раз.
void user_checkIdTimeEnd(void){
expiry_runtime_begin();
}

// Найти конец временной таблицы и обновить указатель добавления.
uint16_t user_getTempIDRecordsCount(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  uint8_t tbuffer[256];
  cellID_s *c;
  uint16_t cnt = 0;

  // по странично читаем
  for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += sizeof(tbuffer)){
    if (!mem_read(adr, tbuffer, sizeof(tbuffer))) return 0xFFFFu;
    // перебираем ячейки
    for (uint16_t a = 0; a < sizeof(tbuffer); a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff){
        device.newAddressTempID = adr + a;
        return cnt;
      }

      cnt++;
    }
  }

  device.newAddressTempID = endAdr;
  return cnt;
}

// Записать изменённые параметры ID и обновить соответствие индекса.
bool user_updateID(cellID_s *c){
  if (!flash_exec_idle()) return false;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t stored[32];
  uint16_t index;
  uint32_t adr;
  /* Команда 13 кладёт ячейку на место её номера: сначала проверяем это место,
     перебор - только если там другая ячейка. */
  if (c->index != 0 && c->index != 0xffff){
    adr = mem[MB_ID].start + (uint32_t)(c->index - 1u) * sizeof(cellID_s);
    if (adr + sizeof(cellID_s) <= endAdr){
      if (!mem_read(adr, stored, 32)) { idcrc_unknown(); return false; }
      memcpy(&index,stored,2);
      if (index == c->index && stored[31] != 255) goto update;
    }
  }
  for (adr = mem[MB_ID].start; adr + sizeof(cellID_s) <= endAdr; adr += sizeof(cellID_s)){
    if (!mem_read(adr, stored, 32)) { idcrc_unknown(); return false; }
      memcpy(&index,stored,2);
    if (index == c->index && stored[31] != 255){
      goto update;
    }
  }
  idcrc_unknown(); return false;
update: {
  bool result = store_id_begin(adr, c);
  if(!result) idcrc_unknown();
  return result;
}
}
