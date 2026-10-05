#ifndef PHOTO_RUNTIME_H
#define PHOTO_RUNTIME_H
#include "photo.h"
#include "sector_runtime.h"
/* ABI/data declaration only; implementation is photo_runtime.s. */
typedef struct {
  uint8_t *tx;
  uint32_t io_address, scan, limit, crc, expected, h[4], target, target_slot;
  uint32_t first, last_number, max_number, requested_number, reply_until, usb_epoch;
  uint16_t offset, io_len, io_done, io_step, prefix, confirmed, slot, free_slot;
  uint16_t count, requested_offset, first_offset, target_size, target_done;
  uint16_t send_size, send_offset, send_crc, payload;
  uint8_t phase, after, waiting, erase, task, quiet, cancelled, committed;
  uint8_t initialized, cleanup, fault, address, reply_wait;
} photo_runtime_s;
bool photo_storage_busy(void);
void photo_storage_poll(uint32_t now);
void photo_storage_cancel(void);
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size);
#endif
