/* Доработка: Кряжев А., 2026.
   Ведёт регистрацию, статусы и надзор за ответами сервера.
   Канал владеет передачей, обработчики приложения собирают прежние кадры.
   Отсутствие ответа запускает восстановление; журнал фиксирует смену связи.
*/
#ifndef COMM_SERVER_H
#define COMM_SERVER_H
#include <stdint.h>
#include "domovoi_diag.h"
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
/* Подключить запись изменений связи в журнал. */
void comm_server_link(bool (*handler)(uint8_t kind, uint8_t address));
/* Передать событие сети или интернета от восстановления. */
void comm_server_note(uint8_t kind); /* NO_NET, NO_INTERNET - от восстановления */
uint8_t comm_server_address(void);
/* Собрать состояние надзора для стендовой команды 112. */
#if DOMOVOI_DIAG
// Выдать состояние регистрации и счётчики надзора.
uint32_t comm_server_diag(void);
#endif
/* Начать регистрацию нового сеанса канала. */
void comm_server_init(const comm_server_handlers_t *handlers);
/* Продвинуть регистрацию, статусы и надзор ответов сервера. */
void comm_server_poll(uint32_t now_ms);
/* Принять ответ регистрации и сбросить счёт попыток соединения. */
void comm_server_registered(uint8_t success);
/* Принять подходящее подтверждение статуса. */
void comm_server_status_ack(uint8_t unique_id);
/* Начать срок ожидания подтверждения отправленного статуса. */
void comm_server_status_sent(uint8_t unique_id);
/* Сообщить, принял ли сервер регистрацию. */
bool comm_server_online(void);
#endif
