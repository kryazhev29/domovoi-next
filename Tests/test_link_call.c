/* Модель среды: ACK через 300 мс после UART 115200 8N1; без Flash/камеры/платы.
   Пакет 14: до 512 байт фото + 22 байта существующего протокола.
   Управляем только физическими ответами; автоматы проекта исполняются целиком. */
#define LC_GROUP 1u
#define LC_BAUD 115200u
static struct {
  unsigned enabled, sent, acked, photo, packets, polls, stalls, errors, hold, late_kind;
  unsigned late_delay, syncs, late_seen, query_timeout;
  uint32_t late_due, sync_due, stalled_at;
  unsigned head, tail, peak, fail_at, failures, retries;
  uint32_t failed_at;
  uint16_t failed_count;
  uint8_t failed[534], stream[24000], all_stream[24000];
  unsigned all_count;
  unsigned stream_count, resume_offset, requests21, sent_lag, bad_ack;
  struct { uint32_t due, total, photo; } pending[128];
} lc;

static void lc_reply_ack(void)
{
  char reply[80];
  snprintf(reply,sizeof(reply),"+QISEND: %u,%u,%u\r\nOK\r\n",tcp_sent-lc.sent_lag,tcp_acked,tcp_sent-lc.sent_lag-tcp_acked+lc.bad_ack);
  sim_feed(reply);
}

static void lc_tick(void)
{
  if (!lc.enabled) return;
  while (!lc.hold && lc.head<lc.tail && sim_now>=lc.pending[lc.head].due) {
    tcp_acked=lc.pending[lc.head].total; lc.acked=lc.pending[lc.head].photo; ++lc.head;
  }
  if (lc.late_due && sim_now >= lc.late_due) {
    lc.late_due=0; ++lc.late_seen;
    if (lc.late_kind) sim_feed("+CSQ: 20,99\r\nOK\r\n"); else lc_reply_ack();
  }
  if (lc.sync_due && sim_now >= lc.sync_due) {
    lc.sync_due=0; sim_feed("+CMEE: 1\r\nOK\r\n");
  }
}

static bool lc_payload(void)
{
  if (!lc.enabled) return false;
  unsigned wire_ms=(sent_count*10000u+LC_BAUD-1u)/LC_BAUD;
  sim_pending[0]=0;
  if (sent_frame[8]==14) {
    if (lc.fail_at && lc.packets==lc.fail_at && !lc.failures) {
      ++lc.failures; tcp_sent-=sent_count; lc.failed_at=sim_now;
      lc.failed_count=sent_count; memcpy(lc.failed,sent_frame,sent_count);
      sim_reply("SEND FAIL\r\n",wire_ms+6u); return true;
    }
    if (lc.failures && !lc.retries) {
      CHECK(sim_now-lc.failed_at>=wire_ms+100u && lc.failed_count==sent_count);
      CHECK(!memcmp(lc.failed,sent_frame,sent_count)); ++lc.retries;
    }
    for (unsigned pos=0;pos<sent_count;) {
      uint8_t *p=sent_frame+pos; unsigned n=rt_get16(p+4)+8u;
      CHECK(n>=22 && n<=534 && pos+n<=sent_count && p[8]==14);
      CHECK(rt_get16(p+18)==lc.photo);
      CHECK(crc16_calcInit(p+4,n-6u,65535)==rt_get16(p+n-2u));
      for (unsigned i=0;i<n-22u;++i) CHECK(p[20+i]==(uint8_t)(lc.photo+i));
      lc.photo+=n-22u; lc.sent+=n; ++lc.packets; pos+=n;
    }
    CHECK(lc.stream_count+sent_count<=sizeof(lc.stream));
    memcpy(lc.stream+lc.stream_count,sent_frame,sent_count); lc.stream_count+=sent_count;
  }
  CHECK(lc.all_count+sent_count<=sizeof(lc.all_stream));
  memcpy(lc.all_stream+lc.all_count,sent_frame,sent_count); lc.all_count+=sent_count;
  CHECK(lc.tail<128);
  lc.pending[lc.tail].due=sim_now+wire_ms+300u;
  lc.pending[lc.tail].total=tcp_sent; lc.pending[lc.tail++].photo=lc.sent;
  if (tcp_sent-tcp_acked>lc.peak) lc.peak=tcp_sent-tcp_acked;
  sim_reply("SEND OK\r\n",wire_ms+6u);
  return false;
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
      snprintf(reply,sizeof(reply),"+QISEND: %u,%u,%u\r\nOK\r\n",tcp_sent-lc.sent_lag,tcp_acked,tcp_sent-lc.sent_lag-tcp_acked+lc.bad_ack);
      sim_reply(reply,20);
    }
  }
  return true;
}

/* Граница приложения: разбор TCP/CRC настоящий, курсор далее проверяется на ELF фото. */
static bool lc_request21(uint8_t *p)
{
  if (!lc.enabled || p[4]!=21) return false;
  uint32_t stamp; memcpy(&stamp,p+5,4);
  CHECK(rt_get16(p)==9 && stamp==1791987129u);
  lc.resume_offset=rt_get16(p+9); ++lc.requests21; return true;
}

/* СЦЕНАРИИ */
static void lc_reset(void)
{
  memset(&lc,0,sizeof(lc)); call_reset();
  for (unsigned i=0;comm_modem_busy() || comm_tx_busy() || tcp_sent!=tcp_acked;++i) { CHECK(i<1000); transport_tick(1); }
  lc.enabled=1;
}

static bool lc_send(unsigned offset)
{
  uint16_t cap; uint8_t *p=comm_tx_acquire(&cap);
  if (!p) return false;
  unsigned total=0;
  for (unsigned j=0;j<LC_GROUP && offset<18570u;++j) {
    unsigned bytes=18570u-offset; if (bytes>512u) bytes=512u;
    CHECK(cap>=total+bytes+22u);
    memset(p,0,bytes+22u); memcpy(p,"SLDN",4); p[4]=(uint8_t)(bytes+14u); p[5]=(uint8_t)((bytes+14u)>>8);
    p[6]=60; p[7]=7; p[8]=14; p[9]=1;
    uint32_t stamp=1791987129u, size=18570u; uint16_t off=(uint16_t)offset;
    memcpy(p+10,&stamp,4); memcpy(p+14,&size,4); memcpy(p+18,&off,2);
    for (unsigned i=0;i<bytes;++i) p[20+i]=(uint8_t)(offset+i);
    uint16_t crc=crc16_calcInit(p+4,(uint16_t)(bytes+16u),65535); memcpy(p+20+bytes,&crc,2);
    p+=bytes+22u; total+=bytes+22u; offset+=bytes;
  }
  CHECK(comm_tx_commit((uint16_t)total));
  return true;
}

static unsigned lc_stream(unsigned fail_at)
{
  lc_reset(); lc.fail_at=fail_at; unsigned queued=0; uint32_t start=sim_now;
  while (queued<18570u || comm_tx_busy() || lc.acked!=lc.sent) {
    sv.periodic_since=sim_now; /* Для побайтного сравнения входная очередь состоит только из фото. */
    CHECK(sim_now-start<60000 && comm_server_online() && !closes);
    if (queued<18570u && lc_send(queued)) queued+=18570u-queued>512u*LC_GROUP ? 512u*LC_GROUP : 18570u-queued;
    transport_tick(1);
  }
  CHECK(lc.all_count==lc.stream_count);
  CHECK(lc.photo==18570 && lc.packets==37 && lc.sent==19384 && lc.acked==lc.sent);
  unsigned elapsed=sim_now-start;
  printf("photo: bytes=%u packets=%u wire=%u acked=%u polls=%u peak=%u ms=%u\n",lc.photo,lc.packets,lc.sent,lc.acked,lc.polls,lc.peak,elapsed);
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
  while (queued<18570u || comm_tx_busy() || lc.acked!=lc.sent) {
    CHECK(sim_now-start<18000 && comm_server_online() && !closes);
    if (queued<18570u && lc_send(queued)) queued+=18570u-queued>512u*LC_GROUP ? 512u*LC_GROUP : 18570u-queued;
    transport_tick(1);
  }
  CHECK(lc.photo==18570 && lc.acked==lc.sent && lc.sent==19384 && lc.packets==37);
  printf("call: TCP unchanged, ATA=%u CHUP=%u photo=%u acked=%u elapsed=%u ms\n",answers,chups,lc.photo,lc.acked,sim_now-start);
}

static void lc_losses(void)
{
  lc_reset(); CHECK(lc_send(0)); sim_feed("+QIURC: \"closed\",0\r\n"); advance(10);
  CHECK(!comm_server_online()); advance(500); CHECK(closes==1 && opens==2);
  lc_reset(); lc.errors=1; CHECK(lc_send(0)); advance(400); CHECK(closes==1);
  lc_reset(); lc.stalls=3; CHECK(lc_send(0)); advance(1200); CHECK(closes==1 && lc.stalls==0);
  /* Успешный ответ ACK между парами таймаутов разрывает серию. */
  lc_reset(); lc.hold=1; lc.stalls=2; CHECK(lc_send(0)); advance(850);
  CHECK(!closes && lc.stalls==0); lc.stalls=2; advance(900); CHECK(!closes && lc.stalls==0);
  lc.hold=0; advance(200); CHECK(!comm_tx_busy());
  /* Новое TCP-подключение не наследует таймаут прежнего. */
  lc_reset(); lc.hold=1; lc.stalls=1; CHECK(lc_send(0)); lc_until_poll(); advance(350);
  lc.stalls=2; lc.hold=0; sim_feed("+QIURC: \"closed\",0\r\n"); advance(1500);
  CHECK(closes==1 && comm_server_online() && lc.stalls==0);
  /* Новые кадры при непустом окне не продлевают 10000 мс без продвижения ACK. */
  lc_reset(); lc.hold=1; CHECK(lc_send(0)); uint32_t since=ch.tx_since;
  unsigned queued=512;
  while (sim_now-since<9000u) {
    if (queued<3584u && sim_now-since>queued*2u && lc_send(queued)) queued+=512;
    transport_tick(1);
  }
  CHECK(queued==3584u);
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

static void lc_window_edges(void)
{
  lc_reset(); unsigned polls=lc.polls; advance(500); CHECK(lc.polls==polls);
  lc.hold=1; unsigned queued=0; uint32_t start=sim_now;
  while (sim_now-start<2000u) {
    if (queued<4096u && lc_send(queued)) queued+=512;
    transport_tick(1);
  }
  CHECK(queued==4096 && lc.photo==3584 && lc.sent==3738 && comm_tx_busy() && lc.peak<=4096);
  CHECK(!memcmp(frame,"SLDN",4) && rt_get16(frame+18)==3584);
  lc.hold=0; advance(800); CHECK(lc.photo==4096 && lc.acked==lc.sent && !closes);
  lc_reset(); ch.sent=ch.acked=tcp_sent=tcp_acked=UINT32_MAX-200u;
  CHECK(lc_send(0)); advance(800);
  CHECK(tcp_sent==333 && tcp_acked==tcp_sent && ch.acked==ch.sent && !closes);
  lc_reset(); lc.hold=1; lc.sent_lag=100; CHECK(lc_send(0)); advance(1000); CHECK(!closes);
  lc.sent_lag=0; lc.hold=0; advance(500); CHECK(ch.sent==ch.acked && !closes);
  lc_reset(); lc.bad_ack=1; CHECK(lc_send(0)); advance(400); CHECK(closes==1);
  /* Продвижение частичного ACK обновляет срок, пустой слот не отключает надзор. */
  lc_reset(); lc.hold=1; CHECK(lc_send(0)); advance(200);
  uint32_t since=ch.tx_since;
  while (sim_now-since<9000u) transport_tick(1);
  tcp_acked+=100; advance(150); CHECK(ch.tx_since>since && !closes && !comm_tx_busy());
  since=ch.tx_since;
  while (sim_now-since<=10000u) { CHECK(!closes); transport_tick(1); }
  advance(100); CHECK(closes==1);
  puts("window: bounded/queued-frame/idle/wrap/lagged-counters/bad-ACK/partial-progress/free-slot-deadline PASS");
}

static void lc_resume(void)
{
  lc_reset(); unsigned queued=0; uint32_t start=sim_now;
  while (lc.packets<12) {
    CHECK(sim_now-start<4000);
    if (lc_send(queued)) queued+=512;
    transport_tick(1);
  }
  unsigned cursor=lc.acked/534u*512u, kept=lc.acked;
  unsigned lost=lc.sent-lc.acked;
  CHECK(cursor && cursor<lc.photo && lost>534);
  lc.enabled=0; sim_feed("+QIURC: \"closed\",0\r\n");
  do { transport_tick(1); CHECK(sim_now-start<6000); } while (opens<2 || !comm_server_online() || comm_tx_busy() || comm_modem_busy() || tcp_sent!=tcp_acked);
  CHECK(closes==1 && opens==2);
  lc.head=lc.tail=0; lc.sent=lc.acked=lc.stream_count=kept;
  lc.photo=cursor; lc.packets=cursor/512u; lc.enabled=1;
  uint8_t packet[32], args[6]; uint32_t stamp=1791987129u;
  memcpy(args,&stamp,4); args[4]=(uint8_t)cursor; args[5]=(uint8_t)(cursor>>8);
  enqueue(packet,server_frame(packet,21,7,args,6));
  while (!lc.requests21) { transport_tick(1); CHECK(sim_now-start<6500); }
  CHECK(lc.resume_offset==cursor); queued=lc.resume_offset;
  while (queued<18570u || comm_tx_busy() || lc.acked!=lc.sent) {
    CHECK(sim_now-start<10000 && comm_server_online() && closes==1);
    if (queued<18570u && lc_send(queued)) queued+=18570u-queued>512u ? 512u : 18570u-queued;
    transport_tick(1);
  }
  CHECK(lc.photo==18570 && lc.sent==19384 && lc.acked==lc.sent);
  unsigned done=18570; args[4]=(uint8_t)done; args[5]=(uint8_t)(done>>8);
  enqueue(packet,server_frame(packet,21,7,args,6));
  while (lc.requests21<2) { transport_tick(1); CHECK(sim_now-start<10500); }
  CHECK(lc.resume_offset==18570);
  printf("resume: cursor=%u lost_wire=%u final=%u acked=%u PASS\n",cursor,lost,lc.resume_offset,lc.acked);
}

static void lc_mixed(const char *path)
{
  lc_reset(); unsigned phase=0; uint32_t start=sim_now;
  while (phase<4 || comm_tx_busy() || tcp_sent!=tcp_acked) {
    CHECK(!closes && sim_now-start<5000);
    if (!comm_tx_busy() && phase<4) {
      if (phase==0) user_send_statusPack(CH_SERVER);
      else if (phase==1 || phase==3) { if (!lc_send(phase==1 ? 0 : 512)) continue; }
      else { uint8_t reply[]={21,0}; user_send_anyData(11,reply,sizeof(reply),CH_SERVER,9); }
      ++phase;
    }
    transport_tick(1);
  }
  FILE *f=fopen(path,"wb"); CHECK(f); CHECK(fwrite(lc.all_stream,1,lc.all_count,f)==lc.all_count); CHECK(!fclose(f));
  puts("mixed: status/photo/response/photo, TCP alive PASS");
}

int main(int argc, char **argv)
{
  (void)previous_main;
  if (argc>2 && !strcmp(argv[1],"mixed")) { lc_mixed(argv[2]); return 0; }
  if (argc>1 && !strcmp(argv[1],"call-baseline")) { lc_call(true); return 0; }
  unsigned elapsed=lc_stream(0);
  if (argc>2) { FILE *f=fopen(argv[2],"wb"); CHECK(f); CHECK(fwrite(lc.stream,1,lc.stream_count,f)==lc.stream_count); CHECK(!fclose(f)); }
  if (argc>1 && strcmp(argv[1],"test")) return 0;
  CHECK(elapsed<4500 && lc.peak<=4096 && lc.peak>1068);
  uint8_t expected[19384]; CHECK(lc.stream_count==sizeof(expected)); memcpy(expected,lc.stream,sizeof(expected));
  lc_stream(12); CHECK(lc.failures==1 && lc.retries==1 && !closes && opens==1);
  CHECK(lc.stream_count==sizeof(expected) && !memcmp(expected,lc.stream,sizeof(expected)));
  puts("send-full: same frame retried, identical stream, TCP alive PASS");
  lc_call(false); lc_losses(); lc_background(); lc_window_edges(); lc_resume();
  CHECK(lc.stream_count==sizeof(expected) && !memcmp(expected,lc.stream,sizeof(expected)));
  puts("link-call PASS"); return 0;
}
