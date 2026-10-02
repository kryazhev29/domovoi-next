/* Доработка: Кряжев А., 2026. SLM336 до готового TCP, без протокола сервера. */
#ifndef MODEM_SLM336_H
#define MODEM_SLM336_H
#include <stdint.h>
#include <stdbool.h>
#include "comm_power_v5.h"
enum {
  SLM_DISCHARGE, SLM_POWER_LEAD, SLM_POWER_RAIL, SLM_POWER_KEY, SLM_POWER_BOOT,
  SLM_AT_READY, SLM_MODEL, SLM_ECHO, SLM_CMEE, SLM_IFC, SLM_CLIP,
  SLM_CMER, SLM_CREG_URC, SLM_CGREG_URC, SLM_CEREG_URC, SLM_URC_PORT,
  SLM_URC_SET, SLM_SIM, SLM_RAT_QUERY, SLM_RAT_SET, SLM_NETWORK,
  SLM_IMSI, SLM_APN, SLM_ACTIVATE, SLM_TCP, SLM_READ, SLM_STEP_COUNT
};
enum { SLM_PHASE_POWER, SLM_PHASE_SETUP, SLM_PHASE_SIM, SLM_PHASE_NETWORK,
       SLM_PHASE_PDP, SLM_PHASE_TCP, SLM_PHASE_COUNT };
enum { SLM_IDLE, SLM_STARTING, SLM_OPEN, SLM_FAILED };
enum { SLM_ERR_NONE, SLM_ERR_TIMEOUT, SLM_ERR_AT, SLM_ERR_RESPONSE,
       SLM_ERR_NO_SIM, SLM_ERR_PIN, SLM_ERR_PUK, SLM_ERR_NETWORK,
       SLM_ERR_PDP, SLM_ERR_TCP, SLM_ERR_STREAM, SLM_ERR_NO_ANSWER, SLM_ERR_TYPE };
typedef struct {
  uint32_t start_ms, duration_ms;
  uint32_t success_start_ms, success_ms, failure_start_ms, failure_ms;
} slm_timing_t;
typedef struct {
  slm_timing_t steps[SLM_STEP_COUNT], phases[SLM_PHASE_COUNT], at;
  /* success/failure — история; completed — только текущий запуск. */
  uint32_t success_steps, failure_steps, completed_steps;
  uint8_t success_phases, failure_phases, completed_phases, at_fault;
  uint8_t state, step, phase, last_success_step, last_failure_step, reason;
  uint16_t modem_code;
} slm_diag_t;
typedef struct {
  char host[64], apn[31], login[31], password[31];
  uint16_t port;
  uint8_t modem_type; /* Совместимо: 2=SLM336; 0/255=пусто, 1/3 здесь не поддержаны. */
  uint8_t network_type; /* Поле команды 12: 0..4, без подмены ручного выбора. */
} slm_config_t;
typedef struct {
  comm_power_gpio_t gpio;
  uint16_t (*binary)(const uint8_t *data, uint16_t count);
  /* Тело предварительно до read_done OK; чужих указателей драйвер не хранит. */
  void (*read_done)(uint16_t count, uint8_t reason);
  void (*type_found)(uint8_t type);
} slm_handlers_t;
/* Только для изолированного нового сеанса: отменяет UART и включает v5.
   Конфигурация копируется в собственный контекст, heap не используется. */
bool modem_slm336_start(const slm_config_t *config, const slm_handlers_t *handlers, uint32_t now_ms);
void modem_slm336_poll(uint32_t now_ms);
bool modem_slm336_ready(void);
/* Вместимость будущего потребителя, 1..256; ноль откладывает новые QIRD. */
void modem_slm336_rx_capacity(uint16_t count);
const slm_diag_t *modem_slm336_diag(void);
void modem_slm336_off(void);
#endif
