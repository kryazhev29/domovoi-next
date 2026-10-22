/* Доработка: Кряжев А., 2026.
   DR164: прозрачный транспорт, RS-485 принадлежит только мосту либо камере.
   Проверено на плате №1 21.10.2026 (1792552760): прежний прозрачный обмен.
   Изменено 2026-10-21 (task/wifi-one): локальный выбор, выключенное ожидание,
   фоновая проверка без разрыва GSM и границы TCP; на плате не проверено.
   Проверено на плате №1 22.10.2026 (1792631158): настройка моста, WSLK/PING, закрытие GSM, регистрация на gate
   через мост (со второй попытки), 5 мин 60 TCPLK=on без переоткрытий.
   Изменено 2026-10-22: событие моста до регистрации не закрывает сеанс — на плате не проверено.
   Изменено 2026-10-22 (разбор отчёта Аргона): байты, пришедшие во время разбора, переносятся, а не стираются;
   при GSM приём моста не уходит серверу; GSM без сеанса (отказ, проверка, неготовый модем) не держит
   возврат на мост до перезапуска модема — на плате не проверено.
   Изменено 2026-10-22: имя сети и уровень (WSLK/WSLQ) для USB 113 — только пока NextConfig читает состояние,
   не чаще раза в 10 с — на плате не проверено.
   Изменено 2026-10-22: список сетей (WSCAN) для USB 115 — ответ моста и сжатый список живут в слоте frame,
   слот на это время закрыт для кадров — на плате не проверено. */
#ifndef __UART_H
void uart_rebuffer(uint8_t *, uint16_t, uint16_t);
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
       WF_RESTART_MS=8000, WF_GAP_MS=50, WF_RETRY_MS=600000, WF_LOG=40, WF_INFO_MS=10000,
       WF_SCAN_MS=6000, WF_SCAN_HOLD_MS=5000 };
enum { W_NONE, W_OFF, W_OFFB, W_ECHO, W_SET_MODE, W_SET_SSID, W_SET_KEY, W_EVENT,
       W_TCPTO, W_NETP, W_TMODE, W_RESTART, W_LINK, W_WSLK, W_PING, W_ON, W_WSLQ, W_WSCAN };
static struct {
  uint32_t since, fell_at, sent_at, info_at;
  uint8_t *buffer; // фактический буфер UART, независимо от смены владельца кадра
  union { uint16_t head, at; } pos; /* приём данных и AT-ответ взаимоисключены */
  uint16_t left;
  uint8_t op, link, prefix, setup, step, misses, mode, rx;
  uint8_t ap, level, ssid_len; /* для USB 113: 0 нет данных, 1 нет точки доступа, 2 подключён; WSLQ, % (255 нет) */
  /* scan — список сетей для USB 115: 0 нет, 1 запрошен, 2 идёт (ответ моста в frame), 3 список в frame, 4 мост не ответил. */
  struct { uint8_t ready:1, sent:1, off:1, retry:1, resume:1, drop:1, dirty:1, stop:1, info, asking, scan; } f; /* восемь флагов одним байтом: 4 Б вместо 11 */
} wf;
static uint8_t wf_line[96];
static uint8_t wf_ssid[32];
/* mode: 0 нет моста, 1 только GSM, 2 Wi-Fi с резервом GSM, 3 только Wi-Fi. */
WF_INLINE static uint8_t wifi_mode(void) { return (uint8_t)comm_modem_settings()->number(SLM_CFG_WIFI); }
static uint32_t wifi_delay(void) { return (wf.step < 2 ? 1u : wf.step == 2 ? 3u : 6u) * (uint32_t)WF_RETRY_MS; }
/* used — разобранная часть приёма; пришедшее после неё во время разбора переносится в новый буфер. */
static void wifi_keep(uint16_t used)
{
  /* Поиск сетей принимает в frame и при GSM; готовый список (3/4) в frame приёмом не трогается. */
  wf.buffer=(ch.wifi || wf.f.scan == 2) && wf.f.scan < 3 && !wf.setup && ch.tx_state == TX_FREE ? frame : wf_line;
  uart_rebuffer(wf.buffer,wf.buffer == frame ? sizeof(frame) : sizeof(wf_line),used); wf.pos.head=0;
}
static void wifi_rebuffer(void) { wifi_keep(UINT16_MAX); }
/* Фоновый AT использует wf_line и не запирает слот GSM. */
static bool wifi_locked(void)
{ return wf.setup || wf.f.scan >= 2 || (ch.wifi && (wf.op || (ch.tx_state == TX_FREE && uart_received()))); }
WF_INLINE static void wifi_reset(void)
{
  wf.op=W_NONE; wf.prefix=1; wf.rx=0; wf.f.scan=0; wf.mode=wifi_mode();
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
  wf.f.scan=0; /* выбранная сеть важнее списка: frame отдаётся настройке */
  wf.f.ready=0; wf.setup=1; if (ch.wifi) wifi_rebuffer();
  memcpy(frame,data,count); return 0;
}
/* Чтение взводит запрос: мост спрашивается, только пока NextConfig опрашивает плату, не чаще раза в WF_INFO_MS. */
uint8_t comm_channel_wifi_info(uint8_t *out)
{
  wf.f.info=1; out[0]=wf.ap; out[1]=wf.level;
  memcpy(out+2,wf_ssid,wf.ssid_len);
  return (uint8_t)(2u+wf.ssid_len);
}
WF_INLINE static bool wifi_info_due(void) { return wf.f.info && wf.mode && ch.now-wf.info_at >= WF_INFO_MS; }
/* Список прочитан, устарел или отброшен: слот снова кадрам; пришедшее тем временем раздаёт wifi_rx. */
static void wifi_scan_free(void) { wf.f.scan=0; if (!uart_received()) wifi_rebuffer(); }
/* WSCAN: строки «канал,SSID,BSSID,защита,уровень» (сильные первыми, концы строк \r и \n вперемешку) сжимаются
   на месте в frame: [уровень %][длина][SSID]…, конец 255. Сжатая строка короче исходной, поэтому запись не
   обгоняет чтение. Скрытые сети, имена длиннее 32 Б и повторы имён (2,4 и 5 ГГц) пропускаются. */
static uint8_t wifi_scan_pack(const uint8_t *p, uint16_t n)
{
  uint16_t k=0, s=0; uint8_t rows=0;
  while (s < n) {
    uint16_t e=s, a=s, c[3], q=0, pos; uint32_t level;
    while (e < n && p[e] != '\r' && p[e] != '\n') ++e;
    if (e >= n) break; /* незаконченная строка (полный буфер) не разбирается */
    for (pos=e; pos > s && q < 3u; --pos) if (p[pos-1u] == ',') c[q++]=pos; /* уровень, защита, BSSID — с конца */
    while (a < e && p[a] != ',') ++a;                                        /* канал до первой запятой */
    pos=c[0];
    if (q == 3u && a+2u < c[2] && p[s] >= '0' && p[s] <= '9' && comm_at_number(p,e,&pos,100u,&level) && pos == e) {
      uint16_t len=(uint16_t)(c[2]-2u-a), d=0; ++a;                           /* SSID между первой запятой и BSSID */
      while (d < k && (frame[d+1u] != len || memcmp(frame+d+2u,p+a,len))) d=(uint16_t)(d+2u+frame[d+1u]);
      if (len <= (uint16_t)sizeof(wf_ssid) && d >= k) {
        frame[k]=(uint8_t)level; frame[k+1u]=(uint8_t)len; memmove(frame+k+2u,p+a,len);
        k=(uint16_t)(k+2u+len); ++rows;
      }
    }
    s=(uint16_t)(e+1u);
  }
  frame[k]=255u; return rows;
}
/* USB 115: row — первая строка страницы, 255 — забыть список и искать заново. Ответ [0 страница / 1 поиск идёт /
   2 Wi-Fi-модем не выбран / 3 мост не ответил][следующая строка, 255 — конец][строки: уровень, длина, SSID].
   Список держится в слоте, пока NextConfig читает страницы, последняя страница или 5 с без чтений отпускают слот. */
uint16_t comm_channel_wifi_scan(uint8_t row, uint8_t *out, uint16_t capacity)
{
  if (!wf.mode) { out[0]=2; return 1; }
  if (row == 255u && wf.f.scan >= 3) wifi_scan_free();
  if (wf.f.scan == 4) { wifi_scan_free(); out[0]=3; return 1; }
  if (wf.f.scan != 3) { if (!wf.f.scan) wf.f.scan=1; out[0]=1; return 1; }
  const uint8_t *p=frame; uint8_t i=0; uint16_t n=2;
  while (i < row && *p != 255u) { p+=2u+p[1]; ++i; }
  while (*p != 255u && n+2u+p[1] <= capacity) { memcpy(out+n,p,2u+p[1]); n=(uint16_t)(n+2u+p[1]); p+=2u+p[1]; ++i; }
  out[0]=0; out[1]=*p == 255u ? 255u : i;
  wf.since=ch.now;
  if (*p == 255u) wifi_scan_free();
  return n;
}
/* Начало значения «+ok=…» и конец его строки; -1 — ответа нет, -2 — «Disconnected». */
static int16_t wifi_value(const uint8_t *p, uint16_t n, uint8_t ok, uint16_t *end)
{
  int16_t at=ok == 1 ? comm_at_find(p,n,"+ok=") : -1;
  if (at < 0) return -1;
  at=(int16_t)(at+4); *end=(uint16_t)at;
  while (*end < n && p[*end] != '\r' && p[*end] != '\n') ++*end;
  return comm_at_prefix(p+at,(uint16_t)(*end-at),"Disconnected") ? -2 : at;
}
/* WSLK: «+ok=SSID(MAC)» — имя до последней скобки; «Disconnected» — точки доступа нет. */
static void wifi_ap(const uint8_t *p, uint16_t n, uint8_t ok)
{
  uint16_t end=0, cut;
  int16_t at=wifi_value(p,n,ok,&end);
  wf.level=255; wf.ssid_len=0; wf.ap=at == -2 ? 1u : 0u;
  if (at < 0) return;
  for (cut=end; cut > (uint16_t)at && p[cut-1u] != '('; --cut) {}
  uint16_t len=(uint16_t)((cut > (uint16_t)at ? cut-1u : end)-(uint16_t)at);
  if (len > (uint16_t)sizeof(wf_ssid)) len=(uint16_t)sizeof(wf_ssid);
  wf.ssid_len=(uint8_t)len; memcpy(wf_ssid,p+at,len); wf.ap=2;
}
/* WSLQ: «+ok=Good,85%» — проценты после запятой. */
static void wifi_quality(const uint8_t *p, uint16_t n, uint8_t ok)
{
  uint16_t end=0; uint32_t v;
  int16_t at=wifi_value(p,n,ok,&end);
  if (at == -2) { wf.ap=1; wf.ssid_len=0; }
  if (at < 0) return;
  uint16_t pos=(uint16_t)at;
  while (pos < end && p[pos] != ',') ++pos;
  if (++pos < end && comm_at_scan(p,end,&pos,100u,&v)) wf.level=(uint8_t)v;
}
static void wifi_asked(void) { wf.f.asking=0; wf.f.info=0; wf.info_at=ch.now; }

static void wifi_begin(uint8_t op)
{
  static const char * const command[]={"TCPDIS=off", "TCPDISB=off,1", "E=off", "WMODE=STA",
    "WSSSID=", "WSKEY=", "EVENT=on", "TCPTO=0", "NETP=TCP,CLIENT,", "TMODE=throughput",
    "Z", "TCPLK", "WSLK", "PING=ya.ru", "TCPDIS=on,0", "WSLQ", "WSCAN"};
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
  wf.f.asking = op == W_WSLQ; /* прерванный запрос не превращает WSLK проверки в запрос: PING не теряется */
}
/* Запрос NextConfig: WSLK, при точке доступа — WSLQ. */
static void wifi_ask(void) { wifi_begin(W_WSLK); wf.f.asking=1; }

static void wifi_log(uint8_t op)
{
  if (op == W_RESTART) return;
  uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
  (void)n; (void)p; /* стенд собирает журнал пустыми макросами */
  if (n) usb_log(2, p, n > WF_LOG ? WF_LOG : n);
  else usb_log_value(4, COMM_AT_TIMEOUT, 0);
}


WF_INLINE static uint8_t wifi_result(void)
{
  if (wf.op == W_WSCAN) { /* список целиком: конец — строка «+ok» (заголовок — «+ok=»), отказ — +ERR;
                             полный буфер или 6 с — разбирается то, что пришло (сильные сети первыми) */
    uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
    if (n >= sizeof(frame) || ch.now - wf.since >= WF_SCAN_MS) return 2;
    if (!uart_getstate()) return 0;
    return comm_at_find(p, n, "+ok\r") >= 0 ? 1 : comm_at_find(p, n, "+ERR") >= 0 ? 2 : 0;
  }
  bool late = ch.now - wf.since >= (wf.op == W_PING ? (uint32_t)WF_PING_MS : (uint32_t)WF_CMD_MS);
  if (wf.op == W_RESTART) return ch.now - wf.since >= WF_RESTART_MS; /* ответа не ждём */
  /* Линия полудуплексная: разбирать только после тишины 10 мс, иначе следующая посылка
     столкнётся с хвостом ответа моста (проверено на плате в командном режиме). */
  if (!uart_getstate() && !late) return 0;
  uint16_t n = uart_received(); const uint8_t *p = wf.buffer;
  int16_t at = comm_at_find(p, n, "+ok");
  if (at < 0) at = comm_at_find(p, n, "+ERR");
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

/* При GSM канал открыт модемом: строки моста (EVENT) серверу не отдаются, их стирает следующая команда мосту. */
WF_INLINE static void wifi_rx(void)
{
  if (!ch.wifi || ch.state != CH_OPEN || wf.op || wf.setup || wf.f.scan >= 2 || ch.tx_state != TX_FREE || !uart_getstate()) return;
  uint16_t n = uart_received();
  if (n > wf.pos.head) wf.pos.head = (uint16_t)(wf.pos.head + wifi_deliver(wf.buffer + wf.pos.head, (uint16_t)(n - wf.pos.head)));
  if (wf.pos.head >= n) wifi_keep(n);
}

/* wifi_locked выдаёт слот только при пустом приёме: пришедшее после проверки уходит в wf_line. */
WF_INLINE static void wifi_hold(void) { wifi_keep(0); }

static void wifi_unhold(void)
{
  wifi_keep(wifi_deliver(wf_line, uart_received()));
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
    /* GSM без сеанса ждёт перезапуска модема до 10/30/60 мин (отказ, неготовый модем) — мост его не ждёт.
       Сокет GSM, открытый или после отказа QIOPEN, сначала закрывается штатно, если модем отвечает. */
    if (ch.state == CH_PAUSE || ch.state == CH_INITIAL || ch.state == CH_FAULT
        || (ch.state != CH_OPEN && !comm_modem_ready())) {
      if (ch.state != CH_INITIAL) wifi_pause(0);
      ch.wifi=1; wf.f.resume=0; wf.rx=0; ch.reopen_pause=0; ch.connections=0;
      ch.operation=OP_NONE; ch.probe=ch.probe_active=0; /* запрос и проверка GSM мосту не принадлежат */
      wifi_rebuffer(); return true;
    }
    if (!ch.operation && !ch.call.active && (ch.state == CH_PROBE
        || (ch.state == CH_OPEN && ch.tx_state == TX_FREE && ch.sent == ch.acked)))
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
    bool on=ok == 1 && comm_at_find(p,n,"+ok=on") >= 0;
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
    wifi_ap(p,n,ok);
    if (wf.f.asking) { /* запрос NextConfig: связь моста не трогает */
      if (wf.ap == 2) { wifi_begin(W_WSLQ); return; }
      wifi_asked(); break;
    }
    wf.link=wf.ap == 2;
    if (wf.link) { wifi_begin(W_PING); return; }
    break;
  case W_WSLQ:
    wifi_quality(p,n,ok); wifi_asked();
    break;
  case W_WSCAN: /* неполный ответ со строками — тоже список; без строк и без +ok — мост не ответил */
    wf.f.scan=wifi_scan_pack(p,n) || ok == 1 ? 3u : 4u; wf.since=ch.now;
    break;
  case W_PING: {
    uint8_t good=ok == 1 && comm_at_find(p,n,"Success") >= 0;
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
  if (wf.f.scan) { /* список сетей: поиск при свободном слоте и пустом приёме, готовый список ждёт чтений в слоте */
    if (wf.f.scan == 1) { if (ch.tx_state == TX_FREE && !uart_received()) { wf.f.scan=2; wf.rx=0; wifi_begin(W_WSCAN); } }
    else if (wf.f.scan >= 3 && ch.now-wf.since >= WF_SCAN_HOLD_MS) wifi_scan_free();
    return;
  }
  /* Сервер через GSM: линия свободна; молчащий мост (не подтвердил off) не спрашивается. */
  if (!ch.wifi) { if (wf.f.off && wifi_info_due()) wifi_ask(); return; }
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
  if (wifi_info_due()) { wifi_ask(); return; }
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
    if (op != W_RESTART && op != W_WSCAN) wifi_strip(); /* список сетей — не данные сервера */
    wf.op=W_NONE;
    wifi_done(op,r);
    if (wf.op) return;
  }
  wifi_rx(); wifi_next();
}
