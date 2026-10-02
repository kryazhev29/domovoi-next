/* Доработка: Кряжев А., 2026. Регистрация, статус и надзор сервера. */
#ifndef COMM_SERVER_H
#define COMM_SERVER_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {
  void (*registration)(void);
  void (*status)(void);
  void (*session)(bool open);
  void (*process)(void);
  bool (*no_traffic)(void); /* true только после сохранённого события 13. */
} comm_server_handlers_t;
/* Ни ping, ни DNS не прошли: вместо переоткрытия TCP зовётся handler. */
void comm_server_no_internet(void (*handler)(void));
void comm_server_init(const comm_server_handlers_t *handlers);
void comm_server_poll(uint32_t now_ms);
void comm_server_registered(uint8_t success);
void comm_server_status_ack(uint8_t unique_id);
void comm_server_status_sent(uint8_t unique_id);
bool comm_server_online(void);
#endif
