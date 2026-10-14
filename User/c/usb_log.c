// Кадр в очередь журнала USB; память ответа никогда не занимается журналом.
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
// Кадр до 63 байт - короткий пакет USB; длинный текст явно помечен битом 7.
void usb_log(uint8_t kind, const uint8_t *body, uint16_t count)
{
  uint8_t bit = kind < 16u ? 1u : kind < 32u ? 2u : 4u;
  if (!usb_log_on(bit)) return;
  usb_log_session();
  if (!usb_log_on(bit)) return;
  uint8_t frame[63];
  if (count > 45u) {
    count = 45u;
    kind |= 128u;
  }
  frame[6] = 255;
  frame[7] = 0;
  frame[8] = 109;
  frame[9] = kind;
  uint32_t now = timers_getMillis();
  memcpy(frame + 10, &now, 4);
  memcpy(frame + 14, &lost, 2);
  if (count) memcpy(frame + 16, body, count);
  packet_frame(frame, count + 10u);
  if (usb_get_state() && USB_DETECT && USB_Log_Try(frame, count + 18u, epoch)) lost = 0;
  else if (lost != UINT16_MAX) ++lost;
}
// Малые числовые тела: данные, сервер и запланированное восстановление.
void usb_log_value(uint8_t kind, uint8_t code, uint32_t value)
{
  uint8_t body[5];
  body[0] = code;
  memcpy(body + 1, &value, 4);
  usb_log(kind, body, kind == 5u || kind == 4u ? 3u : 5u);
}
