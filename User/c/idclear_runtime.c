// Доработка: Кряжев А., 2026
// Фоновая очистка базы ID по секторам с ответом на команду 103.
// Стирание проверяет W25; при частичной очистке таблицы блокируются.
// Ответ уходит отложенным ответом исполнителя; закрытый USB-сеанс его подавляет.
#include "runtime_internal.h"
enum {
  IDC_ERASE=1, IDC_ERASED=2, IDC_DONE=3, IDC_ERROR=4
};
typedef struct {
  uint32_t cursor,end;
  uint8_t cancel,touched;
  uint8_t response[18];
} idclear_s;
RT_ASSERT(idclear_size, sizeof(idclear_s)==28);
static idclear_s *idclear_current(void)
{
  return flash_job.kind == FX_IDCLEAR && flash_job.result == FX_PENDING ? flash_job.data.context : NULL;
}
// Запрашивает отмену очистки и подавляет ответ: CRC базы больше не известна.
void idclear_runtime_cancel(void)
{
  idclear_s *p = idclear_current();
  if (p) {
    idcrc_unknown();
    p->cancel = 1;
    flash_job.quiet = 1;
  }
  fx_reply_drop(FX_IDCLEAR);
}
// Освобождает память автомата после отмены или подавленного ответа.
static void idclear_release(flash_result_t result)
{
  work_release(idclear_current(), WORK_IDCLEAR);
  flash_exec_finish(result);
}
// Начинает разрешённую USB-команду очистки всей базы ID.
uint8_t idclear_runtime_begin(uint8_t channel,uint8_t uniq)
{
  uint32_t start=mem[MB_ID].start,size=mem[MB_ID].size,end=start+size;
  idclear_s *p;
  if(id_scan_cell || channel || !flash_exec_idle() || !flash_exec_read_allowed()) return 0;
  p=work_acquire(sizeof *p, WORK_IDCLEAR);
  if(!p) return 0;
  p->cursor=start;
  p->end=end;
  p->cancel=p->touched=0;
  p->response[6]=255;
  p->response[7]=uniq;
  p->response[8]=103;
  packet_frame(p->response,3);
  flash_exec_args_t args = { p, IDC_ERASE };
  if (!flash_exec_begin(FX_IDCLEAR, &args)) {
    work_release(p, WORK_IDCLEAR);
    return 0;
  }
  flash_job.channel=CH_USB;
  flash_job.uniq=uniq;
  idindex_runtime_suspend();
  return 1;
}
// Блокирует частично очищенную базу и готовит ответ об ошибке.
static void idclear_error(idclear_s *p)
{
  mem_storage_ready=0;
  flash_job.step=IDC_ERROR;
  answer11(p->response,flash_job.uniq,103,2);
}
// Стирает один сектор за шаг и отдаёт итог отложенным ответом исполнителя.
void idclear_runtime_poll(uint32_t now)
{
  idclear_s *p=idclear_current();
  if(!p) return;
  if(fx_waiting(now) && flash_job.wait) return; // итог стирания — в этом же проходе
  if(flash_job.step==FX_STEP_FAIL) { idclear_error(p);
    return;
  }
  if(flash_job.step==IDC_ERASED) {
    if(!counter_replaced(p->cursor)) { idclear_error(p);
      return;
    }
    flash_job.step=IDC_ERASE;
    p->cursor+=4096;
    return;
  }
  if(flash_job.step==IDC_ERASE) {
    if(p->cursor==p->end) {
      device.numbersCount=0;
      id_cells_end=0;
      id_used_bytes=0;
      idindex_scanned(0,0);
      flash_job.step=IDC_DONE;
      return;
    }
    if(p->cancel) {
      if(p->touched) mem_storage_ready=0;
      idclear_release(FX_CANCELLED);
      return;
    }
    fx_start(p->cursor,NULL,0,now,IDC_ERASED);
    if(flash_job.wait) p->touched=1;
    return;
  }
  flash_result_t result = flash_job.step==IDC_DONE ? FX_OK : FX_ERROR;
  if(flash_job.quiet) { idclear_release(result);
    return;
  }
  if(fx_reply.kind) return;
  fx_reply_post(FX_IDCLEAR,p,p->response,flash_job.step==IDC_DONE?11:18,CH_USB);
  flash_exec_finish(result);
}
