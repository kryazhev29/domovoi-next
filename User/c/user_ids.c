// Доработка: Кряжев А., 2026
// Основные и временные ID: поиск, счётчики проходов и сроки действия.
// Индекс ускоряет поиск; во время пересборки остаётся перебор блоками 256 Б.
// Стартовый скан сверяет метку индекса с базой; ошибка чтения отличается от отсутствия ID.
#include "user_logic_internal.h"
#include "fast_kernels.h"
#include "../../Bootloader/app_update.h"
uint32_t id_used_bytes = UINT32_MAX;
uint16_t id_cells_end;
uint16_t id_scan_cell;
static struct { uint32_t sum; uint16_t end, used; } id_scan;
// Один продолжаемый поиск; результат хранит адрес, а не копию счётчиков.
static struct { uint32_t seen; char id[10]; uint16_t next, hit; uint8_t state; } lookup;
RT_ASSERT(id_lookup_ram, sizeof(lookup)==20);
void user_id_lookup_reset(void) { lookup.state=0; }
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
uint16_t id_today(void){
  return device.datetime >= ID_TIME_VALID ? (uint16_t)(device.datetime / ONE_DAY) : 0;
}

// Прочитать сутки последнего прохода из записи ID.
static uint16_t id_pass_day(cellID_s *c){
  uint16_t d;
  memcpy(&d, &c->reserve[4], 2);
  return d == 0xffff ? 0 : d;
}

// Записать сутки последнего прохода в запись ID.
void id_set_pass_day(cellID_s *c, uint16_t day){
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

// 0 — нет, 1 — найден, 2 — ожидание или ошибка чтения. Не более 128 ID за вызов.
uint8_t user_findID(char *id, cellID_s *cell)
{
  uint16_t sum=0;
  for (uint8_t i=0;i<10;i++) sum+=(uint8_t)id[i];
  if (!sum || sum==2550) return 0;
  uint32_t now=timers_getMillis();
  if (!lookup.state || !id_equal(lookup.id,id)) {
    // Другой источник не сбивает текущий поиск; отменённый запрос живёт ещё до 100 мс.
    if (lookup.state==1 && now-lookup.seen<100u) return 2;
    memcpy(lookup.id,id,10); lookup.next=lookup.hit=0; lookup.state=1;
    int32_t r=idindex_find(mem[MB_ID].start,(const uint8_t*)id,(uint8_t*)cell);
    if (r>=0) {
      lookup.state=2; lookup.hit=(uint16_t)r;
      return r!=0;
    }
    if (r==-2) { lookup.state=0; return 2; }
  }
  lookup.seen=now;
  if (lookup.state==2) {
    if (!lookup.hit) return 0;
    if (!mem_read(mem[MB_ID].start+(uint32_t)(lookup.hit-1u)*sizeof(*cell),
                  (uint8_t*)cell,sizeof(*cell))) return 2;
    if (cell->index!=0xffff && cell->reserve[9]!=255 && id_equal(cell->ID,id)) return 1;
    lookup.next=lookup.hit=0; lookup.state=1;
  }
  uint8_t page[256];
  for (uint8_t step=0;step<16 && lookup.next<id_cells_end;step++) {
    uint16_t count=id_cells_end-lookup.next;
    if (count>8) count=8;
    uint32_t address=mem[MB_ID].start+(uint32_t)lookup.next*sizeof(*cell);
    // Поиск не читает счётчики каждого чужого ID: они не меняют номер и признак удаления.
    // counter_base_read сохраняет чтение подтверждённого SJ при фоновом переносе.
    if (!counter_base_read(address,page,count*sizeof(*cell))) return 2;
    for (uint8_t i=0;i<count;i++) {
      cellID_s *c=(cellID_s*)(page+i*sizeof(*cell));
      if (c->index==0xffff || c->reserve[9]==255 || !id_equal(c->ID,id)) continue;
      lookup.hit=lookup.next+i+1u; lookup.state=2;
      memcpy(cell,c,sizeof(*cell));
      return counter_overlay(address+i*sizeof(*cell),(uint8_t*)cell,sizeof(*cell)) ? 1 : 2;
    }
    lookup.next+=count;
  }
  if (lookup.next<id_cells_end) return 2;
  lookup.state=2;
  return 0;
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
// Временный ID содержит поколение и CRC; index фиксируется после тела.
// Любой сброшенный бит commit достаточен: тело уже проверено, старый ELF тоже видит этот ID.
bool user_temp_valid(const cellID_s *cell)
{
  const uint32_t *words=(const uint32_t *)cell;
  if(cell->index==0xffff) return false;
  uint16_t crc=(uint16_t)words[6];
  return words[7]==0x32444954u &&
    crc==(uint16_t)~(words[6]>>16) && crc==crc16_calc((uint8_t *)cell+2,22);
}
// Поколение проверенной записи и позиция образуют курсор кольца.
uint16_t temp_sequence(const cellID_s *cell,uint32_t address)
{
  uint16_t epoch;
  memcpy(&epoch,cell->reserve,2);
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
  for(uint16_t tried=0;tried<mem[MB_TEMPID].size/32u;tried++) {
    uint16_t current=temp_normalize(temp_next);
    uint32_t address=temp_address(current);
    if((address==mem[MB_TEMPID].start || !(address&4095u)) && !temp_reclaim(address)) break;
    if(!mem_read(address,(uint8_t *)&cell,32)) break;
    bool blank=log_blank((const cellLog_s *)&cell);
    temp_next=current+1;
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

// До полного прохода граница поиска охватывает всю базу, записи ждут.
void user_ids_boot(void){
  id_cells_end = (uint16_t)(mem[MB_ID].size / sizeof(cellID_s));
  id_used_bytes = UINT32_MAX;
  memset(&id_scan, 0, sizeof(id_scan));
  device.numbersCount = 0;
  id_scan_cell = 1;
  user_id_lookup_reset();
}

// Один шаг исполнителя: восемь ячеек; при ошибке повторяем ту же страницу.
void user_ids_scan(void){
  uint32_t offset = (uint32_t)(id_scan_cell - 1u) * sizeof(cellID_s);
  uint8_t tbuffer[256];
  cellID_s *c;
  uint16_t n = (uint16_t)(mem[MB_ID].size - offset > sizeof(tbuffer) ?
                         sizeof(tbuffer) : mem[MB_ID].size - offset);
  /* Слой счётчиков меняет только байты 21..28: номер, ID и признак читаем из базы. */
  if (!counter_base_read(mem[MB_ID].start + offset, tbuffer, n)) {
    flash_exec_finish(FX_ERROR);
    return;
  }
  for (uint16_t a = 0; a < n; a += sizeof(cellID_s)){
    c = (cellID_s*)&tbuffer[a];
    /* Последняя ячейка, где есть хоть один записанный байт: за ней флеш стёрта. */
    if (!fast_all_ff(&tbuffer[a], sizeof(cellID_s))) id_scan.used = id_scan_cell;
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

// Восстанавливаем голову из поколений целых записей и ACK.
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

// Записать изменённые параметры ID на место, найденное user_findID.
// Вызывать только в том же проходе после user_findID(c->ID)==1: ответ 1 всегда оставляет
// lookup.hit на этой ячейке, а до записи меняются лишь счётчики (23.10.2026).
bool user_updateID(cellID_s *c){
  if (!flash_exec_idle()) return false; // занятая флеш не сбрасывает кэш CRC
  bool result = store_id_begin(mem[MB_ID].start+(uint32_t)(lookup.hit-1u)*sizeof(*c), c);
  if(!result) idcrc_unknown();
  return result;
}
