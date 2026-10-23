// Доработка: Кряжев А., 2026
// Фоновая проверка сроков ID, чтение базы по 64 Б.
// Отметка прохода дописывается в слой счётчиков; журнал секторов — при полном слое или смене ячейки.
// Рабочая память и флеш освобождаются при отмене, ошибке или завершении.
#include "runtime_internal.h"
enum {
  EXP_SCAN=1, EXP_DIRECT=2,
  EXP_START=4, EXP_JOURNAL=5
};
extern bool id_endScan;
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(const cellID_s *);
extern void id_watch_end(uint32_t);
typedef struct {
  uint32_t scan,used_limit,start,target;
  uint8_t cancelled;
  uint32_t scan_time;
  uint8_t record[32];
  uint32_t snapshot_base;
  uint8_t snapshot[64];
} expiry_s;
RT_ASSERT(expiry_size,sizeof(expiry_s)==124);
static expiry_s *expiry_current(void)
{
  return flash_job.kind == FX_EXPIRY && flash_job.result == FX_PENDING ? flash_job.data.context : NULL;
}
// Сообщает, что проверка сроков дошла до записи сектора: чтение базы закрыто.
bool expiry_runtime_writing(void)
{
  return flash_job.step >= EXP_START;
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
  uint32_t now,start,used;
  expiry_s *p;
  if(id_scan_cell || !flash_exec_idle() || !flash_exec_read_allowed()) return false;
  now=device.datetime;
  if(now<1600000000u) return false;
  if(!id_endScan && (!id_nextEnd || now<id_nextEnd)) return false;
  start=mem[MB_ID].start;
  used=32u*id_cells_end;
  p=work_acquire(sizeof *p, WORK_EXPIRY);
  if(!p) { id_endScan=1;
    return false;
  }
  p->scan=p->start=start;
  p->snapshot_base=start-sizeof(p->snapshot);
  p->used_limit=start+used;
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
  if(!w25_read_data(address,data,64) || !counter_overlay(address,data,64)) return 0;
  if(diff<=32) memcpy(data+diff,p->record,32);
  return 1;
}
// Читает два ID за фрагмент и журналирует завершённый проход.
void expiry_runtime_poll(uint32_t now)
{
  expiry_s *p=expiry_current();
  uint8_t *record;
  uint16_t index;
  uint32_t n,end;
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
      if(!mem_read(p->scan,p->snapshot,(uint16_t)n)) goto retry;
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
    /* Отметка пишется туда, где запись прочитана: пока идёт FX_EXPIRY, область ID никто
       не пишет. Прежний поиск по номеру при одинаковых номерах отмечал чужую ячейку (23.10.2026). */
    p->target=p->scan-32;
    flash_job.step=EXP_DIRECT;
    return;
  case EXP_DIRECT: {
    int written=counter_write(p->target,(const cellID_s *)p->record);
    if(written<0) goto retry;
    if(written==1) {
      flash_job.step=EXP_SCAN;
      log_addControlEvent((logControl_e)138,(char *)p->record+2,255);
      if(p->cancelled) goto retry;
      return;
    }
    flash_job.step=EXP_START;
    return;
  }
  case EXP_START:
    if(!sj_runtime_begin(p->target&~4095u,expiry_chunk,p)) goto retry;
    flash_job.step=EXP_JOURNAL;
    return;
  default: goto retry;
  }
retry:
  idcrc_unknown();
  id_endScan = 1;
  expiry_release(p->cancelled ? FX_CANCELLED : FX_ERROR);
}
