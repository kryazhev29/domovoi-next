/* Доработка: Кряжев А., 2026.
   Разбирает строки AT, приглашения и двоичное тело SLM336.
   Проверяет внешние ответы один раз; внутренние команды уже собраны владельцем.
   Обработчики принимают данные, следующий запрос запускает главный цикл.
*/
#include "usb_log.h"
#include "comm_at.h"
#include "comm_uart.h"
#include <string.h>

static uint8_t command[128], line[192];
static comm_at_handlers_t handlers;
static uint32_t started, timeout, recovery;
static uint16_t remaining, read_limit, modem_code;
static uint8_t command_count, line_count;
static uint8_t state, mode, grammar, result, cr, prompt_space, header, sync_seen;
/* PDU: строка после заголовка ответа - тело SMS (hex), без разбора и кусками;
   1 - ждём тело, 2 - отдан кусок. */
static uint8_t pdu_body;
enum { IDLE, RUN, COMPLETE, DRAIN, CALL_DRAIN, SYNC_SEND, SYNC_WAIT, SYNC_COMPLETE, FAULT };
enum { LINES, BODY, BODY_CR, BODY_LF };

/* Передать событие единственному обработчику AT. */
static void event(uint8_t kind, const uint8_t *data, uint16_t count, uint16_t detail)
{
  if (usb_log_on(1)) {
    if (kind == COMM_AT_LINE || kind == COMM_AT_URC) usb_log(kind == COMM_AT_LINE ? 2u : 3u, data, count > 46u ? 47u : (uint8_t)count);
    else if (kind == COMM_AT_DONE) usb_log_value(4, (uint8_t)detail, count);
    else if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) usb_log_value(4, kind == COMM_AT_NO_ANSWER ? COMM_AT_TIMEOUT : (uint8_t)detail, 0);
  }
  handlers.event(kind, data, count, detail);
}

/* Сравнить всю ограниченную строку с ожидаемым ответом. */
bool comm_at_equal(const uint8_t *data, uint16_t count, const char *text)
{
  size_t n = strlen(text);
  return count == n && !memcmp(data, text, n);
}

/* Проверить начало ограниченной строки без выхода за границу. */
bool comm_at_prefix(const uint8_t *data, uint16_t count, const char *text)
{
  size_t n = strlen(text);
  return count >= n && !memcmp(data, text, n);
}

/* Число без знака, переполнения и неявной обрезки; индекс только в строке. */
bool comm_at_number(const uint8_t *data, uint16_t count, uint16_t *pos,
                   uint32_t limit, uint32_t *value)
{
  uint32_t n = 0;
  uint16_t begin = *pos;
  while (*pos < count && data[*pos] >= '0' && data[*pos] <= '9') {
    uint8_t digit = data[(*pos)++] - '0';
    if (n > UINT32_MAX / 10u || (n == UINT32_MAX / 10u && digit > UINT32_MAX % 10u)) return false;
    n = n * 10u + digit;
    if (n > limit) return false;
  }
  *value = n;
  return *pos != begin;
}

/* Принять ожидаемый символ грамматики и сдвинуть позицию. */
static bool take(const uint8_t *data, uint16_t count, uint16_t *pos, uint8_t c)
{
  if (*pos >= count || data[*pos] != c) return false;
  ++*pos;
  return true;
}

/* Пропустить разрешённые пробелы перед полем ответа. */
uint16_t comm_at_spaces(const uint8_t *data, uint16_t count, uint16_t pos)
{
  while (pos < count && data[pos] == ' ') ++pos;
  return pos;
}

/* Очистить грамматику для нового запроса. */
static void parser_reset(void)
{
  line_count = remaining = 0;
  grammar = LINES;
  cr = prompt_space = header = pdu_body = 0;
}

/* Зафиксировать потерю границ и запретить дальнейший разбор. */
static void fault(uint8_t reason)
{
  uint8_t kind = (state >= RUN && state <= COMPLETE) ? COMM_AT_DONE
                 : state == IDLE ? COMM_AT_STREAM_ERROR : COMM_AT_NO_ANSWER;
  /* После потери границ ни оставшееся тело, ни его OK не разбираются. */
  state = FAULT;
  event(kind, NULL, 0, reason);
}

/* Проверить истечение срока с учётом оборота счётчика времени. */
static bool expired(uint32_t now, uint32_t duration)
{
  return (uint32_t)(now - started) >= duration;
}

/* Размер тела известен из собственной команды канала. */
static void qird_request(void)
{
  const char *p = (const char *)command + sizeof("AT+QIRD=0,") - 1u;
  while (*p >= '0' && *p <= '9') read_limit = (uint16_t)(read_limit * 10u + *p++ - '0');
}
/* Начать чистый AT после отмены прежнего UART. */
void comm_at_init(const comm_at_handlers_t *callbacks, uint32_t recovery_ms)
{
  handlers = *callbacks;
  state = IDLE;
  command_count = read_limit = modem_code = result = 0;
  recovery = recovery_ms;
  sync_seen = 0;
  parser_reset();
}

/* Один раз проверить строковую настройку на границе запуска. */
bool comm_at_field(const char *p, size_t size, bool host)
{
  size_t i;
  for (i = 0; i < size; ++i) {
    unsigned char c = (unsigned char)p[i];
    if (!c) return !host || i != 0;
    if (host ? !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
           || (c >= '0' && c <= '9') || c == '.' || c == '-')
        : c < 32 || c > 126 || c == '"' || c == '\\') return false;
  }
  return false;
}
/* Назначить потребителя двоичного тела канала. */
void comm_at_sink(uint16_t (*binary)(const uint8_t *, uint16_t))
{ handlers.binary = binary; }

/* Сообщить, занят ли движок запросом или восстановлением границ. */
bool comm_at_busy(void)
{
  return state != IDLE;
}
/* Сообщить, ждёт ли модем данные после приглашения. */
bool comm_at_prompt(void) { return state == IDLE && result == COMM_AT_GOT_PROMPT; }

/* Разрешить новый запрос только при пустом приёмном потоке. */
static bool available(void)
{
  const uint8_t *view;
  return !comm_at_busy() && !line_count && !cr && !comm_uart_peek(&view);
}
/* Начать ожидание ответа заданного режима. */
static void run(uint8_t next_mode, uint32_t duration, uint32_t now_ms)
{
  parser_reset(); modem_code = 0;
  mode = next_mode; started = now_ms; timeout = duration; state = RUN;
}
/* Скопировать готовую команду и начать её неблокирующую передачу. */
uint8_t comm_at_start(const comm_at_request_t *request, uint32_t now_ms)
{
  uint8_t trailing_space = prompt_space;
  if (!available()) return COMM_AT_BUSY;
  memcpy(command, request->command, request->count);
  command[request->count] = '\r';
  command_count = (uint8_t)(request->count + 1u);
  mode = request->mode;
  read_limit = 0;
  if (mode == COMM_AT_SLM_QIRD) qird_request();
  if (!comm_uart_tx(command, command_count)) return COMM_AT_BUSY;
  if (usb_log_on(1)) usb_log(1, request->command, request->count > 46u ? 47u : (uint8_t)request->count);
  run(mode, request->timeout_ms, now_ms);
  prompt_space = trailing_space;
  return COMM_AT_ACCEPTED;
}

/* Отличить ответ текущей команды от постороннего уведомления. */
static bool solicited(void)
{
  uint16_t n = 2;
  if ((state != RUN && state != DRAIN) || command_count < 4u || command[2] != '+') return false;
  while (n < command_count && command[n] != '=' && command[n] != '?'
         && command[n] != '\r') ++n;
  n = (uint16_t)(n - 2u);
  return line_count > n && line[n] == ':' && !memcmp(line, command + 2, n);
}

/* Строки без массива указателей; результат 1..N, ноль — нет совпадения. */
uint8_t comm_at_choice(const uint8_t *data, uint16_t count, const char *names)
{
  for (uint8_t i = 1; *names; ++i) {
    size_t n = strlen(names);
    if (count == n && !memcmp(data, names, n)) return i;
    names += n + 1u;
  }
  return 0;
}
/* Определить строку самостоятельного уведомления модема. */
static bool urc(void)
{
  return (line_count && line[0] == '+' && !solicited()) || (state != RUN && state != DRAIN)
    || comm_at_choice(line, line_count, "RING\0NO CARRIER\0Call Ready\0SMS Ready\0"
                                      "AT READY\0RDY\0SEND OK\0SEND FAIL\0");
}

/* Принять проверенную длину тела QIRD. */
static void binary_header(void)
{
  uint16_t pos = comm_at_spaces(line, line_count, 6);
  uint32_t n;
  if (header) { fault(COMM_AT_BAD_FRAME); return; }
  if (!comm_at_number(line, line_count, &pos, read_limit, &n) || pos != line_count) {
    fault(COMM_AT_BAD_LENGTH); return;
  }
  header = 1;
  remaining = (uint16_t)n;
  if (usb_log_on(1)) usb_log_value(5, 0, n);
  grammar = n ? BODY : LINES;
}

/* Передать данные после уже полученного приглашения. */
uint8_t comm_at_payload(const uint8_t *data, uint16_t count, uint8_t finals, uint32_t timeout_ms, uint32_t now_ms)
{
  if (!available()) return COMM_AT_BUSY;
  if (!comm_uart_tx(data, count)) return COMM_AT_BUSY;
  if (usb_log_on(1)) usb_log_value(5, 1, count);
  run(finals, timeout_ms, now_ms); prompt_space = 1; command_count = 0;
  return COMM_AT_ACCEPTED;
}

/* Распознать завершение команды и код ошибки модема. */
static bool final(uint8_t *code)
{
  uint16_t pos; uint32_t value;
  uint8_t matched = comm_at_choice(line, line_count, state == RUN && mode == COMM_AT_SEND
    ? "SEND OK\0SEND FAIL\0ERROR\0" : "OK\0ERROR\0");
  if (matched) { *code = matched == 1 ? COMM_AT_OK : COMM_AT_ERROR; return true; }
  /* +CMS ERROR - отказ команд SMS (27.005 §3.2.5), тот же разбор кода. */
  if (!comm_at_prefix(line, line_count, "+CME ERROR:") && !comm_at_prefix(line, line_count, "+CMS ERROR:")) return false;
  pos = comm_at_spaces(line, line_count, sizeof("+CME ERROR:") - 1u);
  uint8_t letter = pos < line_count ? line[pos] | 32u : 0;
  if (letter >= 'a' && letter <= 'z') {
    modem_code = UINT16_MAX; /* CMEE=2: текст вместо числового кода. */
    *code = COMM_AT_CME;
    return true;
  }
  if (!comm_at_number(line, line_count, &pos, UINT16_MAX, &value) || pos != line_count)
    return false;
  modem_code = (uint16_t)value;
  *code = COMM_AT_CME;
  return true;
}

#ifdef __IAR_SYSTEMS_ICC__
#pragma inline=never
#endif
/* Разобрать законченную строку согласно текущей грамматике. */
static void complete_line(void)
{
  uint8_t code;
  /* Тело кончилось ровно на границе куска: пустой последний кусок. */
  if (!line_count) { if (pdu_body == 2) { pdu_body = 0; handlers.event(COMM_AT_LINE, line, 0, 0); } return; }
  if ((state == RUN || state == SYNC_WAIT) && line_count + 1u == command_count
      && !memcmp(line, command, line_count)) return; /* Эхо до ATE0. */
  if (pdu_body) { pdu_body = 0; handlers.event(COMM_AT_LINE, line, line_count, 0); return; }
  if (comm_at_prefix(line, line_count, "+QIRD:")
      && ((state != RUN && state != DRAIN && state != SYNC_SEND && state != SYNC_WAIT)
          || mode != COMM_AT_SLM_QIRD)) {
    fault(COMM_AT_BAD_FRAME); return; /* Неожиданное тело не становится текстом. */
  }
  if (mode == COMM_AT_CALL && state == RUN && comm_at_equal(line, line_count, "NO CARRIER")) {
    result = COMM_AT_ERROR; state = COMPLETE; return;
  }
  if (final(&code)) {
    if (state == RUN) {
      if (code == COMM_AT_OK && (mode == COMM_AT_SLM_QIRD && !header)) { fault(COMM_AT_BAD_FRAME); return; }
      if (code == COMM_AT_CME && modem_code == UINT16_MAX)
        event(COMM_AT_LINE, line, line_count, 0);
      result = code;
      state = COMPLETE;
    } else if (state == SYNC_WAIT && sync_seen && code == COMM_AT_OK) {
      state = SYNC_COMPLETE;
    } else if (state == DRAIN || state == CALL_DRAIN) state = SYNC_SEND;
    return;
  }
  if ((state == RUN || (state >= DRAIN && state <= SYNC_WAIT))
      && mode == COMM_AT_SLM_QIRD) {
    if (comm_at_prefix(line, line_count, "+QIRD:")) { binary_header(); return; }
  }
  if (state == SYNC_WAIT && comm_at_prefix(line, line_count, "+CMEE:")) {
    uint16_t pos = comm_at_spaces(line, line_count, sizeof("+CMEE:") - 1u); uint32_t value;
    sync_seen = comm_at_number(line, line_count, &pos, 2, &value) && pos == line_count;
  }
  if (urc()) event(COMM_AT_URC, line, line_count, 0);
  else if (state == RUN) {
    event(COMM_AT_LINE, line, line_count, 0);
    if (mode == COMM_AT_PDU && solicited()) pdu_body = 1; /* +CMGR:/+CMGL: - дальше тело */
  }
}

/* Принять один байт строки, приглашения или разделителя тела. */
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
  if (line_count >= sizeof(line)) {
    /* USSD с рекламой и CGCONTRDP с IPv6: нужное в начале, хвост до CRLF отбрасывается. */
    if (comm_at_prefix(line, line_count, "+CUSD:") || comm_at_prefix(line, line_count, "+CGCONTRDP:")) return;
    if (!pdu_body || state != RUN) { fault(COMM_AT_LINE_OVERFLOW); return; }
    handlers.event(COMM_AT_LINE, line, line_count, 1); line_count = 0; pdu_body = 2; /* кусок, дальше продолжение */
  }
  line[line_count++] = value;
}

/* Голосовая команда не имеет бинарного тела/приглашения: её можно прервать.
   CHUP уходит раньше CMEE; поздние финалы голосовых команд не подтверждают следующий запрос. */
bool comm_at_call_abort(uint32_t now_ms)
{
  static const uint8_t stop[] = "AT+CHUP\r";
  if (mode != COMM_AT_CALL || grammar != LINES || state > COMPLETE
      || !comm_uart_tx_done() || !comm_uart_tx(stop, sizeof(stop) - 1u)) return false;
  if (usb_log_on(1)) usb_log(1, stop, sizeof(stop) - 2u);
  uint8_t running = state != IDLE;
  state = line_count || cr ? DRAIN : CALL_DRAIN;
  mode = COMM_AT_TEXT; sync_seen = 0; started = now_ms; timeout = 300u;
  if (running) event(COMM_AT_DONE, NULL, 0, COMM_AT_TIMEOUT);
  return true;
}
/* Обработать ограниченный объём RX и сроки текущего AT. */
void comm_at_poll(uint32_t now_ms)
{
  const uint8_t *view;
  uint16_t budget = COMM_AT_POLL_BYTES, count, used;
  if (state == FAULT) return;
  if (comm_uart_errors()) { fault(COMM_AT_UART_ERROR); return; }
  if ((state >= RUN && state <= COMPLETE) && expired(now_ms, timeout)) {
    if (mode == COMM_AT_CALL) { comm_at_call_abort(now_ms); return; }
    /* Отмена сохраняет грамматику и остаток тела: поиск OK внутри запрещён. */
    state = grammar != LINES || line_count || cr ? DRAIN : SYNC_SEND;
    sync_seen = 0;
    started = now_ms;
    event(COMM_AT_DONE, NULL, 0, COMM_AT_TIMEOUT);
  }
  if ((state >= DRAIN && state <= SYNC_COMPLETE)
      && expired(now_ms, recovery)) {
    state = FAULT;
    event(COMM_AT_NO_ANSWER, NULL, 0, COMM_AT_TIMEOUT);
    return;
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
      if (state == IDLE && !line_count && !cr) started = now_ms;
      byte(*view);
    }
    if (state == FAULT) break;
    if (!comm_uart_consume(used) || comm_uart_errors()) { fault(COMM_AT_UART_ERROR); break; }
    budget = (uint16_t)(budget - used);
  }
  /* Оборванный URC без активной команды иначе навсегда запрещает новый AT. */
  if (state == IDLE && (line_count || cr) && expired(now_ms, recovery)) fault(COMM_AT_BAD_FRAME);
  if (state == CALL_DRAIN && expired(now_ms, timeout)) state = SYNC_SEND;
  if (state == DRAIN && grammar == LINES && !line_count && !cr) state = SYNC_SEND;
  if ((state == COMPLETE || state == SYNC_COMPLETE || state == SYNC_SEND)
      && comm_uart_tx_done() && !line_count && !cr && !comm_uart_peek(&view)) {
    if (state != SYNC_SEND) {
      uint8_t done = state == COMPLETE;
      state = IDLE;
      if (done) event(COMM_AT_DONE, NULL, modem_code, result);
    } else {
      /* Буфер меняется только после физического TC старой передачи. */
      memcpy(command, "AT+CMEE?\r", sizeof("AT+CMEE?\r") - 1u);
      command_count = sizeof("AT+CMEE?\r") - 1u;
      if (comm_uart_tx(command, command_count)) {
        if (usb_log_on(1)) usb_log(1, command, command_count - 1u);
        parser_reset();
        state = SYNC_WAIT;
      }
    }
  }
}

/* Возвращает capacity при переполнении; следующие куски ничего не пишут. */
uint16_t comm_at_copy(char *out, uint16_t capacity, uint16_t pos, const char *text)
{
  while (pos < capacity && *text) out[pos++] = *text++;
  if (pos < capacity) out[pos] = 0;
  return pos;
}
/* Добавить десятичное число в ограниченный буфер команды. */
uint16_t comm_at_decimal(char *out, uint16_t capacity, uint16_t pos, uint16_t value)
{
  uint16_t divisor = 10000u;
  while (divisor > value && divisor > 1u) divisor /= 10u;
  do {
    if (pos >= capacity) return capacity;
    out[pos++] = (char)('0' + value / divisor);
    value %= divisor; divisor /= 10u;
  } while (divisor);
  if (pos < capacity) out[pos] = 0;
  return pos;
}

/* Шаблон во флеш, выход — только числа текущего вызова.
   Число элементов out равно числу числовых меток доверенного шаблона. */
bool comm_at_parse(const uint8_t *data, uint16_t count, const char *pattern, uint32_t *out)
{
  uint16_t pos = 0;
  while (*pattern) {
    uint8_t token = (uint8_t)*pattern++;
    if (token == '*') return true;
    if (token == '|') return pos == count || data[pos] == ',';
    if (token <= 6) {
      if (token == 1 || token >= 5) pos = comm_at_spaces(data, count, pos);
      uint32_t limit = (token == 3 || token == 6) ? 255u : token >= 4 ? UINT16_MAX : UINT32_MAX;
      if (!comm_at_number(data, count, &pos, limit, out++)) return false;
    } else if (!take(data, count, &pos, token)) return false;
  }
  return pos == count;
}

/* Преобразовать цифру шестнадцатеричного тела SMS или USSD. */
uint8_t comm_at_hex(uint8_t value)
{
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10u;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10u;
  return 16;
}

/* Преобразовать общий для SMS и USSD алфавит UCS2 в cp1251. */
char comm_at_cp1251(uint16_t value)
{
  if (value < 0x80) return (char)value;
  if (value >= 0x410 && value <= 0x44F) return (char)(value - 0x410 + 0xC0);
  if (value == 0x401) return (char)0xA8;
  if (value == 0x451) return (char)0xB8;
  return '?';
}
