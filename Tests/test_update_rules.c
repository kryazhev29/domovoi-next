/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
#define _1B(a) (Int8U)(a)
#define _2B(a) (Int8U)(a>>8)
typedef enum {CH_USB=0, CH_SERVER, CH_SMS} dataChanel_e;
typedef struct {
  Int16U stateOut2:1, stateOut1:1, stateIn2:1, stateIn1:1, openAnyUser:1, autoRegistration:1,
         unblockOnScene:1, unblockHand:1, res1:1, res2:1, res3:1, readyToUpdate:1,
         readyLogData:1, readyPhoto:1, readyID:1, defaultConfig:1;
} status_s;
static struct {
  status_s status; uint8_t answerID, answerTimeout, FWinProgress, firmwarePercent, cchFWupdate;
  uint16_t updateTimeout, FWblockIdx, FWblockCount, freeBufferSize; uint32_t datetime; int8_t connectionhandle;
} device;
static uint8_t uniqIDCnt;
static unsigned sent_server, sent_usb;
static uint32_t now_ms;
static uint32_t timers_getMillis(void) { return now_ms; }
static uint16_t crc16_calc(uint8_t *p, uint16_t n) { (void)p; (void)n; return 0; }
static int8_t connection_send(int8_t h, uint8_t *p, uint16_t n) { (void)h; (void)p; (void)n; sent_server++; return 0; }
static void comm_server_status_sent(uint8_t id) { (void)id; }
static void usb_send(uint8_t *p, uint16_t n) { (void)p; (void)n; sent_usb++; }
/* КОД МОДУЛЯ */
static uint16_t bits(void) { uint16_t s; memcpy(&s, &device.status, 2); return s; }
int main(void)
{
  unsigned i;

  /* Статус 1 по таблице D433–D448: бит 0 — выход 2, бит 11 — готовность обновления. */
  device.status.stateOut2 = 1; CHECK(bits() == 0x0001); device.status.stateOut2 = 0;
  device.status.readyToUpdate = 1; CHECK(bits() == 0x0800); device.status.readyToUpdate = 0;

  /* Внеочередные изменения входов, выходов, триггеров и бита 11; биты 8–10 и 12–15 ждут периода. */
  now_ms = 5000; status_event_sent(0, 1000);
  CHECK(!status_event_due(0, now_ms));
  for (i = 0; i < 16; i++)
    CHECK((unsigned)status_event_due((uint16_t)(1u << i), now_ms) == ((0x08FFu >> i) & 1u));
  /* Между внеочередными статусами не меньше секунды от последнего отправленного статуса. */
  status_event_sent(0x0001, 5000);
  CHECK(!status_event_due(0x0001, 5999) && !status_event_due(0x0003, 5999));
  CHECK(status_event_due(0x0003, 6000) && !status_event_due(0x1001, 6000));
  now_ms = 0xFFFFFF00u; status_event_sent(0, now_ms); CHECK(status_event_due(1, 0x000002E8u)); /* Переполнение счётчика миллисекунд. */

  /* Каждый серверный статус запоминает переданное состояние; USB этого не делает. */
  device.status.stateIn1 = 1; now_ms = 20000;
  user_send_statusPack(CH_USB); CHECK(sent_usb == 1 && status_event_due(bits(), now_ms) && device.answerTimeout == 0);
  user_send_statusPack(CH_SERVER); CHECK(sent_server == 1 && !status_event_due(bits(), now_ms + 5000));
  /* Статус без текущего ожидания запускает срок ответа 15 с. */
  CHECK(device.answerID == 2 && device.answerTimeout == 15);
  /* Внеочередной статус не перезапускает действующий срок. */
  device.answerTimeout = 7; user_send_statusPack(CH_SERVER); CHECK(device.answerTimeout == 7);
  /* Окончившееся по ответу ожидание регистрации заменяется. */
  device.answerID = 1; device.answerTimeout = 3; user_send_statusPack(CH_SERVER);
  CHECK(device.answerID == 2 && device.answerTimeout == 15);
  device.status.stateIn1 = 0;

  /* Бит 11 тоже внеочередной: сервер сразу узнаёт о начале загрузки (E95, пункт 5). */
  now_ms = 40000; status_event_sent(bits(), 30000);
  device.status.readyToUpdate = 1; CHECK(status_event_due(bits(), now_ms)); device.status.readyToUpdate = 0;

  puts("PASS update rules: status out of turn (mask, 1 s gap, wait not restarted, bit 11)");
  return 0;
}
