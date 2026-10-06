/* Доработка: Кряжев А., 2026. Command19/rollback, including owned error replies. */
#include "runtime_internal.h"
#include "../fw_crc.h"
extern uint8_t fw_start_rejected(uint16_t,uint32_t,uint32_t,uint32_t,uint16_t);
extern const char *fw_start_refused(void);
extern uint8_t fw_same_download(uint16_t,uint32_t,uint32_t,uint32_t,uint16_t,dataChanel_e);
extern void fw_log(logUpdate_e,const char *),fw_abort(const char *);
extern uint32_t fw_rollback_pos,fw_rollback_end;
extern uint8_t fw_write_errors,store_save_requested;
extern bool store_snapshot(void);
/* Same store layout exported by user_fun; offsets are part of the ASM ABI. */
extern uint8_t store[];
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
void *fw_service_busy(void) { return fw_service_context; }
bool fw_service_active(void)
{ return fw_service_context && (fw_service_context->phase!=8 || !store[18]); }
void fw_service_config_result(uint8_t result)
{
  if(fw_service_context && fw_service_context->phase==8 && (result==2 || result==3)) {
    fw_service_context->config_result=result; flash_claim(FO_FWSERVICE);
  }
}
void fw_service_quiet(uint8_t channel)
{ if(fw_service_context && fw_service_context->channel==channel) fw_service_context->quiet=1; }
void fw_service_cancel(void)
{
  fw_service_s *p=fw_service_context; if(!p) return;
  p->cancel=p->quiet=1; if(p->pending) return;
  fw_service_context=NULL; flash_release(FO_FWSERVICE); work_release(p->reserve, WORK_FW_DATA); work_release(p, WORK_FW_CONTEXT);
}
bool fw_service_begin(const uint8_t *data,dataChanel_e channel,uint8_t uniq)
{
  fw_service_s *p; const char *reason; uint8_t phase;
  if(!store_runtime) return false;
  if(fw_service_context || !user_storage_command_ready() || !mem_tables_ready()) return true;
  if(fw_start_rejected(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14))) {
    fw_log((logUpdate_e)133,"PARAM");
    p=work_acquire(sizeof *p, WORK_FW_CONTEXT); if(!p) return true;
    p->reserve=NULL; memset(&p->phase,0,8); p->channel=channel; p->uniq=uniq;
    p->config_result=1; phase=18; goto publish;
  }
  p=work_acquire(sizeof *p, WORK_FW_CONTEXT); if(!p) return true;
  p->reserve=work_acquire(1024, WORK_FW_DATA); if(!p->reserve) { work_release(p, WORK_FW_CONTEXT); return true; }
  memset(&p->phase,0,8); p->channel=channel; p->uniq=uniq; p->blocks=rt_get16(data+14);
  if(rt_get16(data)!=1 && fw_same_download(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14),channel)) {
    device.startWaitFWtime=0; phase=6; goto publish;
  }
  reason=fw_start_refused();
  if(reason) {
    fw_log((logUpdate_e)133,reason); work_release(p->reserve, WORK_FW_DATA); p->reserve=NULL;
    p->config_result=2; phase=18; goto publish;
  }
  if(rt_get16(data)==1) { p->mode=1; phase=1; }
  else {
    memset(p->header,255,32); p->header[4]=device.hardwareVersion;
    rt_put16(p->header+5,rt_get16(data)); rt_put32(p->header+8,rt_get32(data+6));
    rt_put32(p->header+12,rt_get32(data+10)); rt_put32(p->header+20,rt_get32(data+2));
    p->mode=2; phase=4;
  }
publish: p->phase=phase; flash_claim(FO_FWSERVICE); fw_service_context=p; return true;
}
void fw_service_poll(void)
{
  fw_service_s *p=fw_service_context; uint8_t phase,result,code=2;
  uint32_t address,n; uint8_t *source; bool saved; const char *reason;
  if(!p) return; phase=p->phase;
  if(p->pending) {
    result=w25_async_poll(timers_getMillis()); if(result==1) return;
    p->pending=0; if(p->cancel) goto cancel;
    if(result!=2) goto io_error;
    if(phase==4) p->phase=5;
    else if(phase==5) p->phase=6;
    else if(phase==10) p->phase=11;
    else {
      p->pos+=p->len; fw_rollback_pos=p->pos;
      p->phase=p->pos>=p->end?13:(p->pos&4095u)?11:10;
    }
    return;
  }
  if(p->cancel) goto cancel;
  switch(phase) {
  case 4: case 10:
    address=mem[0].start+(phase==10?p->pos:0); source=NULL; n=0; goto start;
  case 5: address=mem[0].start; source=p->header; n=32; goto start;
  case 12: address=mem[0].start+p->pos; source=p->data; n=p->len;
start:
    if(!w25_async_start(address,source,(uint16_t)n,timers_getMillis())) goto io_error;
    p->pending=1; return;
  case 11:
    n=p->end-p->pos; if(n>64) n=64; p->len=(uint16_t)n;
    if(!w25_read_data(mem[1].start+p->pos,p->data,p->len)) goto read_error;
    if(!p->pos) memset(p->data,255,4);
    p->phase=12; return;
  case 1: case 13:
    if(!w25_read_data(mem[phase==1?1:0].start,p->header,32)) goto read_error;
    if(rt_get32(p->header+20)==0xffffffffu || rt_get32(p->header+12)==0xffffffffu || rt_get16(p->header+5)==65535) {
      reason=phase>=13?"CRC":"NO BACKUP"; goto error_log;
    }
    p->expected=rt_get32(p->header+12); n=rt_get32(p->header+8);
    if(!n || (n&3u) || n>0x17e00u) goto bad_crc;
    p->end=n+512; p->pos=512; p->crc=fw_crc_begin(); p->phase=phase==13?14:2; return;
  case 2: case 14:
    n=p->end-p->pos; if(n>64) n=64; p->len=(uint16_t)n;
    if(!w25_read_data(mem[phase==2?1:0].start+p->pos,p->data,p->len)) goto read_error;
    for(n=0;n<p->len;n+=4) p->crc=fw_crc_next(p->crc,rt_get32(p->data+n));
    p->pos+=p->len; if(p->pos<p->end) return;
    if(p->crc!=p->expected) goto bad_crc;
    if(phase==14) { fw_rollback_end=0; fw_log((logUpdate_e)137,NULL); device.rebootToUpdate=1; goto cancel; }
    fw_rollback_end=p->end; fw_rollback_pos=0;
    device.cchFWupdate=p->channel; device.status.readyToUpdate=1; goto handoff;
  case 6:
    if(p->mode) {
      device.FWblockCount=p->blocks; device.FWblockIdx=0; device.firmwarePercent=0; device.startWaitFWtime=0;
      fw_write_errors=0; device.cchFWupdate=p->channel; fw_log((logUpdate_e)132,NULL);
      if(p->channel) { device.FWinProgress=1; device.updateTimeout=600; device.status.readyToUpdate=1; }
    }
handoff:
    fw_service_context=NULL; source=p->reserve; p->reserve=NULL; work_release(source, WORK_FW_DATA);
    flash_release(FO_FWSERVICE); saved=store_snapshot(); fw_service_context=p;
    if(!saved) { flash_claim(FO_FWSERVICE); goto io_error; }
    store[21]=19; store[22]=p->channel; store[23]=p->uniq; store[24]=1; store[18]=9;
    store_save_requested=0; p->phase=8; return;
  case 8:
    if(store[18]) return;
    if(p->config_result!=2) goto io_error;
    code=0; goto answer;
  case 18: code=p->config_result; goto answer;
  case 16:
    if(!p->quiet && !reply_send(p->data,p->len,p->channel)) return;
    if(p->data[8]==11 && !p->data[10] && p->mode==1) { p->pos=0; p->phase=10; return; }
    goto cancel;
  default: return;
  }
read_error:
  if(phase>=13) { reason="CRC"; goto error_log; }
  if(phase==2) goto bad_crc;
  if(phase==1) goto cleanup;
  reason="READ"; goto error_log;
bad_crc: reason=phase==14?"CRC":"BAD BACKUP"; goto error_log;
io_error:
  if(p->phase<10) goto cleanup;
  reason="WRITE";
error_log:
  if(p->phase>=10 && p->phase<16) {
    fw_service_context=NULL; flash_release(FO_FWSERVICE); fw_abort(reason);
    flash_claim(FO_FWSERVICE); fw_service_context=p;
    if(p->channel && !device.registredOnServer) goto cancel;
    rt_put32(p->data,0x4e444c53u); rt_put16(p->data+4,5); p->data[6]=255;
    p->data[7]=0; p->data[8]=4; p->data[9]=device.firmwarePercent; p->data[10]=253;
    rt_put16(p->data+11,crc16_calc(p->data+4,7)); p->len=13; p->phase=16; return;
  }
  fw_log((logUpdate_e)133,reason);
cleanup:
  fw_rollback_end=0; device.status.readyToUpdate=0; device.FWinProgress=0;
  device.updateTimeout=0; device.FWblockCount=0; device.FWblockIdx=0; fw_write_errors=0; code=2;
answer: answer11(p->data,p->uniq,19,code); p->len=19; p->phase=16; return;
cancel: fw_service_cancel();
}
