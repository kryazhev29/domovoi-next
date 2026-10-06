// Состояние асинхронного хранения и передачи фото.
// Буфер sjr принадлежит FO_PHOTO до ответа W25.
// Частичный курсор живёт в RAM, полный ACK — в заголовке.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_RUNTIME_H
#define PHOTO_RUNTIME_H
#include "photo.h"
#include "sector_runtime.h"
// Флаги и короткие поля в начале дают короткие инструкции Thumb.
typedef struct {
  uint8_t phase, after, waiting, erase, task, quiet, cancelled, committed;
  uint8_t initialized, cleanup, fault, address, reply_wait, frame_size;
  uint16_t offset, io_len, io_done, io_step, confirmed, resume_offset;
  uint16_t count, requested_offset, first_offset, target_size, target_done;
  uint16_t send_size, send_offset, send_crc, payload;
  uint32_t io_address, scan, h[4], target, resume_address;
  uint32_t first, last_number, max_number, requested_number, reply_until, usb_epoch;
  uint8_t *tx;
} photo_runtime_s;
bool photo_storage_busy(void);
void photo_storage_poll(uint32_t now);
void photo_storage_cancel(void);
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size);
#endif
