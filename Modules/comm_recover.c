/* Доработка: Кряжев А., 2026. Восстановление связи. Устройство - в comm_recover.h. */
#include "comm_recover.h"
#include "comm_modem.h"
#include "comm_channel.h"
#include "comm_server.h"
#include "comm_timeouts.h"
#include <string.h>

enum { RC_WATCH, RC_WAIT };
static struct {
  const comm_recover_hooks_t *hooks;
  uint32_t now, wait_since, wait_ms, online_since, last_start;
  uint32_t at_times[3], tcp_times[5];   /* millis последних таймаутов AT и неудач TCP в работе */
  uint16_t night_day;                   /* сутки последнего ночного перезапуска */
  uint8_t state, power, power_step, soft_step, soft_fails, net_step, online;
  uint8_t at_count, tcp_count;
  uint8_t ring, at_i, tcp_i;            /* ring - следующая непрочитанная запись кольца сбоев */
} rc;

/* Ступень паузы: 0, 1, 2 и дальше последняя. */
static uint32_t rc_pause(uint8_t first, uint8_t step)
{
  return comm_timeouts[first + (step < 2u ? step : 2u)];
}

/* Перезапуск модема через pause мс. С питанием - модем на паузу выключен. */
static void rc_schedule(bool power, uint32_t pause)
{
  if (power) comm_modem_off();
  rc.power = (uint8_t)power;
  rc.state = RC_WAIT;
  rc.wait_since = rc.now;
  rc.wait_ms = pause;
}

/* Питание сразу; если и после этого не помогло - по паузам 10/30/60 мин. */
static uint32_t rc_now_then_ladder(void)
{
  return rc.power_step ? rc_pause(COMM_T_POWER_1, (uint8_t)(rc.power_step - 1u)) : 0;
}

static void rc_power_ladder(uint32_t pause)
{
  rc_schedule(true, pause);
  if (rc.power_step < 3u) rc.power_step++;
  rc.soft_fails = 0;
}

/* Старые отметки удаляются каждый poll, в том числе до оборота millis.
   Отдельное число отметок позволяет хранить событие в момент millis == 0. */
static void rc_expire(uint32_t *t, uint8_t n, uint8_t i, uint8_t *count, uint8_t window)
{
  while (*count && rc.now - t[(i + n - *count) % n] >= comm_timeouts[window]) --*count;
}
static bool rc_burst(uint32_t *t, uint8_t n, uint8_t *i, uint8_t *count, uint8_t window)
{
  rc_expire(t, n, *i, count, window);
  t[*i] = rc.now;
  *i = (uint8_t)((*i + 1u) % n);
  if (++*count < n) return false;
  *count = 0;
  return true;
}

/* Новые записи кольца сбоев: в работе (шаги сокета) считаются таймауты AT и TCP. */
static bool rc_early(void)
{
  const slm_diag_t *d = comm_modem_diag();
  bool early = false;
  for (uint8_t k = 0; k < sizeof(d->failure_step) && rc.ring != d->failure_next; k++) {
    uint8_t reason = d->failure_reason[rc.ring], step = d->failure_step[rc.ring];
    rc.ring = (uint8_t)((rc.ring + 1u) % sizeof(d->failure_step));
    if (step < SLM_TCP) continue;
    if (reason == SLM_ERR_TIMEOUT || reason == SLM_ERR_NO_ANSWER)
      early |= rc_burst(rc.at_times, 3, &rc.at_i, &rc.at_count, COMM_T_EARLY_AT);
    else if (reason == SLM_ERR_TCP)
      early |= rc_burst(rc.tcp_times, 5, &rc.tcp_i, &rc.tcp_count, COMM_T_EARLY_TCP);
  }
  return early;
}

/* Попытка модема не удалась: что дальше - по причине. */
static void rc_failed(void)
{
  const slm_status_t *s = comm_modem_status();
  switch (s->reason) {
  case SLM_ERR_NO_ANSWER: case SLM_ERR_STREAM:
    /* Замолчал посреди работы - питание сразу; молчит после включения - по паузам. */
    rc_power_ladder(s->step >= SLM_TCP ? rc_now_then_ladder() : rc_pause(COMM_T_POWER_1, rc.power_step));
    return;
  case SLM_ERR_NO_SIM: case SLM_ERR_PIN: case SLM_ERR_PUK: case SLM_ERR_TYPE:
    rc_power_ladder(rc_pause(COMM_T_POWER_1, rc.power_step));
    return;
  case SLM_ERR_TIMEOUT:
    if (s->step <= SLM_AT_READY) { rc_power_ladder(rc_pause(COMM_T_POWER_1, rc.power_step)); return; }
    break;
  default:
    break;
  }
  /* Сеть, мобильный интернет, ответ модема - заново с AT; три раза подряд - питание. */
  if (++rc.soft_fails >= 3u) { rc_power_ladder(rc_pause(COMM_T_POWER_1, rc.power_step)); return; }
  rc_schedule(false, rc_pause(COMM_T_SOFT_1, rc.soft_step));
  if (rc.soft_step < 3u) rc.soft_step++;
}

/* Ночной перезапуск модема: окно 03:00-04:00 раз в сутки, без часов - сутки работы. */
static void rc_night(void)
{
  uint32_t t = rc.hooks && rc.hooks->local_time ? rc.hooks->local_time() : UINT32_MAX;
  bool busy = comm_modem_busy() || comm_tx_busy() || (rc.hooks && rc.hooks->busy && rc.hooks->busy());
  if (busy) return;
  if (t != UINT32_MAX) {
    uint32_t second = t % 86400u;
    uint16_t day = (uint16_t)(t / 86400u);
    if (second >= 3u * 3600u && second < 4u * 3600u && day != rc.night_day) {
      rc.night_day = day;
      rc_schedule(true, 0);
    }
  } else if (rc.now - rc.last_start >= comm_timeouts[COMM_T_DAY]) {
    rc_schedule(true, 0);
  }
}

/* Диагностика (команда 112): ожидание, ступени питания/AT/интернета; остаток паузы, мс. */
uint32_t comm_recover_diag(uint32_t now_ms, uint32_t *wait_left)
{
  *wait_left = rc.state == RC_WAIT && now_ms - rc.wait_since < rc.wait_ms ? rc.wait_ms - (now_ms - rc.wait_since) : 0;
  return rc.state | (uint32_t)rc.power << 4 | (uint32_t)rc.power_step << 8 | (uint32_t)rc.soft_step << 16 | (uint32_t)rc.net_step << 24;
}
void comm_recover_init(const comm_recover_hooks_t *hooks, uint32_t now_ms)
{
  memset(&rc, 0, sizeof(rc));
  rc.hooks = hooks;
  rc.now = rc.last_start = now_ms;
  rc.ring = comm_modem_diag()->failure_next;
}

/* TCP заново -> модем заново с AT (шаг QIDEACT сбрасывает мобильный интернет) ->
   снятие питания сразу, дальше по паузам. */
void comm_recover_no_internet(void)
{
  if (rc.net_step == 0u) comm_channel_reopen(0);
  else if (rc.net_step == 1u) rc_schedule(false, 0);
  else rc_power_ladder(rc_now_then_ladder());
  if (rc.net_step < 2u) rc.net_step++;
}

void comm_recover_poll(uint32_t now_ms)
{
  rc.now = now_ms;
  rc_expire(rc.at_times, 3, rc.at_i, &rc.at_count, COMM_T_EARLY_AT);
  rc_expire(rc.tcp_times, 5, rc.tcp_i, &rc.tcp_count, COMM_T_EARLY_TCP);
  bool early = rc_early();
  if (comm_server_online()) {
    if (!rc.online) { rc.online = 1; rc.online_since = now_ms; }
    else if (now_ms - rc.online_since >= comm_timeouts[COMM_T_STABLE])
      rc.power_step = rc.soft_step = rc.soft_fails = rc.net_step = 0;
  } else rc.online = 0;
  if (rc.state == RC_WAIT) {
    if (now_ms - rc.wait_since < rc.wait_ms) return;
    rc.state = RC_WATCH;
    if (comm_modem_restart(rc.power != 0u, now_ms)) {
      rc.last_start = now_ms;
      if (rc.hooks && rc.hooks->reset_session) rc.hooks->reset_session();
    }
    return;
  }
  uint8_t state = comm_modem_state();
  if (state == SLM_FAILED) { rc_failed(); return; }
  if (state != SLM_READY) return;
  /* В работе модем перестал отвечать или сбои пошли пачкой - сразу без питания. */
  if (early || comm_channel_fault()) { rc_power_ladder(rc_now_then_ladder()); return; }
  rc_night();
}
