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
/* События журнала связи (лист «Журнал событий» 11, 12, 14-19): адрес 0 - основной, 1 - резервный.
   UP - сервер принял регистрацию, FAIL - TCP не открылся или регистрацию не приняли,
   LOST - связь после регистрации пропала. FAIL, NO_NET, NO_INTERNET - один раз до UP;
   handler вернул false - повтор при следующем случае. */
enum { COMM_LINK_UP, COMM_LINK_FAIL, COMM_LINK_LOST, COMM_LINK_NO_NET, COMM_LINK_NO_INTERNET };
void comm_server_link(bool (*handler)(uint8_t kind, uint8_t address));
void comm_server_note(uint8_t kind); /* NO_NET, NO_INTERNET - от восстановления */
uint8_t comm_server_address(void);
uint32_t comm_server_diag(void);
void comm_server_init(const comm_server_handlers_t *handlers);
void comm_server_poll(uint32_t now_ms);
void comm_server_registered(uint8_t success);
void comm_server_status_ack(uint8_t unique_id);
void comm_server_status_sent(uint8_t unique_id);
bool comm_server_online(void);
#endif
