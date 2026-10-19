/* Доработка: Кряжев А., 2026.
   Сервер через Wi-Fi-мост USR-DR164 на линии RS-485: мост сам держит TCP, плата шлёт AT+SEND и опрашивает AT+RECV.
   Ответы читаются после тишины линии; данные RECV ложатся в общий слот frame, короткие ответы — в wf_line.
   Отказ моста переводит канал на GSM до нового сеанса; точка доступа задаётся тем же автоматом по USB 113.
   Включается в конец comm_channel.c. Написано 19.10.2026, на плате не проверено.
*/
#ifndef __UART_H /* стенд без драйвера USART1: прототипы совпадают с uart_stm32f10x.h */
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize);
void uart_send(uint8_t *buf, uint16_t size);
uint16_t uart_received(void);
uint8_t uart_getstate(void);
#endif
/* Сроки моста: опрос данных, проверка TCP, ожидание TCP после включения, ответ, ping, пауза после данных и перезапуска;
   серия включений: молчащий мост отдаёт сервер GSM через WF_DEAD_MS, отвечающий без TCP — через WF_GIVEUP_MS,
   с GSM мост пробуется снова раз в WF_RETRY_MS. */
enum { WF_POLL_MS = 100, WF_LINK_MS = 3000, WF_OPEN_MS = 40000, WF_CMD_MS = 800, WF_PING_MS = 6000,
       WF_DATA_MS = 30, WF_RESTART_MS = 8000, WF_CHUNK = 1000, WF_READ = 512,
       WF_DEAD_MS = 5000, WF_GIVEUP_MS = 60000, WF_RETRY_MS = 600000 };
enum { W_NONE, W_ECHO, W_MODE, W_RESTART, W_NETP, W_LINK, W_WSLK, W_SEND, W_DATA, W_RECV, W_PING,
       W_DIS_OFF, W_DIS_ON, W_SET_MODE, W_SET_SSID, W_SET_KEY };
static struct {
  uint32_t since, polled, linked, opening, series_at, fell_at;
  uint16_t count, offset, head;
  uint8_t op, link, prefix, setup, paused;
  struct { uint8_t series:1, answered:1, fell:1, resume:1; } f;
} wf;
static uint8_t wf_line[48];
/* Слот frame занят чтением или настройкой (на GSM любая операция моста - её часть): кадр не выдавать, мост опрашивать. */
static bool wifi_locked(void) { return wf.setup || wf.op == W_RECV || (!ch.wifi && wf.op); }
/* Новый сеанс канала: прежняя операция моста и серия включений забыты. */
static void wifi_reset(void) { wf.op = W_NONE; wf.prefix = 0; wf.f.series = wf.f.answered = wf.f.fell = wf.f.resume = 0; }
/* Канал идёт через мост или мост настраивается: порт RS-485 принадлежит каналу. */
bool comm_channel_wifi(void) { return ch.wifi || wf.setup; }
/* 0 - точки доступа нет, 1 - точка доступа есть, 2 - TCP сервера открыт. */
uint8_t comm_channel_wifi_link(void) { return wf.link; }
/* Съёмка занимает линию: операция моста отменяется, после съёмки TCP проверяется заново. */
void comm_channel_wifi_pause(bool on)
{
  if (on && wf.op == W_RECV) channel_read_done(0);
  if (on) { wf.op = W_NONE; wf.linked = ch.now - WF_LINK_MS; }
  wf.paused = (uint8_t)on;
}
/* Точка доступа по USB: "SSID\0ключ\0" копируется в свободный слот, автомат пишет её в мост и перезапускает его.
   0 - принято, 1 - занято (слот, съёмка или прежняя настройка), 2 - неверные данные. */
uint8_t comm_channel_wifi_setup(const uint8_t *data, uint16_t count)
{
  uint16_t ssid = 0;
  while (ssid < count && data[ssid]) ++ssid;
  if (!ssid || ssid > 32u || ssid >= count || count > 100u || data[count - 1u]) return 2;
  if (wf.setup || wf.paused || ch.tx_state != TX_FREE || wf.op == W_RECV) return 1;
  memcpy(frame, data, count);
  wf.setup = 1; return 0;
}
/* Собрать [USR]текст[число][хвост]<CR>, передать мосту и начать ожидание ответа. */
static void wifi_begin(uint8_t op, const char *text, uint16_t number, const char *tail)
{
  char cmd[80];
  uint16_t n = wf.prefix ? comm_at_copy(cmd, sizeof(cmd), 0, "USR") : 0;
  n = comm_at_copy(cmd, sizeof(cmd), n, text);
  if (number) n = comm_at_decimal(cmd, sizeof(cmd), n, number);
  if (tail) { if (number) n = comm_at_copy(cmd, sizeof(cmd), n, ","); n = comm_at_copy(cmd, sizeof(cmd), n, tail); }
  if (n + 1u >= sizeof(cmd)) n = (uint16_t)(sizeof(cmd) - 2u);
  cmd[n++] = '\r';
  if (op == W_RECV) uart_rebuffer(frame, (uint16_t)sizeof(frame));
  else uart_rebuffer(wf_line, (uint16_t)sizeof(wf_line));
  if (usb_log_on(1)) usb_log(1, (const uint8_t *)cmd, (uint16_t)(n - 1u)); /* команда мосту в AT-журнале */
  uart_send((uint8_t *)cmd, n);
  wf.op = op; wf.since = ch.now;
}
/* Ответ моста в USB-журнале: первые байты строкой и итог как у AT. */
static void wifi_log(uint8_t op, uint8_t result)
{
  uint16_t n = uart_received();
  (void)result; (void)n; /* стенд собирает журнал пустыми макросами */
  if (!usb_log_on(1) || op == W_DATA || op == W_RESTART) return;
  if (n) usb_log(2, op == W_RECV ? frame : wf_line, n > 40u ? 40u : n);
  usb_log_value(4, result == 1 ? COMM_AT_OK : n ? COMM_AT_ERROR : COMM_AT_TIMEOUT, 0);
}
/* Найти короткий ответ в начале принятого; -1 - нет. */
static int16_t wifi_find(const uint8_t *p, uint16_t n, const char *text)
{
  uint16_t len = (uint16_t)strlen(text);
  for (uint16_t i = 0; i + len <= n && i < 40u; ++i) if (!memcmp(p + i, text, len)) return (int16_t)i;
  return -1;
}
/* 0 - ответ ещё идёт, 1 - +ok, 2 - отказ или срок. Для RECV длина данных в wf.count, их начало - wf.head. */
static uint8_t wifi_result(void)
{
  uint32_t limit = wf.op == W_PING ? WF_PING_MS : wf.op == W_DATA ? WF_DATA_MS : WF_CMD_MS;
  bool late = ch.now - wf.since >= limit;
  if (wf.op == W_DATA || wf.op == W_RESTART) return (uint8_t)late; /* ответа не ждём */
  uint16_t n = uart_received();
  const uint8_t *p = wf.op == W_RECV ? frame : wf_line;
  int16_t at = wifi_find(p, n, "+ok");
  if (wf.op == W_RECV && at >= 0) {
    uint16_t pos = (uint16_t)(at + 4); uint32_t len = 0;
    if (pos > n || p[pos - 1u] != '=' || !comm_at_number(p, n, &pos, WF_READ, &len) || pos >= n) return late ? 2 : 0;
    wf.count = (uint16_t)len; wf.head = (uint16_t)(pos + 1u);
    if (!len) return 1;
    return n >= wf.head + len ? 1 : late ? 2 : 0;
  }
  if (!uart_getstate() && !late) return 0; /* байты ещё идут */
  if (at >= 0) return 1;
  if (wifi_find(p, n, "+ERR") >= 0) return 2;
  return late ? 2 : 0;
}
/* Назначить паузу перед новым включением; слот и окно как после закрытия TCP. */
static void wifi_pause(uint32_t pause_ms)
{
  ch.flags.recv = 0; ch.tx_state = TX_FREE; ch.frame_count = 0; ch.sent = ch.acked = 0;
  ch.state = CH_PAUSE; ch.reopen_pause = pause_ms; ch.operation_since = ch.now;
}
/* Серия включений моста не удалась: сервер идёт через GSM, мост пробуется снова через WF_RETRY_MS. */
static void wifi_fallback(void)
{
  if (ch.state == CH_OPEN) channel_emit(COMM_CH_CLOSED, 0);
  wifi_pause(0);
  ch.wifi = 0; wf.op = W_NONE; wf.link = 0; ch.state = CH_INITIAL;
  ch.connections = ch.retry_step = 0; ch.probe = ch.probe_active = 0;
  wf.f.series = wf.f.resume = 0; wf.f.fell = 1; wf.fell_at = ch.now;
  uart_rebuffer(wf_line, (uint16_t)sizeof(wf_line));
  if (usb_log_on(2)) usb_log_value(17, 10, 1);
}
/* Отказ настройки или включения: повтор через 2 с, пока не вышел срок серии; сервер об этом не узнаёт. */
static void wifi_fail(void)
{
  wf.setup = 0; wf.prefix = 0;
  if (!ch.wifi) return; /* настройка моста на GSM: состояние GSM-канала не трогать */
  if (ch.now - wf.series_at >= (wf.f.answered ? (uint32_t)WF_GIVEUP_MS : (uint32_t)WF_DEAD_MS)) { wifi_fallback(); return; }
  if (ch.state != CH_OPEN) wifi_pause(2000);
}
/* На GSM после отказа моста: раз в WF_RETRY_MS закрыть TCP и, дойдя до паузы, снова включить мост.
   true - мост только что включён: путь GSM в этом проходе не продолжать. */
static bool wifi_retry(void)
{
  if (ch.wifi || !wf.f.fell) return false;
  if (wf.f.resume) {
    if (ch.state != CH_PAUSE) return false;
    ch.wifi = 1; wifi_reset(); return true;
  }
  if (ch.state == CH_OPEN && ch.tx_state == TX_FREE && !ch.call.active && ch.now - wf.fell_at >= WF_RETRY_MS) {
    wf.f.resume = 1; comm_channel_reopen(0);
  }
  return false;
}
/* Передать мосту очередной кусок кадра не длиннее WF_CHUNK. */
static void wifi_send_chunk(void)
{
  uint16_t left = (uint16_t)(ch.frame_count - wf.offset);
  wf.count = left > WF_CHUNK ? WF_CHUNK : left;
  ch.tx_state = TX_SENDING;
  wifi_begin(W_SEND, "AT+SEND=", wf.count, NULL);
}
/* Применить законченную операцию моста; следующая начинается здесь же или в wifi_next. */
static void wifi_done(uint8_t op, uint8_t ok)
{
  const uint8_t *p = wf_line; uint16_t n = uart_received();
  switch (op) {
  case W_RECV:
    if (ok == 1 && wf.count) { ch.flags.read_active = 1; ch.bytes = 0; channel_binary(frame + wf.head, wf.count); }
    channel_read_done(0);
    if (ok != 1) wf.linked = ch.now - WF_LINK_MS; /* проверить TCP сразу */
    break;
  case W_SEND:
    if (ok == 1) { uart_rebuffer(wf_line, (uint16_t)sizeof(wf_line)); uart_send(frame + wf.offset, wf.count); wf.op = W_DATA; wf.since = ch.now; return; }
    comm_channel_reopen(0); break;
  case W_DATA:
    wf.offset = (uint16_t)(wf.offset + wf.count);
    if (wf.offset < ch.frame_count) { wifi_send_chunk(); return; }
    ch.tx_state = TX_FREE; ch.frame_count = 0; break;
  case W_LINK: {
    bool on = ok == 1 && wifi_find(p, n, "=on") >= 0;
    wf.linked = ch.now;
    if (on) {
      wf.link = 2;
      if (ch.state == CH_OPENING) { ch.state = CH_OPEN; wf.f.series = 0; channel_emit(COMM_CH_OPENED, 0); }
    } else {
      if (wf.link == 2) wf.link = 1;
      if (ch.state == CH_OPEN || ch.state == CH_PROBE) comm_channel_reopen(0);
      else if (ch.state == CH_OPENING) { wifi_begin(W_WSLK, "AT+WSLK", 0, NULL); return; }
    }
    break; }
  case W_WSLK: wf.link = (uint8_t)(ok == 1 && wifi_find(p, n, "Disconnected") < 0); break;
  case W_PING: {
    uint8_t good = (uint8_t)(ok == 1 && wifi_find(p, n, "Success") >= 0);
    probe_result = good ? ch.probe_active : 3u;
    ch.probe = ch.probe_active = 0; channel_emit(COMM_CH_PROBE_DONE, good); break; }
  case W_ECHO:
    if (ok == 1) {
      if (wf.setup) wifi_begin(W_SET_MODE, "AT+WMODE=STA", 0, NULL);
      else { uint16_t capacity = 0; const slm_settings_t *s = comm_modem_settings();
        const char *host = s->text(SLM_CFG_HOST, &capacity);
        wifi_begin(W_NETP, "AT+NETP=TCP,CLIENT,", s->number(SLM_CFG_PORT), host ? host : ""); }
      return;
    }
    if (!wf.prefix) { wf.prefix = 1; wifi_begin(W_MODE, "AT+TMODE=cmd", 0, NULL); return; } /* мост в прозрачном режиме */
    wifi_fail(); break;
  case W_MODE:
    if (ok == 1) { wifi_begin(wf.setup ? W_SET_MODE : W_RESTART, wf.setup ? "AT+WMODE=STA" : "AT+Z", 0, NULL); return; }
    wifi_fail(); break;
  case W_SET_MODE:
    if (ok == 1) { wifi_begin(W_SET_SSID, "AT+WSSSID=", 0, (const char *)frame); return; }
    wifi_fail(); break;
  case W_SET_SSID:
    if (ok == 1) { const char *key = (const char *)frame + strlen((const char *)frame) + 1u;
      wifi_begin(W_SET_KEY, "AT+WSKEY=", 0, *key ? key : "OPEN,NONE"); return; }
    wifi_fail(); break;
  case W_SET_KEY:
    wf.setup = 0;
    if (ok == 1) { wifi_begin(W_RESTART, "AT+Z", 0, NULL); return; }
    wifi_fail(); break;
  case W_RESTART:
    wf.prefix = 0; wf.link = 0;
    if (ch.wifi) wifi_pause(WF_RESTART_MS);
    else { wf.f.fell = 1; wf.fell_at = ch.now - WF_RETRY_MS + WF_RESTART_MS; } /* на GSM: мост перезапущен, попробовать его через 8 с */
    break;
  case W_NETP:
    if (ok == 1) { wf.opening = ch.now; wf.linked = ch.now - 1000u; } else wifi_fail();
    break;
  case W_DIS_OFF: wifi_begin(W_DIS_ON, "AT+TCPDIS=on", 0, NULL); return;
  case W_DIS_ON: wifi_pause(ch.reopen_pause); break;
  default: break;
  }
  uart_rebuffer(wf_line, (uint16_t)sizeof(wf_line));
}
/* Начать следующую операцию по состоянию канала; служебный запрос модему идёт в свободное время. */
static void wifi_next(void)
{
  if (wf.setup == 1) {
    if (ch.state == CH_OPEN) channel_emit(COMM_CH_CLOSED, 0);
    if (ch.wifi) wifi_pause(WF_RESTART_MS);
    wf.setup = 2; wf.prefix = 0;
    wifi_begin(W_ECHO, "AT+E=off", 0, NULL); return;
  }
  if (!ch.wifi) return;
  if (ch.state == CH_INITIAL) { wifi_pause(0); if (usb_log_on(2)) usb_log_value(17, 10, 0); }
  if (ch.state == CH_PAUSE) {
    if (ch.now - ch.operation_since < ch.reopen_pause) return;
    ch.state = CH_OPENING; ch.flags.peer_closed = 0; channel_emit(COMM_CH_OPENING, 0);
    ++ch.connections; wf.opening = ch.now; wf.prefix = 0;
    if (!wf.f.series) { wf.f.series = 1; wf.f.answered = 0; wf.series_at = ch.now; }
    wifi_begin(W_ECHO, "AT+E=off", 0, NULL); return;
  }
  if (ch.state == CH_CLOSE) { wifi_begin(W_DIS_OFF, "AT+TCPDIS=off", 0, NULL); return; }
  if (ch.state == CH_OPENING) {
    if (ch.now - wf.opening >= WF_OPEN_MS || ch.now - wf.series_at >= WF_GIVEUP_MS) { wifi_fail(); return; }
    if (ch.now - wf.linked >= 1000u) wifi_begin(W_LINK, "AT+TCPLK", 0, NULL);
    return;
  }
  if (ch.state != CH_OPEN && ch.state != CH_PROBE) return;
  if (ch.state == CH_OPEN && (ch.tx_state == TX_QUEUED || ch.tx_state == TX_RETRY)) { wf.offset = 0; wifi_send_chunk(); return; }
  if (ch.probe && !ch.probe_active) { ch.probe_active = ch.probe; wifi_begin(W_PING, "AT+PING=ya.ru", 0, NULL); return; }
  if (ch.state != CH_OPEN) return;
  if (ch.now - wf.linked >= WF_LINK_MS) { wifi_begin(W_LINK, "AT+TCPLK", 0, NULL); return; }
  if (ch.tx_state == TX_FREE && !ch.flags.urgent && ch.capacity && ch.now - wf.polled >= WF_POLL_MS) {
    wf.polled = ch.now; wifi_begin(W_RECV, "AT+RECV=", ch.capacity, "0"); return;
  }
  if (!ch.operation && query.cmd && comm_modem_ready() && !comm_modem_busy())
    channel_command(query.cmd, COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_QUERY);
}
/* Продвинуть мост: завершить текущую операцию и начать следующую; модем обслуживает только служебные запросы. */
static void wifi_poll(void)
{
  if (ch.operation == OP_QUERY) channel_completed();
  if (wf.paused) return;
  if (wf.op) {
    uint8_t r = wifi_result();
    if (!r) return;
    uint8_t op = wf.op; wf.op = W_NONE;
    if (r == 1 && op != W_DATA && op != W_RESTART) wf.f.answered = 1; /* мост жив: серия длиннее */
    wifi_log(op, r);
    wifi_done(op, r);
    if (wf.op) return;
  }
  wifi_next();
}
