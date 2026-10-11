/* Доработка: Кряжев А., 2026.
   Входящий вызов: проверка доступа и отбой без снятия трубки. */
#ifndef COMM_CALL_H
#define COMM_CALL_H
#include <stdint.h>
#include <stdbool.h>
/* caller проверяет номер и открывает один раз на вызов, до ATH. */
/* volte() - настройка VoLTE (команда 12): модем приводится к ней в каждом запуске. */
void comm_call_init(void (*caller)(const char *number), uint8_t (*volte)(void));
/* access_ready откладывает только действие: таймеры отбоя работают всегда. */
void comm_call_poll(uint32_t now_ms, bool access_ready);
/* Идёт вызов: от первого RING/CLIP до тишины после сброса. */
bool comm_call_active(void);
#endif
