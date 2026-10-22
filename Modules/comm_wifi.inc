/* Доработка: Кряжев А., 2026.
   DR164: прозрачный транспорт, RS-485 принадлежит только мосту либо камере.
   Проверено на плате №1 21.10.2026 (1792552760): прежний прозрачный обмен.
   Изменено 2026-10-21 (task/wifi-one): локальный выбор, выключенное ожидание,
   фоновая проверка без разрыва GSM и границы TCP; на плате не проверено.
   Проверено на плате №1 22.10.2026 (1792631158): настройка моста, WSLK/PING, закрытие GSM, регистрация на gate
   через мост (со второй попытки), 5 мин 60 TCPLK=on без переоткрытий.
   Изменено 2026-10-22: событие моста до регистрации не закрывает сеанс — на плате не проверено. */
#ifndef __UART_H
void uart_rebuffer(uint8_t *, uint16_t);
void uart_send(uint8_t *, uint16_t);
uint16_t uart_received(void);
uint8_t uart_getstate(void);
bool uart_isReceiving(void);
#endif
void user_comm_wifi_stopped(bool ok);
#ifdef __IAR_SYSTEMS_ICC__
#define WF_INLINE _Pragma("inline=forced")
#else
#define WF_INLINE
#endif
enum { WF_LINK_MS=5000, WF_OPEN_MS=40000, WF_CMD_MS=800, WF_PING_MS=6000,
       WF_RESTART_MS=8000, WF_GAP_MS=50, WF_RETRY_MS=600000, WF_LOG=40 };
enum { W_NONE, W_OFF, W_OFFB, W_ECHO, W_SET_MODE, W_SET_SSID, W_SET_KEY, W_EVENT,
       W_TCPTO, W_NETP, W_TMODE, W_RESTART, W_LINK, W_WSLK, W_PING, W_ON };
static struct {
  uint32_t since, fell_at, sent_at;
  uint8_t *buffer; // фактический буфер UART, независимо от смены владельца кадра
  union { uint16_t head, at; } pos; /* приём данных и AT-ответ взаимоисключены */
  uint16_t left;
  uint8_t op, link, prefix, setup, step, misses, mode, rx;
  struct { uint8_t ready, sent, off, retry, resume, drop, dirty, stop; } f;
} wf;
static uint8_t wf_line[96];
/* mode: 0 нет моста, 1 только GSM, 2 Wi-Fi с резервом GSM, 3 только Wi-Fi. */
WF_INLINE static uint8_t wifi_mode(void) { return (uint8_t)comm_modem_settings()->number(SLM_CFG_WIFI); }
static uint32_t wifi_delay(void) { return (wf.step < 2 ? 1u : wf.step == 2 ? 3u : 6u) * (uint32_t)WF_RETRY_MS; }
static void wifi_rebuffer(void)
{
  wf.buffer=ch.wifi && !wf.setup && ch.tx_state == TX_FREE ? frame : wf_line;
  uart_rebuffer(wf.buffer,wf.buffer == frame ? sizeof(frame) : sizeof(wf_line)); wf.pos.head=0;
}
/* Фоновый AT использует wf_line и не запирает слот GSM. */
static bool wifi_locked(void)
{ return wf.setup || (ch.wifi && (wf.op || (ch.tx_state == TX_FREE && uart_received()))); }
WF_INLINE static void wifi_reset(void)
{
  wf.op=W_NONE; wf.prefix=1; wf.rx=0; wf.mode=wifi_mode();
  wf.f.stop=comm_modem_settings()->number(SLM_CFG_WIFI_STOP) != 0 || wf.mode == 1;
  wf.f.resume=0; ch.connections=0;
  ch.wifi=wf.mode >= 2 && (!wf.f.retry || wf.mode == 3);
}
WF_INLINE static void wifi_registered(void) { if (ch.wifi) { wf.step=0; wf.f.retry=0; } }
bool comm_channel_wifi(void) { return (wf.mode | ch.wifi | wf.op | wf.f.stop) != 0; }
uint8_t comm_channel_wifi_link(void) { return wf.f.stop ? 5u : wf.link; }
/* Только после verify записи device/config; текущий кадр сначала отдаётся. */
void comm_channel_wifi_changed(void)
{
  uint8_t mode=wifi_mode();
  if (!mode && !wf.mode) return;
  wf.f.ready=0; wf.f.dirty=1;
  if (mode != wf.mode) {
    wf.step=0; wf.f.retry=wf.f.resume=0;
    wf.f.stop=mode < 2; wf.mode=mode;
  }
}
uint8_t comm_channel_wifi_setup(const uint8_t *data, uint16_t count)
{
  uint16_t ssid=0;
  while (ssid < count && data[ssid]) ++ssid;
  if (!wf.mode || ssid-1u >= 32u || count-ssid-2u > 63u || data[count-1u]) return 2;
  if ((wf.setup | wf.op) || ch.tx_state != TX_FREE || (ch.wifi && uart_received())) return 1;
  wf.f.ready=0; wf.setup=1; if (ch.wifi) wifi_rebuffer();
  memcpy(frame,data,count); return 0;
}

static void wifi_begin(uint8_t op)
{
  static const char * const command[]={"TCPDIS=off", "TCPDISB=off,1", "E=off", "WMODE=STA",
    "WSSSID=", "WSKEY=", "EVENT=on", "TCPTO=0", "NETP=TCP,CLIENT,", "TMODE=throughput",
    "Z", "TCPLK", "WSLK", "PING=ya.ru", "TCPDIS=on,0"};
  const char *text=command[op-W_OFF], *tail=NULL;
  uint16_t number=0;
  if (op == W_OFF) tail=wf.f.ready && !wf.f.stop ? ",0" : ",1";
  if (op == W_SET_SSID) tail=(const char *)frame;
  if (op == W_SET_KEY) { tail=(const char *)frame+strlen((const char *)frame)+1u; if (!*tail) tail="OPEN,NONE"; }
  if (op == W_NETP) { const slm_settings_t *s=comm_modem_settings(); uint16_t cap=0;
    number=s->number(SLM_CFG_PORT); tail=s->text(SLM_CFG_HOST,&cap); }
  char cmd[80];
  uint16_t n = comm_at_copy(cmd, sizeof(cmd), 0, wf.prefix ? "USRAT+" : "AT+");
  n = comm_at_copy(cmd, sizeof(cmd), n, text);
  if (number) n = comm_at_decimal(cmd, sizeof(cmd), n, number);
  if (tail) { if (number) n = comm_at_copy(cmd, sizeof(cmd), n, ","); n = comm_at_copy(cmd, sizeof(cmd), n, tail); }
  if (n + 1u >= sizeof(cmd)) n = (uint16_t)(sizeof(cmd) - 2u);
  cmd[n++] = '\r';
  wifi_rebuffer();
  usb_log(1, (const uint8_t *)cmd, (uint16_t)(n - 1u)); /* команда мосту в AT-журнале */
  uart_send((uint8_t *)cmd, n);
  wf.op = op; wf.since = ch.now;
}

static void wifi_log(uint8_t op)
{
  if (op == W_RESTART) return;
  uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
  (void)n; (void)p; /* стенд собирает журнал пустыми макросами */
  if (n) usb_log(2, p, n > WF_LOG ? WF_LOG : n);
  else usb_log_value(4, COMM_AT_TIMEOUT, 0);
}

static int16_t wifi_find(const uint8_t *p, uint16_t n, const char *text)
{
  uint16_t len = (uint16_t)strlen(text);
  for (uint16_t i = 0; i + len <= n; ++i) if (!memcmp(p + i, text, len)) return (int16_t)i;
  return -1;
}

WF_INLINE static uint8_t wifi_result(void)
{
  bool late = ch.now - wf.since >= (wf.op == W_PING ? (uint32_t)WF_PING_MS : (uint32_t)WF_CMD_MS);
  if (wf.op == W_RESTART) return ch.now - wf.since >= WF_RESTART_MS; /* ответа не ждём */
  /* Линия полудуплексная: разбирать только после тишины 10 мс, иначе следующая посылка
     столкнётся с хвостом ответа моста (проверено на плате в командном режиме). */
  if (!uart_getstate() && !late) return 0;
  uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
  int16_t at = wifi_find(p, n, "+ok");
  if (at < 0) at = wifi_find(p, n, "+ERR");
  if (at < 0) { wf.pos.at = n; return late ? 2 : 0; }
  wf.pos.at = (uint16_t)at;
  return p[at + 1] == 'o' ? 1 : 2;
}

/* Следить за длиной SLDN: текст EVENT внутри бинарного тела не является событием.
   Служебные строки вне SLDN штатный приёмник отбрасывает. Состояние переживает дробление UART. */
WF_INLINE static bool wifi_boundary(const uint8_t *p, uint16_t n)
{
  while (n--) {
    uint8_t c=*p++;
    if (wf.rx == 6) { if (!--wf.left) wf.rx=0; continue; }
    if (wf.rx == 4) { wf.left=c; wf.rx=5; continue; }
    if (wf.rx == 5) {
      wf.left |= (uint16_t)c << 8;
      wf.rx=wf.left >= 3u && wf.left <= 2040u ? 6u : 0u;
      wf.left+=2u; continue;
    }
    // Индексы 4..6 заняты длиной/телом SLDN, 7..23 — текстом EVENT.
    if (c == (uint8_t)"SLDN\0\0\0+EVENT=SOCKA_OFF\r"[wf.rx]) { if (++wf.rx == 24) { wf.rx=0; return true; } }
    else if (wf.rx == 21 && c == 'N') wf.rx=23;
    else wf.rx=c == 'S' ? 1u : c == '+' ? 8u : 0u;
  }
  return false;
}

static uint16_t wifi_deliver(const uint8_t *p, uint16_t n)
{
  if (ch.state != CH_OPEN || !n) return n;
  ch.flags.read_active = 1; ch.bytes = 0;
  uint16_t used = channel_binary(p, n);
  channel_read_done(0);
  /* SOCKA_ON/OFF до регистрации — это открываемый сейчас сеанс (событие может прийти после TCPLK=on),
     после регистрации — мост переподключился сам, сервер плату уже не помнит. Код 11: 0 до, 1 после. */
  if (wifi_boundary(p,used)) {
    usb_log_value(17,11,ch.flags.registered_session);
    if (ch.flags.registered_session) comm_channel_reopen(0);
  }
  return used;
}

static void wifi_strip(void)
{
  uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
  if (p != frame) return; /* слот занят: данных сервера в приёме нет */
  uint16_t end = wf.pos.at;
  while (end < n && p[end] != '\r' && p[end] != '\n') ++end;
  while (end < n && (p[end] == '\r' || p[end] == '\n')) ++end;
  wifi_deliver(p, wf.pos.at); wifi_deliver(p + end, (uint16_t)(n - end));
}

WF_INLINE static void wifi_rx(void)
{
  if (ch.state != CH_OPEN || wf.op || wf.setup || ch.tx_state != TX_FREE || !uart_getstate()) return;
  uint16_t n = uart_received();
  if (n > wf.pos.head) wf.pos.head = (uint16_t)(wf.pos.head + wifi_deliver(frame + wf.pos.head, (uint16_t)(n - wf.pos.head)));
  if (wf.pos.head >= n) wifi_rebuffer();
}

WF_INLINE static void wifi_hold(void) { wifi_rebuffer(); }

static void wifi_unhold(void)
{
  uint16_t n = uart_received();
  wifi_deliver(wf_line, n);
  wifi_rebuffer();
}

WF_INLINE static void wifi_send(void)
{
  uart_send(frame, ch.frame_count);
  ch.tx_state = TX_FREE; ch.frame_count = 0; ch.sent = ch.acked = 0; wf.f.sent = 1;
  wifi_unhold();
}

static void wifi_pause(uint32_t pause_ms)
{
  ch.flags.recv = 0; ch.tx_state = TX_FREE; ch.frame_count = 0; ch.sent = ch.acked = 0;
  ch.state = CH_PAUSE; ch.reopen_pause = pause_ms; ch.operation_since = ch.now;
}

static void wifi_gsm(void)
{
  ch.wifi=0; ch.state=CH_INITIAL; ch.connections=ch.retry_step=0;
  ch.probe=ch.probe_active=0; ch.flags.registered_session=0; wf.f.resume=0;
  wf.rx=0; wifi_rebuffer();
  usb_log_value(17,10,1);
}
/* Неудача регистрации: один сеанс, затем 10/30/60 минут. Только Wi-Fi не включает GSM TCP. */
static void wifi_fallback(void)
{
  if (ch.state == CH_OPEN) channel_emit(COMM_CH_CLOSED,0);
  if (ch.state == CH_OPEN && wf.step < 3) ++wf.step;
  wf.f.retry=1; wf.fell_at=ch.now; wf.f.drop=1; wf.f.resume=0;
  wf.since=ch.now-WF_LINK_MS;
  if (wf.mode == 3) wifi_pause(wifi_delay()); else wifi_gsm();
}
static void wifi_fail(void)
{
  wf.setup=0; wf.f.ready=0;
  if (ch.wifi) wifi_fallback();
  else { wf.f.retry=1; wf.fell_at=ch.now; }
}
static void wifi_off(void)
{ wf.misses=0; wf.prefix=1; wifi_begin(W_OFF); }
/* Проверка идёт своим UART/буфером. Закрывать GSM разрешает только успешный PING. */
WF_INLINE static bool wifi_retry(void)
{
  if (ch.wifi || wf.mode < 2) return false;
  if (wf.mode == 3) wf.f.resume=1;
  if (wf.f.resume) {
    if (ch.state == CH_PAUSE || ch.state == CH_INITIAL) {
      ch.wifi=1; wf.f.resume=0; wf.rx=0; ch.reopen_pause=0; ch.connections=0;
      wifi_rebuffer(); return true;
    }
    if (ch.state == CH_OPEN && ch.tx_state == TX_FREE && ch.sent == ch.acked && !ch.operation && !ch.call.active)
      comm_channel_reopen(0);
    return false;
  }
  if (wf.op || wf.setup || wf.f.drop) return false;
  if (wf.f.retry && ch.now-wf.fell_at >= wifi_delay() && !uart_isReceiving()) {
    wf.fell_at=ch.now;
    if (!wf.f.ready || !wf.f.off) wifi_off();
    else wifi_begin(W_WSLK);
  }
  return false;
}
WF_INLINE static void wifi_done(uint8_t op, uint8_t ok)
{
  uint16_t n=uart_received(); const uint8_t *p=wf.buffer;
  /* Последовательная настройка: один путь ошибки и один отправитель. */
  if (op >= W_OFFB && op < W_RESTART) {
    if (ok != 1) { wifi_fail(); wifi_rebuffer(); return; }
    ++op;
    if (op == W_SET_MODE && !wf.setup) op=W_EVENT;
    if (op == W_EVENT) wf.setup=0;
    wifi_begin(op); return;
  }
  switch (op) {
  case W_OFF:
    if (ok != 1) {
      if (++wf.misses <= 3) {
        wf.prefix=wf.misses < 3;
        wifi_begin(W_OFF); return;
      }
      wf.f.off=0;
      if (wf.f.stop) { wf.f.stop=0; user_comm_wifi_stopped(false); }
      if (wf.mode < 2) { wf.f.dirty=wf.f.drop=0; wf.setup=0; if (ch.wifi) wifi_gsm(); }
      else { wifi_fail(); wf.f.drop=0; } /* молчащий мост: повтор по расписанию 10/30/60 мин, не каждые 5 с */
      wf.link=4; break;
    }
    wf.f.off=1; wf.f.drop=0; wf.link=3;
    wf.f.stop=0; user_comm_wifi_stopped(true);
    if (wf.mode < 2 && !wf.setup) { wf.f.dirty=0; if (ch.wifi) wifi_gsm(); break; }
    if (!wf.f.ready) { wifi_begin(W_OFFB); return; }
    if (ch.wifi && ch.state == CH_CLOSE) wifi_pause(ch.reopen_pause);
    break;
  case W_RESTART:
    wf.f.ready=1; wf.prefix=1;
    if (!ch.wifi && wf.mode >= 2) { wf.f.retry=1; wf.fell_at=ch.now-wifi_delay(); }
    wifi_off(); return; /* Явное выключение после запуска, без предположения о сохранённом состоянии. */
  case W_ON:
    if (ok == 1) { wf.f.off=0; wf.since=ch.now-1000u; } else wifi_fail();
    break;
  case W_LINK: {
    bool on=ok == 1 && wifi_find(p,n,"+ok=on") >= 0;
    wf.since=ch.now;
    if (on) {
      wf.link=2;
      if (ch.state == CH_OPENING) { ch.state=CH_OPEN; wf.rx=0; channel_emit(COMM_CH_OPENED,0); }
    } else {
      wf.link=ok == 1;
      if (ch.state == CH_OPEN || ch.state == CH_PROBE) comm_channel_reopen(0);
    }
    break; }
  case W_WSLK:
    wf.link=ok == 1 && wifi_find(p,n,"Disconnected") < 0;
    if (wf.link) { wifi_begin(W_PING); return; }
    break;
  case W_PING: {
    uint8_t good=ok == 1 && wifi_find(p,n,"Success") >= 0;
    if (!ch.wifi) wf.f.resume=good;
    else { probe_result=good ? ch.probe_active : 3u; ch.probe=ch.probe_active=0; channel_emit(COMM_CH_PROBE_DONE,good); }
    break; }
  default: break;
  }
  wifi_rebuffer();
}
WF_INLINE static void wifi_next(void)
{
  if (comm_modem_settings()->number(SLM_CFG_RS485_BUSY)) return;
  if (ch.wifi && ch.state == CH_OPEN && (ch.tx_state == TX_QUEUED || ch.tx_state == TX_RETRY)) {
    if (!uart_isReceiving()) wifi_send(); return;
  }
  if (uart_isReceiving() || ch.now-wf.sent_at < WF_GAP_MS) return;
  if (wf.f.dirty || wf.setup == 1) {
    if (ch.tx_state != TX_FREE) return;
    if (ch.wifi) { if (ch.state == CH_OPEN) channel_emit(COMM_CH_CLOSED,0); wifi_pause(0); }
    if (wf.setup) wf.setup=2;
    wf.f.dirty=0; wifi_off(); return;
  }
  if (wf.f.stop) { wifi_off(); return; }
  if (wf.f.drop) { if (ch.now-wf.since >= WF_LINK_MS) wifi_off(); return; }
  if (!ch.wifi) return;
  if (ch.state == CH_INITIAL) wifi_pause(0);
  if (ch.state == CH_PAUSE) {
    if (ch.now-ch.operation_since < ch.reopen_pause) return;
    if (!wf.f.ready || !wf.f.off) { wifi_off(); return; }
    ch.state=CH_OPENING; ch.operation_since=ch.now; ch.flags.registered_session=0;
    ch.flags.peer_closed=0; ++ch.connections; channel_emit(COMM_CH_OPENING,0);
    wf.rx=0; wifi_begin(W_ON); return;
  }
  if (ch.state == CH_CLOSE) { wifi_off(); return; }
  if (ch.state == CH_OPENING) {
    if (ch.now-ch.operation_since >= WF_OPEN_MS) { wifi_fallback(); return; }
    if (ch.now-wf.since >= 1000u) wifi_begin(W_LINK);
    return;
  }
  if (ch.state != CH_OPEN && ch.state != CH_PROBE) return;
  if (ch.tx_state != TX_FREE || uart_received()) return;
  if (ch.probe && !ch.probe_active) { ch.probe_active=ch.probe; wifi_begin(W_PING); return; }
  if (ch.state != CH_OPEN) return;
  if (ch.now-wf.since >= WF_LINK_MS) { wifi_begin(W_LINK); return; }
  if (!ch.operation && query.cmd && comm_modem_ready() && !comm_modem_busy())
    channel_command(query.cmd,COMM_AT_TEXT,COMM_T_AT_SIMPLE,OP_QUERY);
}
WF_INLINE static void wifi_poll(void)
{
  if (wf.f.sent) { wf.f.sent=0; wf.sent_at=ch.now; }
  if (ch.wifi && ch.operation == OP_QUERY) channel_completed();
  if (wf.op) {
    uint8_t r=wifi_result();
    if (!r) return;
    uint8_t op=wf.op;
    wifi_log(op);
    if (op != W_RESTART) wifi_strip();
    wf.op=W_NONE;
    wifi_done(op,r);
    if (wf.op) return;
  }
  wifi_rx(); wifi_next();
}
