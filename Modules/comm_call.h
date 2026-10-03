/* Доработка: Кряжев А., 2026. Входящий вызов без ответа: номер, доступ, сброс. */
#ifndef COMM_CALL_H
#define COMM_CALL_H
#include <stdint.h>
#include <stdbool.h>
/* caller - проверка номера и открытие, один раз на вызов, из главного цикла.
   Трубка не снимается: вызов сбрасывается AT+CHUP, звонящий не платит. */
/* volte() - настройка VoLTE (команда 12): модем приводится к ней в каждом запуске. */
void comm_call_init(void (*caller)(const char *number), uint8_t (*volte)(void));
void comm_call_poll(uint32_t now_ms);
/* Идёт вызов: от первого RING/CLIP до тишины после сброса. */
bool comm_call_active(void);
#endif
