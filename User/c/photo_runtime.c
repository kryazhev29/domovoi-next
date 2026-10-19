// Доработка: Кряжев А., 2026
// Камера C328/OV528: съёмка по RS485 и поток пакетов 14.
// Флеш обслуживает асинхронный автомат; каждое фото начинается с сектора.
// Заголовок публикуется после данных, частичная докачка живёт только в RAM.
#include "photo.h"
#include "../../Modules/comm_channel.h"
extern uint8_t port_buffer[PORTBUFSIZE];
#include "photo_context.h"
#define PHOTO_BLOCK 512u
#define PHOTO_RESOLUTION 7u // Разрешение C328: 640 на 480 точек.
photo_s *photo;
// Граница уже стёртых секторов и срок ожидания следующей отправки.
uint32_t photo_erased_to, photo_pending_since;
// Фазы синхронизации, настройки и запроса идут в порядке команд камеры.
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA };
#include "photo_runtime.inc"

// Выделяем контекст камеры только на время работы.
static uint8_t photo_alloc(void)
{
  if (!photo) photo = work_acquire(sizeof(*photo), WORK_PHOTO_CONTEXT);
  return photo != NULL;
}

// Не включает просто сохранённые снимки: без запроса 21 журнал не ждёт бесконечно.
bool photo_busy(void) { return photo && (photo->stage || photo->pending); }

// Завершаем съёмку и возвращаем обычный приём RS485.
static void photo_stop(void)
{
  uart_init(rs485_baud(), 0, port_buffer, PORTBUFSIZE);
  photo->stage = P_IDLE;
  device.getPhoto = 0;
  comm_channel_wifi_pause(false);
}

// Выбираем общую скорость RS485 по сохранённой настройке.
uint32_t rs485_baud(void)
{
  switch (config.reserv[0]) {
  case 1: return 57600;
  case 2: return 38400;
  case 3: return 19200;
  case 4: return 9600;
  default: return 115200;
  }
}

// Настраиваем RS485 и устраняем недопустимые значения конфигурации.
void rs485_init(void)
{
  if (config.rs485Mode > 1) config.rs485Mode = 0;
  if (config.reserv[0] > 4) config.reserv[0] = 0;
  uart_init(rs485_baud(), 0, port_buffer, PORTBUFSIZE);
  uart_setRecTimeout(10);
}

// Прерываем текущую съёмку при изменении параметров её шины.
void rs485_set_config(const uint8_t *data)
{
  const config_s *next = (const config_s *)data;
  uint8_t changed = config.reserv[0] != next->reserv[0] ||
    config.rs485Mode != next->rs485Mode ||
    memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery)) != 0;
  // Не трогаем курсор и запросы выгрузки уже сохранённых фотографий.
  if (changed && photo && photo->stage) photo_stop();
  memcpy(&config, data, sizeof(config));
  if (!photo || !photo->stage) rs485_init();
}

// Отправляем команду камеры и назначаем срок ожидания ответа.
static void photo_command(uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  uint8_t p[6] = {0xAA, cmd, a, b, c, d};
  uart_send(p, sizeof(p));
  photo->last = timers_getMillis();
  photo->deadline = photo->last + 2000u;
}

// Отправляем команду текущего шага съёмки.
static void photo_issue(void)
{
  static const uint8_t commands[5][5] = {
    {0x0D, 0, 0, 0, 0}, {1, 0, 7, 0, PHOTO_RESOLUTION},
    {6, 8, (uint8_t)PHOTO_PACKET, (uint8_t)(PHOTO_PACKET >> 8), 0}, {5, 0, 0, 0, 0}, {4, 1, 0, 0, 0}
  };
  if (photo->stage == P_DATA)
    photo_command(0x0E, 0, 0, (uint8_t)photo->packet, (uint8_t)(photo->packet >> 8));
  else {
    const uint8_t *cmd = commands[photo->stage - P_SYNC];
    photo_command(cmd[0], cmd[1], cmd[2], cmd[3], cmd[4]);
  }
  if (photo->stage == P_SYNC) photo->deadline = photo->last + 50u;
}

// Вся область доступна для фото: резервных секторов больше нет.
uint32_t photo_storage_end(void)
{
  return mem[MB_PHOTO].start + mem[MB_PHOTO].size;
}

// Разбираем собранный кадр камеры; запись выполняет автомат хранения.
static void photo_frame(uint8_t *p, uint16_t n)
{
  if (photo->stage == P_DATA) {
    pr_frame(p, n);
    return;
  }
  if (p[0] != 0xAA) return;
  if (p[1] == 0x0F) {
    photo_stop();
    return;
  }
  if (photo->stage == P_SYNC) {
    if (p[1] == 0x0E && p[2] == 0x0D) photo->syncAck = 1;
    if (p[1] == 0x0D) photo->syncSeen = 1;
    if (!photo->syncAck || !photo->syncSeen) return;
    photo_command(0x0E, 0x0D, 0, 0, 0);
    photo->stage = P_INIT;
  } else if (photo->stage == P_GET && p[1] == 0x0A && p[2] == 1) {
    uint32_t size = p[3] | ((uint32_t)p[4] << 8) | ((uint32_t)p[5] << 16);
    if (!pr_prepare(size)) {
      photo_stop();
      return;
    }
  } else if (p[1] == 0x0E &&
            ((photo->stage == P_INIT && p[2] == 1) ||
             (photo->stage == P_SIZE && p[2] == 6) ||
             (photo->stage == P_SNAP && p[2] == 5))) photo->stage++;
  else return;
  photo->retry = 0;
  photo->deadline = timers_getMillis();
}

// Останавливаем поток сервера, сохраняя принятый курсор докачки.
void photo_connection_lost(void)
{
  if (photo && photo->channel == CH_SERVER) {
    photo->pending = 0;
    if (flash_job.kind == FX_PHOTO && flash_job.result == FX_PENDING) pj.quiet = 1;
  }
}
