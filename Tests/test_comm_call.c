/* Доработка: Кряжев А., 2026. Доступ до ATA, ограниченный ответ и возврат данных.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static unsigned calls, chups_at_call, answers_at_call, call_actions; static char call_seen[24];
static uint32_t call_action_at;
// Запомнить номер и результат проверки доступа до ответа.
static bool call_caller(const char *number)
{
  ++calls; chups_at_call=chups; answers_at_call=answers; call_action_at=sim_now;
  CHECK(strlen(number)<sizeof(call_seen)); memcpy(call_seen,number,strlen(number)+1u);
  /* Модель допуска: скрытый и чужой номер не открывают. Производственный доступ прежний. */
  if (number[0] && strcmp(number,"+79990000000")) ++call_actions;
  return true;
}
// Смоделировать занятость приложения обновлением прошивки.
static bool user_comm_busy_model(void) { return comm_call_active(); }
// Сбросить наблюдения и модель вызова.
static void call_reset(void) { server_reset(); comm_recover_init(&rc_hooks,sim_now); calls=call_actions=0; call_seen[0]=0; call_action_at=0; }
// Подать номер и сигнал входящего вызова.
static void ring(const char *clip) { sim_feed("RING\r\n"); if (clip) sim_feed(clip); }
// Продвинуть модель до физической отправки отбоя.
static void call_until_hangup(void)
{
  for (unsigned i=0; !aths && !chups; ++i) { CHECK(i<400); transport_tick(10); }
  CHECK(hangup_at-answer_at<=comm_timeouts[COMM_T_CALL_LIMIT]);
}
// Продвинуть модель до возврата серверной регистрации.
static uint32_t call_until_registered(void)
{
  for (unsigned i=0; !comm_server_online() || comm_channel_call_pending(); ++i) { CHECK(i<1200); transport_tick(10); comm_recover_poll(sim_now); }
  return sim_now-hangup_at;
}
// Проверить доступ, сроки ATA/ATH и восстановление данных после вызова.
static void call_tests(void)
{
  call_reset(); unsigned opened=opens;
  ring("+CLIP: \"+79161234567\",145,\"\",0,\"\",0\r\n"); advance(1700);
  CHECK(calls==1 && call_actions==1 && !strcmp(call_seen,"+79161234567"));
  CHECK(!answers_at_call && !chups_at_call && call_action_at<answer_at);
  CHECK(answers==1 && aths==1 && !chups && hangup_at-answer_at>=1040 && hangup_at-answer_at<=1070);
  CHECK(comm_call_active() && comm_server_online() && opens==opened && !closes);
  CHECK(!call_net_queries && !call_pdp_queries && !call_tcp_queries);
  advance(3300); ring("+CLIP: \"+79161234567\",145\r\n"); advance(200);
  CHECK(calls==1 && answers==1 && aths==1 && !chups);
  advance(8100); CHECK(!comm_call_active());
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700); CHECK(calls==2 && answers==2 && aths==2);
  call_reset(); sim_feed("+CLIP: \"89035551122\",129\r\nRING\r\n"); advance(1700);
  CHECK(calls==1 && !strcmp(call_seen,"89035551122") && answers==1 && aths==1);
  call_reset(); ring(NULL); advance(500); CHECK(!calls && !answers);
  advance(2300); CHECK(!calls && !call_actions && answers==1 && aths==1);
  call_reset(); ring("+CLIP: \"\",128\r\n"); advance(1700);
  CHECK(calls==1 && !call_actions && !call_seen[0] && answers==1 && aths==1);
  call_reset(); ring("+CLIP: \"+79990000000\",145\r\n"); advance(1700);
  CHECK(calls==1 && !call_actions && answers==1 && aths==1);
  call_reset(); ring("+CLIP: \"+7916x\",145\r\n"); advance(2800); CHECK(!calls && answers==1 && aths==1);
  call_reset(); ring("+CLIP: \"+791612345678901234567\",145\r\n"); advance(2800); CHECK(!calls && answers==1 && aths==1);
  /* Занятая флеш задерживает только действие до ATA, а не таймер снятой трубки. */
  call_reset(); call_access_ready=false; ring("+CLIP: \"+79161234567\",145\r\n"); advance(2000);
  CHECK(!calls && !answers); call_access_ready=true; advance(100); CHECK(calls==1 && answers==1);
  call_access_ready=false; advance(1600); CHECK(aths==1 && !chups);
  call_reset(); ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  sim_feed("NO CARRIER\r\n"); advance(20); CHECK(chups==1 && !aths && hangup_at-answer_at<1500);
  /* Сроки переживают переполнение millis. */
  call_reset(); ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
  uint32_t shift=UINT32_MAX-sim_now-200u;
  sim_now+=shift; call.now+=shift; call.since+=shift; call.last+=shift; call.answered+=shift;
  ch.now+=shift; sv.now+=shift; sv.status_since+=shift; answer_at+=shift; started+=shift;
  advance(1600); CHECK(answers==1 && aths==1 && !chups && hangup_at-answer_at<=2500);
  /* Поздний OK оставляет запас: абсолютный предел не сдвигается. */
  call_reset(); answer_delay=1490; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
  CHECK(calls==1 && answers==1 && hangup_at-answer_at<=2500); advance(500);
  CHECK(aths==1 && chups==1 && hangup_at-answer_at==2500);
  for (unsigned failure=1;failure<=3;++failure) {
    call_reset(); answer_mode=failure; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
    CHECK(calls==1 && answers==1 && !aths && chups==1);
    CHECK(hangup_at-answer_at==(failure==3 ? 1500u : 40u)); advance(500);
    CHECK(comm_server_online() && !comm_channel_call_pending());
  }
  /* Старый OK после CHUP не является ответом следующего запроса. */
  call_reset(); answer_delay=1800; ring("+CLIP: \"+79160000001\",145\r\n"); call_until_hangup();
  CHECK(chups==1 && hangup_at-answer_at==1500); advance(800); CHECK(comm_server_online());
  call_reset(); answer_delay=1400; ath_silence=1;
  ring("+CLIP: \"+79160000001\",145\r\n"); advance(2700);
  CHECK(answers==1 && aths==1 && chups==1 && hangup_at-answer_at==2500);
  call_reset(); ring("+CLIP: \"+79160000001\",145\r\n"); advance(100);
  ring("+CLIP: \"+79160000002\",145\r\n"); advance(100);
  CHECK(calls==2 && !strcmp(call_seen,"+79160000002") && answers==1);
  advance(2800); CHECK(calls==2 && answers==2 && aths==2);
  /* Между приглашением и данными нет ATA/ATH, включая одновременный closed URC. */
  call_reset(); prompt_silence=1;
  uint16_t cap; uint8_t *p=comm_tx_acquire(&cap); CHECK(p); memset(p,0x5A,12); CHECK(comm_tx_commit(12));
  for (unsigned i=0;ch.tx_state!=TX_PROMPT && i<100;++i) transport_tick(10);
  CHECK(ch.tx_state==TX_PROMPT);
  ring("+CLIP: \"+79161112233\",145\r\n"); advance(50);
  CHECK(calls==1 && !answers && slm.side.queued);
  sim_feed("> "); advance(1700); CHECK(answers==1 && aths==1 && sent_count==12 && !comm_tx_busy());
  call_reset(); prompt_silence=1;
  p=comm_tx_acquire(&cap); CHECK(p); memset(p,0x5A,12); CHECK(comm_tx_commit(12));
  for (unsigned i=0;ch.tx_state!=TX_PROMPT && i<100;++i) transport_tick(10);
  ring("+CLIP: \"+79161112233\",145\r\n"); advance(50); CHECK(!answers);
  prompt_silence=0; socket_model=false; sim_feed("+QIURC: \"closed\",0\r\n> "); advance(2000);
  CHECK(calls==1 && answers==1 && aths==1 && opens==2 && comm_server_online());
  /* Обновление FW не запрещает телефонный доступ/ответ. */
  call_reset(); device.status|=1u<<11; device.firmwarePercent=37;
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(1700); CHECK(calls==1 && answers==1 && aths==1);
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
  /* Вызов после SIM во время запуска: долгий QIACT не занимает окно ATA/ATH. */
  call_reset(); CHECK(comm_modem_restart(false,sim_now)); sim_activation_delay=10000;
  comm_server_init(&test_server_handlers); comm_channel_receive(user_comm_rx,user_comm_rx_done);
  for (unsigned i=0; slm.attempt.status.step!=SLM_SIM; ++i) { CHECK(i<1000); transport_tick(10); }
  ring("+CLIP: \"+79161234567\",145\r\n"); advance(200); CHECK(answers==1 && call.state==CALL_HOLD);
  uint8_t startup_step=slm.attempt.status.step; advance(500);
  CHECK(comm_modem_state()==SLM_STARTING && slm.attempt.status.step==startup_step && !aths);
  advance(1000); CHECK(aths==1 && !chups && hangup_at-answer_at==1040);
  /* Ожидаемый обрыв по URC: один обычный reconnect без роста ступеней. */
  for (unsigned loss=1;loss<=2;++loss) {
    call_reset(); call_drop=loss; call_drop_urc=1;
    ch.connections=3; ch.retry_step=2; ch.address_failures=2;
    rc.power_step=rc.soft_step=rc.net_step=2;
#if DOMOVOI_DIAG
    unsigned failures=slm.diag.failure_next;
#endif
    ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup();
    uint32_t elapsed=call_until_registered();
    CHECK(elapsed <= (loss==1 ? 1580u : 1660u) && opens==2);
    CHECK(!rc.soft_fails && !rc.tcp_count && rc.power_step==2 && rc.soft_step==2 && rc.net_step==2);
    DIAG_CHECK(slm.diag.failure_next==failures);
    CHECK(links==3 && link_kinds[1]==COMM_LINK_LOST && link_kinds[2]==COMM_LINK_UP);
    printf("comm_call: loss=%u URC hangup->server=%u ms\n",loss,(unsigned)elapsed);
  }
  /* Без URC потерю TCP обнаруживает обычная отправка; дополнительного опроса нет. */
  call_reset(); call_drop=1;
  ring("+CLIP: \"+79161234567\",145\r\n"); call_until_hangup(); advance(100);
  CHECK(opens==1 && !call_net_queries && !call_pdp_queries && !call_tcp_queries);
  user_send_statusPack(CH_SERVER);
  for (unsigned i=0;opens<2;++i) { CHECK(i<400); transport_tick(10); comm_recover_poll(sim_now); }
  uint32_t elapsed=call_until_registered(); CHECK(elapsed<=1580 && opens==2 && !rc.tcp_count);
  printf("comm_call: TCP no URC normal send hangup->server=%u ms\n",(unsigned)elapsed);
  /* Потеря 4G и возврат: работает обычный запуск сети, без второго автомата. */
  for (unsigned wait=4000;wait<=6000;wait+=2000) {
    call_reset(); call_drop=2; call_drop_urc=1;
    ring("+CLIP: \"+79161234567\",145\r\n"); advance(100);
    sim_network_wait=1;
    call_until_hangup(); call_network_due=hangup_at+wait;
    uint32_t elapsed=call_until_registered();
    CHECK(elapsed>=wait && elapsed<=wait+1520u);
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
  CHECK(!comm_channel_expected() && ch.retry_step==1 && ch.reopen_pause==30000 && opens==5);
  DIAG_CHECK(slm.diag.failure_next);
  puts("comm_call: access/ATA/ATH, deadlines/CHUP, repeats, prompt, FW/night, ordinary recovery PASS");
}
