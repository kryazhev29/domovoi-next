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
/* Стенд моста DR164 на USART1 (повадки сняты на стенде 20.10.2026): ответы мгновенные, тишина сразу.
   Мост слышит плату только на своей скорости (w_baud), иначе в его буфере остаётся мусор: в командном режиме
   следующая команда получает +ERR, одиночный CR его закрывает. Прозрачный режим понимает только префикс USR,
   остальное ушло бы в сокет A. Командный режим: сокет B - AT+SNDB ('>' и +ok=n) и AT+RCVB из очереди w_inbox. */
static uint8_t *w_rx; static uint16_t w_cap, w_count;
static char w_cmd[96], w_netp[96], w_sockb[96], w_ssid[40], w_key[72];
static uint8_t w_outbox[2100], w_inbox[1100], w_body[1100];
static uint16_t w_outbox_count, w_inbox_count, w_body_count, w_expect;
static unsigned w_sends, w_recvs, w_links, w_restarts, w_dis, w_pings, w_reads, w_failed, w_commands, w_wrong, w_clears;
static uint32_t w_baud, w_next_baud, w_plate_baud;
static uint8_t w_cmd_mode, w_next_mode, w_tcp, w_dead, w_ping_ok, w_ap, w_garbage;
// Положить байты моста в текущий приёмный буфер канала.
static void w_push(const void *s, uint16_t n)
{
  if (!w_rx || w_dead) return;
  if (n > w_cap - w_count) n = (uint16_t)(w_cap - w_count);
  memcpy(w_rx + w_count, s, n); w_count = (uint16_t)(w_count + n);
}
static void w_text(const char *s) { w_push(s, (uint16_t)strlen(s)); }
bool uart_init(uint32_t baud, uint8_t stop, uint8_t *rxbuffer, uint16_t bfsize)
{ (void)stop; w_plate_baud = baud; w_rx = rxbuffer; w_cap = bfsize; w_count = 0; return true; }
void uart_setRecTimeout(unsigned int t) { (void)t; }
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize) { w_rx = rxbuffer; w_cap = bfsize; w_count = 0; }
uint16_t uart_received(void) { return w_count; }
uint8_t uart_getstate(void) { return w_count != 0; }
bool uart_isReceiving(void) { return false; }
// Разобрать команду мосту или принять данные после '>'.
void uart_send(uint8_t *buf, uint16_t size)
{
  if (w_dead) return;
  if (w_plate_baud != w_baud) { ++w_wrong; if (w_cmd_mode) w_garbage = 1; return; } /* чужая скорость: шум */
  if (w_expect) {
    CHECK(size == w_expect && w_outbox_count + size <= sizeof(w_outbox));
    char done[24]; snprintf(done, sizeof(done), "+ok=%u\r\n\r\n", w_tcp ? size : 0u); w_expect = 0;
    if (w_tcp) { memcpy(w_outbox + w_outbox_count, buf, size); w_outbox_count = (uint16_t)(w_outbox_count + size); }
    w_text(done); return;
  }
  CHECK(size >= 1 && size < sizeof(w_cmd) && buf[size - 1u] == '\r');
  bool usr = size >= 3 && !memcmp(buf, "USR", 3);
  if (!w_cmd_mode && !usr) return; /* прозрачный режим: байты ушли бы в сокет A */
  if (size == 1) { ++w_clears; w_garbage = 0; w_text("+ERR=-1\r\n\r\n"); return; } /* одиночный CR */
  if (w_cmd_mode && (usr || w_garbage)) { w_garbage = 0; w_text("+ERR=-1\r\n\r\n"); return; }
  ++w_commands; memcpy(w_cmd, buf, size - 1u); w_cmd[size - 1u] = 0;
  const char *c = w_cmd + (usr ? 3 : 0);
  if (!strcmp(c, "AT+E=off") || !strcmp(c, "AT+EVENT=off") || !strcmp(c, "AT+WMODE=STA") || !strcmp(c, "AT+TCPTO=0")) { w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TMODE=cmd")) { w_next_mode = 1; w_text("+ok\r\n\r\n"); return; } /* вступит после перезапуска */
  if (!strcmp(c, "AT+UART=460800,8,1,NONE,NFC")) { w_next_baud = 460800u; w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+Z")) { ++w_restarts; w_cmd_mode = w_next_mode; w_baud = w_next_baud; w_garbage = 0; return; }
  if (!strncmp(c, "AT+NETP=", 8)) { strcpy(w_netp, c); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+SOCKB=", 9)) { strcpy(w_sockb, c + 9); w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+SOCKB")) { char r[120]; snprintf(r, sizeof(r), "+ok=%s\r\n\r\n", w_sockb); w_text(r); return; }
  if (!strncmp(c, "AT+TCPDISB=", 11)) { ++w_dis; w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSSSID=", 10)) { strcpy(w_ssid, c + 10); w_text("+ok\r\n\r\n"); return; }
  if (!strncmp(c, "AT+WSKEY=", 9)) { strcpy(w_key, c + 9); w_text("+ok\r\n\r\n"); return; }
  if (!strcmp(c, "AT+TCPLKB")) { ++w_links; w_text(w_tcp && w_cmd_mode && w_sockb[0] ? "+ok=on\r\n\r\n" : "+ok=off\r\n\r\n"); return; }
  if (!strcmp(c, "AT+WSLK")) { w_text(w_ap ? "+ok=Home(AA:BB)\r\n\r\n" : "+ok=Disconnected\r\n\r\n"); return; }
  if (!strcmp(c, "AT+PING=ya.ru")) { ++w_pings; w_text(w_ping_ok ? "+ok=Success:32 bytes\r\n\r\n" : "+ok=Timeout\r\n\r\n"); return; }
  if (!strncmp(c, "AT+SNDB=", 8)) { /* сокет закрыт: приглашение есть, подтверждение +ok=0 */
    ++w_sends; w_expect = (uint16_t)atoi(c + 8); CHECK(w_expect && w_expect <= 1000); w_text(">"); return;
  }
  if (!strncmp(c, "AT+RCVB=", 8)) {
    ++w_recvs; unsigned want = (unsigned)atoi(c + 8); CHECK(want && want <= 512 && !strcmp(strchr(c, ','), ",0"));
    uint16_t n = w_inbox_count < want ? w_inbox_count : (uint16_t)want;
    char head[16]; int h = snprintf(head, sizeof(head), "+ok=%u%s", n, n ? "," : "");
    w_push(head, (uint16_t)h); w_push(w_inbox, n); w_text("\r\n\r\n");
    memmove(w_inbox, w_inbox + n, w_inbox_count - n); w_inbox_count = (uint16_t)(w_inbox_count - n);
    return;
  }
  CHECK(0); /* неизвестная мосту команда */
}
// Приёмник данных сервера через мост: собирает тело без проверки источника.
static uint16_t w_binary(const uint8_t *p, uint16_t n)
{
  CHECK(w_body_count + n <= sizeof(w_body));
  memcpy(w_body + w_body_count, p, n); w_body_count = (uint16_t)(w_body_count + n); return n;
}
static void w_read_done(uint16_t n, uint8_t reason) { (void)n; CHECK(!reason); ++w_reads; }
static void w_event(uint8_t kind, uint8_t value) { if (kind == COMM_CH_OPEN_FAILED) ++w_failed; channel_model_event(kind, value); }
static void w_server(const void *s, uint16_t n) { memcpy(w_inbox + w_inbox_count, s, n); w_inbox_count = (uint16_t)(w_inbox_count + n); }
// Начать независимый сценарий канала через мост: режим (0 прозрачный, 1 командный) и скорость моста.
static void wifi_model_reset(uint8_t cmd_mode, uint32_t baud, uint8_t tcp)
{
  sim_reset(); test_config.wifi = 1;
  channel_opened = channel_closed = channel_faults = 0;
  w_rx = NULL; w_cap = w_count = w_outbox_count = w_inbox_count = w_body_count = w_expect = 0;
  w_sends = w_recvs = w_links = w_restarts = w_dis = w_pings = w_reads = w_failed = w_commands = w_wrong = w_clears = 0;
  w_cmd_mode = w_next_mode = cmd_mode; w_baud = w_next_baud = baud; w_plate_baud = 115200u;
  w_tcp = tcp; w_dead = w_garbage = 0; w_ping_ok = w_ap = 1;
  w_netp[0] = w_sockb[0] = w_ssid[0] = w_key[0] = 0;
  sim_start(); comm_channel_init(w_event); wf.f.ready = 0; wf.baud = 0; wf.trial = 0; /* новый стенд, порт на 115200 */
  comm_channel_receive(w_binary, w_read_done); comm_channel_rx_capacity(1000);
  sim_poll = comm_channel_poll;
}
// Взять слот кадра, когда опрос моста его отпустит (сервер делает так же - пробует в следующем проходе).
static uint8_t *w_acquire(uint16_t *capacity)
{ uint8_t *slot = NULL; for (unsigned i = 0; i < 10 && !(slot = comm_tx_acquire(capacity)); ++i) sim_tick(); return slot; }
// Записать точку доступа, как NextConfig: ответ «занято» (идёт опрос моста) - повтор.
static uint8_t w_setup(const char *data, uint16_t n)
{ uint8_t r = 1; for (unsigned i = 0; i < 10 && (r = comm_channel_wifi_setup((const uint8_t *)data, n)) == 1; ++i) sim_tick(); return r; }
static unsigned w_until_ready(unsigned ticks) { unsigned i = 0; while (i < ticks && !comm_channel_ready()) { sim_tick(); ++i; } return i; }
// Проверить сервер через мост: настройку из коробки, скорость, передачу, приём, потерю TCP, съёмку, ping, точку доступа и уход на GSM.
static void channel_wifi_tests(void)
{
  uint16_t capacity = 0; uint8_t *slot; char expect[96]; uint8_t big[1040];
  /* Мост из коробки (прозрачный на 115200): на 460800 молчит, с префиксом USR на 115200 настроен -
     A сервером на мосту, B на сервер 1, командный режим, 460800, перезапуск; через 8 с канал открыт. */
  wifi_model_reset(0, 115200u, 1);
  w_until_ready(1500);
  CHECK(comm_channel_ready() && channel_opened == 1 && w_restarts == 1 && sim_now <= 13000);
  CHECK(w_cmd_mode == 1 && w_baud == 460800u && w_plate_baud == 460800u && comm_channel_wifi_link() == 2);
  snprintf(expect, sizeof(expect), "TCP,%u,%s", test_config.port, test_config.host);
  CHECK(!strcmp(w_sockb, expect) && !strcmp(w_netp, "AT+NETP=TCP,SERVER,15013,10.0.0.1"));
  /* Кадр уходит через AT+SNDB: приглашение, данные, +ok=5; слот свободен. */
  slot = w_acquire(&capacity);
  CHECK(slot && capacity == 1040); memcpy(slot, "HELLO", 5); CHECK(comm_tx_commit(5));
  for (unsigned i = 0; i < 10; ++i) sim_tick();
  CHECK(w_sends == 1 && w_outbox_count == 5 && !memcmp(w_outbox, "HELLO", 5) && !comm_tx_busy());
  /* Кадр длиннее 1000 Б уходит двумя кусками. */
  w_outbox_count = 0; slot = w_acquire(&capacity); CHECK(slot);
  for (unsigned i = 0; i < sizeof(big); ++i) big[i] = (uint8_t)(i * 7u);
  memcpy(slot, big, sizeof(big)); CHECK(comm_tx_commit(sizeof(big)));
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  CHECK(w_sends == 3 && w_outbox_count == sizeof(big) && !memcmp(w_outbox, big, sizeof(big)) && !comm_tx_busy());
  /* Данные сервера приходят опросом AT+RCVB; 700 Б - кусками не больше 512. */
  w_server("OK\r\nXYZ", 7);
  for (unsigned i = 0; i < 20 && !w_body_count; ++i) sim_tick();
  CHECK(w_body_count == 7 && !memcmp(w_body, "OK\r\nXYZ", 7) && w_reads >= 1 && !w_inbox_count);
  w_body_count = 0; w_server(big, 700);
  for (unsigned i = 0; i < 40 && w_body_count < 700; ++i) sim_tick();
  CHECK(w_body_count == 700 && !memcmp(w_body, big, 700));
  /* Звонок не трогает канал моста. */
  comm_channel_call(true); CHECK(w_acquire(&capacity) != NULL); comm_tx_release(); comm_channel_call(false);
  CHECK(comm_channel_ready() && !comm_channel_expected());
  /* Сокет закрыт: SNDB подтверждает 0 байт - переоткрытие через TCPDISB, после возврата TCP канал открыт без перезапуска. */
  w_tcp = 0; slot = w_acquire(&capacity); CHECK(slot); memcpy(slot, "LOST", 4); CHECK(comm_tx_commit(4));
  for (unsigned i = 0; i < 400 && !channel_closed; ++i) sim_tick();
  for (unsigned i = 0; i < 5; ++i) sim_tick();
  CHECK(channel_closed == 1 && !comm_channel_ready() && w_dis == 2);
  w_tcp = 1;
  for (unsigned i = 0; i < 600 && channel_opened < 2; ++i) sim_tick();
  CHECK(channel_opened == 2 && comm_channel_ready() && w_restarts == 1);
  /* Съёмка: порт у камеры, команд мосту нет; мусор камеры в буфере моста закрывается одиночным CR, TCP проверяется. */
  { unsigned commands = w_commands, links = w_links, clears = w_clears, wrong = w_wrong; comm_channel_wifi_pause(true);
    w_plate_baud = 115200u; w_garbage = 1; /* камера на 115200 */
    for (unsigned i = 0; i < 300; ++i) sim_tick();
    CHECK(w_commands == commands && w_wrong == wrong);
    comm_channel_wifi_pause(false);
    for (unsigned i = 0; i < 10; ++i) sim_tick();
    CHECK(w_clears == clears + 1 && w_plate_baud == 460800u && w_links == links + 1 && comm_channel_ready() && !w_garbage); }
  /* Проверка интернета идёт через AT+PING моста. */
  CHECK(comm_channel_probe(COMM_CH_PING));
  for (unsigned i = 0; i < 10; ++i) sim_tick();
  CHECK(w_pings == 1 && comm_channel_probe_result() == COMM_CH_PING);
  /* Перезапуск платы при настроенном мосте (мусор в его буфере): CR, E=off, сверка сокета B - без перезапуска моста. */
  wifi_model_reset(1, 460800u, 1); snprintf(w_sockb, sizeof(w_sockb), "TCP,%u,%s", test_config.port, test_config.host); w_garbage = 1;
  CHECK(w_until_ready(100) < 100 && w_restarts == 0 && channel_opened == 1 && w_clears == 1);
  /* Мост наш, но сокет B смотрит на другой сервер: настройка без префикса и перезапуск. */
  wifi_model_reset(1, 460800u, 1); strcpy(w_sockb, "TCP,1,other.host");
  w_until_ready(1500);
  CHECK(comm_channel_ready() && w_restarts == 1 && !strcmp(w_sockb, expect));
  /* Мост в командном режиме на 115200 (прежняя прошивка платы): третья попытка, настройка, 460800. */
  wifi_model_reset(1, 115200u, 1);
  w_until_ready(2500);
  CHECK(comm_channel_ready() && w_restarts == 1 && w_baud == 460800u && channel_opened == 1 && sim_now <= 16000);
  /* Точка доступа по USB: SSID и ключ записаны, настройка повторена, перезапуск, канал открыт заново. */
  CHECK(w_setup("Home\0pass123\0", 13) == 0);
  CHECK(comm_channel_wifi_setup((const uint8_t *)"X\0y\0", 4) == 1);
  for (unsigned i = 0; i < 1500 && channel_opened < 2; ++i) sim_tick();
  CHECK(!strcmp(w_ssid, "Home") && !strcmp(w_key, "pass123") && w_restarts == 2 && channel_opened == 2 && !wf.setup);
  memset(big, 'a', 101); big[50] = 0; big[100] = 0;
  CHECK(comm_channel_wifi_setup((const uint8_t *)"\0a\0", 3) == 2 && comm_channel_wifi_setup(big, 101) == 2);
  /* Мост молчит на всех скоростях: через 5 с серии сервер идёт через GSM без отказов открытия для надзора. */
  wifi_model_reset(0, 115200u, 1); w_dead = 1;
  for (unsigned i = 0; i < 3000 && ch.wifi; ++i) sim_tick();
  CHECK(!ch.wifi && sim_now <= 12000 && !w_failed && !comm_channel_wifi());
  w_until_ready(2000);
  CHECK(comm_channel_ready() && channel_opened == 1);
  /* Через 10 мин на GSM канал закрывает TCP и пробует мост; мост всё ещё молчит - GSM возвращается. */
  { unsigned closed = channel_closed;
    for (unsigned i = 0; i < 61000 && !ch.wifi; ++i) sim_tick();
    for (unsigned i = 0; i < 5; ++i) sim_tick();
    CHECK(ch.wifi && channel_closed == closed + 1);
    for (unsigned i = 0; i < 3000 && (ch.wifi || !comm_channel_ready()); ++i) sim_tick();
    CHECK(!ch.wifi && comm_channel_ready() && channel_opened == 2); }
  /* На GSM после отказа моста записана точка доступа: мост настроен и перезапущен, через 8 с канал через мост. */
  w_dead = 0; w_tcp = 1;
  CHECK(w_setup("Home\0pass\0", 10) == 0);
  for (unsigned i = 0; i < 3000 && (!ch.wifi || !comm_channel_ready()); ++i) sim_tick();
  CHECK(ch.wifi && comm_channel_ready() && w_restarts == 1 && !strcmp(w_ssid, "Home") && channel_opened == 3);
  /* Мост отвечает, но TCP не поднимается: один перезапуск, серия длится минуту, затем GSM. */
  wifi_model_reset(0, 115200u, 0);
  for (unsigned i = 0; i < 8000 && ch.wifi; ++i) sim_tick();
  CHECK(!ch.wifi && sim_now >= 60000 && sim_now <= 70000 && w_links >= 20 && w_restarts == 1 && !w_failed);
  w_until_ready(2000);
  CHECK(comm_channel_ready());
  test_config.wifi = 0; w_dead = 0;
  puts("comm_wifi: DR164 socket B 460800 box setup/send/recv/link loss/pause/ping/restart/AP/fallback PASS");
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
