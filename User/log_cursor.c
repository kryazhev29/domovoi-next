/* Durable logical record numbers; legacy device fields and wire frames keep
 * their layout. Reserved device bytes 16..31 hold the new checkpoint. */
#include "c/runtime_internal.h"
#include "log_ring.h"
#include "user_fun.h"
extern uint32_t log_lastIndex;
extern uint16_t log_usbReadIndex,log_unacked[2];
extern bool store_snapshot(void);
extern uint8_t store_save_requested;
uint32_t log_cursor32[2],log_pending32[2];
static struct { uint32_t value; uint8_t phase,channel,uniq,quiet,last[2],seen[2]; } ack;
#define CURSOR_MAGIC 0x354C5055u
void log_ring_checkpoint(void)
{
  uint32_t magic=CURSOR_MAGIC;
  memcpy(device.reserv+16,&magic,4); memcpy(device.reserv+20,log_cursor32,8);
  memcpy(device.reserv+28,&log_lastIndex,4);
}
void log_ring_restore(const device_s *saved)
{
  uint32_t magic,last;
  memcpy(&magic,saved->reserv+16,4);
  if(magic!=CURSOR_MAGIC) {
    log_cursor32[0]=log_cursor32[1]=0;
    return;
  }
  memcpy(log_cursor32,saved->reserv+20,8); memcpy(&last,saved->reserv+28,4);
  if(log_lastIndex<last) log_lastIndex=last;
}
void log_ring_reset(void)
{
  log_cursor32[0]=log_cursor32[1]=log_pending32[0]=log_pending32[1]=0;
  device.readLogIDIndex=log_usbReadIndex=log_unacked[0]=log_unacked[1]=0;
  ack.seen[0]=ack.seen[1]=0; log_ring_checkpoint(); log_ring_invalidate();
}
void log_ring_quiet(uint8_t channel)
{
  if(channel>1) return;
  log_pending32[channel]=0; ack.seen[channel]=0;
  if(ack.phase && ack.channel==channel) ack.quiet=1;
}
void log_ring_sent(uint8_t channel,uint8_t uniq,uint32_t next)
{
  log_pending32[channel]=next; ack.last[channel]=uniq; ack.seen[channel]=1;
}
bool log_ring_ack(uint8_t channel,uint8_t uniq)
{
  uint32_t old;
  if(channel>1 || ack.phase) return false;
  if(!log_pending32[channel] || (ack.seen[channel] && ack.last[channel]==uniq)) return true;
  old=log_cursor32[channel]; log_cursor32[channel]=log_pending32[channel]; log_ring_checkpoint();
  bool accepted=store_snapshot();
  log_cursor32[channel]=old; log_ring_checkpoint();
  if(!accepted) return false;
  ack.value=log_pending32[channel]; ack.channel=channel; ack.uniq=uniq; ack.quiet=0; ack.phase=1;
  return false;
}
bool log_ring_ack_poll(void)
{
  uint8_t result,channel,uniq,quiet;
  if(!ack.phase) return false;
  result=mem_config_result();
  if(result==1) { mem_config_poll(timers_getMillis()); return true; }
  channel=ack.channel; uniq=ack.uniq; quiet=ack.quiet; ack.phase=0;
  if(result==2) { log_cursor32[channel]=ack.value; log_pending32[channel]=0; log_ring_checkpoint(); }
  mem_config_reset();
  if(result==2 && !quiet) user_send_logRecords((dataChanel_e)channel,uniq);
  return true;
}
