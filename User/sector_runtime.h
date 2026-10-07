#ifndef SECTOR_RUNTIME_H
#define SECTOR_RUNTIME_H
#include <stdint.h>

typedef uint8_t (*sj_chunk_reader)(const void *, uint16_t, uint8_t *);
typedef struct {
  sj_chunk_reader reader;
  const void *context;
  uint32_t address, expected, crc, header[8];
  uint16_t offset;
  uint8_t phase, after, waiting, committed, cancelled, fault, changed;
  uint8_t data[64];
} sj_runtime_state_s;

extern sj_runtime_state_s sjr;
uint8_t sj_runtime_begin(uint32_t address, sj_chunk_reader reader, const void *context);
uint8_t sj_runtime_poll(uint32_t now);
void sj_runtime_cancel(void);
void sj_runtime_reset(void);
uint8_t sj_runtime_blocked(void);
#endif
