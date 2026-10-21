/* Доработка: Кряжев А., 2026.
   Сервер через Wi-Fi-мост USR-DR164 на линии RS-485 в прозрачном режиме: мост сам держит TCP, кадры идут
   в линию и с линии как есть, команды мосту - с префиксом USR (AT+CMDPW по умолчанию). Приём с линии -
   в общий слот frame, пока он свободен; ответ моста ищется в принятом, байты вокруг него - данные сервера.
   Отказ моста переводит канал на GSM до нового сеанса; точка доступа задаётся тем же автоматом по USB 113.
   Включается в конец comm_channel.c. Прозрачный режим написан вечером 19.10.2026; проверен на плате №1 (aa0c162, сборка 1792419754): мост из командного
   режима настроен и перезапущен прошивкой, регистрация и статусы через мост со стендовым сервером, 4 мин без переоткрытий.
   Изменено 2026-10-21 (3cc4118): отключение TCP перед камерой и новый сеанс после неё — на плате не проверено.
   Изменено 2026-10-21 (после 09eef64): пауза перед командой мосту от конца кадра; закрытие на съёмку плановое — на плате не проверено.
*/
#ifndef __UART_H /* стенд без драйвера USART1: прототипы совпадают с uart_stm32f10x.h */
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize);
void uart_send(uint8_t *buf, uint16_t size);
uint16_t uart_received(void);
uint8_t uart_getstate(void);
bool uart_isReceiving(void);
#endif
/* Сроки моста: проверка TCP в работе, ожидание TCP после включения, ответ, ping, пауза после перезапуска,
   тишина после своего кадра (мост пакует линию по 20 мс: команда сразу за кадром ушла бы серверу);
   серия включений: молчащий мост отдаёт сервер GSM через WF_DEAD_MS, отвечающий без TCP - через WF_GIVEUP_MS,
   с GSM мост пробуется снова раз в WF_RETRY_MS; WF_MISSES ответов подряд нет - мост настраивается заново. */
enum { WF_LINK_MS = 5000, WF_OPEN_MS = 40000, WF_CMD_MS = 800, WF_PING_MS = 6000, WF_RESTART_MS = 8000,
       WF_GAP_MS = 50, WF_DEAD_MS = 5000, WF_GIVEUP_MS = 60000, WF_RETRY_MS = 600000, WF_MISSES = 3, WF_LOG = 40 };
enum { W_NONE, W_ECHO, W_SET_MODE, W_SET_SSID, W_SET_KEY, W_EVENT, W_TCPTO, W_NETP, W_TMODE, W_RESTART,
       W_LINK, W_WSLK, W_PING, W_DIS_OFF, W_DIS_ON, W_CAMERA_OFF, W_CAMERA_ON };
enum { WF_CAMERA_WAIT = 1, WF_CAMERA_HELD, WF_CAMERA_RELEASE };
static struct {
  uint32_t since, linked, opening, series_at, fell_at, sent_at;
  uint16_t head, at;
  uint8_t op, link, prefix, setup, paused, misses;
  struct { uint8_t series:1, answered:1, fell:1, resume:1, ready:1, sent:1; } f;
} wf;
static uint8_t wf_line[96]; /* ответы моста, пока слот frame занят кадром или настройкой */
/* Приёмный буфер линии: frame, пока канал через мост и слот свободен; иначе короткий wf_line. */
static uint8_t *wifi_rxbuf(uint16_t *size)
{
  bool big = ch.wifi && !wf.setup && ch.tx_state == TX_FREE;
  *size = big ? (uint16_t)sizeof(frame) : (uint16_t)sizeof(wf_line);
  return big ? frame : wf_line;
}
/* Начать приём заново в подходящий буфер; принятое раньше забыто. */
static void wifi_rebuffer(void)
{ uint16_t size; uint8_t *p = wifi_rxbuf(&size); uart_rebuffer(p, size); wf.head = 0; }
/* Слот frame занят настройкой, ожиданием ответа моста или ещё не розданными данными сервера. */
static bool wifi_locked(void)
{ return wf.setup || wf.op || (ch.wifi && (wf.paused || (ch.tx_state == TX_FREE && uart_received()))); }
/* Новый сеанс канала: операция моста и серия включений забыты; что мост уже настроен (ready) - помнится. */
static void wifi_reset(void) { wf.op = W_NONE; wf.prefix = 1; wf.f.series = wf.f.answered = wf.f.fell = wf.f.resume = 0; ch.connections = 0; }
/* Канал идёт через мост или мост настраивается: порт RS-485 принадлежит каналу. */
bool comm_channel_wifi(void) { return ch.wifi || wf.setup || wf.paused; }
/* 0 - точки доступа нет, 1 - точка доступа есть, 2 - TCP сервера открыт. */
uint8_t comm_channel_wifi_link(void) { return wf.link; }
/* Мост остаётся прозрачным, но его TCP закрывается до байтов камеры.
   Без подтверждения и тишины линии камера не получает порт; срок ожидания — у владельца фото. */
bool comm_channel_wifi_pause(bool on)
{
  if (on) {
    if (!comm_modem_settings()->number(SLM_CFG_WIFI)) return true;
    if (!wf.paused) {
      if (wf.setup || !wf.f.ready || (ch.wifi && ch.tx_state != TX_FREE)) return false; /* сначала ответ на команду 7 */
      wf.paused = WF_CAMERA_WAIT; wf.sent_at = ch.now;
      if (ch.wifi) {
        /* Съёмка не штрафует новый сеанс и не стирает прежние отказы регистрации. */
        uint8_t attempts = ch.connections;
        ch.connections = 0; channel_reopen(0, 1); ch.connections = attempts;
      }
    }
    return wf.paused == WF_CAMERA_HELD && !uart_isReceiving() && ch.now - wf.sent_at >= WF_GAP_MS;
  }
  if (wf.paused && wf.paused != WF_CAMERA_RELEASE) {
    wf.paused = WF_CAMERA_RELEASE; wf.sent_at = ch.now;
    wifi_rebuffer();
  }
  return true;
}
/* Точка доступа по USB: "SSID\0ключ\0" копируется в свободный слот, автомат пишет её в мост и перезапускает его.
   0 - принято, 1 - занято (слот, съёмка, ответ моста или прежняя настройка), 2 - неверные данные. */
uint8_t comm_channel_wifi_setup(const uint8_t *data, uint16_t count)
{
  uint16_t ssid = 0;
  while (ssid < count && data[ssid]) ++ssid;
  if (!ssid || ssid > 32u || ssid >= count || count > 100u || data[count - 1u]) return 2;
  if (wf.setup || wf.paused || ch.tx_state != TX_FREE || wf.op || (ch.wifi && uart_received())) return 1;
  wf.setup = 1; if (ch.wifi) wifi_rebuffer(); /* приём в wf_line: данные сервера не затрут SSID */
  memcpy(frame, data, count); return 0;
}
/* Пометка в AT-журнале USB: "TX n"/"RX n" - кадры линии в обе стороны. */
static void wifi_note(uint8_t kind, const char *text, uint16_t value)
{
  char note[12]; uint16_t n; (void)kind; (void)note; (void)n;
  if (!usb_log_on(1)) return;
  n = comm_at_copy(note, sizeof(note), 0, text);
  n = comm_at_decimal(note, sizeof(note), n, value);
  usb_log(kind, (const uint8_t *)note, n);
}
/* Собрать [USR]текст[число][,хвост]<CR>, передать мосту и начать ожидание ответа. */
static void wifi_begin(uint8_t op, const char *text, uint16_t number, const char *tail)
{
  char cmd[80];
  uint16_t n = wf.prefix ? comm_at_copy(cmd, sizeof(cmd), 0, "USR") : 0;
  n = comm_at_copy(cmd, sizeof(cmd), n, text);
  if (number) n = comm_at_decimal(cmd, sizeof(cmd), n, number);
  if (tail) { if (number) n = comm_at_copy(cmd, sizeof(cmd), n, ","); n = comm_at_copy(cmd, sizeof(cmd), n, tail); }
  if (n + 1u >= sizeof(cmd)) n = (uint16_t)(sizeof(cmd) - 2u);
  cmd[n++] = '\r';
  wifi_rebuffer();
  if (usb_log_on(1)) usb_log(1, (const uint8_t *)cmd, (uint16_t)(n - 1u)); /* команда мосту в AT-журнале */
  uart_send((uint8_t *)cmd, n);
  wf.op = op; wf.since = ch.now;
}
/* Ответ моста в USB-журнале: первые байты строкой и итог как у AT. */
static void wifi_log(uint8_t op, uint8_t result)
{
  uint16_t size, n = uart_received(); const uint8_t *p = wifi_rxbuf(&size);
  (void)result; (void)n; (void)p; /* стенд собирает журнал пустыми макросами */
  if (!usb_log_on(1) || op == W_RESTART) return;
  if (n) usb_log(2, p, n > WF_LOG ? WF_LOG : n);
  usb_log_value(4, result == 1 ? COMM_AT_OK : n ? COMM_AT_ERROR : COMM_AT_TIMEOUT, 0);
}
/* Найти текст в принятом (вокруг ответа могут лежать данные сервера); -1 - нет. */
static int16_t wifi_find(const uint8_t *p, uint16_t n, const char *text)
{
  uint16_t len = (uint16_t)strlen(text);
  for (uint16_t i = 0; i + len <= n; ++i) if (!memcmp(p + i, text, len)) return (int16_t)i;
  return -1;
}
/* 0 - ответ ещё идёт, 1 - +ok, 2 - +ERR или срок; начало ответа - wf.at (без ответа - конец принятого). */
static uint8_t wifi_result(void)
{
  bool late = ch.now - wf.since >= (wf.op == W_PING ? (uint32_t)WF_PING_MS : (uint32_t)WF_CMD_MS);
  if (wf.op == W_RESTART) return (uint8_t)late; /* ответа не ждём */
  /* Линия полудуплексная: разбирать только после тишины 10 мс, иначе следующая посылка
     столкнётся с хвостом ответа моста (проверено на плате в командном режиме). */
  if (!uart_getstate() && !late) return 0;
  uint16_t size, n = uart_received(); const uint8_t *p = wifi_rxbuf(&size);
  int16_t at = wifi_find(p, n, "+ok");
  if (at < 0) at = wifi_find(p, n, "+ERR");
  if (at < 0) { wf.at = n; return late ? 2 : 0; }
  wf.at = (uint16_t)at;
  return p[at + 1] == 'o' ? 1 : 2;
}
/* Отдать байты линии приёмнику сервера (только при открытом канале, иначе они отброшены); вернуть, сколько взято. */
static uint16_t wifi_deliver(const uint8_t *p, uint16_t n)
{
  if (ch.state != CH_OPEN || !n) return n;
  wifi_note(2, "RX ", n);
  ch.flags.read_active = 1; ch.bytes = 0;
  uint16_t used = channel_binary(p, n);
  channel_read_done(0);
  return used;
}
/* Ответ моста разобран: байты до него и после его строки - данные сервера. */
static void wifi_strip(void)
{
  uint16_t size, n = uart_received(); const uint8_t *p = wifi_rxbuf(&size);
  if (p != frame) return; /* слот занят: данных сервера в приёме нет */
  uint16_t end = wf.at;
  while (end < n && p[end] != '\r' && p[end] != '\n') ++end;
  while (end < n && (p[end] == '\r' || p[end] == '\n')) ++end;
  wifi_deliver(p, wf.at); wifi_deliver(p + end, (uint16_t)(n - end));
}
/* Данные сервера пришли сами: после тишины линии отдать их приёмнику; остаток ждёт места в нём. */
static void wifi_rx(void)
{
  if (ch.state != CH_OPEN || wf.op || wf.setup || ch.tx_state != TX_FREE || !uart_getstate()) return;
  uint16_t n = uart_received();
  if (n > wf.head) wf.head = (uint16_t)(wf.head + wifi_deliver(frame + wf.head, (uint16_t)(n - wf.head)));
  if (wf.head >= n) wifi_rebuffer();
}
/* Слот выдан производителю: приём линии уходит в wf_line, кадр в frame не затирается. */
static void wifi_hold(void) { if (!wf.paused) wifi_rebuffer(); }
/* Слот свободен: принятое за это время отдаётся, приём возвращается в frame. */
static void wifi_unhold(void)
{
  if (wf.paused) return;
  uint16_t n = uart_received();
  if (n) wifi_deliver(wf_line, n);
  wifi_rebuffer();
}
/* Кадр в линию как есть: мост упакует его в один TCP-пакет; подтверждений нет, слот свободен сразу.
   uart_send ждёт последний байт, а ch.now взят до отправки: паузу WF_GAP_MS отсчитывает следующий проход. */
static void wifi_send(void)
{
  wifi_note(1, "TX ", ch.frame_count);
  uart_send(frame, ch.frame_count);
  ch.tx_state = TX_FREE; ch.frame_count = 0; ch.sent = ch.acked = 0; wf.f.sent = 1;
  wifi_unhold();
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
  wifi_rebuffer(); /* на GSM линию чистит порт RS-485 */
  if (usb_log_on(2)) usb_log_value(17, 10, 1);
}
/* Отказ настройки или включения: повтор через 2 с, пока не вышел срок серии; сервер об этом не узнаёт. */
static void wifi_fail(void)
{
  wf.setup = 0;
  if (!ch.wifi) return; /* настройка моста на GSM: состояние GSM-канала не трогать */
  if (ch.now - wf.series_at >= (wf.f.answered ? (uint32_t)WF_GIVEUP_MS : (uint32_t)WF_DEAD_MS)) { wifi_fallback(); return; }
  if (ch.state != CH_OPEN) wifi_pause(2000);
}
/* На GSM после отказа моста: раз в WF_RETRY_MS закрыть TCP и, дойдя до паузы, снова включить мост.
   true - мост только что включён: путь GSM в этом проходе не продолжать. */
static bool wifi_retry(void)
{
  if (ch.wifi || wf.paused || !wf.f.fell) return false;
  if (wf.f.resume) {
    if (ch.state != CH_PAUSE) return false;
    ch.wifi = 1; wifi_reset(); return true;
  }
  if (ch.state == CH_OPEN && ch.tx_state == TX_FREE && !ch.call.active && ch.now - wf.fell_at >= WF_RETRY_MS) {
    wf.f.resume = 1; comm_channel_reopen(0);
  }
  return false;
}
/* Применить законченную операцию моста; следующая начинается здесь же или в wifi_next.
   Настройка: E=off, [WMODE, WSSSID, WSKEY,] EVENT=off, TCPTO=0, NETP, TMODE=throughput, Z - один раз за включение платы. */
static void wifi_done(uint8_t op, uint8_t ok)
{
  uint16_t size, n = uart_received(); const uint8_t *p = wifi_rxbuf(&size);
  switch (op) {
  case W_ECHO:
    if (ok == 1) { wifi_begin(wf.setup ? W_SET_MODE : W_EVENT, wf.setup ? "AT+WMODE=STA" : "AT+EVENT=off", 0, NULL); return; }
    if (wf.prefix) { wf.prefix = 0; wifi_begin(W_ECHO, "AT+E=off", 0, NULL); return; } /* мост в командном режиме: без префикса */
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
    if (ok == 1) { wifi_begin(W_EVENT, "AT+EVENT=off", 0, NULL); return; }
    wifi_fail(); break;
  case W_EVENT: /* уведомления моста не должны попадать в поток данных */
    if (ok == 1) { wifi_begin(W_TCPTO, "AT+TCPTO=0", 0, NULL); return; }
    wifi_fail(); break;
  case W_TCPTO: { /* мост не рвёт TCP по тишине */
    if (ok != 1) { wifi_fail(); break; }
    uint16_t capacity = 0; const slm_settings_t *s = comm_modem_settings();
    const char *host = s->text(SLM_CFG_HOST, &capacity);
    wifi_begin(W_NETP, "AT+NETP=TCP,CLIENT,", s->number(SLM_CFG_PORT), host ? host : ""); return; }
  case W_NETP:
    if (ok == 1) { wifi_begin(W_TMODE, "AT+TMODE=throughput", 0, NULL); return; }
    wifi_fail(); break;
  case W_TMODE:
    if (ok == 1) { wifi_begin(W_RESTART, "AT+Z", 0, NULL); return; }
    wifi_fail(); break;
  case W_RESTART: /* после перезапуска мост в прозрачном режиме с новыми настройками */
    wf.f.ready = 1; wf.prefix = 1; wf.link = 0;
    if (ch.wifi) wifi_pause(WF_RESTART_MS);
    else { wf.f.fell = 1; wf.fell_at = ch.now - WF_RETRY_MS + WF_RESTART_MS; } /* на GSM: попробовать мост через 8 с */
    break;
  case W_LINK: {
    bool on = ok == 1 && n - wf.at >= 6u && !memcmp(p + wf.at, "+ok=on", 6);
    wf.linked = ch.now;
    if (ok == 1) wf.misses = 0; else if (++wf.misses >= WF_MISSES) wf.f.ready = 0; /* мост не отвечает: настроить заново */
    if (on) {
      wf.link = 2;
      if (ch.state == CH_OPENING) { ch.state = CH_OPEN; wf.f.series = 0; channel_emit(COMM_CH_OPENED, 0); }
    } else {
      if (wf.link == 2) wf.link = 1;
      if (ch.state == CH_OPEN || ch.state == CH_PROBE) comm_channel_reopen(0);
      else if (ch.state == CH_OPENING) {
        if (!wf.f.ready) { wifi_fail(); break; }
        if (ok == 1) { wifi_begin(W_WSLK, "AT+WSLK", 0, NULL); return; }
      }
    }
    break; }
  case W_WSLK: wf.link = (uint8_t)(ok == 1 && wifi_find(p, n, "Disconnected") < 0); break;
  case W_PING: {
    uint8_t good = (uint8_t)(ok == 1 && wifi_find(p, n, "Success") >= 0);
    probe_result = good ? ch.probe_active : 3u;
    ch.probe = ch.probe_active = 0; channel_emit(COMM_CH_PROBE_DONE, good); break; }
  case W_DIS_OFF: wifi_begin(W_DIS_ON, "AT+TCPDIS=on", 0, NULL); return;
  case W_DIS_ON: wifi_pause(ch.reopen_pause); break;
  case W_CAMERA_OFF:
    if (ok == 1 && wf.paused == WF_CAMERA_WAIT) {
      wf.paused = WF_CAMERA_HELD;
      if (wf.link == 2) wf.link = 1;
    }
    wf.sent_at = ch.now;
    break;
  case W_CAMERA_ON:
    wf.paused = 0;
    if (ok != 1) wf.f.ready = 0;
    if (ch.wifi) wifi_pause(0);
    break;
  default: break;
  }
  wifi_rebuffer();
}
/* Начать следующую операцию по состоянию канала; служебный запрос модему идёт в свободное время. */
static void wifi_next(void)
{
  if (wf.paused) {
    if (uart_isReceiving()) { wf.sent_at = ch.now; return; }
    if (ch.now - wf.sent_at < WF_GAP_MS) return;
    if (wf.paused == WF_CAMERA_WAIT) wifi_begin(W_CAMERA_OFF, "AT+TCPDIS=off", 0, NULL);
    else if (wf.paused == WF_CAMERA_RELEASE) wifi_begin(W_CAMERA_ON, "AT+TCPDIS=on", 0, NULL);
    return;
  }
  if (wf.setup == 1) {
    if (ch.state == CH_OPEN) channel_emit(COMM_CH_CLOSED, 0);
    if (ch.wifi) wifi_pause(WF_RESTART_MS);
    wf.setup = 2; wf.prefix = 1;
    wifi_begin(W_ECHO, "AT+E=off", 0, NULL); return;
  }
  if (!ch.wifi) return;
  if (ch.state == CH_INITIAL) { wifi_pause(0); if (usb_log_on(2)) usb_log_value(17, 10, 0); }
  if (ch.state == CH_PAUSE) {
    if (ch.now - ch.operation_since < ch.reopen_pause) return;
    ch.state = CH_OPENING; ch.flags.peer_closed = 0; channel_emit(COMM_CH_OPENING, 0);
    ++ch.connections; wf.opening = ch.now; wf.prefix = 1; wf.misses = 0;
    if (!wf.f.series) { wf.f.series = 1; wf.f.answered = 0; wf.series_at = ch.now; }
    if (!wf.f.ready) { wifi_begin(W_ECHO, "AT+E=off", 0, NULL); return; } /* настройка моста и перезапуск */
    wf.linked = ch.now - 1000u;
  }
  if (ch.state == CH_CLOSE) { wifi_begin(W_DIS_OFF, "AT+TCPDIS=off", 0, NULL); return; }
  if (ch.state == CH_OPENING) {
    if (ch.now - wf.opening >= WF_OPEN_MS || ch.now - wf.series_at >= WF_GIVEUP_MS) { wifi_fail(); return; }
    if (ch.now - wf.linked >= 1000u) wifi_begin(W_LINK, "AT+TCPLK", 0, NULL);
    return;
  }
  if (ch.state != CH_OPEN && ch.state != CH_PROBE) return;
  if (ch.state == CH_OPEN && (ch.tx_state == TX_QUEUED || ch.tx_state == TX_RETRY)) { if (!uart_isReceiving()) wifi_send(); return; }
  /* Кадр строится, данные не розданы или только что ушёл кадр: команды мосту ждут. */
  if (ch.tx_state != TX_FREE || uart_received() || ch.now - wf.sent_at < WF_GAP_MS) return;
  if (ch.probe && !ch.probe_active) { ch.probe_active = ch.probe; wifi_begin(W_PING, "AT+PING=ya.ru", 0, NULL); return; }
  if (ch.state != CH_OPEN) return;
  if (ch.now - wf.linked >= WF_LINK_MS) { wifi_begin(W_LINK, "AT+TCPLK", 0, NULL); return; }
  if (!ch.operation && query.cmd && comm_modem_ready() && !comm_modem_busy())
    channel_command(query.cmd, COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_QUERY);
}
/* Продвинуть мост: завершить текущую операцию, раздать данные линии и начать следующую;
   модем обслуживает только служебные запросы. */
static void wifi_poll(void)
{
  /* Отсчёт от конца кадра: после кадра 534 Б (46 мс) мост склеивал USRAT+TCPLK с данными для сервера (плата №1 21.10). */
  if (wf.f.sent) { wf.f.sent = 0; wf.sent_at = ch.now; }
  if (ch.operation == OP_QUERY) channel_completed();
  if (wf.paused == WF_CAMERA_HELD) {
    if (uart_isReceiving()) wf.sent_at = ch.now;
    return;
  }
  if (wf.op) {
    uint8_t r = wifi_result();
    if (!r) return;
    uint8_t op = wf.op;
    if (r == 1 && op != W_RESTART) wf.f.answered = 1; /* мост жив: серия длиннее */
    wifi_log(op, r);
    if (op != W_RESTART) wifi_strip(); /* слот ещё заперт операцией: приёмник не перехватит frame */
    wf.op = W_NONE;
    wifi_done(op, r);
    if (wf.op) return;
  }
  wifi_rx();
  wifi_next();
}
