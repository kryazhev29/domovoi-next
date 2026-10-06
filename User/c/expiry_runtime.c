/* Доработка: Кряжев А., 2026. Preserve the historical 1024-byte scan snapshot. */
#include "runtime_internal.h"
extern uint8_t id_endScan;
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(const cellID_s *);
extern void id_watch_end(uint32_t);
typedef struct {
  uint32_t scan,used_limit,start,target,find,partition_limit;
  uint8_t phase,cancelled;
  uint16_t fill_offset;
  uint32_t scan_time;
  uint8_t record[32];
  uint32_t snapshot_base;
  uint8_t snapshot[1024];
} expiry_s;
RT_ASSERT(expiry_size,sizeof(expiry_s)==1092);
expiry_s *expiry_job;
bool expiry_runtime_active(void) { return expiry_job!=NULL; }
void expiry_runtime_cancel(void) { if(expiry_job) expiry_job->cancelled=1; }
static void expiry_release(void) { flash_release(FO_EXPIRY); work_release(expiry_job, WORK_EXPIRY); expiry_job=NULL; }
bool expiry_runtime_begin(void)
{
  uint32_t now,start,size,end,used; expiry_s *p;
  if(expiry_job || !user_storage_background_ready() || !mem_tables_ready()) return false;
  now=device.datetime; if(now<1600000000u) return false;
  if(!id_endScan && (!id_nextEnd || now<id_nextEnd)) return false;
  start=mem[MB_ID].start; size=mem[MB_ID].size; end=start+size; used=32u*id_cells_end;
  if((start&31u) || (size&31u) || end<start || used>size) return false;
  p=work_acquire(sizeof *p, WORK_EXPIRY); if(!p) { id_endScan=1; return false; }
  p->scan=p->start=p->snapshot_base=start; p->used_limit=start+used; p->partition_limit=end;
  p->scan_time=now; p->phase=6; p->cancelled=0; p->fill_offset=0;
  expiry_job=p; flash_claim(FO_EXPIRY); id_endScan=0; id_nextEnd=0; return true;
}
static uint8_t expiry_chunk(const void *context,uint16_t offset,uint8_t *data)
{
  const expiry_s *p=context; uint32_t address=(p->target&~4095u)+offset,diff=p->target-address;
  if(!w25_read_data(address,data,64)) return 0;
  if(diff<=32) memcpy(data+diff,p->record,32);
  return 1;
}
void expiry_runtime_poll(uint32_t now)
{
  expiry_s *p=expiry_job; uint8_t scratch[64],*record; uint16_t index,got;
  uint32_t address,n,end; uint8_t result;
  if(!p) return;
  if(p->phase==5) {
    if(p->cancelled) sj_runtime_cancel();
    result=sj_runtime_poll(now); if(result==1) return;
    sj_runtime_reset();
    if(result!=2) { if(sj_runtime_blocked()) mem_storage_ready=0; goto retry; }
    p->phase=1; log_addControlEvent((logControl_e)138,(char *)p->record+2,255);
    if(p->cancelled) goto retry;
    return;
  }
  if(p->cancelled) goto retry;
  switch(p->phase) {
  case 6:
    n=p->used_limit-p->snapshot_base; if(n>1024) n=1024;
    if(n!=p->fill_offset) {
      end=n-p->fill_offset; if(end>64) end=64;
      if(!w25_read_data(p->snapshot_base+p->fill_offset,p->snapshot+p->fill_offset,(uint16_t)end)) goto retry;
      p->fill_offset+=(uint16_t)end; if(p->fill_offset<n) return;
    }
    p->phase=1; return;
  case 1:
    if(p->scan>=p->used_limit) { expiry_release(); return; }
    n=p->scan-p->snapshot_base;
    if(n>=1024) { p->snapshot_base=p->scan; p->fill_offset=0; p->phase=6; return; }
    record=p->snapshot+n; p->scan+=32;
    memcpy(&index,record,2); if(index==65535 || record[31]==255) return;
    end=id_time_end((const cellID_s *)record); if(!end) return;
    if(p->scan_time<end) { id_watch_end(end); return; }
    if(end/86400u!=p->scan_time/86400u) return;
    memcpy(p->record,record,32); p->record[28]=56; p->phase=2; return;
  case 2:
    memcpy(&index,p->record,2);
    if(index && index!=65535 && 32u*(index-1)<p->partition_limit-p->start) {
      address=p->start+32u*(index-1);
      if(!w25_read_data(address,scratch,32)) goto retry;
      memcpy(&got,scratch,2); if(got==index && scratch[31]!=255) goto found;
    }
    p->find=p->start; p->phase=3; return;
  case 3:
    address=p->find; n=p->partition_limit-address; if(!n) goto retry;
    if(n>64) n=64;
    if(!w25_read_data(address,scratch,(uint16_t)n)) goto retry;
    memcpy(&index,p->record,2); memcpy(&got,scratch,2); if(got==index && scratch[31]!=255) goto found;
    if(n==64) { memcpy(&got,scratch+32,2); if(got==index && scratch[63]!=255) { address+=32; goto found; } }
    p->find+=n; return;
  case 4:
    if(!sj_runtime_begin(p->target&~4095u,expiry_chunk,p)) goto retry;
    p->phase=5; return;
  default: goto retry;
  }
found: p->target=address; p->phase=4; return;
retry: id_endScan=1; expiry_release();
}
