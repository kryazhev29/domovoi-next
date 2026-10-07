/* Доработка: Кряжев А., 2026.
   Описывает команды и шаги запуска SLM336 платы v5.
   Переходы таблицы сохраняют порядок питания, SIM, сети и PDP.
   Профилей других модемов и выбора драйвера нет.
*/
#ifndef MODEM_SLM336_H
#define MODEM_SLM336_H
#include <stdint.h>
#include "domovoi_diag.h"
#include <stdbool.h>
#include "comm_power_v5.h"
#include "comm_at.h"
enum {
  SLM_DISCHARGE, SLM_POWER_LEAD, SLM_POWER_RAIL, SLM_POWER_KEY, SLM_POWER_BOOT,
  SLM_AT_READY, SLM_MODEL, SLM_IMEI, SLM_ECHO, SLM_CMEE, SLM_IFC, SLM_CLIP,
  SLM_CMER, SLM_CREG_URC, SLM_CGREG_URC, SLM_CEREG_URC, SLM_URC_PORT,
  SLM_URC_SET, SLM_SIM, SLM_RAT_QUERY, SLM_RAT_SET, SLM_NETWORK,
  SLM_IMSI, SLM_APN, SLM_DEACT, SLM_ACTIVATE, SLM_DATA_ECHO, SLM_TCP, SLM_READ, SLM_STEP_COUNT
};
enum { SLM_PHASE_POWER, SLM_PHASE_SETUP, SLM_PHASE_SIM, SLM_PHASE_NETWORK,
       SLM_PHASE_PDP, SLM_PHASE_TCP, SLM_PHASE_COUNT };
enum { SLM_IDLE, SLM_STARTING, SLM_READY, SLM_FAILED };
enum { SLM_ERR_NONE, SLM_ERR_TIMEOUT, SLM_ERR_AT, SLM_ERR_RESPONSE,
       SLM_ERR_NO_SIM, SLM_ERR_PIN, SLM_ERR_PUK, SLM_ERR_NETWORK,
       SLM_ERR_PDP, SLM_ERR_TCP, SLM_ERR_STREAM, SLM_ERR_NO_ANSWER, SLM_ERR_TYPE };
#if DOMOVOI_DIAG
/* Снимок команды 112: времена шагов и десять последних сбоев. */
typedef struct {
  uint16_t steps[SLM_STEP_COUNT], phases[SLM_PHASE_COUNT];
  uint16_t failure_seconds[10];
  uint8_t failure_step[10], failure_reason[10];
  uint16_t last_code;
  uint8_t failure_next, failure_count;
} slm_diag_t;
#endif
typedef struct {
#if DOMOVOI_DIAG
  uint32_t completed_steps;
#endif
  uint16_t modem_code;
#if DOMOVOI_DIAG
  uint8_t state, step, phase, reason, completed_phases, at_fault;
#else
  uint8_t state, step, phase, reason, at_fault;
#endif
} slm_status_t;
/* Тип модема в настройках и статусе (Протокол2, команда 105): значения прежние. */
enum { MODEM_TYPE_AUTO = 0, MODEM_TYPE_SLM336 = 2,
       MODEM_TYPE_UNDEF = 255 };
enum { SLM_CFG_HOST, SLM_CFG_APN, SLM_CFG_LOGIN, SLM_CFG_PASSWORD,
       SLM_CFG_PORT, SLM_CFG_TYPE, SLM_CFG_NETWORK, SLM_CFG_HOST2, SLM_CFG_PORT2, SLM_CFG_RESERVE };
typedef struct {
  /* Вместимость возвращает владелец через sizeof своего поля.
     Указатель действует только в текущем вызове, драйвер его не сохраняет. */
  const char *(*text)(uint8_t field, uint16_t *capacity);
  uint16_t (*number)(uint8_t field);
} slm_settings_t;
typedef struct {
  comm_power_gpio_t gpio;
  void (*type_found)(uint8_t type);
  void (*imei_found)(const char *imei); /* 15 цифр и 0; указатель действует в вызове */
} slm_handlers_t;
/* Только для изолированного нового сеанса: отменяет UART и включает v5.
   Копируются только функции-геттеры, heap не используется. */
bool modem_slm336_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms);
/* Канал получает строки своей операции и URC; вызов AT из обработчика запрещён. */
void comm_modem_transport(void (*event)(uint8_t, const uint8_t *, uint16_t, uint16_t));
/* Передать команду канала после завершения настройки модема. */
uint8_t comm_modem_request(const comm_at_request_t *request, uint32_t now_ms);
/* Передать данные канала после приглашения QISEND. */
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms);
/* Узнать, разрешён ли резервный адрес сервера. */
bool comm_modem_reserve(void);
/* Сообщить о занятом AT или восстановлении его границ. */
bool comm_modem_busy(void);
/* Продвинуть питание, AT и единственную попытку запуска. */
void comm_modem_poll(uint32_t now_ms);
/* Сообщить, готовы ли SIM, сеть и мобильный интернет. */
bool comm_modem_ready(void);
/* Выдать состояние запуска модема. */
uint8_t comm_modem_state(void);
/* UINT8_MAX означает, что DONE ещё не получен; результат забирается один раз. */
uint8_t comm_modem_result(void);
/* STEP_COUNT завершает текущий шаг, STEP_COUNT+1 обновляет снимок. */
void comm_modem_record(uint8_t step, uint8_t reason, uint16_t code);
#if DOMOVOI_DIAG
/* Выдать стендовые времена шагов и историю отказов. */
const slm_diag_t *comm_modem_diag(void);
#endif
/* Выдать текущее состояние и причину отказа модема. */
const slm_status_t *comm_modem_status(void);
/* Снять питание и отменить прежний сеанс UART. */
void comm_modem_off(void);
/* URC (RING, +CLIP, +CMTI) - слушателям при любом состоянии AT; AT из них нельзя. */
enum { COMM_URC_CALL, COMM_URC_SMS, COMM_URC_BALANCE, COMM_URC_COUNT };
/* Подключить постоянного потребителя уведомлений. */
void comm_modem_listen(uint8_t slot, void (*urc)(const uint8_t *, uint16_t));
/* Номер попытки запуска модема: изменился - модем настроен заново. */
uint8_t comm_modem_session(void);
/* Служебная команда вне очереди канала (сброс вызова, SMS). Запрос живёт у владельца
   до результата: cmd - строка AT, mode - TEXT/PROMPT/PDU, line(p, n, detail) - строки
   ответа (PDU: detail=1 - кусок тела), payload - данные после '>' (PROMPT). */
typedef struct {
  const char *cmd;
  const uint8_t *payload;
  void (*line)(const uint8_t *, uint16_t, uint16_t);
  uint16_t count;
  uint8_t mode, timeout;
} comm_side_t;
/* Одна в работе; false - занято, чужой результат не забран или модем не настроен. */
bool comm_modem_side(const comm_side_t *req);
/* Голосовой вызов владеет служебным слотом до отбоя. */
void comm_modem_call(bool active);
/* Передать потерю мобильных данных обычному восстановлению. */
void comm_modem_data_failed(uint8_t reason);
/* Поставить голосовую команду с приоритетом над запуском данных. */
bool comm_modem_call_side(const comm_side_t *req);
/* Выдать время фактического начала голосовой команды. */
bool comm_modem_side_started(const comm_side_t *req, uint32_t *since);
/* Передать движку AT аварийный отбой. */
bool comm_modem_call_abort(uint32_t now_ms);
/* UINT8_MAX - ещё в работе или не req; результат AT один раз, перезапуск даёт ERROR. */
uint8_t comm_modem_side_result(const comm_side_t *req);
#endif
