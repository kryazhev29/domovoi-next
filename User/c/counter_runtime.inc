// EEPROM по схеме AN2594: данные, затем адрес; перенос последних значений.
// Адреса резервных страниц не пересекаются с базовыми областями 2 МиБ.
// Номера ячеек всех строк слоя продублированы в ОЗУ (ct_keys): поиск строки
// по номеру идёт без чтения флеш, читается только найденная строка.
#include "../counter.h"
#include "../id_crc_cache.h"
#define CT_PAGE0 0x1FB000u
#define CT_HEAD 12u
#define CT_ROWS ((4096u-CT_HEAD)/10u)
typedef struct { uint8_t value[8]; uint16_t key; } ct_row;
static struct { uint32_t page,generation,sector; uint16_t used,live; } ct;
/* Номер ячейки каждой строки страницы; 0xFFFF — оборванная или пустая строка. */
static uint16_t ct_keys[CT_ROWS];
bool counter_busy(void) { return ct.sector!=0; }
// Во время замены читается уже подтверждённый образ SJ, а не стираемый сектор.
bool counter_base_read(uint32_t address,uint8_t *data,uint16_t size)
{
  if (ct.sector && sjr.committed && sjr.phase>=SJ_PHASE_TARGET_ERASE &&
      (address&~4095u)==ct.sector) address=SJ_DATA+(address&4095u);
  return w25_read_data(address,data,size);
}
static uint32_t ct_address(uint16_t pos) { return ct.page+CT_HEAD+10u*pos; }
static bool ct_read(uint16_t pos,ct_row *row)
{
  return w25_read_data(ct_address(pos),(uint8_t *)row,sizeof(*row));
}
// Проверка номера относится только к данным, прочитанным с NOR.
static bool ct_key_valid(uint16_t key) { return key<mem[MB_ID].size/32u; }
// Последняя строка с этим номером: поиск в ОЗУ, чтение одной строки.
// -1 — ошибка NOR, 0 — нет, иначе позиция + 1.
static int ct_find(uint16_t key,ct_row *row)
{
  for (uint16_t n=ct.used;n;) {
    if (ct_keys[--n]!=key) continue;
    return ct_read(n,row) ? n+1 : -1;
  }
  return 0;
}
// Нет ли позже строки с тем же номером: действует только последняя.
static bool ct_last(uint16_t n)
{
  uint16_t key=ct_keys[n];
  for (uint16_t m=n+1;m<ct.used;m++) if (ct_keys[m]==key) return false;
  return true;
}
// Изменён ли логический остаток относительно базы; удалённая ячейка слоя не имеет.
static int ct_changed(const ct_row *row)
{
  uint8_t value[8],valid;
  uint32_t a=mem[MB_ID].start+32u*row->key;
  if (!counter_base_read(a+31,&valid,1) || !counter_base_read(a+21,value,8)) return -1;
  return valid!=255 && memcmp(value,row->value,8)!=0;
}
static bool ct_count(bool all)
{
  ct_row row;ct.live=0;
  for (uint16_t n=0;n<ct.used;n++) {
    if (!ct_key_valid(ct_keys[n]) || !ct_last(n)) continue;
    if (all) { ct.live++; continue; }
    if (!ct_read(n,&row)) return false;
    int changed=ct_changed(&row);
    if (changed<0) return false;
    ct.live+=changed;
  }
  return true;
}
int counter_contains(uint32_t address)
{
  uint16_t key=(uint16_t)((address-mem[MB_ID].start)/32u);
  for (uint16_t n=ct.used;n;) if (ct_keys[--n]==key) return n+1;
  return 0;
}
// Номер записывается последним: оборванное тело не подставляется в ячейку.
static bool ct_append(uint16_t pos,const ct_row *row)
{
  uint32_t address=ct_address(pos);
  ct_keys[pos]=0xffffu;
  if (!w25_write_data(address,(uint8_t *)row->value,8) ||
      !w25_write_data(address+8,(uint8_t *)&row->key,2)) return false;
  ct_keys[pos]=row->key;
  return true;
}
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size)
{
  if (!ct.page || address<mem[MB_ID].start || address>=mem[MB_ID].start+mem[MB_ID].size) return true;
  while (size) {
    uint16_t off=(address-mem[MB_ID].start)%32u,n=32u-off;
    ct_row row;
    if (n>size) n=size;
    int found=ct_find((uint16_t)((address-mem[MB_ID].start)/32u),&row);
    if (found<0) return false;
    uint8_t valid=255;
    if (found && !counter_base_read(address-off+31,&valid,1)) return false;
    if (found && valid!=255) for (uint16_t i=0;i<n;i++)
      if (off+i>=21 && off+i<29) data[i]=row.value[off+i-21];
    address+=n;data+=n;size-=n;
  }
  return true;
}
static uint8_t ct_chunk(const void *context,uint16_t offset,uint8_t *data)
{
  uint32_t address=*(const uint32_t *)context+offset;
  return w25_read_data(address,data,64) && counter_overlay(address,data,64);
}
// Поколение с инверсией проверяется один раз при старте: прерванное стирание
// прежнего заголовка не может превратить его в более новое поколение.
// Таблица номеров уплотняется на месте: count не обгоняет n.
static bool ct_transfer(uint32_t replaced)
{
  uint32_t target=(2u*CT_PAGE0+4096u)-ct.page,h[3]={ct.generation+1u,~(ct.generation+1u),0xffffffffu};
  uint16_t count=0,bytes=0;
  ct_row row;
  if (!w25_write_data(target,(uint8_t *)h,8)) return false;
  for (uint16_t n=0;n<ct.used;n++) {
    uint16_t key=ct_keys[n];
    if (!ct_key_valid(key) || !ct_last(n) || ((mem[MB_ID].start+32u*key)&~4095u)==replaced) continue;
    if (!ct_read(n,&row)) return false;
    int changed=ct.sector ? 1 : ct_changed(&row);
    if (changed<0) return false;
    if (changed) {
      // Пока заголовок RECEIVE, отдельные строки ещё не видны: пишем пачкой.
      memcpy(sjr.data+bytes,&row,10);bytes+=10;ct_keys[count++]=key;
      if (bytes==60) {
        if (!w25_write_data(target+CT_HEAD+10u*count-bytes,sjr.data,bytes)) return false;
        bytes=0;
      }
    }
  }
  if (bytes && !w25_write_data(target+CT_HEAD+10u*count-bytes,sjr.data,bytes)) return false;
  h[2]=0;
  if (!w25_write_data(target+8,(uint8_t *)&h[2],4)) return false;
  uint32_t old=ct.page;
  ct.page=target;ct.generation=h[0];ct.used=ct.live=count;
  return w25_erase_sector(old);
}
static bool ct_load(void)
{
  uint32_t h[3];
  memset(&ct,0,sizeof(ct));
  for (uint32_t page=CT_PAGE0;page<CT_PAGE0+8192u;page+=4096u) {
    if (!w25_read_data(page,(uint8_t *)h,sizeof(h))) return false;
    if (!h[2] && h[1]==~h[0] && (!ct.page || (int32_t)(h[0]-ct.generation)>0)) {
      ct.page=page;ct.generation=h[0];
    }
  }
  if (!ct.page) return true;
  // Только невыбранная страница: незавершённый RECEIVE или остаток старой VALID.
  uint32_t other=(2u*CT_PAGE0+4096u)-ct.page;
  for (uint16_t off=0;off<4096u;off+=64) {
    uint8_t b[64];
    if (!w25_read_data(other+off,b,sizeof(b))) return false;
    if (!fast_all_ff(b,sizeof(b))) {
      if (!w25_erase_sector(other)) return false;
      break;
    }
  }
  for (uint16_t n=0;n<CT_ROWS;n++) {
    ct_row row;
    if (!ct_read(n,&row)) return false;
    ct_keys[n]=row.key;
    if (!fast_all_ff((uint8_t *)&row,sizeof(row))) ct.used=n+1;
  }
  return ct_count(false);
}
bool counter_boot(void)
{
  if (!ct_load()) return false;
  if (ct.page) return true;
  ct.page=CT_PAGE0;
  uint32_t h[3]={0,0xffffffffu,0};
  return w25_erase_sector(CT_PAGE0+4096u) && w25_erase_sector(ct.page) && w25_write_data(ct.page,(uint8_t *)h,sizeof(h));
}
// SJ уже опубликовал базу, но ещё хранит восстановимый образ. Перед очисткой
// его метки обновляем только затронутые адреса; повтор после сброса безопасен.
bool counter_replaced(uint32_t address)
{
  if (address<mem[MB_ID].start || address>=mem[MB_ID].start+mem[MB_ID].size) return true;
  uint32_t marker;
  if (!w25_read_data(SJ_META+32,(uint8_t *)&marker,4)) return false;
  // Фоновый снимок мог отстать от новых проходов: их записи сохраняются.
  if (marker==SJ_COMMIT) return true;
  if (!ct.page && !ct_load()) return false;
  if (!ct.used) return true;
  idcrc_unknown();
  uint16_t end=ct.used;
  ct_row row,last;
  for (uint16_t n=0;n<end;n++) {
    uint16_t key=ct_keys[n];
    if (!ct_key_valid(key) || ((mem[MB_ID].start+32u*key)&~4095u)!=address || !ct_last(n)) continue;
    if (!ct_read(n,&row)) return false;
    if (!w25_read_data(mem[MB_ID].start+32u*key+21,last.value,8)) return false;
    if (!memcmp(row.value,last.value,8)) continue;
    if (ct.used==CT_ROWS) return ct_transfer(address);
    memcpy(row.value,last.value,8);
    if (!ct_append(ct.used++,&row)) return false;
  }
  return ct_count(false);
}
bool counter_pending(void) { return ct.live>CT_ROWS*3u/4u; }
// Один сектор на фоновую транзакцию; более новые состояния остаются в слое.
bool counter_drain_begin(void)
{
  ct_row row;
  for (uint16_t n=ct.used;n;) {
    --n;
    if (!ct_key_valid(ct_keys[n]) || !ct_last(n)) continue;
    if (!ct_read(n,&row)) return false;
    int changed=ct_changed(&row);
    if (changed<0) return false;
    if (!changed) continue;
    ct.sector=(mem[MB_ID].start+32u*row.key)&~4095u;
    idcrc_unknown();
    if (!ct_count(true)) return false;
    return sj_runtime_begin(ct.sector,ct_chunk,&ct.sector);
  }
  return false;
}
// Исполнитель вызывает фон только при свободном NOR; занятая команда завершается
// здесь, а между командами STORE может дописать счётчик в отдельные страницы.
void counter_poll(uint32_t now)
{
  uint8_t result=sj_runtime_poll(now);
  if (result==1) return;
  sj_runtime_reset();ct.sector=0;
  if (result!=2 || !ct_count(false)) mem_storage_ready=false;
}
// 0 — замена ID; 1 — записано; 2 — нужна одна фоновая SJ; -1 — ошибка NOR.
int counter_write(uint32_t address,const cellID_s *cell)
{
  uint8_t old[32];ct_row row;
  if (!ct.page && !counter_boot()) return -1;
  if (!counter_base_read(address,old,32)) return -1;
  if (memcmp(old,cell,21) || memcmp(old+29,(const uint8_t *)cell+29,3)) return 0;
  int found=ct_find((uint16_t)((address-mem[MB_ID].start)/32u),&row);
  if (found<0) return -1;
  if (found) memcpy(old+21,row.value,8);
  if (!memcmp(old+21,(const uint8_t *)cell+21,8)) return 1;
  if (ct.used==CT_ROWS) {
    if (ct.live==CT_ROWS) return 2;
    if (!ct_transfer(0)) return -1;
  }
  // После переноса база не меняется; сравниваем прежнее и новое состояния с ней.
  if (ct.sector) ct.live+=!found;
  else {
    if (!w25_read_data(address+21,row.value,8)) return -1;
    ct.live+=(memcmp(row.value,(const uint8_t *)cell+21,8)!=0)-(memcmp(row.value,old+21,8)!=0);
  }
  memcpy(row.value,(const uint8_t *)cell+21,8);
  row.key=(uint16_t)((address-mem[MB_ID].start)/32u);
  if (!ct_append(ct.used++,&row)) return -1;
  idcrc_counter(address,old+21,row.value);
  return 1;
}
