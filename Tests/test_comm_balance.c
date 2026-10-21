/* Доработка: Кряжев А., 2026. Баланс SIM по USSD на модели модема и канала.
   UCS2 ответы операторов собраны отдельно (Python).
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static void balance_case(const char *reply, unsigned early)
{
  server_reset(); ussd_code="*100#"; cusd_reply=reply; cusd_early=early; advance(303000);
}
/* USSD не мешает первой регистрации (плата 34: запрос при открытии -> отказ TCP).
   Это модель последствий, а не воспроизведение внутренней причины отказа SLM336. */
static void balance_startup_test(void)
{
  cusd_startup_fault=1; server_reset(); cusd_startup_fault=0;
  CHECK(comm_server_online() && !cusd_count);
  advance(299000u - (sim_now - sv.registration_since)); CHECK(!cusd_count);
  advance(5000); CHECK(cusd_count==1);
}
/* Пять минут считаются заново после принятой регистрации, включая переоткрытие. */
static void balance_reopen_test(void)
{
  server_reset(); ussd_code="*100#"; advance(290000); CHECK(!cusd_count);
  comm_channel_reopen(0); advance(5000); CHECK(comm_server_online() && !cusd_count);
  advance(290000); CHECK(!cusd_count);
  advance(15000); CHECK(cusd_count==1);
}
/* На каждом запрете проверяем запуск настоящего автомата, затем снимаем запрет. */
static void balance_busy_test(unsigned first, unsigned count)
{
  server_reset(); advance(301000); ussd_code="*100#";
  /* Дождаться естественной паузы CSQ/CMGL/статуса, не подменяя владельцев AT. */
  ussd_code="";
  for (unsigned i=0; i<1000 && (!comm_channel_idle() || comm_sms_busy() || sv.flags.status_wait); ++i) advance(10);
  CHECK(comm_channel_idle() && !comm_sms_busy()); ussd_code="*100#";
  for (unsigned busy=first; busy<first+count; ++busy) {
    switch (busy) {
    case 0: ch.state=CH_OPENING; break;
    case 1: ch.tx_state=TX_QUEUED; break;
    case 2: ++ch.sent; break;
    case 3: ch.flags.recv=1; break;
    case 4: ch.flags.read_active=1; ch.operation=OP_READ; break;
    case 5: ch.flags.urgent=1; break;
    case 6: ch.call.active=1; break;
    case 7: ch.operation=OP_OPEN; break;
    case 8: ch.probe=COMM_CH_PING; break;
    case 9: query.cmd="AT+CSQ"; break;
    case 10: msg.tx=1; break;
    case 11: msg.deliver=1; break;
    case 12: balance_app_idle=false; break;
    case 13: sv.flags.status_wait=1; break;
    case 14: sv.misses=1; break;
    }
    comm_balance_poll(sim_now, balance_app_idle);
    CHECK(bal.state==BAL_IDLE && !cusd_count);
    ch.state=CH_OPEN; ch.tx_state=TX_FREE; ch.sent=ch.acked;
    ch.flags.recv=ch.flags.read_active=ch.flags.urgent=ch.call.active=0;
    ch.operation=ch.probe=0; query.cmd=NULL; msg.tx=msg.deliver=0; balance_app_idle=true;
    sv.flags.status_wait=sv.misses=0;
  }
  advance(3000); CHECK(cusd_count==1);
}
static void balance_daily_test(void)
{
  /* Меню, отказ, ошибка команды, молчание и неизвестный текст: повтор через сутки. */
  static const char *const fails[] = {"+CUSD: 4\r\n", NULL, "+CUSD: 0,\"Error 30\",15\r\n",
    "+CUSD: 1,\"1-service 2-option\",15\r\n", "+CUSD: 0,\"Balance 5\",15\r\n"};
  for (unsigned i=0;i<5;++i) {
    server_reset(); ussd_code="*100#"; cusd_reply=fails[i]; cusd_error=i==4;
    advance(i==1 ? 333000 : 303000);
    if (cusd_count!=1 || balance_count) fprintf(stderr,"USSD failure case %u: sent=%u value=%u state=%u now=%u reg=%u server=%u idle=%u sms=%u side=%u/%u/%u next=%u\n",i,cusd_count,balance_count,bal.state,sim_now,sv.registration_since,sv.state,comm_channel_idle(),comm_sms_busy(),slm.side.queued,slm.side.running,slm.side.done,bal.next);
    CHECK(cusd_count==1 && !balance_count);
    CHECK(bal.next-sim_now>comm_timeouts[COMM_T_DAY]-5000u);
    advance(1800000); CHECK(cusd_count==1);
    /* Граница суточного срока: время только автомата баланса, AT ещё не исполняем. */
    comm_balance_poll(bal.next-1, true); CHECK(bal.state==BAL_IDLE);
    for (unsigned k=0; k<1000 && (!comm_channel_idle() || comm_sms_busy()); ++k) advance(10);
    comm_balance_poll(bal.next, true); CHECK(bal.state==BAL_ASK);
    advance(35000); CHECK(cusd_count==2 && bal.state==BAL_IDLE);
  }
}
static void balance_cancel_busy_test(void)
{
  server_reset(); advance(301000);
  for (unsigned i=0; i<1000 && (!comm_channel_idle() || comm_sms_busy()); ++i) advance(10);
  static const comm_side_t occupied={"AT+CSQ",NULL,NULL,0,COMM_AT_TEXT,COMM_T_AT_SIMPLE};
  CHECK(comm_modem_side(&occupied));
  bal.state=BAL_WAIT; bal.got=bal.menu=1;
  comm_balance_poll(sim_now,true); comm_balance_poll(sim_now,true);
  CHECK(bal.state==BAL_CLOSE && !cusd_cancel);
  advance(100); CHECK(comm_modem_side_result(&occupied)==COMM_AT_OK);
  advance(1000); CHECK(cusd_cancel==1 && bal.state==BAL_IDLE);
}
// Проверить разбор баланса, завершение меню и повтор запросов.
static void balance_tests(void)
{
  balance_startup_test(); balance_reopen_test(); balance_busy_test(0,15); balance_cancel_busy_test();
  /* Ответ в UCS2 после OK: "Баланс:123,45р". */
  balance_case("+CUSD: 0,\"04110430043B0430043D0441003A003100320033002C003400350440\",72\r\n",0);
  CHECK(cusd_count==1 && balance_count==1 && balance_value==123 && comm_server_online());
  /* Следующий запрос - через сутки, не раньше. */
  advance(60000); CHECK(cusd_count==1);
  CHECK(bal.next-sim_now<=comm_timeouts[COMM_T_DAY] && bal.next-sim_now>comm_timeouts[COMM_T_DAY]-120000u);
  /* Текст GSM; ответ до OK (как solicited-строка команды). */
  balance_case("+CUSD: 0,\"Balance 57.10 rub\",15\r\n",0); CHECK(balance_value==57);
  balance_case("+CUSD: 0,\"Balance 7 rub\",15\r\n",1); CHECK(balance_count==1 && balance_value==7);
  /* Сначала дата, потом слово "баланс"; минус - самое малое значение протокола. */
  balance_case("+CUSD: 0,\"041D0430002000300033002E00310030002004310430043B0430043D0441002000310030003000200440\",72\r\n",0);
  CHECK(balance_value==100);
  balance_case("+CUSD: 0,\"04110430043B0430043D0441003A0020002D00310032002C0033003000200440002E\",72\r\n",0);
  CHECK(balance_value==-12);
  /* Реклама длиннее строки AT: хвост отброшен, связь не рвётся. */
  balance_case("+CUSD: 0,\"04110430043B0430043D0441002000320035003000200440002E0020041F043E0434043A043B044E0447043804420435002004430441043B044304330443002004210443043F0435044000200418043D044204350440043D04350442002004370430002000390020044004430431043B0435043900200432002004340435043D044C002C002004370432043E043D0438044204350020002A0031003200330023\",72\r\n",0);
  CHECK(balance_value==250 && ch.state!=CH_FAULT && comm_server_online());
  /* Меню оператора закрывается AT+CUSD=2. */
  balance_case("+CUSD: 1,\"Balance 10 rub. 1-more\",15\r\n",0); advance(500);
  CHECK(!balance_count && cusd_cancel==1);
  server_reset(); ussd_code="*100#"; cusd_reply="+CUSD: 1,\"1-service\",15\r\n"; cusd_cancel_error=1;
  advance(305000); CHECK(cusd_cancel==1 && bal.state==BAL_IDLE && !balance_count && comm_server_online());
  CHECK(bal.next-sim_now>comm_timeouts[COMM_T_DAY]-10000u);
  balance_daily_test();
  /* Не USSD в настройках - модем не спрашиваем. */
  server_reset(); ussd_code="*100#;AT"; advance(305000); CHECK(!cusd_count);
  server_reset(); ussd_code=""; advance(305000); CHECK(!cusd_count);
  /* Разбор текста напрямую. */
  int32_t v;
  CHECK(comm_balance_parse("\xC2\xE0\xF8 \xE1\xE0\xEB\xE0\xED\xF1 1234.56 \xF0.",&v) && v==1234);
  CHECK(!comm_balance_parse("\xCE\xF8\xE8\xE1\xEA\xE0",&v));
  /* Короткий хвост, перекрывающиеся начала и приоритет русского слова. */
  CHECK(!comm_balance_parse("",&v));
  CHECK(!comm_balance_parse("alanc",&v));
  CHECK(!comm_balance_parse("Error code 30",&v));
  CHECK(comm_balance_parse("  -12.30 rub",&v) && v==-12);
  CHECK(comm_balance_parse("7 balalance -12.30",&v) && v==-12);
  CHECK(comm_balance_parse("\xE0\xEB\xE0\xED\xF1 5 balance 8",&v) && v==5);
  CHECK(comm_balance_parse("balance 8 \xE0\xEB\xE0\xED\xF1 5",&v) && v==5);
  puts("comm_balance: first registration, 5 minutes/reopen, busy guards, UCS2/GSM, menu/error/timeout daily PASS");
}
