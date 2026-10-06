/* Доработка: Кряжев А., 2026. Баланс SIM по USSD на модели модема и канала.
   UCS2 ответы операторов собраны отдельно (Python).
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static void balance_case(const char *reply, unsigned early)
{
  server_reset(); ussd_code="*100#"; cusd_reply=reply; cusd_early=early; advance(3000);
}
// Проверить разбор баланса, завершение меню и повтор запросов.
static void balance_tests(void)
{
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
  CHECK(balance_value==10 && cusd_cancel==1);
  /* Отказ сети, ошибка команды, молчание, текст без числа: значение не трогаем, повтор через 30 мин. */
  static const char *const fails[] = {"+CUSD: 4\r\n", NULL, "+CUSD: 0,\"Error\",15\r\n"};
  for (unsigned i=0;i<4;++i) {
    server_reset(); ussd_code="*100#"; cusd_reply=i<3 ? fails[i] : "+CUSD: 0,\"Balance 5\",15\r\n"; cusd_error=i==3;
    advance(i==1 ? 33000 : 3000);
    CHECK(cusd_count==1 && !balance_count);
    advance(comm_timeouts[COMM_T_USSD_RETRY]); CHECK(cusd_count==2);
  }
  /* Не USSD в настройках - модем не спрашиваем. */
  server_reset(); ussd_code="*100#;AT"; advance(5000); CHECK(!cusd_count);
  server_reset(); ussd_code=""; advance(5000); CHECK(!cusd_count);
  /* Разбор текста напрямую. */
  int32_t v;
  CHECK(comm_balance_parse("\xC2\xE0\xF8 \xE1\xE0\xEB\xE0\xED\xF1 1234.56 \xF0.",&v) && v==1234);
  CHECK(!comm_balance_parse("\xCE\xF8\xE8\xE1\xEA\xE0",&v));
  puts("comm_balance: UCS2/GSM, before/after OK, date first, minus, long ad cut, menu close, failures retry, daily PASS");
}
