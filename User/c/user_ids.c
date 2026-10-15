// Доработка: Кряжев А., 2026
// Основные и временные ID: поиск, счётчики проходов и сроки действия.
// Индекс ускоряет поиск; во время пересборки остаётся перебор блоками 256 Б.
// Стартовый скан сверяет метку индекса с базой; ошибка чтения отличается от отсутствия ID.
#include "user_logic_internal.h"
#include "../../Bootloader/app_update.h"
uint32_t id_used_bytes = UINT32_MAX;
uint16_t id_cells_end;
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
        memcpy(cell, c, sizeof(cellID_s));
        return true;
      }
    }
  }
  // не нашли
  return false;
}

// 5 бит поколения и 11 бит позиции: в области не более 2048 ячеек.
// Живут соседние поколения, поэтому разность int16_t однозначна при переходе через ноль.
uint16_t temp_next;
uint16_t temp_normalize(uint16_t cursor)
{
  return (cursor&2047u)>=mem[MB_TEMPID].size/32u ? (uint16_t)((cursor&~2047u)+2048u) : cursor;
}
uint32_t temp_address(uint16_t cursor)
{ return mem[MB_TEMPID].start+32u*(cursor&2047u); }
static uint16_t temp_read_max(void)
{ return (int16_t)(device.readTempIDIndex-temp_usbReadIndex)>0 ? device.readTempIDIndex : temp_usbReadIndex; }
// У старых временных ID index=0; новые фиксируют 0x8000 после тела и CRC.
// Частичное стирание/программирование не превращает новую метку в старую (бит 15 всегда 1).
// Любой сброшенный бит commit достаточен: тело уже проверено, старый ELF тоже видит этот ID.
bool user_temp_valid(const cellID_s *cell)
{
  const uint32_t *words=(const uint32_t *)cell;
  if(cell->index==0xffff) return false;
  if(!(cell->index&0x8000u)) return true;
  uint16_t crc=(uint16_t)words[6];
  return words[7]==0x32444954u &&
    crc==(uint16_t)~(words[6]>>16) && crc==crc16_calc((uint8_t *)cell+2,22);
}
// Старый линейный формат равен нулевому поколению; адреса и ID не меняются.
uint16_t temp_sequence(const cellID_s *cell,uint32_t address)
{
  uint16_t epoch=0;
  if(((const uint32_t *)cell)[7]==0x32444954u) memcpy(&epoch,cell->reserve,2);
  return (epoch&~2047u)|(uint16_t)((address-mem[MB_TEMPID].start)/32u);
}
// Не удерживаем уже выгруженный ID; оборванное тело не становится записью.
uint8_t user_findTempID(char *id)
{
  cellID_s cell;
  uint16_t limit=temp_normalize(temp_next);
  for(uint16_t cursor=temp_normalize(temp_read_max());cursor!=limit;cursor=temp_normalize(cursor+1)) {
    uint32_t adr=temp_address(cursor);
    if(!mem_read(adr,(uint8_t *)&cell,32)) return 2;
    if(user_temp_valid(&cell) && temp_sequence(&cell,adr)==cursor && id_equal(cell.ID,id)) return 1;
  }
  return 0;
}
// Стираем только сектор впереди записи и только если в нём нет неподтверждённых ID.
static bool temp_reclaim(uint32_t address)
{
  cellID_s cell;
  uint16_t retired=0;
  uint32_t start=address&~4095u, end=start+4096;
  uint32_t limit=mem[MB_TEMPID].start+mem[MB_TEMPID].size;
  if(end>limit) end=limit;
  if(start<mem[MB_TEMPID].start) start=mem[MB_TEMPID].start;
  for(uint32_t a=start;a<end;a+=32) {
    if(!mem_read(a,(uint8_t *)&cell,32)) return false;
    if(user_temp_valid(&cell)) {
      if((int16_t)(temp_sequence(&cell,a)-temp_read_max())>=0) return false;
      retired++;
    }
  }
  if(!mem_erase_sector(address&~4095u)) return false;
  device.tempIDsCount=device.tempIDsCount>retired?device.tempIDsCount-retired:0;
  return true;
}
// Тело и CRC пишутся до index: после обрыва неполная ячейка пропускается.
bool user_addIdToTempBuffer(char *id, codeType_e tp)
{
  if(!flash_exec_idle()) return false;
  uint8_t found=user_findTempID(id);
  if(found) return found==1;
  cellID_s cell;
  flash_exec_args_t args={NULL,FS_SYNC};
  if(!flash_exec_begin(FX_STORE,&args)) return false;
  bool ok=false;
  bool trial=app_trial_active();
  for(uint16_t tried=0;tried<mem[MB_TEMPID].size/32u;tried++) {
    // До подтверждения старый ELF должен видеть непрерывный линейный префикс.
    if(trial && (temp_next>=mem[MB_TEMPID].size/32u || temp_next!=device.tempIDsCount)) break;
    uint16_t current=temp_normalize(temp_next);
    uint32_t address=temp_address(current);
    if(!trial && (address==mem[MB_TEMPID].start || !(address&4095u)) && !temp_reclaim(address)) break;
    if(!mem_read(address,(uint8_t *)&cell,32)) break;
    bool blank=log_blank((const cellLog_s *)&cell);
    if(trial && !blank) break;
    if(!trial) temp_next=current+1;
    if(!blank) continue;
    memset(&cell,0,sizeof(cell));cell.index=0x8000u;memcpy(cell.ID,id,10);
    if(tp==CT_PHONE) cell.isPhone=1;
    else if(tp==CT_AUTO) cell.isAuto=1;
    else if(tp==CT_WIEGAND) cell.isWiegand=1;
    uint16_t epoch=current&~2047u;
    memcpy(cell.reserve,&epoch,2);
    uint16_t crc=crc16_calc((uint8_t *)&cell+2,22);
    uint32_t footer[2]={crc|((uint32_t)(uint16_t)~crc<<16),0x32444954u};
    memcpy((uint8_t *)&cell+24,footer,8);
    ok=mem_write(address+2,(uint8_t *)&cell+2,30) && mem_write(address,(uint8_t *)&cell,2);
    if(ok) { temp_next=current+1;device.tempIDsCount++;device.status.readyID=1; }
    break;
  }
  device.newAddressTempID=temp_address(temp_next);
  flash_exec_finish(ok?FX_OK:FX_ERROR);flash_exec_take_result();
  return ok;
}

// Сверить стартовую базу с меткой индекса и вернуть число живых ID.
uint16_t user_getNumbersCount(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[256];
  cellID_s *c;
  uint16_t cnt = 0, cell = 0, used = 0;
  uint32_t sum = 0;

  id_cells_end = 0;
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += sizeof(tbuffer)){
    if (!mem_read(adr, tbuffer, sizeof(tbuffer))) {
      /* Флеш не прочиталась: индекс не публикуем, поиск идёт по всей базе,
         чтобы ни один записанный ID не пропал из проверки доступа. */
      id_cells_end = (uint16_t)(mem[MB_ID].size / sizeof(cellID_s));
      id_used_bytes = UINT32_MAX;
      return 0;
    }
    // перебираем ячейки
    for (uint16_t a = 0; a < sizeof(tbuffer); a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      cell++;
      /* Последняя ячейка, где есть хоть один записанный байт: за ней флеш стёрта. */
      for (uint8_t b = 0; b < sizeof(cellID_s); b++)
        if (tbuffer[a + b] != 0xff) { used = cell; break; }
      // пустая ячейка
      if (c->index == 0xffff || c->reserve[9] == 255) continue;

      cnt++;
      id_cells_end = cell;
      sum ^= idindex_hash(cell, (const uint8_t*)c->ID);
    }
    wdt_reload();
  }
  id_used_bytes = (uint32_t)used * sizeof(cellID_s);
  idindex_scanned(sum, id_cells_end);

  return cnt;
}

// Записать событие окончания срока ID ровно один раз.
void user_checkIdTimeEnd(void){
expiry_runtime_begin();
}

// Восстанавливаем голову из поколений и ACK; старые линейные ID остаются на месте.
uint16_t user_getTempIDRecordsCount(void)
{
  cellID_s cell;
  uint16_t count=0;
  uint32_t end=mem[MB_TEMPID].start+mem[MB_TEMPID].size;
  temp_next=temp_read_max();
  device.status.readyID=0;
  for(uint32_t adr=mem[MB_TEMPID].start; adr<end; adr+=32) {
    if(!mem_read(adr,(uint8_t *)&cell,32)) return 0xffff;
    if(!user_temp_valid(&cell)) continue;
    uint16_t sequence=temp_sequence(&cell,adr);
    if((int16_t)((uint16_t)(sequence+1)-temp_next)>0) temp_next=sequence+1;
    if((int16_t)(sequence-device.readTempIDIndex)>=0) device.status.readyID=1;
    count++;
  }
  device.newAddressTempID=temp_address(temp_next);
  return count;
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
