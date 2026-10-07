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
  CHECK(sim_now >= 8000 && sim_now <= 11000 && sim_ticks > 800);
  DIAG_CHECK(slm.attempt.status.completed_phases == 63); DIAG_CHECK(!slm.diag.failure_count);
  DIAG_CHECK((slm.diag.steps[SLM_ACTIVATE] * 10u) == 700);
  DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == 500);
  CHECK(power_events == 5 && power_trace[0].pin == 13 && !power_trace[0].high);
  CHECK(power_trace[1].pin == 14 && power_trace[1].high);
  CHECK(power_trace[2].pin == 13 && power_trace[2].time == 2100 && power_trace[2].high);
  CHECK(power_trace[3].pin == 14 && power_trace[3].time == 3000 && !power_trace[3].high);
  CHECK(power_trace[4].time == 5000 && power_trace[4].high);
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
  DIAG_CHECK(slm.diag.steps[SLM_TCP] == 25 && slm.diag.phases[SLM_PHASE_TCP] == 25);
  channel_until(15000);
  CHECK(channel_opened == 1); DIAG_CHECK(slm.diag.steps[SLM_TCP] == 50);
  channel_model_reset();  channel_until(15000);
#if DOMOVOI_DIAG
  uint32_t tcp_ms = (slm.diag.steps[SLM_TCP] * 10u);
#endif
  for (unsigned i = 0; i < 20; ++i) sim_tick();
  DIAG_CHECK((slm.diag.steps[SLM_TCP] * 10u) == tcp_ms);
}
// Запустить проверки путей и отказов канала.
static void channel_regressions(void)
{
  channel_path_tests(); channel_error_tests(); channel_read_tests(); channel_edge_tests(); channel_diagnostic_tests();
  comm_modem_transport(NULL); sim_poll = comm_modem_poll;
  puts("comm_channel: original TCP/READ/late URC/capacity/backpressure/timing PASS");
}
