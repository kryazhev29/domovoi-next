/* Доработка: Кряжев А., 2026. Step-for-step counterpart of idclear_runtime.s. */
#include "runtime_internal.h"
typedef struct {
  uint32_t cursor,end,start,epoch;
  uint8_t phase,waiting,cancel,quiet,touched,reserved,uniq,reserved2;
  uint8_t success[12],error[20];
} idclear_s;
RT_ASSERT(idclear_size, sizeof(idclear_s)==56);
idclear_s *idclear_job;
uint8_t idclear_runtime_active(void) { return idclear_job!=NULL; }
void idclear_runtime_cancel(void) { if(idclear_job) { idcrc_unknown(); idclear_job->cancel=idclear_job->quiet=1; } }
void idclear_runtime_quiet(uint8_t channel) { if(!channel && idclear_job) idclear_job->quiet=1; }
static void idclear_release(void) { idclear_s *p=idclear_job; idclear_job=NULL; work_release(p, WORK_IDCLEAR); }
uint8_t idclear_runtime_begin(uint8_t channel,uint8_t uniq)
{
  uint32_t start=mem[MB_ID].start,size=mem[MB_ID].size,end=start+size;
  idclear_s *p;
  if(channel || idclear_job || !user_storage_command_ready() || !mem_tables_ready()) return 0;
  if(((start|size)&4095u) || start<0x41000u || end<start || end>0x1c8000u) return 0;
  p=work_acquire(sizeof *p, WORK_IDCLEAR); if(!p) return 0;
  p->cursor=p->start=start; p->end=end; p->epoch=usb_bus_resets;
  p->phase=1; p->waiting=p->cancel=p->quiet=p->touched=p->reserved=p->reserved2=0; p->uniq=uniq;
  rt_put32(p->success,0x4e444c53u); rt_put16(p->success+4,3);
  p->success[6]=255; p->success[7]=uniq; p->success[8]=103;
  rt_put16(p->success+9,crc16_calc(p->success+4,5));
  idindex_runtime_suspend(); flash_claim(FO_IDCLEAR); idclear_job=p; return 1;
}
static void idclear_error(idclear_s *p)
{
  mem_storage_ready=0; p->waiting=0; p->phase=3;
  answer11(p->error,p->uniq,103,2);
}
void idclear_runtime_poll(uint32_t now)
{
  idclear_s *p=idclear_job; uint8_t result;
  if(!p) return;
  if(p->epoch!=usb_bus_resets) p->quiet=1;
  if(p->phase==1) {
    if(p->waiting) {
      result=w25_async_poll(now); if(result==1) return;
      if(result!=2) { idclear_error(p); return; }
      p->waiting=0; p->cursor+=4096; return;
    }
    if(p->cursor==p->end) {
      device.numbersCount=0; id_cells_end=0; id_used_bytes=0;
      idindex_scanned(0,0); p->phase=2; return;
    }
    if(p->cancel) {
      if(p->touched) mem_storage_ready=0;
      flash_release(FO_IDCLEAR); idclear_release(); return;
    }
    if(!w25_async_start(p->cursor,NULL,0,now)) { idclear_error(p); return; }
    p->waiting=p->touched=1; return;
  }
  if(!p->quiet && !usb_try_send(p->phase==2?p->success:p->error,p->phase==2?11:19)) return;
  flash_release(FO_IDCLEAR); idclear_release();
}
