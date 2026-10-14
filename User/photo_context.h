#ifndef PHOTO_CONTEXT_H
#define PHOTO_CONTEXT_H
#include "photo.h"
#define PHOTO_PACKET 512u
#define PHOTO_DATA (PHOTO_PACKET - 6u)
// Контекст и буферы камеры занимают общий рабочий буфер только во время работы.
typedef struct {
  // Номер и адрес снимка; сроки общей съёмки, команды и паузы RS485.
  uint32_t stamp, address, deadline, started, last;
  // Курсор пакета 14 не подтверждает доставку: её фиксирует только команда 21.
  uint32_t readAddress;
  uint16_t size, written, offset, packet, used, frame_size;
  uint8_t stage, retry, syncAck, syncSeen, pending, requestId;
  dataChanel_e channel;
  uint8_t rx[PHOTO_PACKET], frame[PHOTO_PACKET];
} photo_s;
#endif
