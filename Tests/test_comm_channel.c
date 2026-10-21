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
static uint8_t w_enabled, w_silent_dis, w_receiving, w_sockb;
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
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize) { w_rx = rxbuffer; w_cap = bfsize; w_count = 0; }
uint16_t uart_received(void) { return w_count; }
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
    if ((w_tcp && w_enabled) || w_sockb) { CHECK(w_outbox_count + size <= sizeof(w_outbox)); memcpy(w_outbox + w_outbox_count, buf, size); w_outbox_count = (uint16_t)(w_outbox_count + size); }
    if (w_wire && !glued) { sim_now += size * 10000u / 115200u; w_wire_end = sim_now; }
    return;
  }
  if (w_cmd_mode && usr) { ++w_stray; return; } /* командный режим: префикс мосту неизвестен, ответа нет */
  CHECK(size >= 2 && size < sizeof(w_cmd) && buf[size - 1u] == '\r');
  ++w_commands; if (!usr) ++w_plain;
  memcpy(w_cmd, buf, size - 1u); w_cmd[size - 1u] = 0;
  const char *c = w_cmd + (usr ? 3 : 0);
  if (!strcmp(c, "AT+E=off") || !strcmp(c, "AT+EVENT=off") || !strcmp(c, "AT+WMODE=STA")) { w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TCPTO=0")) { w_tcpto = 0; w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TMODE=throughput")) { w_next_mode = 0; w_text("+ok\r\n\r\n"); return; } /* вступит после перезапуска */
  if (!strcmp(c, "AT+SOCKB")) { w_text(w_sockb ? "+ok=TCP,15010,example.org\r\n\r\n" : "+ok=NONE\r\n\r\n"); return; }
  if (!strncmp(c, "AT+TCPDIS=", 10)) {
    ++w_dis;
    if (w_silent_dis) return;
    w_enabled = !strcmp(c + 10, "on");
    w_text("+ok\r\n\r\n"); return;
  }
  if (!strcmp(c, "AT+Z")) { ++w_restarts; w_cmd_mode = w_next_mode; w_enabled = 1; return; }
  if (!strncmp(c, "AT+NETP=", 8)) { strcpy(w_netp, c); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSSSID=", 10)) { strcpy(w_ssid, c + 10); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSKEY=", 9)) { strcpy(w_key, c + 9); w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TCPLK")) {
    ++w_links; if (w_mix) w_text("AB"); /* данные сервера вокруг ответа */
    w_text(w_tcp && w_enabled ? "+ok=on\r\n\r\n" : "+ok=off\r\n\r\n");
    if (w_mix) w_text("CD"); return;
  }
  if (!strcmp(c, "AT+WSLK")) { w_text(w_ap ? "+ok=Home(AA:BB)\r\n\r\n" : "+ok=Disconnected\r\n\r\n"); return; }
  if (!strcmp(c, "AT+PING=ya.ru")) { ++w_pings; w_text(w_ping_ok ? "+ok=Success:32 bytes\r\n\r\n" : "+ok=Timeout\r\n\r\n"); return; }
  CHECK(0); /* неизвестная мосту команда */
}
// Приёмник данных сервера через мост: собирает тело без проверки источника, не больше w_take за раз.
static uint16_t w_binary(const uint8_t *p, uint16_t n)
{
  if (w_take && n > w_take) n = w_take;
  CHECK(w_body_count + n <= sizeof(w_body));
  memcpy(w_body + w_body_count, p, n); w_body_count = (uint16_t)(w_body_count + n); return n;
}
static void w_read_done(uint16_t n, uint8_t reason) { (void)n; CHECK(!reason); ++w_reads; }
static void w_event(uint8_t kind, uint8_t value) { if (kind == COMM_CH_OPEN_FAILED) ++w_failed; channel_model_event(kind, value); }
// Начать независимый сценарий канала через мост: 0 - мост в прозрачном режиме, 1 - в командном.
static void wifi_model_reset(uint8_t cmd_mode, uint8_t tcp)
{
  sim_reset(); test_config.wifi = 1;
  channel_opened = channel_closed = channel_faults = 0;
  w_rx = NULL; w_cap = w_count = w_outbox_count = w_body_count = w_take = 0;
  w_frames = w_links = w_restarts = w_dis = w_pings = w_reads = w_failed = w_commands = w_plain = w_stray = 0;
  w_cmd_mode = w_next_mode = cmd_mode; w_tcp = tcp; w_dead = w_mix = 0; w_ping_ok = w_ap = w_tcpto = 1;
  w_enabled = 1; w_silent_dis = w_receiving = w_wire = w_sockb = 0; w_wire_end = 0; w_glued = 0;
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
  /* Потеря TCP замечена проверкой TCPLK не позже 5 с: закрытие, новое открытие после возврата без перезапуска. */
  w_tcp = 0;
  for (unsigned i = 0; i < 600 && !channel_closed; ++i) sim_tick();
  for (unsigned i = 0; i < 5; ++i) sim_tick();
  CHECK(channel_closed == 1 && !comm_channel_ready() && w_dis == 2);
  w_tcp = 1;
  for (unsigned i = 0; i < 600 && channel_opened < 2; ++i) sim_tick();
  CHECK(channel_opened == 2 && comm_channel_ready() && w_restarts == 1);
  /* Съёмка: TCP физически отключён до камеры, после неё новый сеанс без штрафной паузы. */
  { unsigned dis = w_dis, opened = channel_opened;
    comm_channel_registered();
    wifi_begin(W_LINK, "AT+TCPLK", 0, NULL); /* съёмка пришла во время запроса мосту */
    unsigned links = w_links;
    CHECK(!comm_channel_wifi_pause(true));
    for (unsigned i = 0; i < 600; ++i) sim_tick();
    CHECK(w_links == links && w_dis == dis + 1 && !w_enabled && !comm_channel_ready());
    w_receiving = 1; /* после +ok ещё выходит хвост данных с линии */
    for (unsigned i = 0; i < 10; ++i) sim_tick();
    CHECK(!comm_channel_wifi_pause(true));
    w_receiving = 0;
    for (unsigned i = 0; i < 4; ++i) sim_tick();
    CHECK(!comm_channel_wifi_pause(true));
    sim_tick();
    CHECK(comm_channel_wifi_pause(true) && comm_tx_acquire(&capacity) == NULL);
    uint8_t camera[] = {0xaa, 0x0d, 0, 0, 0, 0};
    unsigned before = w_outbox_count;
    uart_send(camera, sizeof(camera));
    CHECK(w_outbox_count == before); /* байты камеры не уходят серверу */
    comm_channel_wifi_pause(false);
    CHECK(!w_enabled);
    w_receiving = 1;
    for (unsigned i = 0; i < 10; ++i) sim_tick();
    CHECK(!w_enabled);
    w_receiving = 0;
    for (unsigned i = 0; i < 4; ++i) sim_tick();
    CHECK(!w_enabled); /* хвост камеры должен выйти до команды TCPDIS=on */
    for (unsigned i = 0; i < 200 && !comm_channel_ready(); ++i) sim_tick();
    CHECK(w_dis == dis + 2 && w_enabled && channel_opened == opened + 1 && comm_channel_ready()); }
  /* Нет подтверждения отключения: линию камере не выдавать, отмена возвращает мост. */
  { w_silent_dis = 1;
    CHECK(!comm_channel_wifi_pause(true));
    for (unsigned i = 0; i < 160; ++i) sim_tick();
    CHECK(!comm_channel_wifi_pause(true) && w_enabled);
    comm_channel_wifi_pause(false); w_silent_dis = 0;
    for (unsigned i = 0; i < 200 && !comm_channel_ready(); ++i) sim_tick();
    CHECK(comm_channel_ready() && w_enabled); }
  /* Повторные снимки не копят штрафы подключения и не оставляют мост выключенным. */
  for (unsigned shot = 0; shot < 10; ++shot) {
    comm_channel_registered();
    CHECK(!comm_channel_wifi_pause(true));
    for (unsigned i = 0; i < 20; ++i) sim_tick();
    CHECK(comm_channel_wifi_pause(true) && !w_enabled);
    comm_channel_wifi_pause(false);
    for (unsigned i = 0; i < 200 && !comm_channel_ready(); ++i) sim_tick();
    CHECK(comm_channel_ready() && ch.wifi && w_enabled && w_restarts == 1);
  }
  /* Ответ на команду съёмки должен уйти до разрыва серверного TCP. */
  slot = comm_tx_acquire(&capacity); CHECK(slot); memcpy(slot, "PHOTO-ACK", 9);
  CHECK(comm_tx_commit(9));
  { unsigned sent = w_outbox_count;
    CHECK(!comm_channel_wifi_pause(true) && !wf.paused);
    sim_tick();
    CHECK(w_outbox_count == sent + 9 && !memcmp(w_outbox + sent, "PHOTO-ACK", 9)); }
  /* Команда возврата потеряна: обычная повторная настройка оживляет мост. */
  unsigned attempts_before_photo = ch.connections;
  CHECK(!comm_channel_wifi_pause(true));
  CHECK(ch.connections == attempts_before_photo); /* съёмка не обнуляет прежние отказы регистрации */
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  CHECK(comm_channel_wifi_pause(true));
  w_silent_dis = 1; comm_channel_wifi_pause(false);
  for (unsigned i = 0; i < 1500 && !comm_channel_ready(); ++i) sim_tick();
  CHECK(comm_channel_ready() && ch.wifi && w_enabled && w_restarts == 2);
  w_silent_dis = 0;
  /* Проверка интернета идёт через AT+PING моста. */
  CHECK(comm_channel_probe(COMM_CH_PING));
  for (unsigned i = 0; i < 10; ++i) sim_tick();
  CHECK(w_pings == 1 && comm_channel_probe_result() == COMM_CH_PING);
  /* Мост в командном режиме (после прежней прошивки): префикс без ответа, настройка без него, прозрачный режим после перезапуска. */
  wifi_model_reset(1, 1);
  for (unsigned i = 0; i < 3000 && !comm_channel_ready(); ++i) sim_tick();
  CHECK(comm_channel_ready() && w_restarts == 1 && !w_cmd_mode && w_plain == 6 && w_stray == 1 && channel_opened == 1 && sim_now <= 25000);
  /* Точка доступа по USB: SSID и ключ записаны, настройка моста повторена, перезапуск, канал открыт заново. */
  CHECK(comm_channel_wifi_setup((const uint8_t *)"Home\0pass123\0", 13) == 0);
  CHECK(comm_channel_wifi_setup((const uint8_t *)"X\0y\0", 4) == 1);
  for (unsigned i = 0; i < 1500 && channel_opened < 2; ++i) sim_tick();
  CHECK(!strcmp(w_ssid, "Home") && !strcmp(w_key, "pass123") && w_restarts == 2 && channel_opened == 2 && !wf.setup);
  memset(big, 'a', sizeof(big)); big[50] = 0; big[sizeof(big) - 1u] = 0;
  CHECK(comm_channel_wifi_setup((const uint8_t *)"\0a\0", 3) == 2 && comm_channel_wifi_setup(big, sizeof(big)) == 2);
  /* Мост молчит: через 5 с серии сервер идёт через GSM без отказов открытия для надзора. */
  wifi_model_reset(0, 1); w_dead = 1;
  for (unsigned i = 0; i < 3000 && ch.wifi; ++i) sim_tick();
  CHECK(!ch.wifi && sim_now <= 12000 && !w_failed && !comm_channel_wifi());
  for (unsigned i = 0; i < 2000 && !comm_channel_ready(); ++i) sim_tick();
  CHECK(comm_channel_ready() && channel_opened == 1);
  /* Через 10 мин на GSM канал закрывает TCP и пробует мост; мост всё ещё молчит - GSM возвращается. */
  { unsigned commands = w_commands;
    for (unsigned i = 0; i < 61000 && !ch.wifi; ++i) sim_tick();
    for (unsigned i = 0; i < 5; ++i) sim_tick();
    CHECK(ch.wifi && channel_closed == 1 && w_commands > commands);
    for (unsigned i = 0; i < 3000 && (ch.wifi || !comm_channel_ready()); ++i) sim_tick();
    CHECK(!ch.wifi && comm_channel_ready() && channel_opened == 2); }
  /* На GSM после отказа моста записана точка доступа: мост настроен и перезапущен, через 8 с канал открывается через мост. */
  w_dead = 0; w_tcp = 1;
  CHECK(comm_channel_wifi_setup((const uint8_t *)"Home\0pass\0", 10) == 0);
  for (unsigned i = 0; i < 3000 && (!ch.wifi || !comm_channel_ready()); ++i) sim_tick();
  CHECK(ch.wifi && comm_channel_ready() && w_restarts == 1 && !strcmp(w_ssid, "Home") && channel_opened == 3);
  /* Мост отвечает, но TCP не поднимается: один перезапуск, серия длится минуту, затем GSM. */
  wifi_model_reset(0, 0);
  for (unsigned i = 0; i < 8000 && ch.wifi; ++i) sim_tick();
  CHECK(!ch.wifi && sim_now >= 60000 && sim_now <= 70000 && w_links >= 20 && w_restarts == 1 && !w_failed);
  for (unsigned i = 0; i < 2000 && !comm_channel_ready(); ++i) sim_tick();
  CHECK(comm_channel_ready());
  /* GSM продолжает работу, но автономный TCP моста тоже выключается перед камерой. */
  CHECK(!comm_channel_wifi_pause(true));
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  CHECK(comm_channel_wifi_pause(true) && !w_enabled && !ch.wifi && comm_channel_ready());
  wf.fell_at = sim_now - WF_RETRY_MS;
  for (unsigned i = 0; i < 10; ++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready()); /* возврат на Wi-Fi не отбирает линию у камеры */
  slot = comm_tx_acquire(&capacity); CHECK(slot); comm_tx_release();
  wf.fell_at = sim_now;
  comm_channel_wifi_pause(false);
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  CHECK(!ch.wifi && comm_channel_ready() && w_enabled);
  /* Ненастроенный прозрачный мост: камера получает линию только после отключения TCP.
     0 - сразу после старта; 1 - GSM после молчания моста, когда prefix остался равен 0. */
  for (unsigned failed = 0; failed < 2; ++failed) {
    wifi_model_reset(0, 1);
    if (failed) {
      w_dead = 1;
      for (unsigned i = 0; i < 5000 && (ch.wifi || !comm_channel_ready()); ++i) sim_tick();
      CHECK(!ch.wifi && comm_channel_ready() && !wf.prefix);
      w_dead = 0;
    }
    CHECK(!wf.f.ready);
    CHECK(!comm_channel_wifi_pause(true) && wf.paused);
    for (unsigned i = 0; i < 20; ++i) sim_tick();
    CHECK(comm_channel_wifi_pause(true) && !w_enabled && !w_restarts);
    uint8_t camera[] = {0xaa, 0x0d, 0, 0, 0, 0};
    unsigned before = w_outbox_count;
    uart_send(camera, sizeof(camera));
    CHECK(w_outbox_count == before && !w_plain);
    if (failed) CHECK(!ch.wifi && comm_channel_ready());
    comm_channel_wifi_pause(false);
    for (unsigned i = 0; i < 20; ++i) sim_tick();
    CHECK(w_enabled && !wf.paused);
  }
  /* Нет ответа, либо автономный SOCKB настроен: камера не должна передавать ни байта.
     Отключение только SOCKA недостаточно, а состояние SOCKB прошивка не меняет. */
  for (unsigned second = 0; second < 2; ++second) {
    wifi_model_reset(0, 1);
    w_sockb = second; w_silent_dis = !second;
    CHECK(!comm_channel_wifi_pause(true) && wf.paused);
    for (unsigned i = 0; i < 200; ++i) {
      sim_tick();
      CHECK(!comm_channel_wifi_pause(true));
    }
    CHECK(w_enabled == !second && w_sockb == second && !w_outbox_count);
    comm_channel_wifi_pause(false); w_silent_dis = 0;
    for (unsigned i = 0; i < 200 && wf.paused; ++i) sim_tick();
    CHECK(w_enabled && !wf.paused);
  }
  /* Незаконченная настройка не подменяется командой паузы и не выдаёт порт камере. */
  wifi_model_reset(0, 1);
  wifi_begin(W_ECHO, "AT+E=off", 0, NULL);
  CHECK(!comm_channel_wifi_pause(true) && !wf.paused && wf.op == W_ECHO);
  test_config.wifi = 0; w_dead = 0;
  puts("comm_wifi: DR164 transparent setup/send/recv/mix/link loss/pause/ping/cmd-mode/AP/fallback PASS");
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
  channel_path_tests(); channel_error_tests(); channel_read_tests(); channel_edge_tests(); channel_diagnostic_tests(); channel_info_tests(); channel_wifi_tests();
  comm_modem_transport(NULL); sim_poll = comm_modem_poll;
  puts("comm_channel: original TCP/READ/late URC/capacity/backpressure/timing PASS");
}
