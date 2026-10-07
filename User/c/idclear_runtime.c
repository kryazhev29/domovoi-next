// Доработка: Кряжев А., 2026
// Фоновая очистка базы ID по секторам с ответом на команду 103.
// Стирание проверяет W25; при частичной очистке таблицы блокируются.
// Смена USB-сеанса подавляет ответ, сохраняя завершение текущей команды W25.
#include "runtime_internal.h"
enum {
  IDC_ERASE=1, IDC_DONE=2, IDC_ERROR=3
};
typedef struct {
  uint32_t cursor,end,epoch;
  uint8_t phase,waiting,cancel,quiet,touched,uniq;
  uint8_t response[19];
} idclear_s;
RT_ASSERT(idclear_size, sizeof(idclear_s)==40);
idclear_s *idclear_job;
// Сообщает, выполняется ли очистка базы.
uint8_t idclear_runtime_active(void) { return idclear_job!=NULL; }
// Запрашивает отмену очистки и подавляет ответ: CRC базы больше не известна.
void idclear_runtime_cancel(void) { if(idclear_job) { idcrc_unknown(); idclear_job->cancel=idclear_job->quiet=1; } }
// Подавляет ответ для завершившегося USB-сеанса.
void idclear_runtime_quiet(uint8_t channel) { if(!channel && idclear_job) idclear_job->quiet=1; }
// Освобождает память автомата после ответа или отмены.
static void idclear_release(void) { idclear_s *p=idclear_job; idclear_job=NULL; work_release(p, WORK_IDCLEAR); }
// Начинает разрешённую USB-команду очистки всей базы ID.
uint8_t idclear_runtime_begin(uint8_t channel,uint8_t uniq)
{
  uint32_t start=mem[MB_ID].start,size=mem[MB_ID].size,end=start+size;
  idclear_s *p;
  if(channel || idclear_job || !user_storage_command_ready() || !mem_tables_ready()) return 0;
  p=work_acquire(sizeof *p, WORK_IDCLEAR); if(!p) return 0;
  p->cursor=start; p->end=end; p->epoch=usb_bus_resets;
  p->phase=IDC_ERASE; p->waiting=p->cancel=p->quiet=p->touched=0; p->uniq=uniq;
  p->response[6]=255; p->response[7]=uniq; p->response[8]=103;
  packet_frame(p->response,3);
  idindex_runtime_suspend(); flash_claim(FO_IDCLEAR); idclear_job=p; return 1;
}
// Блокирует частично очищенную базу и готовит ответ об ошибке.
static void idclear_error(idclear_s *p)
{
  mem_storage_ready=0; p->waiting=0; p->phase=IDC_ERROR;
  answer11(p->response,p->uniq,103,2);
}
// Стирает один сектор за шаг и отправляет итог через общий кадр.
void idclear_runtime_poll(uint32_t now)
{
  idclear_s *p=idclear_job; uint8_t result;
  if(!p) return;
  if(p->epoch!=usb_bus_resets) p->quiet=1;
  if(p->phase==IDC_ERASE) {
    if(p->waiting) {
      result=w25_async_poll(now); if(result==1) return;
      if(result!=2) { idclear_error(p); return; }
      p->waiting=0; p->cursor+=4096; return;
    }
    if(p->cursor==p->end) {
      device.numbersCount=0; id_cells_end=0; id_used_bytes=0;
      idindex_scanned(0,0); p->phase=IDC_DONE; return;
    }
    if(p->cancel) {
      if(p->touched) mem_storage_ready=0;
      flash_release(FO_IDCLEAR); idclear_release(); return;
    }
    if(!w25_async_start(p->cursor,NULL,0,now)) { idclear_error(p); return; }
    p->waiting=p->touched=1; return;
  }
  if(!p->quiet && !reply_send(p->response,p->phase==IDC_DONE?11:19,0)) return;
  flash_release(FO_IDCLEAR); idclear_release();
}
