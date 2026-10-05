/* Доработка: Кряжев А., 2026. Owned command 9/10 upload, same polling phases. */
#include "runtime_internal.h"
extern uint16_t log_unacked[2],temp_unacked[2];
extern uint32_t log_lastIndex;
extern bool store_upload_request(uint8_t,dataChanel_e,uint8_t);
extern bool upload_ack(uint8_t,dataChanel_e),upload_finish(uint8_t);
extern uint16_t upload_index(uint8_t,dataChanel_e);
extern bool log_blank(const void *),log_valid(const void *);
typedef struct {
  uint8_t *packet;
  uint32_t pos,limit,epoch;
  uint16_t total,first,wanted,copied,seen,crc,offset,length;
  uint8_t command,channel,uniq,phase;
} upload_s;
RT_ASSERT(upload_size,sizeof(upload_s)==36);
upload_s *upload_runtime_context;
bool upload_runtime_active(void) { return upload_runtime_context!=NULL; }
void upload_runtime_cancel(void)
{
  upload_s *p=upload_runtime_context; if(!p) return;
  upload_runtime_context=NULL; flash_release(FO_UPLOAD);
  if(p->packet) work_release(p->packet, WORK_UPLOAD_PACKET); work_release(p, WORK_UPLOAD_CONTEXT);
}
void upload_runtime_quiet(uint8_t channel)
{ if(upload_runtime_context && upload_runtime_context->channel==channel) upload_runtime_cancel(); }
bool upload_runtime_begin(uint8_t command,dataChanel_e channel,uint8_t uniq)
{
  upload_s *p; unsigned part=command==10?7:8;
  if(!store_runtime) return false;
  if(upload_runtime_context || !mem_tables_ready()) return true;
  p=work_acquire(sizeof *p, WORK_UPLOAD_CONTEXT); if(!p) return true;
  p->packet=work_acquire(1040, WORK_UPLOAD_PACKET); if(!p->packet) { work_release(p, WORK_UPLOAD_CONTEXT); return true; }
  if(!store_upload_request(command,channel,uniq)) {
    work_release(p->packet, WORK_UPLOAD_PACKET); work_release(p, WORK_UPLOAD_CONTEXT); return true;
  }
  if(!work_upload_ack(command,(uint8_t)channel,p)) return true;
  p->first=upload_index(command,channel); p->total=p->wanted=p->copied=p->seen=p->crc=p->offset=p->length=0;
  p->command=command; p->channel=channel; p->uniq=uniq; p->phase=1; p->epoch=usb_bus_resets;
  p->pos=mem[part].start; p->limit=p->pos+mem[part].size;
  upload_runtime_context=p; flash_claim(FO_UPLOAD); return true;
}
void upload_runtime_poll(void)
{
  upload_s *p=upload_runtime_context;
  uint32_t record[8]; uint8_t *b=(uint8_t *)record;
  uint16_t n; bool done; uint8_t command;
  if(!p) return;
  command=p->command;
  if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
  switch(p->phase) {
  case 1:
    done=p->pos+32>p->limit;
    if(!done) {
      if(!w25_read_data(p->pos,b,32)) goto cancel;
      done=command==10?log_blank(b):((record[0]&65535u)==65535u);
      if(!done) {
        if(command!=10 || log_valid(b)) {
          if(command==10 && log_lastIndex<record[0]) log_lastIndex=record[0];
          p->total++;
        }
        p->pos+=32; return;
      }
    }
    if(command==10) { device.logsCount=p->total; device.newAddressLog=p->pos; }
    else { device.tempIDsCount=p->total; device.newAddressTempID=p->pos; }
    if(p->total<=p->first) {
      upload_runtime_context=NULL; flash_release(FO_UPLOAD); done=upload_finish(command);
      flash_claim(FO_UPLOAD); upload_runtime_context=p; if(!done) goto cancel;
      rt_put32(p->packet,0x4e444c53u); rt_put16(p->packet+4,6); p->length=14;
      p->packet[6]=255; p->packet[7]=p->uniq; p->packet[8]=command;
      p->packet[9]=p->packet[10]=p->packet[11]=0; p->wanted=0;
      p->crc=65535; p->offset=4; p->phase=4; return;
    }
    n=p->total-p->first; if(n>(command==10?32:100)) n=command==10?32:100;
    p->wanted=n; p->length=n*(command==10?32:10)+14; p->phase=2; return;
  case 2:
    rt_put32(p->packet,0x4e444c53u); rt_put16(p->packet+4,p->length-8);
    p->packet[6]=255; p->packet[7]=p->uniq; p->packet[8]=command;
    rt_put16(p->packet+9,p->total-p->first-p->wanted); p->packet[11]=(uint8_t)p->wanted;
    p->pos=command==10?mem[7].start:mem[8].start+32u*p->first; p->phase=3; return;
  case 3:
    if(p->pos+32>p->limit) goto cancel;
    if(command!=10) {
      if(!w25_read_data(p->pos+2,p->packet+12+10*p->copied,10)) goto cancel;
    } else {
      if(!w25_read_data(p->pos,b,32) || log_blank(b)) goto cancel;
      if(!log_valid(b)) { p->pos+=32; return; }
      if(p->seen++<p->first) { p->pos+=32; return; }
      record[6]=record[7]=0; memcpy(p->packet+12+32*p->copied,b,32);
    }
    if(++p->copied>=p->wanted) { p->crc=65535; p->offset=4; p->phase=4; }
    else p->pos+=32;
    return;
  case 4:
    n=p->length-2-p->offset; if(n>64) n=64;
    p->crc=crc16_calcInit(p->packet+p->offset,n,p->crc); p->offset+=n;
    if(p->offset>=p->length-2) { rt_put16(p->packet+p->offset,p->crc); p->phase=5; }
    return;
  case 5:
    if(!reply_send(p->packet,p->length,p->channel)) return;
    if(!p->channel && p->epoch!=usb_bus_resets) goto cancel;
    if(!p->wanted) goto cancel;
    (command==10?log_unacked:temp_unacked)[p->channel]=p->first+p->wanted;
    if(command==10) { if(p->channel) device.status.readyLogData=1; }
    else device.status.readyID=1;
    break;
  default: break;
  }
cancel: upload_runtime_cancel();
}
