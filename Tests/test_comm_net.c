/* Снимок причины задержки TCP: настоящий планировщик AT, ответы модема моделируются. */
static void net_diagnostic_tests(void)
{
  for (unsigned mode=0; mode<3; ++mode) {
    server_reset(); advance(1000);
    unsigned closed=closes;
    net_log_mask=mode ? 1 : 0; stalled_error=mode==2;
    tx_unacked=1; user_send_statusPack(CH_SERVER);
    uint32_t since=sim_now;
    advance(4900); CHECK(!stalled_count && closes==closed);
    advance(3000); CHECK(stalled_count==(mode ? 5u : 0u) && closes==closed);
    if (mode) CHECK(stalled_first-since>=5000 && stalled_first-since<6000);
    if (mode==1) {
      /* Продвижение ACK разрешает один новый снимок при следующей задержке. */
      tx_unacked=0; advance(500); CHECK(ch.sent==ch.acked && !ch.diagnostic);
      stalled_count=0; tx_unacked=1; user_send_statusPack(CH_SERVER); advance(8000);
      CHECK(stalled_count==5 && closes==closed);
    }
    advance(3000); CHECK(closes>closed);
  }
  net_log_mask=0;
  puts("net diagnostic: USB off/on, 5s threshold, one snapshot, ACK rearm, ERROR harmless, 10s close PASS");
}
