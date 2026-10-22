/* Доработка: Кряжев А., 2026. TCP/QIRD перенесены из модели модема в канал.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static unsigned channel_opened, channel_closed, channel_faults;
// Запомнить открытие, потерю и отказ канала.
static void channel_model_event(uint8_t kind, uint8_t value)
{
  (void)value;
  if (kind == COMM_CH_OPENED) ++channel_opened;
  if (kind == COMM_CH_CLOSED) ++channel_closed;
  if (kind == COMM_CH_FAULT) ++channel_faults;
}
// Начать независимый сценарий канала.
static void channel_model_reset(void)
{
  sim_reset();
  channel_opened = channel_closed = channel_faults = 0;
  /* AT сначала инициализирует модем; канал назначается в том же сеансе. */
  sim_start(); comm_channel_init(channel_model_event);
  comm_channel_receive(sim_binary, sim_read_done);
  sim_poll = comm_channel_poll;
}
// Продвинуть модель до требуемого состояния канала.
static void channel_until(uint32_t limit)
{
  unsigned watchdog = 0;
  while ((comm_modem_state() == SLM_STARTING || ch.state == CH_INITIAL
      || ch.state == CH_PAUSE || ch.state == CH_OPENING) && watchdog++ < limit / 10u) sim_tick();
  CHECK(watchdog < limit / 10u);
}
// Проверить штатное открытие, передачу и закрытие TCP.
static void channel_path_tests(void)
{
  channel_model_reset();  channel_until(15000);
  CHECK(comm_channel_ready() && slm.attempt.status.reason == SLM_ERR_NONE && !sim_gmm);
  CHECK(sim_now >= 5000 && sim_now <= 6500 && sim_ticks > 500);
  DIAG_CHECK(slm.attempt.status.completed_phases == 63); DIAG_CHECK(!slm.diag.failure_count);
  DIAG_CHECK((slm.diag.steps[SLM_ACTIVATE] * 10u) == 700);
  DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == 500);
  CHECK(power_events == 5 && power_trace[0].pin == 13 && !power_trace[0].high);
  CHECK(power_trace[1].pin == 14 && power_trace[1].high);
  CHECK(power_trace[2].pin == 13 && power_trace[2].time == 2010 && power_trace[2].high);
  CHECK(power_trace[3].pin == 14 && power_trace[3].time == 2040 && !power_trace[3].high);
  CHECK(power_trace[4].time == 4040 && power_trace[4].high);
  printf("SLM normal model: %u ms incl discharge, %u ms from PC13 on, %u polls\n",sim_now,sim_now-power_trace[2].time,sim_ticks);
  CHECK(!strcmp(slm_apn(), "internet.mts.ru"));
  CHECK(sim_commands < 25); /* Нет команд регистрации на сервере. */
  channel_model_reset(); test_config.modem_type = 255;  channel_until(15000);
  CHECK(comm_channel_ready() && sim_gmm == 1 && sim_found == 1);
  channel_model_reset(); sim_port_change = sim_rat_change = sim_early_open = 1;
  memcpy(test_config.apn, "manual.apn", sizeof("manual.apn"));  channel_until(15000);
  CHECK(comm_channel_ready() && !strcmp(slm_apn(),"manual.apn"));
  DIAG_CHECK(slm.attempt.status.completed_steps & ((uint32_t)1u << SLM_URC_SET));
  DIAG_CHECK(slm.attempt.status.completed_steps & ((uint32_t)1u << SLM_RAT_SET));
  /* Выбор APN по IMSI для всех операторов и неизвестного профиля. */
  static const struct { const char *imsi, *apn; } profiles[] = {
    {"250010123456789\r\nOK\r\n", "internet.mts.ru"}, {"250020123456789\r\nOK\r\n", "internet"},
    {"250990123456789\r\nOK\r\n", "internet.beeline.ru"}, {"250200123456789\r\nOK\r\n", "internet.tele2.ru"},
    {"250110123456789\r\nOK\r\n", "internet.yota"}, {"310010123456789\r\nOK\r\n", ""}
  };
  for (uint8_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
    channel_model_reset(); sim_imsi = profiles[i].imsi;  channel_until(15000);
    CHECK(comm_channel_ready() && !strcmp(slm_apn(), profiles[i].apn));
  }
  channel_model_reset(); sim_trickle = 1;  channel_until(20000); CHECK(comm_channel_ready());
}
// Проверить чтение QIRD, включая двоичные данные.
static void channel_read_tests(void)
{
  channel_model_reset(); sim_empty_urc = 1;
  for (unsigned guard = 0; slm.attempt.status.step != SLM_TCP; ++guard) {
    CHECK(guard < 1500u); sim_tick();
  }
  sim_feed("+QIURC: \"recv\",0\r\n"); channel_until(15000);
  CHECK(comm_channel_ready() && sim_qird == 0); /* Данные до TCP/вместимости не теряются. */
#if DOMOVOI_DIAG
#if DOMOVOI_DIAG
  uint16_t tcp_time = slm.diag.steps[SLM_TCP];
#endif
#endif
  sim_read_size = 64; comm_channel_rx_capacity(sim_read_size);
  for (unsigned i = 0; i < 100; ++i) sim_tick();
  CHECK(sim_qird == 4 && sim_done_reads == 4 && sim_body_count == 7 && !sim_read_reason);
  DIAG_CHECK(slm.diag.steps[SLM_TCP] == tcp_time);
  CHECK(!memcmp(sim_body, "OK\r\nXYZ", 7) && !comm_at_busy());
  sim_feed("+QIURC: \"recv\",1\r\n");
  for (unsigned i = 0; i < 10; ++i) sim_tick(); CHECK(sim_qird == 4);
  comm_channel_rx_capacity(0); sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 10; ++i) sim_tick(); CHECK(sim_qird == 4);
  channel_model_reset();  channel_until(15000); sim_read_size = 64;
  comm_channel_rx_capacity(64); sim_silence_step = SLM_READ; sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 110; ++i) sim_tick();
  CHECK(slm.attempt.status.reason == SLM_ERR_TIMEOUT && slm.attempt.status.step == SLM_READ && sim_read_reason == SLM_ERR_TIMEOUT);
}
// Проверить отказы и поздние ответы операций канала.
static void channel_error_tests(void)
{
  channel_model_reset(); sim_open_delay = 149000;  channel_until(170000);
  CHECK(comm_channel_ready()); DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == 149000);
  channel_model_reset(); sim_open_delay = 151000;  channel_until(170000);
  CHECK(slm.attempt.status.reason == SLM_ERR_TIMEOUT && slm.attempt.status.step == SLM_TCP);
  for (unsigned i = 0; i < 200; ++i) sim_tick();
  CHECK(!comm_channel_ready());
  channel_model_reset(); sim_open_error = 1;  channel_until(15000); CHECK(slm.attempt.status.reason == SLM_ERR_TCP && slm.attempt.status.modem_code == 566);
}
// Проверить границы очереди и запрет передачи при вызове.
static void channel_edge_tests(void)
{
  channel_model_reset();  channel_until(15000);
  sim_feed("+QIURC: \"closed\",1\r\n"); sim_tick(); CHECK(comm_channel_ready());
  /* USSD оператора без запроса длиннее строки AT (реклама): хвост отбрасывается, поток не рвётся. */
  { char urc[320]; memcpy(urc, "+CUSD: 0,\"", 10); memset(urc + 10, 'A', 300); memcpy(urc + 310, "\",15\r\n", 7);
    sim_feed(urc); for (unsigned i = 0; i < 5; ++i) sim_tick(); CHECK(comm_channel_ready()); }
  sim_feed("+QIURC: \"closed\",0\r\n"); sim_tick(); CHECK(slm.attempt.status.reason == SLM_ERR_TCP);
  channel_model_reset();  channel_until(15000); sim_read_size = 64; comm_channel_rx_capacity(64);
  sim_pause = 1; sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 15; ++i) sim_tick();
  CHECK(!sim_done_reads && !sim_body_count && comm_at_busy());
  sim_pause = 0;
  for (unsigned i = 0; i < 30; ++i) sim_tick();
  CHECK(sim_qird == 3 && sim_body_count == 7 && sim_done_reads == 3);
  channel_model_reset();  channel_until(15000); sim_read_size = 64; comm_channel_rx_capacity(64);
  sim_feed("+QIURC: \"recv\",0\r\n"); sim_tick();
  sim_pending[0] = 0; sim_feed("+QIRD: 4\r\nxy"); sim_tick();
  CHECK(sim_body_count == 2 && !sim_done_reads);
  interrupt_byte('z', USART_SR_FE); sim_tick();
  CHECK(slm.attempt.status.reason == SLM_ERR_STREAM && sim_done_reads == 1 && sim_read_reason == SLM_ERR_STREAM);
  CHECK(!comm_channel_ready());
}
// Проверить снимок диагностики канала.
static void channel_diagnostic_tests(void)
{
  channel_model_reset();
  for (unsigned guard = 0; slm.attempt.status.step != SLM_TCP; ++guard) {
    CHECK(guard < 1500u); sim_tick();
  }
  for (unsigned i = 0; i < 25; ++i) sim_tick();
  DIAG_CHECK(slm.diag.steps[SLM_TCP] == 25 && slm.diag.phases[SLM_PHASE_TCP] == 250);
  channel_until(15000);
  CHECK(channel_opened == 1); DIAG_CHECK(slm.diag.steps[SLM_TCP] == 50);
  channel_model_reset();  channel_until(15000);
#if DOMOVOI_DIAG
  uint32_t tcp_ms = (slm.diag.steps[SLM_TCP] * 10u);
#endif
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == tcp_ms);
}
/* Стенд моста DR164 на USART1 в прозрачном режиме: ответы мгновенные, тишина сразу; кадры платы без префикса USR
   уходят в сокет (w_outbox), данные сервера кладёт в линию w_push - в конец принятого, как ISR. */
static uint8_t *w_rx; static uint16_t w_cap, w_count;
static char w_cmd[96], w_netp[96], w_ssid[40], w_key[72];
static uint8_t w_outbox[1100], w_body[64];
static uint16_t w_outbox_count, w_body_count, w_take;
static unsigned w_frames, w_links, w_restarts, w_dis, w_pings, w_reads, w_failed, w_commands, w_plain, w_stray;
static uint8_t w_cmd_mode, w_next_mode, w_tcp, w_dead, w_ping_ok, w_ap, w_tcpto, w_mix;
static unsigned w_off_errors, w_asked, w_quality, w_scans;
static const char *w_wslk, *w_wslq, *w_scan; /* NULL — ответы по w_ap / список по умолчанию */
static const char *w_late; static uint32_t w_late_at; /* хвост ответа WSCAN приходит после паузы поиска */
/* Ответ WSCAN как у моста (UDP 48899, 22.10.2026): заголовок, строки с концами \n\r и \r\n вперемешку, сильные первыми,
   в конце «+ok». Повтор имени (5 ГГц), скрытая сеть, имя в 33 байта и запятая в имени — для разбора. */
static const char w_scan_default[] =
  "+ok=\n\rCh,SSID,BSSID,Security,Indicator\r\n"
  "13,Home,AA:BB:CC:DD:EE:01,WPA2PSK/AES,100\n\r"
  "36,Home,AA:BB:CC:DD:EE:02,WPA2PSK/AES,90\n\r"
  "1,Cafe, Bar,AA:BB:CC:DD:EE:03,WPA2PSK/TKIPAES,47\r\n"
  "6,,AA:BB:CC:DD:EE:04,WPA2PSK/AES,40\r\n"
  "11,ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456,AA:BB:CC:DD:EE:05,OPEN/NONE,30\r\n"
  "11,Open Net,AA:BB:CC:DD:EE:06,OPEN/NONE,6\n\r"
  "+ok\r\n\r\n";
void user_comm_wifi_stopped(bool ok) { test_config.wifi_stop=!ok; if (!ok) ++w_off_errors; }
static uint8_t w_saved, w_enabled, w_silent_dis, w_receiving;
/* w_wire: кадр занимает линию 10 бит на байт при 115200, как блокирующий uart_send; команда ближе 20 мс
   к концу кадра (Pack Interval моста) уходит серверу как данные и остаётся без ответа (w_glued). */
static uint8_t w_wire; static uint32_t w_wire_end; static unsigned w_glued;
// Положить байты моста в текущий приёмный буфер канала.
static void w_push(const void *s, uint16_t n)
{
  if (!w_rx || w_dead) return;
  if (n > w_cap - w_count) n = (uint16_t)(w_cap - w_count);
  memcpy(w_rx + w_count, s, n); w_count = (uint16_t)(w_count + n);
}
static void w_text(const char *s) { w_push(s, (uint16_t)strlen(s)); }
// Живой мост (22.10.2026): заголовок WSCAN сразу, строки и «+ok» через 0,9 с поиска.
static void w_answer_scan(const char *s)
{
  const char *rows = s; unsigned lines = 0;
  while (lines < 2) if (*rows++ == '\n') ++lines; /* «+ok=» и строка заголовка */
  w_push(s, (uint16_t)(rows - s)); w_late = rows; w_late_at = sim_now + 900u;
}
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize, uint16_t used)
{
  uint16_t left = w_rx && w_count > used ? (uint16_t)(w_count - used) : 0u;
  if (left > bfsize) left = bfsize;
  if (left) memmove(rxbuffer, w_rx + used, left);
  w_rx = rxbuffer; w_cap = bfsize; w_count = left;
}
uint16_t uart_received(void)
{
  if (w_late && (int32_t)(sim_now - w_late_at) >= 0) { const char *s = w_late; w_late = NULL; w_text(s); }
  return w_count;
}
uint8_t uart_getstate(void) { return w_count != 0; }
bool uart_isReceiving(void) { return w_receiving != 0; }
// Разобрать команду мосту или принять кадр платы в сокет.
void uart_send(uint8_t *buf, uint16_t size)
{
  CHECK(size >= 1 && size <= sizeof(w_outbox));
  bool usr = size >= 3 && !memcmp(buf, "USR", 3);
  bool glued = w_wire && usr && !w_cmd_mode && sim_now - w_wire_end < 20u;
  if (!w_cmd_mode && (!usr || glued)) { /* прозрачный режим: байты уходят серверу */
    ++w_frames; w_glued += glued;
    if (w_tcp && w_enabled) { CHECK(w_outbox_count + size <= sizeof(w_outbox)); memcpy(w_outbox + w_outbox_count, buf, size); w_outbox_count = (uint16_t)(w_outbox_count + size); }
    if (w_wire && !glued) { sim_now += size * 10000u / 115200u; w_wire_end = sim_now; }
    return;
  }
  if (w_cmd_mode && usr) { ++w_stray; return; } /* командный режим: префикс мосту неизвестен, ответа нет */
  CHECK(size >= 2 && size < sizeof(w_cmd) && buf[size - 1u] == '\r');
  ++w_commands; if (!usr) ++w_plain;
  memcpy(w_cmd, buf, size - 1u); w_cmd[size - 1u] = 0;
  const char *c = w_cmd + (usr ? 3 : 0);
  if (!strcmp(c, "AT+E=off") || !strcmp(c, "AT+EVENT=on") || !strcmp(c, "AT+WMODE=STA") || !strcmp(c,"AT+TCPDISB=off,1") || !strcmp(c,"AT+SOCKB=NONE")) { w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TCPTO=0")) { w_tcpto = 0; w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TMODE=throughput")) { w_next_mode = 0; w_text("+ok\r\n\r\n"); return; } /* вступит после перезапуска */
  if (!strncmp(c, "AT+TCPDIS=", 10)) {
    ++w_dis;
    if (w_silent_dis) return;
    w_enabled = !strncmp(c + 10, "on", 2);
    if (strstr(c,",1")) w_saved=w_enabled;
    w_text("+ok\r\n\r\n"); return;
  }
  if (!strcmp(c, "AT+Z")) { ++w_restarts; w_cmd_mode = w_next_mode; w_enabled = w_saved; return; }
  if (!strncmp(c, "AT+NETP=", 8)) { strcpy(w_netp, c); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSSSID=", 10)) { strcpy(w_ssid, c + 10); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSKEY=", 9)) { strcpy(w_key, c + 9); w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TCPLK")) {
    ++w_links; if (w_mix) w_text("AB"); /* данные сервера вокруг ответа */
    w_text(w_tcp && w_enabled ? "+ok=on\r\n\r\n" : "+ok=off\r\n\r\n");
    if (w_mix) w_text("CD"); return;
  }
  if (!strcmp(c, "AT+WSLK")) {
    ++w_asked; if (w_mix) w_text("AB");
    w_text(w_wslk ? w_wslk : w_ap ? "+ok=Home(AA:BB)\r\n\r\n" : "+ok=Disconnected\r\n\r\n");
    if (w_mix) w_text("CD"); return;
  }
  if (!strcmp(c, "AT+WSLQ")) { ++w_quality; w_text(w_wslq ? w_wslq : w_ap ? "+ok=Good,85%\r\n\r\n" : "+ok=Disconnected\r\n\r\n"); return; }
  if (!strcmp(c, "AT+PING=ya.ru")) { ++w_pings; w_text(w_ping_ok ? "+ok=Success:32 bytes\r\n\r\n" : "+ok=Timeout\r\n\r\n"); return; }
  if (!strcmp(c, "AT+WSCAN")) { ++w_scans; w_answer_scan(w_scan ? w_scan : w_scan_default); return; }
  CHECK(0); /* неизвестная мосту команда */
}
/* w_during: байты, пришедшие по линии, пока приёмник разбирает предыдущие (прерывание UART во время разбора). */
static const char *w_during;
// Приёмник данных сервера через мост: собирает тело без проверки источника, не больше w_take за раз.
static uint16_t w_binary(const uint8_t *p, uint16_t n)
{
  if (w_take && n > w_take) n = w_take;
  CHECK(w_body_count + n <= sizeof(w_body));
  memcpy(w_body + w_body_count, p, n); w_body_count = (uint16_t)(w_body_count + n);
  if (w_during) { const char *s = w_during; w_during = NULL; w_text(s); }
  return n;
}
static void w_read_done(uint16_t n, uint8_t reason) { (void)n; CHECK(!reason); ++w_reads; }
static void w_event(uint8_t kind, uint8_t value) { if (kind == COMM_CH_OPEN_FAILED) ++w_failed; channel_model_event(kind, value); }
// Начать независимый сценарий канала через мост: 0 - мост в прозрачном режиме, 1 - в командном.
static void wifi_model_reset(uint8_t cmd_mode, uint8_t tcp)
{
  sim_reset(); memset(&wf,0,sizeof(wf)); test_config.wifi = 2;
  w_off_errors=w_asked=w_quality=w_scans=0; w_saved=1; w_wslk=w_wslq=w_scan=w_late=NULL;
  channel_opened = channel_closed = channel_faults = 0;
  w_rx = NULL; w_cap = w_count = w_outbox_count = w_body_count = w_take = 0;
  w_frames = w_links = w_restarts = w_dis = w_pings = w_reads = w_failed = w_commands = w_plain = w_stray = 0;
  w_cmd_mode = w_next_mode = cmd_mode; w_tcp = tcp; w_dead = w_mix = 0; w_ping_ok = w_ap = w_tcpto = 1;
  w_enabled = 1; w_silent_dis = w_receiving = w_wire = 0; w_wire_end = 0; w_glued = 0; w_during = NULL;
  w_netp[0] = w_ssid[0] = w_key[0] = 0;
  sim_start(); comm_channel_init(w_event); wf.f.ready = 0; /* новый стенд - ненастроенный мост */
  comm_channel_receive(w_binary, w_read_done);
  sim_poll = comm_channel_poll;
}
// Проверить сервер через мост: настройку, передачу, приём, ответ среди данных, потерю TCP, съёмку, ping, командный режим, точку доступа и уход на GSM.
static void channel_wifi_tests(void)
{
  uint16_t capacity = 0; uint8_t *slot; char expect[96]; uint8_t big[101];
  /* Мост из коробки (прозрачный режим, TCP есть): настройка с префиксом USR, перезапуск, через 8 с TCPLK - канал открыт. */
  wifi_model_reset(0, 1);
  for (unsigned i = 0; i < 1500 && !comm_channel_ready(); ++i) sim_tick();
  CHECK(comm_channel_ready() && channel_opened == 1 && w_restarts == 1 && !w_tcpto && !w_plain && !w_stray && sim_now <= 12000);
  comm_channel_registered(); /* дальнейший обмен моделирует уже принятый сервером сеанс */
  snprintf(expect, sizeof(expect), "AT+NETP=TCP,CLIENT,%u,%s", test_config.port, test_config.host);
  CHECK(!strcmp(w_netp, expect) && comm_channel_wifi() && comm_channel_wifi_link() == 2);
  /* Кадр уходит в линию как есть, слот свободен сразу. */
  slot = comm_tx_acquire(&capacity);
  CHECK(slot && capacity == 1040); memcpy(slot, "HELLO", 5); CHECK(comm_tx_commit(5));
  for (unsigned i = 0; i < 5; ++i) sim_tick();
  CHECK(w_frames == 1 && w_outbox_count == 5 && !memcmp(w_outbox, "HELLO", 5) && !comm_tx_busy());
  /* Данные сервера приходят с линии сами. */
  w_push("OK\r\nXYZ", 7); sim_tick();
  CHECK(w_body_count == 7 && !memcmp(w_body, "OK\r\nXYZ", 7) && w_reads == 1 && !w_count);
  /* Приёмник берёт по 4 байта: остаток ждёт в слоте, слот производителю не выдаётся. */
  w_body_count = 0; w_take = 4; w_push("0123456789", 10); sim_tick();
  CHECK(w_body_count == 4 && comm_tx_acquire(&capacity) == NULL);
  for (unsigned i = 0; i < 3; ++i) sim_tick();
  w_take = 0; CHECK(w_body_count == 10 && !memcmp(w_body, "0123456789", 10) && !w_count);
  /* Данные, пришедшие пока слот занят кадром, не теряются. */
  w_body_count = 0; slot = comm_tx_acquire(&capacity); CHECK(slot); w_push("QRS", 3);
  memcpy(slot, "HI", 2); CHECK(comm_tx_commit(2)); sim_tick();
  CHECK(w_frames == 2 && w_body_count == 3 && !memcmp(w_body, "QRS", 3));
  /* Начало следующего пакета, пришедшее во время разбора предыдущего, не стирается очисткой буфера:
     при свободном слоте и при раздаче принятого во время передачи кадра. */
  w_body_count = 0; w_during = "NEXT"; w_push("PREV", 4);
  for (unsigned i = 0; i < 3; ++i) sim_tick();
  CHECK(w_body_count == 8 && !memcmp(w_body, "PREVNEXT", 8) && !w_count && !w_during);
  w_body_count = 0; slot = comm_tx_acquire(&capacity); CHECK(slot); w_push("QRS", 3); w_during = "TUV";
  memcpy(slot, "HI", 2); CHECK(comm_tx_commit(2));
  for (unsigned i = 0; i < 3; ++i) sim_tick();
  CHECK(w_frames == 3 && w_body_count == 6 && !memcmp(w_body, "QRSTUV", 6) && !w_count && !w_during);
  /* Ответ моста на TCPLK среди данных сервера: данные до и после отдаются приёмнику. */
  w_body_count = 0; w_mix = 1;
  for (unsigned i = 0; i < 600 && !w_body_count; ++i) sim_tick();
  w_mix = 0; CHECK(w_body_count == 4 && !memcmp(w_body, "ABCD", 4) && comm_channel_ready());
  /* Звонок не трогает канал моста. */
  comm_channel_call(true); CHECK(comm_tx_acquire(&capacity) != NULL); comm_tx_release(); comm_channel_call(false);
  CHECK(comm_channel_ready() && !comm_channel_expected());
  /* Фото кадрами по 534 Б (46 мс на линии) через проход: проверка TCPLK не уходит серверу вместе с данными
     и сеанс не переоткрывается (плата №1 21.10: USRAT+TCPLK в хвосте куска фото, таймаут, новый сеанс). */
  { unsigned opened = channel_opened, closed = channel_closed, links = w_links; w_wire = 1;
    for (unsigned k = 0; k < 400; ++k) {
      w_outbox_count = 0;
      if (k % 2 == 0 && (slot = comm_tx_acquire(&capacity)) != NULL) { memset(slot, 0x55, 534); CHECK(comm_tx_commit(534)); }
      sim_tick();
    }
    for (unsigned i = 0; i < 600 && w_links == links; ++i) sim_tick();
    w_wire = 0; w_outbox_count = 0;
    CHECK(!w_glued && channel_opened == opened && channel_closed == closed && w_links > links && comm_channel_ready()); }
  /* Сервер закрыл: во всей паузе автоподключение модели моста выключено. */
  w_tcp=0;
  for (unsigned i=0;i<600 && !channel_closed;++i) sim_tick();
  for (unsigned i=0;i<10;++i) sim_tick();
  CHECK(channel_closed==1 && !w_enabled && wf.f.off && ch.state==CH_PAUSE);
  w_tcp=1;
  unsigned empty_connections=0;
  while (sim_now-ch.operation_since < ch.reopen_pause-20u) {
    if (w_enabled) ++empty_connections;
    sim_tick();
  }
  CHECK(!empty_connections && !w_enabled);
  for (unsigned i=0;i<300 && !comm_channel_ready();++i) sim_tick();
  CHECK(comm_channel_ready() && w_enabled && w_restarts==1);
  comm_channel_registered();
  /* EVENT внутри бинарного SLDN и дроблёный приём не закрывают TCP. */
  { const uint8_t packet[]="SLDN\x15\0\1\0\0+EVENT=SOCKA_OFF\r\n\0\0";
    w_body_count=0; w_take=2; unsigned closed=channel_closed;
    w_push(packet,sizeof(packet)-1);
    for (unsigned i=0;i<30;++i) sim_tick();
    CHECK(channel_closed==closed && comm_channel_ready());
    w_take=0; w_body_count=0;
    const char *event="+EVENT=SOCKA_OFF\r\n";
    for (unsigned i=0;event[i];++i) { w_push(event+i,1); sim_tick(); }
    CHECK(channel_closed==closed+1 && !w_enabled);
  }
  /* Обрыв SLDN скрывает текст EVENT, но независимый TCPLK обнаруживает потерю сокета. */
  for (unsigned i=0;i<5000 && !comm_channel_ready();++i) sim_tick();
  comm_channel_registered();
  { unsigned closed=channel_closed; uint32_t begin=sim_now;
    const uint8_t partial[]="SLDN\xF8\x07\1\0\0+EVENT=SOCKA_OFF\r\n";
    w_body_count=0; w_push(partial,sizeof(partial)-1); sim_tick();
    CHECK(channel_closed==closed && wf.rx==6);
    w_tcp=0;
    for (unsigned i=0;i<1500 && channel_closed==closed;++i) sim_tick();
    CHECK(channel_closed==closed+1 && sim_now-begin<10000u);
    w_tcp=1;
  }
  /* Смена NETP — повторная настройка, без перезагрузки платы. */
  strcpy(test_config.host,"new.example"); test_config.port=9876;
  comm_channel_wifi_changed();
  for (unsigned i=0;i<2000 && !comm_channel_ready();++i) sim_tick();
  CHECK(strstr(w_netp,"9876,new.example") && w_restarts==2 && comm_channel_ready());
  /* Мост сообщает о своём подключении уже после открытия канала (плата №1 22.10: канал закрыт через 32 мс
     после TCPLK=on, до регистрации): до регистрации событие сеанс не закрывает, после — закрывает. */
  { unsigned closed=channel_closed; const char *event="+EVENT=SOCKA_ON\r\n";
    w_push(event,(uint16_t)strlen(event));
    for (unsigned i=0;i<30;++i) sim_tick();
    CHECK(channel_closed==closed && ch.wifi && comm_channel_ready());
    comm_channel_registered(); w_push(event,(uint16_t)strlen(event));
    for (unsigned i=0;i<30;++i) sim_tick();
    CHECK(channel_closed==closed+1 && ch.wifi);
    for (unsigned i=0;i<5000 && !comm_channel_ready();++i) sim_tick();
    CHECK(comm_channel_ready()); }
  CHECK(comm_channel_probe(COMM_CH_PING));
  for (unsigned i=0;i<10;++i) sim_tick();
  CHECK(w_pings==1 && comm_channel_probe_result()==COMM_CH_PING);
  /* Командный режим: три USR без ответа, обычный AT выключает мост, дальше настройка. */
  wifi_model_reset(1,1);
  for (unsigned i=0;i<2000 && !comm_channel_ready();++i) sim_tick();
  CHECK(comm_channel_ready() && !w_cmd_mode && w_stray==3 && w_plain>=8);
  CHECK(comm_channel_wifi_setup((const uint8_t *)"Home\0pass123\0",13)==0);
  CHECK(comm_channel_wifi_setup((const uint8_t *)"X\0y\0",4)==1);
  for (unsigned i=0;i<2000 && w_restarts<2;++i) sim_tick();
  for (unsigned i=0;i<1200 && !comm_channel_ready();++i) sim_tick();
  CHECK(!strcmp(w_ssid,"Home") && !strcmp(w_key,"pass123") && comm_channel_ready());
  memset(big,'a',sizeof(big)); big[50]=big[100]=0;
  CHECK(comm_channel_wifi_setup((const uint8_t *)"\0a\0",3)==2 && comm_channel_wifi_setup(big,sizeof(big))==2);
  /* Предельные строки и усечённые поля: ни чтения за буфером, ни обрезанного AT. */
  CHECK(comm_channel_wifi_setup((const uint8_t *)"x",1)==2);
  CHECK(comm_channel_wifi_setup((const uint8_t *)"x\0",2)==2);
  memset(big,'a',sizeof(big)); big[32]=big[96]=big[97]=0;
  CHECK(comm_channel_wifi_setup(big,98)==2);
  CHECK(comm_channel_wifi_setup(big,97)==0);
  for (unsigned i=0;i<3000 && (wf.setup || !comm_channel_ready());++i) sim_tick();
  CHECK(strlen(w_ssid)==32 && strlen(w_key)==63 && comm_channel_ready());
  /* Моста нет: GSM работает и при фоновой проверке через 10 минут не закрывается. */
  wifi_model_reset(0,1); w_dead=1;
  for (unsigned i=0;i<3000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && comm_channel_wifi()); /* порт всё ещё выбран мосту */
  /* Молчащий мост на GSM не получает выключение каждые несколько секунд: до проверки по расписанию тишина. */
  unsigned dis=w_dis;
  for (unsigned i=0;i<6000;++i) sim_tick();
  CHECK(w_dis==dis);
  unsigned closed=channel_closed, opened=channel_opened;
  wf.fell_at=sim_now-WF_RETRY_MS;
  for (unsigned i=0;i<1000;++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && channel_closed==closed && channel_opened==opened);
  /* Мост вернулся, но Wi-Fi отсутствует: WSLK не разрешает закрыть GSM. */
  w_dead=0; w_ap=0; wf.fell_at=sim_now-WF_RETRY_MS;
  for (unsigned i=0;i<2000;++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && channel_closed==closed && !w_enabled);
  /* Wi-Fi есть, PING не проходит — GSM по-прежнему открыт. */
  w_ap=1; w_ping_ok=0; wf.fell_at=sim_now-WF_RETRY_MS;
  for (unsigned i=0;i<100;++i) sim_tick();
  CHECK(!ch.wifi && channel_closed==closed && w_pings);
  /* Только WSLK + PING разрешают закрытие старого GSM TCP. */
  w_ping_ok=1; wf.fell_at=sim_now-WF_RETRY_MS;
  for (unsigned i=0;i<2000 && !(ch.wifi && comm_channel_ready());++i) sim_tick();
  CHECK(ch.wifi && comm_channel_ready() && channel_closed==closed+1);
  /* Отказы регистрации подряд: 10, 30, 60 минут. */
  for (unsigned attempt=0;attempt<3;++attempt) {
    comm_channel_reopen(0);
    for (unsigned i=0;i<1000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
    CHECK(!ch.wifi && comm_channel_ready());
    CHECK(wifi_delay()==(attempt==0 ? 600000u : attempt==1 ? 1800000u : 3600000u));
    uint32_t delay=wifi_delay(); wf.fell_at=sim_now-delay+1000;
    for (unsigned i=0;i<90;++i) { sim_tick(); CHECK(!ch.wifi); }
    for (unsigned i=0;i<2000 && !(ch.wifi && comm_channel_ready());++i) sim_tick();
    CHECK(ch.wifi && comm_channel_ready());
  }
  comm_channel_registered(); CHECK(wifi_delay()==600000u && !wf.f.retry);
  /* Выключение без ответа: три USR + один AT, событие, порт свободен. */
  test_config.wifi=0; test_config.wifi_stop=1; comm_channel_wifi_changed(); w_dead=1;
  unsigned commands=w_commands, stray=w_stray;
  for (unsigned i=0;i<500;++i) sim_tick();
  CHECK(!comm_channel_wifi() && w_off_errors==1 && test_config.wifi_stop);
  CHECK(w_commands-commands+w_stray-stray==3); /* четвёртый обычный AT в прозрачном режиме */
  /* Следующий запуск с долгом: съёмка блокирует повторы, после неё выключение завершается. */
  w_dead=0; test_config.camera_busy=1; comm_channel_init(w_event);
  commands=w_commands;
  for (unsigned i=0;i<100;++i) sim_tick();
  CHECK(w_commands==commands);
  test_config.camera_busy=0;
  for (unsigned i=0;i<100;++i) sim_tick();
  CHECK(!test_config.wifi_stop && !w_enabled && !comm_channel_wifi());
  /* Сервер меняет адрес без выбранного моста: никаких AT на линии камеры. */
  commands=w_commands; comm_channel_wifi_changed();
  for (unsigned i=0;i<100;++i) sim_tick();
  CHECK(w_commands==commands);
  /* Только Wi-Fi: отсутствие моста не открывает серверный TCP через GSM. */
  wifi_model_reset(0,1); test_config.wifi=3; comm_channel_wifi_changed(); w_dead=1;
  for (unsigned i=0;i<7000;++i) sim_tick();
  CHECK(ch.wifi && !channel_opened && comm_modem_ready());
  /* Живой GSM -> «Только Wi-Fi»: старый сокет закрывается даже при отсутствующем мосте. */
  wifi_model_reset(0,1); w_dead=1;
  for (unsigned i=0;i<3000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready());
  closed=channel_closed; opened=channel_opened;
  test_config.wifi=3; comm_channel_wifi_changed();
  for (unsigned i=0;i<7000;++i) sim_tick();
  CHECK(ch.wifi && !comm_channel_ready() && channel_closed==closed+1 && channel_opened==opened);
  /* Выбранный USR / Только GSM: TCP моста выключен, порт остаётся за ним. */
  wifi_model_reset(0,1); test_config.wifi=1; comm_channel_wifi_changed();
  for (unsigned i=0;i<3000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && comm_channel_wifi() && !w_enabled && wf.f.off);
  /* Событие моста при работе через GSM не попадает в приёмник сервера (раньше уходил старый кадр слота). */
  { unsigned body=w_body_count; memcpy(frame,"SLDN",4); w_text("+EVENT=CON_OFF\r\n");
    for (unsigned i=0;i<30;++i) sim_tick();
    CHECK(w_body_count==body && comm_channel_ready()); }
  /* GSM в отказе ждёт перезапуска модема (до 10/30/60 мин): ожившая сеть Wi-Fi забирает связь сразу. */
  wifi_model_reset(0,1); w_dead=1;
  for (unsigned i=0;i<3000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready());
  ch.state=CH_FAULT; w_dead=0; wf.fell_at=sim_now-wifi_delay();
  for (unsigned i=0;i<2000 && !(ch.wifi && comm_channel_ready());++i) sim_tick();
  CHECK(ch.wifi && comm_channel_ready() && !comm_channel_fault());
  test_config.wifi=0; w_dead=0;
  puts("comm_wifi: session/off pause/background probe/recovery/10-30-60/NETP/disable debt/only-WiFi PASS");
}
/* Чтение USB 113 с запросом моста, срок прошёл: дождаться WSLK/WSLQ и прочитать снова. */
static uint8_t w_info(uint8_t *out)
{
  wf.info_at=sim_now-WF_INFO_MS; comm_channel_wifi_info(out);
  for (unsigned i=0;i<100;++i) sim_tick();
  return comm_channel_wifi_info(out);
}
/* Имя сети и уровень для USB 113 (22.10.2026): мост спрашивается только по чтению NextConfig, не чаще раза в 10 с,
   при сервере и через мост, и через GSM; ответ среди данных сервера их не портит; прерванный запрос не съедает PING. */
static void channel_wifi_info_tests(void)
{
  uint8_t out[40];
  wifi_model_reset(0,1);
  for (unsigned i=0;i<1500 && !comm_channel_ready();++i) sim_tick();
  comm_channel_registered();
  CHECK(comm_channel_ready() && ch.wifi);
  /* Без чтений USB мост о точке доступа не спрашивается. */
  for (unsigned i=0;i<3000;++i) sim_tick();
  CHECK(!w_asked && !w_quality);
  /* Первое чтение: сведений ещё нет; запрос — WSLK, затем WSLQ. */
  CHECK(comm_channel_wifi_info(out)==2 && out[0]==0);
  for (unsigned i=0;i<100;++i) sim_tick();
  CHECK(w_asked==1 && w_quality==1 && comm_channel_wifi_info(out)==6 && out[0]==2 && out[1]==85 && !memcmp(out+2,"Home",4));
  CHECK(comm_channel_ready() && ch.wifi && wf.link==2); /* связь моста запросом не меняется */
  /* Чтения чаще 10 с отдают прежнее без команд мосту. */
  { uint32_t start=sim_now; unsigned asked=w_asked;
    while (sim_now-start < 9000u) { comm_channel_wifi_info(out); sim_tick(); }
    CHECK(w_asked==asked);
    while (sim_now-start < 11000u) { comm_channel_wifi_info(out); sim_tick(); }
    CHECK(w_asked==asked+1); }
  /* Пробел перед процентами и скобки в имени сети; без MAC; длинное имя — 32 байта; уровень вне 0..100 и без процентов — нет данных. */
  w_wslk="+ok=My (Net)(AA:BB:CC)\r\n\r\n"; w_wslq="+ok=Normal, 55%\r\n\r\n";
  CHECK(w_info(out)==10 && out[0]==2 && out[1]==55 && !memcmp(out+2,"My (Net)",8));
  w_wslk="+ok=Home\r\n\r\n"; w_wslq="+ok=Weak\r\n\r\n";
  CHECK(w_info(out)==6 && out[1]==255 && !memcmp(out+2,"Home",4));
  w_wslk="+ok=ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789(AA)\r\n\r\n"; w_wslq="+ok=Good,150%\r\n\r\n";
  CHECK(w_info(out)==34 && out[1]==255 && !memcmp(out+2,"ABCDEFGHIJKLMNOPQRSTUVWXYZ012345",32));
  /* Нет точки доступа: WSLQ не нужен. */
  { unsigned quality=w_quality; w_wslk="+ok=Disconnected\r\n\r\n";
    CHECK(w_info(out)==2 && out[0]==1 && out[1]==255 && w_quality==quality); }
  /* Ответ на WSLK среди данных сервера: данные отдаются приёмнику целиком. */
  w_wslk=w_wslq=NULL; w_body_count=0; w_mix=1;
  CHECK(w_info(out)==6 && out[0]==2); w_mix=0;
  CHECK(w_body_count==4 && !memcmp(w_body,"ABCD",4) && comm_channel_ready());
  /* Мост молчит: сведений нет, сеанс не трогается. */
  { unsigned closed=channel_closed; w_dead=1;
    wf.info_at=sim_now-WF_INFO_MS; comm_channel_wifi_info(out);
    for (unsigned i=0;i<120;++i) sim_tick();
    w_dead=0; CHECK(comm_channel_wifi_info(out)==2 && out[0]==0 && channel_closed==closed); }
  /* Без выбранного модема запрос не идёт. */
  wf.mode=0; wf.f.info=1; wf.info_at=sim_now-WF_INFO_MS; CHECK(!wifi_info_due()); wf.mode=2;
  /* Сервер через GSM, мост молчит: его не спрашивают (ни команд, ни строк в линию). */
  wifi_model_reset(0,1); w_dead=1;
  for (unsigned i=0;i<3000 && (ch.wifi || !comm_channel_ready());++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready());
  { unsigned commands=w_commands, frames=w_frames; w_dead=0;
    CHECK(w_info(out)==2 && out[0]==0 && w_commands==commands && w_frames==frames); }
  /* Сервер через GSM, мост отвечает, сети Wi-Fi нет: по чтению мост спрашивается, GSM не трогается. */
  wifi_model_reset(0,0); w_ap=0;
  for (unsigned i=0;i<8000 && !(!ch.wifi && comm_channel_ready() && wf.f.off);++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && wf.f.off);
  { unsigned closed=channel_closed, quality=w_quality;
    CHECK(w_info(out)==2 && out[0]==1 && w_quality==quality && !ch.wifi && channel_closed==closed && comm_channel_ready());
    w_ap=1; CHECK(w_info(out)==6 && out[0]==2 && out[1]==85 && !ch.wifi && channel_closed==closed && comm_channel_ready()); }
  /* Прерванный запрос не превращает WSLK проверки в запрос: проверка доходит до PING. */
  { unsigned pings=w_pings; w_ping_ok=0; wf.f.asking=1; wf.fell_at=sim_now-wifi_delay();
    for (unsigned i=0;i<3000 && w_pings==pings;++i) sim_tick();
    CHECK(w_pings==pings+1 && !wf.f.asking && !ch.wifi && comm_channel_ready()); }
  test_config.wifi=0;
  puts("comm_wifi_info: on-demand WSLK/WSLQ, 10 s limit, SSID/level parse, server data around, silent bridge, GSM, PING kept PASS");
}
/* Список сетей для USB 115 (22.10.2026): поиск только по запросу NextConfig, ответ моста и сжатый список в слоте frame,
   слот закрыт для кадров на время поиска и пока список читают; страницы, повтор, устаревание, молчащий мост, GSM, настройка. */
static void channel_wifi_scan_tests(void)
{
  uint8_t out[242]; uint16_t capacity=0, n; unsigned i; uint32_t at;
  static char big[2200];
  wifi_model_reset(0,1);
  for (i=0;i<1500 && !comm_channel_ready();++i) sim_tick();
  comm_channel_registered(); CHECK(comm_channel_ready() && ch.wifi);
  /* Без запроса мост не сканируется. */
  for (i=0;i<2000;++i) sim_tick();
  CHECK(!w_scans);
  /* Запрос взводит поиск: ответ 1, команда одна, слот на время поиска закрыт, состояние 113 не трогается. */
  at=sim_now; CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==1);
  for (i=0;i<5;++i) sim_tick();
  CHECK(w_scans==1 && wf.f.scan==2 && comm_tx_acquire(&capacity)==NULL);
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==1 && comm_channel_wifi_info(out)==2);
  for (i=0;i<50;++i) sim_tick();
  CHECK(wf.f.scan==2 && comm_tx_acquire(&capacity)==NULL); /* заголовок «+ok=» — ещё не конец списка */
  for (i=0;i<50;++i) sim_tick();
  CHECK(w_scans==1 && wf.f.scan==3 && sim_now-at < 2000u);
  /* Список: сильные первыми, без повтора имени, скрытой и длинной сети; запятая в имени сохраняется. */
  n=comm_channel_wifi_scan(0,out,sizeof out);
  CHECK(n==29 && out[0]==0 && out[1]==255 && out[2]==100 && out[3]==4 && !memcmp(out+4,"Home",4)
        && out[8]==47 && out[9]==9 && !memcmp(out+10,"Cafe, Bar",9) && out[19]==6 && out[20]==8 && !memcmp(out+21,"Open Net",8));
  /* Последняя страница отпускает слот; сеанс цел, список не ушёл приёмнику сервера. */
  CHECK(!wf.f.scan && comm_channel_ready() && ch.wifi && !w_body_count && !channel_closed);
  CHECK(comm_tx_acquire(&capacity)!=NULL); comm_tx_release();
  /* Страницы по вместимости ответа: в ответе следующая строка, конец — 255; после конца — новый поиск. */
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==1);
  for (i=0;i<100;++i) sim_tick();
  CHECK(w_scans==2 && comm_channel_wifi_scan(0,out,12)==8 && out[0]==0 && out[1]==1 && out[2]==100);
  CHECK(comm_channel_wifi_scan(1,out,20)==13 && out[0]==0 && out[1]==2 && out[2]==47);
  CHECK(comm_channel_wifi_scan(2,out,sizeof out)==12 && out[0]==0 && out[1]==255 && out[2]==6 && !wf.f.scan);
  /* Список не читают: 5 с слот держится (TCPLK стоит), потом отпускается и опрос TCPLK идёт; байты моста,
     пришедшие пока список держался, приёмнику отдаются только после отпускания. */
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1);
  for (i=0;i<100;++i) sim_tick();
  { unsigned links=w_links; w_body_count=0;
    w_push("ABCD",4); sim_tick(); CHECK(wf.f.scan==3 && !w_body_count);
    for (i=0;i<380;++i) sim_tick();
    CHECK(wf.f.scan==3 && w_links==links && !w_body_count);
    for (i=0;i<120;++i) sim_tick(); /* 5 с от готовности списка */
    CHECK(!wf.f.scan && w_body_count==4 && !memcmp(w_body,"ABCD",4));
    for (i=0;i<600;++i) sim_tick();
    CHECK(w_links>links && comm_channel_ready() && ch.wifi); }
  /* 255 — забыть готовый список и искать заново. */
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1);
  for (i=0;i<100;++i) sim_tick();
  { unsigned scans=w_scans; CHECK(wf.f.scan==3 && comm_channel_wifi_scan(255,out,sizeof out)==1 && out[0]==1);
    for (i=0;i<100;++i) sim_tick();
    CHECK(w_scans==scans+1 && wf.f.scan==3 && comm_channel_wifi_scan(0,out,sizeof out)==29 && !wf.f.scan); }
  /* Длинный ответ (40 сетей, 1,8 КБ) не влезает в слот: разбираются целые строки, сильные первыми; сеанс цел. */
  { unsigned total=(unsigned)snprintf(big,sizeof big,"+ok=\r\nCh,SSID,BSSID,Security,Indicator\r\n"), fit=0, first;
    for (i=0;i<40;++i) { unsigned len=(unsigned)snprintf(big+total,sizeof big-total,"%u,Net%02u,AA:BB:CC:DD:EE:%02u,WPA2PSK/AES,%u\r\n",1u+i%13u,i,i,99u-2u*i);
      if (total+len <= sizeof(frame)) ++fit; total+=len; }
    CHECK(total > sizeof(frame) && fit > 20 && fit < 40); first=fit > 34 ? 34 : fit; /* страница — до 34 строк по 7 Б */
    w_scan=big; w_body_count=0; CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1);
    for (i=0;i<100;++i) sim_tick();
    n=comm_channel_wifi_scan(0,out,sizeof out); CHECK(wf.f.scan==(fit > 34 ? 3 : 0) && n==2+7*first && out[0]==0 && out[1]==(fit > 34 ? 34 : 255)); /* одна страница отпускает слот сразу */
    CHECK(out[2]==99 && out[3]==5 && !memcmp(out+4,"Net00",5) && out[2+7*20]==99-2*20 && !memcmp(out+2+7*20+2,"Net20",5));
    if (fit > 34) { n=comm_channel_wifi_scan(34,out,sizeof out); CHECK(n==2+7*(fit-34) && out[0]==0 && out[1]==255 && out[2]==99-2*34); }
    w_scan=NULL; CHECK(!wf.f.scan && comm_channel_ready() && ch.wifi && !w_body_count); }
  /* Мост молчит: через 10 с ответ 3 и слот свободен; следующий запрос ищет снова. */
  w_dead=1; at=sim_now; CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1);
  for (i=0;i<1100 && wf.f.scan!=4;++i) sim_tick();
  w_dead=0; CHECK(wf.f.scan==4 && sim_now-at>=10000u && sim_now-at<10200u && comm_tx_acquire(&capacity)==NULL);
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==3 && !wf.f.scan);
  sim_tick(); CHECK(comm_tx_acquire(&capacity)!=NULL); comm_tx_release();
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==1);
  for (i=0;i<100;++i) sim_tick();
  CHECK(wf.f.scan==3 && comm_channel_wifi_scan(0,out,sizeof out)==29);
  /* Wi-Fi-модем не выбран: ответ 2, мост не спрашивается. */
  { unsigned scans=w_scans; wf.mode=0; CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==2 && !wf.f.scan); wf.mode=2;
    for (i=0;i<100;++i) sim_tick(); CHECK(w_scans==scans); }
  /* Выбранная сеть важнее списка: настройка принимается, список забыт. */
  CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1);
  for (i=0;i<100;++i) sim_tick();
  CHECK(wf.f.scan==3 && comm_channel_wifi_setup((const uint8_t *)"Home\0secret\0",12)==0 && !wf.f.scan && wf.setup);
  for (i=0;i<1200 && !(comm_channel_ready() && wf.f.ready);++i) sim_tick();
  CHECK(comm_channel_ready() && !strcmp(w_ssid,"Home") && !strcmp(w_key,"secret"));
  /* Сервер через GSM, мост отвечает: поиск идёт через слот, GSM не трогается. */
  wifi_model_reset(0,0); w_ap=0;
  for (i=0;i<8000 && !(!ch.wifi && comm_channel_ready() && wf.f.off);++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && wf.f.off);
  { unsigned closed=channel_closed; CHECK(comm_channel_wifi_scan(0,out,sizeof out)==1 && out[0]==1);
    for (i=0;i<100;++i) sim_tick();
    CHECK(w_scans==1 && comm_channel_wifi_scan(0,out,sizeof out)==29 && out[2]==100 && !wf.f.scan);
    CHECK(!ch.wifi && comm_channel_ready() && channel_closed==closed); }
  test_config.wifi=0;
  puts("comm_wifi_scan: on-demand WSCAN, slot lock, pack/dedupe/limits, pages, hold 5 s, rescan, long list, silent bridge, no modem, setup wins, GSM PASS");
}
/* Опрос сигнала: при открытом TCP - очередью канала (прежний путь), без TCP - служебной командой
   модема: и в паузе после отказа 566, и пока QIOPEN ждёт ответа (плата №1 20.10.2026: 150 с без URC).
   Порядок и период прежние; ожидающие вызов, кадр и приоритетная операция обходят фоновый запрос. */
static void channel_info_tests(void)
{
  for (unsigned mode = 0; mode < 3; ++mode) { /* 0 - TCP открыт, 1 - отказ 566, 2 - QIOPEN без ответа */
    channel_model_reset(); sim_open_error = mode;
    if (mode < 2) channel_until(15000);
    else for (unsigned guard = 0; !(ch.state == CH_OPENING && ch.flags.open_ack); ++guard) { CHECK(guard < 1500); sim_tick(); }
    CHECK(comm_channel_ready() == !mode);
    if (mode == 1) CHECK(slm.attempt.status.modem_code == 566);
    info.step = info.antenna = info.daily = 0; info.next = sim_now;
    info.v.csq = 99; info.v.creg = info.v.cgreg = info.v.net = 0;
    bool channel_path = false;
    for (unsigned i = 0; i < 300 && (info.v.net != 4 || sim_qeng_queries == 0); ++i) {
      comm_info_poll(sim_now); channel_path |= query.cmd != NULL; sim_tick();
    }
    CHECK(channel_path == !mode);
    CHECK(comm_info()->csq == 22 && comm_info()->creg && comm_info()->cgreg && comm_info()->net == 4);
    CHECK(sim_qeng_queries == 1);
    if (mode == 2) CHECK(ch.state == CH_OPENING);
    sim_csq = 18;
    while ((int32_t)(sim_now - info.next) < 0) { comm_info_poll(sim_now); sim_tick(); }
    CHECK(comm_info()->csq == 22 && sim_qeng_queries == 1);
    for (unsigned i = 0; i < 100; ++i) { comm_info_poll(sim_now); sim_tick(); }
    CHECK(comm_info()->csq == 18 && sim_qeng_queries == 2);
  }
  channel_model_reset(); channel_until(15000);
  CHECK(comm_channel_query("AT+CSQ", info_line));
  comm_channel_call(true); unsigned before = sim_commands;
  for (unsigned i = 0; i < 10; ++i) sim_tick();
  CHECK(sim_commands == before && query.cmd);
  comm_channel_call(false);
  comm_channel_priority(true); sim_tick(); CHECK(sim_commands == before);
  comm_channel_priority(false);
  uint16_t capacity; uint8_t *tx = comm_tx_acquire(&capacity);
  CHECK(tx && capacity >= 4); memcpy(tx, "TEST", 4); CHECK(comm_tx_commit(4));
  sim_tick(); CHECK(!strcmp(sim_sent, "AT+QISEND=0,4") && query.cmd);
  for (unsigned i = 0; i < 100; ++i) sim_tick();
  CHECK(!comm_tx_busy() && !query.cmd && comm_info()->csq == 22);
  puts("comm_info no-server: TCP/566/QIOPEN wait/30s/call/frame/priority PASS");
}
// Запустить проверки путей и отказов канала.
static void channel_regressions(void)
{
  channel_path_tests(); channel_error_tests(); channel_read_tests(); channel_edge_tests(); channel_diagnostic_tests(); channel_info_tests(); channel_wifi_tests(); channel_wifi_info_tests(); channel_wifi_scan_tests();
  comm_modem_transport(NULL); sim_poll = comm_modem_poll;
  puts("comm_channel: original TCP/READ/late URC/capacity/backpressure/timing PASS");
}
