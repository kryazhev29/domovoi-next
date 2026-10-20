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
            sim_now, cusd_silence, comm_modem_state(), slm.attempt.status.reason, ch.state, sv.state, rc.state, rc.wait_ms);
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
    stall_start(); ussd_code = "*100#"; cusd_silence = silent;
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
  stall_start(); ussd_code = "*100#"; cusd_silence = 2;
  unsigned session = slm.session, resets = rc_session_resets, statuses = stall_statuses;
  stall_run(50000);
  printf("silent modem after 50 s: state=%u step=%u reason=%u recovery=%u/%u power=%u\n",
         comm_modem_state(), slm.attempt.status.step, slm.attempt.status.reason, rc.state, rc.wait_ms, sim_power_on);
  CHECK(slm.session == session + 1 && rc_session_resets == resets);
  CHECK(rc.state == RC_WAIT && rc.wait_ms == 1800000 && sim_power_on);
  CHECK(stall_statuses > statuses + 1 && bal.state == BAL_IDLE);
  stall_run(1820000);
  CHECK(rc.state == RC_WAIT && rc.wait_ms == 3600000 && sim_power_on);
  /* Модем снова отвечает на следующем коротком старте; серверный сеанс прежний. */
  modem_silent = 0; cusd_silence = 0; ussd_code = "";
  stall_run(rc.wait_ms - (sim_now - rc.wait_since) + 20000);
  CHECK(comm_modem_ready() && rc_session_resets == resets && info.v.csq != 99);
  /* URC сокета GSM не закрывают TCP моста. */
  sim_feed("+QIURC: \"closed\",0\r\n+QIURC: \"pdpdeact\",1\r\n"); stall_run(100);
  CHECK(comm_server_online() && ch.wifi);
  test_config.wifi = 0; stall_keep_online = 0;
  puts("modem stall: CUSD without URC/OK, late URC, immediate restart then powered 30/60 min, Wi-Fi statuses and GSM URCs PASS");
}
