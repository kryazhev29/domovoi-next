/* Independent C specification. NEVER compiled in a production project.
 * The checked-in IAR Thumb-2 module is the executable implementation. */
#include "sector_runtime.h"
#include "sector_journal.h"
#include "flash_w25.h"
#include <string.h>

sj_runtime_state_s sjr;

uint8_t sj_runtime_blocked(void) { return sjr.fault || (sjr.phase && sjr.phase < 30); }
void sj_runtime_cancel(void) { sjr.cancelled = 1; }
void sj_runtime_reset(void) { if (!sjr.waiting) sjr.phase = 0; }
uint8_t sj_runtime_begin(uint32_t address, sj_chunk_reader reader, const void *context)
{
  if (sjr.phase || sjr.fault || !reader || !sj_destination(address) || w25_async_active()) return 0;
  sjr.address=address; sjr.reader=reader; sjr.context=context;
  sjr.offset=0; sjr.expected=0xffffffffu; sjr.committed=sjr.cancelled=0;
  sjr.changed=0; sjr.phase=16; return 1;
}
static void sjr_fail(void)
{
  if (sjr.committed) sjr.fault=1; /* Only startup recovery may resolve uncertainty. */
  sjr.phase=31;
}
static void sjr_start(uint32_t address, const uint8_t *data, uint16_t n, uint32_t now, uint8_t after)
{
  if (!w25_async_start(address,data,n,now)) { sjr_fail(); return; }
  sjr.waiting=1; sjr.after=after;
}
uint8_t sj_runtime_poll(uint32_t now)
{
  if (!sjr.phase) return 0;
  if (sjr.waiting) {
    uint8_t r=w25_async_poll(now);
    if (r==1) return 1;
    sjr.waiting=0;
    if (r!=2) sjr_fail(); else sjr.phase=sjr.after;
    return sjr.phase==31 ? 3 : 1;
  }
  if (sjr.cancelled && !sjr.committed && sjr.phase<30) sjr.phase=31;
  switch (sjr.phase) {
  case 16:
    if (!sjr.reader(sjr.context,sjr.offset,sjr.data) ||
        !w25_read_data(sjr.address+sjr.offset,sjr.check,64)) { sjr_fail(); break; }
    if (memcmp(sjr.data,sjr.check,64)) sjr.changed=1;
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.phase=sjr.changed ? 1 : 30; }
    break;
  case 1: sjr_start(SJ_META,0,0,now,2); break;
  case 2: sjr_start(SJ_DATA,0,0,now,3); break;
  case 3:
    if (!sjr.reader(sjr.context,sjr.offset,sjr.data)) { sjr_fail(); break; }
    sjr.expected=crc32_update(sjr.expected,sjr.data,64);
    sjr_start(SJ_DATA+sjr.offset,sjr.data,64,now,4); break;
  case 4:
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.crc=0xffffffffu; sjr.phase=5; }
    else sjr.phase=3;
    break;
  case 5:
    if (!w25_read_data(SJ_DATA+sjr.offset,sjr.check,64)) { sjr_fail(); break; }
    sjr.crc=crc32_update(sjr.crc,sjr.check,64); sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) {
      if (sjr.crc!=sjr.expected) { sjr_fail(); break; }
      sjr.header[0]=SJ_MAGIC; sjr.header[1]=SJ_VERSION; sjr.header[2]=sjr.address;
      sjr.header[3]=SJ_SECTOR; sjr.header[4]=~sjr.expected; sjr.header[5]=sjr.expected;
      sjr.header[6]=~crc32_update(0xffffffffu,(uint8_t*)sjr.header,24);
      sjr.header[7]=SJ_COMMIT; sjr.phase=6;
    }
    break;
  case 6: sjr_start(SJ_META,(uint8_t*)sjr.header,28,now,7); break;
  case 7:
    if (!w25_read_data(SJ_META,sjr.check,32) || memcmp(sjr.header,sjr.check,28) ||
        sjr.check[28]!=255 || sjr.check[29]!=255 || sjr.check[30]!=255 || sjr.check[31]!=255) {
      sjr_fail(); break;
    }
    sjr.phase=8; break;
  case 8:
    sjr.committed=1; /* Marker write may have reached NOR even if its check fails. */
    sjr_start(SJ_META+28,(uint8_t*)sjr.header+28,4,now,9); break;
  case 9:
    if (!w25_read_data(SJ_META,sjr.check,32) || memcmp(sjr.header,sjr.check,32)) { sjr_fail(); break; }
    sjr.phase=10; break;
  case 10: sjr_start(sjr.address,0,0,now,11); break;
  case 11: sjr.offset=0; sjr.phase=12; break;
  case 12:
    if (!w25_read_data(SJ_DATA+sjr.offset,sjr.data,64)) { sjr_fail(); break; }
    sjr_start(sjr.address+sjr.offset,sjr.data,64,now,13); break;
  case 13:
    sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) { sjr.offset=0; sjr.crc=0xffffffffu; sjr.phase=14; }
    else sjr.phase=12;
    break;
  case 14:
    if (!w25_read_data(sjr.address+sjr.offset,sjr.check,64) ||
        !w25_read_data(SJ_DATA+sjr.offset,sjr.data,64) || memcmp(sjr.data,sjr.check,64)) { sjr_fail(); break; }
    sjr.crc=crc32_update(sjr.crc,sjr.check,64); sjr.offset+=64;
    if (sjr.offset==SJ_SECTOR) {
      if (sjr.crc!=sjr.expected) sjr_fail(); else sjr.phase=15;
    }
    break;
  case 15: sjr_start(SJ_META,0,0,now,30); break;
  case 30: return 2;
  case 31: return 3;
  default: sjr_fail(); break;
  }
  return sjr.phase==31 ? 3 : 1;
}
