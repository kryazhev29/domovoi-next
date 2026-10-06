/* Доработка: Кряжев А., 2026. AT на настоящем comm_uart с моделью IRQ.
   Все ответы ниже, кроме scenarios из журналов, — СИНТЕТИКА. */
static uint8_t received[1600];
static unsigned binary_count, done_count, urc_count, line_events, no_answer;
static unsigned binary_calls, rejected, sink_pause, sink_bad, inject_error, stream_errors;
static uint16_t last_result, last_code;
static uint32_t model_now;

static void on_event(uint8_t kind, const uint8_t *data, uint16_t count, uint16_t detail)
{
  if (kind == COMM_AT_DONE) { done_count++; last_result = detail; last_code = count; }
  else if (kind == COMM_AT_NO_ANSWER) no_answer++;
  else if (kind == COMM_AT_STREAM_ERROR) { stream_errors++; last_result = detail; }
  else if (kind == COMM_AT_URC) {
    CHECK(data && count && !detail);
    urc_count++;
  } else { CHECK(kind == COMM_AT_LINE && data && count && !detail); line_events++; }
}

static uint16_t on_binary(const uint8_t *data, uint16_t count)
{
  CHECK(data >= rx && data < rx + sizeof(rx)); /* Только вид UART, без копии. */
  CHECK(count <= COMM_AT_POLL_BYTES);
  binary_calls++;
  if (inject_error) { inject_error = 0; interrupt_byte(0, USART_SR_FE); }
  if (sink_bad) return (uint16_t)(count + 1u);
  if (sink_pause) return 0;
  CHECK(binary_count + count <= sizeof(received));
  memcpy(received + binary_count, data, count);
  binary_count += count;
  return count;
}

static void reset_at(void)
{
  comm_at_handlers_t callbacks = {on_event, on_binary};
  primask = pending_sr = edge_at = barriers = reads = writes = 0;
  comm_uart_init(115200);
  comm_at_init(&callbacks, 300);
  binary_count = done_count = urc_count = line_events = no_answer = 0;
  binary_calls = rejected = sink_pause = sink_bad = inject_error = stream_errors = 0;
  last_result = last_code = UINT16_MAX;
  model_now = 0;
}

static void tc(void)
{
  while (regs.CR1 & USART_CR1_TXEIE) {
    CHECK(tx_data == command); /* В UART живёт ровно статический владелец AT. */
    interrupt_byte(0, USART_SR_TXE);
  }
  interrupt_byte(0, USART_SR_TC);
}

static uint8_t start(const char *text, uint8_t type, uint32_t now)
{
  comm_at_request_t req = {(const uint8_t *)text, (uint16_t)strlen(text), type, 100};
  model_now = now;
  return comm_at_start(&req, now);
}

static void poll(void) { comm_at_poll(model_now); }

static void feed(const uint8_t *data, unsigned count, unsigned chunk)
{
  if (!chunk) chunk = 400;
  while (count) {
    unsigned n = count < chunk ? count : chunk;
    for (unsigned i = 0; i < n; ++i) interrupt_byte(data[i], USART_SR_RXNE);
    data += n; count -= n;
    const uint8_t *view;
    do { poll(); } while (state != FAULT && !sink_pause && comm_uart_peek(&view));
  }
}

static void text(const char *s) { feed((const uint8_t *)s, (unsigned)strlen(s), 0); }

static void split_tests(void)
{
  const char *reply = "RING\r\nAT+CSQ\r\n+QIURC: \"recv\",0\r\n+CSQ: 15,0\r\n+CLIP: \"123\"\r\nOK\r\nSMS Ready\r\n";
  unsigned n = (unsigned)strlen(reply);
  for (unsigned cut = 0; cut <= n; ++cut) {
    reset_at(); CHECK(start("AT+CSQ", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
    feed((const uint8_t *)reply, cut, 0);
    feed((const uint8_t *)reply + cut, n - cut, 0);
    CHECK(!comm_at_busy() && done_count == 1 && last_result == COMM_AT_OK);
    CHECK(urc_count == 4 && line_events == 1);
  }
  reset_at(); CHECK(start("AT+CSQ", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  feed((const uint8_t *)reply, n, 1);
  CHECK(done_count == 1 && urc_count == 4 && line_events == 1);
  reset_at(); CHECK(start("ATE0", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("ATE0\r\nOK\r\n"); CHECK(done_count == 1);
  CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("XOK\r\nOKAY\r\nERRORX\r\n+CME ERROR: 100junk\r\n"); CHECK(done_count == 1 && comm_at_busy());
  text("+CME ERROR: 100\r\n"); CHECK(done_count == 2 && last_result == COMM_AT_CME && last_code == 100);
  CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("ERROR\r\n"); CHECK(last_result == COMM_AT_ERROR);
  CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("+CME ERROR: SIM not inserted\r\n"); CHECK(last_result == COMM_AT_CME && last_code == UINT16_MAX);
}

static void prompt_tests(void)
{
  const char *reply = "AT+QISEND=0,5\r\nRING\r\n> ";
  for (unsigned cut = 0; cut <= strlen(reply); ++cut) {
    reset_at(); CHECK(start("AT+QISEND=0,5", COMM_AT_PROMPT, 0) == COMM_AT_ACCEPTED); tc();
    feed((const uint8_t *)reply, cut, 0);
    feed((const uint8_t *)reply + cut, (unsigned)strlen(reply) - cut, 0);
    CHECK(done_count == 1 && last_result == COMM_AT_GOT_PROMPT && !comm_at_busy());
    CHECK(urc_count == 1);
  }
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text(">\r\n"); CHECK(comm_at_busy() && done_count == 0);
  reset_at(); CHECK(start("AT+QISEND=0,5", COMM_AT_PROMPT, 0) == COMM_AT_ACCEPTED); tc();
  text(">"); CHECK(last_result == COMM_AT_GOT_PROMPT);
  CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text(" OK\r\n"); CHECK(done_count == 2 && last_result == COMM_AT_OK);
  reset_at(); CHECK(start("AT+QISEND=0,5", COMM_AT_PROMPT, 0) == COMM_AT_ACCEPTED); tc();
  text("X>Y\r\n"); CHECK(done_count == 0 && comm_at_busy());
  text(">"); CHECK(done_count == 1 && last_result == COMM_AT_GOT_PROMPT);
}

static void body_tests(void)
{
  static const uint8_t body[] = {'O','K','\r','\n','E','R','R','O','R','\r','\n','R','I','N','G','\r','\n','S','L','D','N',0,'>'};
  uint8_t reply[200];
  for (unsigned format = COMM_AT_SLM_QIRD; format <= COMM_AT_SLM_QIRD; ++format) {
    const char *cmd = "AT+QIRD=0,64";
    int n = snprintf((char *)reply, sizeof(reply), "RING\r\n+QIRD: %u\r\n", (unsigned)sizeof(body));
    CHECK(n > 0 && (unsigned)n + sizeof(body) + 25 < sizeof(reply));
    memcpy(reply + n, body, sizeof(body)); n += sizeof(body);
    const char *tail = "\r\n+QIRDI: 0,1,0\r\nOK\r\nRDY\r\n";
    memcpy(reply + n, tail, strlen(tail)); n += (int)strlen(tail);
    for (unsigned cut = 0; cut <= (unsigned)n; ++cut) {
      reset_at(); CHECK(start(cmd, format, 0) == COMM_AT_ACCEPTED); tc();
      feed(reply, cut, 0); feed(reply + cut, (unsigned)n - cut, 0);
      CHECK(done_count == 1 && last_result == COMM_AT_OK && !comm_at_busy());
      CHECK(binary_count == sizeof(body) && !memcmp(received, body, sizeof(body)) && urc_count == 3);
    }
    reset_at(); CHECK(start(cmd, format, 0) == COMM_AT_ACCEPTED); tc(); feed(reply, (unsigned)n, 1);
    CHECK(done_count == 1 && binary_count == sizeof(body));
  }
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 0\r\n\r\nOK\r\n"); CHECK(last_result == COMM_AT_OK && binary_count == 0);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 2\r\nxy\r\nOK\r\n"); CHECK(last_result == COMM_AT_OK && binary_count == 2);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 11\r\nOK\r\n"); CHECK(last_result == COMM_AT_BAD_LENGTH && comm_at_busy() && done_count == 1);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 99999999999999999999\r\nOK\r\n"); CHECK(last_result == COMM_AT_BAD_LENGTH);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 2\r\nxyz\r\nOK\r\n"); CHECK(last_result == COMM_AT_BAD_LENGTH && binary_count == 2);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 10\r\nxy"); model_now = 100; poll();
  CHECK(last_result == COMM_AT_TIMEOUT && comm_at_busy() && done_count == 1);
  text("OK\r\n"); CHECK(done_count == 1); /* После потери тела поиск OK запрещён. */
  const char *bad_headers[] = {"+QIRD: -1\r\n", "+QIRD: 1,0\r\n", "+QIRD: 1x\r\n", "+QIRD: \r\n"};
  for (unsigned i = 0; i < sizeof(bad_headers) / sizeof(bad_headers[0]); ++i) {
    reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
    text(bad_headers[i]); CHECK(last_result == COMM_AT_BAD_LENGTH && !binary_count);
  }
  reset_at(); CHECK(start("AT+CSQ", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 4\r\nOK\r\n\r\nOK\r\n");
  CHECK(last_result == COMM_AT_BAD_FRAME && done_count == 1 && comm_at_busy());
}

static void bounds_tests(void)
{
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED);
  const uint8_t *owner = tx_data;
  CHECK(start("AT+CSQ", COMM_AT_TEXT, 0) == COMM_AT_BUSY && tx_data == owner);
  text("OK\r\n"); CHECK(comm_at_busy() && done_count == 0); /* До физического TC. */
  tc(); poll(); CHECK(done_count == 1 && !comm_at_busy());
  char long_command[130]; memset(long_command, 'A', sizeof(long_command));
  long_command[1] = 'T'; long_command[127] = 0;
  CHECK(start(long_command, COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc(); text("OK\r\n");
  uint8_t large_line[193]; memset(large_line, 'X', sizeof(large_line));
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  feed(large_line, sizeof(large_line) - 1, 0); text("\r\nOK\r\n"); CHECK(last_result == COMM_AT_OK && line_events == 1);
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  feed(large_line, sizeof(large_line), 0); text("\r\nOK\r\n");
  CHECK(last_result == COMM_AT_LINE_OVERFLOW && done_count == 1 && comm_at_busy());
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  text("O\rK\n"); CHECK(last_result == COMM_AT_BAD_FRAME);
}

static void budget_tests(void)
{
  reset_at(); CHECK(start("AT+QIRD=0,1500", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 1500\r\n");
  for (unsigned i = 0; i < 400; ++i) interrupt_byte('X', USART_SR_RXNE);
  const uint8_t *view;
  poll(); CHECK(binary_count == COMM_AT_POLL_BYTES && comm_uart_peek(&view) == 400 - COMM_AT_POLL_BYTES);
  sink_pause = 1; unsigned old = binary_count; poll(); CHECK(binary_count == old);
  sink_pause = 0; poll(); CHECK(binary_count == old + COMM_AT_POLL_BYTES);
  poll(); poll();
  uint8_t payload[1100]; memset(payload, 'X', sizeof(payload)); feed(payload, sizeof(payload), 200);
  text("\r\nOK\r\n"); CHECK(last_result == COMM_AT_OK && binary_count == 1500);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 1\r\n"); sink_bad = 1; text("x\r\nOK\r\n"); CHECK(last_result == COMM_AT_SINK_ERROR);
  reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 2\r\n"); inject_error = 1; text("xy\r\nOK\r\n"); CHECK(last_result == COMM_AT_UART_ERROR && done_count == 1);
  reset_at(); CHECK(start("AT+QIRD=0,1500", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 1500\r\n");
  for (unsigned i = 0; i < sizeof(rx); ++i) interrupt_byte('X', USART_SR_RXNE);
  poll(); CHECK(last_result == COMM_AT_UART_ERROR && !binary_count);
  for (unsigned kind = COMM_UART_ORE; kind <= COMM_UART_NE; kind <<= 1) {
    reset_at(); CHECK(start("AT+QIRD=0,10", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
    text("+QIRD: 2\r\n");
    interrupt_byte('x', kind == COMM_UART_ORE ? USART_SR_ORE : kind == COMM_UART_FE ? USART_SR_FE : USART_SR_NE);
    text("y\r\nOK\r\n"); CHECK(last_result == COMM_AT_UART_ERROR && done_count == 1 && !binary_count);
  }
  reset_at(); CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  for (unsigned i = 0; i < 80; ++i) {
    interrupt_byte('R', USART_SR_RXNE); interrupt_byte('I', USART_SR_RXNE); interrupt_byte('N', USART_SR_RXNE);
    interrupt_byte('G', USART_SR_RXNE); interrupt_byte('\r', USART_SR_RXNE); interrupt_byte('\n', USART_SR_RXNE);
  }
  poll(); CHECK(urc_count == COMM_AT_POLL_BYTES / 6u);
}

static void idle_fragment_tests(void)
{
  const char *fragments[] = {"\r", "+QIURC: \"recv\""};
  for (unsigned i=0; i<2; ++i) {
    reset_at(); model_now=500000; text(fragments[i]);
    model_now=500299; poll(); CHECK(state==IDLE && !stream_errors);
    model_now=500300; poll();
    CHECK(state==FAULT && stream_errors==1 && last_result==COMM_AT_BAD_FRAME);
  }
  reset_at(); model_now=UINT32_MAX-99u; text("RIN");
  model_now=199; poll(); CHECK(state==IDLE && !stream_errors);
  text("G\r\n"); CHECK(urc_count==1);
  model_now=201; poll(); CHECK(!stream_errors);
  CHECK(start("AT",COMM_AT_TEXT,model_now)==COMM_AT_ACCEPTED); tc(); text("OK\r\n");
  CHECK(done_count==1 && last_result==COMM_AT_OK);
}
static void timeout_tests(void)
{
  reset_at(); comm_at_init(&(comm_at_handlers_t){on_event, on_binary}, 2000);
  CHECK(start("AT+CSQ", COMM_AT_TEXT, UINT32_MAX - 49u) == COMM_AT_ACCEPTED); tc();
  model_now = 49; poll(); CHECK(!done_count);
  model_now = 50; poll();
  CHECK(done_count == 1 && last_result == COMM_AT_TIMEOUT && comm_at_busy());
  /* Старый финал потерян: запрос синхронизации уже передаётся. */
  CHECK(state == SYNC_WAIT && command_count == 9 && !memcmp(command, "AT+CMEE?\r", 9));
  CHECK(start("AT+CREG?", COMM_AT_TEXT, model_now) == COMM_AT_BUSY); tc();
  text("OK\r\nERROR\r\nRING\r\n"); CHECK(comm_at_busy() && done_count == 1);
  text("+CMEE: 2\r\nOK\r\n"); CHECK(!comm_at_busy());
  /* Поздний старый OK после синхронизации в покое не создаёт результата. */
  text("OK\r\n"); CHECK(!comm_at_busy() && done_count == 1);
  CHECK(start("AT+CREG?", COMM_AT_TEXT, model_now) == COMM_AT_ACCEPTED); tc();
  text("+CREG: 0,1\r\nOK\r\n"); CHECK(done_count == 2 && last_result == COMM_AT_OK);
  reset_at(); comm_at_init(&(comm_at_handlers_t){on_event, on_binary}, 2000);
  CHECK(start("AT", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  model_now = 100; poll(); tc(); text("+CMEE: 22\r\nOK\r\n");
  model_now = 2099; poll(); CHECK(!no_answer && comm_at_busy());
  model_now = 2100; poll(); CHECK(no_answer == 1 && comm_at_busy()); poll(); CHECK(no_answer == 1);
  reset_at(); CHECK(start("AT+CSQ", COMM_AT_TEXT, 0) == COMM_AT_ACCEPTED); tc();
  model_now = 100; poll();
  text("OK\r\n+CMEE: 2\r\nOK\r\n"); CHECK(state == SYNC_COMPLETE && comm_at_busy());
  tc(); poll(); CHECK(!comm_at_busy());
  for (unsigned format = COMM_AT_SLM_QIRD; format <= COMM_AT_SLM_QIRD; ++format) {
    reset_at();
    CHECK(start("AT+QIRD=0,32", format, 0) == COMM_AT_ACCEPTED); tc();
    text("+QIRD: 10\r\nxy");
    model_now = 100; poll(); CHECK(state == DRAIN && binary_count == 2);
    text("OK\r\nRING"); /* Восемь оставшихся байт тела. Финала нет. */
    CHECK(state == DRAIN && binary_count == 2);
    text("\r\n"); CHECK(state == SYNC_WAIT && done_count == 1); tc();
    text("OK\r\n+CMEE: 2\r\nOK\r\n"); CHECK(!comm_at_busy() && binary_count == 2);
  }
  /* Заголовок старого QIRD может запоздать до уже отправленного CMEE?. */
  reset_at(); CHECK(start("AT+QIRD=0,32", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  model_now = 100; poll(); tc();
  text("+QIRD: 4\r\nOK\r\n\r\nOK\r\n+CMEE: 2\r\nOK\r\n");
  CHECK(!comm_at_busy() && done_count == 1 && !binary_calls);
  reset_at(); CHECK(start("AT+QIRD=0,32", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  text("+QIRD: 10\r\nxy"); model_now = 100; poll(); text("OK\r\n");
  CHECK(state == DRAIN && done_count == 1); model_now = 400; poll(); CHECK(no_answer == 1 && state == FAULT);
}

/* Поддельная метка CMEE внутри позднего тела не завершает синхронизацию. */
static void late_marker_test(void)
{
  reset_at(); CHECK(start("AT+QIRD=0,32", COMM_AT_SLM_QIRD, 0) == COMM_AT_ACCEPTED); tc();
  model_now = 100; poll(); CHECK(state == SYNC_WAIT); tc();
  text("+QIRD: 14\r\n+CMEE: 2\r\nOK\r\n\r\nOK\r\n");
  CHECK(state == SYNC_WAIT && comm_at_busy() && !sync_seen && !binary_count && done_count == 1);
  text("+CMEE: 2\r\nOK\r\n"); CHECK(!comm_at_busy() && !binary_count && done_count == 1);
}

static void capture_tests(void)
{
  for (unsigned scenario = 0; scenario < sizeof(scenarios) / sizeof(scenarios[0]); ++scenario) {
    unsigned baseline_urc = 0, baseline_lines = 0, baseline_done = 0;
    for (unsigned split = 0; split < 2; ++split) {
      reset_at();
      for (unsigned j = 0; j < scenario_counts[scenario]; ++j) {
        const sim_step_t *step = &scenarios[scenario][j];
        if (step->tx) {
          comm_at_request_t request = {step->data, step->count,
              comm_at_prefix(step->data, step->count, "AT+QISEND=") ? COMM_AT_PROMPT : COMM_AT_TEXT, 100};
          unsigned before = writes;
          uint8_t accepted = comm_at_start(&request, model_now);
          CHECK(accepted != COMM_AT_INVALID);
          if (accepted == COMM_AT_BUSY) { rejected++; CHECK(writes == before); }
          else {
            tc(); CHECK(writes == before + step->count + 1u);
            CHECK(!memcmp(wire + before, step->data, step->count) && wire[writes - 1u] == '\r');
          }
        } else feed(step->data, step->count, split ? 1 : 0);
      }
      CHECK(!binary_count && !no_answer && state != FAULT && urc_count > 0);
      if (!split) { baseline_urc = urc_count; baseline_lines = line_events; baseline_done = done_count; }
      else CHECK(urc_count == baseline_urc && line_events == baseline_lines && done_count == baseline_done);
    }
  }
}

/* Пробелы перед октетами сохраняются; граница каждого — 255. */
static void octet_tests(void)
{
  uint32_t ip[4];
  const char *text = " 1. 2. 3. 255";
  CHECK(comm_at_parse((const uint8_t *)text, (uint16_t)strlen(text), "\006.\006.\006.\006", ip));
  CHECK(ip[0] == 1 && ip[1] == 2 && ip[2] == 3 && ip[3] == 255);
  text = "1.2.3.256";
  CHECK(!comm_at_parse((const uint8_t *)text, (uint16_t)strlen(text), "\006.\006.\006.\006", ip));
}

int main(void)
{
  octet_tests(); split_tests(); prompt_tests(); body_tests(); bounds_tests(); budget_tests(); idle_fragment_tests(); timeout_tests(); late_marker_test(); capture_tests();
  puts("comm_at: split/concat/URC/echo/prompt/QIRD/late/wrap/owner/budget/captures PASS");
  return 0;
}
