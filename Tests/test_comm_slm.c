/* Доработка: Кряжев А., 2026. Имитатор SLM336 на настоящих AT/UART.
   Ответы и времена — СИНТЕТИКА; SIM URC взяты из журнала без SIM этапа 2. */
static uint32_t sim_now, reply_due;
static unsigned sim_ticks, sim_commands, sim_gmm, sim_found, sim_qird, sim_done_reads;
static uint16_t sim_body_count, sim_read_size;
static uint8_t sim_read_reason;
static uint8_t sim_body[1024];
static char sim_pending[400], sim_sent[128];
static int sim_silence_step, sim_error_step;
static unsigned sim_no_sim, sim_pin, sim_open_delay, sim_open_error, sim_rat_change;
static unsigned sim_port_change, sim_network_wait, sim_early_open, sim_trickle;
static unsigned sim_empty_urc, sim_pause, sim_send_sim2;
static const char *sim_imsi;
static unsigned sim_activation_delay;
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
static uint16_t sim_number(uint8_t field)
{
  return field == SLM_CFG_PORT ? test_config.port : field == SLM_CFG_TYPE
    ? test_config.modem_type : field == SLM_CFG_NETWORK ? test_config.network_type : 0;
}
static const slm_settings_t sim_settings = {sim_text, sim_number};

static void sim_gpio(uint8_t pin, bool high)
{
  CHECK((pin == 13 || pin == 14) && power_events < sizeof(power_trace) / sizeof(power_trace[0]));
  power_trace[power_events].time = sim_now;
  power_trace[power_events].pin = pin;
  power_trace[power_events++].high = (uint8_t)high;
}
static uint16_t sim_binary(const uint8_t *p, uint16_t n)
{
  CHECK(p >= rx && p < rx + sizeof(rx));
  if (sim_pause) return 0;
  CHECK(sim_body_count + n <= sizeof(sim_body));
  memcpy(sim_body + sim_body_count, p, n); sim_body_count = (uint16_t)(sim_body_count + n);
  return n;
}
static void sim_read_done(uint16_t n, uint8_t reason)
{
  CHECK(n <= sim_read_size); sim_read_reason = reason;
  ++sim_done_reads;
}
static void sim_type(uint8_t type) { CHECK(type == 2); ++sim_found; }
static const slm_handlers_t sim_handlers = {sim_gpio, sim_binary, sim_read_done, sim_type};

static void sim_reset(void)
{
  modem_slm336_off();
  memset(&slm_diag, 0, sizeof(slm_diag));
  memset(&slm_status, 0, sizeof(slm_status));
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
  sim_send_sim2 = 1; sim_open_delay = 500; sim_activation_delay = 700;
  sim_imsi = "250011234567890\r\nOK\r\n"; sim_open_pending = false; power_events = 0;
}
static void sim_feed(const char *s)
{
  unsigned n = (unsigned)strlen(s);
  CHECK(n < sizeof(rx));
  for (unsigned i = 0; i < n; ++i) interrupt_byte((uint8_t)s[i], USART_SR_RXNE);
}
static void sim_reply(const char *s, unsigned delay)
{
  CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));
  memcpy(sim_pending, s, strlen(s) + 1u); reply_due = sim_now + delay;
}
static void sim_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE)) return;
  CHECK(tx_data == command && tx_count <= sizeof(sim_sent));
  memcpy(sim_sent, command, tx_count); sim_sent[tx_count - 1u] = 0;
  /* IRQ передаёт из единственного статического буфера AT, без ожидания CPU. */
  for (unsigned i = 0; i < tx_count; ++i) interrupt_byte(0, USART_SR_TXE);
  interrupt_byte(0, USART_SR_TC);
  ++sim_commands;
  unsigned step = slm_status.step;
  if (!strcmp(sim_sent, "AT+CMEE?")) { sim_reply("OK\r\n+CMEE: 1\r\nOK\r\n", 30); return; }
  if ((int)step == sim_silence_step) return;
  if ((int)step == sim_error_step) { sim_reply("ERROR\r\n", 30); return; }
  if (!strcmp(sim_sent, "AT")) sim_reply("OK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+GMM")) { sim_gmm++; sim_reply("SLM336E\r\nOK\r\n", 30); }
  else if (!strcmp(sim_sent, "AT+CPIN?")) {
    if (sim_no_sim) sim_reply("+SIM1 NOT INSERT\r\n+SIM2 NOT INSERT\r\n+CME ERROR: 10\r\n", 100);
    else if (sim_pin) sim_reply("+CPIN: SIM PIN\r\nOK\r\n", 100);
    else sim_reply(sim_send_sim2 ? "+SIM2 NOT INSERT\r\n+CPIN: READY\r\nOK\r\n" : "+CPIN: READY\r\nOK\r\n", 100);
  } else if (!strcmp(sim_sent, "AT+CREG?")) {
    sim_reply(sim_network_wait ? "+CREG: 0,2\r\nOK\r\n" : "+CREG: 0,1\r\nOK\r\n", 80);
  } else if (!strcmp(sim_sent, "AT+CEREG?")) {
    sim_reply(sim_network_wait ? "+CEREG: 0,2\r\nOK\r\n" : "+CEREG: 0,1\r\nOK\r\n", 80);
  } else if (!strcmp(sim_sent, "AT+CIMI")) sim_reply(sim_imsi, 30);
  else if (!strcmp(sim_sent, "AT+QURCCFG=\"urcport\""))
    sim_reply(sim_port_change ? "+QURCCFG: \"urcport\",\"usbat\"\r\nOK\r\n" : "+QURCCFG: \"urcport\",\"uart1\"\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+QCFG=\"nwscanmode\""))
    sim_reply(sim_rat_change ? "+QCFG: \"nwscanmode\",1\r\nOK\r\n" : "+QCFG: \"nwscanmode\",0\r\nOK\r\n", 30);
  else if (!strcmp(sim_sent, "AT+QIACT=1")) sim_reply("OK\r\n", sim_activation_delay);
  else if (!strncmp(sim_sent, "AT+QIOPEN=", 10)) {
    char expected[128];
    CHECK(snprintf(expected,sizeof(expected),"AT+QIOPEN=1,0,\"TCP\",\"%s\",%u,0,0",test_config.host,test_config.port) > 0);
    CHECK(!strcmp(sim_sent,expected));
    if (sim_early_open) sim_reply("+QIOPEN: 0,0\r\nOK\r\n", 30);
    else { sim_reply("OK\r\n", 30); sim_open_pending = true; sim_open_due = sim_now + sim_open_delay; }
  } else if (!strncmp(sim_sent, "AT+QIRD=", 8)) {
    sim_qird++;
    CHECK(sim_read_size && slm_capacity <= 256);
    if (sim_qird == 1) sim_reply("+QIRD: 4\r\nOK\r\n\r\n+QIURC: \"recv\",0\r\nOK\r\n", 20);
    else if (sim_qird == 2) sim_reply("+QIURC: \"recv\",0\r\n+QIRD: 3\r\nXYZ\r\nOK\r\n", 20);
    else if (sim_qird == 3 && sim_empty_urc) sim_reply("+QIRD: 0\r\n+QIURC: \"recv\",0\r\nOK\r\n", 20);
    else sim_reply("+QIRD: 0\r\nOK\r\n", 20);
  } else sim_reply("OK\r\n", 30);
}
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
  modem_slm336_poll(sim_now);
  sim_tx();
  sim_now += 10;
}
static void sim_until(uint32_t limit)
{
  unsigned watchdog = 0;
  while (slm_status.state == SLM_STARTING && watchdog++ < limit / 10u) sim_tick();
  CHECK(watchdog < limit / 10u);
}
static void sim_start(void) { CHECK(modem_slm336_start(&sim_settings, &sim_handlers, sim_now)); }
static void path_tests(void)
{
  sim_reset(); sim_start(); sim_until(15000);
  CHECK(modem_slm336_ready() && slm_status.reason == SLM_ERR_NONE && !sim_gmm);
  CHECK(sim_now >= 8000 && sim_now <= 11000 && sim_ticks > 800);
  CHECK(slm_status.completed_phases == 63 && !slm_diag.failure_count);
  CHECK((slm_diag.steps[SLM_ACTIVATE] * 10u) == 700);
  CHECK((slm_diag.steps[SLM_TCP] * 10u) == 500);
  CHECK(power_events == 5 && power_trace[0].pin == 13 && !power_trace[0].high);
  CHECK(power_trace[1].pin == 14 && power_trace[1].high);
  CHECK(power_trace[2].pin == 13 && power_trace[2].time == 2100 && power_trace[2].high);
  CHECK(power_trace[3].pin == 14 && power_trace[3].time == 3000 && !power_trace[3].high);
  CHECK(power_trace[4].time == 5000 && power_trace[4].high);
  printf("SLM normal model: %u ms incl discharge, %u ms from PC13 on, %u polls\n",sim_now,sim_now-power_trace[2].time,sim_ticks);
  CHECK(!strcmp(slm_apn(), "internet.mts.ru"));
  CHECK(sim_commands < 25); /* Нет команд регистрации на сервере. */
  sim_reset(); test_config.modem_type = 255; sim_start(); sim_until(15000);
  CHECK(modem_slm336_ready() && sim_gmm == 1 && sim_found == 1);
  sim_reset(); sim_port_change = sim_rat_change = sim_early_open = 1;
  memcpy(test_config.apn, "manual.apn", sizeof("manual.apn")); sim_start(); sim_until(15000);
  CHECK(modem_slm336_ready() && !strcmp(slm_apn(),"manual.apn"));
  CHECK(slm_status.completed_steps & ((uint32_t)1u << SLM_URC_SET));
  CHECK(slm_status.completed_steps & ((uint32_t)1u << SLM_RAT_SET));
  sim_reset(); sim_trickle = 1; sim_start(); sim_until(20000); CHECK(modem_slm336_ready());
}
static void error_tests(void)
{
  sim_reset(); sim_no_sim = 1; sim_start(); sim_until(15000);
  CHECK(!modem_slm336_ready() && slm_status.reason == SLM_ERR_NO_SIM && slm_status.step == SLM_SIM);
  CHECK(slm_diag.failure_count == 1 && slm_diag.failure_step[0] == SLM_SIM);
  sim_reset(); sim_pin = 1; sim_start(); sim_until(15000); CHECK(slm_status.reason == SLM_ERR_PIN);
  sim_reset(); sim_error_step = SLM_ACTIVATE; sim_start(); sim_until(15000);
  CHECK(slm_status.reason == SLM_ERR_PDP && slm_status.step == SLM_ACTIVATE);
  sim_reset(); sim_open_delay = 149000; sim_start(); sim_until(170000);
  CHECK(modem_slm336_ready() && (slm_diag.steps[SLM_TCP] * 10u) == 149000);
  sim_reset(); sim_open_delay = 151000; sim_start(); sim_until(170000);
  CHECK(slm_status.reason == SLM_ERR_TIMEOUT && slm_status.step == SLM_TCP);
  for (unsigned i = 0; i < 200; ++i) sim_tick();
  CHECK(!modem_slm336_ready() && slm_status.state == SLM_FAILED);
  sim_reset(); sim_open_error = 1; sim_start(); sim_until(15000); CHECK(slm_status.reason == SLM_ERR_TCP && slm_status.modem_code == 566);
  sim_reset(); sim_network_wait = 1; sim_start(); sim_until(80000); CHECK(slm_status.reason == SLM_ERR_TIMEOUT && slm_status.step == SLM_NETWORK);
}
static void timeout_step_tests(void)
{
  for (unsigned step = SLM_AT_READY; step <= SLM_ACTIVATE; ++step) {
    sim_reset();
    test_config.modem_type = 255; sim_port_change = sim_rat_change = 1;
    sim_silence_step = (int)step; sim_start(); sim_until(180000);
    CHECK(slm_status.state == SLM_FAILED && slm_status.reason == SLM_ERR_TIMEOUT);
    CHECK(slm_status.step == step && slm_diag.failure_step[0] == step);
    CHECK((slm_diag.steps[step] * 10u) > 0);
  }
  /* Питание: скачки тиков, wrap и точные границы без сжатия пауз. */
  sim_reset(); sim_now = UINT32_MAX - 999u; sim_start();
  sim_now += 1999u; modem_slm336_poll(sim_now); CHECK(slm_status.step == SLM_DISCHARGE);
  ++sim_now; modem_slm336_poll(sim_now); CHECK(slm_status.step == SLM_POWER_LEAD);
  sim_now += 100000u; modem_slm336_poll(sim_now); CHECK(slm_status.step == SLM_POWER_RAIL);
  sim_now += 899u; modem_slm336_poll(sim_now); CHECK(slm_status.step == SLM_POWER_RAIL);
  ++sim_now; modem_slm336_poll(sim_now); CHECK(slm_status.step == SLM_POWER_KEY);
}
static void read_tests(void)
{
  sim_reset(); sim_empty_urc = 1; sim_start();
  while (slm_status.step != SLM_TCP) sim_tick();
  sim_feed("+QIURC: \"recv\",0\r\n"); sim_until(15000);
  CHECK(modem_slm336_ready() && sim_qird == 0); /* Данные до TCP/вместимости не теряются. */
  uint16_t tcp_time = slm_diag.steps[SLM_TCP];
  sim_read_size = 64; modem_slm336_rx_capacity(sim_read_size);
  for (unsigned i = 0; i < 100; ++i) sim_tick();
  CHECK(sim_qird == 4 && sim_done_reads == 4 && sim_body_count == 7 && !sim_read_reason);
  CHECK(slm_diag.steps[SLM_TCP] == tcp_time);
  CHECK(!memcmp(sim_body, "OK\r\nXYZ", 7) && !comm_at_busy());
  sim_feed("+QIURC: \"recv\",1\r\n");
  for (unsigned i = 0; i < 10; ++i) sim_tick(); CHECK(sim_qird == 4);
  modem_slm336_rx_capacity(0); sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 10; ++i) sim_tick(); CHECK(sim_qird == 4);
  sim_reset(); sim_start(); sim_until(15000); sim_read_size = 64;
  modem_slm336_rx_capacity(64); sim_silence_step = SLM_READ; sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 110; ++i) sim_tick();
  CHECK(slm_status.reason == SLM_ERR_TIMEOUT && slm_status.step == SLM_READ && sim_read_reason == SLM_ERR_TIMEOUT);
}
static void edge_tests(void)
{
  sim_reset(); sim_imsi = "999011234567890\r\nOK\r\n"; sim_start(); sim_until(15000);
  CHECK(modem_slm336_ready() && !slm_apn()[0]);
  sim_reset(); sim_activation_delay = 149000; sim_start(); sim_until(180000);
  CHECK(modem_slm336_ready() && (slm_diag.steps[SLM_ACTIVATE] * 10u) == 149000);
  sim_reset(); memset(test_config.host,'h',sizeof(test_config.host)-1u);
  memset(test_config.apn,'a',sizeof(test_config.apn)-1u);
  memset(test_config.login,'l',sizeof(test_config.login)-1u);
  memset(test_config.password,'p',sizeof(test_config.password)-1u);
  sim_start();
  while (slm_status.step != SLM_TCP && slm_status.state == SLM_STARTING) sim_tick();
  CHECK(slm_status.step == SLM_TCP && slm_status.state == SLM_STARTING); /* Максимальный QICSGP помещается. */
  modem_slm336_off();
  sim_reset(); sim_start(); sim_until(15000);
  sim_feed("+QIURC: \"closed\",1\r\n"); sim_tick(); CHECK(modem_slm336_ready());
  sim_feed("+QIURC: \"closed\",0\r\n"); sim_tick(); CHECK(slm_status.reason == SLM_ERR_TCP);
  sim_reset(); sim_start(); sim_until(15000);
  sim_feed("+QIURC: \"pdpdeact\",1\r\n"); sim_tick(); CHECK(slm_status.reason == SLM_ERR_PDP);
  sim_reset(); sim_start(); sim_until(15000); sim_read_size = 64; modem_slm336_rx_capacity(64);
  sim_pause = 1; sim_feed("+QIURC: \"recv\",0\r\n");
  for (unsigned i = 0; i < 15; ++i) sim_tick();
  CHECK(!sim_done_reads && !sim_body_count && comm_at_busy());
  sim_pause = 0;
  for (unsigned i = 0; i < 30; ++i) sim_tick();
  CHECK(sim_qird == 3 && sim_body_count == 7 && sim_done_reads == 3);
  sim_reset(); sim_start(); sim_until(15000); sim_read_size = 64; modem_slm336_rx_capacity(64);
  sim_feed("+QIURC: \"recv\",0\r\n"); sim_tick();
  sim_pending[0] = 0; sim_feed("+QIRD: 4\r\nxy"); sim_tick();
  CHECK(sim_body_count == 2 && !sim_done_reads);
  interrupt_byte('z', USART_SR_FE); sim_tick();
  CHECK(slm_status.reason == SLM_ERR_STREAM && sim_done_reads == 1 && sim_read_reason == SLM_ERR_STREAM);
  CHECK(!modem_slm336_ready());
}
static void diagnostic_tests(void)
{
  sim_reset(); sim_start(); sim_until(15000);
  uint32_t tcp_ms = (slm_diag.steps[SLM_TCP] * 10u);
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  CHECK((slm_diag.steps[SLM_TCP] * 10u) == tcp_ms);
  CHECK(sizeof(slm_diag) <= 128);
  modem_slm336_off(); sim_no_sim = 1; sim_start(); sim_until(15000);
  CHECK(slm_diag.steps[SLM_SIM] == 10 && slm_status.reason == SLM_ERR_NO_SIM);
  CHECK(slm_diag.failure_count == 1 && slm_diag.failure_step[0] == SLM_SIM);
  CHECK(slm_diag.steps[SLM_ACTIVATE] == 0 && slm_diag.phases[SLM_PHASE_TCP] == 0);
  /* Десять сбоев, оборачивание кольца и сохранение между попытками. */
  for (unsigned i = 0; i < 12; ++i) {
    modem_slm336_off(); power_events = 0; sim_start(); sim_until(sim_now + 15000u);
  }
  CHECK(slm_diag.failure_count == 10 && slm_diag.failure_next == 3);
  for (unsigned i = 0; i < sizeof(slm_diag.failure_step); ++i)
    CHECK(slm_diag.failure_step[i] == SLM_SIM && slm_diag.failure_reason[i] == SLM_ERR_NO_SIM);
  sim_reset(); sim_start(); sim_now = 1500; modem_slm336_poll(sim_now);
  CHECK((slm_diag.steps[SLM_DISCHARGE] * 10u) == 1500 && (slm_diag.phases[SLM_PHASE_POWER] * 10u) == 1500);
  slm_now = slm_step_started + 1000000u; CHECK(slm_elapsed(slm_step_started) == UINT16_MAX);
}
static void settings_tests(void)
{
  sim_reset(); test_config.modem_type = 3; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  test_config.modem_type = 2; test_config.apn[0] = '"'; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  memset(test_config.apn,'A',sizeof(test_config.apn)); CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  sim_reset(); test_config.network_type = 5; CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  /* Геттеры читаются при составлении команды, копии настроек нет. */
  sim_reset(); sim_start();
  memcpy(test_config.apn, "changed.apn", sizeof("changed.apn"));
  sim_until(15000); CHECK(modem_slm336_ready() && !strcmp(slm_apn(), "changed.apn"));
  sim_reset(); sim_start(); CHECK(!modem_slm336_start(&sim_settings,&sim_handlers,0));
  sim_reset(); slm_settings = sim_settings;
  for (unsigned i = 0; i < 5; ++i) {
    static const uint8_t mncs[] = {1,2,99,20,11};
    static const char *expected[] = {"internet.mts.ru","internet","internet.beeline.ru","internet.tele2.ru","internet.yota"};
    slm_mnc = mncs[i]; CHECK(!strcmp(slm_apn(),expected[i]));
  }
  slm_mnc = 88; CHECK(!slm_apn()[0]);
}
int main(void)
{
  path_tests(); error_tests(); timeout_step_tests(); read_tests(); edge_tests(); diagnostic_tests(); settings_tests();
  puts("comm_slm: power/setup/SIM1/SIM2/network/APN/QIACT/late QIOPEN/QIRD/timeouts/timing PASS");
  return 0;
}
