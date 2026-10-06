#ifndef DOMOVOI_LOG_RING_H
#define DOMOVOI_LOG_RING_H
#include "includes.h"
#define LOG_RING_MAGIC 0xD74C4752u
#define LOG_RING_SLOTS 127u
#define LOG_RING_SECTORS 24u
typedef struct { cellLog_s cell; uint32_t address,previous,retry_at; uint8_t phase,waiting,result; } log_job_s;
extern log_job_s log_job;
bool log_async_active(void);
void log_async_begin(const cellLog_s *);
uint8_t log_async_step(uint32_t);
void log_uniqueIndex(cellLog_s *,uint32_t);
bool log_readRecords(uint16_t,uint8_t *,uint16_t);
uint8_t log_ring_init_step(void);
uint32_t log_ring_first(void),log_ring_next(uint32_t);
void log_ring_invalidate(void);
bool log_ring_migrate(void);
void log_ring_checkpoint(void),log_ring_restore(const device_s *),log_ring_reset(void),log_ring_quiet(uint8_t);
bool log_ring_ack(uint8_t,uint8_t),log_ring_ack_poll(void);
extern uint32_t log_cursor32[2],log_pending32[2];
#endif
