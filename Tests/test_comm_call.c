/* Доработка: Кряжев А., 2026. Входящий вызов без ответа на модели модема и канала. */
static unsigned calls, chups_at_call; static char call_seen[24];
static void call_caller(const char *number)
{
  ++calls; chups_at_call=chups; CHECK(strlen(number)<sizeof(call_seen));
  memcpy(call_seen,number,strlen(number)+1u);
}
static bool user_comm_busy_model(void) { return comm_call_active(); }
static void call_reset(void) { server_reset(); calls=0; call_seen[0]=0; }
/* Вызов звонит, пока не сброшен: RING (+CLIP) каждые 5 с. */
static void ring(const char *clip) { sim_feed("RING\r\n"); if (clip) sim_feed(clip); }
static void call_tests(void)
{
  /* RING, затем CLIP: открыть один раз, потом сбросить, без ATA и без обрыва TCP. */
  call_reset(); unsigned opened=opens;
  ring("+CLIP: \"+79161234567\",145,\"\",0,\"\",0\r\n"); advance(200);
  CHECK(calls==1 && !strcmp(call_seen,"+79161234567") && chups_at_call==0 && chups==1);
  CHECK(comm_call_active() && comm_server_online());
  /* Повтор того же вызова - только сброс, второго открытия нет. */
  advance(4800); ring("+CLIP: \"+79161234567\",145,\"\",0,\"\",0\r\n"); advance(200);
  CHECK(calls==1 && chups==2);
  advance(9000); CHECK(!comm_call_active() && opens==opened && comm_server_online());
  /* Тот же номер после тишины - новый вызов. */
  ring("+CLIP: \"+79161234567\",145,\"\",0,\"\",0\r\n"); advance(200); CHECK(calls==2 && chups==3);
  /* CLIP раньше RING. */
  call_reset(); sim_feed("+CLIP: \"89035551122\",129,\"\",0,\"\",0\r\nRING\r\n"); advance(200);
  CHECK(calls==1 && !strcmp(call_seen,"89035551122") && chups==1);
  /* Номер скрыт или CLIP потерян: через 1 с сброс без открытия. */
  call_reset(); ring(NULL); advance(500); CHECK(!calls && !chups);
  advance(700); CHECK(!calls && chups==1);
  /* Пустой номер передаётся проверке как есть (старое поведение), мусорный - нет. */
  call_reset(); ring("+CLIP: \"\",128,\"\",0,\"\",0\r\n"); advance(200); CHECK(calls==1 && !call_seen[0]);
  call_reset(); ring("+CLIP: \"+7916x\",145\r\n"); advance(1300); CHECK(!calls && chups==1);
  call_reset(); ring("+CLIP: \"+791612345678901234567\",145\r\n"); advance(1300); CHECK(!calls && chups==1);
  /* Во время сброса звонит другой номер: открыть и для него. */
  call_reset(); chup_silence=1; ring("+CLIP: \"+79160000001\",145\r\n"); advance(100);
  CHECK(calls==1 && chups==1);
  ring("+CLIP: \"+79160000002\",145\r\n"); advance(100); CHECK(calls==2 && !strcmp(call_seen,"+79160000002"));
  chup_silence=0; advance(6000); CHECK(chups==2);
  /* Сброс не проходит, вызов жив: следующий RING сбрасывает снова, без открытия. */
  call_reset(); chup_silence=1; ring("+CLIP: \"+79160000003\",145\r\n"); advance(5500);
  CHECK(calls==1 && chups==1 && comm_call_active());
  chup_silence=0; ring("+CLIP: \"+79160000003\",145\r\n"); advance(200); CHECK(calls==1 && chups==2);
  /* Перезапуск модема посреди сброса: ожидание снимается, вызов не зависает. */
  call_reset(); chup_silence=1; ring("+CLIP: \"+79160000004\",145\r\n"); advance(100);
  CHECK(chups==1 && comm_modem_restart(false,sim_now));
  CHECK(slm.side.done && slm.side.result==COMM_AT_ERROR); comm_call_poll(sim_now); CHECK(comm_call_active() && !slm.side.done);
  chup_silence=0; advance(9000); CHECK(!comm_call_active() && calls==1);
  /* Вызов посреди передачи кадра: сброс ждёт, пока после '>' уйдут данные. */
  call_reset(); prompt_silence=1;
  uint16_t cap; uint8_t *p=comm_tx_acquire(&cap); CHECK(p); memset(p,0x5A,12); CHECK(comm_tx_commit(12));
  for (unsigned i=0;ch.tx_state!=TX_PROMPT && i<100;++i) transport_tick(10);
  CHECK(ch.tx_state==TX_PROMPT);
  ring("+CLIP: \"+79161112233\",145\r\n"); advance(50);
  CHECK(calls==1 && !chups && slm.side.queued); /* сброс ждёт ответа на QISEND */
  sim_feed("> "); advance(500); CHECK(chups==1 && sent_count==12 && !comm_tx_busy());
  /* Вызов виден как занятость (ночной перезапуск модема ждёт его конца). */
  call_reset(); CHECK(!user_comm_busy_model()); ring(NULL); advance(20); CHECK(comm_call_active());
  puts("comm_call: RING/CLIP order, one open per call, hidden/bad number, repeat hangup, restart, prompt guard PASS");
}
