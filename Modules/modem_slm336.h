/* Доработка: Кряжев А., 2026. SLM336 до активного PDP, без протокола сервера. */
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
/* Длительности последней попытки: 10 мс, насыщение при 655,35 с.
   Кольцо сбоев — единственная история; времена в секундах от старта попытки. */
typedef struct {
  #if DOMOVOI_DIAG
  uint16_t steps[SLM_STEP_COUNT], phases[SLM_PHASE_COUNT];
  uint16_t failure_seconds[10]; /* насыщение при 65535 с */
  #endif
  uint8_t failure_step[10], failure_reason[10];
  #if DOMOVOI_DIAG
  uint16_t last_code;
  uint8_t failure_next, failure_count;
  #else
  uint8_t failure_next;
  #endif
} slm_diag_t;
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
enum { MODEM_TYPE_AUTO = 0, MODEM_TYPE_UG95 = 1, MODEM_TYPE_SLM336 = 2, MODEM_TYPE_M66 = 3,
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
/* Канал получает строки своей операции и URC; вызов AT из callback запрещён. */
void comm_modem_transport(void (*event)(uint8_t, const uint8_t *, uint16_t, uint16_t));
uint8_t comm_modem_request(const comm_at_request_t *request, uint32_t now_ms);
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms);
bool comm_modem_reserve(void);
bool comm_modem_busy(void);
void comm_modem_poll(uint32_t now_ms);
bool comm_modem_ready(void);
uint8_t comm_modem_state(void);
/* UINT8_MAX означает, что DONE ещё не получен; результат забирается один раз. */
uint8_t comm_modem_result(void);
/* STEP_COUNT завершает текущий шаг, STEP_COUNT+1 обновляет снимок. */
void comm_modem_record(uint8_t step, uint8_t reason, uint16_t code);
const slm_diag_t *comm_modem_diag(void);
const slm_status_t *comm_modem_status(void);
void comm_modem_off(void);
/* URC (RING, +CLIP, +CMTI) - слушателям при любом состоянии AT; AT из них нельзя. */
enum { COMM_URC_CALL, COMM_URC_SMS, COMM_URC_BALANCE, COMM_URC_COUNT };
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
/* Вызов владеет служебным слотом до отбоя; 2 - проверка данных после него. */
void comm_modem_call(uint8_t phase);
void comm_modem_data_failed(uint8_t reason);
bool comm_modem_call_side(const comm_side_t *req);
bool comm_modem_side_started(const comm_side_t *req, uint32_t *since);
bool comm_modem_call_abort(uint32_t now_ms);
/* UINT8_MAX - ещё в работе или не req; результат AT один раз, перезапуск даёт ERROR. */
uint8_t comm_modem_side_result(const comm_side_t *req);
/* Совместимость принятых слоёв; реализация общая, дополнительных функций нет. */
#define modem_slm336_transport comm_modem_transport
#define modem_slm336_command comm_modem_request
#define modem_slm336_payload comm_modem_payload
#define modem_slm336_reserve comm_modem_reserve
#define modem_slm336_busy comm_modem_busy
#define modem_slm336_poll comm_modem_poll
#define modem_slm336_ready comm_modem_ready
#define modem_slm336_diag comm_modem_diag
#define modem_slm336_status comm_modem_status
#define modem_slm336_off comm_modem_off
#endif
