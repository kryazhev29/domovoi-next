/* Доработка: Кряжев А., 2026. Модель суток и серверных отказов; без платы. */
static struct {
  int8_t connectionhandle; uint8_t registredOnServer, answerTimeout, answerID;
  char IMEIGSM[16]; uint8_t protocolVersion, hardwareVersion;
  uint32_t datetime, datetimeID, datetimeGroup, datetimeShedule, datetimeConfig, datetimeHoliday;
  uint16_t status, freeBufferSize;
} device;
#define CH_SERVER 1
#define LOG_SYS_CONNECT1 14
static uint8_t flash_model[2u * 1024u * 1024u], flash_before[sizeof(flash_model)];
static unsigned server_commands, server_sessions, event13, event_retry;
static unsigned registrations_sent, statuses_sent, opens, closes, pings, dns_requests;
static unsigned tx_unacked, drop_register, drop_status, ping_success, dns_success, close_error;
static unsigned send_silence, prompt_silence, reserve_enabled, junk_mode;
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
static uint32_t timers_getMillis(void) { return sim_now; }
static void photo_connection_lost(void) {}
static bool log_addSystemEvent(unsigned e) { CHECK(e == LOG_SYS_CONNECT1); return true; }
static void user_parse_comand(uint8_t *p, unsigned channel);
/* CRC */
/* RX */

static void enqueue(const uint8_t *p, uint16_t n)
{
  CHECK(incoming_count + n <= sizeof(incoming));
  memcpy(incoming + incoming_count, p, n); incoming_count += n;
  sim_feed("+QIURC: \"recv\",0\r\n");
}
static uint16_t server_frame(uint8_t *p, uint8_t cmd, uint8_t id, const uint8_t *args, uint16_t n)
{
  uint16_t body = n + 3u; memcpy(p,"SLDN",4); p[4]=(uint8_t)body; p[5]=(uint8_t)(body>>8);
  p[6]=255; p[7]=id; p[8]=cmd; if (n) memcpy(p+9,args,n);
  uint16_t crc=crc16_calc(p+4,body+2u); p[body+6]=(uint8_t)crc; p[body+7]=(uint8_t)(crc>>8);
  return body+8u;
}
static void user_parse_comand(uint8_t *p, unsigned channel)
{
  CHECK(channel == CH_SERVER && !comm_tx_busy()); ++server_commands;
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
  if (f==SLM_CFG_HOST2) { *capacity=sizeof("backup.local"); return "backup.local"; }
  return sim_text(f,capacity);
}
static uint16_t server_number(uint8_t f)
{ return f==SLM_CFG_RESERVE ? reserve_enabled : f==SLM_CFG_PORT2 ? 8888 : sim_number(f); }
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
static bool usb_send(uint8_t *p, uint16_t n) { (void)p; (void)n; return true; }
static int8_t connection_send(int8_t h, uint8_t *p, uint16_t n)
{ (void)h; CHECK(n<=sizeof(legacy_frame)); memcpy(legacy_frame,p,n); legacy_count=n; return CE_OK; }
static void status_event_sent(uint16_t status, uint32_t now) { (void)status; (void)now; }
/* PRODUCERS */
static void registration_producer(void)
{
  uint8_t id=uniqIDCnt;
  legacy_user_send_registrationData(CH_SERVER); uniqIDCnt=id;
  user_send_registrationData(CH_SERVER);
  CHECK(ch.frame_count==legacy_count && !memcmp(frame,legacy_frame,ch.frame_count));
  ++registrations_sent;
}
static void status_producer(void)
{
  uint8_t id=uniqIDCnt;
  legacy_user_send_statusPack(CH_SERVER); uniqIDCnt=id;
  user_send_statusPack(CH_SERVER); last_status=frame[7];
  CHECK(ch.frame_count==legacy_count && !memcmp(frame,legacy_frame,ch.frame_count));
  ++statuses_sent;
}
static void server_session(bool open) { ++server_sessions; user_comm_session(open); }
static bool no_traffic(void)
{
  if (event_retry) { --event_retry; return false; }
  ++event13; flash_model[0x70000]=13; return true;
}
static const comm_server_handlers_t test_server_handlers={registration_producer,status_producer,server_session,server_rx_process,no_traffic};
static const slm_handlers_t transport_handlers={sim_gpio,user_comm_rx,user_comm_rx_done,sim_type};

static void transport_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE)) return;
  if (tx_data == frame) {
    CHECK(tx_count <= sizeof(sent_frame)); sent_count=tx_count; memcpy(sent_frame,tx_data,tx_count);
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!send_silence) sim_reply("SEND OK\r\n",20);
    else if (send_silence == 2) sim_reply("SEND FAIL\r\n",20);
    else if (send_silence == 3) sim_reply("OK\r\n",20);
    uint8_t response[32], args[5]={0}; uint16_t n=0;
    if (sent_frame[8]==1 && !drop_register) n=server_frame(response,1,0,args,sizeof(args));
    else if (sent_frame[8]==2 && !drop_status) { args[0]=sent_frame[7]; n=server_frame(response,24,0,args,1); }
    if (n) enqueue(response,n);
    return;
  }
  CHECK(tx_data==command);
  char cmd[128]; CHECK(tx_count<=sizeof(cmd)); memcpy(cmd,command,tx_count); cmd[tx_count-1u]=0;
  if (!strncmp(cmd,"AT+QISEND=",10) || !strcmp(cmd,"AT+QICLOSE=0") || !strncmp(cmd,"AT+QPING=",9)
      || !strncmp(cmd,"AT+QIDNSGIP=",12) || !strncmp(cmd,"AT+QIRD=",8) || !strncmp(cmd,"AT+QIOPEN=",10) || !strcmp(cmd,"AT+QISTATE=0,1")) {
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (!strcmp(cmd,"AT+QISEND=0,0")) sim_reply(tx_unacked ? "+QISEND: 48,0,48\r\nOK\r\n" : "+QISEND: 48,48,0\r\nOK\r\n",20);
    else if (!strncmp(cmd,"AT+QISEND=",10)) { if (!prompt_silence) sim_reply("> ",20); }
    else if (!strcmp(cmd,"AT+QICLOSE=0")) {
      ++closes; if (close_error!=1) socket_model=false;
      sim_reply(close_error ? "ERROR\r\n" : "OK\r\n",20);
    } else if (!strcmp(cmd,"AT+QISTATE=0,1")) {
      sim_reply(socket_model ? "+QISTATE: 0,\"TCP\",\"1.2.3.4\",7777,4444,2,1,0,0,\"uart1\"\r\nOK\r\n" : "OK\r\n",20);
    } else if (!strncmp(cmd,"AT+QIOPEN=",10)) {
      CHECK(!socket_model && opens<sizeof(open_addresses));
      open_times[opens]=sim_now; open_addresses[opens]=ch.address;
      socket_model=true; ++opens;
      CHECK(strstr(cmd,ch.address ? "backup.local" : "test.local"));
      sim_reply(sim_open_error ? "OK\r\n+QIOPEN: 0,566\r\n" : "OK\r\n+QIOPEN: 0,0\r\n",30);
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
static void transport_tick(unsigned increment)
{
  writes = 0;
  if (sim_pending[0] && (int32_t)(sim_now-reply_due)>=0) { sim_feed(sim_pending); sim_pending[0]=0; }
  if (dns_late && (int32_t)(sim_now-dns_late)>=0) { dns_late=0;
    sim_feed(dns_success==2 ? "+QIURC: \"dnsgip\",0,1,60\r\n"
      : dns_success==3 ? "+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",1.2.3.4\r\n"
      : "+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",\"1.2.3.4\"\r\n"); }
  if (ping_reply_kind && (int32_t)(sim_now-ping_reply_due)>=0) {
    sim_feed(ping_reply_kind==3 ? "+QPING: 0,2,1,1,30,30,30\r\n" : ping_reply_kind==1 ? "+QPING: 0,\"1.2.3.4\",32,30,64\r\n+QPING: 0,2,1,1,30,30,30\r\n" : "+QPING: 0,2,0,2,0,0,0\r\n"); ping_reply_kind=0;
  }
  modem_slm336_rx_capacity(user_comm_rx_capacity()); comm_server_poll(sim_now); transport_tx(); sim_now+=increment;
}
static void advance(unsigned ms) { uint32_t begin=sim_now; while (sim_now-begin<ms) transport_tick(10); }
static void server_reset(void)
{
  modem_slm336_transport(NULL); sim_reset();
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
  /* Выход к TCP — принятой моделью этапа 3. */
  comm_server_init(&test_server_handlers);
  for (unsigned i=0;!comm_server_online() && i<1500;++i) transport_tick(10);
  CHECK(comm_server_online() && modem_slm336_ready() && sim_now<11000);
  advance(500);
}
static void server_tests(void)
{
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
  server_reset(); CHECK(comm_server_online() && registrations_sent==1); advance(16000);
  CHECK(statuses_sent==1 && !sv.misses && !event13);
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
  server_reset(); comm_channel_priority(true); CHECK(!comm_tx_acquire(NULL)); comm_channel_priority(false);
  p=comm_tx_acquire(&cap); CHECK(p && !comm_tx_acquire(NULL));
  CHECK(!comm_tx_commit(cap+1u)); comm_tx_release(); CHECK(!comm_tx_busy());
  uint8_t packet[24], args[1]={0}; uint16_t n=server_frame(packet,27,3,args,0);
  enqueue(packet,n); enqueue(packet,n); unsigned before=server_commands;
  for (unsigned i=0;server_commands==before && i<100;++i) transport_tick(10);
  CHECK(server_commands==before+1 && comm_tx_busy() && server_rx_ready);
  server_rx_process(); CHECK(server_commands==before+1);
  advance(1000); CHECK(server_commands==before+2);
  server_reset(); advance(14000); p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
  tx_unacked=1; advance(1500); CHECK(sv.flags.status_due && !comm_tx_acquire(NULL));
  tx_unacked=0; advance(1500); CHECK(statuses_sent==1);
  server_reset(); prompt_silence=1; p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
  advance(4000); CHECK(closes>=1 && opens>=2);
  for (unsigned failure=1;failure<=3;++failure) {
    server_reset(); send_silence=failure; p=comm_tx_acquire(&cap); CHECK(p); memset(p,0,12); CHECK(comm_tx_commit(12));
    advance(6000); CHECK(closes>=1 && opens>=2);
  }
  server_reset(); drop_status=1; ping_success=dns_success=0; advance(80000);
  CHECK(dns_requests==1 && statuses_sent==5 && ch.probe_active==COMM_CH_DNS && opens==1);
  advance(3000); CHECK(opens>=2 && event13==0);
  sim_feed("+QIURC: \"dnsgip\",0,1,60\r\n+QIURC: \"dnsgip\",\"1.2.3.4\"\r\n");
  advance(500); CHECK(event13==0);
  server_reset(); drop_register=1; read_error=1; enqueue(packet,n); before=server_commands;
  advance(500); CHECK(server_commands==before && !server_rx_count && closes>=1);
  puts("comm_server: registration/status/ping/DNS/close/backpressure/ACK/address/backoff PASS");
}
static void garbage_test(void)
{
  server_reset();
  for (size_t i=0;i<sizeof(flash_model);++i) flash_model[i]=(uint8_t)(i*37u+(i>>8));
  memcpy(flash_before,flash_model,sizeof(flash_model));
  uint32_t start=sim_now, random=17; unsigned overflows=0, fragments=0; long_lines=0;
  while (sim_now-start<86400000u) {
    random=random*1664525u+1013904223u;
    if ((random&255u)==0 && !modem_slm336_busy() && !comm_tx_busy()) {
      /* Тело QIRD содержит управляющие слова, NUL и ложный SLDN/CRC. */
      uint8_t noise[256]; for (unsigned i=0;i<sizeof(noise);++i) noise[i]=(uint8_t)(random+i*11u);
      memcpy(noise+7,"OK\r\nERROR\r\nRING\r\nSLDN\0",23); enqueue(noise,sizeof(noise)); ++fragments;
    }
    if ((random&8191u)==0 && !sim_pending[0] && !modem_slm336_busy()) {
      for (unsigned i=0;i<700;++i) interrupt_byte('X',USART_SR_RXNE);
      transport_tick(10); comm_uart_stats_t stats; comm_uart_stats(&stats); CHECK(stats.overflow); ++overflows;
      /* Новая изолированная сессия модели; питание/восстановление этапа 5 не тестируются. */
      uint32_t saved=sim_now; unsigned saved_events=event13;
      server_reset(); sim_now=saved; sv.now=saved; sv.status_since=saved; event13=saved_events;
    }
    if ((random&8191u)==4096u && !sim_pending[0] && !modem_slm336_busy()) {
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
int main(void)
{ CHECK(slm_regressions()==0); server_tests(); garbage_test(); return 0; }
