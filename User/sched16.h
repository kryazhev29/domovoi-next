#ifndef SCHED16_H
#define SCHED16_H
#include "includes.h"
#define SCHED16_RECORD_SIZE 16u
#define SCHED16_COUNT 64u
#define SCHED16_TAIL_OFFSET (SCHED16_COUNT * SCHED16_RECORD_SIZE)
#define SCHED16_TAIL_SIZE 16u
uint32_t sched16_crc_size(void);
bool sched16_valid(const uint8_t *packet, uint16_t length);
bool sched16_get(uint16_t number, cellShedule_s *schedule);
void sched16_restore(const device_s *saved);
bool sched16_boot(void);
bool sched16_clear(void);
#endif
