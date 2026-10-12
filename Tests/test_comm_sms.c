/* Доработка: Кряжев А., 2026. SMS в режиме PDU на модели модема и канала.
   PDU приёма и отправки собраны отдельно (Python, не этим кодом); hellohello - пример
   SMS-DELIVER из открытого описания формата. */
static char sms_got_number[24], sms_got_text[SMS_TEXT + 1u];
static const char *sms_reply_to;
static bool sms_hold;
// Запомнить принятую SMS для сравнения с независимым эталоном.
static bool sms_received_model(char *number, char *text, bool *first)
{
  sms_events+=*first; *first=false;
  if (sms_hold) return false;
  if (!*text) { strcpy(sms_got_number,number); return true; }
  ++sms_received_count;
  CHECK(strlen(number) < sizeof(sms_got_number) && strlen(text) <= SMS_TEXT);
  strcpy(sms_got_number, number); strcpy(sms_got_text, text);
  if (sms_reply_to) CHECK(comm_sms_send(number, sms_reply_to));
  return true;
}
static const char pdu_cmd7[] = "07919730071111F1040B919761214365F70000623001210000210DA3393D4CAFCF41B4992CC602";
static const char pdu_ucs2[] = "07919730071111F10406D04DEA140008623001210000212004110430043B0430043D0441002000310030003000200440002E002004010451";
static const char pdu_concat[] = "07919730071111F1440B819830551521F20000623001210000210F0500032A0201E061391DF4769701";
static const char pdu_bit8[] = "07919730071111F1040B919761000000F10004623001210000210541420D0A43";
static const char pdu_long7[] = "07919730071111F1040B919761000000F2000062300121000021A0783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1783C1E8FC7E3F1";
static const char pdu_hello[] = "07917283010010F5040BC87238880900F10000993092516195800AE8329BFD4697D9EC37";
static const char pdu_inject[] = "07919730071111F1040B919761000000F500006230012100002119AB213309D58144AB5B2E6683C16030182C2762C56835";
static const char pdu_submit_type[] = "07919730071111F101000B919761214365F700000141";
static const struct { const char *number, *text, *pdu; } sms_phone[] = { /* СЦЕНАРИИ SMS SLM336 */ };
// Сбросить очередь и наблюдения модели SMS.
static void sms_reset(void)
{
  memset(sim_store, 0, sizeof(sim_store)); memset(sim_me, 0, sizeof(sim_me));
  cnmi_error=cmgf_error=cmgr_error=cmgd_error=cpms_error=0; sms_hold=false; sms_reply_to=NULL; call_reset();
  sms_got_number[0]=sms_got_text[0]=0;
}
/* Новая SMS: модем кладёт на SIM и сообщает ячейку. */
static void sms_arrive(unsigned cell, const char *pdu)
{
  char urc[32]; sim_store[cell]=pdu;
  snprintf(urc,sizeof(urc),"+CMTI: \"SM\",%u\r\n",cell); sim_feed(urc); advance(300);
}
// Проверить принятую SMS и необходимые операции SIM.
static void sms_expect(const char *number, const char *text)
{
  CHECK(sms_received_count==1 && !strcmp(sms_got_number,number) && !strcmp(sms_got_text,text));
}
// Проверить PDU, кодировки, границы и очередь отправки SMS.
static void sms_tests(void)
{
  /* SMS читается и стирается на SIM при отказе PDP, без отмены длинной паузы. */
  sms_reset(); comm_server_no_internet(comm_recover_no_internet);
  sim_error_step=SLM_ACTIVATE; rc_fresh(); rc_wait_power();
  unsigned powers=rc_power_starts; sms_arrive(1,pdu_hello); advance(1000);
  sms_expect("27838890001","hellohello");
  CHECK(!sim_store[1] && rc.state==RC_WAIT && rc_power_starts==powers && sim_power_on);

  /* Непринятое SMS остаётся на SIM и в текущем буфере до освобождения канала. */
  sms_reset();sms_hold=true;sms_arrive(1,pdu_hello);advance(1000);
  CHECK(!sms_received_count && sim_store[1] && !strstr(sms_log,"AT+CMGD=1;"));
  sms_hold=false;advance(1000);CHECK(sms_received_count==1 && !sim_store[1]);

  /* Настройка после SIM: PDU, память SIM, +CMTI, затем обзор SIM. */
  sms_reset();
  CHECK(strstr(sms_log,"AT+CMGF=0;AT+CPMS=\"SM\",\"SM\",\"SM\";AT+CNMI=0,1,0,0,0;AT+CMGL=4;"));
  CHECK(comm_server_online());
  sms_arrive(1,pdu_cmd7); sms_expect("+79161234567","#status 4321,");
  CHECK(!sim_store[1] && strstr(sms_log,"AT+CMGR=1;AT+CMGD=1;"));
  /* Кириллица UCS2 от буквенного отправителя -> cp1251. */
  sms_reset(); sms_arrive(3,pdu_ucs2);
  sms_expect("MTS","\xC1\xE0\xEB\xE0\xED\xF1 100 \xF0. \xA8\xB8");
  /* Часть склейки: заголовок и заполнение пропущены. 8 бит - как есть. */
  sms_reset(); sms_arrive(2,pdu_concat); sms_expect("89035551122","part one");
  sms_reset(); sms_arrive(2,pdu_bit8); sms_expect("+79160000001","AB\r\nC");
  sms_reset(); sms_arrive(2,pdu_hello); sms_expect("27838890001","hellohello");
  /* 160 символов: тело длиннее строки AT приходит кусками. */
  sms_reset(); sms_arrive(9,pdu_long7);
  { char x[161]; memset(x,'x',160); x[160]=0; sms_expect("+79160000002",x); }
  /* Текст SMS не становится ответом модема: поддельный +CLIP не открывает. */
  sms_reset(); sms_arrive(4,pdu_inject); sms_expect("+79160000005","+CLIP: \"+79160000009\",145");
  advance(2000); CHECK(!calls && !chups && !comm_call_active());
  /* Битая, не DELIVER и пустая ячейка: не отдаются, ячейка стирается. */
  static const char *const bad[] = {pdu_submit_type, "07919730071111F1040B9197612143", "0791973007ZZ"};
  for (unsigned i=0;i<3;++i) {
    sms_reset(); sms_arrive(5,bad[i]);
    CHECK(!sms_received_count && sms_events==1 && !sim_store[5] && strstr(sms_log,"AT+CMGD=5;"));
  }
  sms_reset(); sim_feed("+CMTI: \"SM\",7\r\n"); advance(300);
  CHECK(!sms_received_count && !sms_events && strstr(sms_log,"AT+CMGR=7;") && !strstr(sms_log,"AT+CMGD=7;"));
  CHECK(!msg.count);
  /* Лежавшие на SIM до запуска модема - прочитаны и стёрты. */
  memset(sim_store,0,sizeof(sim_store)); sim_store[2]=pdu_cmd7; sim_store[6]=pdu_hello;
  call_reset(); advance(500);
  CHECK(sms_received_count==2 && !sim_store[2] && !sim_store[6] && !strcmp(sms_got_text,"hellohello"));
  /* Отправка: GSM 7 бит, UCS2, обрезка; длина в CMGS - без адреса SMSC. */
  sms_reset(); CHECK(comm_sms_send("+79161234567","status: ok 1")); CHECK(!comm_sms_send("+79161234567","x"));
  advance(300);
  CHECK(strstr(sms_log,"AT+CMGS=25;") && !strcmp(sms_sent_pdu,"0011000B919761214365F70000AA0C737A985E9FEB40EF352806"));
  CHECK(comm_sms_send("79161234567","\xCF\xF0\xE8\xE2\xE5\xF2")); advance(300);
  CHECK(strstr(sms_log,"AT+CMGS=26;") && !strcmp(sms_sent_pdu,"0011000B919761214365F70008AA0C041F04400438043204350442"));
  { char a[131]; memset(a,'A',130); a[130]=0; CHECK(comm_sms_send("1234",a)); }
  advance(300);
  CHECK(strstr(sms_log,"AT+CMGS=110;") && !strncmp(sms_sent_pdu,"001100048121430000AA72C16030180C0683",36) && strlen(sms_sent_pdu)==222);
  CHECK(!comm_sms_send("+7916x",".") && !comm_sms_send("",".") && !comm_sms_send("123456789012345678901","."));
  /* Только национальные 11 цифр с 8 получают 0x81; '+' и прочие формы прежние. */
  { static const struct { const char *number, *prefix; } cases[] = {
      {"89035551122", "0011000B819830551521F2"},
      {"+89035551122", "0011000B919830551521F2"},
      {"79035551122", "0011000B919730551521F2"},
      {"8903555112", "0011000A819830551521"},
      {"890355511222", "0011000C91983055152122"}
    };
    for (unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
      sms_reset(); CHECK(comm_sms_send(cases[i].number,"x")); advance(300);
      CHECK(!strncmp(sms_sent_pdu,cases[i].prefix,strlen(cases[i].prefix)));
    }
  }
  /* Ответ из обработчика уходит; отказ сети освобождает очередь. */
  sms_reset(); sms_reply_to="#status: Ok"; sms_arrive(1,pdu_cmd7); advance(300);
  CHECK(strstr(sms_log,"AT+CMGD=1;AT+CMGS=") && comm_server_online());
  sms_reset(); sms_reply_to="ADD: Error"; sms_arrive(1,pdu_concat); advance(300);
  CHECK(!strncmp(sms_sent_pdu,"0011000B819830551521F2",20));
  cmgs_error=1; CHECK(comm_sms_send("+79161234567","x")); advance(300); CHECK(comm_sms_send("+79161234567","y"));
  /* Перезапуск модема: настройка заново, ожидающая команда снимается. */
  sms_reset(); CHECK(comm_modem_restart(false,sim_now)); sms_log[0]=0;
  for (unsigned i=0;!strstr(sms_log,"AT+CMGL=4;") && i<3000;++i) transport_tick(10);
  CHECK(strstr(sms_log,"AT+CMGF=0;") && strstr(sms_log,"AT+CMGL=4;"));
  /* Передача кадра и отправка SMS не смешиваются после '>'. */
  sms_reset();
  for (unsigned k=0;k<20;++k) {
    uint16_t cap; uint8_t *p=comm_tx_acquire(&cap);
    if (p) { memset(p,0x33,12); CHECK(comm_tx_commit(12)); }
    if (k%3==0) comm_sms_send("+79161234567","ping");
    advance(70);
  }
  advance(2000); CHECK(comm_server_online() && !comm_tx_busy());
  /* Телефон: OA +7, RP/MMS, GSM/UCS2, UDH; нулевая ячейка. */
  for (unsigned i=0;i<sizeof(sms_phone)/sizeof(sms_phone[0]);++i) {
    sms_reset();sms_arrive(0,sms_phone[i].pdu);sms_expect(sms_phone[i].number,sms_phone[i].text);
    CHECK(sms_events==1 && !sim_store[0] && strstr(sms_log,"AT+CMGR=0;AT+CMGD=0;"));
    DIAG_CHECK(msg.diag.cmti==1 && msg.diag.index==0x100 && msg.diag.read==COMM_AT_OK && msg.diag.stop==P_END && msg.diag.deleted==1);
  }
  /* CPMS: пустая и полная SIM; очередь 8 — повторный CMGL. */
  sms_reset();DIAG_CHECK(msg.diag.used==0 && msg.diag.total==11);
  for(unsigned i=0;i<11;++i)sim_store[i]=i==4?pdu_submit_type:sms_phone[0].pdu;
  call_reset();advance(4000);
  CHECK(msg.ready==1 && sms_events==11 && sms_received_count==10);
  for(unsigned i=0;i<11;++i)CHECK(!sim_store[i]);
  DIAG_CHECK(msg.diag.used==11 && msg.diag.total==11 && msg.diag.cmgl>=11 && msg.diag.deleted==11);
  /* CNMI ERROR: обзор остаётся рабочим, URC не обязательна. */
  sms_reset();cnmi_error=1;sim_store[1]=sms_phone[0].pdu;call_reset();advance(1000);
  CHECK(msg.ready==2 && sms_events==1 && !sim_store[1]);DIAG_CHECK(msg.diag.setup==COMM_AT_ERROR);
  sim_store[2]=sms_phone[1].pdu;advance(16000);CHECK(sms_events==2 && !sim_store[2]);
  sms_reset();cmgf_error=1;call_reset();CHECK(!msg.ready && msg.step==0);
  advance(31000);CHECK(msg.ready==1);
  sms_reset();cpms_error=1;sim_store[0]=sms_phone[0].pdu;call_reset();CHECK(!msg.ready && msg.step==1);
  DIAG_CHECK(msg.diag.setup==COMM_AT_CME && msg.diag.cms==313);
  advance(31000);CHECK(msg.ready==1 && sms_events==1 && !sim_store[0]);
  /* Индексы ME/MT выбирают свою mem1. */
  sms_reset();sim_me[5]=sms_phone[0].pdu;sim_feed("+CMTI: \"ME\",5\r\n");advance(500);
  sms_expect(sms_phone[0].number,sms_phone[0].text);CHECK(!sim_me[5]);
  sim_store[2]=sms_phone[0].pdu;sim_feed("+CMTI: \"MT\",2\r\n");advance(500);CHECK(sms_events==2 && !sim_store[2]);
  /* Транзитная ошибка чтения/удаления оставляет ячейку. */
  sms_reset();cmgr_error=1;sms_arrive(0,sms_phone[0].pdu);CHECK(!sms_events && sim_store[0]);
  advance(31000);CHECK(sms_events==1 && !sim_store[0]);
  sms_reset();cmgd_error=1;sms_arrive(0,sms_phone[0].pdu);CHECK(sms_events==1 && sim_store[0]);
  advance(31000);CHECK(sms_events==1 && !sim_store[0]);
  sms_reset();sms_hold=true;sms_arrive(0,sms_phone[0].pdu);advance(1000);CHECK(sms_events==1 && sim_store[0]);
  sms_hold=false;advance(500);CHECK(sms_events==1 && !sim_store[0]);
  /* Неожиданная +CMT: отдельное тело, при занятом PDU — событие без команды. */
  sms_reset();sim_feed("+CMT: ,37\r\n");sim_feed(sms_phone[0].pdu);sim_feed("\r\n");advance(300);
  sms_expect(sms_phone[0].number,sms_phone[0].text);CHECK(sms_events==1 && !strstr(sms_log,"AT+CMGD="));
  sms_reset();sim_feed("+CMT: ,159\r\n");sim_feed(pdu_long7);sim_feed("\r\n");advance(300);
  {char expected[161];memset(expected,'x',160);expected[160]=0;sms_expect("+79160000002",expected);CHECK(sms_events==1);}
  sms_reset();sms_hold=true;sms_arrive(1,sms_phone[0].pdu);
  sim_feed("+CMT: ,37\r\n");sim_feed(sms_phone[0].pdu);sim_feed("\r\n");advance(300);
  sms_hold=false;advance(500);CHECK(sms_events==2 && sms_received_count==1 && !sim_store[1]);
  /* Разрыв между заголовком CMT и телом запрещает чужой AT; обрыв даёт событие. */
  sms_reset();for(unsigned i=0;comm_at_busy() && i<100;++i)advance(10);
  sim_feed("+CMT: ,37\r\n");advance(20);CHECK(comm_at_busy() && msg.direct);
  advance(500);CHECK(msg.direct && !sms_events);
  sim_feed(sms_phone[0].pdu);sim_feed("\r\n");advance(300);
  sms_expect(sms_phone[0].number,sms_phone[0].text);CHECK(sms_events==1);
  sms_reset();for(unsigned i=0;comm_at_busy() && i<100;++i)advance(10);
  sim_feed("+CMT: ,37\r\n");advance(2200);CHECK(sms_events==1 && !sms_received_count && !msg.direct);
  /* DCS, UDH, лишний/неполный октет: событие и удаление, команды нет. */
  { char pdu[300];for(unsigned i=0;i<4;++i) {
      strcpy(pdu,sms_phone[i==1?2:0].pdu);
      if(i==0){pdu[36]='2';pdu[37]='0';}
      if(i==1){pdu[54]='F';pdu[55]='F';}
      if(i==2)strcat(pdu,"00");if(i==3)pdu[strlen(pdu)-1]=0;
      sms_reset();sms_arrive(1,pdu);CHECK(sms_events==1 && !sms_received_count && !sim_store[1]);
  }}
  /* TCP и 4 статуса за минуту: SMS не голодает. */
  sms_reset();for(unsigned k=0;k<60;++k) {
    uint16_t cap;uint8_t *p=comm_tx_acquire(&cap);if(p){memset(p,0x33,12);CHECK(comm_tx_commit(12));}
    if(k%10==0)sms_arrive(0,sms_phone[0].pdu);advance(1000);CHECK(comm_server_online());
  }
  CHECK(sms_events==6 && sms_received_count==6 && !sim_store[0]);
  puts("comm_sms: SLM336 setup/CPMS full/CMGL, index 0, phone GSM/UCS2/UDH, ME/MT/CMT, retries/events, TCP/status PASS");
}
