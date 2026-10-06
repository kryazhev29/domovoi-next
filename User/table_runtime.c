/* Shared C/ASM/HYBRID table transaction and append-only ID path. */
#include "c/runtime_internal.h"
#include "table_runtime.h"
#include "fast_kernels.h"
#include "user_fun.h"
#include "sched16.h"
extern uint8_t sync_pending,sync_quiet;
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(cellID_s *);
extern void id_watch_end(uint32_t);
static uint16_t id_today(void) { return device.datetime>=1600000000u ? (uint16_t)(device.datetime/86400u) : 0; }
static void id_set_pass_day(cellID_s *c,uint16_t day) { memcpy(c->reserve+4,&day,2); }
static void sync_packet(uint8_t bit) { sync_pending|=bit; sync_quiet=30; }
enum { SYNC_ID=1,SYNC_GROUP=4,SYNC_SHEDULE=8,SYNC_HOLIDAY=16 };
#include "c/table_runtime.inc"

/* All variants and command-17 paths use this single CRC coverage rule. */
uint32_t sched16_crc_size(void) { return 1040u; }

bool sched16_valid(const uint8_t *packet, uint16_t length)
{
  if (length < 24u) return false;
  uint16_t count=packet[9];
  if (!count || count>SCHED16_COUNT || length!=24u+count*SCHED16_RECORD_SIZE) return false;
  for (uint16_t i=0;i<count;i++)
    if (!packet[10u+i*SCHED16_RECORD_SIZE] || packet[10u+i*SCHED16_RECORD_SIZE]>SCHED16_COUNT) return false;
  return true; /* Same field validation policy as the old command. */
}

bool sched16_get(uint16_t number, cellShedule_s *schedule)
{
  if (!number || number>SCHED16_COUNT) return false;
  uint32_t base=mem[MB_SHEDULE].start;
  if (!mem_read(base+(number-1u)*SCHED16_RECORD_SIZE,(uint8_t *)schedule,16) ||
      schedule->index!=number ||
      !mem_read(base+SCHED16_TAIL_OFFSET,(uint8_t *)schedule+16,16)) return false;
  /* Keep the access ABI: scene at 13, onHoliday at 21. Access ignores reserves. */
  memmove(schedule->scene,(uint8_t *)schedule+16,9);
  memset(schedule->reserve,255,sizeof schedule->reserve);
  return true;
}

#define SCHED16_MARK_OFFSET 32u
#define SCHED16_MAGIC 0x36314853u
static const uint32_t sched16_mark[2]={SCHED16_MAGIC,~SCHED16_MAGIC};
void sched16_restore(const device_s *saved)
{ memcpy(device.reserv+SCHED16_MARK_OFFSET,saved->reserv+SCHED16_MARK_OFFSET,8); }

static uint8_t sched16_empty_page(const void *context,uint16_t offset,uint8_t *page)
{
  (void)context;
  uint32_t address=(mem[MB_SHEDULE].start&~4095u)+offset;
  if (address<mem[MB_SHEDULE].start) return w25_read_data(address,page,256);
  memset(page,255,256); return 1;
}

bool sched16_boot(void)
{
  if (!memcmp(device.reserv+SCHED16_MARK_OFFSET,sched16_mark,8)) return true;
  device.datetimeShedule=0;
  /* The absent marker is the durable cleanup intent. Recover the journal
   * first; publish the marker only after verified clearing. */
  if (!mem_tables_ready() ||
      !mem_replace_pages(mem[MB_SHEDULE].start&~4095u,sched16_empty_page,0)) goto bad;
  memcpy(device.reserv+SCHED16_MARK_OFFSET,sched16_mark,8);
  if (user_writeConfigAndDevice()) return true;
bad:
  memset(device.reserv+SCHED16_MARK_OFFSET,0,8);
  table_fault=1;
  return false;
}

bool sched16_clear(void)
{
  /* A factory reset invalidates its existing marker before clearing. */
  device.datetimeShedule=0;
  memset(device.reserv+SCHED16_MARK_OFFSET,0,8);
  if (!user_writeConfigAndDevice()) { table_fault=1; return false; }
  return sched16_boot();
}
