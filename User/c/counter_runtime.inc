// Слой точных байтов 21..28 ячейки ID. Освобождение только после переноса в базу.
// Восемь резервных секторов ниже 2 МиБ; в конце каждого — намерение стирания соседа.
#include "../counter.h"
#include "../sector_journal.h"
#include "../fast_kernels.h"
#include "../../Bootloader/app_update.h"
enum { CT_SECTORS=8, CT_PER_SECTOR=170, CT_ROWS=1360, CT_LIMIT=1024, CT_ACTIVE=0x8000 };
static const uint32_t ct_areas[CT_SECTORS]={
  0x1C8000u,0x1CB000u,0x1CF000u,0x1FB000u,0x1FC000u,0x1FD000u,0x1FE000u,0x1FF000u
};
typedef struct {
  uint16_t key,link;
  uint32_t base;
  uint8_t value[8];
  uint32_t sequence,commit;
} ct_row;
RT_ASSERT(counter_row_size,sizeof(ct_row)==24);
static struct {
  uint16_t heads[256],dirty,used,delta;
  uint32_t sequence,intent_sequence,fallbacks;
  uint8_t initialized,intent_sector;
} ct;
enum { CT_SELECT=1, CT_GC, CT_OLDEST, CT_FOLD, CT_GC_FOLD,
       CT_WRITE, CT_ERASE, CT_ERASE_WAIT, CT_DONE, CT_ERROR };
static struct {
  ct_row pending;
  uint32_t address,oldest,fold;
  uint16_t scan,delta;
  uint8_t phase,waiting,offset,chunk,kind,victim,self;
  int8_t change;
} cj;
static struct {
  uint32_t sector;
  uint16_t scan,delta,count;
  uint8_t phase,mark;
} cf;
static uint32_t ct_address(uint16_t row)
{
  return ct_areas[row/CT_PER_SECTOR]+24u*(row%CT_PER_SECTOR);
}
static uint8_t ct_bucket(uint16_t key) { return (uint8_t)(key^(key>>8)); }
static uint32_t ct_commit(ct_row *r)
{
  uint16_t link=r->link;
  r->link|=CT_ACTIVE;
  uint16_t crc=crc16_calcInit((uint8_t *)r,20,65535);
  r->link=link;
  return crc|((uint32_t)(uint16_t)~crc<<16);
}
static bool ct_valid(ct_row *r,uint16_t pos)
{
  return r->key && r->key<=mem[MB_ID].size/32u && r->sequence &&
    (r->link&~CT_ACTIVE)<=CT_ROWS && r->commit==ct_commit(r) &&
    !(pos/CT_PER_SECTOR==ct.intent_sector && r->sequence<=ct.intent_sequence);
}
static bool ct_read(uint16_t pos,ct_row *r)
{
  return w25_read_data(ct_address(pos),(uint8_t *)r,sizeof(*r));
}
static uint32_t ct_cell_address(uint16_t key)
{
  return mem[MB_ID].start+32u*(key-1u);
}
static uint32_t ct_fingerprint(const uint8_t *cell)
{
  return ~crc32_update(0xffffffffu,cell,32);
}
// Ссылки идут только назад по номеру записи; повторно занятый адрес завершает цепь.
static int ct_find(uint16_t key,const uint8_t *cell,ct_row *result)
{
  uint16_t pos=ct.heads[ct_bucket(key)];
  uint32_t newer=0xffffffffu;
  ct_row row;
  while (pos) {
    if (!ct_read(pos-1,&row)) return -1;
    if (!ct_valid(&row,pos-1) || row.sequence>=newer) return 0;
    if (row.key==key) {
      if (!(row.link&CT_ACTIVE) || row.base!=ct_fingerprint(cell)) return 0;
      if (result) *result=row;
      return pos;
    }
    newer=row.sequence;
    pos=row.link&~CT_ACTIVE;
  }
  return 0;
}
static uint16_t ct_difference(uint32_t address,const uint8_t *old,const uint8_t *value)
{
  uint16_t d=crc16_calcInit((uint8_t *)old,8,0)^crc16_calcInit((uint8_t *)value,8,0);
  uint32_t tail=mem[MB_ID].size-(address-mem[MB_ID].start)-29u;
  return crc16_ffInit(tail,d)^crc16_ffInit(tail,0);
}
// Возвращает 1 лишь для последней применимой записи, 0 для мусора, -1 при отказе NOR.
static int ct_live(uint16_t pos,ct_row *row)
{
  uint8_t base[32];
  if (!ct_valid(row,pos) || !(row->link&CT_ACTIVE)) return 0;
  if (!w25_read_data(ct_cell_address(row->key),base,32)) return -1;
  int found=ct_find(row->key,base,0);
  return found<0 ? -1 : found==(int)pos+1 && memcmp(base+21,row->value,8)!=0;
}
bool counter_boot(void)
{
  memset(&ct,0,sizeof(ct));
  ct.intent_sector=255;
  // Последнее намерение исключает даже заголовки, восстановленные оборванным erase.
  for (uint8_t s=0;s<CT_SECTORS;s++) {
    uint32_t h[4];
    if (!w25_read_data(ct_areas[s]+4080,(uint8_t *)h,16)) return false;
    if (!fast_all_ff((uint8_t *)h,16)) ct.dirty|=1u<<s;
    uint16_t crc=crc16_calcInit((uint8_t *)h,12,65535);
    if (h[0]==0x31454349u && h[2]<CT_SECTORS &&
        h[3]==(crc|((uint32_t)(uint16_t)~crc<<16)) && h[1]>=ct.intent_sequence) {
      ct.intent_sequence=h[1]; ct.intent_sector=(uint8_t)h[2];
    }
  }
  for (uint16_t pos=0;pos<CT_ROWS;pos++) {
    ct_row row,head;
    if (!ct_read(pos,&row)) return false;
    if (!fast_all_ff((uint8_t *)&row,24)) ct.dirty|=1u<<(pos/CT_PER_SECTOR);
    if (!ct_valid(&row,pos)) continue;
    if (row.sequence>ct.sequence) ct.sequence=row.sequence;
    uint16_t *bucket=&ct.heads[ct_bucket(row.key)];
    if (*bucket && !ct_read(*bucket-1,&head)) return false;
    if (!*bucket || row.sequence>head.sequence) *bucket=pos+1;
    wdt_reload();
  }
  for (uint16_t pos=0;pos<CT_ROWS;pos++) {
    ct_row row; uint8_t base[32];
    if (!ct_read(pos,&row)) return false;
    int live=ct_live(pos,&row);
    if (live<0) return false;
    if (!live) continue;
    if (!w25_read_data(ct_cell_address(row.key),base,32)) return false;
    ct.delta^=ct_difference(ct_cell_address(row.key),base+21,row.value);
    ct.used++;
    wdt_reload();
  }
  ct.initialized=1;
  return true;
}
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size)
{
  if (!ct.initialized || !ct.used) return true;
  uint32_t begin=mem[MB_ID].start,end=begin+mem[MB_ID].size;
  if (address<begin || address>=end || size>end-address) return true;
  uint32_t cell=address&~31u;
  for (;cell<address+size;cell+=32u) {
    uint32_t lo=cell+21u,hi=cell+29u;
    if (lo<address) lo=address;
    if (hi>address+size) hi=address+size;
    if (lo>=hi) continue;
    uint8_t scratch[32]; ct_row row;
    uint8_t *base;
    if (cell>=address && cell+32u<=address+size) base=data+cell-address;
    else { if (!w25_read_data(cell,scratch,32)) return false; base=scratch; }
    int found=ct_find((uint16_t)((cell-begin)/32u+1u),base,&row);
    if (found<0) return false;
    if (found) memcpy(data+lo-address,row.value+lo-cell-21u,hi-lo);
  }
  return true;
}
uint16_t counter_crc_delta(void) { return ct.delta; }
uint32_t counter_turns(void) { return ct.sequence ? (ct.sequence-1u)/CT_ROWS : 0; }
uint32_t counter_fallbacks(void) { return ct.fallbacks; }
// Образ объединяет все изменённые ID сектора; один SJ вместо транзакции на ячейку.
static uint8_t ct_page(const void *context,uint16_t offset,uint8_t *data)
{
  (void)context;
  uint32_t address=cf.sector+offset;
  if (!w25_read_data(address,data,64)) return 0;
  uint16_t before=crc16_calcInit(data,64,0);
  for (uint8_t n=0;n<64 && ct.used;n+=32) {
    ct_row row;
    int found=ct_find((uint16_t)((address+n-mem[MB_ID].start)/32u+1u),data+n,&row);
    if (found<0) return 0;
    if (found && memcmp(data+n+21,row.value,8)) {
      if (sjr.phase==SJ_PHASE_COMPARE) cf.count++;
      memcpy(data+n+21,row.value,8);
    }
  }
  if (sjr.phase==SJ_PHASE_COMPARE) {
    uint16_t d=before^crc16_calcInit(data,64,0);
    uint32_t tail=mem[MB_ID].size-(address-mem[MB_ID].start)-64u;
    cf.delta^=crc16_ffInit(tail,d)^crc16_ffInit(tail,0);
  }
  return 1;
}
uint8_t counter_prepare(uint32_t sector,uint32_t now)
{
  if (!ct.initialized || !ct.dirty || sector<mem[MB_ID].start ||
      sector>=mem[MB_ID].start+mem[MB_ID].size) return 2;
  if (!cf.phase) {
    cf.sector=sector; cf.scan=cf.count=cf.delta=0;
    if (!sj_runtime_begin(sector,ct_page,0)) return 3;
    idcrc_unknown(); cf.phase=1;
    return 1;
  }
  if (cf.phase==1) {
    uint8_t r=sj_runtime_poll(now);
    if (r==1) return 1;
    sj_runtime_reset();
    if (r!=2) { cf.phase=0; return 3; }
    ct.delta^=cf.delta; ct.used-=cf.count; cf.phase=2;
  }
  if (cf.phase==3) {
    uint8_t r=w25_async_poll(now);
    if (r==1) return 1;
    if (r!=2) { cf.phase=0; return 3; }
    cf.phase=2;
    return 1;
  }
  if (cf.scan==CT_ROWS) { cf.phase=0; return 2; }
  uint16_t pos=cf.scan++;
  ct_row row;
  if (!ct_read(pos,&row)) { cf.phase=0; return 3; }
  if (ct_valid(&row,pos) && (row.link&CT_ACTIVE) &&
      (ct_cell_address(row.key)&~4095u)==sector) {
    cf.mark=(uint8_t)(row.link>>8)&0x7fu;
    if (!w25_async_start(ct_address(pos)+3,&cf.mark,1,now)) { cf.phase=0; return 3; }
    cf.phase=3;
  }
  return 1;
}
// 0 оставляет прежний путь (проба или изменение других байтов); -1 — отказ чтения.
int counter_begin(uint32_t address,const cellID_s *cell)
{
  if (!ct.initialized || app_trial_active()) return 0;
  uint8_t base[32],old[32];
  if (!w25_read_data(address,base,32)) return -1;
  if (memcmp(base,cell,21) || memcmp(base+29,(const uint8_t *)cell+29,3)) return 0;
  memcpy(old,base,32);
  if (!counter_overlay(address,old,32)) return -1;
  memset(&cj,0,sizeof(cj));
  cj.pending.key=(uint16_t)((address-mem[MB_ID].start)/32u+1u);
  cj.pending.base=ct_fingerprint(base);
  memcpy(cj.pending.value,(const uint8_t *)cell+21,8);
  cj.delta=ct_difference(address,old+21,cj.pending.value);
  if (!memcmp(old+21,cj.pending.value,8)) { cj.phase=CT_DONE; return 1; }
  int found=ct_find(cj.pending.key,base,0);
  if (found<0) return -1;
  cj.change=!found && memcmp(base+21,cj.pending.value,8)!=0;
  cj.phase=CT_SELECT;
  if (cj.change && ct.used>=CT_LIMIT) {
    cj.phase=CT_OLDEST; cj.oldest=0xffffffffu; ct.fallbacks++;
  }
  return 1;
}
// Запись тела и commit используют тот же асинхронный W25; источник живёт до verify.
static void ct_write_begin(uint8_t kind,uint32_t address)
{
  cj.kind=kind; cj.address=address; cj.offset=cj.waiting=0; cj.phase=CT_WRITE;
}
static void ct_row_begin(ct_row *row,uint8_t kind)
{
  uint16_t pos=(uint16_t)(ct.sequence%CT_ROWS);
  row->sequence=++ct.sequence;
  row->link=CT_ACTIVE|ct.heads[ct_bucket(row->key)];
  row->commit=ct_commit(row);
  ct_write_begin(kind,ct_address(pos));
}
static void ct_intent(void)
{
  uint16_t head=(uint16_t)(ct.sequence%CT_ROWS);
  uint8_t source=(uint8_t)((head ? head-1u : CT_ROWS-1u)/CT_PER_SECTOR);
  if (head%CT_PER_SECTOR) source=(uint8_t)(head/CT_PER_SECTOR);
  uint32_t *h=(uint32_t *)sjr.data;
  h[0]=0x31454349u; h[1]=ct.sequence; h[2]=cj.victim;
  uint16_t crc=crc16_calcInit((uint8_t *)h,12,65535);
  h[3]=crc|((uint32_t)(uint16_t)~crc<<16);
  ct_write_begin(2,ct_areas[source]+4080u);
}
uint8_t counter_poll(uint32_t now)
{
  ct_row row;
  uint16_t head=(uint16_t)(ct.sequence%CT_ROWS);
  uint8_t sector=(uint8_t)(head/CT_PER_SECTOR);
  switch (cj.phase) {
  case CT_OLDEST: {
    if (cj.scan==CT_ROWS) { cj.phase=cj.oldest==0xffffffffu ? CT_ERROR : CT_FOLD; break; }
    uint16_t pos=cj.scan++;
    if (!ct_read(pos,&row)) goto failed;
    int live=ct_live(pos,&row);
    if (live<0) goto failed;
    if (live && row.sequence<cj.oldest) {
      cj.oldest=row.sequence; cj.fold=ct_cell_address(row.key)&~4095u;
    }
    break;
  }
  case CT_FOLD:
  case CT_GC_FOLD: {
    uint8_t r=counter_prepare(cj.fold,now);
    if (r==1) break;
    if (r!=2) goto failed;
    cj.phase=cj.phase==CT_FOLD ? CT_SELECT : CT_GC;
    break;
  }
  case CT_SELECT:
    if (!(head%CT_PER_SECTOR) && (ct.dirty&(1u<<sector))) {
      cj.victim=sector; cj.self=1; cj.scan=0; cj.phase=CT_GC; break;
    }
    if (!ct_read(head,&row)) goto failed;
    if (!fast_all_ff((uint8_t *)&row,24)) {
      ct.sequence++; break; // Не переиспользуем оборванное тело.
    }
    cj.victim=(uint8_t)((sector+1u)%CT_SECTORS);
    if (ct.dirty&(1u<<cj.victim)) {
      cj.self=0; cj.scan=0; cj.phase=CT_GC; break;
    }
    {
      uint8_t base[32];
      if (!w25_read_data(ct_cell_address(cj.pending.key),base,32)) goto failed;
      ct_row previous;
      int found=ct_find(cj.pending.key,base,&previous);
      if (found<0) goto failed;
      cj.pending.base=ct_fingerprint(base);
      cj.change=(memcmp(base+21,cj.pending.value,8)!=0) -
        (found && memcmp(base+21,previous.value,8)!=0);
    }
    ct_row_begin(&cj.pending,0);
    break;
  case CT_GC: {
    if (cj.scan==CT_PER_SECTOR) {
      if (!ct.sequence) cj.phase=CT_ERASE;
      else ct_intent();
      break;
    }
    uint16_t pos=(uint16_t)(cj.victim*CT_PER_SECTOR+cj.scan);
    if (!ct_read(pos,&row)) goto failed;
    int live=ct_live(pos,&row);
    if (live<0) goto failed;
    if (!live) { cj.scan++; break; }
    if (cj.self || sector==cj.victim) {
      // Оборванные тела могут съесть запас: переносим целый сектор базы, не теряя ID.
      cj.fold=ct_cell_address(row.key)&~4095u; cj.phase=CT_GC_FOLD; ct.fallbacks++; break;
    }
    ct_row blank;
    if (!ct_read(head,&blank)) goto failed;
    if (!fast_all_ff((uint8_t *)&blank,24)) { ct.sequence++; return 1; }
    memcpy(sjr.data,&row,24); cj.scan++;
    ct_row_begin((ct_row *)sjr.data,1);
    break;
  }
  case CT_WRITE: {
    uint8_t *data=cj.kind ? sjr.data : (uint8_t *)&cj.pending;
    uint8_t body=cj.kind==2 ? 12 : 20;
    if (cj.waiting) {
      uint8_t r=w25_async_poll(now);
      if (r==1) break;
      if (r!=2) goto failed;
      cj.waiting=0; cj.offset+=cj.chunk;
      return 1;
    }
    if (cj.offset==body+4u) {
      if (cj.kind==2) {
        ct.intent_sequence=((uint32_t *)data)[1]; ct.intent_sector=cj.victim;
        cj.phase=CT_ERASE;
      } else {
        ct_row *done=(ct_row *)data;
        uint16_t pos=(uint16_t)((done->sequence-1u)%CT_ROWS);
        ct.heads[ct_bucket(done->key)]=pos+1;
        ct.dirty|=1u<<(pos/CT_PER_SECTOR);
        if (!cj.kind) { ct.delta^=cj.delta; ct.used+=cj.change; cj.phase=CT_DONE; }
        else cj.phase=CT_GC;
      }
      break;
    }
    uint16_t page=256u-((cj.address+cj.offset)&255u);
    cj.chunk=cj.offset<body ? body-cj.offset : 4;
    if (cj.chunk>page) cj.chunk=(uint8_t)page;
    if (!w25_async_start(cj.address+cj.offset,data+cj.offset,cj.chunk,now)) goto failed;
    cj.waiting=1;
    break;
  }
  case CT_ERASE:
    if (!w25_async_start(ct_areas[cj.victim],0,0,now)) goto failed;
    cj.phase=CT_ERASE_WAIT;
    break;
  case CT_ERASE_WAIT: {
    uint8_t r=w25_async_poll(now);
    if (r==1) break;
    if (r!=2) goto failed;
    ct.dirty&=~(1u<<cj.victim); cj.phase=CT_SELECT;
    break;
  }
  case CT_DONE: return 2;
  default: goto failed;
  }
  return 1;
failed:
  cj.phase=CT_ERROR; mem_storage_ready=false; return 3;
}
