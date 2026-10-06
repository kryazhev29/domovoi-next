/* Durable logical record numbers; legacy device fields and wire frames keep
 * their layout. Reserved device bytes 16..31 hold the new checkpoint. */
#include "c/runtime_internal.h"
#include "log_ring.h"
#include "user_fun.h"
extern uint32_t log_lastIndex;
extern bool store_snapshot(void);
extern uint8_t store_save_requested;
extern bool log_blank(const cellLog_s *),log_valid(const cellLog_s *);
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
    uint16_t ordinal[2],seen=0; cellLog_s cell;
    log_cursor32[0]=log_cursor32[1]=0;
    memcpy(&magic,saved->reserv,4);
    if(magic!=0x314C5055u) return;
    memcpy(&ordinal[0],saved->reserv+4,2); ordinal[1]=saved->readLogIDIndex;
    /* Upgrade the old ordinal checkpoint before the legacy log is moved.
     * Unknown/corrupt positions replay from the oldest live record. */
    for(uint16_t i=0;i<3072 && (seen<ordinal[0] || seen<ordinal[1]);i++) {
      if(!w25_read_data(0x1CF400u+32u*i,(uint8_t *)&cell,32)) {
        log_cursor32[0]=log_cursor32[1]=0; return;
      }
      if(log_blank(&cell)) break;
      if(!log_valid(&cell)) continue;
      seen++;
      for(uint8_t ch=0;ch<2;ch++) if(seen==ordinal[ch]) log_cursor32[ch]=cell.index+1u;
    }
    log_ring_checkpoint(); store_save_requested=1; return;
  }
  memcpy(log_cursor32,saved->reserv+20,8); memcpy(&last,saved->reserv+28,4);
  if(log_lastIndex<last) log_lastIndex=last;
}
void log_ring_reset(void)
{
  log_cursor32[0]=log_cursor32[1]=log_pending32[0]=log_pending32[1]=0;
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
