// Доработка: Кряжев А., 2026
// Фоновая очистка базы ID по секторам с ответом на команду 103.
// Стирание проверяет W25; при частичной очистке таблицы блокируются.
// Смена USB-сеанса подавляет ответ, сохраняя завершение текущей команды W25.
#include "runtime_internal.h"
enum {
  IDC_ERASE=1, IDC_WAIT=2, IDC_DONE=3, IDC_ERROR=4
};
typedef struct {
  uint32_t cursor,end,epoch;
  uint8_t cancel,quiet,touched,uniq;
  uint8_t response[19];
} idclear_s;
RT_ASSERT(idclear_size, sizeof(idclear_s)==36);
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
    p->quiet = 1;
  }
}
// Подавляет ответ для завершившегося USB-сеанса.
void idclear_runtime_quiet(uint8_t channel)
{
  idclear_s *p = idclear_current();
  if (!channel && p) p->quiet = 1;
}
// Освобождает память автомата после ответа или отмены.
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
  p->epoch=usb_bus_resets;
  p->cancel=p->quiet=p->touched=0;
  p->uniq=uniq;
  p->response[6]=255;
  p->response[7]=uniq;
  p->response[8]=103;
  packet_frame(p->response,3);
  flash_exec_args_t args = { p, IDC_ERASE };
  if (!flash_exec_begin(FX_IDCLEAR, &args)) {
    work_release(p, WORK_IDCLEAR);
    return 0;
  }
  idindex_runtime_suspend();
  return 1;
}
// Блокирует частично очищенную базу и готовит ответ об ошибке.
static void idclear_error(idclear_s *p)
{
  mem_storage_ready=0;
  flash_job.step=IDC_ERROR;
  answer11(p->response,p->uniq,103,2);
}
// Стирает один сектор за шаг и отправляет итог через общий кадр.
void idclear_runtime_poll(uint32_t now)
{
  idclear_s *p=idclear_current();
  uint8_t result;
  if(!p) return;
  if(p->epoch!=usb_bus_resets) p->quiet=1;
  if(flash_job.step==IDC_ERASE || flash_job.step==IDC_WAIT) {
    if(flash_job.step==IDC_WAIT) {
      result=w25_async_poll(now);
      if(result==1) return;
      if(result!=2) { idclear_error(p);
        return;
      }
      flash_job.step=IDC_ERASE;
      p->cursor+=4096;
      return;
    }
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
    if(!w25_async_start(p->cursor,NULL,0,now)) { idclear_error(p);
      return;
    }
    flash_job.step=IDC_WAIT;
    p->touched=1;
    return;
  }
  if(!p->quiet && !reply_send(p->response,flash_job.step==IDC_DONE?11:19,0)) return;
  idclear_release(flash_job.step==IDC_DONE ? FX_OK : FX_ERROR);
}
