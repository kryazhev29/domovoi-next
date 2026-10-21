/* Отказ SLM336 при работающем прозрачном мосте: настоящие UART/AT/канал/сервер,
   питание и ответы оператора моделируются. Проверка платы этим не заменяется. */
static unsigned stall_keep_online, stall_drop_registration, stall_reject_registration;
static unsigned stall_registrations, stall_statuses;
static uint32_t stall_registration_at[8];
/* На RS-485 свободный слот временно заперт ответом моста: производитель может
   отложить кадр; сравнение GSM-эталона здесь не предполагает обязательную отправку. */
static void stall_registration(void) { user_send_registrationData(CH_SERVER); }
static void stall_status(void) { user_send_statusPack(CH_SERVER); }
static const comm_server_handlers_t stall_handlers = {
  stall_registration, stall_status, server_session, server_rx_process, no_traffic
};

/* Сервер получает только физически отправленный мосту кадр. */
static void stall_bridge_reply(void)
{
  if (!w_outbox_count) return;
  CHECK(w_outbox_count >= 11 && !memcmp(w_outbox, "SLDN", 4));
  uint8_t response[32], args[5] = {0};
  uint16_t n = 0;
  if (w_outbox[8] == 1) {
    if (stall_registrations < 8) stall_registration_at[stall_registrations] = sim_now;
    ++stall_registrations;
    args[0] = stall_reject_registration ? 2 : 0;
    if (!stall_drop_registration) n = server_frame(response, 1, 0, args, sizeof(args));
  } else if (w_outbox[8] == 2) {
    ++stall_statuses; args[0] = w_outbox[7];
    n = server_frame(response, 24, 0, args, 1);
  }
  w_outbox_count = 0;
  if (n) w_push(response, n);
}

static void stall_tick(void)
{
  power_events = 0; /* ограниченная трасса GPIO не копит сутки одинаковых сценариев */
  transport_tick(10);
  stall_bridge_reply();
  comm_recover_poll(sim_now);
  comm_info_poll(sim_now);
  if (stall_keep_online && !(ch.wifi && comm_server_online() && ch.state == CH_OPEN)) {
    fprintf(stderr, "Wi-Fi lost: now=%u CUSD silence=%u modem=%u reason=%u channel=%u server=%u recovery=%u/%u\n",
            sim_now, cusd_silence, comm_modem_state(), slm.attempt.status.reason, ch.state, sv.state, rc.state, rc_left(sim_now));
    CHECK(0);
  }
}
static void stall_run(uint32_t ms)
{
  uint32_t start = sim_now;
  while (sim_now - start < ms) stall_tick();
}
static void stall_start(void)
{
  stall_keep_online = stall_drop_registration = stall_reject_registration = 0;
  server_reset();
  test_config.wifi = 1; w_rx = NULL; w_count = w_outbox_count = 0;
  w_dead = w_cmd_mode = w_next_mode = w_mix = 0; w_tcp = w_ap = w_ping_ok = 1;
  stall_registrations = stall_statuses = 0;
  CHECK(comm_modem_restart(true, sim_now)); socket_model = false;
  comm_server_init(&stall_handlers); comm_channel_receive(user_comm_rx, user_comm_rx_done);
  comm_recover_init(&rc_hooks, sim_now);
  for (unsigned i = 0; !comm_server_online() && i < 2000; ++i) stall_tick();
  for (unsigned i = 0; !comm_modem_ready() && i < 2000; ++i) stall_tick();
  CHECK(ch.wifi && comm_server_online() && comm_modem_ready());
  stall_keep_online = 1;
}

static void modem_stall_tests(void)
{
  /* OK без URC и совсем без OK, но с ответом на ресинхронизацию:
     ожидание заканчивается, слот освобождён, мост всё время передаёт статусы. */
  for (unsigned silent = 0; silent < 2; ++silent) {
    stall_start(); stall_run(301000); ussd_code = "*100#"; cusd_silence = silent;
    unsigned session = slm.session, statuses = stall_statuses, links0 = w_links;
    stall_run(40000);
    CHECK(cusd_count == 1 && bal.state == BAL_IDLE && !balance_count);
    CHECK(!slm.side.running && !slm.side.queued && !slm.side.done);
    CHECK(slm.session == session && comm_modem_ready() && sim_power_on);
    CHECK(stall_statuses > statuses + 1 && w_links > links0);
    CHECK((int32_t)(bal.next - sim_now) > 1700000);
    /* Поздний URC уже отменённого запроса не присваивает баланс. */
    sim_feed("+CUSD: 0,\"Balance 99 rub\",15\r\n"); stall_run(100);
    CHECK(!balance_count && bal.state == BAL_IDLE);
    test_config.wifi = 0; stall_keep_online = 0;
  }
  /* Модем замолк вместе с CUSD: первый короткий перезапуск немедленно,
     затем действующие ступени 30/60 минут при включённом питании. Мост жив каждый poll. */
  stall_start(); stall_run(301000); ussd_code = "*100#"; cusd_silence = 2;
  unsigned session = slm.session, resets = rc_session_resets, statuses = stall_statuses;
  stall_run(50000);
  printf("silent modem after 50 s: state=%u step=%u reason=%u recovery=%u/%u power=%u\n",
         comm_modem_state(), slm.attempt.status.step, slm.attempt.status.reason, rc.state, rc_left(sim_now), sim_power_on);
  CHECK(slm.session == session + 1 && rc_session_resets == resets);
  CHECK(rc.state == RC_WAIT && rc_left(sim_now) > 1750000 && rc_left(sim_now) <= 1800000 && sim_power_on);
  CHECK(stall_statuses > statuses + 1 && bal.state == BAL_IDLE);
  stall_run(1820000);
  CHECK(rc.state == RC_WAIT && rc_left(sim_now) > 3550000 && rc_left(sim_now) <= 3600000 && sim_power_on);
  /* Модем снова отвечает на следующем коротком старте; серверный сеанс прежний. */
  modem_silent = 0; cusd_silence = 0; ussd_code = "";
  stall_run(rc_left(sim_now) + 20000);
  CHECK(comm_modem_ready() && rc_session_resets == resets && info.v.csq != 99);
  /* URC сокета GSM не закрывают TCP моста. */
  sim_feed("+QIURC: \"closed\",0\r\n+QIURC: \"pdpdeact\",1\r\n"); stall_run(100);
  CHECK(comm_server_online() && ch.wifi);
  test_config.wifi = 0; stall_keep_online = 0;
  puts("modem stall: CUSD without URC/OK, late URC, immediate restart then powered 30/60 min, Wi-Fi statuses and GSM URCs PASS");
}

/* Съёмка закрывает сеанс моста: в журнал связи ни отказа, ни нового подключения (плата №1 21.10: события 18 и 14
   на каждый снимок). Если после съёмки сервер не принял регистрацию - отказ и затем подключение пишутся. */
static void wifi_camera_link_tests(void)
{
  for (unsigned drop = 0; drop < 2; ++drop) {
    stall_start(); stall_keep_online = 0;
    unsigned before = links;
    for (unsigned i = 0; !comm_channel_wifi_pause(true) && i < 300; ++i) stall_tick();
    CHECK(comm_channel_wifi_pause(true) && !comm_server_online() && links == before);
    stall_drop_registration = drop; comm_channel_wifi_pause(false);
    if (!drop) {
      for (unsigned i = 0; !comm_server_online() && i < 2000; ++i) stall_tick();
      CHECK(comm_server_online() && ch.wifi && links == before);
    } else {
      for (unsigned i = 0; links == before && i < 30000; ++i) stall_tick();
      CHECK(links > before && link_kinds[before] == COMM_LINK_FAIL);
      stall_drop_registration = 0;
      for (unsigned i = 0; !comm_server_online() && i < 60000; ++i) stall_tick();
      CHECK(comm_server_online() && link_kinds[links - 1u] == COMM_LINK_UP);
    }
    test_config.wifi = 0;
  }
  puts("Wi-Fi camera pause: no link events for planned close, refusal after it still logged PASS");
}

static void wifi_server_refusal_tests(void)
{
  for (unsigned reserve = 0; reserve < 2; ++reserve) {
    for (unsigned reject = 0; reject < 2; ++reject) {
      stall_start(); stall_keep_online = 0; reserve_enabled = reserve;
      stall_registrations = 0;
      stall_drop_registration = !reject; stall_reject_registration = reject;
      uint32_t start = sim_now;
      comm_channel_reopen(0);
      for (unsigned i = 0; (ch.wifi || !comm_server_online()) && i < 30000; ++i) stall_tick();
      printf("Wi-Fi registration refusal: reserve=%u reject=%u attempts=%u elapsed=%u GSM=%u\n",
             reserve, reject, stall_registrations, sim_now - start, !ch.wifi && comm_server_online());
      CHECK(!ch.wifi && comm_server_online() && sim_now - start < 300000);
      CHECK(stall_registrations == (reject ? 3u : 6u));
      unsigned n = reject ? 1u : 2u;
      CHECK(stall_registration_at[n] - stall_registration_at[0] >= 30000);
      CHECK(stall_registration_at[2 * n] - stall_registration_at[n] >= 60000);
      /* Принятая регистрация GSM и возврат на мост через прежние 10 мин. */
      stall_drop_registration = stall_reject_registration = 0;
      uint32_t fell_at = wf.fell_at;
      while (sim_now - fell_at < WF_RETRY_MS - 1000u) { stall_tick(); CHECK(!ch.wifi); }
      stall_run(15000);
      CHECK(ch.wifi && comm_server_online() && ch.connections == 0);
      test_config.wifi = 0;
    }
  }
  puts("Wi-Fi registration silence/rejection: growing pauses, GSM after three connections, return in 10 min PASS");
}
