// Состояние асинхронного хранения и передачи фото.
// Буфер sjr принадлежит FX_PHOTO до verify W25.
// Частичный курсор живёт в RAM, полный ACK — в заголовке.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_RUNTIME_H
#define PHOTO_RUNTIME_H
#include "photo.h"
// Постоянные курсоры очереди и потока; transient находится в union исполнителя.
typedef struct {
  uint32_t last_number, resume_address, usb_epoch;
  // Частный пакет остаётся у владельца до принятия транспортом или отмены.
  uint8_t *tx;
  uint16_t resume_offset;
  uint8_t initialized, cleanup, fault;
  uint16_t tx_length;
  uint32_t tx_address, tx_until;
} photo_runtime_s;
// Выполнить один ограниченный шаг хранения или отправки.
void photo_storage_poll(uint32_t now);
// Отменить запрос после завершения уже принятой операции W25.
void photo_storage_cancel(void);
// Сбросить курсоры после фактической заводской очистки области фото.
void photo_storage_reset(void);
// Обработать команду 21 и ответить по исходному каналу.
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size);
#endif
