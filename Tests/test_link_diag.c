/* Доработка: Кряжев А., 2026. Сервисная диагностика на настоящей модели связи. */
static void link_expect(uint8_t code)
{
  uint32_t wait;
  uint8_t actual=comm_recover_link(sim_now, &wait);
  if (actual!=code) printf("link expected=%u actual=%u modem=%u reason=%u server=%u probe=%u\n",code,actual,comm_modem_status()->state,comm_modem_status()->reason,sv.state,comm_channel_probe_result());
  CHECK(actual == code);
  user_send_commDiag(37);
  CHECK(link_wire_size == 93 && link_wire[8] == 112 && link_wire[9] == 3 && link_wire[10] == code);
}
static void link_diag_tests(void)
{
  rc_online();
  link_expect(LINK_ONLINE);
  const uint32_t times[6]={7000,340,100,250,960,30}; /* +40 мс: CPMS/CMGL памяти ME */
  CHECK(!memcmp(slm.diag.phases,times,sizeof(times)));
  advance(1000);
  CHECK(!memcmp(slm.diag.phases,times,sizeof(times)));
  rc_fresh();
  link_expect(LINK_CONNECTING);
  sim_no_sim=1;
  for (unsigned i=0;comm_modem_state()!=SLM_FAILED;++i) {
    CHECK(i<2000);
    transport_tick(10);
  }
  link_expect(LINK_NO_SIM);
  uint32_t failed_ms=slm.diag.phases[SLM_PHASE_SIM];
  CHECK(failed_ms>0);
  advance(1000);
  CHECK(slm.diag.phases[SLM_PHASE_SIM]==failed_ms);
  comm_recover_poll(sim_now);
  link_expect(LINK_RESTART_WAIT);
  uint32_t wait;
  CHECK(comm_recover_link(sim_now+1234,&wait)==LINK_RESTART_WAIT && wait==600000u-1234u);
  CHECK(comm_recover_link(sim_now+600001,&wait)==LINK_RESTART_WAIT && wait==0);
  rc_online();
  rc_fresh();
  sim_network_wait=1;
  for (unsigned i=0;comm_modem_state()!=SLM_FAILED;++i) {
    CHECK(i<10000);
    transport_tick(10);
  }
  link_expect(LINK_NO_NETWORK);
  CHECK(slm.diag.phases[SLM_PHASE_NETWORK]>=60000u);
  rc_online();
  rc_fresh();
  sim_pdp_failures=100;
  for (unsigned i=0;comm_modem_state()!=SLM_FAILED;++i) {
    CHECK(i<10000);
    transport_tick(10);
  }
  link_expect(LINK_NO_INTERNET);
  rc_online();
  sim_open_error=1;
  comm_channel_reopen(0);
  advance(2000);
  link_expect(LINK_NO_SERVER);
  rc_online();
  drop_register=1;
  comm_channel_reopen(0);
  for (unsigned i=0;sv.state!=SV_REG_WAIT;++i) {
    CHECK(i<1000);
    transport_tick(10);
  }
  comm_server_registered(0);
  link_expect(LINK_IMEI_REJECTED);
  CHECK(((slm.diag.failures[(slm.diag.failure_next+9u)%10u]>>3)&31u)==SLM_ERR_IMEI);
  advance(1000);
  link_expect(LINK_IMEI_REJECTED);
  drop_register=0;
  until_online();
  link_expect(LINK_ONLINE);
  /* Последняя законченная проверка сохраняется после переоткрытия TCP. */
  rc_online();
  probe_result=0;
  CHECK(comm_channel_probe_result()==0);
  drop_status=1;
  advance(62000);
  CHECK(comm_channel_probe_result()==1);
  link_expect(LINK_NO_SERVER);
  rc_online();
  drop_status=1;
  ping_success=0;
  advance(68000);
  CHECK(comm_channel_probe_result()==2);
  link_expect(LINK_NO_SERVER);
  comm_channel_reopen(0);
  CHECK(comm_channel_probe_result()==2);
  rc_online();
  drop_status=1;
  ping_success=0;
  dns_success=0;
  advance(68000);
  CHECK(comm_channel_probe_result()==3);
  link_expect(LINK_NO_SERVER);
  for (unsigned i=0;!sv.flags.probe_failed;++i) {
    CHECK(i<6000);
    transport_tick(10);
  }
  link_expect(LINK_NO_INTERNET);
  CHECK(((slm.diag.failures[(slm.diag.failure_next+9u)%10u]>>3)&31u)==SLM_ERR_PROBE);
  /* Используются ответы существующих опросов; пяти- и шестизначный PLMN. */
  rc_online();
  rc_run(40000);
  CHECK(comm_info()->csq==22 && comm_info()->net==4 && !strcmp(comm_info()->operator_code,"25001"));
  const char *nw="+QNWINFO: \"FDD LTE\",\"001001\",\"LTE BAND 3\",1300";
  info_line((const uint8_t*)nw,(uint16_t)strlen(nw));
  CHECK(!strcmp(comm_info()->operator_code,"001001"));
  user_send_commDiag(37);
  CHECK(link_wire[12]==22 && link_wire[13]==4 && !memcmp(link_wire+15,"001001",7));
  nw="+QNWINFO: \"FDD LTE\",\"123X\"";
  info_line((const uint8_t*)nw,(uint16_t)strlen(nw));
  const char empty[7]={0};
  CHECK(!memcmp(comm_info()->operator_code,empty,7));
  /* Побайтный эталон: разные этапы/причины/секунды и обход кольца от старого к новому. */
  memset(&slm.diag,0,sizeof(slm.diag));
  slm.attempt.attempt_started=UINT32_MAX-500u;
  for (unsigned i=0;i<13;++i) {
    slm.attempt.now=slm.attempt.attempt_started+(i+1u)*10000000u;
    slm.attempt.status.phase=(uint8_t)(i%6u);
    comm_modem_record(SLM_STEP_COUNT,(uint8_t)(i%13u+1u),0);
  }
  CHECK(slm.diag.failure_next==3 && slm.diag.failure_count==10);
  uint32_t saved[10];
  memcpy(saved,slm.diag.failures,sizeof(saved));
  CHECK(comm_modem_restart(false,sim_now));
  CHECK(!memcmp(saved,slm.diag.failures,sizeof(saved)) && slm.diag.failure_next==3);
  for (unsigned i=0;i<6;++i) slm.diag.phases[i]=123u+i*70000u;
  user_send_commDiag(37);
  uint8_t expected[93]={83,76,68,78,85,0,0,37,112,3,LINK_CONNECTING,0,99,0,3};
  for (unsigned i=0;i<6;++i)
    for (unsigned j=0;j<4;++j) expected[26+4*i+j]=(uint8_t)((123u+i*70000u)>>(8*j));
  expected[50]=10;
  for (unsigned i=0;i<10;++i) {
    unsigned event=i+3u;
    uint32_t value=(event%6u) | (event%13u+1u)<<3 | (event+1u)*10000u<<8;
    for (unsigned j=0;j<4;++j) expected[51+4*i+j]=(uint8_t)(value>>(8*j));
  }
  uint16_t crc=65535;
  for (unsigned i=4;i<91;++i) {
    crc^=expected[i];
    for (unsigned j=0;j<8;++j) crc=(crc>>1)^((crc&1u)?0xa001u:0u);
  }
  expected[91]=(uint8_t)crc;
  expected[92]=(uint8_t)(crc>>8);
  CHECK(!memcmp(expected,link_wire,sizeof(expected)));
  puts("USB link diag: all reason codes, frozen phases, probe, operator, ring and exact 112 frame PASS");
}
