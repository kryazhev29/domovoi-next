// Кадры в очередь журнала USB; память ответа никогда не занимается журналом.
#include "usb_log.h"
#include "packet.h"
#include "hw_config.h"
#include "usb_lib.h"

static uint32_t epoch;
static uint16_t lost;

// Смена USB-сессии обслуживается только главным циклом.
void usb_log_session(void)
{
  if (epoch != usb_bus_resets || !USB_DETECT) {
    epoch = usb_bus_resets;
    device.logBits = 0;
    lost = 0;
  }
}
// Новое включение начинает счёт; смена активной маски сохраняет потери.
void usb_log_enable(uint8_t bits)
{
  usb_log_session();
  if (!device.logBits) lost = 0;
  device.logBits = bits & 7u;
}
// Кадр до 63 байт - короткий пакет USB. Длинный текст идёт кусками по 45 байт,
// бит 7 вида - «продолжение в следующем кадре»; потерянный кусок обрывает строку.
/* Проверено на плате №1 14.10.2026 (0e107dc): запуск модема и работа по 60 с, AT/этапы/сервер/события,
   опрос статуса раз в секунду - потерь 0; категории включаются и выключаются из NextConfig на ходу.
   Изменено 14.10.2026: длинная строка кусками вместо обрезки - на плате не проверено. */
void usb_log(uint8_t kind, const uint8_t *body, uint16_t count)
{
  uint8_t bit = kind < 16u ? 1u : kind < 32u ? 2u : 4u;
  if (!usb_log_on(bit)) return;
  usb_log_session();
  if (!usb_log_on(bit)) return;
  uint8_t frame[63];
  frame[6] = 255;
  frame[7] = 0;
  frame[8] = 109;
  uint32_t now = timers_getMillis();
  memcpy(frame + 10, &now, 4);
  do {
    uint8_t part = count > 45u ? 45u : (uint8_t)count;
    count -= part;
    frame[9] = count ? (uint8_t)(kind | 128u) : kind;
    memcpy(frame + 14, &lost, 2);
    if (part) memcpy(frame + 16, body, part);
    body += part;
    packet_frame(frame, part + 10u);
    if (!usb_get_state() || !USB_DETECT || !USB_Log_Try(frame, part + 18u, epoch)) {
      if (lost != UINT16_MAX) ++lost;
      return; /* Хвост без начала строки не нужен. */
    }
    lost = 0;
  } while (count);
}
// Малые числовые тела: данные, сервер и запланированное восстановление.
void usb_log_value(uint8_t kind, uint8_t code, uint32_t value)
{
  uint8_t body[5];
  body[0] = code;
  memcpy(body + 1, &value, 4);
  usb_log(kind, body, kind == 5u || kind == 4u ? 3u : 5u);
}
