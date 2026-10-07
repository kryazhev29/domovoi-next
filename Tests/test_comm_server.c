/* Доработка: Кряжев А., 2026. Модель суток и серверных отказов; без платы.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static struct {
  int8_t connectionhandle; uint8_t registredOnServer, answerTimeout, answerID;
  char IMEIGSM[16]; uint8_t protocolVersion, hardwareVersion;
  uint32_t datetime, datetimeID, datetimeGroup, datetimeShedule, datetimeConfig, datetimeHoliday;
  uint16_t status, freeBufferSize; uint8_t firmwarePercent;
} device;
#define CH_SERVER 1
static uint8_t flash_model[2u * 1024u * 1024u], flash_before[sizeof(flash_model)];
static unsigned server_commands, server_sessions, event13, event_retry;
static unsigned registrations_sent, statuses_sent, opens, closes, pings, dns_requests;
static unsigned tx_unacked, drop_register, drop_status, ping_success, dns_success, close_error;
static unsigned send_silence, prompt_silence, reserve_enabled, junk_mode, invalid_reserve;
static uint8_t unique, last_status;
static uint16_t incoming_count;
static uint16_t log_unacked[2], temp_unacked[2];
static uint8_t incoming[2048], sent_frame[1040];
static uint16_t sent_count;
static bool socket_model, read_pending;
static uint32_t dns_late, ping_reply_due;
static unsigned ping_reply_kind, read_error, long_lines;
static uint32_t open_times[64];
static uint8_t open_addresses[64];
// Выдать время модели для кадров приложения.
static uint32_t timers_getMillis(void) { return sim_now; }
/* Звонок: сколько раз сброшен, ответ модема на сброс, последняя команда перед данными. */
static bool call_access_ready, call_registration_noise;
static unsigned chups, chup_silence, answers, aths, answer_mode, answer_delay, ath_silence;
static unsigned call_drop, call_drop_urc, call_activations, call_activation_error, call_net_queries, call_pdp_queries, call_tcp_queries;
static uint8_t call_cgreg_model, call_cereg_model, call_creg_model, call_pdp_model, call_other_pdp, call_other_socket;
static uint32_t answer_at, answer_due, hangup_at, hangup_due, call_network_due, call_tcp_due;
static char last_command[32];
/* SMS: ячейки SIM 1..10 (PDU hex), журнал команд SMS, последняя отправленная PDU. */
static const char *sim_store[11]; static char sms_log[512]; static char sms_sent_pdu[300];
static unsigned cmgs_error, sms_received_count;
static bool sms_received_model(char *number, char *text);
/* VoLTE: настройка, состояние модема, ответ ERROR, счётчики запросов и записей. */
static uint8_t volte_flag, volte_modem; static unsigned volte_error, volte_queries, volte_sets;
// Выдать требуемую настройку VoLTE.
static uint8_t volte_flag_model(void) { return volte_flag; }
/* Баланс: ответ модели на AT+CUSD=1 (URC через 2 с или до OK), счётчики, итог. */
static const char *cusd_reply, *ussd_code; static unsigned cusd_count, cusd_cancel, cusd_error, cusd_early, balance_count;
static uint32_t cusd_due; static int32_t balance_value;
static const char *ussd_model(void) { return ussd_code; }
// Запомнить полученный баланс SIM.
static void balance_model(int32_t rubles) { balance_value=rubles; ++balance_count; }
// Обозначить границу фото, не участвующую в данном сценарии.
static void photo_connection_lost(void) {}
static void user_parse_comand(uint8_t *p, unsigned channel);
static bool call_caller(const char *number);
/* КОНТРОЛЬНАЯ СУММА */
/* ПРИЁМ */

// Поставить входящий кадр сервера в модель TCP.
static void enqueue(const uint8_t *p, uint16_t n)
{
  CHECK(incoming_count + n <= sizeof(incoming));
  memcpy(incoming + incoming_count, p, n); incoming_count += n;
  sim_feed("+QIURC: \"recv\",0\r\n");
}
// Собрать независимый проверенный кадр команды сервера.
static uint16_t server_frame(uint8_t *p, uint8_t cmd, uint8_t id, const uint8_t *args, uint16_t n)
{
  uint16_t body = n + 3u; memcpy(p,"SLDN",4); p[4]=(uint8_t)body; p[5]=(uint8_t)(body>>8);
  p[6]=255; p[7]=id; p[8]=cmd; if (n) memcpy(p+9,args,n);
  uint16_t crc=crc16_calc(p+4,body+2u); p[body+6]=(uint8_t)crc; p[body+7]=(uint8_t)(crc>>8);
  return body+8u;
}
// Принять моделируемые подтверждения регистрации и статуса.
static void user_parse_comand(uint8_t *p, unsigned channel)
{
  CHECK(channel == CH_SERVER && !comm_tx_busy() && server_rx_verified(p)); ++server_commands;
  /* Модель разрешает только законные ответы. Любая мусорная команда — ошибка теста. */
  if (p[4] == 1) { CHECK(p[5] == 0); comm_server_registered(1); }
  else if (p[4] == 24) comm_server_status_ack(p[5]);
  else if (p[4] == 27) {
    uint16_t capacity; uint8_t *out = comm_tx_acquire(&capacity); CHECK(out && capacity >= 12);
    uint8_t args[1]={0}; CHECK(comm_tx_commit(server_frame(out,11,p[3],args,sizeof(args))));
  } else CHECK(!"unexpected valid garbage command");
}
static const char *server_text(uint8_t f, uint16_t *capacity)
{
  if (f==SLM_CFG_HOST2) { *capacity=sizeof("backup.local"); return invalid_reserve == 1 ? "bad\"host" : "backup.local"; }
  return sim_text(f,capacity);
}
// Выдать числовые настройки канала модели.
static uint16_t server_number(uint8_t f)
{ return f==SLM_CFG_RESERVE ? reserve_enabled : f==SLM_CFG_PORT2 ? (invalid_reserve == 2 ? 0 : 8888) : sim_number(f); }
static const slm_settings_t server_settings={server_text,server_number};
typedef unsigned dataChanel_e;
#define CH_USB 0
#define CH_SMS 2
#define CE_OK 0
#define CE_BUSY 5
#define TIMETOREGRESPONSE 5
#define TIMETOSTATUSRESPONSE 15
#define _1B(x) ((uint8_t)(x))
#define _2B(x) ((uint8_t)((x)>>8))
static uint8_t uniqIDCnt, legacy_frame[1040];
static uint16_t legacy_count;
// Обозначить границу USB, не участвующую в данном сценарии.
static bool usb_send(uint8_t *p, uint16_t n) { (void)p; (void)n; return true; }
/* Эталон: те же сборщики кадров пишут в отдельный буфер, кадр канала сверяется с ним байт в байт. */
static uint8_t legacy_buffer[1040];
// Выдать отдельный буфер для независимого эталона кадра.
static uint8_t *legacy_acquire(uint16_t needed) { return needed <= sizeof(legacy_buffer) ? legacy_buffer : NULL; }
// Выдать вместимость отдельного эталонного буфера.
static uint8_t *legacy_tx_acquire(uint16_t *capacity) { *capacity = sizeof(legacy_buffer); return legacy_buffer; }
// Запомнить длину эталонного кадра.
static bool legacy_commit(uint16_t n) { CHECK(n<=sizeof(legacy_frame)); memcpy(legacy_frame,legacy_buffer,n); legacy_count=n; return true; }
#define legacy_free(p, cch) ((void)(p), (void)(cch))
/* ПРОИЗВОДИТЕЛИ ПАКЕТОВ */
// Сверить кадр с эталоном и показать первое расхождение при отказе.
static void compare_frame(void)
{
  if (ch.frame_count != legacy_count || memcmp(frame, legacy_frame, ch.frame_count)) {
    fprintf(stderr, "wire sizes %u/%u state %u\n", ch.frame_count, legacy_count, sv.state);
    for (unsigned i = 0; i < ch.frame_count; ++i)
      if (frame[i] != legacy_frame[i]) { fprintf(stderr, "wire byte %u: %u/%u\n", i, frame[i], legacy_frame[i]); break; }
  }
  CHECK(ch.frame_count == legacy_count && !memcmp(frame, legacy_frame, ch.frame_count));
}
// Сравнить кадр регистрации с эталоном и передать его каналу.
static void registration_producer(void)
{
  uint8_t id=uniqIDCnt;
  legacy_user_send_registrationData(CH_SERVER); uniqIDCnt=id;
  user_send_registrationData(CH_SERVER);
  compare_frame();
  ++registrations_sent;
}
// Сравнить кадр статуса с эталоном и запомнить уникальный номер.
static void status_producer(void)
{
  uint8_t id=uniqIDCnt;
  legacy_user_send_statusPack(CH_SERVER); uniqIDCnt=id;
  user_send_statusPack(CH_SERVER); last_status=frame[7];
  compare_frame();
  ++statuses_sent;
}
// Передать смену TCP-сеанса реальному сборщику кадров.
static void server_session(bool open) { ++server_sessions; user_comm_session(open); }
// Смоделировать сохранение события отсутствия серверного трафика.
static bool no_traffic(void)
{
  if (event_retry) { --event_retry; return false; }
  ++event13; flash_model[0x70000]=13; return true;
}
static const comm_server_handlers_t test_server_handlers={registration_producer,status_producer,server_session,server_rx_process,no_traffic};
/* События журнала связи по порядку: вид и адрес; link_refuse - журнал не принял, повтор при следующем случае. */
static uint8_t link_kinds[256], link_addrs[256]; static unsigned links, link_refuse;
// Запомнить события состояния серверной связи.
static bool link_model(uint8_t kind, uint8_t address)
{
  if (link_refuse) { --link_refuse; return false; }
  if (links < sizeof(link_kinds)) { link_kinds[links] = kind; link_addrs[links] = address; }
  ++links; return true;
}
static unsigned link_count(uint8_t kind, uint8_t address)
{
  unsigned n = 0;
  for (unsigned i = 0; i < links && i < sizeof(link_kinds); ++i) n += link_kinds[i] == kind && link_addrs[i] == address;
  return n;
}
static const slm_handlers_t transport_handlers={sim_gpio,sim_type,sim_imei};

// Ответить на физически отправленные команды и кадры модели.
static void transport_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE)) return;
  if (tx_data == msg.out) {
    CHECK(!strncmp(last_command,"AT+CMGS=",8) && tx_count < sizeof(sms_sent_pdu) && tx_data[tx_count-1u] == 0x1A);
    memcpy(sms_sent_pdu,tx_data,tx_count-1u); sms_sent_pdu[tx_count-1u]=0;
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    sim_reply(cmgs_error ? "+CMS ERROR: 500\r\n" : "+CMGS: 12\r\n\r\nOK\r\n",40);
    return;
  }
  if (tx_data == frame) {
    /* После '>' - только данные: служебная команда не встаёт между QISEND и кадром. */
    CHECK(!strncmp(last_command,"AT+QISEND=0,",12) && strcmp(last_command,"AT+QISEND=0,0"));
    CHECK(tx_count <= sizeof(sent_frame)); sent_count=tx_count; memcpy(sent_frame,tx_data,tx_count);
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    /* SLM336 может возвращать бинарный пакет даже после ATE0. */
    if (sim_data_echo) for (unsigned i=0;i<sent_count;++i) interrupt_byte(sent_frame[i],USART_SR_RXNE);
    if (!send_silence) sim_reply("SEND OK\r\n",20);
    else if (send_silence == 2) sim_reply("SEND FAIL\r\n",20);
    else if (send_silence == 3) sim_reply("OK\r\n",20);
    uint8_t response[32], args[5]={0}; uint16_t n=0;
    if (sent_frame[8]==1 && !drop_register) n=server_frame(response,1,0,args,sizeof(args));
    else if (sent_frame[8]==2 && !drop_status) { args[0]=sent_frame[7]; n=server_frame(response,24,0,args,1); }
    if (n) enqueue(response,n);
    return;
  }
  CHECK(tx_count>=3 && tx_count<=128 && tx_data[0]=='A' && tx_data[1]=='T');
  char cmd[128]; CHECK(tx_count<=sizeof(cmd)); memcpy(cmd,tx_data,tx_count); cmd[tx_count-1u]=0;
  snprintf(last_command,sizeof(last_command),"%s",cmd);
  if (!strncmp(cmd,"AT+CMG",6) || !strncmp(cmd,"AT+CPMS",7) || !strncmp(cmd,"AT+CNMI",7)) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (strlen(sms_log)+strlen(cmd)+2u>=sizeof(sms_log)) sms_log[0]=0; /* долгие прогоны восстановления */
    strcat(sms_log,cmd); strcat(sms_log,";");
    unsigned i;
    char head[48];
    if (!strcmp(cmd,"AT+CMGL=4")) {
      for (i=1;i<11;++i) if (sim_store[i]) {
        snprintf(head,sizeof(head),"+CMGL: %u,0,,%u\r\n",i,(unsigned)strlen(sim_store[i])/2u-8u);
        sim_feed(head); sim_feed(sim_store[i]); sim_feed("\r\n");
      }
      sim_feed("\r\nOK\r\n");
    } else if (sscanf(cmd,"AT+CMGR=%u",&i)==1) {
      if (i<11 && sim_store[i]) {
        snprintf(head,sizeof(head),"+CMGR: 0,,%u\r\n",(unsigned)strlen(sim_store[i])/2u-8u);
        sim_feed(head); sim_feed(sim_store[i]); sim_feed("\r\n\r\nOK\r\n");
      } else sim_feed("+CMS ERROR: 321\r\n");
    } else if (sscanf(cmd,"AT+CMGD=%u",&i)==1) { CHECK(i<11); sim_store[i]=NULL; sim_reply("OK\r\n",30); }
    else if (!strncmp(cmd,"AT+CMGS=",8)) sim_reply("> ",30);
    else sim_reply("OK\r\n",30);
    return;
  }
  if (!strncmp(cmd,"AT+SETVOLTE",11)) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!strcmp(cmd,"AT+SETVOLTE?")) volte_queries+=volte_error;
    if (volte_error) { sim_reply("ERROR\r\n",30); return; }
    if (!strcmp(cmd,"AT+SETVOLTE?")) { ++volte_queries; sim_reply(volte_modem ? "+SETVOLTE: 1\r\nOK\r\n" : "+SETVOLTE: 0\r\nOK\r\n",30); }
    else { CHECK(!strcmp(cmd,"AT+SETVOLTE=0") || !strcmp(cmd,"AT+SETVOLTE=1")); ++volte_sets; volte_modem=cmd[12]=='1'; sim_reply("OK\r\n",30); }
    return;
  }
  if (!strncmp(cmd,"AT+CUSD=",8)) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!strcmp(cmd,"AT+CUSD=2")) { ++cusd_cancel; sim_reply("OK\r\n",30); return; }
    CHECK(!strcmp(cmd,"AT+CUSD=1,\"*100#\",15")); ++cusd_count;
    if (cusd_error) sim_reply("+CME ERROR: 4\r\n",30);
    else if (cusd_early && cusd_reply) { char r[400]; snprintf(r,sizeof(r),"%sOK\r\n",cusd_reply); sim_reply(r,30); }
    else { sim_reply("OK\r\n",30); cusd_due=sim_now+2000; }
    return;
  }
  if (!strcmp(cmd,"ATA") || !strcmp(cmd,"ATH") || !strcmp(cmd,"AT+CHUP")) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!strcmp(cmd,"ATA")) {
      ++answers; answer_at=sim_now;
      if (answer_mode!=3) answer_due=sim_now+answer_delay;
      if (call_drop) {
        socket_model=false;
        if (call_drop==2) call_pdp_model=0;
        if (call_drop_urc) sim_feed(call_drop==2 ? "+QIURC: \"pdpdeact\",1\r\n" : "+QIURC: \"closed\",0\r\n");
      }
    } else {
      hangup_at=sim_now;
      if (!strcmp(cmd,"ATH")) { ++aths; if (!ath_silence) hangup_due=sim_now+40; }
      else { ++chups; if (!chup_silence) hangup_due=sim_now+40; }
    }
    return;
  }
  if (comm_modem_ready() && (!strcmp(cmd,"AT+CGREG?") || !strcmp(cmd,"AT+CEREG?") || !strcmp(cmd,"AT+CREG?"))) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC); ++call_net_queries;
    const char *tag=!strcmp(cmd,"AT+CGREG?") ? "+CGREG" : !strcmp(cmd,"AT+CEREG?") ? "+CEREG" : "+CREG";
    uint8_t v=!strcmp(cmd,"AT+CGREG?") ? call_cgreg_model : !strcmp(cmd,"AT+CEREG?") ? call_cereg_model : call_creg_model;
    char reply[40]; CHECK(snprintf(reply,sizeof(reply),"%s: 1,%u\r\nOK\r\n",tag,v)>0); sim_reply(reply,30); return;
  }
  if (call_drop==2 && !strcmp(cmd,"AT+QIACT=1")) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!strcmp(cmd,"AT+QIACT?")) {
      ++call_pdp_queries; if (call_registration_noise) sim_feed("+CEREG: 1\r\n");
      if (!call_pdp_model && call_other_pdp) sim_reply(call_other_pdp==1 ? "+QIACT: 2,1,1,\"10.0.0.2\"\r\nOK\r\n" : "+QIACT: 1,1,3,\"10.0.0.1\"\r\nOK\r\n",30);
      else sim_reply(call_pdp_model ? "+QIACT: 1,1,1,\"10.0.0.1\"\r\nOK\r\n" : "OK\r\n",30);
    } else {
      ++call_activations; if (!call_activation_error) call_pdp_model=1;
      sim_reply(call_activation_error ? "ERROR\r\n" : "OK\r\n",100);
    }
    return;
  }
  if (!strncmp(cmd,"AT+QISEND=",10) || !strcmp(cmd,"AT+QICLOSE=0") || !strncmp(cmd,"AT+QPING=",9)
      || !strncmp(cmd,"AT+QIDNSGIP=",12) || !strncmp(cmd,"AT+QIRD=",8) || !strncmp(cmd,"AT+QIOPEN=",10) || !strcmp(cmd,"AT+QISTATE=0,1")) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!socket_model && !strncmp(cmd,"AT+QISEND=",10)) sim_reply("ERROR\r\n",20);
    else if (!strcmp(cmd,"AT+QISEND=0,0")) sim_reply(tx_unacked ? "+QISEND: 48,0,48\r\nOK\r\n" : "+QISEND: 48,48,0\r\nOK\r\n",20);
    else if (!strncmp(cmd,"AT+QISEND=",10)) { if (!prompt_silence) sim_reply("> ",20); }
    else if (!strcmp(cmd,"AT+QICLOSE=0")) {
      ++closes; if (close_error!=1) socket_model=false;
      sim_reply(close_error ? "ERROR\r\n" : "OK\r\n",20);
    } else if (!strcmp(cmd,"AT+QISTATE=0,1")) {
      ++call_tcp_queries; if (call_registration_noise) sim_feed("+CEREG: 1\r\n");
      if (call_other_socket) {
        const char *reply=call_other_socket==1 ? "+QISTATE: 1,\"TCP\",\"1.2.3.4\",7777,4444,2,1,0,0,\"uart1\"\r\nOK\r\n"
          : call_other_socket==2 ? "+QISTATE: 0,\"TCP\",\"1.2.3.4\",7777,4444,1,1,0,0,\"uart1\"\r\nOK\r\n"
          : call_other_socket==3 ? "+QISTATE: 0,\"TCP\",\"1.2.3.4\",7777,4444,2,2,0,0,\"uart1\"\r\nOK\r\n"
          : "+QISTATE: 0,\"UDP\",\"1.2.3.4\",7777,4444,2,1,0,0,\"uart1\"\r\nOK\r\n";
        sim_reply(reply,20);
      } else sim_reply(socket_model ? "+QISTATE: 0,\"TCP\",\"1.2.3.4\",7777,4444,2,1,0,0,\"uart1\"\r\nOK\r\n" : "OK\r\n",20);
    } else if (!strncmp(cmd,"AT+QIOPEN=",10)) {
      CHECK(call_pdp_model && !socket_model && opens<sizeof(open_addresses));
      open_times[opens]=sim_now; open_addresses[opens]=ch.address;
      socket_model=true; ++opens;
      CHECK(strstr(cmd,ch.address ? "backup.local" : "test.local"));
      if (call_drop) { sim_reply("OK\r\n",30); call_tcp_due=sim_now+300; }
      else sim_reply(sim_open_error ? "OK\r\n+QIOPEN: 0,566\r\n" : "OK\r\n+QIOPEN: 0,0\r\n",30);
    } else if (!strncmp(cmd,"AT+QPING=",9)) {
      CHECK(!strcmp(cmd,"AT+QPING=1,\"ya.ru\",3,2")); ++pings;
      sim_reply("OK\r\n",20); ping_reply_due=sim_now+1000;
      ping_reply_kind=ping_success==2 ? 3 : ping_success ? 1 : 2;
    } else if (!strncmp(cmd,"AT+QIDNSGIP=",12)) {
      ++dns_requests; sim_reply("OK\r\n",20);
      if (dns_success) dns_late=sim_now+500;
    } else {
      ++sim_qird; unsigned want; CHECK(sscanf(cmd,"AT+QIRD=0,%u",&want)==1 && want<=256);
      uint16_t n=incoming_count<want ? incoming_count : (uint16_t)want;
      char head[32]; int h=snprintf(head,sizeof(head),"+QIRD: %u\r\n",n); CHECK(h>0);
      sim_feed(head); for (uint16_t i=0;i<n;++i) interrupt_byte(incoming[i],USART_SR_RXNE);
      if (n) sim_feed("\r\n"); sim_feed(read_error ? "ERROR\r\n" : "OK\r\n");
      incoming_count-=n; memmove(incoming,incoming+n,incoming_count); read_pending=true;
    }
    return;
  }
  legacy_sim_tx();
}
// Продвинуть UART, модем, канал и сервер на один шаг.
static void transport_tick(unsigned increment)
{
  writes = 0;
  if (answer_due && (int32_t)(sim_now-answer_due)>=0) {
    answer_due=0; sim_feed(answer_mode==1 ? "ERROR\r\n" : answer_mode==2 ? "NO CARRIER\r\n" : "OK\r\n");
  }
  if (hangup_due && (int32_t)(sim_now-hangup_due)>=0) { hangup_due=0; sim_feed("OK\r\n"); }
  if (call_tcp_due && (int32_t)(sim_now-call_tcp_due)>=0) { call_tcp_due=0; sim_feed(sim_open_error ? "+QIOPEN: 0,566\r\n" : "+QIOPEN: 0,0\r\n"); }
  if (call_network_due && (int32_t)(sim_now-call_network_due)>=0) {
    call_network_due=0; sim_network_wait=0; call_cereg_model=1; sim_feed("+CEREG: 1,\"1234\",\"ABCDEF\",7\r\n");
  }
  if (sim_pending[0] && (int32_t)(sim_now-reply_due)>=0) { sim_feed(sim_pending); sim_pending[0]=0; }
  if (dns_late && (int32_t)(sim_now-dns_late)>=0) { dns_late=0;
    sim_feed(dns_success==2 ? "+QIURC: \"dnsgip\",0,1,60\r\n"
      : dns_success==3 ? "+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",1.2.3.4\r\n"
      : "+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",\"1.2.3.4\"\r\n"); }
  if (ping_reply_kind && (int32_t)(sim_now-ping_reply_due)>=0) {
    sim_feed(ping_reply_kind==3 ? "+QPING: 0,2,1,1,30,30,30\r\n" : ping_reply_kind==1 ? "+QPING: 0,\"1.2.3.4\",32,30,64\r\n+QPING: 0,2,1,1,30,30,30\r\n" : "+QPING: 0,2,0,2,0,0,0\r\n"); ping_reply_kind=0;
  }
  server_rx_begin_pass(); comm_channel_rx_capacity(user_comm_rx_capacity()); comm_server_poll(sim_now); comm_call_poll(sim_now, call_access_ready); comm_sms_poll(sim_now); comm_balance_poll(sim_now);
  if (cusd_due && (int32_t)(sim_now-cusd_due)>=0) { cusd_due=0; if (cusd_reply) sim_feed(cusd_reply); }
  transport_tx(); sim_now+=increment;
}
// Пройти заданный интервал с шагом модели 10 мс.
static void advance(unsigned ms) { uint32_t begin=sim_now; while (sim_now-begin<ms) transport_tick(10); }
// Запросить обычное восстановление после отказа ping и DNS.
static void server_no_internet(void) { comm_channel_reopen(0); }
// Начать независимый сеанс модема и зарегистрироваться на сервере.
static void server_reset(void)
{
  comm_modem_transport(NULL); sim_reset();
  server_commands=server_sessions=event13=event_retry=registrations_sent=statuses_sent=opens=closes=pings=dns_requests=0;
  tx_unacked=drop_register=drop_status=close_error=send_silence=prompt_silence=reserve_enabled=junk_mode=0;
  ping_success=dns_success=1; unique=last_status=uniqIDCnt=0;
  memcpy(device.IMEIGSM,"123456789012345",sizeof(device.IMEIGSM));
  device.protocolVersion=2; device.hardwareVersion=139;
  device.datetime=1790900000u; device.datetimeID=1; device.datetimeGroup=2;
  device.datetimeShedule=3; device.datetimeConfig=4; device.datetimeHoliday=5;
  device.status=0x1234; device.freeBufferSize=2048; device.answerID=0; incoming_count=sent_count=0;
  socket_model=false; read_pending=false; read_error=0; dns_late=ping_reply_due=0; ping_reply_kind=0;
  CHECK(modem_slm336_start(&server_settings,&transport_handlers,sim_now));
  call_access_ready=true; call_registration_noise=false;
  chups=chup_silence=answers=aths=answer_mode=ath_silence=0; answer_delay=40;
  answer_at=answer_due=hangup_at=hangup_due=call_network_due=call_tcp_due=0;
  call_drop=call_drop_urc=call_activations=call_activation_error=call_net_queries=call_pdp_queries=call_tcp_queries=0;
  call_cgreg_model=call_cereg_model=call_creg_model=call_pdp_model=1; call_other_pdp=call_other_socket=0;
  chups=chup_silence=0; last_command[0]=0; volte_error=volte_queries=volte_sets=0; comm_call_init(call_caller, volte_flag_model);
  sms_log[0]=sms_sent_pdu[0]=0; cmgs_error=sms_received_count=0; comm_sms_init(sms_received_model);
  ussd_code=""; cusd_reply=NULL; cusd_due=0; cusd_count=cusd_cancel=cusd_error=cusd_early=balance_count=0; balance_value=0;
  comm_balance_init(ussd_model, balance_model);
  /* Выход к TCP — принятой моделью этапа 3. */
  comm_server_link(link_model); comm_server_no_internet(server_no_internet);
  comm_server_init(&test_server_handlers); links=link_refuse=0;
  comm_channel_receive(user_comm_rx, user_comm_rx_done);
  for (unsigned i=0;!comm_server_online() && i<1500;++i) transport_tick(10);
  CHECK(comm_server_online() && comm_modem_ready() && sim_now<11000);
  advance(500); call_activations=call_net_queries=call_pdp_queries=call_tcp_queries=0;
}
// Проверить кадры, сроки статусов, регистрацию и отказы сервера.
static void server_tests(void)
{
  /* Резервный адрес и порт проверяются до первого построения команды TCP. */
  sim_reset(); reserve_enabled=1; invalid_reserve=1;
  CHECK(!modem_slm336_start(&server_settings,&sim_handlers,0));
  invalid_reserve=2;
  CHECK(!modem_slm336_start(&server_settings,&sim_handlers,0));
  invalid_reserve=0;

  server_reset();
  uint8_t arbitrary[1029]; for (unsigned i=0;i<sizeof(arbitrary);++i) arbitrary[i]=(uint8_t)i;
  legacy_user_send_anyData(27,arbitrary,sizeof(arbitrary),CH_SERVER,42);
  user_send_anyData(27,arbitrary,sizeof(arbitrary),CH_SERVER,42);
  CHECK(ch.frame_count==1040 && legacy_count==1040 && !memcmp(frame,legacy_frame,1040));
  advance(500);
  CHECK(!comm_tx_busy()); user_send_anyData(27,arbitrary,1030,CH_SERVER,42); CHECK(!comm_tx_busy());
  legacy_user_send_anyData(27,NULL,0,CH_SERVER,42);
  user_send_anyData(27,NULL,0,CH_SERVER,42);
  CHECK(ch.frame_count==11 && !memcmp(frame,legacy_frame,11)); advance(500);
  static char sms[70000]; memset(sms,'S',sizeof(sms)-1u); sms[1029]=0;
  uint8_t sms_unique=uniqIDCnt; legacy_user_send_smsToServer(sms); uniqIDCnt=sms_unique; user_send_smsToServer(sms);
  CHECK(ch.frame_count==1040 && !memcmp(frame,legacy_frame,1040)); advance(500);
  sms[1029]='S'; user_send_smsToServer(sms); CHECK(!comm_tx_busy());
  user_send_smsToServer(NULL); CHECK(!comm_tx_busy());
  /* Первый период — 5 с; внеочередной статус не переносит следующий период 15 с. */
  server_reset(); CHECK(comm_server_online() && registrations_sent==1);
  advance(4490); CHECK(statuses_sent==0);
  advance(10); CHECK(statuses_sent==1);
  advance(500); CHECK(!comm_tx_busy());
  advance(4500); user_send_statusPack(CH_SERVER); advance(500);
  CHECK(statuses_sent==1);
  advance(9490); CHECK(statuses_sent==1);
  advance(10); CHECK(statuses_sent==2 && !sv.misses && !event13);
  uint8_t id=last_status; comm_server_status_ack(id+1u); CHECK(!sv.misses);
  server_reset(); drop_status=1;
  for (unsigned i=0;i<124;++i) {
    if (!comm_tx_busy()) user_send_statusPack(CH_SERVER);
    advance(500);
  }
  CHECK(pings==1 && event13==1);
  server_reset(); drop_status=1; event_retry=1; advance(62000);
  CHECK(pings==1 && dns_requests==0 && event13==1 && sv.state==SV_SILENT && closes==0);
  advance(10000); CHECK(event13==1);
  drop_status=0; advance(15000); CHECK(comm_server_online() && sv.flags.traffic_event==0);
  server_reset(); drop_status=1; ping_success=0; dns_success=0; advance(90000);
  CHECK(pings>=1 && dns_requests>=1 && closes>=1 && opens>=2 && event13==0);
  server_reset(); drop_status=1; ping_success=2; advance(62000);
  CHECK(event13==1 && dns_requests==0);
  server_reset(); drop_status=1; ping_success=0; dns_success=2; advance(90000);
  CHECK(event13==0 && closes>=1);
  server_reset(); drop_status=1; ping_success=0; dns_success=3; advance(68000);
  CHECK(event13==1 && !closes);
  server_reset(); drop_status=1; ping_success=0; advance(68000);
  CHECK(dns_requests==1 && event13==1 && !closes);
  server_reset(); log_unacked[0]=3; temp_unacked[0]=4; log_unacked[1]=11; temp_unacked[1]=12;
  sim_feed("+QIURC: \"closed\",0\r\n"); advance(1000);
  CHECK(log_unacked[0]==3 && temp_unacked[0]==4 && !log_unacked[1] && !temp_unacked[1]);
  CHECK(closes==1 && opens==2 && comm_server_online());
  server_reset(); close_error=2;
  sim_feed("+QIURC: \"closed\",0\r\n"); advance(1000);
  CHECK(closes==1 && opens==2 && comm_server_online());
  server_reset(); tx_unacked=1; uint16_t cap; uint8_t *p=comm_tx_acquire(&cap); CHECK(p && cap==1040);
  memset(p,0,12); CHECK(comm_tx_commit(12)); advance(11000); CHECK(closes>=1);
  server_reset(); close_error=2; comm_channel_reopen(0); advance(1000);
  CHECK(closes==1 && opens==2 && comm_server_online());
  server_reset(); close_error=1; comm_channel_reopen(0); advance(1000); CHECK(closes==1 && opens==1 && ch.state==CH_FAULT);
  server_reset(); drop_register=1; comm_channel_reopen(0); advance(9000);
  CHECK(registrations_sent==3 && opens==2 && closes==1);
  advance(2000); CHECK(closes==2 && opens==3);
  server_reset(); drop_register=1; reserve_enabled=1; comm_channel_reopen(0); advance(210000);
  CHECK(opens>=8 && ch.retry_step>=2);
  for (unsigned i=1;i<8;++i) CHECK(open_addresses[i]==((i-1u)/3u)%2u);
  CHECK(open_times[4]-open_times[3]>=comm_timeouts[COMM_T_SERVER_RETRY_1]);
  CHECK(open_times[7]-open_times[6]>=comm_timeouts[COMM_T_SERVER_RETRY_2]);
  server_reset(); comm_channel_priority(true); CHECK(!comm_tx_acquire(&cap)); comm_channel_priority(false);
  p=comm_tx_acquire(&cap); CHECK(p && !comm_tx_acquire(&cap));
  CHECK(!comm_tx_commit(cap+1u)); comm_tx_release(); CHECK(!comm_tx_busy());
  uint8_t packet[24], args[1]={0}; uint16_t n=server_frame(packet,27,3,args,0);
  enqueue(packet,n); enqueue(packet,n); unsigned before=server_commands;
  for (unsigned i=0;server_commands==before && i<100;++i) transport_tick(10);
  CHECK(server_commands==before+1 && comm_tx_busy() && server_rx_ready);
  server_rx_process(); CHECK(server_commands==before+1);
  advance(1000); CHECK(server_commands==before+2);
  server_reset(); advance(4000); p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
  tx_unacked=1; advance(1500); CHECK(sv.flags.status_due && !comm_tx_acquire(&cap));
  tx_unacked=0; advance(1500); CHECK(statuses_sent==1);
  server_reset(); prompt_silence=1; p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
  advance(4000); CHECK(closes>=1 && opens>=2);
  for (unsigned failure=1;failure<=3;++failure) {
    server_reset(); send_silence=failure; p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
    advance(6000); CHECK(closes>=1 && opens>=2);
  }
  server_reset(); drop_status=1; ping_success=dns_success=0; advance(70000);
  CHECK(dns_requests==1 && statuses_sent==5 && ch.probe_active==COMM_CH_DNS && opens==1);
  advance(3000); CHECK(opens>=2 && event13==0);
  sim_feed("+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",\"1.2.3.4\"\r\n");
  advance(500); CHECK(event13==0);
  server_reset(); drop_register=1; read_error=1; enqueue(packet,n); before=server_commands;
  advance(500); CHECK(server_commands==before && !server_rx_count && closes>=1);
  puts("comm_server: registration/status/ping/DNS/close/backpressure/ACK/address/backoff PASS");
}
/* Ответ ping/DNS может прийти посреди подтверждения другой передачи. */
static void probe_overlap_tests(void)
{
  for (unsigned probe=COMM_CH_PING; probe<=COMM_CH_DNS; ++probe) {
    server_reset(); drop_status=1; ping_success=probe==COMM_CH_PING;
    for (unsigned i=0;ch.probe_active!=probe && i<10000;++i) transport_tick(10);
    CHECK(ch.probe_active==probe);
    ping_reply_kind=0; dns_late=0; advance(50);
    uint16_t capacity; uint8_t *p=comm_tx_acquire(&capacity);
    CHECK(p && capacity>=12); memset(p,0,12); CHECK(comm_tx_commit(12));
    for (unsigned i=0;ch.operation!=OP_ACK && i<100;++i) transport_tick(10);
    CHECK(ch.operation==OP_ACK && ch.probe_active==probe);
    sim_feed(probe==COMM_CH_PING ? "+QPING: 0,\"1.2.3.4\",32,30,64\r\n"
      : "+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",\"1.2.3.4\"\r\n");
    advance(100);
    CHECK(sv.state==SV_SILENT && event13==1 && !ch.probe_active && !comm_tx_busy());
  }
}
// Проверить сутки случайного трафика и неизменность NOR.
static void garbage_test(void)
{
  server_reset();
  for (size_t i=0;i<sizeof(flash_model);++i) flash_model[i]=(uint8_t)(i*37u+(i>>8));
  memcpy(flash_before,flash_model,sizeof(flash_model));
  uint32_t start=sim_now, random=17; unsigned overflows=0, fragments=0; long_lines=0;
  while (sim_now-start<86400000u) {
    random=random*1664525u+1013904223u;
    if ((random&255u)==0 && !comm_modem_busy() && !comm_tx_busy()) {
      /* Тело QIRD содержит управляющие слова, NUL и ложный SLDN/CRC. */
      uint8_t noise[256]; for (unsigned i=0;i<sizeof(noise);++i) noise[i]=(uint8_t)(random+i*11u);
      memcpy(noise+7,"OK\r\nERROR\r\nRING\r\nSLDN\0",23); enqueue(noise,sizeof(noise)); ++fragments;
    }
    if ((random&8191u)==0 && !sim_pending[0] && !comm_modem_busy()) {
      for (unsigned i=0;i<700;++i) interrupt_byte('X',USART_SR_RXNE);
      transport_tick(10); comm_uart_stats_t stats; comm_uart_stats(&stats); DIAG_CHECK(stats.overflow); ++overflows;
      /* Новая изолированная сессия модели; питание/восстановление этапа 5 не тестируются. */
      uint32_t saved=sim_now; unsigned saved_events=event13;
      server_reset(); sim_now=saved; sv.now=saved; sv.status_since=saved; event13=saved_events;
    }
    if ((random&8191u)==4096u && !sim_pending[0] && !comm_modem_busy()) {
      for (unsigned i=0;i<240;++i) interrupt_byte('Y',USART_SR_RXNE);
      sim_feed("\r\n"); transport_tick(10); transport_tick(10);
      CHECK(ch.state==CH_FAULT); ++long_lines;
      uint32_t saved=sim_now; server_reset(); sim_now=sv.now=sv.status_since=saved;
    }
    transport_tick(100);
  }
  CHECK(fragments>100 && overflows>10 && long_lines>10);
  CHECK(!memcmp(flash_model,flash_before,sizeof(flash_model)));
  printf("comm_server garbage: 24 model hours, %u payloads, %u ring overflows, %u long lines, entire NOR unchanged PASS\n",fragments,overflows,long_lines);
}
// Дождаться серверной регистрации с ограничением времени модели.
static void until_online(void) { for (unsigned i=0;!comm_server_online() && i<200000;++i) transport_tick(10); CHECK(comm_server_online()); }
/* Лист «Журнал событий»: 14/15 подключён (сервер принял регистрацию), 16/17 ошибка соединения -
   один раз до удачного, 18/19 потеря связи после регистрации. Номер сервера - по адресу. */
static void link_tests(void)
{
  server_reset(); CHECK(links==1 && link_kinds[0]==COMM_LINK_UP && link_addrs[0]==0);
  sim_feed("+QIURC: \"closed\",0\r\n"); advance(2000);
  CHECK(links==3 && link_kinds[1]==COMM_LINK_LOST && link_addrs[1]==0 && link_kinds[2]==COMM_LINK_UP);
  /* Перезапуск модема на связи - тоже потеря. */
  comm_server_init(&test_server_handlers); CHECK(links==4 && link_kinds[3]==COMM_LINK_LOST);
  /* Сервер не открывает TCP: одно событие на всю серию попыток. */
  server_reset(); sim_open_error=1; comm_channel_reopen(0);
  for (unsigned i=0;opens<6 && i<100000;++i) transport_tick(10);
  CHECK(opens>=6 && link_count(COMM_LINK_LOST,0)==1 && link_count(COMM_LINK_FAIL,0)==1 && links==3);
  sim_open_error=0; until_online(); CHECK(links==4 && link_kinds[3]==COMM_LINK_UP);
  /* После удачного соединения ошибка пишется снова. */
  sim_open_error=1; comm_channel_reopen(0);
  for (unsigned i=0;links<6;++i) { CHECK(i<100000); transport_tick(10); }
  CHECK(link_count(COMM_LINK_FAIL,0)==2);
  /* TCP открыт, регистрацию сервер не принял - ошибка соединения, не «подключён» и не «потеря». */
  server_reset(); drop_register=1; comm_channel_reopen(0); advance(30000);
  CHECK(link_count(COMM_LINK_LOST,0)==1 && link_count(COMM_LINK_FAIL,0)==1 && link_count(COMM_LINK_UP,0)==1); /* UP - от server_reset */
  /* Журнал не принял запись - повтор при следующей неудаче. */
  server_reset(); link_refuse=1; sim_open_error=1; comm_channel_reopen(0); link_refuse=1;
  for (unsigned i=0;opens<4 && i<100000;++i) transport_tick(10);
  CHECK(link_count(COMM_LINK_FAIL,0)==1);
  /* Резервный сервер: ошибка каждого - по разу, «подключён» - с адресом, где открылся TCP. */
  server_reset(); reserve_enabled=1; sim_open_error=1; comm_channel_reopen(0);
  for (unsigned i=0;opens<10 && i<200000;++i) transport_tick(10);
  CHECK(link_count(COMM_LINK_FAIL,0)==1 && link_count(COMM_LINK_FAIL,1)==1);
  sim_open_error=0; until_online(); CHECK(link_kinds[links-1u]==COMM_LINK_UP && link_addrs[links-1u]==open_addresses[opens-1u]);
  CHECK(comm_server_address()==open_addresses[opens-1u]);
  puts("comm_server link events: connected, lost, connect error once until success, rejected registration, retry, reserve PASS");
}
/* Пакет 8 (ответ на команду 6): занятый слот канала не теряет его - уходит, когда слот свободен. */
static void unregistered_sms_tests(void)
{
  server_reset();uint8_t busy=0;user_send_anyData(27,&busy,1,CH_SERVER,1);
  unreg_sms_note("1234567890");unreg_sms_poll();CHECK(ch.frame_count==12);
  for(unsigned i=0;i<1000 && sent_frame[8]!=7;i++){transport_tick(10);unreg_sms_poll();}
  CHECK(sent_frame[8]==7 && !memcmp(sent_frame+9,"1234567890: ID is unregistred!",sizeof("1234567890: ID is unregistred!")-1));
  advance(500);sent_frame[8]=0;unreg_sms_poll();advance(500);CHECK(sent_frame[8]!=7);
  puts("unknown ID notification: waits for a free slot and is sent once PASS");
}

// Проверить сохранение ответа обновления до освобождения слота.
static void pending_update_tests(void)
{
  server_reset();uint16_t old=device.status;
  status_event_sent(old,sim_now-2000);uint8_t busy=0;user_send_anyData(27,&busy,1,CH_SERVER,1);
  device.status^=1;CHECK(status_event_due(device.status,sim_now));user_send_statusPack(CH_SERVER);
  CHECK(status_event_due(device.status,sim_now));advance(500);user_send_statusPack(CH_SERVER);
  CHECK(!status_event_due(device.status,sim_now));advance(500);

  server_reset();uint8_t a=0;user_send_anyData(27,&a,1,CH_SERVER,1);
  device.firmwarePercent=37;user_send_statusUpdateFW(20,253,CH_SERVER,77);
  device.firmwarePercent=0;
  for(unsigned i=0;i<1000 && !(sent_count==13 && sent_frame[8]==4);i++){transport_tick(10);fw_status_poll();}
  CHECK(sent_count==13 && sent_frame[8]==4 && sent_frame[7]==77 && sent_frame[9]==37 && sent_frame[10]==253);
  advance(500);sent_frame[8]=0;fw_status_poll();advance(500);CHECK(sent_frame[8]!=4);
  puts("update result: busy slot keeps code, percent and ID until sent once PASS");
}

// Проверить однократную передачу ответов считывателя после занятости.
static void reader_tests(void)
{
  server_reset(); uint16_t cap; uint8_t *p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
  user_send_readedIDtoServer(NULL,255); reader_reply_poll(); CHECK(ch.frame_count==12);
  unsigned i; for (i=0;!(sent_count==22 && sent_frame[8]==8) && i<1000;++i) { transport_tick(10); reader_reply_poll(); }
  CHECK(sent_count==22 && sent_frame[8]==8 && sent_frame[9]==255);
  for (i=10;i<20;++i) CHECK(sent_frame[i]==0);
  advance(500); user_send_readedIDtoServer((uint8_t*)"0012345678",1);
  for (i=0;!(sent_frame[8]==8 && sent_frame[9]==1) && i<1000;++i) { transport_tick(10); reader_reply_poll(); }
  CHECK(sent_frame[9]==1 && !memcmp(sent_frame+10,"0012345678",10));
  /* Отправлен один раз: дальше пакетов 8 нет. */
  sent_frame[8]=0; for (i=0;i<300;++i) { transport_tick(10); reader_reply_poll(); }
  CHECK(sent_frame[8]!=8);
  puts("reader reply: packet 8 waits for a free slot, timeout and card both delivered once PASS");
}
/* ВОССТАНОВЛЕНИЕ */
/* ВЫЗОВ */
int main(void)
{ CHECK(slm_regressions()==0); reset_session_contract(); channel_regressions(); server_tests(); probe_overlap_tests(); call_tests(); sms_tests(); balance_tests(); volte_tests(); garbage_test(); link_tests(); pending_update_tests(); unregistered_sms_tests(); reader_tests(); recover_tests(); return 0; }
