#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define DEBUG_GSM
typedef uint8_t Int8U;
typedef enum {CREG, CGREG} gsmresp_e;
static struct {Int8U creg, cgreg, regSilent, regSkip;} GSM;
static char buf[128];
static unsigned creg_sent, cgreg_sent, events;
static int modem_alive;
#define gsm_debug(s) ((void)(s))
static char *gsm_runcmd(char *str, char *resp)
{
  (void)resp;
  if (!strcmp(str, "AT+CREG?\r")) { creg_sent++; strcpy(buf, modem_alive ? "\r\n+CREG: 1,1\r\n\r\nOK\r\n" : ""); }
  else if (!strcmp(str, "AT+CGREG?\r")) { cgreg_sent++; strcpy(buf, modem_alive ? "\r\n+CGREG: 1,1\r\n\r\nOK\r\n" : ""); }
  else CHECK(0);
  return buf;
}
static void gsm_dataread(gsmresp_e op, char *p1, char *p2, uint32_t v1, int32_t v2) { (void)op; (void)p1; (void)p2; (void)v1; (void)v2; events++; }
/* PRODUCTION */
static void poll(unsigned n) { while (n--) gsm_getregstate(); }
int main(void)
{
  unsigned c0, g0;
  /* Answering module: both queries on every poll, state reported once. */
  modem_alive = 1; poll(3);
  CHECK(creg_sent == 3 && cgreg_sent == 3 && GSM.creg == 1 && GSM.cgreg == 1 && events == 2);
  /* Silent module: no +CGREG after an unanswered +CREG. */
  modem_alive = 0; c0 = creg_sent; g0 = cgreg_sent;
  poll(2); CHECK(creg_sent == c0 + 2 && cgreg_sent == g0);
  /* After two silent polls only every third call reaches the module. */
  poll(2); CHECK(creg_sent == c0 + 2);
  poll(1); CHECK(creg_sent == c0 + 3);
  poll(3); CHECK(creg_sent == c0 + 4 && cgreg_sent == g0);
  /* The module answers again: the next reaching poll restores full polling. */
  modem_alive = 1; poll(2); CHECK(creg_sent == c0 + 4);
  poll(1); CHECK(creg_sent == c0 + 5 && cgreg_sent == g0 + 1 && GSM.regSilent == 0);
  poll(2); CHECK(creg_sent == c0 + 7 && cgreg_sent == g0 + 3);
  /* A module power-on clears the state (gsm_initvar memsets GSM). */
  modem_alive = 0; poll(4); memset(&GSM, 0, sizeof GSM); c0 = creg_sent; poll(1); CHECK(creg_sent == c0 + 1);
  puts("PASS registration polling: silent module skips +CGREG and backs off to every third poll, any reply restores");
  return 0;
}
