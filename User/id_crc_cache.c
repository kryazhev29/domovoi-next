/* Volatile raw-byte ID CRC. Power loss always discards the cache. */
#define IDCRC_RAW_IO 1
#include "c/runtime_internal.h"
#include "id_crc_cache.h"

/* Existing NOR job ABI, also exported by the frozen assembly driver.
 * An idle job holds the command while old bytes are read before raw start. */
extern struct {
  const uint8_t *data;
  uint32_t address, started;
  uint16_t length, checked;
  uint8_t active;
} w25_job;
static struct {
  uint32_t scan;
  uint16_t crc, partial, delta;
  uint8_t state; /* 0 unknown, 1 known, 2 old-byte scan, 3 NOR verify */
} ic;
RT_ASSERT(idcrc_ram, sizeof(ic)<=16);

static void invalidate(void) { ic.state=0; ic.scan=0; }
void idcrc_unknown(void)
{
  /* A cancelled owner still drains its in-flight NOR command. Keep that state
   * but forbid publishing its delta when verification completes. */
  ic.state=ic.state>1?ic.state|4u:0; ic.scan=0;
}
static bool id_span(uint32_t a,uint32_t n)
{
  uint32_t offset=a-mem[MB_ID].start;
  return n && offset<mem[MB_ID].size && n<=mem[MB_ID].size-offset;
}
bool idcrc_io_start(uint32_t address,const uint8_t *source,uint16_t length,uint32_t now)
{
  uint32_t n=source?length:4096u;
  if(ic.state>1 || w25_async_active()) return false;
  if(!id_span(address,n)) return w25_async_start(address,source,length,now);
  ic.scan=0;
  if(!ic.state) return w25_async_start(address,source,length,now);
  w25_job.address=address; w25_job.data=source; w25_job.length=(uint16_t)n;
  w25_job.checked=0; ic.delta=0; ic.state=2;
  return true;
}
uint8_t idcrc_io_poll(uint32_t now)
{
  uint8_t b[64],r;
  if((ic.state&3u)==2) {
    uint16_t n=w25_job.length-w25_job.checked;
    if(n) {
      if(n>64) n=64;
      if(!w25_read_data(w25_job.address+w25_job.checked,b,n)) goto failed;
      for(uint16_t i=0;i<n;i++) b[i]^=w25_job.data?w25_job.data[w25_job.checked+i]:255u;
      ic.delta=crc16_calcInit(b,n,ic.delta); w25_job.checked+=n;
      return 1;
    }
    n=ic.delta;
    uint32_t tail=mem[MB_ID].size-(w25_job.address-mem[MB_ID].start)-w25_job.length;
    ic.delta=crc16_ffInit(tail,n)^crc16_ffInit(tail,0);
    if(!w25_async_start(w25_job.address,w25_job.data,w25_job.data?w25_job.length:0,now)) goto failed;
    ic.state=(ic.state&4u)|3u; return 1;
  }
  r=w25_async_poll(now);
  if(r==3) goto failed;
  if(r==2 && ic.state>1) {
    if(ic.state&4u) ic.state=0;
    else { ic.crc^=ic.delta; ic.state=1; }
  }
  return r;
failed:
  invalidate(); return 3;
}
bool idcrc_io_active(void) { return ic.state>1 || w25_async_active(); }
uint8_t idcrc_release_tail(void)
{
  if(!mem_tables_ready()) invalidate();
  return 0;
}
static bool sync_finish(void)
{
  uint8_t r;
  do { wdt_reload(); r=idcrc_io_poll(timers_getMillis()); } while(r==1);
  return r==2;
}
bool idcrc_write(uint32_t address,uint8_t *source,uint16_t length)
{
  if(!id_span(address,source?length:4096u))
    return source?w25_write_data(address,source,length):w25_erase_sector(address);
  do {
    uint16_t n=64u-(address&63u);
    if(n>length) n=length;
    if(!idcrc_io_start(address,source,n,timers_getMillis()) || !sync_finish()) { invalidate(); return false; }
    length-=n;
    if(length) { address+=n; source+=n; }
  } while(length);
  return true;
}
bool idcrc_erase(uint32_t address)
{
  return idcrc_write(address,0,0);
}
bool idcrc_get(uint32_t offset,uint32_t length,uint16_t *crc)
{
  if(offset || length!=mem[MB_ID].size || ic.state!=1) return false;
  *crc=ic.crc; return true;
}
/* One 64-byte read per pass, below 1 ms. Release ownership before another
 * storage task runs; a competing reader never restarts the scan. */
bool idcrc_background(void)
{
  uint8_t b[64]; uint32_t end=id_used_bytes;
  if(ic.state || !user_storage_background_ready() || !mem_tables_ready()) return false;
  if(end>mem[MB_ID].size) end=mem[MB_ID].size;
  if(!ic.scan) ic.partial=65535;
  if(ic.scan<end) {
    uint16_t n=end-ic.scan>64?64:(uint16_t)(end-ic.scan);
    if(!flash_claim(FO_CRC)) return false;
    if(w25_read_data(mem[MB_ID].start+ic.scan,b,n)) {
      ic.partial=crc16_calcInit(b,n,ic.partial); ic.scan+=n;
    } else ic.scan=0;
    flash_release(FO_CRC);
  } else { ic.crc=crc16_ffInit(mem[MB_ID].size-ic.scan,ic.partial); ic.state=1; }
  return false;
}
