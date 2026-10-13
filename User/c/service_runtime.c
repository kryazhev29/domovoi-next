// Доработка: Кряжев А., 2026
// Фоновые CRC таблиц, настроек и сегментов ID.
// Чтение ограничено фрагментами 64 Б и временем одного прохода.
// Собственный ответ ждёт свободного канала; SLDN собирает packet.c.
#include "runtime_internal.h"
#include "fast_kernels.h"
#include "../sched16.h"
enum {
  SC_CONFIG=4, SC_TABLES=5, SC_ID_BLOCKS=6,
  SC_ID_BLOCK=15
};
enum {
  SC_SETUP=1, SC_READ=2, SC_FRAME=3,
  SC_SEND=4
};
typedef struct {
  uint32_t address,real,erased,datetime;
  uint16_t crc;
  uint8_t kind,failed,channel,uniq,segment,index;
  uint16_t length;
  uint8_t waiting;
  uint32_t used;
  uint8_t packet[212];
  uint32_t wait_start;
} service_crc_s;
RT_ASSERT(service_crc_size,sizeof(service_crc_s)==248);
// Контекст ответа удерживается текущим kind до завершения передачи.
static service_crc_s *sc_current(void)
{
  return flash_job.kind == FX_CRC && flash_job.result == FX_PENDING ? flash_job.data.context : NULL;
}
static void sc_finish(flash_result_t result)
{
  work_release(sc_current(), WORK_SERVICE);
  flash_exec_finish(result);
}
// Освобождает флеш и буфер ответа CRC.
void service_crc_cancel(void)
{
  service_crc_s *p;
  p = sc_current();
  if (p) sc_finish(FX_CANCELLED);
}
// Отменяет запрос CRC закрывшегося канала.
void service_crc_quiet(uint8_t channel)
{
  service_crc_s *p = sc_current();
  if (p && p->channel == channel) service_crc_cancel();
}
// Создаёт запрос CRC и готовит общие поля тела ответа.
static uint8_t sc_begin(uint8_t kind,uint8_t channel,uint8_t uniq,uint8_t index)
{
  service_crc_s *p;
  if((kind!=SC_CONFIG && id_scan_cell) || channel>1 || !flash_exec_idle() || mem_config_result()==1) return 0;
  if(!flash_exec_read_allowed() || index>100 || !(p=work_acquire(sizeof *p, WORK_SERVICE))) {
    if(kind!=SC_CONFIG) user_send_answer_comand(kind==SC_TABLES?17:18,(cmdCode_e)2,(dataChanel_e)channel,uniq);
    return 0;
  }
  memset(p,0,sizeof *p);
  p->kind=kind;
  p->channel=channel;
  p->uniq=uniq;
  p->index=index;
  p->used=id_used_bytes;
  p->datetime=config.datetime;
  p->length=kind==SC_ID_BLOCKS?211:kind==SC_ID_BLOCK?14:21;
  p->packet[6]=255;
  p->packet[7]=uniq;
  p->packet[8]=kind;
  if(kind==SC_ID_BLOCK) p->packet[9]=index;
  flash_exec_args_t args = { p, SC_SETUP };
  if (!flash_exec_begin(FX_CRC, &args)) {
    work_release(p, WORK_SERVICE);
    return 0;
  }
  return 1;
}
// Начинает расчёт CRC всех таблиц для команды 17.
uint8_t service_crc_begin_pack(uint8_t channel,uint8_t uniq) { return sc_begin(SC_TABLES,channel,uniq,0);
}
// Начинает CRC всей базы или выбранного сегмента ID.
uint8_t service_crc_begin_id(uint8_t index,uint8_t channel,uint8_t uniq) { return sc_begin(index?SC_ID_BLOCK:SC_ID_BLOCKS,channel,uniq,index);
}
// Начинает CRC выбранного образа настроек.
uint8_t service_crc_begin_config(uint8_t channel,uint8_t uniq) { return sc_begin(SC_CONFIG,channel,uniq,0);
}
// Выбирает область очередного CRC, учитывая пустой хвост базы ID.
static void sc_setup(service_crc_s *p)
{
  static const uint8_t parts[5]={MB_ID,MB_GROUP,MB_SHEDULE,MB_CONFIG,MB_HOLIDAY};
  uint32_t offset=0,real,size,address;
  uint8_t part;
  if(p->kind==SC_TABLES) part=parts[p->segment];
  else if(p->kind==SC_CONFIG) part=MB_CONFIG;
  else { part=MB_ID;
    offset=16000u*(p->kind==SC_ID_BLOCK?p->index-1:p->segment);
  }
  address=mem[part].start;
  size=part==MB_SHEDULE ? sched16_crc_size() : mem[part].size;
  if(part==MB_ID) {
    if(p->kind!=SC_TABLES) size=16000;
    real=p->used<offset?0:p->used-offset;
    if(real>size) real=size;
    p->erased=size-real;
    size=real;
    address+=offset;
  } else {
    p->erased=0;
    if(part==MB_CONFIG) address=address-MEM_CONFIG_LEGACY+cfg_cached_active;
  }
  p->address=address;
  p->real=size;
  p->waiting=0;
  p->crc=65535;
  flash_job.step=SC_READ;
  if (part==MB_ID && p->kind==SC_TABLES && idcrc_get(&p->crc)) p->real=p->erased=0;
}
// Считает CRC в пределах бюджета и повторяет отправку при занятом канале.
void service_crc_poll(void)
{
  service_crc_s *p=sc_current();
  uint32_t now,n,started=timers_getMillis();
  uint8_t scratch[64],*out,chunks=4;
  if(!p) return;
  if(flash_job.step==SC_SETUP) { sc_setup(p);
    return;
  }
  if(flash_job.step==SC_READ) {
read_more:
    if(p->real) {
      if(!w25_read_available()) {
        now=timers_getMillis();
        if(!p->waiting) { p->wait_start=now;
          p->waiting=1;
          return;
        }
        if(now-p->wait_start>=6000u) goto bad;
        return;
      }
      p->waiting=0;
      n=p->real>64?64:p->real;
      if(!w25_read_data(p->address,scratch,(uint16_t)n)) goto bad;
      p->address+=n;
      p->real-=n;
      p->crc=crc16_calcInit(scratch,(uint16_t)n,p->crc);
      if (--chunks && timers_getMillis()-started<1u) goto read_more;
      return;
    }
    if(p->erased) {
      p->crc=crc16_ffInit(p->erased,p->crc);
      p->erased=0;
    }
    if(p->kind==SC_CONFIG) {
      p->length=18;
      p->packet[8]=12;
      p->packet[9]=4;
      rt_put32(p->packet+10,p->datetime);
      rt_put16(p->packet+14,p->crc);
    } else {
      out=p->packet+(p->kind==SC_ID_BLOCK?10:9+2*p->segment);
      rt_put16(out,p->crc);
      if(p->kind!=SC_ID_BLOCK && ++p->segment!=(p->kind==SC_ID_BLOCKS?100:5)) {
        flash_job.step = SC_SETUP;
        return;
      }
    }
    flash_job.step=SC_FRAME;
    return;
  }
  if(flash_job.step==SC_FRAME) {
    packet_frame(p->packet,p->length-8);
    flash_job.step=SC_SEND;
    return;
  }
  if(reply_send(p->packet,p->length,p->channel)) sc_finish(p->failed ? FX_ERROR : FX_OK);
  return;
bad:
  p->failed = 1;
  p->length=19;
  answer11(p->packet,p->packet[7],p->kind==SC_TABLES?17:p->kind==SC_CONFIG?12:18,2);
  flash_job.step=SC_SEND;
}
