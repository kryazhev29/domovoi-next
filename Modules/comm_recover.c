/* Доработка: Кряжев А., 2026.
   Восстанавливает связь по причинам отказа и окну отказов AT.
   Ступени задают паузы перезапуска модема, процессор не перезагружается.
   Ожидаемая потеря PDP после звонка получает первую попытку без отката.
*/
#include "domovoi_diag.h"
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
  uint32_t at_times[3];                /* millis последних таймаутов AT в работе */
  uint16_t night_day;                   /* сутки последнего ночного перезапуска */
  uint8_t state, power, power_step, soft_step, soft_fails, net_step, online;
  uint8_t at_count;
  uint8_t at_i, early;
} rc;

/* Ступень паузы: 0, 1, 2 и дальше последняя. */
static uint32_t rc_pause(uint8_t first, uint8_t step)
{
  return comm_timeouts[first + (step < 2u ? step : 2u)];
}

/* Перезапуск через pause мс; до него питание и служебные AT сохраняются. */
static void rc_schedule(bool power, uint32_t pause)
{
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

/* Продвинуть ступень перезапуска со снятием питания. */
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
/* Добавить отказ и определить достижение порога раннего перезапуска. */
static bool rc_burst(uint32_t *t, uint8_t n, uint8_t *i, uint8_t *count, uint8_t window)
{
  rc_expire(t, n, *i, count, window);
  t[*i] = rc.now;
  *i = (uint8_t)((*i + 1u) % n);
  if (++*count < n) return false;
  *count = 0;
  return true;
}

/* Отказы TCP проверяет сервер через ping/DNS; здесь только окно отказов AT. */
void comm_recover_failure(uint8_t step, uint8_t reason, uint32_t now_ms)
{
  if (step < SLM_TCP || (step == SLM_TCP && reason == SLM_ERR_TIMEOUT)) return;
  rc.now = now_ms;
  if (reason == SLM_ERR_TIMEOUT || reason == SLM_ERR_NO_ANSWER)
    rc.early |= rc_burst(rc.at_times, 3, &rc.at_i, &rc.at_count, COMM_T_EARLY_AT);
}

/* Попытка модема не удалась: что дальше - по причине. */
static void rc_failed(void)
{
  const slm_status_t *s = comm_modem_status();
  /* Журнал: не дошёл до сети - событие 11, до мобильного интернета - 12. */
  if (s->reason == SLM_ERR_PDP || s->phase == SLM_PHASE_PDP) comm_server_note(COMM_LINK_NO_INTERNET);
  else if (s->phase == SLM_PHASE_SIM || s->phase == SLM_PHASE_NETWORK) comm_server_note(COMM_LINK_NO_NET);
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
  uint32_t t = rc.hooks->local_time();
  bool busy = comm_modem_busy() || comm_tx_busy() || rc.hooks->busy();
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

/* Сервисный код выбирается из действующих состояний, без своего автомата. */
uint8_t comm_recover_link(uint32_t now_ms, uint32_t *wait_left)
{
  *wait_left = rc.state == RC_WAIT && now_ms - rc.wait_since < rc.wait_ms ? rc.wait_ms - (now_ms - rc.wait_since) : 0;
  if (rc.state == RC_WAIT) return LINK_RESTART_WAIT;
  const slm_status_t *s = comm_modem_status();
  if (s->state == SLM_STARTING) return LINK_CONNECTING;
  if (s->reason == SLM_ERR_NO_SIM || s->reason == SLM_ERR_PIN || s->reason == SLM_ERR_PUK) return LINK_NO_SIM;
  if (s->reason == SLM_ERR_NETWORK || (s->state == SLM_FAILED && s->phase == SLM_PHASE_NETWORK)) return LINK_NO_NETWORK;
  if (s->reason == SLM_ERR_PDP || (s->state == SLM_FAILED && s->phase == SLM_PHASE_PDP)) return LINK_NO_INTERNET;
  if (s->state != SLM_READY) return LINK_CONNECTING;
  return comm_server_condition();
}
/* Отладочная диагностика (команда 111): ожидание, ступени питания/AT/интернета; остаток паузы, мс. */
#if DOMOVOI_DIAG
/* Собрать ступени и остаток паузы для команды 111. */
uint32_t comm_recover_diag(uint32_t now_ms, uint32_t *wait_left)
{
  *wait_left = rc.state == RC_WAIT && now_ms - rc.wait_since < rc.wait_ms ? rc.wait_ms - (now_ms - rc.wait_since) : 0;
  return rc.state | (uint32_t)rc.power << 4 | (uint32_t)rc.power_step << 8 | (uint32_t)rc.soft_step << 16 | (uint32_t)rc.net_step << 24;
}
#endif
/* Начать надзор связи с обязательными обработчиками приложения. */
void comm_recover_init(const comm_recover_hooks_t *hooks, uint32_t now_ms)
{
  memset(&rc, 0, sizeof(rc));
  rc.hooks = hooks;
  rc.now = rc.last_start = now_ms;
}

/* TCP заново -> модем заново с AT (шаг QIDEACT сбрасывает мобильный интернет) ->
   снятие питания сразу, дальше по паузам. */
void comm_recover_no_internet(void)
{
  comm_server_note(COMM_LINK_NO_INTERNET);
  if (rc.net_step == 0u) comm_channel_reopen(0);
  else if (rc.net_step == 1u) rc_schedule(false, 0);
  else rc_power_ladder(rc_now_then_ladder());
  if (rc.net_step < 2u) rc.net_step++;
}

/* Модем сам зарегистрировался: подключаться с AT, отменив остаток паузы. */
void comm_recover_registered(void)
{
  if (rc.state == RC_WAIT) { rc.power = 0; rc.wait_ms = 0; }
}

/* Исполнить назначенный перезапуск или проверить условия надзора. */
void comm_recover_poll(uint32_t now_ms)
{
  rc.now = now_ms;
  rc_expire(rc.at_times, 3, rc.at_i, &rc.at_count, COMM_T_EARLY_AT);
  if (comm_channel_call_pending() && !comm_modem_status()->at_fault) return;
  bool early = rc.early;
  rc.early = 0;
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
      rc.hooks->reset_session();
    }
    return;
  }
  uint8_t state = comm_modem_state();
  if (state == SLM_FAILED) {
    if (comm_channel_expected() && comm_modem_status()->reason == SLM_ERR_PDP) rc_schedule(false, 0);
    else rc_failed();
    return;
  }
  if (state != SLM_READY) return;
  /* В работе модем перестал отвечать или сбои пошли пачкой - короткий перезапуск. */
  if (early || comm_channel_fault()) { rc_power_ladder(rc_now_then_ladder()); return; }
  rc_night();
}
