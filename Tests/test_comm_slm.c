/* Доработка: Кряжев А., 2026. Имитатор SLM336 на настоящих AT/UART.
   Ответы и времена синтетические; границы канала и восстановления задаются явно.
   Каждый сценарий начинает новый сеанс и проверяет физическую отправку команд. */
/* Изолированный модем не имеет канала/восстановления: явные заглушки его границы. */
#ifndef COMM_CHANNEL_MODEL
// Обозначить отсутствие ожидаемого обрыва в изолированном модеме.
bool comm_channel_expected(void) { return false; }
// Обозначить границу восстановления изолированного модема.
void comm_recover_failure(uint8_t step,uint8_t reason,uint32_t now) { (void)step; (void)reason; (void)now; }
#endif
// Обозначить неиспользуемую в изолированном стенде доставку событий.
static void ignore_event(uint8_t k,const uint8_t *p,uint16_t n,uint16_t d) { (void)k; (void)p; (void)n; (void)d; }
// Обозначить неиспользуемого в изолированном стенде получателя URC.
static void ignore_urc(const uint8_t *p,uint16_t n) { (void)p; (void)n; }
static uint32_t sim_now, reply_due;
static unsigned sim_ticks, sim_commands, sim_gmm, sim_found, sim_qird, sim_done_reads;
static uint16_t sim_body_count, sim_read_size;
static uint8_t sim_read_reason;
#ifdef COMM_CHANNEL_MODEL
static uint8_t sim_body[1024];
#endif
static char sim_pending[400], sim_sent[128];
static int sim_silence_step, sim_error_step;
static unsigned sim_no_sim, sim_pin, sim_open_delay, sim_open_error, sim_rat_change;
static unsigned sim_port_change, sim_network_wait, sim_early_open, sim_trickle;
static unsigned sim_empty_urc, sim_pause, sim_send_sim2, sim_nwinfo_error, sim_csq;
static const char *sim_imsi;
static const char *sim_apn_response;
static unsigned sim_apn_queries, sim_imsi_queries, sim_activations, sim_pdp_failures, sim_pdp_timeouts, sim_apn_count;
static char sim_apn_commands[8][128];
static unsigned sim_activation_delay, sim_data_echo, sim_send_race;
static uint32_t sim_open_due;
static bool sim_open_pending;
static unsigned power_events;
static struct { uint32_t time; uint8_t pin, high; } power_trace[16];
static struct {
  char host[64], apn[31], login[31], password[31];
  uint16_t port;
  uint8_t modem_type, network_type;
} test_config;
static const char *sim_text(uint8_t field, uint16_t *capacity)
{
  switch (field) {
  case SLM_CFG_HOST: *capacity = sizeof(test_config.host); return test_config.host;
  case SLM_CFG_APN: *capacity = sizeof(test_config.apn); return test_config.apn;
  case SLM_CFG_LOGIN: *capacity = sizeof(test_config.login); return test_config.login;
  case SLM_CFG_PASSWORD: *capacity = sizeof(test_config.password); return test_config.password;
  default: *capacity = 0; return NULL;
  }
}
// Выдать числовую настройку имитатора модема.
static uint16_t sim_number(uint8_t field)
{
  if (field == SLM_CFG_NETWORK && sim_send_race && slm.attempt.status.step == SLM_RAT_SET) {
    unsigned race = sim_send_race; sim_send_race = 0;
    interrupt_byte('\r', USART_SR_RXNE);
    if (race == 1) interrupt_byte('\n', USART_SR_RXNE);
  }
  return field == SLM_CFG_PORT ? test_config.port : field == SLM_CFG_TYPE
    ? test_config.modem_type : field == SLM_CFG_NETWORK ? test_config.network_type : 0;
}
static const slm_settings_t sim_settings = {sim_text, sim_number};

// Запомнить переключение выводов питания модема.
static void sim_gpio(uint8_t pin, bool high)
{
  CHECK((pin == 13 || pin == 14) && power_events < sizeof(power_trace) / sizeof(power_trace[0]));
  power_trace[power_events].time = sim_now;
  power_trace[power_events].pin = pin;
  power_trace[power_events++].high = (uint8_t)high;
}
#ifdef COMM_CHANNEL_MODEL
// Запомнить двоичные данные QIRD имитатора.
static uint16_t sim_binary(const uint8_t *p, uint16_t n)
{
  CHECK(p >= rx && p < rx + sizeof(rx));
  if (sim_pause) return 0;
  CHECK(sim_body_count + n <= sizeof(sim_body));
  memcpy(sim_body + sim_body_count, p, n); sim_body_count = (uint16_t)(sim_body_count + n);
  return n;
}
// Запомнить завершение чтения имитатора.
static void sim_read_done(uint16_t n, uint8_t reason)
{
  CHECK(n <= sim_read_size); sim_read_reason = reason;
  ++sim_done_reads;
}
#endif
// Запомнить обнаруженный тип модема.
static void sim_type(uint8_t type) { CHECK(type == 2); ++sim_found; }
static unsigned sim_imei_count;
// Запомнить полученный IMEI.
static void sim_imei(const char *imei) { CHECK(!strcmp(imei, "864180068438709")); ++sim_imei_count; }
static const slm_handlers_t sim_handlers = {sim_gpio, sim_type, sim_imei};

// Сбросить UART, время и наблюдения имитатора.
static void sim_reset(void)
{
  comm_modem_off();
#if DOMOVOI_DIAG
  memset(&slm.diag, 0, sizeof(slm.diag));
#endif
  comm_modem_transport(ignore_event);
  for (uint8_t i=0;i<COMM_URC_COUNT;++i) comm_modem_listen(i,ignore_urc);
  memset(&slm.attempt.status, 0, sizeof(slm.attempt.status));
  memset(&test_config, 0, sizeof(test_config));
  memcpy(test_config.host, "test.local", sizeof("test.local"));
  test_config.port = 7777; test_config.modem_type = 2;
  primask = pending_sr = edge_at = barriers = reads = writes = 0;
  sim_now = reply_due = sim_open_due = 0;
  sim_ticks = sim_commands = sim_gmm = sim_found = sim_qird = sim_done_reads = 0;
  sim_body_count = sim_read_size = 0; sim_read_reason = 0; sim_pending[0] = sim_sent[0] = 0;
  sim_silence_step = sim_error_step = -1;
  sim_no_sim = sim_pin = sim_open_error = sim_rat_change = sim_port_change = 0;
  sim_network_wait = sim_early_open = sim_trickle = sim_empty_urc = sim_pause = 0;
  sim_send_sim2 = 1; sim_nwinfo_error = 0; sim_csq = 22; sim_open_delay = 500; sim_activation_delay = 700; sim_data_echo = 1; sim_send_race = 0;
  sim_imsi = "250011234567890\r\nOK\r\n"; sim_open_pending = false; power_events = 0;
  sim_apn_response = "OK\r\n";
  sim_apn_queries = sim_imsi_queries = sim_activations = sim_pdp_failures = sim_pdp_timeouts = sim_apn_count = 0;
}
// Передать ответ имитатора в приёмное кольцо UART.
static void sim_feed(const char *s)
{
  unsigned n = (unsigned)strlen(s);
  CHECK(n < sizeof(rx));
  for (unsigned i = 0; i < n; ++i) interrupt_byte((uint8_t)s[i], USART_SR_RXNE);
}
// Подготовить очередной ответ имитатора модема.
static void sim_reply(const char *s, unsigned delay)
{
  CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));
  memcpy(sim_pending, s, strlen(s) + 1u); reply_due = sim_now + delay;
}
// Обработать физически отправленную команду имитатора.
static void sim_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE)) return;
  CHECK(tx_data == command && tx_count <= sizeof(sim_sent));
  memcpy(sim_sent, command, tx_count); sim_sent[tx_count - 1u] = 0;
  /* IRQ передаёт из единственного статического буфера AT, без ожидания CPU. */
  for (unsigned i = 0; i < tx_count; ++i) interrupt_byte(0, USART_SR_TXE);
  interrupt_byte(0, USART_SR_TC);
  ++sim_commands;
  unsigned step = slm.attempt.status.step;
  if (!strcmp(sim_sent, "AT+CMEE?")) { sim_reply("OK\r\n+CMEE: 1\r\nOK\r\n", 30); return; }
  if ((int)step == sim_silence_step) return;
  if ((int)step == sim_error_step) { sim_reply("ERROR\r\n", 30); return; }
  if (!strcmp(sim_sent, "AT")) sim_reply("OK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+QISDE=0")) { sim_data_echo = 0; sim_reply("OK\r\n", 30); }
  else if (!strcmp(sim_sent, "AT+CSQ")) { char r[32]; snprintf(r, sizeof(r), "+CSQ: %u,99\r\nOK\r\n", sim_csq); sim_reply(r, 30); }
  else if (!strcmp(sim_sent, "AT+QNWINFO")) sim_reply(sim_nwinfo_error ? "ERROR\r\n" : "+QNWINFO: \"FDD LTE\",\"25001\",\"LTE BAND 3\",1300\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+GSN")) sim_reply("864180068438709\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+GMM")) { sim_gmm++; sim_reply("SLM336E\r\nOK\r\n", 30); }
  else if (!strcmp(sim_sent, "AT+CPIN?")) {
    if (sim_no_sim) sim_reply("+SIM1 NOT INSERT\r\n+SIM2 NOT INSERT\r\n+CME ERROR: 10\r\n", 100);
    else if (sim_pin) sim_reply("+CPIN: SIM PIN\r\nOK\r\n", 100);
    else sim_reply(sim_send_sim2 ? "+SIM2 NOT INSERT\r\n+CPIN: READY\r\nOK\r\n" : "+CPIN: READY\r\nOK\r\n", 100);
  } else if (!strcmp(sim_sent, "AT+CREG?")) {
    sim_reply("+CREG: 0,1\r\nOK\r\n", 80);
  } else if (!strcmp(sim_sent, "AT+CGREG?")) {
    sim_reply(sim_network_wait == 1 ? "+CGREG: 0,2\r\nOK\r\n" : (sim_network_wait == 2 || sim_network_wait == 4) ? "+CGREG: 0,3\r\nOK\r\n" : "+CGREG: 0,1\r\nOK\r\n", 80);
  } else if (!strcmp(sim_sent, "AT+CEREG?")) {
    sim_reply(sim_network_wait == 1 ? "+CEREG: 0,2\r\nOK\r\n" : (sim_network_wait == 3 || sim_network_wait == 4) ? "+CEREG: 0,3\r\nOK\r\n" : "+CEREG: 0,1\r\nOK\r\n", 80);
  } else if (!strcmp(sim_sent, "AT+CIMI")) { ++sim_imsi_queries; sim_reply(sim_imsi, 30); }
  else if (!strcmp(sim_sent, "AT+CGCONTRDP=1")) { ++sim_apn_queries; sim_reply(sim_apn_response, 30); }
  else if (!strncmp(sim_sent, "AT+QICSGP=", 10)) {
    strcpy(sim_apn_commands[sim_apn_count++ % 8u], sim_sent); sim_reply("OK\r\n", 30);
  }
  else if (!strcmp(sim_sent, "AT+QURCCFG=\"urcport\""))
    sim_reply(sim_port_change ? "+QURCCFG: \"urcport\",\"usbat\"\r\nOK\r\n" : "+QURCCFG: \"urcport\",\"uart1\"\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+QCFG=\"nwscanmode\""))
    sim_reply(sim_rat_change == 8 ? "+QCFG: \"nwscanmode\",8\r\nOK\r\n" : sim_rat_change ? "+QCFG: \"nwscanmode\",1\r\nOK\r\n" : "+QCFG: \"nwscanmode\",0\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+QIACT=1")) {
    ++sim_activations;
    if (sim_activations <= sim_pdp_timeouts) return;
    sim_reply(sim_activations <= sim_pdp_failures ? "ERROR\r\n" : "OK\r\n", sim_activation_delay);
  }
  else if (!strncmp(sim_sent, "AT+QIOPEN=", 10)) {
    char expected[128];
    CHECK(snprintf(expected,sizeof(expected),"AT+QIOPEN=1,0,\"TCP\",\"%s\",%u,0,0",test_config.host,test_config.port) > 0);
    CHECK(!strcmp(sim_sent,expected));
    if (sim_early_open) sim_reply("+QIOPEN: 0,0\r\nOK\r\n", 30);
    else { sim_reply("OK\r\n", 30); sim_open_pending = true; sim_open_due = sim_now + sim_open_delay; }
  } else if (!strncmp(sim_sent, "AT+QIRD=", 8)) {
    sim_qird++;
    unsigned want; CHECK(sim_read_size && sscanf(sim_sent, "AT+QIRD=0,%u", &want) == 1 && want <= 256);
    if (sim_qird == 1) sim_reply("+QIRD: 4\r\nOK\r\n\r\n+QIURC: \"recv\",0\r\nOK\r\n", 20);
    else if (sim_qird == 2) sim_reply("+QIURC: \"recv\",0\r\n+QIRD: 3\r\nXYZ\r\nOK\r\n", 20);
    else if (sim_qird == 3 && sim_empty_urc) sim_reply("+QIRD: 0\r\n+QIURC: \"recv\",0\r\nOK\r\n", 20);
    else sim_reply("+QIRD: 0\r\nOK\r\n", 20);
  } else sim_reply("OK\r\n", 30);
}
static void (*sim_poll)(uint32_t) = comm_modem_poll;
// Продвинуть время, UART и модем на один шаг.
static void sim_tick(void)
{
  ++sim_ticks;
  if (sim_open_pending && (int32_t)(sim_now - sim_open_due) >= 0) {
    sim_open_pending = false;
    sim_feed(sim_open_error ? "+QIOPEN: 0,566\r\n" : "+QIOPEN: 0,0\r\n");
  }
  if (sim_pending[0] && (int32_t)(sim_now - reply_due) >= 0) {
    if (sim_trickle) {
      for (unsigned j = 0; j < 3 && sim_pending[0]; ++j) {
        interrupt_byte((uint8_t)sim_pending[0], USART_SR_RXNE);
        memmove(sim_pending, sim_pending + 1, strlen(sim_pending));
      }
    } else { sim_feed(sim_pending); sim_pending[0] = 0; }
  }
  sim_poll(sim_now);
  sim_tx();
  sim_now += 10;
}
// Продвинуть запуск до указанного шага таблицы.
static void sim_until(uint32_t limit)
{
  unsigned watchdog = 0;
  while (slm.attempt.status.state == SLM_STARTING && watchdog++ < limit / 10u) sim_tick();
  CHECK(watchdog < limit / 10u);
}
// Запустить реальный модем на функциях настроек имитатора.
static void sim_start(void) { CHECK(modem_slm336_start(&sim_settings, &sim_handlers, sim_now)); }
// Проверить штатные пути питания, SIM, сети и контекста данных.
static void path_tests(void)
{
  sim_reset(); sim_start(); sim_until(15000);
  CHECK(comm_modem_ready() && slm.attempt.status.reason == SLM_ERR_NONE && !sim_gmm);
  CHECK(sim_now >= 8000 && sim_now <= 11000 && sim_ticks > 800);
  DIAG_CHECK(slm.attempt.status.completed_phases == 31); DIAG_CHECK(!slm.diag.failure_count);
  DIAG_CHECK((slm.diag.steps[SLM_ACTIVATE] * 10u) == 700);
  DIAG_CHECK(slm.diag.steps[SLM_TCP] == 0);
  CHECK(power_events == 5 && power_trace[0].pin == 13 && !power_trace[0].high);
  CHECK(power_trace[1].pin == 14 && power_trace[1].high);
  CHECK(power_trace[2].pin == 13 && power_trace[2].time == 2100 && power_trace[2].high);
  CHECK(power_trace[3].pin == 14 && power_trace[3].time == 3000 && !power_trace[3].high);
  CHECK(power_trace[4].time == 5000 && power_trace[4].high);
  printf("SLM normal model: %u ms incl discharge, %u ms from PC13 on, %u polls\n",sim_now,sim_now-power_trace[2].time,sim_ticks);
  CHECK(!strcmp(slm_apn(), "internet.mts.ru"));
  CHECK(sim_commands < 26); /* Нет команд регистрации на сервере. */
  sim_reset(); test_config.modem_type = 255; sim_start(); sim_until(15000);
  CHECK(comm_modem_ready() && sim_gmm == 1 && sim_found == 1);
  sim_reset(); sim_port_change = sim_rat_change = sim_early_open = 1;
  memcpy(test_config.apn, "manual.apn", sizeof("manual.apn")); sim_start(); sim_until(15000);
  CHECK(comm_modem_ready() && !strcmp(slm_apn(),"manual.apn"));
  DIAG_CHECK(slm.attempt.status.completed_steps & ((uint32_t)1u << SLM_URC_SET));
  DIAG_CHECK(slm.attempt.status.completed_steps & ((uint32_t)1u << SLM_RAT_SET));
  /* Выбор APN по IMSI для всех операторов и неизвестного профиля. */
  static const struct { const char *imsi, *apn; } profiles[] = {
    {"250010123456789\r\nOK\r\n", "internet.mts.ru"}, {"250020123456789\r\nOK\r\n", "internet"},
    {"250990123456789\r\nOK\r\n", "internet.beeline.ru"}, {"250200123456789\r\nOK\r\n", "internet.tele2.ru"},
    {"250110123456789\r\nOK\r\n", "internet.yota"}, {"310010123456789\r\nOK\r\n", ""}
  };
  for (uint8_t i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
    sim_reset(); sim_imsi = profiles[i].imsi; sim_start(); sim_until(15000);
    CHECK(comm_modem_ready() && !strcmp(slm_apn(), profiles[i].apn));
  }
  sim_reset(); sim_trickle = 1; sim_start(); sim_until(20000); CHECK(comm_modem_ready());
  /* DMA принял хвост CRLF между проверкой busy и постановкой команды. */
  sim_reset(); sim_rat_change = 1; sim_start(); sim_send_race = 1; sim_until(15000);
  CHECK(comm_modem_ready() && !sim_send_race); DIAG_CHECK(!slm.diag.failure_count);
  /* Незавершённая строка не должна удерживать запуск бесконечно. */
  sim_reset(); sim_rat_change = 1; sim_start(); sim_send_race = 2; sim_until(15000);
  CHECK(!comm_modem_ready() && slm.attempt.status.reason == SLM_ERR_TIMEOUT);
}
// Проверить отказы отдельных шагов запуска.
static void error_tests(void)
{
  sim_reset(); sim_no_sim = 1; sim_start(); sim_until(15000);
  CHECK(!comm_modem_ready() && slm.attempt.status.reason == SLM_ERR_NO_SIM && slm.attempt.status.step == SLM_SIM);
  DIAG_CHECK(slm.diag.failure_step[0] == SLM_SIM); DIAG_CHECK(slm.diag.failure_count == 1);
  sim_reset(); sim_pin = 1; sim_start(); sim_until(15000); CHECK(slm.attempt.status.reason == SLM_ERR_PIN);
  sim_reset(); sim_error_step = SLM_ACTIVATE; sim_start(); sim_until(15000);
  CHECK(slm.attempt.status.reason == SLM_ERR_PDP && slm.attempt.status.step == SLM_ACTIVATE);
  sim_reset(); sim_network_wait = 1; sim_start(); sim_until(80000); CHECK(slm.attempt.status.reason == SLM_ERR_TIMEOUT && slm.attempt.status.step == SLM_NETWORK);
}
// Проверить сроки ответов каждого шага таблицы.
static void timeout_step_tests(void)
{
  for (unsigned step = SLM_AT_READY; step <= SLM_DATA_ECHO; ++step) {
    sim_reset();
    test_config.modem_type = 255; sim_port_change = sim_rat_change = 1;
    sim_silence_step = (int)step; sim_start();
    if (step == SLM_ACTIVATE) slm.apn.source = SLM_APN_EMPTY; /* Срок последнего источника. */
    sim_until(180000);
    CHECK(slm.attempt.status.state == SLM_FAILED && slm.attempt.status.reason == SLM_ERR_TIMEOUT);
    CHECK(slm.attempt.status.step == step); DIAG_CHECK(slm.diag.failure_step[0] == step);
    DIAG_CHECK((slm.diag.steps[step] * 10u) > 0);
  }
  /* Питание: скачки тиков, wrap и точные границы без сжатия пауз. */
  sim_reset(); sim_now = UINT32_MAX - 999u; sim_start();
  sim_now += 1999u; comm_modem_poll(sim_now); CHECK(slm.attempt.status.step == SLM_DISCHARGE);
  ++sim_now; comm_modem_poll(sim_now); CHECK(slm.attempt.status.step == SLM_POWER_LEAD);
  sim_now += 100000u; comm_modem_poll(sim_now); CHECK(slm.attempt.status.step == SLM_POWER_RAIL);
  sim_now += 899u; comm_modem_poll(sim_now); CHECK(slm.attempt.status.step == SLM_POWER_RAIL);
  ++sim_now; comm_modem_poll(sim_now); CHECK(slm.attempt.status.step == SLM_POWER_KEY);
}

// Проверить пограничные ответы и повторные запуски.
static void edge_tests(void)
{
  sim_reset(); sim_imsi = "999011234567890\r\nOK\r\n"; sim_start(); sim_until(15000);
  CHECK(comm_modem_ready() && !slm_apn()[0]);
  sim_reset(); sim_activation_delay = 149000; sim_start(); sim_until(180000);
  CHECK(comm_modem_ready()); DIAG_CHECK((slm.diag.steps[SLM_ACTIVATE] * 10u) == 149000);
  sim_reset(); memset(test_config.host,'h',sizeof(test_config.host)-1u);
  memset(test_config.apn,'a',sizeof(test_config.apn)-1u);
  memset(test_config.login,'l',sizeof(test_config.login)-1u);
  memset(test_config.password,'p',sizeof(test_config.password)-1u);
  sim_start();
  while (slm.attempt.status.state != SLM_READY && slm.attempt.status.state == SLM_STARTING) sim_tick();
  CHECK(slm.attempt.status.step == SLM_DATA_ECHO && slm.attempt.status.state == SLM_READY); /* Максимальный QICSGP помещается. */
  comm_modem_off();
  sim_reset(); sim_start(); sim_until(15000);
  sim_feed("+QIURC: \"pdpdeact\",1\r\n"); sim_tick(); CHECK(comm_modem_ready());
}
// Проверить сроки фаз и кольцо сбоев диагностики.
static void diagnostic_tests(void)
{
  sim_reset(); sim_start(); sim_until(15000);
#if DOMOVOI_DIAG
  uint32_t tcp_ms = (slm.diag.steps[SLM_TCP] * 10u);
#endif
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == tcp_ms);
  DIAG_CHECK(sizeof(slm.diag) <= 128);
  comm_modem_off(); sim_no_sim = 1; sim_start(); sim_until(15000);
  CHECK(slm.attempt.status.reason == SLM_ERR_NO_SIM); DIAG_CHECK(slm.diag.steps[SLM_SIM] == 10);
  DIAG_CHECK(slm.diag.failure_step[0] == SLM_SIM); DIAG_CHECK(slm.diag.failure_count == 1);
  DIAG_CHECK(slm.diag.steps[SLM_ACTIVATE] == 0 && slm.diag.phases[SLM_PHASE_TCP] == 0);
  /* Десять сбоев, оборачивание кольца и сохранение между попытками. */
  for (unsigned i = 0; i < 12; ++i) {
    comm_modem_off(); power_events = 0; sim_start(); sim_until(sim_now + 15000u);
  }
  DIAG_CHECK(slm.diag.failure_next == 3); DIAG_CHECK(slm.diag.failure_count == 10);
#if DOMOVOI_DIAG
  for (unsigned i = 0; i < sizeof(slm.diag.failure_step); ++i)
    CHECK(slm.diag.failure_step[i] == SLM_SIM && slm.diag.failure_reason[i] == SLM_ERR_NO_SIM);
#endif
  sim_reset(); sim_start(); sim_now = 1500; comm_modem_poll(sim_now);
  DIAG_CHECK((slm.diag.steps[SLM_DISCHARGE] * 10u) == 1500 && (slm.diag.phases[SLM_PHASE_POWER] * 10u) == 1500);
  slm.attempt.now = slm.attempt.step_started + 1000000u; DIAG_CHECK(slm_elapsed(slm.attempt.step_started) == UINT16_MAX);
}
// Проверить отказ неверных настроек до запуска UART.
static void settings_tests(void)
{
  sim_reset(); test_config.modem_type = 3; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  test_config.modem_type = 2; test_config.apn[0] = '"'; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  memset(test_config.apn,'A',sizeof(test_config.apn)); CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  sim_reset(); test_config.network_type = 6; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  /* Геттеры читаются при составлении команды, копии настроек нет. */
  sim_reset(); sim_start();
  memcpy(test_config.apn, "changed.apn", sizeof("changed.apn"));
  sim_until(15000); CHECK(comm_modem_ready() && !strcmp(slm_apn(), "changed.apn"));
  sim_reset(); sim_start(); CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  /* Новый сеанс отменяет служебный запрос даже без снятия питания. */
  sim_reset(); sim_start(); sim_until(15000);
  static const comm_side_t side = {"AT+CSQ", NULL, NULL, 0, COMM_AT_TEXT, COMM_T_AT_SIMPLE};
  CHECK(comm_modem_side(&side));
  CHECK(comm_modem_restart(false,sim_now));
  CHECK(comm_modem_side_result(&side) == COMM_AT_ERROR);
  sim_reset(); slm.settings = sim_settings; slm.apn.source = SLM_APN_MNC;
  for (unsigned i = 0; i < 5; ++i) {
    static const uint8_t mncs[] = {1,2,99,20,11};
    static const char *expected[] = {"internet.mts.ru","internet","internet.beeline.ru","internet.tele2.ru","internet.yota"};
    slm.apn.mnc = mncs[i]; CHECK(!strcmp(slm_apn(),expected[i]));
  }
  slm.apn.mnc = 88; CHECK(!slm_apn()[0]);
}
static const struct {
  const char *name, *manual, *login, *password, *imsi, *network, *commands[4];
  unsigned rejects, source, queries, imsi_queries, gsm, ready, count;
} apn_cases[] = {
/* СЦЕНАРИИ APN */
};
// Проверить источники по физическим QICSGP и ответы синтетических сценариев.
static void apn_tests(void)
{
  for (unsigned i = 0; i < sizeof(apn_cases) / sizeof(apn_cases[0]); ++i) {
    sim_reset();
    strcpy(test_config.apn, apn_cases[i].manual); strcpy(test_config.login, apn_cases[i].login);
    strcpy(test_config.password, apn_cases[i].password);
    sim_imsi = apn_cases[i].imsi; sim_apn_response = apn_cases[i].network;
    sim_pdp_failures = apn_cases[i].rejects;
    if (apn_cases[i].gsm) { test_config.network_type = 1; sim_rat_change = 1; sim_network_wait = 3; }
    sim_start(); sim_until(20000);
    CHECK(comm_modem_ready() == apn_cases[i].ready && slm.apn.source == apn_cases[i].source);
    CHECK(sim_apn_queries == apn_cases[i].queries && sim_imsi_queries == apn_cases[i].imsi_queries);
    CHECK(sim_apn_count == apn_cases[i].count && sim_activations == apn_cases[i].count);
    for (unsigned j = 0; j < sim_apn_count; ++j) CHECK(!strcmp(sim_apn_commands[j], apn_cases[i].commands[j]));
    DIAG_CHECK(comm_modem_diag()->apn_source == apn_cases[i].source);
    if (!apn_cases[i].ready) {
      CHECK(slm.attempt.status.reason == SLM_ERR_PDP);
      /* Без успешного источника следующая попытка начинает цепочку заново. */
      sim_pdp_failures = 0;
      CHECK(comm_modem_restart(false, sim_now)); sim_until(15000);
      CHECK(comm_modem_ready() && slm.apn.source == SLM_APN_MANUAL);
      CHECK(sim_apn_queries == apn_cases[i].queries && sim_activations == apn_cases[i].count + 1);
      CHECK(!strcmp(sim_apn_commands[apn_cases[i].count], apn_cases[i].commands[0]));
      continue;
    }
    /* Потеря PDP и мягкий запуск сохраняют источник, без CIMI/CGCONTRDP. */
    comm_modem_data_failed(SLM_ERR_PDP); CHECK(comm_modem_restart(false, sim_now)); sim_until(15000);
    CHECK(comm_modem_ready() && slm.apn.source == apn_cases[i].source && slm.apn.locked);
    CHECK(sim_apn_queries == apn_cases[i].queries && sim_imsi_queries == apn_cases[i].imsi_queries);
    CHECK(sim_apn_count == apn_cases[i].count + 1 && sim_activations == sim_apn_count);
    CHECK(!strcmp(sim_apn_commands[sim_apn_count - 1], apn_cases[i].commands[apn_cases[i].count - 1]));
  }
  /* Новое питание снова читает сеть; отказ кэшированного источника не перебирает APN. */
  sim_reset(); sim_apn_response = "+CGCONTRDP: 1,5,first.apn\r\nOK\r\n";
  sim_start(); sim_until(15000); CHECK(comm_modem_ready());
  sim_pdp_failures = 2;
  CHECK(comm_modem_restart(false, sim_now)); sim_until(15000);
  CHECK(!comm_modem_ready() && sim_apn_queries == 1 && sim_activations == 2);
  sim_apn_response = "+CGCONTRDP: 1,5,second.apn\r\nOK\r\n";
  CHECK(comm_modem_restart(true, sim_now)); sim_until(15000);
  CHECK(comm_modem_ready() && sim_apn_queries == 2 && !strcmp(slm_apn(), "second.apn"));
  /* Таймаут QIACT сохраняет 150 с; AT восстанавливает границы перед резервом. */
  sim_reset(); sim_apn_response = "+CGCONTRDP: 1,5,m2m.apn\r\nOK\r\n"; sim_pdp_timeouts = 1;
  sim_start(); sim_until(200000);
  CHECK(comm_modem_ready() && sim_now >= 150000 && sim_activations == 2);
  CHECK(slm.apn.source == SLM_APN_MNC && sim_apn_queries == 1 && sim_imsi_queries == 1);
}
// Запустить все сценарии этого стенда.
int main(void)
{
  path_tests(); error_tests(); timeout_step_tests(); edge_tests(); diagnostic_tests(); settings_tests(); apn_tests();
  for (unsigned net = 2; net <= 3; ++net) {
    sim_reset(); sim_network_wait = net; sim_start(); sim_until(15000); CHECK(comm_modem_ready());
  }
  sim_reset(); sim_network_wait = 4; sim_start(); sim_until(80000); CHECK(slm.attempt.status.reason == SLM_ERR_TIMEOUT);
  sim_reset(); sim_rat_change = 8; sim_start(); sim_until(15000); CHECK(comm_modem_ready());
  /* Шум выключения/включения не должен стать ошибкой уже новой попытки. */
  sim_reset(); sim_start();
  interrupt_byte(0, USART_SR_FE);
  for (unsigned i = 0; i < 520; ++i) interrupt_byte('Y', USART_SR_RXNE);
  sim_until(15000);
  CHECK(comm_modem_ready() && !comm_uart_errors()); DIAG_CHECK(!slm.diag.failure_count);
  puts("comm_slm: power/setup/SIM1/SIM2/network/APN/QIACT/timeouts/timing PASS");
  return 0;
}
