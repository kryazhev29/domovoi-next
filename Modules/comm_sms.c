/* Доработка: Кряжев А., 2026.
   Принимает и отправляет SMS в формате PDU.
   Разбор проверяет внешний поток и преобразует GSM/UCS2 в cp1251.
   Сообщение удаляется с SIM после чтения, приложение принимает его вне AT.
*/
#include "comm_sms.h"
#include "comm_modem.h"
#include "comm_timeouts.h"
#include <string.h>
#define SMS_TEXT 160u
#define SMS_UD 100u /* данные отправки: UCS2 50 символов, GSM 114 */
/* Поля SMS-DELIVER (23.040 §9.2.2.1) по порядку разбора. */
enum { P_SMSC_LEN, P_SMSC, P_FO, P_OA_LEN, P_OA_TYPE, P_OA, P_PID, P_DCS, P_SCTS, P_UDL, P_UDHL, P_UD, P_END, P_BAD };
enum { CODE_GSM, CODE_8BIT, CODE_UCS2 };
enum { SMS_OP_NONE, SMS_OP_SETUP, SMS_OP_LIST, SMS_OP_READ, SMS_OP_DELETE, SMS_OP_SEND };
static const char *const sms_setup[] = { "AT+CMGF=0", "AT+CPMS=\"SM\",\"SM\",\"SM\"", "AT+CNMI=2,1,0,0,0" };
static struct {
  bool (*received)(char *, char *);
  uint32_t now, retry;
  comm_side_t req;
  uint16_t acc, out_count;
  uint8_t session, step, op, ready, scan, deliver, erase, tx;
  uint8_t queue[8], count;
  /* Разбор PDU потоком: hex -> октет -> поле; 7 бит - через накопитель acc. */
  uint8_t field, left, half, hi, nib_half, nib, alpha, coding, udh, skip, bits, septets, nlen, tlen;
  char cmd[24], tx_cmd[16]; /* чтение/стирание и отправка не делят строку */
  char number[21];
  char text[SMS_TEXT + 1u];
  uint8_t out[2u * (19u + SMS_UD) + 1u];
} msg;
/* Добавить декодированный символ в ограниченное поле SMS. */
static void sms_char(char c)
{
  if (msg.alpha) { if (msg.nlen + 1u < sizeof(msg.number)) msg.number[msg.nlen++] = c; }
  else if (msg.tlen < SMS_TEXT) msg.text[msg.tlen++] = c;
}
/* GSM 7 бит (23.038) -> ASCII; расширения и национальные буквы - '?'. */
static void sms_septet(uint8_t s)
{
  if (msg.skip) { --msg.skip; return; }
  if (s == 0x1B) return;
  sms_char(s == 0 ? '@' : s == 2 ? '$' : s == 0x11 ? '_' : s == '\r' || s == '\n' ? (char)s
           : s < 0x20 || s == 0x24 || s == 0x40 || (s >= 0x5B && s <= 0x60) || s >= 0x7B ? '?' : (char)s);
}
/* Разобрать накопленные семибитные символы PDU. */
static void sms_unpack(uint8_t o)
{
  msg.acc |= (uint16_t)o << msg.bits; msg.bits = (uint8_t)(msg.bits + 8u);
  while (msg.bits >= 7u && msg.septets) {
    sms_septet((uint8_t)(msg.acc & 0x7Fu)); msg.acc >>= 7; msg.bits = (uint8_t)(msg.bits - 7u); --msg.septets;
  }
}
/* Принять следующий октет внешней PDU по её полю. */
static void sms_octet(uint8_t o)
{
  switch (msg.field) {
  case P_SMSC_LEN: msg.left = o; msg.field = o ? P_SMSC : P_FO; break;
  case P_SMSC: if (!--msg.left) msg.field = P_FO; break;
  case P_FO: msg.udh = o & 0x40u; msg.field = (o & 3u) ? P_BAD : P_OA_LEN; break; /* только DELIVER */
  case P_OA_LEN: msg.left = o; msg.field = o > 20u ? P_BAD : P_OA_TYPE; break;
  case P_OA_TYPE:
    msg.alpha = (o & 0x70u) == 0x50u;
    if (o == 0x91u && !msg.alpha) msg.number[msg.nlen++] = '+';
    if (msg.alpha) { msg.septets = (uint8_t)(msg.left * 4u / 7u); msg.acc = msg.bits = 0; }
    msg.left = (uint8_t)((msg.left + 1u) / 2u);
    msg.field = msg.left ? P_OA : P_PID; break;
  case P_OA:
    if (msg.alpha) sms_unpack(o);
    else for (uint8_t i = 0; i < 2u; ++i, o >>= 4) {
      uint8_t d = o & 15u;
      if (d < 10u && msg.nlen + 1u < sizeof(msg.number)) msg.number[msg.nlen++] = (char)('0' + d);
    }
    if (!--msg.left) { msg.alpha = 0; msg.field = P_PID; } break;
  case P_PID: msg.field = P_DCS; break;
  case P_DCS: { /* группы 00xx/01xx: биты 2-3; 1110 - UCS2; 1111 - бит 2 */
    uint8_t a = (o & 0x80u) == 0 ? (o >> 2) & 3u : (o & 0xF0u) == 0xE0u ? 2u
              : (o & 0xF0u) == 0xF0u ? (o >> 2) & 1u : 0u;
    msg.coding = a == 2u ? CODE_UCS2 : a == 1u ? CODE_8BIT : CODE_GSM;
    msg.left = 7; msg.field = P_SCTS; break;
  }
  case P_SCTS: if (!--msg.left) msg.field = P_UDL; break;
  case P_UDL:
    msg.left = o; msg.septets = o; msg.acc = msg.bits = msg.skip = msg.half = 0;
    if (msg.coding == CODE_GSM) msg.left = (uint8_t)(((uint16_t)o * 7u + 7u) / 8u);
    msg.field = !msg.left ? P_END : msg.udh ? P_UDHL : P_UD; break;
  case P_UDHL: /* заголовок склейки: октеты пропускаются, в 7 битах - с заполнением */
    if (msg.coding == CODE_GSM) { msg.skip = (uint8_t)(((o + 1u) * 8u + 6u) / 7u); sms_unpack(o); }
    else msg.skip = o;
    msg.field = --msg.left ? P_UD : P_END; break;
  case P_UD:
    if (msg.coding == CODE_GSM) sms_unpack(o);
    else if (msg.coding == CODE_8BIT && msg.skip) --msg.skip;
    else if (msg.coding == CODE_8BIT) sms_char((char)o);
    else if (msg.skip) --msg.skip;
    else if (!msg.half) { msg.hi = o; msg.half = 1; }
    else { sms_char(comm_at_cp1251((uint16_t)(msg.hi << 8 | o))); msg.half = 0; }
    if (!--msg.left) msg.field = P_END; break;
  default: msg.field = P_BAD; break; /* данные после конца */
  }
}
/* Проверить индекс сообщения в ограниченном ответе SIM. */
static uint8_t sms_index(const uint8_t *p, uint16_t n, uint16_t pos)
{
  uint32_t v;
  return comm_at_scan(p, n, &pos, 255u, &v) && (pos == n || p[pos] == ',') ? (uint8_t)v : 0u;
}
/* Добавить новый индекс SIM без дубля или назначить повторный обзор. */
static void sms_queue(uint8_t index)
{
  for (uint8_t i = 0; i < msg.count; ++i) if (msg.queue[i] == index) return;
  if (msg.count < sizeof(msg.queue)) msg.queue[msg.count++] = index;
  else msg.scan = 1; /* не влезло - повторный обзор SIM после разбора очереди */
}
/* Разобрать заголовок или очередной кусок тела SMS. */
static void sms_line(const uint8_t *p, uint16_t n, uint16_t detail)
{
  (void)detail;
  if (msg.op == SMS_OP_LIST) {
    /* +CMGL: <index>,<stat>,[<alpha>],<length>; тело - мимо */
    if (comm_at_prefix(p, n, "+CMGL:")) { uint8_t i = sms_index(p, n, 6); if (i) sms_queue(i); }
    return;
  }
  if (msg.op != SMS_OP_READ) return;
  if (comm_at_prefix(p, n, "+CMGR:")) {
    msg.field = P_SMSC_LEN; msg.nlen = msg.tlen = msg.half = msg.nib_half = msg.alpha = 0; return;
  }
  for (uint16_t i = 0; i < n && msg.field < P_END; ++i) {
    uint8_t v = comm_at_hex(p[i]);
    if (v > 15u) { msg.field = P_BAD; break; }
    if (!msg.nib_half) { msg.nib = v; msg.nib_half = 1; } else { msg.nib_half = 0; sms_octet((uint8_t)(msg.nib << 4 | v)); }
  }
}
/* +CMTI: <mem>,<index> - новая SMS уже на SIM. */
static void sms_urc(const uint8_t *p, uint16_t n)
{
  if (!comm_at_prefix(p, n, "+CMTI:")) return;
  uint16_t pos = n;
  while (pos && p[pos - 1u] != ',') --pos;
  uint8_t i = pos ? sms_index(p, n, pos) : 0;
  if (i) sms_queue(i);
}
/* Назначить приём SMS приложением и начать новый сеанс. */
void comm_sms_init(bool (*received)(char *, char *))
{
  memset(&msg, 0, sizeof(msg));
  msg.received = received; msg.session = (uint8_t)(comm_modem_session() - 1u);
  comm_modem_listen(COMM_URC_SMS, sms_urc);
}
/* Занять служебный слот для операции SMS. */
static bool sms_issue(uint8_t op, const char *cmd, uint8_t mode, uint8_t timeout)
{
  msg.req.cmd = cmd; msg.req.mode = mode; msg.req.timeout = timeout;
  msg.req.line = sms_line; msg.req.payload = op == SMS_OP_SEND ? msg.out : NULL; msg.req.count = msg.out_count;
  if (!comm_modem_side(&msg.req)) return false;
  msg.op = op; return true;
}
/* Собрать команду чтения, удаления или обзора SIM. */
static void sms_command(uint8_t op, const char *at, uint8_t index, uint8_t mode, uint8_t timeout)
{
  uint16_t n = comm_at_copy(msg.cmd, sizeof(msg.cmd), 0, at);
  if (index) n = comm_at_decimal(msg.cmd, sizeof(msg.cmd), n, index);
  (void)n; sms_issue(op, msg.cmd, mode, timeout);
}
/* Применить завершение настройки, чтения, удаления или отправки SMS. */
static void sms_done(uint8_t r)
{
  uint8_t op = msg.op;
  msg.op = SMS_OP_NONE;
  if (op == SMS_OP_SETUP) {
    if (r != COMM_AT_OK) { msg.retry = msg.now + comm_timeouts[COMM_T_SMS_RETRY]; return; }
    if (++msg.step == sizeof(sms_setup) / sizeof(sms_setup[0])) { msg.ready = 1; msg.scan = 1; }
  } else if (op == SMS_OP_READ) {
    /* Прочитанная или битая - стирается, иначе SIM заполнится и новые SMS не придут. */
    msg.erase = msg.queue[0];
    memmove(msg.queue, msg.queue + 1, --msg.count);
    if (r == COMM_AT_OK && msg.field == P_END && msg.nlen) {
      msg.number[msg.nlen] = 0; msg.text[msg.tlen] = 0; msg.deliver = 1;
    }
  } else if (op == SMS_OP_DELETE) msg.erase = 0;
  else if (op == SMS_OP_SEND) msg.tx = 0;
}
/* Продвинуть очередь SIM и передать готовое сообщение приложению. */
void comm_sms_poll(uint32_t now_ms)
{
  msg.now = now_ms;
  uint8_t session = comm_modem_session();
  if (session != msg.session) { /* новый запуск модема: настройка и обзор SIM заново */
    msg.session = session; msg.step = msg.ready = 0; msg.retry = now_ms;
  }
  if (msg.op) {
    uint8_t r = comm_modem_side_result(&msg.req);
    if (r == UINT8_MAX) return;
    sms_done(r);
  }
  if (msg.deliver) {
    if (!msg.received(msg.number, msg.text)) return;
    msg.deliver = 0;
  }
  const slm_status_t *st = comm_modem_status();
  if (st->state != SLM_READY && !(st->state == SLM_STARTING && st->step > SLM_SIM)) return;
  if (!msg.ready) {
    if ((int32_t)(now_ms - msg.retry) >= 0) sms_issue(SMS_OP_SETUP, sms_setup[msg.step], COMM_AT_TEXT, COMM_T_SMS);
  } else if (msg.erase) sms_command(SMS_OP_DELETE, "AT+CMGD=", msg.erase, COMM_AT_TEXT, COMM_T_SMS);
  else if (msg.tx) sms_issue(SMS_OP_SEND, msg.tx_cmd, COMM_AT_PROMPT, COMM_T_SMS_SEND);
  else if (msg.count) sms_command(SMS_OP_READ, "AT+CMGR=", msg.queue[0], COMM_AT_PDU, COMM_T_SMS);
  else if (msg.scan) { msg.scan = 0; sms_command(SMS_OP_LIST, "AT+CMGL=4", 0, COMM_AT_PDU, COMM_T_SMS); }
}
/* Добавить октет в шестнадцатеричное тело исходящей SMS. */
static void sms_put(uint8_t o)
{
  static const char hex[] = "0123456789ABCDEF";
  msg.out[msg.out_count++] = (uint8_t)hex[o >> 4]; msg.out[msg.out_count++] = (uint8_t)hex[o & 15u];
}
/* ASCII -> GSM 7 бит; символы расширения - '?'. */
static uint8_t sms_gsm(uint8_t c)
{
  return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' || c == '\n' ? c
       : c < 0x20 || (c >= 0x5B && c <= 0x5E) || c == 0x60 || c >= 0x7B ? '?' : c;
}
/* Проверить номер и собрать исходящую SMS в свободном слоте. */
bool comm_sms_send(const char *number, const char *text)
{
  if (msg.tx) return false;
  uint8_t type = 0x81, n = 0, len = 0;
  if (*number == '+') { ++number; type = 0x91; }
  while (number[n]) { if (number[n] < '0' || number[n] > '9' || ++n > 20u) return false; }
  if (!n) return false;
  if (n >= 11u) type = 0x91; /* код страны с цифрами без '+' */
  bool gsm = true;
  while (text[len] && len < 255u) { if ((uint8_t)text[len] >= 0x80u) gsm = false; ++len; }
  uint8_t chars = gsm ? (len > SMS_UD * 8u / 7u ? (uint8_t)(SMS_UD * 8u / 7u) : len)
                      : (len > SMS_UD / 2u ? (uint8_t)(SMS_UD / 2u) : len);
  msg.out_count = 0;
  sms_put(0x00); sms_put(0x11); sms_put(0x00); /* SMSC из SIM; SUBMIT, срок жизни; MR */
  sms_put(n); sms_put(type);
  for (uint8_t i = 0; i < n; i = (uint8_t)(i + 2u))
    sms_put((uint8_t)((i + 1u < n ? (number[i + 1u] - '0') << 4 : 0xF0) | (number[i] - '0')));
  sms_put(0x00); sms_put(gsm ? 0x00 : 0x08); sms_put(0xAA); /* PID, DCS, 4 суток */
  sms_put(gsm ? chars : (uint8_t)(chars * 2u));
  uint16_t acc = 0; uint8_t bits = 0;
  for (uint8_t i = 0; i < chars; ++i) {
    uint8_t c = (uint8_t)text[i];
    if (!gsm) {
      uint16_t u = c < 0x80u ? c : c >= 0xC0u ? (uint16_t)(c - 0xC0u + 0x410u) : c == 0xA8u ? 0x401u : c == 0xB8u ? 0x451u : '?';
      sms_put((uint8_t)(u >> 8)); sms_put((uint8_t)u); continue;
    }
    acc |= (uint16_t)sms_gsm(c) << bits; bits = (uint8_t)(bits + 7u);
    if (bits >= 8u) { sms_put((uint8_t)acc); acc >>= 8; bits = (uint8_t)(bits - 8u); }
  }
  if (bits) sms_put((uint8_t)acc);
  uint16_t tpdu = (uint16_t)(msg.out_count / 2u - 1u);
  msg.out[msg.out_count++] = 0x1A; /* Ctrl+Z - конец PDU */
  uint16_t k = comm_at_copy(msg.tx_cmd, sizeof(msg.tx_cmd), 0, "AT+CMGS=");
  comm_at_decimal(msg.tx_cmd, sizeof(msg.tx_cmd), k, tpdu);
  msg.tx = 1;
  return true;
}
