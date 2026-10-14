/* Модель среды: ACK через 50 мс после UART 115200 8N1; без Flash/камеры/платы.
   Пакет 14: до 512 байт фото + 22 байта существующего протокола.
   Управляем только физическими ответами; автоматы проекта исполняются целиком. */
static struct {
  unsigned enabled, sent, acked, photo, packets, polls, stalls, errors, hold, late_kind;
  unsigned late_delay, syncs, late_seen, query_timeout;
  uint32_t ack_due, late_due, sync_due, stalled_at;
} lc;

static void lc_reply_ack(void)
{
  char reply[80];
  snprintf(reply,sizeof(reply),"+QISEND: %u,%u,%u\r\nOK\r\n",lc.sent,lc.acked,lc.sent-lc.acked);
  sim_feed(reply);
}

static void lc_tick(void)
{
  if (!lc.enabled) return;
  if (!lc.hold && lc.ack_due && sim_now >= lc.ack_due) { lc.acked=lc.sent; lc.ack_due=0; }
  if (lc.late_due && sim_now >= lc.late_due) {
    lc.late_due=0; ++lc.late_seen;
    if (lc.late_kind) sim_feed("+CSQ: 20,99\r\nOK\r\n"); else lc_reply_ack();
  }
  if (lc.sync_due && sim_now >= lc.sync_due) {
    lc.sync_due=0; sim_feed("+CMEE: 1\r\nOK\r\n");
  }
}

static void lc_payload(void)
{
  if (!lc.enabled || sent_frame[8]!=14) return;
  CHECK(sent_count>=22 && sent_count<=534);
  CHECK(rt_get16(sent_frame+4)==sent_count-8u && rt_get16(sent_frame+18)==lc.photo);
  CHECK(crc16_calcInit(sent_frame+4,sent_count-6u,65535)==rt_get16(sent_frame+sent_count-2u));
  for (unsigned i=0;i<sent_count-22u;++i) CHECK(sent_frame[20+i]==(uint8_t)(lc.photo+i));
  lc.photo+=sent_count-22u; lc.sent+=sent_count; ++lc.packets;
  unsigned wire_ms=(sent_count*10000u+115199u)/115200u;
  lc.ack_due=sim_now+wire_ms+50u;
  sim_pending[0]=0; /* Заменить обычный SEND OK модели ответом с учётом времени UART. */
  sim_reply("SEND OK\r\n",wire_ms+6u);
}

static bool lc_command(const char *cmd)
{
  if (!lc.enabled) return false;
  bool ack=!strcmp(cmd,"AT+QISEND=0,0"), sync=!strcmp(cmd,"AT+CMEE?");
  bool query=lc.query_timeout && !strcmp(cmd,"AT+CSQ");
  if (lc.late_due || lc.sync_due) CHECK(sync); /* Ни ATA, ни новый опрос до границы старого ответа. */
  if (!ack && !sync && !query) return false;
  for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
  interrupt_byte(0,USART_SR_TC);
  if (sync) {
    ++lc.syncs;
    lc.sync_due=lc.late_due ? lc.late_due+20u : sim_now+30u;
  } else if (query || lc.stalls) {
    if (query) lc.query_timeout=0; else { --lc.stalls; ++lc.polls; }
    lc.stalled_at=sim_now; lc.late_kind=query;
    if (lc.late_delay) lc.late_due=sim_now+lc.late_delay;
  } else {
    ++lc.polls;
    if (lc.errors) { --lc.errors; sim_reply("ERROR\r\n",20); }
    else {
      char reply[80];
      snprintf(reply,sizeof(reply),"+QISEND: %u,%u,%u\r\nOK\r\n",lc.sent,lc.acked,lc.sent-lc.acked);
      sim_reply(reply,20);
    }
  }
  return true;
}

/* СЦЕНАРИИ */
static void lc_reset(void)
{
  memset(&lc,0,sizeof(lc)); call_reset();
  for (unsigned i=0;comm_modem_busy() || comm_tx_busy();++i) { CHECK(i<1000); transport_tick(1); }
  lc.enabled=1;
}

static bool lc_send(unsigned offset)
{
  uint16_t cap; uint8_t *p=comm_tx_acquire(&cap);
  if (!p) return false;
  unsigned bytes=18570u-offset; if (bytes>512u) bytes=512u;
  CHECK(cap>=bytes+22u);
  memset(p,0,bytes+22u); memcpy(p,"SLDN",4); p[4]=(uint8_t)(bytes+14u); p[5]=(uint8_t)((bytes+14u)>>8);
  p[6]=60; p[7]=7; p[8]=14; p[9]=1;
  uint32_t stamp=1791987129u, size=18570u; uint16_t off=(uint16_t)offset;
  memcpy(p+10,&stamp,4); memcpy(p+14,&size,4); memcpy(p+18,&off,2);
  for (unsigned i=0;i<bytes;++i) p[20+i]=(uint8_t)(offset+i);
  uint16_t crc=crc16_calcInit(p+4,(uint16_t)(bytes+16u),65535); memcpy(p+20+bytes,&crc,2);
  CHECK(comm_tx_commit((uint16_t)(bytes+22u)));
  return true;
}

static unsigned lc_stream(void)
{
  lc_reset(); unsigned queued=0; uint32_t start=sim_now;
  while (queued<18570u || comm_tx_busy()) {
    CHECK(sim_now-start<60000 && comm_server_online() && !closes);
    if (queued<18570u && lc_send(queued)) queued+=18570u-queued>512u ? 512u : 18570u-queued;
    transport_tick(1);
  }
  CHECK(lc.photo==18570 && lc.packets==37 && lc.sent==19384 && lc.acked==lc.sent);
  unsigned elapsed=sim_now-start;
  printf("photo: bytes=%u packets=%u wire=%u acked=%u polls=%u ms=%u\n",lc.photo,lc.packets,lc.sent,lc.acked,lc.polls,elapsed);
  return elapsed;
}

static void lc_until_poll(void)
{
  unsigned before=lc.polls;
  for (unsigned i=0;lc.polls==before;++i) { CHECK(i<1500); transport_tick(1); }
}

/* Журнал: +GSM Service до медленного опроса, RING через 366, CLIP через 728,
   поздний +QISEND через 731. Ответ CHUP/CLCC завершает вызов; TCP прежний. */
static void lc_call(bool baseline)
{
  lc_reset(); sim_feed("+GSM Service\r\n"); advance(1090);
  lc.hold=1; lc.stalls=1; lc.late_delay=731;
  CHECK(lc_send(0)); lc_until_poll(); uint32_t start=lc.stalled_at;
  while (sim_now-start<301u) transport_tick(1);
  if (baseline) { CHECK(!comm_server_online()); puts("baseline call: TCP closed at first 300 ms timeout"); return; }
  CHECK(comm_server_online() && !closes && !comm_channel_call_pending());
  while (sim_now-start<366u) transport_tick(1);
  ring(NULL);
  while (sim_now-start<728u) transport_tick(1);
  sim_feed("+CLIP: \"79635775629\",129\r\n");
  while (sim_now-start<735u) transport_tick(1);
  CHECK(lc.late_seen==1 && lc.syncs==1 && !answers && comm_modem_busy());
  while (!chups || comm_channel_call_pending()) { CHECK(sim_now-start<7000); transport_tick(1); }
  sim_feed("+CIEV: \"CALL\",0\r\n");
  CHECK(answers==1 && chups==1 && !call_present && comm_server_online() && !closes && opens==1);
  lc.hold=0;
  for (unsigned i=0;comm_tx_busy();++i) { CHECK(i<500); transport_tick(1); }
  unsigned queued=512;
  while (queued<18570u || comm_tx_busy()) {
    CHECK(sim_now-start<18000 && comm_server_online() && !closes);
    if (queued<18570u && lc_send(queued)) queued+=18570u-queued>512u ? 512u : 18570u-queued;
    transport_tick(1);
  }
  CHECK(lc.photo==18570 && lc.acked==lc.sent && lc.sent==19384 && lc.packets==37);
  printf("call: TCP unchanged, ATA=%u CHUP=%u photo=%u acked=%u elapsed=%u ms\n",answers,chups,lc.photo,lc.acked,sim_now-start);
}

static void lc_losses(void)
{
  lc_reset(); CHECK(lc_send(0)); sim_feed("+QIURC: \"closed\",0\r\n"); advance(10);
  CHECK(!comm_server_online()); advance(500); CHECK(closes==1 && opens==2);
  lc_reset(); lc.errors=1; CHECK(lc_send(0)); advance(200); CHECK(closes==1);
  lc_reset(); lc.stalls=3; CHECK(lc_send(0)); advance(1200); CHECK(closes==1 && lc.stalls==0);
  /* Успешный ответ ACK между парами таймаутов разрывает серию. */
  lc_reset(); lc.hold=1; lc.stalls=2; CHECK(lc_send(0)); advance(850);
  CHECK(!closes && lc.stalls==0); lc.stalls=2; advance(900); CHECK(!closes && lc.stalls==0);
  lc.hold=0; advance(200); CHECK(!comm_tx_busy());
  /* Новое TCP-подключение не наследует таймаут прежнего. */
  lc_reset(); lc.hold=1; lc.stalls=1; CHECK(lc_send(0)); lc_until_poll(); advance(350);
  lc.stalls=2; lc.hold=0; sim_feed("+QIURC: \"closed\",0\r\n"); advance(1500);
  CHECK(closes==1 && comm_server_online() && lc.stalls==0);
  /* Ровно прежний срок: >10000 мс от захвата кадра, даже при успешных опросах. */
  lc_reset(); lc.hold=1; CHECK(lc_send(0)); uint32_t since=ch.tx_since;
  while (sim_now-since<=10000u) { CHECK(!closes && comm_server_online()); transport_tick(1); }
  advance(100); CHECK(closes==1);
  lc_reset(); comm_channel_call(true); comm_channel_call(false);
  drop_status=1; advance(21000); CHECK(closes>=1); /* Нет ответа сервера после голоса. */
  puts("losses: closed, ERROR, 3 timeouts, counter reset/session, 10s unacked, server silence PASS");
}

static unsigned lc_query_done;
static void lc_query_line(const uint8_t *p, uint16_t n)
{ if (!p) { CHECK(n==1); ++lc_query_done; } }

static void lc_background(void)
{
  lc_reset(); lc.query_timeout=1; lc.late_delay=731; lc_query_done=0;
  CHECK(comm_channel_query("AT+CSQ",lc_query_line));
  for (unsigned i=0;!lc.stalled_at;++i) { CHECK(i<1000); transport_tick(1); }
  advance(740); CHECK(lc_query_done==1 && !closes && comm_modem_busy() && lc.late_seen==1);
  advance(50); CHECK(!closes && comm_server_online());
  puts("background: late CSQ drained without TCP loss PASS");
}

int main(int argc, char **argv)
{
  (void)previous_main;
  if (argc>1 && !strcmp(argv[1],"call-baseline")) { lc_call(true); return 0; }
  unsigned elapsed=lc_stream();
  if (argc>1) return 0;
  CHECK(elapsed<9000);
  lc_call(false); lc_losses(); lc_background();
  puts("link-call PASS"); return 0;
}
