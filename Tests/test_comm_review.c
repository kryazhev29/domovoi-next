/* Доработка: Кряжев А., 2026. Регрессии пяти дефектов новой связи. */
static void review_parser_fault(void)
{
  for (unsigned uart_error = 0; uart_error < 2; ++uart_error) {
    rc_online(); prompt_silence = 1;
    uint16_t capacity;
    uint8_t *p = comm_tx_acquire(&capacity);
    CHECK(p && capacity); p[0] = 1; CHECK(comm_tx_commit(1));
    for (unsigned i = 0; ch.operation != OP_PROMPT && i < 100; ++i) rc_tick(10);
    CHECK(ch.operation == OP_PROMPT && state == RUN);
    if (uart_error) interrupt_byte(0, USART_SR_ORE);
    else sim_feed("+QIRD: 65535\r\n");
    prompt_silence = 0;
    rc_run(30000);
    CHECK(comm_server_online() && rc_power_starts == 1);
  }
}
/* Незавершённый URC перед QICLOSE не должен блокировать всё восстановление. */
static void review_idle_fragment(void)
{
  rc_online();
  CHECK(state == IDLE);
  comm_channel_reopen(0);
  sim_feed("\r");
  rc_run(30000);
  CHECK(comm_server_online() && rc_power_starts == 1);
}
static void review_error_windows(void)
{
  rc_online();
  comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000);
  comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(65537000u);
  comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(100);
  CHECK(!rc_logged && comm_server_online());
  /* Exact boundary and millis rollover, including a timestamp of zero. */
  uint32_t times[3]; uint8_t index = 0, count = 0;
  rc.now = UINT32_MAX - 100u;
  CHECK(!rc_burst(times, 3, &index, &count, COMM_T_EARLY_AT));
  rc.now = 0;
  CHECK(!rc_burst(times, 3, &index, &count, COMM_T_EARLY_AT));
  rc.now = 100;
  CHECK(rc_burst(times, 3, &index, &count, COMM_T_EARLY_AT));
  rc.now = 1000;
  CHECK(!rc_burst(times, 3, &index, &count, COMM_T_EARLY_AT));
  rc.now += comm_timeouts[COMM_T_EARLY_AT];
  CHECK(!rc_burst(times, 3, &index, &count, COMM_T_EARLY_AT));
  CHECK(count == 1);
}
static void review_settings(void)
{
  rc_online();
  strcpy(config.server1, "test.local:7777");
  strcpy(config.server2, "backup.local:8888");
  config.useServer1 = 1; config.useServer2 = 0;
  config.netType = 0; device.moduleType = 2;
  slm.settings.text = comm_text; slm.settings.number = comm_number;
  strcpy(config.netOperator, "private.apn,user,password");
  char cmd[128];
  const char *apn = slm.profile->commands + slm.profile->steps[SLM_APN - SLM_AT_READY].command;
  comm_modem_build(cmd, sizeof(cmd), apn, NULL, 0);
  CHECK(!strcmp(cmd, "AT+QICSGP=1,1,\"private.apn\",\"user\",\"password\",1"));
  strcpy(config.netOperator, "private.apn");
  comm_modem_build(cmd, sizeof(cmd), apn, NULL, 0);
  CHECK(!strcmp(cmd, "AT+QICSGP=1,1,\"private.apn\",\"\",\"\",0"));
  config.netOperator[0] = 0; slm.attempt.mnc = 1;
  comm_modem_build(cmd, sizeof(cmd), apn, NULL, 0);
  CHECK(strstr(cmd, "internet.mts.ru"));
  strcpy(config.netOperator, "bad\"apn"); CHECK(!slm_valid_settings());
  config.netOperator[0] = 0;
  static const uint8_t expected[] = {0, 1, 0, 5, 3};
  const char *rat = slm.profile->commands + slm.profile->steps[SLM_RAT_SET - SLM_AT_READY].command;
  for (unsigned i = 0; i < sizeof(expected); ++i) {
    config.netType = (uint8_t)i;
    comm_modem_build(cmd, sizeof(cmd), rat, NULL, 0);
    char want[40]; snprintf(want, sizeof(want), "AT+QCFG=\"nwscanmode\",%u,1", expected[i]);
    CHECK(!strcmp(cmd, want) && slm_valid_settings());
  }
  /* A settings update must change the next real QIOPEN without a MCU reboot. */
  comm_split(0, "old.local:1234"); comm_split(1, "old-backup.local:1235");
  uint16_t capacity;
  const char *host = comm_text(SLM_CFG_HOST, &capacity);
  comm_modem_build(cmd, sizeof(cmd), slm.profile->commands + slm.profile->tcp, host, comm_number(SLM_CFG_PORT));
  CHECK(strstr(cmd, "test.local\",7777"));
  strcpy(config.server1, "new.local:9999");
  host = comm_text(SLM_CFG_HOST, &capacity);
  comm_modem_build(cmd, sizeof(cmd), slm.profile->commands + slm.profile->tcp, host, comm_number(SLM_CFG_PORT));
  CHECK(strstr(cmd, "new.local\",9999"));
  config.useServer1 = 0; config.useServer2 = 1;
  CHECK(!strcmp(comm_text(SLM_CFG_HOST, &capacity), "backup.local") && comm_number(SLM_CFG_PORT) == 8888);
  strcpy(config.server2, "next-backup.local:7778");
  CHECK(!strcmp(comm_text(SLM_CFG_HOST2, &capacity), "next-backup.local") && comm_number(SLM_CFG_PORT2) == 7778);
}
int main(void)
{
  (void)original_regressions;
  review_parser_fault(); review_idle_fragment(); review_error_windows(); review_settings();
  puts("comm_review: active/idle parser and UART faults recover; time windows; manual APN; RAT; live endpoints PASS");
  return 0;
}
