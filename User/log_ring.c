/* Sector log shared by C, ASM and HYBRID. The first 8 header bytes are magic
 * and generation. The rest of the first 32-byte slot protects generations,
 * carries the preceding record number and commits a rotation intent. */
#include "c/runtime_internal.h"
#include "log_ring.h"
#include "user_fun.h"
#include "fast_kernels.h"
extern bool log_blank(const cellLog_s *),log_valid(const cellLog_s *);
extern uint32_t log_lastIndex;
log_job_s log_job;
static struct {
  uint32_t seq[24],maximum,header[8],word;
  uint16_t count[24],position,low,high,scan;
  uint8_t head,target,forced,part,init,ready,after;
} lr;
#define NONE 255u
static uint32_t sector(uint8_t n) { return mem[MB_LOG].start+4096u*n; }
static bool header_valid(const uint32_t *h)
{
  return h[0]==LOG_RING_MAGIC && h[1] && h[1]!=0xffffffffu &&
    h[2]==~h[1] && h[4]==~h[3];
}
static bool pair_valid(uint32_t v) { return (uint16_t)v==(uint16_t)~(v>>16); }
static uint32_t pair(uint16_t v) { return v|((uint32_t)(uint16_t)~v<<16); }
void log_ring_invalidate(void) { lr.init=lr.ready=0; }
uint8_t log_ring_init_step(void)
{
  uint32_t h[8]; cellLog_s cell;
  if(lr.ready) return 2;
  if(!lr.init) {
    if((mem[MB_LOG].start&4095u) || mem[MB_LOG].size!=24u*4096u) return 3;
    memset(lr.seq,0,sizeof(lr.seq)); memset(lr.count,0,sizeof(lr.count));
    lr.head=lr.forced=NONE; lr.maximum=0; lr.part=0; lr.init=1;
  }
  if(lr.init==1) {
    if(!w25_read_data(sector(lr.part),(uint8_t *)h,32)) return 3;
    if(header_valid(h)) {
      lr.seq[lr.part]=h[1];
      if(pair_valid(h[5]) && (uint16_t)h[5]<=127) lr.count[lr.part]=(uint16_t)h[5];
      if(h[1]>lr.maximum) { lr.maximum=h[1]; lr.head=lr.part; memcpy(lr.header,h,32); }
    }
    if(++lr.part<24) return 1;
    if(lr.head==NONE) { lr.position=0; device.logsCount=0; device.newAddressLog=mem[MB_LOG].start+32u; device.status.readyLogData=0; lr.ready=1; return 2; }
    /* A verified intent removes the entire old tail, even when an interrupted
     * erase leaves its header intact. No record offset is used as an ACK. */
    if(pair_valid(lr.header[6]) && (uint16_t)lr.header[6]<24 && (uint16_t)lr.header[6]!=lr.head) {
      lr.forced=(uint8_t)lr.header[6]; lr.seq[lr.forced]=0; lr.count[lr.forced]=0;
    }
    lr.low=0; lr.high=127; lr.init=2; return 1;
  }
  if(lr.init==2) {
    if(lr.low<lr.high) {
      uint16_t middle=(uint16_t)((lr.low+lr.high)/2);
      if(!w25_read_data(sector(lr.head)+32u+32u*middle,(uint8_t *)&cell,32)) return 3;
      if(log_blank(&cell)) lr.high=middle; else lr.low=middle+1;
      return 1;
    }
    lr.position=lr.low; lr.scan=0; lr.count[lr.head]=0;
    if(log_lastIndex<lr.header[3]) log_lastIndex=lr.header[3];
    lr.init=3; return 1;
  }
  /* Only the head is counted/validated. Closed-sector counts are in headers.
   * This also skips interrupted body writes without treating them as a hole. */
  if(lr.scan<lr.position) {
    if(!w25_read_data(sector(lr.head)+32u+32u*lr.scan++,(uint8_t *)&cell,32)) return 3;
    if(log_blank(&cell)) lr.position=lr.scan-1u;
    if(log_valid(&cell)) { lr.count[lr.head]++; if(log_lastIndex<cell.index) log_lastIndex=cell.index; }
    return 1;
  }
  device.logsCount=0;
  for(uint8_t i=0;i<24;i++) device.logsCount+=lr.count[i];
  device.newAddressLog=sector(lr.head)+32u+32u*lr.position;
  device.status.readyLogData=device.logsCount!=0; lr.ready=1; return 2;
}
static uint8_t next_sector(uint32_t seq)
{
  uint8_t found=NONE;
  for(uint8_t i=0;i<24;i++)
    if(lr.seq[i]>seq && (found==NONE || lr.seq[i]<lr.seq[found])) found=i;
  return found;
}
uint32_t log_ring_first(void)
{
  uint8_t n=next_sector(0); return n==NONE?0:sector(n)+32;
}
uint32_t log_ring_next(uint32_t address)
{
  uint8_t n=(uint8_t)((address-mem[MB_LOG].start)/4096u);
  uint32_t offset=(address-mem[MB_LOG].start)&4095u;
  if(n>=24) return 0;
  if(offset+32u<4096u && (n!=lr.head || offset/32u<lr.position)) return address+32u;
  n=next_sector(lr.seq[n]); return n==NONE?0:sector(n)+32u;
}
uint16_t log_getRecordsCount(void)
{
  uint8_t r;
  if(!log_ring_migrate()) return 0xffff;
  log_ring_invalidate();
  do { wdt_reload(); r=log_ring_init_step(); } while(r==1);
  return r==2?device.logsCount:0xffff;
}
bool log_readRecords(uint16_t first,uint8_t *data,uint16_t count)
{
  cellLog_s cell; uint32_t a=log_ring_first();
  while(a && count) {
    if(!w25_read_data(a,(uint8_t *)&cell,32)) return false;
    if(log_valid(&cell)) {
      if(first) first--;
      else { memset(cell.reserve,0,8); memcpy(data,&cell,32); data+=32; count--; }
    }
    a=log_ring_next(a);
  }
  return count==0;
}
void log_uniqueIndex(cellLog_s *cell,uint32_t address)
{
  (void)address;
  if(log_lastIndex && cell->index<=log_lastIndex) cell->index=log_lastIndex+1u;
}
bool log_async_active(void) { return log_job.phase!=0; }
void log_async_begin(const cellLog_s *cell)
{
  log_job.cell=*cell; log_job.previous=log_lastIndex; log_job.waiting=0;
  log_job.phase=flash_claim(FO_LOG)?1:0; log_job.result=log_job.phase?1:3;
}
static uint8_t fail(uint32_t now,bool rotation)
{
  log_job.result=3; log_job.retry_at=now+1000u;
  if(rotation) { log_ring_invalidate(); log_job.phase=1; }
  else log_job.phase=6;
  return 3;
}
static bool start(uint32_t a,const uint8_t *p,uint16_t n,uint32_t now,uint8_t after)
{
  if(!w25_async_start(a,p,n,now)) return false;
  lr.after=after; log_job.waiting=1; return true;
}
static void target_select(void)
{
  uint8_t n=lr.forced;
  if(n==NONE) {
    for(n=0;n<24;n++) if(!lr.seq[n]) break;
    if(n==24) n=next_sector(0);
  }
  lr.target=n;
}
uint8_t log_async_step(uint32_t now)
{
  cellLog_s actual; uint8_t result; uint16_t crc;
  if(!log_job.phase) return log_job.result;
  if(log_job.result==3 && (int32_t)(now-log_job.retry_at)<0) return 3;
  if(log_job.waiting) {
    result=w25_async_poll(now); if(result==1) return 1;
    log_job.waiting=0;
    if(result!=2) return fail(now,log_job.phase>=10);
    log_job.phase=lr.after; return log_job.result=1;
  }
  if(!w25_read_available()) return 1;
  switch(log_job.phase) {
  case 1:
    result=log_ring_init_step(); if(result==3) return fail(now,true);
    if(result!=2) return 1;
    log_job.phase=2; return 1;
  case 2:
    if(lr.head==NONE || lr.position==127 || lr.forced!=NONE) {
      target_select(); log_job.phase=lr.head==NONE?12:10; return 1;
    }
    log_job.address=sector(lr.head)+32u+32u*lr.position;
    if(!w25_read_data(log_job.address,(uint8_t *)&actual,32)) return fail(now,true);
    if(!log_blank(&actual)) { lr.position++; return 1; }
    log_uniqueIndex(&log_job.cell,log_job.address);
    crc=crc16_calc((uint8_t *)&log_job.cell,24);
    memcpy(log_job.cell.reserve,&crc,2); crc=(uint16_t)~crc;
    memcpy(log_job.cell.reserve+2,&crc,2);
    lr.word=0x32474f4cu; memcpy(log_job.cell.reserve+4,&lr.word,4);
    log_job.phase=3; return 1;
  case 3:
    if(!start(log_job.address,(uint8_t *)&log_job.cell,28,now,4)) return fail(now,false);
    return 1;
  case 4:
    if(!start(log_job.address+28,(uint8_t *)&log_job.cell+28,4,now,5)) return fail(now,false);
    return 1;
  case 5:
  case 6:
    if(!w25_read_data(log_job.address,(uint8_t *)&actual,32)) return fail(now,false);
    if(!memcmp(&actual,&log_job.cell,32) && log_valid(&actual)) {
      log_lastIndex=actual.index; lr.position++; lr.count[lr.head]++; device.logsCount++;
      device.newAddressLog=log_job.address+32; device.status.readyLogData=1;
      log_job.phase=0; flash_release(FO_LOG); return log_job.result=2;
    }
    if(log_job.phase==5) return fail(now,false);
    for(uint8_t i=0;i<32;i++) if((((uint8_t *)&actual)[i]&((uint8_t *)&log_job.cell)[i])!=((uint8_t *)&log_job.cell)[i]) {
      if(log_valid(&actual)) { if(log_lastIndex<actual.index) log_lastIndex=actual.index; lr.count[lr.head]++; device.logsCount++; }
      lr.position++; log_job.phase=2; return log_job.result=1;
    }
    log_job.phase=3; return log_job.result=1;
  case 10:
    lr.word=pair(lr.count[lr.head]);
    if(!start(sector(lr.head)+20,(uint8_t *)&lr.word,4,now,11)) return fail(now,true);
    return 1;
  case 11:
    lr.word=pair(lr.target);
    if(!start(sector(lr.head)+24,(uint8_t *)&lr.word,4,now,12)) return fail(now,true);
    return 1;
  case 12:
    if(!start(sector(lr.target),0,0,now,13)) return fail(now,true);
    device.logsCount-=lr.count[lr.target]; lr.count[lr.target]=0; lr.seq[lr.target]=0;
    return 1;
  case 13:
    lr.header[0]=LOG_RING_MAGIC; lr.header[1]=lr.maximum+1u; lr.header[2]=~lr.header[1];
    lr.header[3]=log_lastIndex; lr.header[4]=~log_lastIndex;
    lr.header[5]=lr.header[6]=lr.header[7]=0xffffffffu;
    if(!start(sector(lr.target)+4,(uint8_t *)lr.header+4,16,now,14)) return fail(now,true);
    return 1;
  case 14:
    if(!start(sector(lr.target),(uint8_t *)lr.header,4,now,15)) return fail(now,true);
    return 1;
  case 15:
    lr.head=lr.target; lr.maximum=lr.header[1]; lr.seq[lr.head]=lr.maximum;
    lr.position=0; lr.count[lr.head]=0; lr.forced=NONE;
    log_job.phase=2; return 1;
  default: return fail(now,true);
  }
}
bool log_addEvent(cellLog_s *cell)
{
  if(!user_storage_flash_ready()) return false;
  log_async_begin(cell);
  while(log_async_active()) {
    wdt_reload(); uint8_t r=log_async_step(timers_getMillis());
    if(r==3) { log_job.phase=0; flash_release(FO_LOG); log_ring_invalidate(); return false; }
  }
  if(log_job.result!=2) return false;
  cell->index=log_job.cell.index; return true;
}
bool log_clearLogBuffer(void)
{
  for(uint8_t i=0;i<24;i++) { if(!mem_erase_sector(sector(i))) return false; wdt_reload(); }
  log_ring_reset(); device.logsCount=0; device.status.readyLogData=0;
  device.newAddressLog=mem[MB_LOG].start+32; return true;
}
