// Состояние асинхронного хранения и передачи фото.
// Буфер sjr принадлежит FO_PHOTO до ответа W25.
// Частичный курсор живёт в RAM, полный ACK — в заголовке.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_RUNTIME_H
#define PHOTO_RUNTIME_H
#include "photo.h"
// Управление операцией, курсоры записи W25, скана очереди и пакета 14.
typedef struct {
  // Фаза, продолжение ввода-вывода, задача и правила отмены.
  uint8_t phase, after, waiting, task, quiet, cancelled, committed;
  uint8_t initialized, cleanup, fault, address, reply_wait;
  // Запись делится по страницам NOR; частичный ACK хранится только в ОЗУ.
  uint16_t offset, io_len, io_done, io_step, resume_offset;
  // Результат скана и проверенные параметры команды 21.
  uint16_t count, requested_offset, first_offset, target_done;
  // Порция пакета 14 и его текущая контрольная сумма.
  uint16_t send_size, send_offset, send_crc;
  // Заголовок занимает 13 байт; четвёртое слово содержит байт полного ACK.
  uint32_t io_address, scan, h[4], target, resume_address;
  uint32_t first, last_number, max_number, requested_number, reply_until, usb_epoch;
  // Частный пакет остаётся у владельца до принятия транспортом или отмены.
  uint8_t *tx;
} photo_runtime_s;
// Сообщить арбитру о занятой флеш.
bool photo_storage_busy(void);
// Выполнить один ограниченный шаг хранения или отправки.
void photo_storage_poll(uint32_t now);
// Отменить запрос после завершения уже принятой операции W25.
void photo_storage_cancel(void);
// Обработать команду 21 и ответить по исходному каналу.
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size);
#endif
