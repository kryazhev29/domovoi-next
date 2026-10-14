/* Доработка: Кряжев А., 2026. Доступ до ATA, ограниченный ответ и возврат данных.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static unsigned calls, chups_at_call, answers_at_call, call_actions; static char call_seen[24];
static uint32_t call_action_at;
// Запомнить номер и результат проверки доступа до ответа.
#define CT_PHONE 1
static bool user_incomingcall(char *number, uint8_t typ)
{
  CHECK(typ == CT_PHONE);
  ++calls; chups_at_call=chups; answers_at_call=answers; call_action_at=sim_now;
  CHECK(strlen(number)<sizeof(call_seen)); memcpy(call_seen,number,strlen(number)+1u);
  /* Модель допуска: скрытый и чужой номер не открывают. Производственный доступ прежний. */
  if (number[0] && strcmp(number,"+79990000000")) ++call_actions;
  return true;
}
/* Чтение доступно между шагами, но доступ с записью ждёт свободного исполнителя. */
static bool call_writer_ready;
static unsigned call_access_attempts;
static char *user_last10digitsnumber(char *number)
{ size_t n = strlen(number); return n > 10u ? number + n - 10u : number; }
static bool user_access_ready(const char *id)
{ CHECK(strlen(id) <= 10u); ++call_access_attempts; return call_writer_ready; }
/* ОБРАБОТЧИК ЗВОНКА ПРИЛОЖЕНИЯ */
// Смоделировать занятость приложения обновлением прошивки.
static bool user_comm_busy_model(void) { return comm_call_active(); }
// Сбросить наблюдения и модель вызова.
static void call_reset(void) { call_writer_ready=true; call_access_attempts=0; server_reset(); comm_recover_init(&rc_hooks,sim_now); comm_info_antenna(sim_now); calls=call_actions=0; call_seen[0]=0; call_action_at=0; }
// Подать номер и сигнал входящего вызова.
static void ring(const char *clip) { if (call.state == CALL_IDLE) call_present=1; sim_feed("RING\r\n"); if (clip) sim_feed(clip); }
// Продвинуть модель до физической отправки отбоя.
static void call_until_hangup(void)
{
  for (unsigned i=0; !chups; ++i) { CHECK(i<400); transport_tick(10); }
  CHECK(hangup_at-answer_at<=comm_timeouts[COMM_T_CALL_LIMIT]);
}
// Продвинуть модель до возврата серверной регистрации.
static uint32_t call_until_registered(void)
{
  for (unsigned i=0; !comm_server_online() || comm_channel_call_pending(); ++i) { CHECK(i<1200); transport_tick(10); comm_recover_poll(sim_now); }
  return sim_now-hangup_at;
}
// Проверить доступ, сроки ATA/CHUP и восстановление данных после вызова.
static void call_tests(void)
{
  /* Нет мобильного интернета: в длинной паузе номер открывает, затем ATA и CHUP. */
  rc_online(); call_writer_ready=true; calls=call_actions=0; sim_error_step=SLM_ACTIVATE; rc_fresh(); rc_wait_power();
  unsigned powers=rc_power_starts, resets=rc_session_resets; uint32_t pause=rc.wait_since;
  ring("+CLIP: \"+79161234567\",145\r\n");
  for (unsigned i=0; !chups || comm_channel_call_pending(); ++i) { CHECK(i<400); rc_tick(10); }
  CHECK(calls==1 && call_actions==1 && answers==1 && chups==1 && !call_present);
  CHECK(call_action_at<=answer_at && answer_at<hangup_at && hangup_at-answer_at<=2500);
  CHECK(rc.state==RC_WAIT && rc.wait_since==pause && rc_power_starts==powers && rc_session_resets==resets && sim_power_on);
  /* Конец паузы во время разговора ждёт штатного отбоя. */
  advance(10000); ring("+CLIP: \"+79161234567\",145\r\n"); rc_tick(10);
  rc.wait_since=sim_now-rc.wait_ms; rc_tick(500);
  CHECK(comm_channel_call_pending() && rc_power_starts==powers && sim_power_on);
  for (unsigned i=0; rc_power_starts==powers; ++i) { CHECK(i<500); rc_tick(10); }
  CHECK(chups==2 && rc_power_starts==powers+1u);

  /* Сломанный AT в звонке не должен заблокировать назначенный перезапуск. */
  rc_online(); call_writer_ready=true; sim_error_step=SLM_ACTIVATE; rc_fresh(); rc_wait_power();
  ring("+CLIP: \"+79161234567\",145\r\n");
  for (unsigned i=0; !answers; ++i) { CHECK(i<100); rc_tick(10); }
  CHECK(comm_channel_call_pending()); powers=rc_power_starts;
  interrupt_byte(0,USART_SR_ORE); rc_tick(10);
  CHECK(comm_modem_status()->at_fault);
  rc.wait_since=sim_now-rc.wait_ms; rc_tick(10);
  CHECK(rc_power_starts==powers+1u && comm_power_v5_state()==COMM_PWR_DISCHARGE);
  sim_error_step=-1; rc_run(30000u);
  CHECK(comm_server_online() && !comm_channel_call_pending());

  /* OK от CHUP при живом вызове: CLCC требует повтор, без повторного доступа.
     После предела CHUP больше не отправляется, но даже пауза RING > 8 с не даёт доступ. */
  for (unsigned fw=0; fw<2; ++fw) {
    call_reset(); if (fw) device.status |= 1u << 11;
    hangup_ignored=1; ring("+CLIP: \"+79161234567\",145\r\n"); advance(2000);
    CHECK(chups==2 && clccs==2 && !call_present && !comm_channel_call_pending());
    CHECK(calls==!fw && call_actions==!fw && answers==!fw);
    call_reset(); if (fw) device.status |= 1u << 11;
    hangup_ignored=100; uint32_t begin=sim_now;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(3000);
    CHECK(call_present && call.stopped && comm_call_active() && !comm_channel_call_pending());
    CHECK(hangup_at-(fw ? begin : answer_at)<=2500);
    unsigned sent=chups; advance(9000);
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(1500);
    CHECK(chups==sent && calls==!fw && answers==!fw && comm_call_active());
    call_present=0; advance(10000); CHECK(!comm_call_active());
    device.status &= ~(1u << 11); hangup_ignored=0;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700);
    CHECK(calls==!fw+1u && answers==!fw+1u && chups==sent+1u);
  }
  /* ERROR/тишина CLCC не равны пустому списку; фоновая проверка не шлёт CHUP. */
  for (unsigned silent=0; silent<2; ++silent) {
    call_reset(); clcc_error=!silent; clcc_silence=silent;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(3500);
    CHECK(comm_call_active() && call.stopped && !comm_channel_call_pending());
    unsigned sent=chups; advance(6000); CHECK(chups==sent && calls==1);
    clcc_error=clcc_silence=0; advance(3500); CHECK(!comm_call_active());
  }
  /* ACK уже ждал 9,99 с: обычный звонок и отклонение FW не закрывают исправный TCP. */
  for (unsigned fw=0; fw<2; ++fw) {
    call_reset(); unsigned kept_opens=opens, kept_closes=closes, kept_links=links;
    if (fw) device.status |= 1u << 11;
    tx_unacked=1; user_send_statusPack(CH_SERVER); advance(100);
    CHECK(!comm_tx_busy() && tcp_sent!=tcp_acked); tx_unacked=0;
    ch.tx_since=sim_now-9990u;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700);
    CHECK(comm_server_online() && !comm_tx_busy());
    CHECK(opens==kept_opens && closes==kept_closes && links==kept_links);
  }
  /* Настоящий допуск приложения: бит 11 отсекает доступ и ATA даже без CLIP
     и при занятой флеш. Живое TCP-соединение после CHUP остаётся открытым. */
  for (unsigned busy=0; busy<2; ++busy) {
    for (unsigned clip=0; clip<2; ++clip) {
      call_reset(); unsigned fw_opens=opens, fw_closes=closes;
      device.status |= 1u << 11; call_access_ready=!busy;
      ring(clip ? "+CLIP: \"+79161234567\",145\r\n" : NULL);
      advance(200);
      CHECK(!calls && !call_actions && !answers && chups==1);
      CHECK(!call.pending && !call_access_attempts && clccs==1);
      CHECK(comm_server_online() && opens==fw_opens && closes==fw_closes);
      ring(clip ? "+CLIP: \"+79161234567\",145\r\n" : NULL); advance(200);
      CHECK(!calls && !answers && chups==1);
      device.status &= ~(1u << 11); call_access_ready=true;
      advance(8100); ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700);
      CHECK(calls==1 && call_actions==1 && answers==1 && chups==2);
      CHECK(hangup_at-answer_at>=1000 && hangup_at-answer_at<=2500);
    }
  }
  /* caller(number)==false: флеш читается, но запись занята. Номер остаётся
     pending; нет ни ATA, ни CHUP, ни доступа до освобождения исполнителя. */
  call_reset(); call_writer_ready=false;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(2000);
  CHECK(call_access_attempts>1 && call.pending==1 && call.state==CALL_NUMBER);
  CHECK(!calls && !call_actions && !answers && !chups && call_present);
  call_writer_ready=true; advance(1700);
  CHECK(calls==1 && call_actions==1 && answers==1 && chups==1 && !call.pending);
  /* Бит 11 во время повторов отменяет pending раньше следующего caller(number).
     Снятие бита после CHUP не возобновляет доступ для отклонённого вызова. */
  call_reset(); call_writer_ready=false;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  CHECK(call_access_attempts>1 && call.pending==1 && !chups);
  unsigned attempts=call_access_attempts;
  device.status |= 1u << 11; advance(200);
  CHECK(!call.pending && !calls && !call_actions && !answers && chups==1);
  CHECK(call_access_attempts==attempts && clccs==1 && !call_present);
  device.status &= ~(1u << 11); call_writer_ready=true; advance(500);
  CHECK(!call.pending && !calls && !answers && chups==1 && call_access_attempts==attempts);
  /* Загрузка началась между CLIP и отложенной проверкой доступа. */
  call_reset(); call_access_ready=false;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  device.status |= 1u << 11; advance(200);
  CHECK(!calls && !call_actions && !answers && chups==1);
  call_reset(); unsigned opened=opens;
  ring("+CLIP: \"+79161234567\",145,\"\",0,\"\",0\r\n"); advance(1700);
  CHECK(calls==1 && call_actions==1 && !strcmp(call_seen,"+79161234567"));
  CHECK(!answers_at_call && !chups_at_call && call_action_at<answer_at);
  CHECK(answers==1 && chups==1 && hangup_at-answer_at>=1040 && hangup_at-answer_at<=1070);
  CHECK(comm_call_active() && comm_server_online() && opens==opened && !closes);
  CHECK(!call_net_queries && !call_pdp_queries && !call_tcp_queries);
  advance(3300); ring("+CLIP: \"+79161234567\",145\r\n"); advance(200);
  CHECK(calls==1 && answers==1 && chups==1);
  advance(8100); CHECK(!comm_call_active());
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700); CHECK(calls==2 && answers==2 && chups==2);
  call_reset(); sim_feed("+CLIP: \"89035551122\",129\r\nRING\r\n"); advance(1700);
  CHECK(calls==1 && !strcmp(call_seen,"89035551122") && answers==1 && chups==1);
  call_reset(); ring(NULL); advance(500); CHECK(!calls && !answers);
  advance(2300); CHECK(!calls && !call_actions && answers==1 && chups==1);
  call_reset(); ring("+CLIP: \"\",128\r\n"); advance(1700);
  CHECK(calls==1 && !call_actions && !call_seen[0] && answers==1 && chups==1);
  call_reset(); ring("+CLIP: \"+79990000000\",145\r\n"); advance(1700);
  CHECK(calls==1 && !call_actions && answers==1 && chups==1);
  call_reset(); ring("+CLIP: \"+7916x\",145\r\n"); advance(2800); CHECK(!calls && answers==1 && chups==1);
  call_reset(); ring("+CLIP: \"+791612345678901234567\",145\r\n"); advance(2800); CHECK(!calls && answers==1 && chups==1);
  /* Занятая флеш задерживает только действие до ATA, а не таймер снятой трубки. */
  call_reset(); call_access_ready=false; ring("+CLIP: \"+79161234567\",145\r\n"); advance(2000);
  CHECK(!calls && !answers && !chups && call.pending==1); call_access_ready=true; advance(100); CHECK(calls==1 && answers==1);
  call_access_ready=false; advance(1600); CHECK(chups==1);
  call_reset(); ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  sim_feed("NO CARRIER\r\n"); advance(20); CHECK(chups==1 && hangup_at-answer_at<1500);
  /* Сроки переживают переполнение millis. */
  call_reset(); ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  uint32_t shift=UINT32_MAX-sim_now-200u;
  sim_now+=shift; call.now+=shift; call.since+=shift; call.last+=shift; call.answered+=shift;
  ch.now+=shift; sv.now+=shift; sv.status_since+=shift; answer_at+=shift; started+=shift;
  advance(1600); CHECK(answers==1 && chups==1 && hangup_at-answer_at<=2500);
  /* Поздний OK оставляет запас: абсолютный предел не сдвигается. */
  call_reset(); answer_delay=1490; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
  CHECK(calls==1 && answers==1 && hangup_at-answer_at<=2500); advance(500);
  CHECK(chups==2 && hangup_at-answer_at==2500);
  for (unsigned failure=1;failure<=3;++failure) {
    call_reset(); answer_mode=failure; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
    CHECK(calls==1 && answers==1 && chups==1);
    CHECK(hangup_at-answer_at==(failure==3 ? 1500u : 40u)); advance(500);
    CHECK(comm_server_online() && !comm_channel_call_pending());
  }
  /* Старый OK после CHUP не является ответом следующего запроса. */
  call_reset(); answer_delay=1800; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
  CHECK(chups==1 && hangup_at-answer_at==1500); advance(800); CHECK(comm_server_online());
  call_reset(); answer_delay=1400; chup_silence=1;
  ring("+CLIP: \"+79160000001\",145\r\n"); advance(2700);
  CHECK(answers==1 && chups==2 && hangup_at-answer_at==2500);
  call_reset(); ring("+CLIP: \"+79160000001\",145\r\n"); advance(100);
  ring("+CLIP: \"+79160000002\",145\r\n"); advance(100);
  CHECK(calls==2 && !strcmp(call_seen,"+79160000002") && answers==1);
  advance(2800); CHECK(calls==2 && answers==2 && chups==2);
  /* Между приглашением и данными нет ATA/CHUP, включая одновременный closed URC. */
  call_reset(); prompt_silence=1;
  uint16_t cap; uint8_t *p=comm_tx_acquire(&cap); CHECK(p); memset(p,0x5A,12); CHECK(comm_tx_commit(12));
  for (unsigned i=0;ch.tx_state!=TX_PROMPT && i<100;++i) transport_tick(10);
  CHECK(ch.tx_state==TX_PROMPT);
  ring("+CLIP: \"+79161112233\",145\r\n"); advance(50);
  CHECK(calls==1 && !answers && slm.side.queued);
  sim_feed("> "); advance(1700); CHECK(answers==1 && chups==1 && sent_count==12 && !comm_tx_busy());
  call_reset(); prompt_silence=1;
  p=comm_tx_acquire(&cap); CHECK(p); memset(p,0x5A,12); CHECK(comm_tx_commit(12));
  for (unsigned i=0;ch.tx_state!=TX_PROMPT && i<100;++i) transport_tick(10);
  ring("+CLIP: \"+79161112233\",145\r\n"); advance(50); CHECK(!answers);
  prompt_silence=0; socket_model=false; sim_feed("+QIURC: \"closed\",0\r\n> "); advance(2000);
  CHECK(calls==1 && answers==1 && chups==1 && opens==2 && comm_server_online());
  /* Прогресс загрузки не меняет запрет телефонного доступа/ответа. */
  call_reset(); device.status|=1u<<11; device.firmwarePercent=37;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700); CHECK(!calls && !answers && chups==1);
  /* Ночной перезапуск проходит только после тихого конца вызова и проверки данных. */
  call_reset(); rc_clock=86400u+3u*3600u; static const comm_recover_hooks_t hooks={rc_session,rc_time,user_comm_busy_model};
  comm_recover_init(&hooks,sim_now); uint8_t session=comm_modem_session();
  ring("+CLIP: \"+79161234567\",145\r\n");
  for (unsigned i=0;i<790;++i) { transport_tick(10); comm_recover_poll(sim_now); CHECK(comm_modem_session()==session); }
  for (unsigned i=0;i<50 && comm_modem_session()==session;++i) { transport_tick(10); comm_recover_poll(sim_now); }
  CHECK(!comm_call_active() && comm_modem_session()!=session); rc_clock=UINT32_MAX;
  /* Перезапуск извне снимает владельца AT; старый вызов не блокирует новый сеанс. */
  call_reset(); answer_mode=3; ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  CHECK(answers==1 && comm_modem_restart(false,sim_now)); rc_session(); advance(9000); CHECK(!comm_call_active());
  /* Вызов после SIM во время запуска: долгий QIACT не занимает окно ATA/CHUP. */
  call_reset(); CHECK(comm_modem_restart(false,sim_now)); sim_activation_delay=10000;
  comm_server_init(&test_server_handlers); comm_channel_receive(user_comm_rx,user_comm_rx_done);
  for (unsigned i=0; slm.attempt.status.step!=SLM_SIM; ++i) { CHECK(i<1000); transport_tick(10); }
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(200); CHECK(answers==1 && call.state==CALL_HOLD);
  uint8_t startup_step=slm.attempt.status.step; advance(500);
  CHECK(comm_modem_state()==SLM_STARTING && slm.attempt.status.step==startup_step && !chups);
  advance(1000); CHECK(chups==1 && hangup_at-answer_at==1040);
  /* Ожидаемый обрыв по URC: один обычный reconnect без роста ступеней. */
  for (unsigned loss=1;loss<=2;++loss) {
    call_reset(); call_drop=loss; call_drop_urc=1;
    ch.connections=3; ch.retry_step=2;
    rc.power_step=rc.soft_step=rc.net_step=2;
#if DOMOVOI_DIAG
    unsigned failures=slm.diag.failure_next;
#endif
    ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup();
    uint32_t elapsed=call_until_registered();
    CHECK(elapsed <= (loss==1 ? 1580u : 1660u) && opens==2);
    CHECK(!rc.soft_fails && rc.power_step==2 && rc.soft_step==2 && rc.net_step==2);
    DIAG_CHECK(slm.diag.failure_next==failures);
    CHECK(links==3 && link_kinds[1]==COMM_LINK_LOST && link_kinds[2]==COMM_LINK_UP);
    printf("comm_call: loss=%u URC hangup->server=%u ms\n",loss,(unsigned)elapsed);
  }
  /* Звонок на резерве: сначала тот же адрес; его отказ возвращает цепочку с основного. */
  for (unsigned fail=0;fail<2;++fail) {
    call_reset(); reserve_enabled=1; sim_open_error=1; reserve_works=true;
    comm_channel_reopen(0); until_online(); CHECK(comm_server_address()==1);
    unsigned first=opens; uint32_t began=sim_now;
    call_drop=call_drop_urc=1; reserve_works=!fail;
    ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup();
    if (!fail) {
      CHECK(call_until_registered()<=1580u && opens==first+1u && open_addresses[first]==1 && !pings && !event13);
    } else {
      for (unsigned i=0;opens<first+3u;++i) { CHECK(i<5000); transport_tick(10); comm_recover_poll(sim_now); }
      CHECK(open_addresses[first]==1 && !open_addresses[first+1u] && open_addresses[first+2u]==1);
      CHECK(pings && event13==1 && open_times[first+1u]-open_times[first]>=30000u && sim_now-began<50000u);
    }
  }
  /* Первая попытка после звонка не расходует счётчик: TCP закрывают до регистрации.
     При двух прежних попытках следующая идёт сразу, а после неё уже нужна пауза. */
  call_reset(); ch.connections=2; ch.retry_step=0;
  call_drop=call_drop_urc=1; sim_open_error=4;
  unsigned first_open=opens;
  ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup();
  for (unsigned i=0;opens<first_open+2u;++i) { CHECK(i<200); transport_tick(10); }
  CHECK(open_times[first_open+1u]-open_times[first_open]<1000u && !comm_channel_expected());
  for (unsigned i=0;opens<first_open+3u;++i) { CHECK(i<3300); transport_tick(10); }
  CHECK(open_times[first_open+2u]-open_times[first_open+1u]>=30000u);
  /* Без URC потерю TCP обнаруживает обычная отправка; дополнительного опроса нет. */
  call_reset(); call_drop=1;
  ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup(); advance(100);
  CHECK(opens==1 && !call_net_queries && !call_pdp_queries && !call_tcp_queries);
  user_send_statusPack(CH_SERVER);
  for (unsigned i=0;opens<2;++i) { CHECK(i<400); transport_tick(10); comm_recover_poll(sim_now); }
  uint32_t elapsed=call_until_registered(); CHECK(elapsed<=1580 && opens==2);
  printf("comm_call: TCP no URC normal send hangup->server=%u ms\n",(unsigned)elapsed);
  /* Потеря 4G и возврат: работает обычный запуск сети, без второго автомата. */
  for (unsigned wait=4000;wait<=6000;wait+=2000) {
    call_reset(); call_drop=2; call_drop_urc=1;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
    sim_network_wait=1;
    call_until_hangup(); call_network_due=hangup_at+wait;
    uint32_t elapsed=call_until_registered();
    CHECK(elapsed>=wait && elapsed<=wait+1000u); /* URC не ждёт ещё секунду опроса. */
    printf("comm_call: network=%u ms hangup->server=%u ms\n",wait,(unsigned)elapsed);
  }
  /* Первый отказ быстрой попытки возвращает обычные паузы и счёт. */
  call_reset(); call_drop=2; call_drop_urc=1; call_activation_error=1;
  ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup();
  for (unsigned i=0;rc.state!=RC_WAIT || !rc.wait_ms;++i) {
    CHECK(i<600); transport_tick(10); comm_recover_poll(sim_now);
  }
  CHECK(rc.wait_ms==10000 && rc.soft_fails==1 && call_activations==1);
  call_reset(); call_drop=1; call_drop_urc=1; sim_open_error=1;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(2500);
  CHECK(!comm_channel_expected() && opens==2);
  advance(5000); CHECK(pings==1 && event13==1 && opens==2);
  DIAG_CHECK(slm.diag.failure_next);
  puts("comm_call: access/ATA/CHUP, deadlines/CHUP, repeats, prompt, FW/night, ordinary recovery PASS");
}
