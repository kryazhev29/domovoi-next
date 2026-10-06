// Доработка: Кряжев А., 2026
// C328/OV528: последовательность команд из старого Modules/jpeg.c.
#include "photo.h"
#include "fast_kernels.h"
#include "sector_journal.h"
#include "../../Modules/comm_channel.h"
extern device_s device;
extern config_s config;
extern uint8_t port_buffer[PORTBUFSIZE];
#define PHOTO_PACKET 64u
#define PHOTO_RX PHOTO_PACKET
#define PHOTO_DATA (PHOTO_PACKET - 6u)
#define PHOTO_LIMIT 65535u
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
static uint8_t photo_restore_phase;
uint8_t photo_cleanup_active(void);
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA, P_PREP };
#include "photo_runtime.inc"


static uint8_t photo_alloc(void)
{
  if (!photo) photo = work_acquire(sizeof(*photo), WORK_PHOTO_CONTEXT);
  return photo != NULL;
}
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
void rs485_init(void)
{
  uint8_t i, camera = 0, other = 0;
  if (config.rs485Mode > 1) config.rs485Mode = 0;
  if (config.reserv[0] > 4) {
    user_debug("RS485: invalid baud code, using 115200\r\n");
    config.reserv[0] = 0;
  }
  for (i = 0; i < 8; i++) {
    if (config.rs485Periphery[i].type == 3) camera = 1;
    else if (config.rs485Periphery[i].type) other = 1;
  }
  if (camera && other) user_debug("RS485: C328 exclusive, other devices ignored\r\n");
  uart_init(PORT1_PORT, rs485_baud(), 0, port_buffer, PORTBUFSIZE);
  uart_setRecTimeout(PORT1_PORT, 10);
}
void rs485_set_config(const uint8_t *data)
{
  const config_s *next = (const config_s *)data;
  uint8_t changed = config.reserv[0] != next->reserv[0] ||
    config.rs485Mode != next->rs485Mode ||
    memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery)) != 0;
  // Не трогаем курсор и запросы выгрузки уже сохранённых фотографий.
  if (changed && photo && photo->stage) {
    user_debug("Photo: capture error, RS485 settings changed\r\n");
    photo_stop();
  }
  memcpy(&config, data, sizeof(config));
  if (!photo || !photo->stage) rs485_init();
}
/* Съёмка не удалась: в отладку (бит 4) шаг, попытки и что пришло с 485 - по этому видно,
 * есть ли связь с камерой (28.09.2026). */
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
static void photo_command(uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  uint8_t p[6] = {0xAA, cmd, a, b, c, d};
  uart_send(PHOTO_PORT, p, sizeof(p));
  photo->last = timers_getMillis();
  photo->deadline = photo->last + 2000u;
}
static void photo_issue(void)
{
  switch (photo->stage) {
  case P_SYNC: photo_command(0x0D,0,0,0,0); break;
  case P_INIT: photo_command(1,0,7,0,PHOTO_RESOLUTION); break; // JPEG
  case P_SIZE: photo_command(6,8,PHOTO_PACKET,0,0); break;
  case P_SNAP: photo_command(5,0,0,0,0); break;
  case P_GET: photo_command(4,1,0,0,0); break;
  case P_DATA: photo_command(0x0E,0,0,(uint8_t)photo->packet,(uint8_t)(photo->packet >> 8)); break;
  }
  if (photo->stage == P_SYNC) photo->deadline = photo->last + 50u;
}
cmdCode_e photo_start(uint8_t address)
{
  return pr_start(address);
}
/* Два последних сектора MB_PHOTO: копия начала общего сектора и журнал.
 * Это позволяет повторить запись после сброса без потери предыдущего фото. */
uint32_t photo_storage_end(void)
{
  return mem[MB_PHOTO].start + (mem[MB_PHOTO].size >= 8192u ? mem[MB_PHOTO].size - 8192u : 0);
}
static uint8_t photo_backup_crc(uint32_t address, uint16_t n, uint32_t *result)
{
  uint8_t b[64];
  uint16_t step;
  uint32_t crc = 0xFFFFFFFFu;
  while (n) {
    step = n > sizeof(b) ? sizeof(b) : n;
    if (!mem_read(address, b, step)) return 0;
    crc = crc32_update(crc, b, step);
    wdt_reload(); address += step; n -= step;
  }
  *result = crc;
  return 1;
}
uint8_t photo_cleanup_active(void)
{
  uint32_t marker, scratch = photo_storage_end();
  if (mem[MB_PHOTO].size < 8192u) return 0;
  if (!mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4)) return 2;
  return marker == 0u;
}
uint8_t photo_recover(void)
{
  uint32_t h[4], crc, scratch = photo_storage_end();
  if (mem[MB_PHOTO].size < 8192u || photo_cleanup_active()) return 0;
  if (!mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))) return 0;
  if (h[3] != 0x50484F54u) return 1;
  if (h[0] < mem[MB_PHOTO].start || h[0] >= scratch || (h[0] & 4095u) ||
      !h[1] || h[1] >= MEMSECTORSIZE || (!photo_restore_phase && (!photo_backup_crc(scratch, (uint16_t)h[1], &crc) || crc != h[2]))) return 0;
  return 0; // Копия восстанавливается по шагам из photo_poll.
}
static uint8_t photo_prepare(uint32_t size)
{
  return pr_prepare(size);
}
// Перед запросом пакета стираем только сектор, в который попадут его данные.
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
      user_debug("Photo: camera sync OK\r\n");
    } else if (photo->stage == P_GET && p[1] == 0x0A && p[2] == 1) {
      uint32_t size = p[3] | ((uint32_t)p[4] << 8) | ((uint32_t)p[5] << 16);
      if (!photo_prepare(size)) { photo_stop(); return; }
    } else if (p[1] == 0x0E &&
              ((photo->stage == P_INIT && p[2] == 1) ||
               (photo->stage == P_SIZE && p[2] == 6) ||
               (photo->stage == P_SNAP && p[2] == 5))) photo->stage++;
    else return;
  }
  photo->retry = 0;
  photo->deadline = timers_getMillis();
}
/* В каждой записи 128 ячеек по 8 байт и отдельный байт полного подтверждения.
 * Ячейка содержит смещение, дополнение и фиксацию.
 * Недописанная ячейка не подтверждает данные. Повтор не расходует ячейку. */
uint16_t photo_confirmed_offset(uint32_t address, uint16_t size)
{
  uint16_t i, done = 0, entry[4];
  uint8_t full;
  if (!mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1)) return 0;
  if (full == 0) return size;
  for (i = 0; i < PHOTO_ACK_SLOTS; i++) {
    if (!mem_read(address + sizeof(photoParam_s) + 8u * i, (uint8_t*)entry, sizeof(entry))) return 0;
    if (entry[0] == 0xFFFFu && entry[1] == 0xFFFFu &&
        entry[2] == 0xFFFFu && entry[3] == 0xFFFFu) break;
    if (entry[2] == 0 && entry[3] == 0 &&
        (uint16_t)(entry[0] ^ entry[1]) == 0xFFFFu &&
        entry[0] <= size && entry[0] > done) done = entry[0];
  }
  return done;
}
/* Сектор фиксации стирается последним. После обрыва питания нулевой маркер
 * заставляет завершить очистку; до неё все записи уже подтверждены. */
/* Сначала проверяем весь запрос и свободные ячейки, затем фиксируем префикс.
 * При сбое питания подтверждённым может стать только уже запрошенный префикс. */
cmdCode_e photo_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  return pr_request(channel,id,args,size);
}
void photo_connection_lost(void)
{
  if (photo && photo->channel == CH_SERVER) photo->pending = 0;
  if (photo && photo->channel==CH_SERVER) pr.quiet=1;
}
void photo_poll(void)
{
  pr_poll();
}

