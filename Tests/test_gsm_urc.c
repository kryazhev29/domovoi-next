// Доработка: Кряжев А., 2026
/* A URC that arrives while an AT command is running must still reach gsm_read.
   Log of 20.09.2026: +QIURC: "recv" answered AT+CREG?, the next command cleared the buffer
   and the 13-byte server frame was gone. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define DEBUG_GSM
#define GSM_PORT 0
#define GSMRESPTIMEOUT 500
typedef uint8_t Int8U;
typedef uint16_t Int16U;

/* The receive buffer the uart driver keeps for the modem port. */
static uint8_t rx[2048];
static uint16_t rxlen;
static char gsm_strbuf[128];

static uint16_t uart_received(Int8U num) { (void)num; return rxlen; }
static uint8_t *uart_getptr(Int8U num) { (void)num; return rx; }
static void uart_clear(Int8U num) { (void)num; rxlen = 0; }

/* One scripted modem answer per AT command, in order. */
static const char *answer[8];
static uint16_t answerlen[8];
static unsigned answers, answer_i;

static void script(const char *text, uint16_t len) { answer[answers] = text; answerlen[answers] = len; answers++; }
static void pending(const char *text, uint16_t len) { memcpy(rx, text, len); rxlen = len; }

static int uart_sendStrAndWaitResp(Int8U num, char *str, char *rec, Int16U recsize, Int16U t_ms, char *pattern)
{
  (void)num; (void)str; (void)t_ms; (void)pattern;
  uart_clear(GSM_PORT);                       /* the driver drops whatever was in the buffer */
  if (answer_i < answers) {
    rxlen = answerlen[answer_i];
    memcpy(rx, answer[answer_i], rxlen);
    answer_i++;
  }
  if (rec != NULL) {                          /* bounded like the driver, Int16U recsize */
    Int16U len = rxlen;
    if (recsize == 0) len = 0;
    else if (len > (Int16U)(recsize - 1)) len = (Int16U)(recsize - 1);
    memcpy(rec, rx, len);
    rec[len] = 0;
  }
  return 1;
}
#define gsm_sendStrAndWaitResp(a,b,c,d) uart_sendStrAndWaitResp(GSM_PORT, a, b, sizeof(gsm_strbuf), d, c)
#define gsm_debug(a) ((void)(a))

/* What the main loop handed to the parser. */
static uint8_t seen[2048];
static uint16_t seenlen;
static unsigned reads;
static void gsm_read(char *ptr, Int16U size) { reads++; seenlen = size; memcpy(seen, ptr, size); }

/* Счётчик с выравниванием и заголовком basic heap IAR. */
static size_t heap_live, heap_peak;
static void *checked_calloc(size_t count,size_t size) {
 size_t n=count*size, use=(n+15u)&~7u;
 size_t *p=calloc(1,n+2*sizeof(size_t));CHECK(p);p[0]=use;
 heap_live+=use;if(heap_live>heap_peak)heap_peak=heap_live;return p+2;
}
static void checked_free(void *ptr) {if(ptr){size_t *p=(size_t*)ptr-2;heap_live-=p[0];free(p);}}
static void *checked_realloc(void *ptr,size_t n) {
 size_t *p=(size_t*)ptr-2, old=p[0], use=(n+15u)&~7u;
 p=realloc(p,n+2*sizeof(size_t));CHECK(p);p[0]=use;heap_live=heap_live-old+use;
 if(heap_live>heap_peak)heap_peak=heap_live;return p+2;
}
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
/* PRODUCTION */


static const char creg[] = "\r\n+CREG: 1,1\r\n\r\nOK\r\n";
static const char cgreg[] = "\r\n+CGREG: 1,1\r\n\r\nOK\r\n";
#define CREG_LEN ((uint16_t)(sizeof creg - 1))
#define CGREG_LEN ((uint16_t)(sizeof cgreg - 1))

/* +QIURC: "recv",<id>,<len><CR><LF><data>; the frame carries a zero byte on purpose. */
static char frame[2048];
static uint16_t framelen;
static char frame2[2048];
static uint16_t frame2len;
static char big[2048];
static uint16_t biglen;

static uint16_t build(char *out, const char *head, uint16_t body)
{
  uint16_t head_len = (uint16_t)strlen(head), i;
  memcpy(out, head, head_len);
  out[head_len + 0] = 'S'; out[head_len + 1] = 'L'; out[head_len + 2] = 'D'; out[head_len + 3] = 'N';
  for (i = 4; i < body; i++) out[head_len + i] = (char)((i == 5) ? 0 : (i * 7) & 0xff);
  return (uint16_t)(head_len + body);
}

static void reset(void) { answers = 0; answer_i = 0; reads = 0; seenlen = 0; rxlen = 0; }

int main(void)
{
  framelen = build(frame, "\r\n+QIURC: \"recv\",0,13\r\n", 13);
  frame2len = build(frame2, "\r\n+QIURC: \"recv\",0,20\r\n", 20);
  biglen = build(big, "\r\n+QIURC: \"recv\",0,600\r\n", 600);

  /* The 20.09 case: the URC answers AT+CREG?, the next command would have wiped it. */
  reset();
  script(frame, framelen);
  script(creg, CREG_LEN);
  gsm_runcmd("AT+CREG?\r", "OK");
  gsm_runcmd("AT+CGREG?\r", "OK");
  gsm_takeurc();
  CHECK(reads == 1);
  CHECK(seenlen == (uint16_t)(framelen - 2));            /* from "+QIURC:" on, without the leading CRLF */
  CHECK(memcmp(seen, frame + 2, seenlen) == 0);          /* byte for byte, zero inside the frame kept */

  /* Ordinary answers are left alone: the main loop reads the last one from the port itself. */
  reset();
  script(creg, CREG_LEN);
  script(cgreg, CGREG_LEN);
  gsm_runcmd("AT+CREG?\r", "OK");
  gsm_runcmd("AT+CGREG?\r", "OK");
  gsm_takeurc();
  CHECK(reads == 0);

  /* A URC already in the buffer when the command is sent is kept, not cleared. */
  reset();
  pending(frame, framelen);
  script(creg, CREG_LEN);
  gsm_runcmd("AT+CREG?\r", "OK");
  gsm_takeurc();
  CHECK(reads == 1 && seenlen == (uint16_t)(framelen - 2) && memcmp(seen, frame + 2, seenlen) == 0);

  /* Two URCs inside one sequence of commands: both are delivered, in order. */
  reset();
  script(frame, framelen);
  script(frame2, frame2len);
  script(creg, CREG_LEN);
  gsm_runcmd("AT+CREG?\r", "OK");
  gsm_runcmd("AT+CGREG?\r", "OK");
  gsm_runcmd("AT+CSQ\r", "OK");
  gsm_takeurc();
  CHECK(reads == 1);
  CHECK(seenlen == (uint16_t)((framelen - 2) + frame2len));
  CHECK(memcmp(seen, frame + 2, (size_t)(framelen - 2)) == 0);
  CHECK(memcmp(seen + (framelen - 2), frame2, (size_t)frame2len) == 0);

  /* A frame far longer than gsm_strbuf survives whole, the answer copy stays inside its buffer. */
  reset();
  script(big, biglen);
  script(creg, CREG_LEN);
  gsm_runcmd("AT+CREG?\r", "OK");
  gsm_runcmd("AT+CGREG?\r", "OK");
  gsm_takeurc();
  CHECK(strlen(gsm_strbuf) < sizeof gsm_strbuf);
  CHECK(reads == 1 && seenlen == (uint16_t)(biglen - 2) && memcmp(seen, big + 2, seenlen) == 0);

  /* Nothing pending, nothing parsed. */
  reset();
  gsm_takeurc();
  CHECK(reads == 0);

  reset();memset(big,'x',1024);memcpy(big,"+IPD",4);
  pending(big,1024);gsm_stashurc();pending(big,1024);gsm_stashurc();
  CHECK(gsm_urclen==2048 && heap_peak==2064 && heap_live==2064);
  gsm_takeurc();CHECK(!heap_live && seenlen==2048);
  puts("PASS URC in an AT answer: kept for the main loop, byte for byte, before and after the command, two in a row, longer than the answer buffer");
  return 0;
}
