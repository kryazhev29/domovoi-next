// Доработка: Кряжев А., 2026
// Фоновая проверка сроков ID, чтение базы по 64 Б.
// Запись завершённого прохода выполняется через журнал секторов.
// Рабочая память и флеш освобождаются при отмене, ошибке или завершении.
#include "runtime_internal.h"
enum {
  EXP_SCAN=1, EXP_DIRECT=2, EXP_SEARCH=3,
  EXP_START=4, EXP_JOURNAL=5
};
extern uint8_t id_endScan;
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(const cellID_s *);
extern void id_watch_end(uint32_t);
typedef struct {
  uint32_t scan,used_limit,start,target,find,partition_limit;
  uint8_t cancelled;
  uint32_t scan_time;
  uint8_t record[32];
  uint32_t snapshot_base;
  uint8_t snapshot[64];
} expiry_s;
RT_ASSERT(expiry_size,sizeof(expiry_s)==132);
static expiry_s *expiry_current(void)
{
  return flash_job.kind == FX_EXPIRY && flash_job.result == FX_PENDING ? flash_job.data.context : NULL;
}
// Запрашивает отмену проверки сроков: CRC базы больше не известна.
void expiry_runtime_cancel(void)
{
  expiry_s *p = expiry_current();
  if (p) {
    idcrc_unknown();
    p->cancelled = 1;
  }
}
// Освобождает флеш и память завершённой проверки.
static void expiry_release(flash_result_t result)
{
  work_release(expiry_current(), WORK_EXPIRY);
  flash_exec_finish(result);
}
// Начинает проход при наступлении ближайшего срока ID.
bool expiry_runtime_begin(void)
{
  uint32_t now,start,size,end,used;
  expiry_s *p;
  if(id_scan_cell || !flash_exec_idle() || !flash_exec_read_allowed()) return false;
  now=device.datetime;
  if(now<1600000000u) return false;
  if(!id_endScan && (!id_nextEnd || now<id_nextEnd)) return false;
  start=mem[MB_ID].start;
  size=mem[MB_ID].size;
  end=start+size;
  used=32u*id_cells_end;
  p=work_acquire(sizeof *p, WORK_EXPIRY);
  if(!p) { id_endScan=1;
    return false;
  }
  p->scan=p->start=start;
  p->snapshot_base=start-sizeof(p->snapshot);
  p->used_limit=start+used;
  p->partition_limit=end;
  p->scan_time=now;
  p->cancelled=0;
  flash_exec_args_t args = { p, EXP_SCAN };
  if (!flash_exec_begin(FX_EXPIRY, &args)) {
    work_release(p, WORK_EXPIRY);
    return false;
  }
  id_endScan = 0;
  id_nextEnd = 0;
  return true;
}
// Подставляет обновлённую запись ID в страницу журналируемого сектора.
static uint8_t expiry_chunk(const void *context,uint16_t offset,uint8_t *data)
{
  const expiry_s *p=context;
  uint32_t address=(p->target&~4095u)+offset,diff=p->target-address;
  if(!w25_read_data(address,data,64)) return 0;
  if(diff<=32) memcpy(data+diff,p->record,32);
  return 1;
}
// Читает два ID за фрагмент и журналирует завершённый проход.
void expiry_runtime_poll(uint32_t now)
{
  expiry_s *p=expiry_current();
  uint8_t scratch[64],*record;
  uint16_t index,got;
  uint32_t address,n,end;
  uint8_t result;
  if(!p) return;
  if(flash_job.step==EXP_JOURNAL) {
    if(p->cancelled) sj_runtime_cancel();
    result=sj_runtime_poll(now);
    if(result==1) return;
    sj_runtime_reset();
    if(result!=2) { if(sj_runtime_blocked()) mem_storage_ready=0;
      goto retry;
    }
    flash_job.step=EXP_SCAN;
    log_addControlEvent((logControl_e)138,(char *)p->record+2,255);
    if(p->cancelled) goto retry;
    return;
  }
  if(p->cancelled) goto retry;
  switch(flash_job.step) {
  case EXP_SCAN:
    if(p->scan>=p->used_limit) { expiry_release(FX_OK);
      return;
    }
    n=p->scan-p->snapshot_base;
    if(n>=sizeof(p->snapshot)) {
      p->snapshot_base=p->scan;
      n=p->used_limit-p->scan;
      if(n>sizeof(p->snapshot)) n=sizeof(p->snapshot);
      if(!w25_read_data(p->scan,p->snapshot,(uint16_t)n)) goto retry;
      n=0;
    }
    record=p->snapshot+n;
    p->scan+=32;
    memcpy(&index,record,2);
    if(index==65535 || record[31]==255) return;
    end=id_time_end((const cellID_s *)record);
    if(!end) return;
    if(p->scan_time<end) { id_watch_end(end);
      return;
    }
    if(end/86400u!=p->scan_time/86400u) return;
    memcpy(p->record,record,32);
    p->record[28]=56;
    flash_job.step=EXP_DIRECT;
    return;
  case EXP_DIRECT:
    memcpy(&index,p->record,2);
    if(index && index!=65535 && 32u*(index-1)<p->partition_limit-p->start) {
      address=p->start+32u*(index-1);
      if(!w25_read_data(address,scratch,32)) goto retry;
      memcpy(&got,scratch,2);
      if(got==index && scratch[31]!=255) goto found;
    }
    p->find=p->start;
    flash_job.step=EXP_SEARCH;
    return;
  case EXP_SEARCH:
    address=p->find;
    n=p->partition_limit-address;
    if(!n) goto retry;
    if(n>64) n=64;
    if(!w25_read_data(address,scratch,(uint16_t)n)) goto retry;
    memcpy(&index,p->record,2);
    memcpy(&got,scratch,2);
    if(got==index && scratch[31]!=255) goto found;
    if(n==64) { memcpy(&got,scratch+32,2);
      if(got==index && scratch[63]!=255) { address+=32;
        goto found;
      } }
    p->find+=n;
    return;
  case EXP_START:
    if(!sj_runtime_begin(p->target&~4095u,expiry_chunk,p)) goto retry;
    flash_job.step=EXP_JOURNAL;
    return;
  default: goto retry;
  }
found: p->target=address;
flash_job.step=EXP_START;
return;
retry:
  idcrc_unknown();
  id_endScan = 1;
  expiry_release(p->cancelled ? FX_CANCELLED : FX_ERROR);
}
