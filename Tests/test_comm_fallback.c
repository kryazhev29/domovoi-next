/* LTE без данных: реальные автоматы UART/AT/TCP/сервера, время и сеть моделируются. */
static void fallback_start(unsigned mode)
{
  rc_online(); test_config.fallback = 1; rc_keep_opens = true;
  CHECK(comm_modem_lte());
  if (mode == 0) { reserve_enabled = 1; sim_open_error = 1; }
  if (mode == 1 || mode == 3) drop_register = 1;
  if (mode == 3) rc.first = 0; /* прежний однократный restart первой регистрации */
  if (mode == 2) { drop_status = 1; tx_unacked = 2; }
  comm_channel_reopen(0);
}

/* Каждая причина достигает второго LTE-сеанса; повтор того же сеанса не считается. */
static void fallback_failures(void)
{
  for (unsigned mode = 0; mode < 4; ++mode) {
    fallback_start(mode);
    unsigned before = opens;
    while (comm_recover_network(0) == 0) {
      CHECK(sim_now < 350000u);
      rc_tick(10);
      CHECK(opens <= before + 2u);
    }
    CHECK(opens == before + 2u && rc_power_starts == (mode == 3 ? 2u : 1u) && test_config.network_type == 0);
    CHECK(rc.rat == RAT_GSM && comm_recover_network(0) == 1);
    printf("LTE fallback cause=%u: second session, power starts=%u PASS\n", mode, rc_power_starts);
    /* Действительный новый запуск получает nwscanmode=1. */
    sim_open_error = drop_register = drop_status = tx_unacked = 0;
    sim_network_wait = 3; call_cereg_model = 0; /* GSM: только CGREG зарегистрирован. */
    rc_run(20000);
    printf("GSM after fallback: modem=%u step=%u net=%u server=%u RAT=%u opens=%u\n",
      comm_modem_state(), slm.attempt.status.step, slm.attempt.net_registered, sv.state, rc.rat, opens);
    CHECK(comm_server_online() && comm_recover_network(0) == 1 && !comm_modem_lte());
  }
}

static void fallback_success(void)
{
  fallback_start(0);
  for (unsigned i = 0; rc.rat == RAT_AUTO; ++i) { CHECK(i < 500); rc_tick(10); }
  CHECK(rc.rat == RAT_FAILED);
  sim_open_error = 0; drop_status = 1;
  for (unsigned i = 0; !comm_server_online(); ++i) { CHECK(i < 500); rc_tick(10); }
  CHECK(rc.rat == RAT_FAILED); /* Одна регистрация ещё не подтверждает статус. */
  drop_status = 0;
  rc_run(20000);
  CHECK(rc.rat == RAT_AUTO && !rc_power_starts && comm_server_online());
  unsigned before = rc_power_starts;
  /* После подтверждённого статуса зависший журнал не превращается в отказ первого статуса. */
  drop_status = 1; tx_unacked = 1;
  while (comm_server_online()) rc_tick(10);
  CHECK(rc.rat == RAT_AUTO && rc_power_starts == before);
  puts("LTE success: registration alone keeps count; status clears it; later ACK stall ignored PASS");
}

static void fallback_modes(void)
{
  static const uint8_t modes[] = {0,1,0,5,3};
  for (unsigned raw = 1; raw < 5; ++raw) {
    fallback_start(0); test_config.fallback = 0; test_config.network_type = modes[raw];
    rc_run(15000);
    CHECK(!rc_power_starts && rc.rat == RAT_AUTO);
    CHECK(comm_recover_network((uint8_t)raw) == modes[raw]);
  }
  fallback_start(0); slm.attempt.net_registered &= (uint8_t)~NET_EPS;
  sim_network_wait = 3; call_cereg_model = 0;
  rc_run(15000); CHECK(!rc_power_starts && rc.rat == RAT_AUTO);
  fallback_start(0); slm.attempt.status.state = SLM_STARTING;
  comm_recover_data(COMM_DATA_BEGIN); comm_recover_data(COMM_DATA_FAILED);
  comm_recover_data(COMM_DATA_BEGIN); comm_recover_data(COMM_DATA_FAILED);
  CHECK(rc.rat == RAT_AUTO);
  fallback_start(0); ch.wifi = 1;
  comm_recover_data(COMM_DATA_BEGIN); comm_recover_data(COMM_DATA_FAILED);
  comm_recover_data(COMM_DATA_BEGIN); comm_recover_data(COMM_DATA_FAILED);
  CHECK(rc.rat == RAT_AUTO); ch.wifi = 0;
  puts("LTE fallback guards: explicit modes, GSM, PDP not ready and Wi-Fi PASS");
}

static void fallback_day(void)
{
  /* Обычная шкала и сутки через UINT32_MAX. RTC специально не участвует. */
  for (unsigned wrap = 0; wrap < 2; ++wrap) {
    fallback_start(0);
    while (comm_recover_network(0) == 0) rc_tick(10);
    uint32_t start = wrap ? UINT32_MAX - 1000u : sim_now;
    rc.rat_since = start; rc_clock = 0; rc_busy_flag = false;
    unsigned session = slm.session;
    /* Остальные восстановления выключены состоянием SLM_STARTING; проверяем именно срок RAT. */
    slm.attempt.status.state = SLM_STARTING;
    comm_recover_poll(start + 86400000u - 1u);
    CHECK(slm.session == session && comm_recover_network(0) == 1);
    test_config.fallback = 0; test_config.network_type = 3;
    comm_recover_poll(start + 86400000u);
    CHECK(slm.session == session && sim_number(SLM_CFG_NETWORK) == 3);
    test_config.fallback = 1;
    comm_recover_poll(start + 86400000u);
    CHECK(slm.session == session + 1u && comm_recover_network(0) == 0);
    CHECK(rc.rat == RAT_PROBE);
    slm.attempt.status.state = SLM_READY; slm.attempt.net_registered = NET_EPS;
    comm_recover_data(COMM_DATA_BEGIN); comm_recover_data(COMM_DATA_FAILED);
    CHECK(comm_recover_network(0) == 1); /* После суток достаточно одной неудачи. */
    /* Явный режим выше временного, повторная синхронизация авто сохраняет срок. */
    uint32_t since = rc.rat_since;
    CHECK(comm_recover_network(4) == 3 && comm_recover_network(1) == 1 && comm_recover_network(2) == 0);
    CHECK(comm_recover_network(0) == 1 && rc.rat_since == since);
    comm_recover_init(&rc_hooks, start);
    CHECK(comm_recover_network(0) == 0);
  }
  rc_keep_opens = false;
  puts("LTE fallback: exact 24h, millis wrap, one auto retry, config priority, boot reset PASS");
}

static void fallback_tests(void)
{
  /* Переписанное общее ожидание: до/в срок через оборот millis, фактический restart один. */
  rc_online(); rc.now = UINT32_MAX - 500u; rc_schedule(true, 10000u);
  uint32_t start = rc.now, left; unsigned session = slm.session;
  CHECK(comm_recover_link(start + 9999u, &left) == LINK_RESTART_WAIT && left == 1u);
  comm_recover_poll(start + 9999u); CHECK(slm.session == session);
  comm_recover_poll(start + 10000u); CHECK(slm.session == session + 1u);
  fallback_failures(); fallback_success(); fallback_modes(); fallback_day();
}
