// Доработка: Кряжев А., 2026
// Выгрузка журнала и временных ID с устойчивыми курсорами подтверждения.
// Кадр принадлежит автомату до успешной передачи; общий формат задаёт packet.c.
// Отмена освобождает память, не подтверждая неотправленные записи.
#include "runtime_internal.h"
#include "../log_ring.h"
extern uint16_t log_unacked[2],temp_unacked[2];
extern uint32_t log_lastIndex;
extern bool store_upload_request(uint8_t,dataChanel_e,uint8_t);
extern bool upload_finish(uint8_t);
extern uint16_t upload_index(uint8_t,dataChanel_e);
extern bool log_blank(const void *),log_valid(const void *);
extern void log_ring_sent(uint8_t,uint8_t,uint32_t);
typedef struct {
  uint8_t *packet;
  uint32_t pos,limit,epoch;
  uint16_t total,first,wanted,copied,seen,length;
  uint8_t command,channel,uniq,phase;
} upload_s;
RT_ASSERT(upload_size,sizeof(upload_s)==32);
upload_s *upload_runtime_context;
// Сообщает, выполняется ли выгрузка.
bool upload_runtime_active(void) { return upload_runtime_context!=NULL; }
// Освобождает кадр и контекст без подтверждения неотправленных данных.
void upload_runtime_cancel(void)
{
  upload_s *p=upload_runtime_context; if(!p) return;
  upload_runtime_context=NULL; flash_release(FO_UPLOAD);
  if(p->packet) work_release(p->packet,WORK_UPLOAD_PACKET); work_release(p,WORK_UPLOAD_CONTEXT);
}
// Останавливает выгрузку при закрытии её канала.
void upload_runtime_quiet(uint8_t channel)
{
  log_ring_quiet(channel);
  if(upload_runtime_context && upload_runtime_context->channel==channel) upload_runtime_cancel();
}
// Принимает запрос выгрузки после старта и сохраняет подтверждённый курсор.
bool upload_runtime_begin(uint8_t command,dataChanel_e channel,uint8_t uniq)
{
  upload_s *p; unsigned part=command==10?7:8;
  if(channel>1) return false;
  if(upload_runtime_context || !mem_tables_ready()) return true;
  p=work_acquire(sizeof *p,WORK_UPLOAD_CONTEXT); if(!p) return true;
  p->packet=work_acquire(1040,WORK_UPLOAD_PACKET);
  if(!p->packet) { work_release(p,WORK_UPLOAD_CONTEXT); return true; }
  if(!store_upload_request(command,channel,uniq)) {
    work_release(p->packet,WORK_UPLOAD_PACKET); work_release(p,WORK_UPLOAD_CONTEXT); return true;
  }
  if(command==10) {
    work_release(p->packet,WORK_UPLOAD_PACKET); work_release(p,WORK_UPLOAD_CONTEXT);
    if(!log_ring_ack((uint8_t)channel,uniq)) return true;
    p=work_acquire(sizeof *p,WORK_UPLOAD_CONTEXT); if(!p) return true;
    p->packet=work_acquire(1040,WORK_UPLOAD_PACKET);
    if(!p->packet) { work_release(p,WORK_UPLOAD_CONTEXT); return true; }
  } else if(!work_upload_ack(command,(uint8_t)channel,p)) return true;
  p->first=command==10?0:upload_index(command,channel);
  p->total=p->wanted=p->copied=p->seen=p->length=0;
  p->command=command; p->channel=channel; p->uniq=uniq; p->phase=command==10?0:1; p->epoch=usb_bus_resets;
  p->pos=mem[part].start; p->limit=p->pos+mem[part].size;
  upload_runtime_context=p; flash_claim(FO_UPLOAD); return true;
}
// Считает записи, заполняет тело кадра и передаёт его выбранному каналу.
void upload_runtime_poll(void)
{
  upload_s *p=upload_runtime_context; uint32_t record[8]; uint8_t *b=(uint8_t *)record;
  uint16_t n; bool done; uint8_t command,result;
  if(!p) return;
  command=p->command;
  if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
  switch(p->phase) {
  case 0:
    result=log_ring_init_step(); if(result==3) goto cancel;
    if(result==2) { p->pos=log_ring_first(); p->phase=1; }
    return;
  case 1:
    if(command==10) {
      if(p->pos) {
        if(!w25_read_data(p->pos,b,32)) goto cancel;
        if(log_valid(b)) {
          p->seen++; if(record[0]>=log_cursor32[p->channel]) p->total++;
          if(log_lastIndex<record[0]) log_lastIndex=record[0];
        }
        p->pos=log_ring_next(p->pos); return;
      }
      device.logsCount=p->seen;
    } else {
      done=p->pos+32>p->limit;
      if(!done) {
        if(!w25_read_data(p->pos,b,32)) goto cancel;
        done=(record[0]&65535u)==65535u;
        if(!done) { p->total++; p->pos+=32; return; }
      }
      device.tempIDsCount=p->total; device.newAddressTempID=p->pos;
    }
    if(p->total<=p->first) {
      upload_runtime_context=NULL; flash_release(FO_UPLOAD); done=upload_finish(command);
      flash_claim(FO_UPLOAD); upload_runtime_context=p; if(!done) goto cancel;
      p->length=14;
      p->packet[6]=255; p->packet[7]=p->uniq; p->packet[8]=command;
      p->packet[9]=p->packet[10]=p->packet[11]=0; p->wanted=0;
      p->phase=4; return;
    }
    n=p->total-p->first; if(n>(command==10?32:100)) n=command==10?32:100;
    p->wanted=n; p->length=n*(command==10?32:10)+14; p->phase=2; return;
  case 2:
    p->packet[6]=255; p->packet[7]=p->uniq; p->packet[8]=command;
    rt_put16(p->packet+9,p->total-p->first-p->wanted); p->packet[11]=(uint8_t)p->wanted;
    p->pos=command==10?log_ring_first():mem[8].start+32u*p->first; p->phase=3; return;
  case 3:
    if(command!=10) {
      if(p->pos+32>p->limit || !w25_read_data(p->pos+2,p->packet+12+10*p->copied,10)) goto cancel;
    } else {
      if(!p->pos || !w25_read_data(p->pos,b,32)) goto cancel;
      p->pos=log_ring_next(p->pos);
      if(!log_valid(b) || record[0]<log_cursor32[p->channel]) return;
      p->limit=record[0]+1u; record[6]=record[7]=0;
      memcpy(p->packet+12+32*p->copied,b,32);
    }
    if(++p->copied>=p->wanted) { p->phase=4; }
    else if(command!=10) p->pos+=32;
    return;
  case 4: packet_frame(p->packet,p->length-8); p->phase=5; return;
  case 5:
    if(!reply_send(p->packet,p->length,p->channel)) return;
    if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
    if(!p->wanted) goto cancel;
    if(command==10) { log_ring_sent(p->channel,p->uniq,p->limit); if(p->channel) device.status.readyLogData=1; }
    else { temp_unacked[p->channel]=p->first+p->wanted; device.status.readyID=1; }
    break;
  default: break;
  }
cancel: upload_runtime_cancel();
}
