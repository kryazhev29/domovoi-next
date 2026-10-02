/* Доработка: Кряжев А., 2026. SLM336 до активного PDP, без протокола сервера. */
#ifndef MODEM_SLM336_H
#define MODEM_SLM336_H
#include <stdint.h>
#include <stdbool.h>
#include "comm_power_v5.h"
#include "comm_at.h"
enum {
  SLM_DISCHARGE, SLM_POWER_LEAD, SLM_POWER_RAIL, SLM_POWER_KEY, SLM_POWER_BOOT,
  SLM_AT_READY, SLM_MODEL, SLM_ECHO, SLM_CMEE, SLM_IFC, SLM_CLIP,
  SLM_CMER, SLM_CREG_URC, SLM_CGREG_URC, SLM_CEREG_URC, SLM_URC_PORT,
  SLM_URC_SET, SLM_SIM, SLM_RAT_QUERY, SLM_RAT_SET, SLM_NETWORK,
  SLM_IMSI, SLM_APN, SLM_ACTIVATE, SLM_TCP, SLM_READ, SLM_STEP_COUNT
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
  uint16_t steps[SLM_STEP_COUNT], phases[SLM_PHASE_COUNT];
  uint32_t failure_seconds[10];
  uint8_t failure_step[10], failure_reason[10];
  uint16_t last_code;
  uint8_t failure_next, failure_count;
} slm_diag_t;
typedef struct {
  uint32_t completed_steps;
  uint16_t modem_code;
  uint8_t state, step, phase, reason, completed_phases, at_fault;
} slm_status_t;
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
