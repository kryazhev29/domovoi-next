// Доработка: Кряжев А., 2026
// Расписания: 64 записи по 16 Б и общий хвост 16 Б.
// CRC охватывает все 1040 Б; внешний кадр проверяется до начала записи.
// Переход формата фиксируется настройками после журнальной очистки.
#ifndef SCHED16_H
#define SCHED16_H
#include "includes.h"
#define SCHED16_RECORD_SIZE 16u
#define SCHED16_COUNT 64u
#define SCHED16_TAIL_OFFSET (SCHED16_COUNT * SCHED16_RECORD_SIZE)
#define SCHED16_TAIL_SIZE 16u
// Возвращает общую длину записей и хвоста для CRC.
uint32_t sched16_crc_size(void);
// Проверяет длину, количество и номера расписаний входящего кадра.
bool sched16_valid(const uint8_t *packet, uint16_t length);
// Читает запись и общий хвост: 0 — записи нет, 1 — прочитана, 2 — отказ чтения.
uint8_t sched16_get(uint16_t number, cellShedule_s *schedule);
// Восстанавливает маркер завершённого перехода формата.
void sched16_restore(const device_s *saved);
// Очищает старый формат и публикует маркер после журнальной записи.
bool sched16_boot(void);
// Готовит маркер заводской очистки; сохранением владеет STORE.
void sched16_reset_mark(bool complete);

#endif
