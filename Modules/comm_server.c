/* Доработка: Кряжев А., 2026. Протокол строят прежние производители. */
#include "comm_server.h"
#include "comm_channel.h"
#include "comm_timeouts.h"
#include <string.h>
static struct {
  comm_server_handlers_t handlers;
  uint32_t now, registration_since, status_since, stable_since, silent_since;
  uint8_t state, registrations, misses, status_id, last_status_id, silent_step;
  struct { uint8_t status_wait:1, status_due:1, traffic_event:1, stable:1, check_pending:1; } flags;
} sv;
enum { SV_OFF, SV_REG, SV_REG_WAIT, SV_ONLINE, SV_PING, SV_DNS, SV_SILENT };
static void server_channel_event(uint8_t kind, uint8_t value)
{
  if (kind == COMM_CH_OPENED) {
    sv.state = SV_REG; sv.registrations = sv.misses = sv.flags.status_wait = sv.flags.status_due = sv.flags.check_pending = 0;
    if (sv.handlers.session) sv.handlers.session(true);
  } else if (kind == COMM_CH_CLOSED || kind == COMM_CH_FAULT) {
    sv.state = SV_OFF; sv.flags.status_due = sv.flags.status_wait = sv.flags.stable = sv.flags.check_pending = 0;
    if (sv.handlers.session) sv.handlers.session(false);
  } else if (kind == COMM_CH_PROBE_DONE) {
    if (sv.state != SV_PING && sv.state != SV_DNS) return;
    sv.flags.check_pending = 0;
    if (value) { sv.state = SV_SILENT; sv.silent_since = sv.now; }
    else if (sv.state == SV_PING) sv.state = SV_DNS;
    else comm_channel_reopen(0);
  }
}
void comm_server_init(const comm_server_handlers_t *handlers)
{
  memset(&sv.handlers, 0, sizeof(sv.handlers));
  if (handlers) sv.handlers = *handlers;
  sv.state = SV_OFF;
  sv.now = sv.registration_since = sv.status_since = sv.stable_since = sv.silent_since = 0;
  sv.registrations = sv.misses = sv.flags.status_wait = sv.status_id = sv.last_status_id = sv.flags.status_due = sv.flags.traffic_event = sv.silent_step = sv.flags.stable = sv.flags.check_pending = 0;
  comm_channel_init(server_channel_event);
}
bool comm_server_online(void) { return sv.state >= SV_ONLINE; }
void comm_server_registered(uint8_t success)
{
  if (sv.state != SV_REG_WAIT) return;
  if (!success) { comm_channel_reopen(0); return; }
  comm_channel_registered(); sv.state = SV_ONLINE; sv.flags.traffic_event = sv.misses = sv.flags.status_wait = 0;
  sv.flags.stable = 1; sv.stable_since = sv.status_since = sv.now;
}
void comm_server_status_sent(uint8_t unique_id)
{
  sv.last_status_id = unique_id;
  /* Внеочередной статус не продлевает уже идущий срок ответа. */
  if (!sv.flags.status_wait) {
    sv.status_id = unique_id; sv.flags.status_wait = 1; sv.status_since = sv.now;
  }
}
void comm_server_status_ack(uint8_t unique_id)
{
  if (!comm_server_online() || !sv.flags.status_wait || (unique_id != sv.status_id && unique_id != sv.last_status_id)) return;
  sv.flags.status_wait = sv.misses = sv.flags.traffic_event = sv.flags.check_pending = 0;
  sv.state = SV_ONLINE;
  if (!sv.flags.stable) { sv.flags.stable = 1; sv.stable_since = sv.now; }
}
void comm_server_poll(uint32_t now_ms)
{
  sv.now = now_ms;
  if (comm_server_online() && now_ms - sv.status_since >= comm_timeouts[COMM_T_STATUS]) {
    if (!sv.flags.status_due && sv.flags.status_wait && sv.misses < 3) ++sv.misses;
    sv.flags.status_wait = 0; sv.flags.status_due = 1;
  }
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  comm_channel_poll(now_ms);
  if (sv.state == SV_OFF) return;
  if (sv.state == SV_REG_WAIT && now_ms - sv.registration_since >= comm_timeouts[COMM_T_SERVER_REPLY]) {
    if (sv.registrations < 2) sv.state = SV_REG;
    else { comm_channel_reopen(0); return; }
  }
  if (sv.state == SV_REG && !comm_tx_busy()) {
    comm_channel_priority(false);
    if (sv.handlers.registration) sv.handlers.registration();
    if (comm_tx_busy()) { ++sv.registrations; sv.registration_since = now_ms; sv.state = SV_REG_WAIT; }
  }
  if (sv.flags.status_due && !comm_tx_busy()) {
    comm_channel_priority(false);
    if (sv.handlers.status) sv.handlers.status();
    if (comm_tx_busy()) { sv.flags.status_due = 0; sv.status_since = now_ms; }
  }
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  /* Обработка входящих подтверждений также ждёт освобождения слота. */
  if (!comm_tx_busy() && !sv.flags.status_due && sv.handlers.process) sv.handlers.process();
  if (sv.flags.stable && now_ms - sv.stable_since >= comm_timeouts[COMM_T_STABLE]) { sv.silent_step = 0; sv.flags.stable = 0; comm_channel_stable(); }
  if (sv.state == SV_ONLINE && sv.misses >= 3) { sv.state = SV_PING; sv.flags.stable = 0; }
  if ((sv.state == SV_PING || sv.state == SV_DNS) && !sv.flags.check_pending)
    sv.flags.check_pending = comm_channel_probe(sv.state == SV_PING ? COMM_CH_PING : COMM_CH_DNS);
  if (sv.state == SV_SILENT) {
    if (!sv.flags.traffic_event && sv.handlers.no_traffic) sv.flags.traffic_event = sv.handlers.no_traffic();
    uint8_t t = sv.silent_step < 3 ? (uint8_t)(COMM_T_SERVER_RETRY_1 + sv.silent_step) : COMM_T_SERVER_RETRY_MAX;
    if (now_ms - sv.silent_since >= comm_timeouts[t]) {
      if (sv.silent_step < 3) ++sv.silent_step;
      comm_channel_reopen(0);
    }
  }
}
