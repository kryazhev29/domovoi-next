/* Доработка: Кряжев А., 2026. Восстановление связи (этап 5) на имитаторе SLM336:
   решения модуля (пауза, со снятием питания или с AT) и сами перезапуски,
   модельное время без ожиданий. Подключается в сборку comm_server после его тестов. */
static struct { uint32_t at, ms; uint8_t power; unsigned opens; } rc_log[16];
static unsigned rc_logged, rc_power_starts, rc_soft_starts;
static uint32_t rc_last_since, rc_last_attempt, rc_clock = UINT32_MAX;
static bool rc_busy_flag;
static void rc_session(void) { comm_server_init(&test_server_handlers); comm_channel_receive(user_comm_rx, user_comm_rx_done); }
static uint32_t rc_time(void) { return rc_clock == UINT32_MAX ? UINT32_MAX : rc_clock + sim_now / 1000u; }
static bool rc_busy(void) { return rc_busy_flag; }
static const comm_recover_hooks_t rc_hooks = {rc_session, rc_time, rc_busy};
/* Шаг модели: связь, восстановление, учёт решений и перезапусков. */
static void rc_note(void);
static void rc_tick(unsigned ms)
{
  power_events = 0; /* в трассе питания 16 мест, здесь считаем перезапуски сами */
  transport_tick(ms);
  comm_info_poll(sim_now);
  rc_note(); /* решение могло прийти из сервера (нет интернета) и исполниться в этом же такте */
  comm_recover_poll(sim_now);
  rc_note();
}
static void rc_note(void)
{
  if (rc.state == RC_WAIT && rc.wait_since != rc_last_since) {
    rc_last_since = rc.wait_since;
    if (rc_logged < sizeof(rc_log) / sizeof(rc_log[0])) { rc_log[rc_logged].at = sim_now; rc_log[rc_logged].ms = rc.wait_ms; rc_log[rc_logged].power = rc.power; rc_log[rc_logged].opens = opens; }
    ++rc_logged;
  }
  if (opens > 8) opens = 0; /* учёт модели, не поведение */
  if (slm.attempt.attempt_started != rc_last_attempt) {
    rc_last_attempt = slm.attempt.attempt_started;
    socket_model = false; /* новая попытка модема: старого сокета нет */
    if (slm.attempt.status.step == SLM_DISCHARGE || slm.attempt.status.phase == SLM_PHASE_POWER) ++rc_power_starts; else ++rc_soft_starts;
  }
}
/* Шаг 10 мс первую минуту, дальше 100 мс (ответы AT успевают), в паузе восстановления 1 с. */
static void rc_run(uint32_t ms) { uint32_t begin = sim_now; while (sim_now - begin < ms) rc_tick(rc.state == RC_WAIT ? 1000 : sim_now - begin < 60000u ? 10 : 100); }
/* Плата на связи, восстановление включено; дальше сценарий задаёт сбой. */
static void rc_online(void)
{
  server_reset();
  comm_server_no_internet(comm_recover_no_internet);
  comm_recover_init(&rc_hooks, sim_now);
  rc_logged = rc_power_starts = rc_soft_starts = 0; rc_last_since = UINT32_MAX;
  rc_last_attempt = slm.attempt.attempt_started; rc_clock = UINT32_MAX; rc_busy_flag = false;
}
/* Перезапуск со снятием питания, как после сбоя: дальше модем идёт уже с флагами сценария. */
static void rc_fresh(void) { CHECK(comm_modem_restart(true, sim_now)); rc_session(); }
static void rc_expect(unsigned i, uint32_t ms, uint8_t power)
{
  CHECK(i < rc_logged && rc_log[i].ms == ms && rc_log[i].power == power);
}
/* Сигнал, регистрация, тип сети: опрос через канал раз в 30 с, ERROR на запрос не рвёт TCP,
   без связи - «неизвестно». */
static unsigned low_signals; static uint8_t low_csq;
static void low_model(uint8_t csq) { ++low_signals; low_csq = csq; }
static void info_tests(void)
{
  rc_online(); rc_run(40000u);
  CHECK(comm_info()->csq == 22 && comm_info()->creg == 1 && comm_info()->cgreg == 1 && comm_info()->net == 4);
  { unsigned before = opens; sim_nwinfo_error = 1; rc_run(70000u); CHECK(opens == before && comm_server_online()); }
  CHECK(comm_info()->csq == 22);
  comm_modem_off(); rc_run(100); CHECK(comm_info()->csq == 99 && !comm_info()->creg && !comm_info()->net);
  /* Событие 38: сигнал ниже 15 - после первого полного опроса, дальше раз в сутки. */
  /* Время модели после rc_online идёт с начала, срок опроса - из прошлого сценария: 2 мин на свежий цикл. */
  rc_online(); sim_csq = 12; rc_run(120000u); comm_info_low_signal(low_model); rc_run(40000u);
  CHECK(low_signals == 1 && low_csq == 12);
  rc_run(3600000u); CHECK(low_signals == 1);
  rc_run(23u * 3600000u + 60000u); CHECK(low_signals == 2);
  static const unsigned levels[] = {15, 99, 14};
  for (unsigned i = 0; i < 3; ++i) { sim_csq = levels[i]; rc_run(40000u); comm_info_low_signal(low_model); rc_run(40000u); }
  CHECK(low_signals == 3 && low_csq == 14);
  sim_csq = 22;
}
static void recover_tests(void)
{
  info_tests();
  /* 1. Нет SIM час: только снятие питания, паузы 10/30 мин, AT-перезапусков нет. */
  rc_online(); sim_no_sim = 1; rc_fresh(); rc_run(3600000u);
  CHECK(rc_logged == 3 && !rc_soft_starts && rc_power_starts == 3);
  rc_expect(0, 600000u, 1); rc_expect(1, 1800000u, 1); rc_expect(2, 3600000u, 1);
  CHECK(link_count(COMM_LINK_NO_NET, 0) == 1 && !link_count(COMM_LINK_NO_INTERNET, 0)); /* событие 11 один раз */
  /* 2. Нет сети: с AT 10 с, 30 с, третья неудача подряд - питание 10 мин, дальше 60 с. */
  rc_online(); sim_network_wait = 1; rc_fresh(); rc_run(3600000u);
  rc_expect(0, 10000u, 0); rc_expect(1, 30000u, 0); rc_expect(2, 600000u, 1);
  rc_expect(3, 60000u, 0); rc_expect(4, 60000u, 0); rc_expect(5, 1800000u, 1);
  CHECK(link_count(COMM_LINK_NO_NET, 0) == 1);
  /* Сеть нашлась, связь есть - при следующей потере сети событие 11 снова. */
  sim_network_wait = 0; rc_run(3600000u); CHECK(comm_server_online());
  sim_network_wait = 1; rc_fresh(); rc_run(150000u); CHECK(link_count(COMM_LINK_NO_NET, 0) == 2);
  /* Мобильный интернет не включается (QIACT): событие 12 один раз, 11 нет. */
  rc_online(); sim_error_step = SLM_ACTIVATE; rc_fresh(); rc_run(3600000u);
  CHECK(rc_logged >= 3 && link_count(COMM_LINK_NO_INTERNET, 0) == 1 && !link_count(COMM_LINK_NO_NET, 0));
  /* 3. Модем молчит после включения: питание 10/30/60 мин. */
  rc_online(); sim_silence_step = SLM_AT_READY; rc_fresh(); rc_run(3 * 3600000u);
  rc_expect(0, 600000u, 1); rc_expect(1, 1800000u, 1); rc_expect(2, 3600000u, 1); CHECK(!rc_soft_starts);
  /* 4. Модем замолчал посреди работы: питание сразу, затем по паузам. */
  rc_online();
  { unsigned before = rc_power_starts; /* длинная строка - движок AT в сбое, канал в CH_FAULT */
    for (unsigned i = 0; i < 240; ++i) interrupt_byte('Y', USART_SR_RXNE); sim_feed("\r\n");
    rc_run(200); CHECK(rc_logged >= 1 && rc_log[0].ms == 0 && rc_log[0].power); CHECK(rc_power_starts == before + 1u); }
  rc_run(30000u); CHECK(comm_server_online());
  /* 5. Потеря мобильного интернета: модем заново с AT через 10 с, без питания, снова на связи. */
  rc_online(); sim_feed("+QIURC: \"pdpdeact\",1\r\n"); rc_run(60000u);
  rc_expect(0, 10000u, 0); CHECK(!rc_power_starts && rc_soft_starts == 1 && comm_server_online());
  /* QIDEACT ответил ошибкой (контекста нет) - шаг пройден, модем снова на связи. */
  rc_online(); sim_error_step = SLM_DEACT; sim_feed("+QIURC: \"pdpdeact\",1\r\n"); rc_run(60000u);
  rc_expect(0, 10000u, 0); CHECK(rc_soft_starts == 1 && comm_server_online() && rc_logged == 1);
  /* 6. Сервер молчит, интернет есть: событие 13 и переоткрытие TCP, модем не трогается. */
  rc_online(); drop_status = 1; rc_run(600000u);
  /* Событие 13 - на каждое переподключение: регистрация снимает его флаг (этап 4). */
  CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts && event13 >= 1 && opens >= 2);
  /* 7. Нет интернета (ping и DNS молчат): TCP заново, модем с AT (QIDEACT/QIACT),
        потом питание сразу, дальше 10 мин. */
  rc_online(); drop_status = 1; ping_success = dns_success = 0; rc_run(1800000u);
  CHECK(rc_logged >= 3 && rc_soft_starts == 1 && rc_power_starts >= 1);
  rc_expect(0, 0, 0); rc_expect(1, 0, 1); rc_expect(2, 600000u, 1);
  CHECK(rc_log[0].opens >= 2); /* до перезапуска модема TCP переоткрыт */
  CHECK(link_count(COMM_LINK_NO_INTERNET, 0) >= 1 && !link_count(COMM_LINK_NO_NET, 0)); /* ping и DNS молчат - событие 12 */
  /* 8. Ранний перезапуск: 3 таймаута AT в работе за 10 мин - питание сразу; 2 - нет. */
  rc_online();
  comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000);
  CHECK(!rc_logged);
  comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(100); rc_expect(0, 0, 1);
  rc_online();  /* те же три, но растянуты больше чем на 10 мин - не считаются пачкой */
  for (int i = 0; i < 3; ++i) { comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(330000u); }
  CHECK(!rc_logged);
  /* Таймауты при запуске модема (нет сети) не копятся в пачку с таймаутом в работе. */
  rc_online(); sim_network_wait = 1; rc_fresh(); rc_run(150000u); CHECK(rc_logged == 2);
  sim_network_wait = 0; rc_run(120000u); CHECK(comm_server_online());
  { unsigned before = rc_logged; comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000); CHECK(rc_logged == before); }
  /* 5 неудач TCP за час - питание сразу. */
  rc_online(); for (int i = 0; i < 4; ++i) { comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TCP, 566); rc_run(1000); }
  CHECK(!rc_logged); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TCP, 566); rc_run(100); rc_expect(0, 0, 1);
  /* 9. Ночь: в 03:00 модем перезапускается раз; занят - ждёт; без часов - раз в сутки работы. */
  rc_online(); rc_clock = 1790899200u + 2u * 3600u + 3590u; /* 02:59:50 по модели */
  rc_busy_flag = true; rc_run(60000u); CHECK(!rc_logged);
  rc_busy_flag = false; rc_run(60000u); CHECK(rc_logged == 1); rc_expect(0, 0, 1);
  rc_run(1800000u); CHECK(rc_logged == 1 && comm_server_online());  /* в то же окно второй раз нет */
  rc_online(); rc_run(23u * 3600000u); CHECK(!rc_logged); rc_run(3600000u + 60000u); CHECK(rc_logged == 1); rc_expect(0, 0, 1);
  /* 10 мин устойчивой связи - ступени с начала. */
  rc_online(); sim_no_sim = 1; rc_fresh(); rc_run(700000u); CHECK(rc.power_step == 2);
  sim_no_sim = 0; rc_run(1800000u + 700000u); CHECK(comm_server_online() && rc.power_step == 0);
  printf("comm_recover: no SIM, no network, silent modem, silence in work, PDP loss, silent server, no internet, early AT/TCP, night/day restart, ladder reset PASS\n");
}
