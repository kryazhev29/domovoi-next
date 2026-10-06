// Доработка: Кряжев А., 2026
// Камера C328/OV528: съёмка по RS485 и поток пакетов 14.
// Флеш обслуживает асинхронный автомат; каждое фото начинается с сектора.
// Заголовок публикуется после данных, частичная докачка живёт только в RAM.
#include "photo.h"
#include "sector_journal.h"
#include "../../Modules/comm_channel.h"
extern device_s device;
extern config_s config;
extern uint8_t port_buffer[PORTBUFSIZE];
#define PHOTO_PACKET 64u
#define PHOTO_RX PHOTO_PACKET
#define PHOTO_DATA (PHOTO_PACKET - 6u)
#define PHOTO_BLOCK 512u
#define PHOTO_RESOLUTION 7u // C328: 5 = 320x240, 7 = 640x480
#define PHOTO_PORT PORT1_PORT
#define PHOTO_BAUD rs485_baud()
/* Только указатель в статической RAM. Буферы существуют в heap во время работы.
 * Размер SETPACKSIZE включает ID, длину и verify; RX оставлен с запасом 6 байт. */
typedef struct {
  uint32_t stamp, address, deadline, started, last;
  uint32_t readAddress;
  uint16_t size, written, offset, packet, used;
  uint8_t stage, retry, syncAck, syncSeen, pending, requestId;
  uint16_t rxBytes; // диагностика связи с камерой: байт с 485 за съёмку
  uint8_t rxHead[8]; // и первые из них
  dataChanel_e channel;
  uint8_t rx[PHOTO_RX], frame[PHOTO_RX];
} photo_s;
photo_s *photo;
// Служебное состояние хранится в статической RAM и не расходует heap.
uint32_t photo_erased_to, photo_pending_since;
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA, P_PREP };
#include "photo_runtime.inc"


// Выделяем контекст камеры только на время работы.
static uint8_t photo_alloc(void)
{
  if (!photo) photo = work_acquire(sizeof(*photo), WORK_PHOTO_CONTEXT);
  return photo != NULL;
}
// Завершаем съёмку и возвращаем обычный приём RS485.
static void photo_stop(void)
{
  uart_init(PORT1_PORT, rs485_baud(), 0, port_buffer, PORTBUFSIZE);
  photo->stage = P_IDLE;
  photo->used = 0;
  device.getPhoto = 0;
}
// Доработка: Кряжев А., 2026. Скорость общая для всей шины.
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
#if DOMOVOI_DIAG
  uint8_t i, camera = 0, other = 0;
#endif
  if (config.rs485Mode > 1) config.rs485Mode = 0;
  if (config.reserv[0] > 4) {
#if DOMOVOI_DIAG
    user_debug("RS485: invalid baud code, using 115200\r\n");
#endif
    config.reserv[0] = 0;
  }
#if DOMOVOI_DIAG
  for (i = 0; i < 8; i++) {
    if (config.rs485Periphery[i].type == 3) camera = 1;
    else if (config.rs485Periphery[i].type) other = 1;
  }
  if (camera && other) user_debug("RS485: C328 exclusive, other devices ignored\r\n");
#endif
  uart_init(PORT1_PORT, rs485_baud(), 0, port_buffer, PORTBUFSIZE);
  uart_setRecTimeout(PORT1_PORT, 10);
}
// Прерываем текущую съёмку при изменении параметров её шины.
void rs485_set_config(const uint8_t *data)
{
  const config_s *next = (const config_s *)data;
  uint8_t changed = config.reserv[0] != next->reserv[0] ||
    config.rs485Mode != next->rs485Mode ||
    memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery)) != 0;
  // Не трогаем курсор и запросы выгрузки уже сохранённых фотографий.
  if (changed && photo && photo->stage) {
#if DOMOVOI_DIAG
    user_debug("Photo: capture error, RS485 settings changed\r\n");
#endif
    photo_stop();
  }
  memcpy(&config, data, sizeof(config));
  if (!photo || !photo->stage) rs485_init();
}
/* Съёмка не удалась: в отладку (бит 4) шаг, попытки и что пришло с 485 - по этому видно,
 * есть ли связь с камерой (28.09.2026). */
#if DOMOVOI_DIAG
// Показываем причину ошибки камеры только в диагностической сборке.
static void photo_fail(const char *why)
{
  static const char hex[] = "0123456789ABCDEF";
  char text[96] = "Photo: ";
  uint8_t i, n = 7, stage = photo->stage, retry = photo->retry;
  uint16_t bytes = photo->rxBytes;
  while (*why && n < 40) text[n++] = *why++;
  memcpy(&text[n], ", stage ", 8); n += 8; text[n++] = (char)('0' + stage);
  memcpy(&text[n], ", retry ", 8); n += 8;
  text[n++] = (char)('0' + retry / 10 % 10); text[n++] = (char)('0' + retry % 10);
  memcpy(&text[n], ", RX ", 5); n += 5;
  text[n++] = (char)('0' + bytes / 10000 % 10); text[n++] = (char)('0' + bytes / 1000 % 10);
  text[n++] = (char)('0' + bytes / 100 % 10); text[n++] = (char)('0' + bytes / 10 % 10);
  text[n++] = (char)('0' + bytes % 10);
  for (i = 0; i < sizeof(photo->rxHead) && i < bytes; i++) {
    text[n++] = ' '; text[n++] = hex[photo->rxHead[i] >> 4]; text[n++] = hex[photo->rxHead[i] & 15];
  }
  memcpy(&text[n], "\r\n", 3);
  user_debug(text);
  photo_stop();
}
#endif
// Отправляем команду камеры и назначаем срок ожидания ответа.
static void photo_command(uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  uint8_t p[6] = {0xAA, cmd, a, b, c, d};
  uart_send(PHOTO_PORT, p, sizeof(p));
  photo->last = timers_getMillis();
  photo->deadline = photo->last + 2000u;
}
// Отправляем команду текущего шага съёмки.
static void photo_issue(void)
{
  static const uint8_t commands[5][5] = {
    {0x0D,0,0,0,0}, {1,0,7,0,PHOTO_RESOLUTION},
    {6,8,PHOTO_PACKET,0,0}, {5,0,0,0,0}, {4,1,0,0,0}
  };
  if (photo->stage==P_DATA) photo_command(0x0E,0,0,(uint8_t)photo->packet,(uint8_t)(photo->packet>>8));
  else {
    const uint8_t *cmd=commands[photo->stage-P_SYNC];
    photo_command(cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
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
    pr_frame(p,n); return;
  } else {
    if (p[0] != 0xAA) return;
    if (p[1] == 0x0F) { photo_stop(); return; }
    if (photo->stage == P_SYNC) {
      if (p[1] == 0x0E && p[2] == 0x0D) photo->syncAck = 1;
      if (p[1] == 0x0D) photo->syncSeen = 1;
      if (!photo->syncAck || !photo->syncSeen) return;
      photo_command(0x0E,0x0D,0,0,0);
      photo->stage = P_INIT;
#if DOMOVOI_DIAG
      user_debug("Photo: camera sync OK\r\n");
#endif
    } else if (photo->stage == P_GET && p[1] == 0x0A && p[2] == 1) {
      uint32_t size = p[3] | ((uint32_t)p[4] << 8) | ((uint32_t)p[5] << 16);
      if (!pr_prepare(size)) { photo_stop(); return; }
    } else if (p[1] == 0x0E &&
              ((photo->stage == P_INIT && p[2] == 1) ||
               (photo->stage == P_SIZE && p[2] == 6) ||
               (photo->stage == P_SNAP && p[2] == 5))) photo->stage++;
    else return;
  }
  photo->retry = 0;
  photo->deadline = timers_getMillis();
}
// Останавливаем поток сервера, сохраняя принятый курсор докачки.
void photo_connection_lost(void)
{
  if (photo && photo->channel==CH_SERVER) { photo->pending=0; pr.quiet=1; }
}
