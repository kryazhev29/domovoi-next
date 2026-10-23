// Доработка: Кряжев А., 2026. Один хозяин линии 485: мост, камера, пассивный приём ID или никто.
// Зависимости моделируются, код проверяемого модуля вставляет стенд (run_host.py work rs485owner).
// Проверки не используют физическую плату и сеть.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* ТИПЫ ПРОТОКОЛА */
enum { P_IDLE, P_SYNC };
static config_s config;
static struct { uint8_t wifiModem, getPhoto; } device;
static struct { uint8_t stage; } camera, *photo;
static uint8_t wf_line[96];
static unsigned inits, timeouts, timeout; static uint32_t baud; static uint8_t *rx; static uint16_t rx_size;
static uint8_t wifi_busy;
static bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize)
{ CHECK(!Stopbits); ++inits; baud = Baudrate; rx = rxbuffer; rx_size = bfsize; return true; }
static void uart_setRecTimeout(unsigned int t) { ++timeouts; timeout = t; }
static bool comm_channel_wifi(void) { return wifi_busy != 0; }
void rs485_init(void);
void rs485_set_config(const uint8_t *data);
static void photo_stop(void);
/* КОД МОДУЛЯ */
static void reset(void)
{
  memset(&config, 0, sizeof(config)); memset(&device, 0, sizeof(device)); camera.stage = P_IDLE; photo = NULL;
  inits = timeouts = timeout = 0; baud = 0; rx = (uint8_t *)1; rx_size = 1; wifi_busy = 0;
}
/* Последний вызов uart_init: скорость и буфер. */
static void expect(uint32_t b, uint8_t *buffer, uint16_t size) { CHECK(baud == b && rx == buffer && rx_size == size); }
int main(void)
{
  static const uint32_t rates[] = {115200, 57600, 38400, 19200, 9600};
  config_s next;
  /* Пассивный режим без моста: буфер — строка моста, скорость по настройке, пауза приёма 10 мс. */
  for (uint8_t code = 0; code < 5; ++code) {
    reset(); config.reserv[0] = code; rs485_init();
    CHECK(inits == 1 && timeouts == 1 && timeout == 10); expect(rates[code], wf_line, sizeof(wf_line));
  }
  /* Недопустимые значения: режим 2 -> пассивный, скорость 7 -> 115200. */
  reset(); config.rs485Mode = 2; config.reserv[0] = 7; rs485_init();
  CHECK(!config.rs485Mode && !config.reserv[0]); expect(115200, wf_line, 96);
  /* Активный режим без опроса: буфера нет, байты отбрасывает драйвер. */
  reset(); config.rs485Mode = 1; config.reserv[0] = 4; rs485_init(); expect(9600, NULL, 0);
  /* Выбран Wi-Fi-модем: порт мосту — 115200 без буфера, и в пассивном режиме. */
  reset(); config.reserv[0] = 4; device.wifiModem = 1; rs485_init(); expect(115200, NULL, 0);
  /* Мост ещё выключается после снятия выбора: порт пока его; после последнего off — слушаем. */
  reset(); config.reserv[0] = 4; wifi_busy = 1; rs485_init(); expect(115200, NULL, 0);
  wifi_busy = 0; rs485_init(); expect(9600, wf_line, 96);
  /* Конец съёмки возвращает линию хозяину и паузу 10 мс (у камеры была 2 мс драйвера). */
  reset(); config.reserv[0] = 3; photo = &camera; camera.stage = P_SYNC; device.getPhoto = 5;
  photo_stop(); CHECK(camera.stage == P_IDLE && !device.getPhoto && inits == 1 && timeout == 10); expect(19200, wf_line, 96);
  reset(); config.rs485Mode = 1; photo = &camera; camera.stage = P_SYNC; device.getPhoto = 5;
  photo_stop(); CHECK(camera.stage == P_IDLE && !device.getPhoto); expect(115200, NULL, 0);
  /* Команда 12: смена скорости прерывает съёмку и переназначает линию; те же поля 485 — ничего. */
  reset(); photo = &camera; camera.stage = P_SYNC; device.getPhoto = 5; memset(&next, 0, sizeof(next)); next.reserv[0] = 4;
  rs485_set_config((const uint8_t *)&next);
  CHECK(camera.stage == P_IDLE && !device.getPhoto && config.reserv[0] == 4 && inits == 2); expect(9600, wf_line, 96);
  reset(); photo = &camera; camera.stage = P_SYNC; device.getPhoto = 5; memset(&next, 0, sizeof(next)); next.pinCode = 5;
  rs485_set_config((const uint8_t *)&next);
  CHECK(camera.stage == P_SYNC && device.getPhoto == 5 && !inits && config.pinCode == 5);
  /* Режим: пассивный -> активный без съёмки — буфер снят; обратно — слушаем. */
  reset(); memset(&next, 0, sizeof(next)); next.rs485Mode = 1; rs485_set_config((const uint8_t *)&next);
  CHECK(inits == 1); expect(115200, NULL, 0);
  memset(&next, 0, sizeof(next)); rs485_set_config((const uint8_t *)&next); CHECK(inits == 2); expect(115200, wf_line, 96);
  /* Мост занят линией: настройки приняты, порт не трогается. */
  reset(); wifi_busy = 1; memset(&next, 0, sizeof(next)); next.reserv[0] = 2; rs485_set_config((const uint8_t *)&next);
  CHECK(!inits && config.reserv[0] == 2);
  puts("rs485owner: passive buffer in bridge line, active/bridge without buffer, photo_stop and command 12 hand the line back PASS");
  return 0;
}
