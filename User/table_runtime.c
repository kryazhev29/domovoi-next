/* Shared C/ASM/HYBRID table transaction and append-only ID path. */
#include "c/runtime_internal.h"
#include "table_runtime.h"
#include "fast_kernels.h"
#include "user_fun.h"
extern uint8_t sync_pending,sync_quiet;
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(cellID_s *);
extern void id_watch_end(uint32_t);
static uint16_t id_today(void) { return device.datetime>=1600000000u ? (uint16_t)(device.datetime/86400u) : 0; }
static void id_set_pass_day(cellID_s *c,uint16_t day) { memcpy(c->reserve+4,&day,2); }
static void sync_packet(uint8_t bit) { sync_pending|=bit; sync_quiet=30; }
enum { SYNC_ID=1,SYNC_GROUP=4,SYNC_SHEDULE=8,SYNC_HOLIDAY=16 };
#include "c/table_runtime.inc"
