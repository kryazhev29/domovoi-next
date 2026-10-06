// Доработка: Кряжев А., 2026
// Фоновые CRC таблиц, настроек и сегментов ID.
// Чтение ограничено фрагментами 64 Б и временем одного прохода.
// Собственный ответ ждёт свободного канала; SLDN собирает packet.c.
#include "runtime_internal.h"
#include "fast_kernels.h"
#include "../sched16.h"
typedef struct {
  uint32_t address,real,erased,datetime;
  uint16_t crc;
  uint8_t kind,phase,channel,uniq,segment,index;
  uint16_t length;
  uint8_t power,waiting;
  uint32_t used;
  uint8_t scratch[64],packet[212];
  uint32_t wait_start;
} service_crc_s;
RT_ASSERT(service_crc_size,sizeof(service_crc_s)==312);
RT_ASSERT(service_crc_packet,offsetof(service_crc_s,packet)==96);
service_crc_s *service_crc_job;
// Возвращает владельца текущего запроса CRC.
void *service_crc_active(void) { return service_crc_job; }
// Освобождает флеш и буфер ответа CRC.
void service_crc_cancel(void)
{
  service_crc_s *p;
  flash_release(FO_CRC); p=service_crc_job; service_crc_job=NULL; work_release(p, WORK_SERVICE);
}
// Отменяет запрос CRC закрывшегося канала.
void service_crc_quiet(uint8_t channel)
{ if(service_crc_job && service_crc_job->channel==channel) service_crc_cancel(); }
// Создаёт запрос CRC и готовит общие поля тела ответа.
static uint8_t sc_begin(uint8_t kind,uint8_t channel,uint8_t uniq,uint8_t index)
{
  service_crc_s *p;
  if(channel>1 || service_crc_job || !user_storage_flash_ready() || mem_config_result()==1) return 0;
  if(!mem_tables_ready() || index>100 || !(p=work_acquire(sizeof *p, WORK_SERVICE))) {
    if(kind!=4) user_send_answer_comand(kind==5?17:18,(cmdCode_e)2,(dataChanel_e)channel,uniq);
    return 0;
  }
  memset(p,0,sizeof *p); p->kind=kind; p->phase=1; p->channel=channel; p->uniq=uniq; p->index=index;
  p->used=id_used_bytes; p->datetime=config.datetime;
  p->length=kind==6?211:kind==15?14:21;
  p->packet[6]=255; p->packet[7]=uniq; p->packet[8]=kind;
  if(kind==15) p->packet[9]=index;
  service_crc_job=p; flash_claim(FO_CRC); return 1;
}
// Начинает расчёт CRC всех таблиц для команды 17.
uint8_t service_crc_begin_pack(uint8_t channel,uint8_t uniq) { return sc_begin(5,channel,uniq,0); }
// Начинает CRC всей базы или выбранного сегмента ID.
uint8_t service_crc_begin_id(uint8_t index,uint8_t channel,uint8_t uniq) { return sc_begin(index?15:6,channel,uniq,index); }
// Начинает CRC выбранного образа настроек.
uint8_t service_crc_begin_config(uint8_t channel,uint8_t uniq) { return sc_begin(4,channel,uniq,0); }
// Выбирает область очередного CRC, учитывая пустой хвост базы ID.
static void sc_setup(service_crc_s *p)
{
  static const uint8_t parts[5]={2,3,4,6,5};
  uint32_t offset=0,real,size,address; uint8_t part;
  if(p->kind==5) part=parts[p->segment];
  else if(p->kind==4) part=6;
  else { part=2; offset=16000u*(p->kind==15?p->index-1:p->segment); }
  address=mem[part].start; size=part==MB_SHEDULE ? sched16_crc_size() : mem[part].size;
  if(part==2) {
    if(p->kind!=5) size=16000;
    real=p->used<offset?0:p->used-offset; if(real>size) real=size;
    p->erased=size-real; size=real; address+=offset;
  } else {
    p->erased=0;
    if(part==6) address=address-0x1cd400u+cfg_cached_active;
  }
  p->address=address; p->real=size; p->power=p->waiting=0; p->crc=65535; p->phase=2;
  if (part==MB_ID && p->kind==5 && idcrc_get(0,mem[MB_ID].size,&p->crc)) p->real=p->erased=0;
}
// Считает CRC в пределах бюджета и повторяет отправку при занятом канале.
void service_crc_poll(void)
{
  service_crc_s *p=service_crc_job; uint32_t now,n,started=timers_getMillis(); uint8_t *out,chunks=4;
  if(!p) return;
  if(p->phase==1) { sc_setup(p); return; }
  if(p->phase==2) {
read_more:
    if(p->real) {
      if(!w25_read_available()) {
        now=timers_getMillis();
        if(!p->waiting) { p->wait_start=now; p->waiting=1; return; }
        if(now-p->wait_start>=6000u) goto bad;
        return;
      }
      p->waiting=0; n=p->real>64?64:p->real;
      if(!w25_read_data(p->address,p->scratch,(uint16_t)n)) goto bad;
      p->address+=n; p->real-=n;
      p->crc=crc16_calcInit(p->scratch,(uint16_t)n,p->crc);
      if (--chunks && timers_getMillis()-started<1u) goto read_more;
      return;
    }
    if(p->erased) {
      p->crc=crc16_ffInit(p->erased,p->crc); p->erased=0;
    }
    if(p->kind==4) {
      p->length=18; p->packet[8]=12; p->packet[9]=4;
      rt_put32(p->packet+10,p->datetime); rt_put16(p->packet+14,p->crc);
    } else {
      out=p->packet+(p->kind==15?10:9+2*p->segment); rt_put16(out,p->crc);
      if(p->kind!=15 && ++p->segment!=(p->kind==6?100:5)) { p->phase=1; return; }
    }
    p->phase=3; return;
  }
  if(p->phase==3) {
    packet_frame(p->packet,p->length-8); p->phase=4; return;
  }
  if(p->channel==1 && comm_tx_busy()) return;
  if(reply_send(p->packet,p->length,p->channel)) service_crc_cancel();
  return;
bad:
  p->length=19; answer11(p->packet,p->packet[7],p->kind==5?17:p->kind==4?12:18,2); p->phase=4;
}
