/* Доработка: Кряжев А., 2026. AT: строки, prompt и тело строго по длине. */
#include "comm_at.h"
#include "comm_uart.h"
#include <string.h>

static uint8_t command[128], line[192];
static comm_at_handlers_t handlers;
static uint32_t started, timeout, recovery;
static uint16_t command_count, line_count, remaining, read_limit, modem_code;
static uint8_t state, mode, grammar, result, in_poll, cr, prompt_space, header, sync_seen;
enum { IDLE, RUN, COMPLETE, DRAIN, SYNC_SEND, SYNC_WAIT, SYNC_COMPLETE, FAULT };
enum { LINES, M66_HEADER, BODY, BODY_CR, BODY_LF };

static void event(uint8_t kind, const uint8_t *data, uint16_t count, uint16_t detail)
{
  if (handlers.event) handlers.event(kind, data, count, detail);
}

static bool equal(const uint8_t *data, uint16_t count, const char *text)
{
  size_t n = strlen(text);
  return count == n && !memcmp(data, text, n);
}

static bool prefix(const uint8_t *data, uint16_t count, const char *text)
{
  size_t n = strlen(text);
  return count >= n && !memcmp(data, text, n);
}

/* Число без знака, переполнения и неявной обрезки; индекс только в строке. */
static bool number(const uint8_t *data, uint16_t count, uint16_t *pos,
                   uint16_t limit, uint16_t *value)
{
  uint32_t n = 0;
  uint16_t begin = *pos;
  while (*pos < count && data[*pos] >= '0' && data[*pos] <= '9') {
    n = n * 10u + data[(*pos)++] - '0';
    if (n > limit) return false;
  }
  *value = (uint16_t)n;
  return *pos != begin;
}

static bool take(const uint8_t *data, uint16_t count, uint16_t *pos, uint8_t c)
{
  if (*pos >= count || data[*pos] != c) return false;
  ++*pos;
  return true;
}

static uint16_t spaces(const uint8_t *data, uint16_t count, uint16_t pos)
{
  while (pos < count && data[pos] == ' ') ++pos;
  return pos;
}

static void parser_reset(void)
{
  line_count = remaining = 0;
  grammar = LINES;
  cr = prompt_space = header = 0;
}

static void fault(uint8_t reason)
{
  uint8_t kind = (state == RUN || state == COMPLETE) ? COMM_AT_DONE
                 : state == IDLE ? COMM_AT_STREAM_ERROR : COMM_AT_NO_ANSWER;
  /* После потери границ ни оставшееся тело, ни его OK не разбираются. */
  state = FAULT;
  event(kind, NULL, 0, reason);
}

static bool expired(uint32_t now, uint32_t duration)
{
  return (uint32_t)(now - started) >= duration;
}

/* Длина запроса QIRD — из собственной проверенной команды, не из URC. */
static bool qird_request(uint8_t format)
{
  uint16_t pos = 8, value;
  uint16_t count = (uint16_t)(command_count - 1u);
  if (!prefix(command, count, "AT+QIRD=")) return false;
  if (!number(command, count, &pos, format == COMM_AT_SLM_QIRD ? 7u : 1u, &value)
      || !take(command, count, &pos, ',')) return false;
  if (format == COMM_AT_M66_QIRD) {
    if (!number(command, count, &pos, 2, &value) || !value
        || !take(command, count, &pos, ',')
        || !number(command, count, &pos, 5, &value)
        || !take(command, count, &pos, ',')) return false;
  }
  return number(command, count, &pos, 1500, &read_limit) && read_limit
         && pos == count;
}

void comm_at_init(const comm_at_handlers_t *callbacks, uint32_t recovery_ms)
{
  if (in_poll) return;
  memset(&handlers, 0, sizeof(handlers));
  if (callbacks) handlers = *callbacks;
  state = IDLE;
  command_count = read_limit = modem_code = 0;
  recovery = recovery_ms;
  sync_seen = 0;
  parser_reset();
}

bool comm_at_busy(void)
{
  return state != IDLE || in_poll;
}

uint8_t comm_at_start(const comm_at_request_t *request, uint32_t now_ms)
{
  const uint8_t *view;
  uint16_t i;
  uint8_t trailing_space = prompt_space;
  if (comm_at_busy() || line_count || cr || comm_uart_peek(&view)) return COMM_AT_BUSY;
  if (!request || !request->command || request->count < 2u
      || request->count >= sizeof(command) || request->mode > COMM_AT_M66_QIRD
      || !request->timeout_ms || request->timeout_ms > INT32_MAX
      || !recovery || recovery > INT32_MAX) return COMM_AT_INVALID;
  if (request->command[0] != 'A' || request->command[1] != 'T') return COMM_AT_INVALID;
  for (i = 0; i < request->count; ++i)
    if (request->command[i] < ' ' || request->command[i] > '~') return COMM_AT_INVALID;
  memcpy(command, request->command, request->count);
  command[request->count] = '\r';
  command_count = (uint16_t)(request->count + 1u);
  mode = request->mode;
  read_limit = 0;
  if ((mode == COMM_AT_SLM_QIRD || mode == COMM_AT_M66_QIRD)
      && (!handlers.binary || !qird_request(mode))) return COMM_AT_INVALID;
  if (!comm_uart_tx(command, command_count)) return COMM_AT_BUSY;
  parser_reset();
  prompt_space = trailing_space;
  modem_code = 0;
  started = now_ms;
  timeout = request->timeout_ms;
  state = RUN;
  return COMM_AT_ACCEPTED;
}

static bool solicited(void)
{
  uint16_t n = 2;
  if ((state != RUN && state != DRAIN) || command_count < 4u || command[2] != '+') return false;
  while (n < command_count && command[n] != '=' && command[n] != '?'
         && command[n] != '\r') ++n;
  n = (uint16_t)(n - 2u);
  return line_count > n && line[n] == ':' && !memcmp(line, command + 2, n);
}

static bool urc(void)
{
  /* Остальные неизвестные +строки тоже события, а не подтверждение AT. */
  return (line_count && line[0] == '+' && !solicited())
      || equal(line, line_count, "RING") || equal(line, line_count, "NO CARRIER")
      || equal(line, line_count, "Call Ready") || equal(line, line_count, "SMS Ready")
      || equal(line, line_count, "AT READY") || equal(line, line_count, "RDY")
      || equal(line, line_count, "SEND OK") || equal(line, line_count, "SEND FAIL")
      || (state != RUN && state != DRAIN);
}

/* IPv4:port,TCP|UDP,length — M66 AT V2.1, §11.31 (стр. 175–176). */
static bool m66_length(uint16_t pos, uint16_t *length)
{
  uint16_t i, value;
  for (i = 0; i < 4u; ++i) {
    if (!number(line, line_count, &pos, 255, &value)) return false;
    if (!take(line, line_count, &pos, i == 3u ? ':' : '.')) return false;
  }
  if (!number(line, line_count, &pos, UINT16_MAX, &value) || !value
      || !take(line, line_count, &pos, ',')) return false;
  if (line_count - pos < 4u || (memcmp(line + pos, "TCP,", 4)
      && memcmp(line + pos, "UDP,", 4))) return false;
  pos = (uint16_t)(pos + 4u);
  return number(line, line_count, &pos, read_limit, length) && pos == line_count;
}

static void binary_header(void)
{
  uint16_t pos = grammar == M66_HEADER ? 0u : spaces(line, line_count, 6);
  uint16_t n;
  if (header) { fault(COMM_AT_BAD_FRAME); return; }
  if (mode == COMM_AT_M66_QIRD && pos == line_count) {
    grammar = M66_HEADER;
    return;
  }
  if (mode == COMM_AT_SLM_QIRD) {
    if (!number(line, line_count, &pos, read_limit, &n) || pos != line_count) {
      fault(COMM_AT_BAD_LENGTH); return;
    }
  } else if (!m66_length(pos, &n)) { fault(COMM_AT_BAD_LENGTH); return; }
  header = 1;
  remaining = n;
  grammar = n ? BODY : LINES;
}

uint8_t comm_at_payload(const uint8_t *data, uint16_t count, uint32_t timeout_ms, uint32_t now_ms)
{
  const uint8_t *view;
  if (comm_at_busy() || line_count || cr || comm_uart_peek(&view)) return COMM_AT_BUSY;
  if (!data || !count || !timeout_ms || timeout_ms > INT32_MAX || result != COMM_AT_GOT_PROMPT)
    return COMM_AT_INVALID;
  if (!comm_uart_tx(data, count)) return COMM_AT_BUSY;
  parser_reset(); prompt_space = 1;
  mode = COMM_AT_SEND; command_count = 0; modem_code = 0;
  started = now_ms; timeout = timeout_ms; state = RUN;
  return COMM_AT_ACCEPTED;
}

static bool final(uint8_t *code)
{
  uint16_t pos, value;
  if (state == RUN && mode == COMM_AT_SEND) {
    if (equal(line, line_count, "SEND OK")) { *code = COMM_AT_OK; return true; }
    if (equal(line, line_count, "SEND FAIL")) { *code = COMM_AT_ERROR; return true; }
    if (equal(line, line_count, "OK")) return false;
  }
  if (equal(line, line_count, "OK")) { *code = COMM_AT_OK; return true; }
  if (equal(line, line_count, "ERROR")) { *code = COMM_AT_ERROR; return true; }
  if (!prefix(line, line_count, "+CME ERROR:")) return false;
  pos = spaces(line, line_count, sizeof("+CME ERROR:") - 1u);
  if (pos < line_count && ((line[pos] >= 'A' && line[pos] <= 'Z')
      || (line[pos] >= 'a' && line[pos] <= 'z'))) {
    modem_code = UINT16_MAX; /* CMEE=2: текст вместо числового кода. */
    *code = COMM_AT_CME;
    return true;
  }
  if (!number(line, line_count, &pos, UINT16_MAX, &value) || pos != line_count)
    return false;
  modem_code = value;
  *code = COMM_AT_CME;
  return true;
}

static void complete_line(void)
{
  uint8_t code;
  if (!line_count) return;
  if ((state == RUN || state == SYNC_WAIT) && line_count + 1u == command_count
      && !memcmp(line, command, line_count)) return; /* Эхо до ATE0. */
  if (prefix(line, line_count, "+QIRD:")
      && ((state != RUN && state != DRAIN && state != SYNC_SEND && state != SYNC_WAIT)
          || (mode != COMM_AT_SLM_QIRD && mode != COMM_AT_M66_QIRD))) {
    fault(COMM_AT_BAD_FRAME); return; /* Неожиданное тело не становится текстом. */
  }
  if (final(&code)) {
    if (state == RUN) {
      if (code == COMM_AT_OK && ((mode == COMM_AT_SLM_QIRD && !header)
          || grammar == M66_HEADER)) { fault(COMM_AT_BAD_FRAME); return; }
      if (code == COMM_AT_CME && modem_code == UINT16_MAX)
        event(COMM_AT_LINE, line, line_count, 0);
      result = code;
      state = COMPLETE;
    } else if (state == SYNC_WAIT && sync_seen && code == COMM_AT_OK) {
      state = SYNC_COMPLETE;
    } else if (state == DRAIN) state = SYNC_SEND;
    return;
  }
  if ((state == RUN || state == DRAIN || state == SYNC_SEND || state == SYNC_WAIT)
      && (mode == COMM_AT_SLM_QIRD || mode == COMM_AT_M66_QIRD)) {
    if (prefix(line, line_count, "+QIRD:") || grammar == M66_HEADER) {
      if (grammar != M66_HEADER || !urc()) { binary_header(); return; }
    }
  }
  if (state == SYNC_WAIT && prefix(line, line_count, "+CMEE:")) {
    uint16_t pos = spaces(line, line_count, sizeof("+CMEE:") - 1u), value;
    sync_seen = number(line, line_count, &pos, 2, &value) && pos == line_count;
  }
  if (urc()) event(COMM_AT_URC, line, line_count, 0);
  else if (state == RUN) event(COMM_AT_LINE, line, line_count, 0);
}

static void byte(uint8_t value)
{
  if (grammar == BODY_CR || grammar == BODY_LF) {
    if (value != (grammar == BODY_CR ? '\r' : '\n')) { fault(COMM_AT_BAD_LENGTH); return; }
    grammar = grammar == BODY_CR ? BODY_LF : LINES;
    return;
  }
  if (prompt_space) {
    prompt_space = 0;
    if (value == ' ') return;
  }
  if (state == RUN && mode == COMM_AT_PROMPT && !line_count && !cr && value == '>') {
    result = COMM_AT_GOT_PROMPT;
    state = COMPLETE;
    prompt_space = 1;
    return;
  }
  if (value == '\r') { cr = 1; return; }
  if (value == '\n') {
    complete_line();
    line_count = cr = 0;
    return;
  }
  if (!value || cr) { fault(COMM_AT_BAD_FRAME); return; }
  if (line_count >= sizeof(line)) { fault(COMM_AT_LINE_OVERFLOW); return; }
  line[line_count++] = value;
}

void comm_at_poll(uint32_t now_ms)
{
  const uint8_t *view;
  uint16_t budget = COMM_AT_POLL_BYTES, count, used;
  if (in_poll || state == FAULT) return;
  in_poll = 1;
  if (comm_uart_errors()) { fault(COMM_AT_UART_ERROR); goto end; }
  if ((state == RUN || state == COMPLETE) && expired(now_ms, timeout)) {
    /* Отмена сохраняет грамматику и остаток тела: поиск OK внутри запрещён. */
    state = grammar != LINES || line_count || cr ? DRAIN : SYNC_SEND;
    sync_seen = 0;
    started = now_ms;
    event(COMM_AT_DONE, NULL, 0, COMM_AT_TIMEOUT);
  }
  if ((state == DRAIN || state == SYNC_SEND || state == SYNC_WAIT || state == SYNC_COMPLETE)
      && expired(now_ms, recovery)) {
    state = FAULT;
    event(COMM_AT_NO_ANSWER, NULL, 0, COMM_AT_TIMEOUT);
    goto end;
  }
  while (budget && state != FAULT && (count = comm_uart_peek(&view)) != 0u) {
    if (count > budget) count = budget;
    if (grammar == BODY) {
      if (count > remaining) count = remaining;
      /* Позднее тело уже отменённой операции вычитывается без выдачи наверх. */
      used = state != RUN ? count : handlers.binary(view, count);
      if (comm_uart_errors()) { fault(COMM_AT_UART_ERROR); break; }
      if (used > count) { fault(COMM_AT_SINK_ERROR); break; }
      if (!used) break;
      remaining = (uint16_t)(remaining - used);
      if (!remaining) grammar = BODY_CR;
    } else {
      used = 1;
      byte(*view);
    }
    if (state == FAULT) break;
    if (!comm_uart_consume(used) || comm_uart_errors()) { fault(COMM_AT_UART_ERROR); break; }
    budget = (uint16_t)(budget - used);
  }
  if (state == COMPLETE && comm_uart_tx_done() && !line_count && !cr
      && !comm_uart_peek(&view)) {
    state = IDLE;
    event(COMM_AT_DONE, NULL, modem_code, result);
  }
  if (state == SYNC_COMPLETE && comm_uart_tx_done() && !line_count && !cr
      && !comm_uart_peek(&view)) state = IDLE;
  if (state == DRAIN && grammar == LINES && !line_count && !cr) state = SYNC_SEND;
  if (state == SYNC_SEND && comm_uart_tx_done() && !line_count && !cr
      && !comm_uart_peek(&view)) {
    /* Статический буфер AT можно менять только после TC старой передачи. */
    memcpy(command, "AT+CMEE?\r", sizeof("AT+CMEE?\r") - 1u);
    command_count = sizeof("AT+CMEE?\r") - 1u;
    if (comm_uart_tx(command, command_count)) { parser_reset(); state = SYNC_WAIT; }
  }
end:
  in_poll = 0;
}
