/* Доработка: Кряжев А., 2026. Восстановление связи (этап 5) на имитаторе SLM336:
   решения модуля (пауза, со снятием питания или с AT) и сами перезапуски,
   модельное время без ожиданий. Подключается в сборку comm_server после его тестов. */
static struct { uint32_t at, ms; uint8_t power; unsigned opens; } rc_log[16];
static unsigned rc_logged, rc_power_starts, rc_soft_starts, rc_session_resets;
static uint32_t rc_last_since, rc_last_attempt, rc_clock = UINT32_MAX;
static bool rc_busy_flag, rc_keep_opens;
// Запомнить новый сеанс восстановления.
static void rc_session(void) { ++rc_session_resets; socket_model=false; comm_server_init(&test_server_handlers); comm_channel_receive(user_comm_rx, user_comm_rx_done); }
// Выдать моделируемое время восстановления.
static uint32_t rc_time(void) { return rc_clock == UINT32_MAX ? UINT32_MAX : rc_clock + sim_now / 1000u; }
// Сообщить о занятости вызовом или приложением.
static bool rc_busy(void) { return rc_busy_flag; }
static const comm_recover_hooks_t rc_hooks = {rc_session, rc_time, rc_busy};
/* Шаг модели: связь, восстановление, учёт решений и перезапусков. */
static void rc_note(void);
// Продвинуть модем и восстановление на один шаг.
static void rc_tick(unsigned ms)
{
  power_events = 0; /* в трассе питания 16 мест, здесь считаем перезапуски сами */
  transport_tick(ms);
  comm_info_poll(sim_now);
  rc_note(); /* решение могло прийти из сервера (нет интернета) и исполниться в этом же такте */
  unsigned session_before = slm.session, resets_before = rc_session_resets;
  comm_recover_poll(sim_now);
  if (rc.state == RC_WAIT) CHECK(sim_power_on && comm_power_v5_state() == COMM_PWR_READY);
  if (slm.session != session_before) CHECK(rc_session_resets == resets_before + 1u);
  rc_note();
}
// Запомнить событие восстановления.
static void rc_note(void)
{
  if (rc.state == RC_WAIT && rc.power != RC_QPOWD && rc.wait_until != rc_last_since) { /* срок QPOWD - не новое решение */
    rc_last_since = rc.wait_until;
    if (rc_logged < sizeof(rc_log) / sizeof(rc_log[0])) { rc_log[rc_logged].at = sim_now; rc_log[rc_logged].ms = rc_left(rc.now); rc_log[rc_logged].power = rc.power; rc_log[rc_logged].opens = opens; }
    ++rc_logged;
  }
  if (!rc_keep_opens && opens > 8) opens = 0; /* учёт модели, не поведение */
  if (slm.session != rc_last_attempt) {
    rc_last_attempt = slm.session;
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
  comm_recover_init(&rc_hooks, sim_now); rc.first = 1; /* плата уже была на связи после старта */
  rc_logged = rc_power_starts = rc_soft_starts = qpowd_count = 0; rc_last_since = UINT32_MAX;
  rc_last_attempt = slm.session; rc_clock = UINT32_MAX; rc_busy_flag = false;
}
/* Перезапуск со снятием питания, как после сбоя: дальше модем идёт уже с флагами сценария. */
static void rc_fresh(void) { CHECK(comm_modem_restart(true, sim_now)); rc_session(); }
/* Дойти настоящими отказами до первой длинной паузы при включённом модеме. */
static void rc_wait_power(void)
{
  for (unsigned i = 0; rc.state != RC_WAIT || !rc.power; ++i) { CHECK(i < 60000); rc_tick(10); }
  CHECK(rc_left(rc.now) == 600000u && sim_power_on);
}
// Проверить ожидаемую ступень и срок восстановления.
static void rc_expect(unsigned i, uint32_t ms, uint8_t power)
{
  CHECK(i < rc_logged && rc_log[i].ms == ms && rc_log[i].power == power);
}
/* Сигнал, регистрация, тип сети: опрос через канал раз в 30 с, ERROR на запрос не рвёт TCP,
   без связи - «неизвестно». */
static unsigned low_signals; static uint8_t low_csq;
// Пройти штатный запуск модема без сервера.
static void low_model(uint8_t csq) { ++low_signals; low_csq = csq; }
// Проверить сигналы сети, время и информацию SIM.
static void info_tests(void)
{
  rc_online(); rc_run(40000u);
  CHECK(comm_info()->csq == 22 && comm_info()->creg == 1 && comm_info()->cgreg == 1 && comm_info()->net == 4);
  CHECK(comm_info()->rsrp == 105 && comm_info()->rsrq == 10 && comm_info()->sinr == 175);
  CHECK(sim_qeng_queries >= 1 && sim_qeng_queries <= 2);
  const char *lte = sim_qeng, *nw_lte = sim_nwinfo;
  /* Плата №1, 13.10: SLM336 выдаёт индексы CESQ, а не модули дБм/дБ. */
  static const char *const slm_lte[] = {
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",\"FDD\",250,02,36DE11F,178,200,1,3,3,899,48,21,-67,54,27\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",\"FDD\",250,02,36DE105,88,3048,7,5,5,899,41,19,-70,61,20\r\nOK\r\n"
  };
  for (unsigned i = 0; i < 2u; ++i) {
    sim_qeng = slm_lte[i]; rc_run(40000u);
    CHECK(comm_info()->rsrp == (i ? 100 : 93) && comm_info()->rsrq == (i ? 10 : 9)
          && comm_info()->sinr == (i ? 61 : 54));
    CHECK(comm_info()->band == (i ? 7 : 1) && comm_info()->bandwidth == (i ? 5 : 3));
    /* Без последнего поля и с хвостом строка не публикуется. */
    const char *end = strrchr(slm_lte[i], ',');
    info_lte_line((const uint8_t *)slm_lte[i], (uint16_t)(end - slm_lte[i]));
    CHECK(info.v.rsrp == 255 && info.v.rsrq == 255 && info.v.sinr == 255);
    CHECK(info.v.band == 255 && info.v.bandwidth == 255);
    info_lte_line((const uint8_t *)slm_lte[i], (uint16_t)strlen(slm_lte[i]));
    CHECK(info.v.rsrp == 255 && info.v.rsrq == 255 && info.v.sinr == 255);
  }
  /* Плата №1, 13.10: NextConfig в режиме наведения видел 255 между постановкой QENG и ответом. */
  sim_qeng = slm_lte[1]; rc_run(40000u);
  CHECK(comm_info()->rsrp == 100 && comm_info()->band == 7);
  comm_info_antenna(sim_now); comm_info_poll(sim_now);
  CHECK(comm_info()->rsrp == 100 && comm_info()->rsrq == 10 && comm_info()->band == 7);
  info_lte_line(NULL, 0); /* OK без строки QENG */
  CHECK(comm_info()->rsrp == 255 && comm_info()->band == 255);
  sim_qeng = slm_lte[1]; rc_run(40000u);
  CHECK(comm_info()->rsrp == 100);
  static const char *const invalid[] = {
    "+QENG: \"servingscell\",\"NOCONN\",\"GSM\",250,01,00AF,ABCD,17,62,0,40\r\nOK\r\n",
    "+QENG: \"servingcell\",\"SEARCH\"\r\nOK\r\n",
    "+QENG: \"servingcell\",\"LIMSRV\"\r\nOK\r\n",
    "ERROR\r\n", "OK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-105,-10,-73,175\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-105,-10,-73,251,25\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-105,-10,-73,-5,25\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-105,-10,-73,175,25\r\nERROR\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,98,21,-67,54,27\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,48,35,-67,54,27\r\nOK\r\n",
    "+QENG: \"servingcell\",\"NOCONN\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,48,21,67,54,27\r\nOK\r\n"
  };
  for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
    sim_nwinfo = i == 0 ? "+QNWINFO: \"GSM\",\"25001\",\"GSM 900\",62\r\nOK\r\n" : nw_lte;
    sim_qeng = invalid[i]; rc_run(40000u);
    CHECK(comm_info()->rsrp == 255 && comm_info()->rsrq == 255 && comm_info()->sinr == 255);
    CHECK(comm_info()->csq == 22 && comm_info()->creg && comm_info()->cgreg && comm_info()->net == (i == 0 ? 1 : 4));
    CHECK(comm_server_online());
    sim_nwinfo = nw_lte; sim_qeng = lte; rc_run(40000u);
    CHECK(comm_info()->rsrp == 105 && comm_info()->rsrq == 10 && comm_info()->sinr == 175);
  }
  /* Границы сырого SINR и обрыв в любой позиции полного ответа. */
  const char *edge = "+QENG: \"servingcell\",\"CONNECT\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-89,-7,-60,0,-3";
  info_lte_line((const uint8_t *)edge, (uint16_t)strlen(edge));
  CHECK(info.v.rsrp == 89 && info.v.rsrq == 7 && info.v.sinr == 0);
  edge = "+QENG: \"servingcell\",\"CONNECT\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,-89,-7,-60,250,0";
  info_lte_line((const uint8_t *)edge, (uint16_t)strlen(edge));
  CHECK(info.v.sinr == 250);
  for (uint16_t n = 0; n < strlen(edge); ++n) {
    info_lte_line((const uint8_t *)edge, n);
    CHECK(info.v.rsrp == 255 && info.v.rsrq == 255 && info.v.sinr == 255);
  }
  edge = "+QENG: \"servingcell\",\"CONNECT\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,0,0,-60,0,-3";
  info_lte_line((const uint8_t *)edge, (uint16_t)strlen(edge));
  CHECK(info.v.rsrp == 141 && info.v.rsrq == 20 && info.v.sinr == 0);
  edge = "+QENG: \"servingcell\",\"CONNECT\",\"LTE\",0,250,01,ABCD,17,1300,3,5,5,00AF,97,34,-60,250,0";
  info_lte_line((const uint8_t *)edge, (uint16_t)strlen(edge));
  CHECK(info.v.rsrp == 44 && info.v.rsrq == 3 && info.v.sinr == 250);
  { unsigned before = opens; sim_nwinfo_error = 1; rc_run(70000u); CHECK(opens == before && comm_server_online()); }
  CHECK(comm_info()->csq == 22);
  comm_modem_off(); rc_run(100); CHECK(comm_info()->csq == 99 && !comm_info()->creg && !comm_info()->net);
  CHECK(comm_info()->rsrp == 255 && comm_info()->rsrq == 255 && comm_info()->sinr == 255);
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
// Проверить ступени восстановления при отказах и смене суток.
/* Рабочий интернет отделён от отказа TCP и регистрации; час без перезапуска. */
static void server_outage_tests(void)
{
  /* TCP принят, но закрыт до регистрации: ограничитель канала ещё необходим. */
  rc_online(); reserve_enabled=1; sim_open_error=4; rc_keep_opens=true;
  unsigned closed_first=opens;
  comm_channel_reopen(0);
  static const uint32_t closed_delays[]={30000u,60000u,120000u,300000u};
  unsigned observed=opens;
  for (uint32_t start=sim_now;sim_now-start<15u*60000u;) {
    rc_tick(10);
    if (opens==observed) continue;
    unsigned i=opens-1u; observed=opens;
    unsigned attempt=i-closed_first;
    CHECK(open_addresses[i]==attempt%2u);
    if (!attempt) continue;
    uint32_t gap=open_times[i]-open_times[i-1];
    if (attempt%3u) CHECK(gap<1000u);
    else {
      unsigned step=attempt/3u-1u;
      uint32_t pause=closed_delays[step<4u ? step : 3u];
      CHECK(gap>=pause && gap<pause+1000u);
    }
  }
  CHECK(!comm_server_online() && !rc_logged && !rc_power_starts && !rc_soft_starts);
  CHECK(opens>closed_first+12u);
  printf("server closed before registration: 15 min, %u attempts, max 3 per burst, 30/60/120/300 s, alternating addresses PASS\n",opens-closed_first);
  rc_keep_opens=false;
  /* Основной мёртв, резерв работает: без ya.ru и паузы. Обрыв резерва начинает с основного. */
  rc_online(); reserve_enabled=1; reserve_works=true; sim_open_error=1;
  unsigned first=opens; uint32_t begin=sim_now;
  comm_channel_reopen(0);
  while (!comm_server_online() && sim_now-begin<2000u) rc_tick(10);
  CHECK(comm_server_online() && comm_server_address()==1 && opens==first+2u && !pings && !dns_requests && !event13);
  CHECK(!open_addresses[first] && open_addresses[first+1u]==1 && open_times[first+1u]-open_times[first]<1000u);
  first=opens; socket_model=false; sim_feed("+QIURC: \"closed\",0\r\n");
  for (unsigned i=0;opens<first+2u || !comm_server_online();++i) { CHECK(i<200); rc_tick(10); }
  CHECK(!open_addresses[first] && open_addresses[first+1u]==1 && !pings && !event13);
  CHECK(link_count(COMM_LINK_FAIL,0)==2 && !link_count(COMM_LINK_FAIL,1));
  /* Резерв отключён: ни одной попытки второго адреса, обычная лестница. */
  rc_online(); sim_open_error=1; rc_keep_opens=true; first=opens;
  comm_channel_reopen(0); rc_run(900000u);
  for (unsigned i=first;i<opens;++i) {
    CHECK(!open_addresses[i]);
    if (i>first) {
      unsigned step=i-first-1u; uint32_t gap=open_times[i]-open_times[i-1u];
      CHECK(gap>=closed_delays[step<4u ? step : 3u] && gap<closed_delays[step<4u ? step : 3u]+1000u);
    }
  }
  CHECK(opens>first+4u && event13==1 && !rc_logged && !rc_power_starts && !rc_soft_starts);
  rc_keep_opens=false;
  /* Плата 12.10: закрытый сеанс, статус/журнал в TX, первый адрес отказывает,
     второй молчит до таймаута QIOPEN. Интернет всё время работает. */
  for (unsigned packet=0;packet<5;++packet) {
    rc_online(); reserve_enabled=1; sim_open_error=3; probe_result=0;
    if (!(packet&1u)) user_send_statusPack(CH_SERVER);
    else { uint8_t data[35]={0}; data[2]=1; user_send_anyData(10,data,sizeof(data),CH_SERVER,1); }
    CHECK(comm_tx_busy());
    socket_model=false; sim_feed("+QIURC: \"closed\",0\r\n");
    rc_keep_opens=true;
    unsigned first=opens;
    if (packet>=2) { /* Поздний closed пробу сохраняет; публичное переоткрытие её отменяет. */
      for (unsigned i=0;!ch.probe;++i) { CHECK(i<16000); rc_tick(10); }
      CHECK(!ch.probe_active && !pings);
      if (packet==4) comm_channel_reopen(0); /* Отмена через публичный запрос нового TCP. */
      else sim_feed("+QIURC: \"closed\",0\r\n");
      if (packet!=4) {
        /* Без TCP теперь мог уже начаться фоновый AT: он заканчивается за свой срок. */
        for (unsigned wait=0;!ch.probe_active;++wait) { CHECK(wait<31); rc_tick(10); }
        CHECK(ch.probe_active==COMM_CH_PING && opens==first+2u);
      }
    }
    rc_run(comm_timeouts[COMM_T_QIOPEN]+10000u);
    printf("board outage packet=%u: sv=%u ch=%u tx=%u probe=%u pings=%u opens=%u event13=%u\n",
      packet,sv.state,ch.state,ch.tx_state,ch.probe,pings,opens,event13);
    CHECK(pings && comm_channel_probe_result()==COMM_CH_PING && event13==1);
    unsigned checked=opens;
    rc_run(3600000u);
    CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts && event13==1 && !rc.soft_fails);
    CHECK(opens>first+6);
    CHECK(link_count(COMM_LINK_FAIL,0)==1 && link_count(COMM_LINK_FAIL,1)==1);
    for (unsigned i=first;i<opens;++i) {
      CHECK(open_addresses[i]==(i-first)%2u);
      if (open_addresses[i]) CHECK(open_times[i]-open_times[i-1]<1000u);
      else if (i>=checked+2u) {
        uint32_t gap=open_times[i]-open_times[i-2];
        unsigned step=(i-first)/2u-1u-(packet==4); /* Отменённая проба ступень не расходует. */
        uint32_t want=closed_delays[step<4u ? step : 3u];
        if (want<comm_timeouts[COMM_T_QIOPEN]) want=comm_timeouts[COMM_T_QIOPEN]+1000u;
        CHECK(gap>=want && gap<want+2000u); /* QIOPEN и ya.ru должны закончиться. */
      }
    }
    uint32_t back=sim_now; primary_restored=true;
    while (!comm_server_online() && sim_now-back<=302000u) rc_tick(10);
    CHECK(comm_server_online() && !comm_server_address() && sim_now-back<=302000u && event13==1);
    rc_keep_opens=false;
  }
  /* Возврат только основного в разных фазах зрелой пары, включая уже начатый QIOPEN. */
  static const uint32_t phases[]={0u,10u,40u,1000u,149990u,150010u,152000u,299900u};
  for (unsigned phase=0;phase<sizeof(phases)/sizeof(phases[0]);++phase) {
    rc_online(); reserve_enabled=1; sim_open_error=3; rc_keep_opens=true;
    first=opens; comm_channel_reopen(0);
    for (unsigned i=0;opens<first+9u;++i) { CHECK(i<150000); rc_tick(10); }
    CHECK(!open_addresses[opens-1u] && sv.silent_step==3);
    rc_run(phases[phase]);
    uint32_t back=sim_now; primary_restored=true;
    while (!comm_server_online() && sim_now-back<=302000u) rc_tick(10);
    CHECK(comm_server_online() && !comm_server_address() && sim_now-back<=302000u);
    CHECK(event13==1 && !rc_logged && !rc_power_starts && !rc_soft_starts);
    printf("primary restored phase=%u ms: registered in %u ms, backup still dead PASS\n",phases[phase],sim_now-back);
    rc_keep_opens=false;
  }
  /* Регистрация не отвечает: поставленный кадр уступает принятой пробе. */
  rc_online(); drop_register=1; comm_channel_reopen(0);
  for (unsigned i=0;!ch.probe;++i) { CHECK(i<3000); rc_tick(10); }
  CHECK(!comm_tx_busy() && !ch.probe_active);
  { uint8_t data[16]={0}; user_send_anyData(8,data,sizeof(data),CH_SERVER,1); }
  CHECK(comm_tx_busy());
  rc_tick(10); CHECK(ch.probe_active==COMM_CH_PING && ch.tx_state==TX_QUEUED);
  rc_run(2000u); CHECK(pings && event13==1);
  for (unsigned failure=0; failure<5; ++failure) {
    rc_online(); reserve_enabled=1;
    if (failure==4) sim_open_error=2;
    else if (failure==0 || failure==3) sim_open_error=1;
    else if (failure==1) drop_register=1;
    else reject_register=1;
    if (failure==3) ping_success=0; /* DNS подтверждает интернет после отказа ping. */
    rc_keep_opens=true;
    unsigned first=opens;
    if (failure==0) { /* Сервер недоступен уже при первом TCP-сеансе. */
      socket_model=false; // Первый сеанс начинается без сокета от подготовительного подключения.
      comm_server_init(&test_server_handlers); comm_channel_receive(user_comm_rx,user_comm_rx_done);
    } else comm_channel_reopen(0);
    for (uint32_t start=sim_now; sim_now-start<3600000u;) rc_tick(10);
    CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts && event13==1);
    CHECK(!rc.soft_fails && !rc.early && comm_modem_ready() && pings>=4);
    if (failure==3) CHECK(dns_requests>=4 && comm_channel_probe_result()==COMM_CH_DNS);
    CHECK(opens>first+4);
    for (unsigned i=first;i<opens;++i) {
      CHECK(open_addresses[i]==(i-first)%2u);
      if (open_addresses[i]) CHECK(open_times[i]-open_times[i-1]<(failure==4 ? comm_timeouts[COMM_T_QIOPEN]+1000u : 12000u));
      else if (i>first) {
        unsigned step=(i-first)/2u-1u;
        uint32_t want=closed_delays[step<4u ? step : 3u];
        if (failure==4 && want<2u*comm_timeouts[COMM_T_QIOPEN]) want=2u*comm_timeouts[COMM_T_QIOPEN];
        uint32_t gap=open_times[i]-open_times[i-2];
        CHECK(gap>=want && gap<=want+(failure==4 ? 2000u : failure==3 ? 19000u : 1000u));
      }
    }
    uint32_t back=sim_now; sim_open_error=drop_register=reject_register=0;
    while (!comm_server_online() && sim_now-back<=302000u) rc_tick(10);
    CHECK(comm_server_online() && sim_now-back<=302000u);
    CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts && event13==1);
    rc_keep_opens=false;
    printf("server outage %u: 1h no restarts; one event13; restored in %u ms PASS\n",failure,sim_now-back);
  }
  /* Плата №1 21.10: первая регистрация после старта без ответа - питание модема сразу, один раз за включение;
     дальше молчание сервера идёт обычным путём без перезапусков. */
  rc_online(); rc.first = 0; drop_register = 1; rc_keep_opens = true;
  comm_channel_reopen(0);
  for (unsigned i = 0; !rc_logged; ++i) { CHECK(i < 2000); rc_tick(10); }
  rc_expect(0, 0, RC_PLANNED); CHECK(registrations_sent >= 1 && link_count(COMM_LINK_FAIL, 0) == 1);
  rc_run(1800000u);
  CHECK(rc_logged == 1 && rc_power_starts == 1 && qpowd_count == 1 && !rc_soft_starts && rc.first && pings);
  { uint32_t back = sim_now; drop_register = 0;
    while (!comm_server_online() && sim_now - back <= 302000u) rc_tick(10);
    CHECK(comm_server_online() && rc_logged == 1); }
  rc_keep_opens = false;
  /* Сервер ответил хотя бы раз: молчание следующей регистрации модем не перезапускает. */
  rc_online(); rc.first = 0; rc_tick(10); CHECK(rc.first);
  drop_register = 1; comm_channel_reopen(0); rc_run(60000u);
  CHECK(!rc_logged && !rc_power_starts); drop_register = 0;
  puts("first registration silent: one power restart at once, then ordinary path; not after online PASS");
  /* Плановый перезапуск: QPOWD, питание снимается по POWERED DOWN (модель 0,25 с); канал, SMS и опрос
     в это время в UART не пишут. Молчащий модем - через COMM_T_POWER_DOWN; отказ с ладдера - без QPOWD. */
  for (unsigned silent = 0; silent < 2; ++silent) {
    rc_online(); rc_run(20000u); CHECK(comm_server_online());
    unsigned powers = rc_power_starts; uint32_t begin = sim_now;
    modem_silent = silent; rc_schedule(RC_PLANNED, 0);
    while (rc_power_starts == powers) { CHECK(sim_now - begin < 3000u); rc_tick(10); }
    uint32_t spent = sim_now - begin;
    CHECK(qpowd_count == 1 && !strcmp(last_command, "AT+QPOWD")); /* после QPOWD - ни одной команды */
    CHECK(silent ? spent >= comm_timeouts[COMM_T_POWER_DOWN] && spent < comm_timeouts[COMM_T_POWER_DOWN] + 100u
                 : spent >= 250u && spent < 400u);
    modem_silent = 0; rc_run(60000u); CHECK(comm_server_online() && rc_power_starts == powers + 1u);
    printf("planned restart, %s modem: QPOWD, power cut after %u ms PASS\n", silent ? "silent" : "answering", spent);
  }
  rc_online(); rc_run(20000u); CHECK(comm_server_online());
  rc_schedule(RC_POWER, 0); rc_run(60000u); CHECK(comm_server_online() && rc_power_starts == 1u && !qpowd_count);
  /* При отказе открытия TCP без интернета остаётся прежняя лестница TCP/AT/питание. */
  rc_online(); reserve_enabled=1; sim_open_error=3; ping_success=dns_success=0; probe_result=0;
  user_send_statusPack(CH_SERVER); socket_model=false; sim_feed("+QIURC: \"closed\",0\r\n"); rc_run(1800000u);
  CHECK(pings && dns_requests && comm_channel_probe_result()==3);
  printf("server no internet: decisions=%u soft=%u power=%u event13=%u\n", rc_logged, rc_soft_starts, rc_power_starts, event13);
  CHECK(rc_logged>=3 && rc_soft_starts==1 && rc_power_starts>=1 && !event13);
  rc_expect(0,0,0); rc_expect(1,0,1); rc_expect(2,600000u,1);
}
/* Поздний OK после closed; потерянный OK при живом CMEE; действительно молчащий модем. */
static void probe_ack_tests(void)
{
  for (unsigned mode=0;mode<3;++mode) {
    rc_online(); reserve_enabled=1; sim_open_error=1;
    probe_ok_delay=1500; probe_no_ok=mode!=0;
    comm_channel_reopen(0);
    for (unsigned guard=0;!pings;++guard) { CHECK(guard<1000); rc_tick(10); }
    sim_feed("+QIURC: \"closed\",0\r\n");
    if (mode==2) { modem_silent=1; ping_reply_kind=0; }
    rc_run(8000);
    if (!mode) CHECK(comm_channel_probe_result()==COMM_CH_PING && !dns_requests);
    if (mode<2) CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts && comm_modem_ready());
    else CHECK(rc_power_starts && rc_logged);
  }
  rc_online(); probe_ok_delay=1500;
  CHECK(comm_channel_probe(COMM_CH_DNS)); rc_run(3000);
  CHECK(comm_channel_probe_result()==COMM_CH_DNS && !rc_logged && comm_modem_ready());
  puts("probe OK: 1500ms + closed / missing OK + live CMEE / silent restart / DNS 1500ms PASS");
}
static void recover_tests(void)
{
  probe_ack_tests();
  server_outage_tests();
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
  /* Каждый домен регистрации снимает длинную паузу без перезапуска питания. */
  static const char *const registered[] = {"+CREG: 1\r\n", "+CGREG: 5,\"1234\",\"5678\"\r\n", "+CEREG: 1\r\n"};
  for (unsigned i = 0; i < 3; ++i) {
    rc_online(); sim_network_wait = 1; rc_fresh(); rc_wait_power();
    sim_feed("+CEREG: 2\r\n+CGREG: 0\r\n+CREG: 3\r\n"); rc_tick(10);
    CHECK(rc.state == RC_WAIT && rc_log[rc_logged-1].ms == 600000u && rc_left(rc.now) > 599000u);
    unsigned powers = rc_power_starts; uint32_t begin = sim_now;
    sim_network_wait = 0; sim_feed(registered[i]); rc_tick(10);
    CHECK(rc.state == RC_WATCH && rc_soft_starts == 3 && rc_power_starts == powers);
    for (unsigned n = 0; !comm_server_online(); ++n) { CHECK(n < 1500); rc_tick(10); }
    CHECK(sim_now - begin < 15000u && sim_power_on);
  }
  rc_online(); sim_network_wait = 1; rc_fresh(); rc_run(3600000u);
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
  comm_modem_record(SLM_READ, 0, 0); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000); comm_modem_record(SLM_READ, 0, 0); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000);
  CHECK(!rc_logged);
  comm_modem_record(SLM_READ, 0, 0); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(100); rc_expect(0, 0, 1);
  rc_online();  /* те же три, но растянуты больше чем на 10 мин - не считаются пачкой */
  for (int i = 0; i < 3; ++i) { comm_modem_record(SLM_READ, 0, 0); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(330000u); }
  CHECK(!rc_logged);
  /* Таймауты при запуске модема (нет сети) не копятся в пачку с таймаутом в работе. */
  rc_online(); sim_network_wait = 1; rc_fresh(); rc_run(150000u); CHECK(rc_logged == 2);
  sim_network_wait = 0; rc_run(120000u); CHECK(comm_server_online());
  { unsigned before = rc_logged; comm_modem_record(SLM_READ, 0, 0); comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TIMEOUT, 0); rc_run(1000); CHECK(rc_logged == before); }
  /* Отказы TCP не доказывают потерю интернета и не дают ранний перезапуск. */
  rc_online(); for (int i = 0; i < 5; ++i) { comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TCP, 566); rc_run(1000); }
  CHECK(!rc_logged && !rc_power_starts && !rc_soft_starts);
  /* 9. Ночь: в 03:00 модем перезапускается раз; занят - ждёт; без часов - раз в сутки работы. */
  rc_online(); rc_clock = 1790899200u + 2u * 3600u + 3590u; /* 02:59:50 по модели */
  rc_busy_flag = true; rc_run(60000u); CHECK(!rc_logged);
  rc_busy_flag = false; rc_run(60000u); CHECK(rc_logged == 1 && qpowd_count == 1); rc_expect(0, 0, RC_PLANNED);
  rc_run(1800000u); CHECK(rc_logged == 1 && comm_server_online());  /* в то же окно второй раз нет */
  rc_online(); rc_run(23u * 3600000u); CHECK(!rc_logged); rc_run(3600000u + 60000u); CHECK(rc_logged == 1 && qpowd_count == 1); rc_expect(0, 0, RC_PLANNED);
  /* 10 мин устойчивой связи - ступени с начала. */
  rc_online(); sim_no_sim = 1; rc_fresh(); rc_run(700000u); CHECK(rc.power_step == 2);
  sim_no_sim = 0; rc_run(1800000u + 700000u); CHECK(comm_server_online() && rc.power_step == 0);
  printf("comm_recover: powered pauses, registration URC wake, no SIM/network/internet, silent modem 10/30/60 min, early AT, TCP classified by ping/DNS, night/day, ladder reset PASS\n");
}

static void reset_session_contract(void)
{
  server_reset(); comm_recover_init(&rc_hooks, sim_now);
  unsigned before = rc_session_resets;
  rc_schedule(true, 0); comm_recover_poll(sim_now);
  CHECK(rc_session_resets == before + 1u);
}
