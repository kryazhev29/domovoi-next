/* Shared ID CRC transaction guard. Raw NOR bytes, including torn cells, count.
 * A/B headers protect bank generations against interrupted erase. Records keep
 * the requested {generation, CRC16, inverse} layout. A dirty record precedes
 * every transaction; only verified data contributes a RAM delta. */
#define IDCRC_RAW_IO 1
#include "c/runtime_internal.h"
#include "id_crc_cache.h"
#include "fast_kernels.h"
#include "sector_journal.h"
#define IC_A 0x1FB000u
#define IC_B 0x1FC000u
#define IC_MAGIC 0x35435249u
static struct {
  uint32_t bank, bank_generation, sequence, next, scan;
  uint16_t crc, scan_crc, last_crc, requested_crc;
  uint8_t loaded, known, opened, background, failed, last_valid;
  uint8_t meta, waiting, after, requested_valid, carry, entry_valid;
  uint32_t words[4];
  uint32_t address, bytes, offset;
  const uint8_t *source;
  uint16_t old_crc, new_crc;
  uint8_t io,delta;
} ic;

static bool id_span(uint32_t a,uint32_t n)
{
  return n && a>=mem[MB_ID].start && a-mem[MB_ID].start<mem[MB_ID].size &&
    n<=mem[MB_ID].size-(a-mem[MB_ID].start);
}
/* A durable SJ header can authorize the unchanged bootloader to mutate IDs.
 * Dirty must precede that authority, not merely the later application erase. */
static bool id_journal_header(uint32_t a,const uint8_t *source,uint16_t n)
{
  uint32_t h[3];
  if(a!=SJ_META || !source || n<sizeof(h)) return false;
  memcpy(h,source,sizeof(h));
  return h[0]==SJ_MAGIC && h[1]==SJ_VERSION && id_span(h[2],4096u);
}
static bool header_valid(const uint32_t *h)
{
  return h[0]==IC_MAGIC && h[3]==~IC_MAGIC && h[1]!=0xffffffffu && h[2]==~h[1];
}
/* One bounded read per call. Only the newest fully committed bank is scanned;
 * torn erases in the inactive bank can never promote its generation. */
static uint8_t load_step(void)
{
  uint32_t b[16];
  if(ic.loaded) return 2;
  if(!ic.scan) {
    uint32_t other[4];
    if(!w25_read_data(IC_A,(uint8_t *)b,16) || !w25_read_data(IC_B,(uint8_t *)other,16)) return 3;
    if(header_valid(other) && (!header_valid(b) || other[1]>b[1])) {
      ic.bank=IC_B; ic.bank_generation=other[1];
    } else if(header_valid(b)) { ic.bank=IC_A; ic.bank_generation=b[1]; }
    else { ic.bank=IC_B; ic.bank_generation=0; ic.next=4096; ic.loaded=1; return 2; }
    ic.scan=16; ic.known=ic.last_valid=0; ic.sequence=(ic.bank_generation-1u)*510u;
    return 1;
  }
  uint16_t n=(uint16_t)(4096-ic.scan); if(n>64) n=64;
  if(!w25_read_data(ic.bank+ic.scan,(uint8_t *)b,n)) return 3;
  for(uint16_t i=0;i<n/4;i+=2) {
    uint32_t generation=b[i],value=b[i+1];
    if(generation==0xffffffffu && value==0xffffffffu) {
      ic.next=ic.scan+4u*i; ic.loaded=1; ic.scan=0; return 2;
    }
    ic.known=(uint16_t)value==(uint16_t)~(value>>16) &&
      generation==ic.sequence+1u;
    /* A torn generation never authorizes data; retain a bounded successor. */
    ic.sequence++;
    ic.crc=ic.last_crc=(uint16_t)value; ic.last_valid=ic.known;
  }
  ic.scan+=n;
  if(ic.scan==4096) { ic.next=4096; ic.loaded=1; ic.scan=0; return 2; }
  return 1;
}
static void entry_prepare(bool carry)
{
  uint16_t crc=carry?ic.last_crc:ic.requested_crc;
  ic.entry_valid=carry?ic.last_valid:ic.requested_valid;
  ic.words[0]=ic.sequence+1u;
  ic.words[1]=ic.entry_valid?(uint32_t)crc|((uint32_t)(uint16_t)~crc<<16):0xffffffffu;
  ic.meta=5;
}
static void meta_begin(bool valid,uint16_t crc)
{
  ic.requested_valid=valid; ic.requested_crc=crc; ic.waiting=0;
  ic.carry=ic.next+8u>4096u && ic.bank_generation!=0;
  if(ic.next+8u>4096u) ic.meta=1; else entry_prepare(false);
}
static uint8_t meta_error(void)
{
  ic.failed=1; ic.known=0; ic.meta=ic.waiting=0; return 3;
}
static uint8_t meta_start(uint32_t a,const uint8_t *p,uint16_t n,uint32_t now,uint8_t after)
{
  if(!w25_async_start(a,p,n,now)) return meta_error();
  ic.waiting=1; ic.after=after; return 1;
}
static uint8_t meta_step(uint32_t now)
{
  uint32_t target=ic.bank==IC_A?IC_B:IC_A;
  if(ic.waiting) {
    uint8_t r=w25_async_poll(now); if(r==1) return 1;
    ic.waiting=0; if(r!=2) return meta_error(); ic.meta=ic.after;
    return 1;
  }
  switch(ic.meta) {
  case 1: return meta_start(target,0,0,now,2);
  case 2:
    ic.words[0]=IC_MAGIC; ic.words[1]=ic.bank_generation+1u;
    ic.words[2]=~ic.words[1]; ic.words[3]=~IC_MAGIC;
    return meta_start(target,(uint8_t *)ic.words,16,now,3);
  case 3:
    ic.bank=target; ic.bank_generation++; ic.next=16;
    entry_prepare(ic.carry!=0); return 1;
  case 5: return meta_start(ic.bank+ic.next,(uint8_t *)ic.words,4,now,6);
  case 6:
    if(ic.entry_valid) return meta_start(ic.bank+ic.next+4,(uint8_t *)ic.words+4,4,now,7);
    ic.meta=7; return 1;
  case 7:
    ic.sequence=ic.words[0]; ic.next+=8; ic.last_valid=ic.entry_valid;
    ic.last_crc=(uint16_t)ic.words[1];
    if(ic.carry) { ic.carry=0; entry_prepare(false); return 1; }
    ic.meta=0; return 2;
  default: return meta_error();
  }
}

bool idcrc_io_start(uint32_t address,const uint8_t *source,uint16_t length,uint32_t now)
{
  uint32_t n=source?length:4096u;
  bool delta=id_span(address,n);
  if(!delta && !id_journal_header(address,source,length)) return w25_async_start(address,source,length,now);
  if(ic.io || ic.meta || ic.failed || w25_async_active()) return false;
  ic.address=address; ic.source=source; ic.bytes=n; ic.offset=0;
  ic.delta=delta; ic.old_crc=ic.new_crc=0; ic.io=1; ic.scan=ic.loaded?0:ic.scan;
  return true;
}
static uint8_t io_error(void) { ic.io=0; ic.known=0; return 3; }
uint8_t idcrc_io_poll(uint32_t now)
{
  uint8_t b[64],r; uint16_t n;
  if(!ic.io) return w25_async_poll(now);
  switch(ic.io) {
  case 1:
    r=load_step(); if(r==3) return io_error(); if(r!=2) return 1;
    ic.io=2; return 1;
  case 2:
    if(ic.delta && ic.known && ic.offset<ic.bytes) {
      n=(uint16_t)(ic.bytes-ic.offset); if(n>64) n=64;
      if(!w25_read_data(ic.address+ic.offset,b,n)) return io_error();
      ic.old_crc=crc16_calcInit(b,n,ic.old_crc);
      ic.new_crc=ic.source?crc16_calcInit((uint8_t *)ic.source+ic.offset,n,ic.new_crc):crc16_ffInit(n,ic.new_crc);
      ic.offset+=n; return 1;
    }
    if(ic.opened) ic.io=5;
    else { meta_begin(false,0); ic.io=4; }
    return 1;
  case 4:
    r=meta_step(now); if(r==3) return io_error(); if(r!=2) return 1;
    ic.opened=1; ic.io=5; return 1;
  case 5:
    if(!w25_async_start(ic.address,ic.source,ic.source?(uint16_t)ic.bytes:0,now)) return io_error();
    ic.io=6; return 1;
  case 6:
    r=w25_async_poll(now); if(r==1) return 1;
    if(r!=2) return io_error();
    if(ic.delta && ic.known) {
      uint32_t tail=mem[MB_ID].size-(ic.address-mem[MB_ID].start)-ic.bytes;
      uint16_t delta=ic.old_crc^ic.new_crc;
      ic.crc^=crc16_ffInit(tail,delta)^crc16_ffInit(tail,0);
    }
    ic.io=0; return 2;
  default: return io_error();
  }
}
bool idcrc_io_active(void) { return ic.io || ic.background || w25_async_active(); }
void idcrc_owner_released(void)
{
  if(ic.opened && !ic.io && !flash_owner) ic.background=1;
}
static bool sync_finish(void)
{
  uint8_t r;
  do { wdt_reload(); r=idcrc_io_poll(timers_getMillis()); } while(r==1);
  idcrc_owner_released(); return r==2;
}
bool idcrc_write(uint32_t address,uint8_t *source,uint16_t length)
{
  if(!id_span(address,length) && !id_journal_header(address,source,length)) return w25_write_data(address,source,length);
  while(length) {
    uint16_t n=length>64?64:length,remain=(uint16_t)(256u-(address&255u));
    if(n>remain) n=remain;
    if(!idcrc_io_start(address,source,n,timers_getMillis()) || !sync_finish()) return false;
    address+=n; source+=n; length-=n;
  }
  return true;
}
bool idcrc_erase(uint32_t address)
{
  if(!id_span(address,4096)) return w25_erase_sector(address);
  return idcrc_io_start(address,0,0,timers_getMillis()) && sync_finish();
}
bool idcrc_get(uint32_t offset,uint32_t length,uint16_t *crc)
{
  if(offset || length!=mem[MB_ID].size || !ic.loaded || !ic.known || ic.io || ic.failed) return false;
  *crc=ic.crc; return true;
}
/* Called before the normal storage scheduler. A scan yields after 64 bytes;
 * any writer restarts it. Metadata publication owns FO_CRC until verification. */
bool idcrc_background(void)
{
  uint8_t b[64],r; uint16_t n;
  if((flash_owner && ic.background!=2) || ic.io || ic.failed) return false;
  if(ic.background) {
    uint32_t now=timers_getMillis();
    if(ic.background==1) {
      /* A failed journal commit may still be replayed by the bootloader.
       * Publish only after the recovery intent has been verified empty. */
      if(!mem_tables_ready()) ic.known=0;
      if(ic.known) {
        if(!w25_read_data(SJ_META,b,32)) return true;
        if(!fast_all_ff(b,32)) ic.known=0;
      }
      ic.opened=0; ic.scan=0;
      if(!ic.known) { ic.background=0; return false; }
      if(flash_owner || !flash_claim(FO_CRC)) return false;
      meta_begin(true,ic.crc); ic.background=2; return true;
    }
    r=meta_step(now); if(r==1) return true;
    ic.background=0; flash_release(FO_CRC); return true;
  }
  if(!user_storage_background_ready() || !mem_tables_ready()) return false;
  if(!ic.loaded) { r=load_step(); if(r==3) ic.failed=1; return true; }
  if(ic.known || !mem_tables_ready()) return false;
  if(!ic.scan) ic.scan_crc=65535;
  if(ic.scan<mem[MB_ID].size) {
    n=(uint16_t)((mem[MB_ID].size-ic.scan)>64?64:mem[MB_ID].size-ic.scan);
    if(!w25_read_data(mem[MB_ID].start+ic.scan,b,n)) { ic.scan=0; return true; }
    ic.scan_crc=crc16_calcInit(b,n,ic.scan_crc); ic.scan+=n; return true;
  }
  ic.crc=ic.scan_crc; ic.known=1; ic.background=1; return true;
}

uint8_t idcrc_release_tail(void) { idcrc_owner_released(); return 0; }
