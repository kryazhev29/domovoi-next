/* Доработка: Кряжев А., 2026.
   Обрабатывает входящий номер и короткий голосовой ответ.
   Проверка доступа предшествует ATA; таймеры отбоя не зависят от занятости флеш.
   Голос ждёт свободного AT, а настройка VoLTE выполняется вне вызова.
*/
#ifndef COMM_CALL_H
#define COMM_CALL_H
#include <stdint.h>
#include <stdbool.h>
/* caller - проверка номера и открытие до ATA, один раз на вызов.
   После OK ответ длится CALL_HOLD; аварийный CHUP до CALL_LIMIT от ATA. */
/* volte() - настройка VoLTE (команда 12): модем приводится к ней в каждом запуске. */
void comm_call_init(bool (*caller)(const char *number), uint8_t (*volte)(void));
/* access_ready откладывает только действие: таймеры отбоя работают всегда. */
void comm_call_poll(uint32_t now_ms, bool access_ready);
/* Идёт вызов: от первого RING/CLIP до тишины после сброса. */
bool comm_call_active(void);
#endif
