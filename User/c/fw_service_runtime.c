// Доработка: Кряжев А., 2026
// Команда 19: начало загрузки, проверка резерва и откат прошивки.
// W25 выполняет проверенные операции по 64 Б; заголовок и CRC прошивки проверяются на границе.
// Сохранение настроек передаётся владельцу store; поля его состояния заданы общим типом.
#include "runtime_internal.h"
#include "user_fw_reason.h"
#include "../fw_crc.h"
enum {
  FWS_BACKUP_HEADER=1, FWS_BACKUP_CRC=2, FWS_NEW_ERASE=4,
  FWS_NEW_HEADER=5, FWS_SAVE=6, FWS_SAVE_WAIT=8,
  FWS_COPY_ERASE=10, FWS_COPY_READ=11, FWS_COPY_WRITE=12,
  FWS_COPY_HEADER=13, FWS_COPY_CRC=14, FWS_REPLY=16,
  FWS_REJECT=18
};
extern uint8_t fw_start_rejected(uint16_t,uint32_t,uint32_t,uint32_t,uint16_t);
extern uint8_t fw_same_download(uint16_t,uint32_t,uint32_t,uint32_t,uint16_t,dataChanel_e);
extern uint32_t fw_rollback_end;
extern uint8_t fw_write_errors,store_save_requested;
extern bool store_snapshot(void);
// Состояние сохранения принадлежит user_store.c; сервис передаёт ему команду 19.
extern store_state_s store;
typedef struct {
  uint8_t data[64],header[32];
  void *reserve;
  uint32_t pos,end,crc,expected;
  uint16_t blocks;
  uint8_t channel,uniq,phase,pending,quiet,mode;
  uint16_t len;
  uint8_t cancel,config_result;
} fw_service_s;
RT_ASSERT(fw_service_size,sizeof(fw_service_s)==128);
fw_service_s *fw_service_context;
// Возвращает контекст занятого сервиса прошивки.
void *fw_service_busy(void) { return fw_service_context; }
// Временный мост: store заканчивает A/B и снимает своё намерение ответа.
bool fw_service_store_waiting(void)
{
  return fw_service_context && fw_service_context->phase == FWS_SAVE_WAIT && store.job;
}
// Принимает результат сохранения настроек и возвращает владение флешем.
void fw_service_config_result(uint8_t result)
{
  if(fw_service_context && fw_service_context->phase==FWS_SAVE_WAIT && (result==2 || result==3)) {
    fw_service_context->config_result=result; flash_claim(FO_FWSERVICE);
  }
}
// Подавляет ответ прошивки для закрытого канала.
void fw_service_quiet(uint8_t channel)
{ if(fw_service_context && fw_service_context->channel==channel) fw_service_context->quiet=1; }
// Отменяет сервис, сохраняя источник до завершения команды W25.
void fw_service_cancel(void)
{
  fw_service_s *p=fw_service_context; if(!p) return;
  p->cancel=p->quiet=1; if(p->pending) return;
  fw_service_context=NULL; flash_release(FO_FWSERVICE); work_release(p->reserve, WORK_FW_DATA); work_release(p, WORK_FW_CONTEXT);
}
// Проверяет команду 19 на границе и выбирает загрузку либо откат.
bool fw_service_begin(const uint8_t *data,dataChanel_e channel,uint8_t uniq)
{
  fw_service_s *p; fw_reason_e reason; uint8_t phase;
  if(fw_service_context || !user_storage_command_ready() || !mem_tables_ready()) return true;
  p=work_acquire(sizeof *p, WORK_FW_CONTEXT); if(!p) return true;
  memset(p,0,sizeof *p); p->channel=channel; p->uniq=uniq;
  if(fw_start_rejected(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14))) {
    fw_event_request((logUpdate_e)133,FW_REASON_PARAM);
    p->config_result=1; phase=FWS_REJECT; goto publish;
  }
  p->reserve=work_acquire(1024, WORK_FW_DATA); if(!p->reserve) { work_release(p, WORK_FW_CONTEXT); return true; }
  p->blocks=rt_get16(data+14);
  if(rt_get16(data)!=1 && fw_same_download(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14),channel)) {
    device.startWaitFWtime=0; phase=FWS_SAVE; goto publish;
  }
  reason=fw_refused();
  if(reason) {
    fw_event_request((logUpdate_e)133,reason); work_release(p->reserve, WORK_FW_DATA); p->reserve=NULL;
    p->config_result=2; phase=FWS_REJECT; goto publish;
  }
  if(rt_get16(data)==1) { p->mode=1; phase=FWS_BACKUP_HEADER; }
  else {
    memset(p->header,255,32); p->header[4]=device.hardwareVersion;
    rt_put16(p->header+5,rt_get16(data)); rt_put32(p->header+8,rt_get32(data+6));
    rt_put32(p->header+12,rt_get32(data+10)); rt_put32(p->header+20,rt_get32(data+2));
    p->mode=2; phase=FWS_NEW_ERASE;
  }
publish: p->phase=phase; flash_claim(FO_FWSERVICE); fw_service_context=p; return true;
}
// Проверяет резерв, копирует прошивку и согласует сохранение со store.
void fw_service_poll(void)
{
  fw_service_s *p=fw_service_context; uint8_t phase,result,code=2;
  uint32_t address,n; uint8_t *source; bool saved; fw_reason_e reason;
  if(!p) return; phase=p->phase;
  if(p->pending) {
    result=w25_async_poll(timers_getMillis()); if(result==1) return;
    p->pending=0; if(p->cancel) goto cancel;
    if(result!=2) goto io_error;
    if(phase==FWS_NEW_ERASE) p->phase=FWS_NEW_HEADER;
    else if(phase==FWS_NEW_HEADER) p->phase=FWS_SAVE;
    else if(phase==FWS_COPY_ERASE) p->phase=FWS_COPY_READ;
    else {
      p->pos+=p->len;
      p->phase=p->pos>=p->end?FWS_COPY_HEADER:(p->pos&4095u)?FWS_COPY_READ:FWS_COPY_ERASE;
    }
    return;
  }
  if(p->cancel) goto cancel;
  switch(phase) {
  case FWS_NEW_ERASE: case FWS_COPY_ERASE:
    address=mem[MB_FIRMWARE].start+(phase==FWS_COPY_ERASE?p->pos:0); source=NULL; n=0; goto start;
  case FWS_NEW_HEADER: address=mem[MB_FIRMWARE].start; source=p->header; n=32; goto start;
  case FWS_COPY_WRITE: address=mem[MB_FIRMWARE].start+p->pos; source=p->data; n=p->len;
start:
    if(!w25_async_start(address,source,(uint16_t)n,timers_getMillis())) goto io_error;
    p->pending=1; return;
  case FWS_COPY_READ:
    n=p->end-p->pos; if(n>64) n=64; p->len=(uint16_t)n;
    if(!w25_read_data(mem[MB_BACKUP].start+p->pos,p->data,p->len)) goto read_error;
    if(!p->pos) memset(p->data,255,4);
    p->phase=FWS_COPY_WRITE; return;
  case FWS_BACKUP_HEADER: case FWS_COPY_HEADER:
    if(!w25_read_data(mem[phase==FWS_BACKUP_HEADER?MB_BACKUP:MB_FIRMWARE].start,p->header,32)) goto read_error;
    if(rt_get32(p->header+20)==0xffffffffu || rt_get32(p->header+12)==0xffffffffu || rt_get16(p->header+5)==65535) {
      reason=phase>=FWS_COPY_HEADER?FW_REASON_CRC:FW_REASON_NO_BACKUP; goto error_log;
    }
    p->expected=rt_get32(p->header+12); n=rt_get32(p->header+8);
    if(!n || (n&3u) || n>0x17e00u) goto bad_crc;
    p->end=n+512; p->pos=512; p->crc=fw_crc_begin(); p->phase=phase==FWS_COPY_HEADER?FWS_COPY_CRC:FWS_BACKUP_CRC; return;
  case FWS_BACKUP_CRC: case FWS_COPY_CRC:
    n=p->end-p->pos; if(n>64) n=64; p->len=(uint16_t)n;
    if(!w25_read_data(mem[phase==FWS_BACKUP_CRC?MB_BACKUP:MB_FIRMWARE].start+p->pos,p->data,p->len)) goto read_error;
    for(n=0;n<p->len;n+=4) p->crc=fw_crc_next(p->crc,rt_get32(p->data+n));
    p->pos+=p->len; if(p->pos<p->end) return;
    if(p->crc!=p->expected) goto bad_crc;
    if(phase==FWS_COPY_CRC) { fw_rollback_end=0; fw_event_request((logUpdate_e)137,FW_REASON_NONE); device.rebootToUpdate=1; goto cancel; }
    fw_rollback_end=p->end;
    device.cchFWupdate=p->channel; device.status.readyToUpdate=1; goto handoff;
  case FWS_SAVE:
    if(p->mode) {
      device.FWblockCount=p->blocks; device.FWblockIdx=0; device.firmwarePercent=0; device.startWaitFWtime=0;
      fw_write_errors=0; device.cchFWupdate=p->channel; fw_event_request((logUpdate_e)132,FW_REASON_NONE);
      if(p->channel) { device.FWinProgress=1; device.updateTimeout=600; device.status.readyToUpdate=1; }
    }
handoff:
    fw_service_context=NULL; source=p->reserve; p->reserve=NULL; work_release(source, WORK_FW_DATA);
    flash_release(FO_FWSERVICE); saved=store_snapshot(); fw_service_context=p;
    if(!saved) { flash_claim(FO_FWSERVICE); goto io_error; }
    store.cmd=19; store.channel=p->channel; store.uniq=p->uniq; store.quiet=1; store.job=STORE_JOB_COMMAND;
    store_save_requested=0; p->phase=FWS_SAVE_WAIT; return;
  case FWS_SAVE_WAIT:
    if(store.job) return;
    if(p->config_result!=2) goto io_error;
    code=0; goto answer;
  case FWS_REJECT: code=p->config_result; goto answer;
  case FWS_REPLY:
    if(!p->quiet && !reply_send(p->data,p->len,p->channel)) return;
    if(p->data[8]==11 && !p->data[10] && p->mode==1) { p->pos=0; p->phase=FWS_COPY_ERASE; return; }
    goto cancel;
  default: return;
  }
read_error:
  if(phase>=FWS_COPY_HEADER) { reason=FW_REASON_CRC; goto error_log; }
  if(phase==FWS_BACKUP_CRC) goto bad_crc;
  if(phase==FWS_BACKUP_HEADER) goto cleanup;
  reason=FW_REASON_UNKNOWN; goto error_log;
bad_crc: reason=phase==FWS_COPY_CRC?FW_REASON_CRC:FW_REASON_BAD_BACKUP; goto error_log;
io_error:
  if(p->phase<FWS_COPY_ERASE) goto cleanup;
  reason=FW_REASON_WRITE;
error_log:
  if(p->phase>=FWS_COPY_ERASE && p->phase<FWS_REPLY) {
    fw_service_context=NULL; flash_release(FO_FWSERVICE); fw_cancel(reason);
    flash_claim(FO_FWSERVICE); fw_service_context=p;
    if(p->channel && !device.registredOnServer) goto cancel;
    p->data[6]=255;
    p->data[7]=0; p->data[8]=4; p->data[9]=device.firmwarePercent; p->data[10]=253;
    packet_frame(p->data,5); p->len=13; p->phase=FWS_REPLY; return;
  }
  fw_event_request((logUpdate_e)133,reason);
cleanup:
  fw_rollback_end=0; device.status.readyToUpdate=0; device.FWinProgress=0;
  device.updateTimeout=0; device.FWblockCount=0; device.FWblockIdx=0; fw_write_errors=0; code=2;
answer: answer11(p->data,p->uniq,19,code); p->len=19; p->phase=FWS_REPLY; return;
cancel: fw_service_cancel();
}
