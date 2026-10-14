// Доработка: Кряжев А., 2026
// Выгрузка журнала и временных ID с устойчивыми курсорами подтверждения.
// Кадр принадлежит автомату до успешной передачи; общий формат задаёт packet.c.
// Отмена освобождает память, не подтверждая неотправленные записи.
#include "runtime_internal.h"
#include "../log_ring.h"
enum {
  UP_INIT=0, UP_COUNT=1, UP_HEADER=2,
  UP_COPY=3, UP_FRAME=4, UP_SEND=5
};
extern uint16_t temp_unacked[2];
extern uint32_t log_lastIndex;
extern bool store_upload_request(uint8_t,dataChanel_e,uint8_t);
extern bool upload_finish(uint8_t);
extern uint16_t upload_index(uint8_t,dataChanel_e);
extern bool log_valid(const cellLog_s *);
extern bool upload_ack(uint8_t,dataChanel_e);
typedef struct {
  uint8_t *packet;
  uint32_t pos,limit,epoch;
  uint16_t total,first,wanted,copied,seen,length;
  uint8_t command,channel,uniq,reply_step;
} upload_s;
RT_ASSERT(upload_size,sizeof(upload_s)==32);
static upload_s *upload_reply;
// По одному намерению на канал; state: 1 ждёт, 2 принято, 3 завершено/истекло.
static struct {
  uint32_t until;
  uint8_t command, uniq, state, ttl;
} upload_pending[2];
static upload_s *upload_current(void)
{
  return flash_job.kind == FX_UPLOAD && flash_job.result == FX_PENDING ? flash_job.data.context : upload_reply;
}
static uint8_t up_phase(const upload_s *p)
{
  return flash_job.kind == FX_UPLOAD && flash_job.result == FX_PENDING ? flash_job.step : p->reply_step;
}
static void up_step(upload_s *p, uint8_t step)
{
  if (flash_job.kind == FX_UPLOAD && flash_job.result == FX_PENDING) flash_job.step = step;
  else p->reply_step = step;
  if (step == UP_SEND) {
    upload_reply = p;
    p->reply_step = step;
  }
}
// Возвращает оба участка выгрузки; USB удерживает заимствованный кадр до завершения.
static void upload_release(upload_s *p)
{
  work_release(p->packet,WORK_UPLOAD_PACKET);
  work_release(p,WORK_UPLOAD_CONTEXT);
}
// Резервирует весь ответ до подтверждения, чтобы нехватка памяти не сдвинула курсор.
static upload_s *upload_acquire(void)
{
  upload_s *p=work_acquire(sizeof *p,WORK_UPLOAD_CONTEXT);
  if(!p) return NULL;
  p->packet=work_acquire(1040,WORK_UPLOAD_PACKET);
  if(!p->packet) { work_release(p,WORK_UPLOAD_CONTEXT);
  return NULL;
  }
  return p;
}
// Освобождает кадр и контекст без подтверждения неотправленных данных.
void upload_runtime_cancel(void)
{
  upload_s *p=upload_current();
  if(!p) return;
  if (upload_pending[p->channel].command == p->command && upload_pending[p->channel].uniq == p->uniq)
    upload_pending[p->channel].state = 3;
  if (upload_reply == p) upload_reply = NULL;
  if (flash_job.kind == FX_UPLOAD) flash_exec_finish(FX_CANCELLED);
  upload_release(p);
}
// Останавливает выгрузку при закрытии её канала.
void upload_runtime_quiet(uint8_t channel)
{
  if (channel < 2) memset(&upload_pending[channel], 0, sizeof(upload_pending[0]));
  log_ring_quiet(channel);
  if(upload_current() && upload_current()->channel==channel) upload_runtime_cancel();
}
// Повтор не продлевает TTL и не создаёт вторую выгрузку с тем же uniq.
void upload_runtime_request(uint8_t command, dataChanel_e channel, uint8_t uniq, uint8_t ttl)
{
  if (channel > 1) return;
  if (upload_pending[channel].state && upload_pending[channel].command == command &&
      upload_pending[channel].uniq == uniq) return;
  upload_pending_poll();
  if (upload_pending[channel].state == 1 || upload_pending[channel].state == 2) {
    user_send_answer_comand(command == 10 ? 9 : 10, CMD_RUNERROR, channel, uniq);
    return;
  }
  upload_pending[channel].command = command;
  upload_pending[channel].uniq = uniq;
  upload_pending[channel].ttl = ttl;
  upload_pending[channel].until = timers_getMillis() + (uint32_t)ttl * 1000u;
  upload_pending[channel].state = 1;
  upload_pending_poll();
}
// Срок 1..250 секунд задан протоколом; 255 бессрочен, резерв 0 совместим со старым USB.
void upload_pending_poll(void)
{
  uint32_t now = timers_getMillis();
  for (uint8_t c = 0; c < 2; ++c) {
    if (upload_pending[c].state != 1) continue;
    if (upload_pending[c].ttl && upload_pending[c].ttl <= 250 &&
        (int32_t)(now - upload_pending[c].until) >= 0) {
      upload_pending[c].state = 3;
      continue;
    }
    if (upload_runtime_begin(upload_pending[c].command, (dataChanel_e)c, upload_pending[c].uniq))
      upload_pending[c].state = 2;
  }
}
// Принимает запрос выгрузки после старта и сохраняет подтверждённый курсор.
bool upload_runtime_begin(uint8_t command,dataChanel_e channel,uint8_t uniq)
{
  upload_s *p;
  unsigned part=command==10?MB_LOG:MB_TEMPID;
  if(channel>1) return false;
  if(upload_current() || !flash_exec_idle() || !flash_exec_read_allowed()) return false;
  p=upload_acquire();
  if(!p) return false;
  if(!store_upload_request(command,channel,uniq)) { upload_release(p);
  return false;
  }
  // ACK сохраняет настройки в том же участке: возвращаем резерв и затем берём его снова.
  upload_release(p);
  if(command==10 ? !log_ring_ack((uint8_t)channel,uniq) : !upload_ack(command,channel)) return true;
  p=upload_acquire();
  if(!p) return false;
  p->first=command==10?0:upload_index(command,channel);
  p->total = 0;
  p->wanted = 0;
  p->copied = 0;
  p->seen = 0;
  p->length = 0;
  p->command=command;
  p->channel=channel;
  p->uniq=uniq;
  p->reply_step=command==10?UP_INIT:UP_COUNT;
  p->epoch=usb_bus_resets;
  p->pos=mem[part].start;
  p->limit=p->pos+mem[part].size;
  flash_exec_args_t args = { p, p->reply_step };
  if (!flash_exec_begin(FX_UPLOAD, &args)) {
    upload_release(p);
    return false;
  }
  return true;
}
// Считает записи, заполняет тело кадра и передаёт его выбранному каналу.
void upload_runtime_poll(void)
{
  upload_s *p=upload_current();
  uint32_t record[8];
  uint8_t *b=(uint8_t *)record;
  uint16_t n;
  bool done;
  uint8_t command,result;
  if(!p) return;
  command=p->command;
  if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
  switch(up_phase(p)) {
  case UP_INIT:
    result=log_ring_init_step();
    if(result==3) goto cancel;
    if(result==2) { p->pos=log_ring_first();
    up_step(p, UP_COUNT);
    }
    return;
  case UP_COUNT:
    if(command==10) {
      if(p->pos) {
        if(!w25_read_data(p->pos,b,32)) goto cancel;
        if(log_valid((const cellLog_s *)b)) {
          p->seen++;
          if(record[0]>=log_cursor32[p->channel]) p->total++;
          if(log_lastIndex<record[0]) log_lastIndex=record[0];
        }
        p->pos=log_ring_next(p->pos);
        return;
      }
      device.logsCount=p->seen;
    } else {
      done=p->pos+32>p->limit;
      if(!done) {
        if(!w25_read_data(p->pos,b,32)) goto cancel;
        done=(record[0]&65535u)==65535u;
        if(!done) { p->total++;
        p->pos+=32;
        return;
        }
      }
      device.tempIDsCount=p->total;
      device.newAddressTempID=p->pos;
    }
    if(p->total<=p->first) {
      upload_reply = p;
      flash_exec_finish(FX_OK);
      flash_exec_take_result();
      done = upload_finish(command);
      if (!done) goto cancel;
      p->length=14;
      p->packet[6]=255;
      p->packet[7]=p->uniq;
      p->packet[8]=command;
      p->packet[9]=p->packet[10]=p->packet[11]=0;
      p->wanted=0;
      packet_frame(p->packet,p->length-8);
      up_step(p, UP_SEND);
      return;
    }
    n=p->total-p->first;
    if(n>(command==10?32:100)) n=command==10?32:100;
    p->wanted=n;
    p->length=n*(command==10?32:10)+14;
    up_step(p, UP_HEADER);
    return;
  case UP_HEADER:
    p->packet[6]=255;
    p->packet[7]=p->uniq;
    p->packet[8]=command;
    rt_put16(p->packet+9,p->total-p->first-p->wanted);
    p->packet[11]=(uint8_t)p->wanted;
    p->pos=command==10?log_ring_first():mem[MB_TEMPID].start+32u*p->first;
    up_step(p, UP_COPY);
    return;
  case UP_COPY:
    if(command!=10) {
      if(p->pos+32>p->limit || !w25_read_data(p->pos+2,p->packet+12+10*p->copied,10)) goto cancel;
    } else {
      if(!p->pos || !w25_read_data(p->pos,b,32)) goto cancel;
      p->pos=log_ring_next(p->pos);
      if(!log_valid((const cellLog_s *)b) || record[0]<log_cursor32[p->channel]) return;
      p->limit=record[0]+1u;
      record[6]=record[7]=0;
      memcpy(p->packet+12+32*p->copied,b,32);
    }
    if(++p->copied>=p->wanted) { up_step(p, UP_FRAME);
    }
    else if(command!=10) p->pos+=32;
    return;
  case UP_FRAME:
    packet_frame(p->packet,p->length-8);
    up_step(p, UP_SEND);
    if (flash_job.kind == FX_UPLOAD) flash_exec_finish(FX_OK);
    return;
  case UP_SEND:
    if(!reply_send(p->packet,p->length,p->channel)) return;
    if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
    if(!p->wanted) goto cancel;
    if(command==10) { log_ring_sent(p->channel,p->uniq,p->limit);
    if(p->channel) device.status.readyLogData=1;
    }
    else { temp_unacked[p->channel]=p->first+p->wanted;
    device.status.readyID=1;
    }
    break;
  default: break;
  }
cancel: upload_runtime_cancel();
}
bool upload_reply_poll(void)
{
  upload_s *p = upload_current();
  if (!p || up_phase(p) != UP_SEND || flash_job.kind == FX_UPLOAD) return false;
  upload_runtime_poll();
  return upload_current() != p;
}
