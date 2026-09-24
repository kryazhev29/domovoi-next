// Доработка: Кряжев А., 2026
// C328/OV528: последовательность команд из старого Modules/jpeg.c.
#include "photo.h"
extern device_s device;
extern config_s config;
extern uint8_t port_buffer[PORTBUFSIZE];
#define PHOTO_PACKET 64u
#define PHOTO_RX 70u
#define PHOTO_DATA (PHOTO_PACKET - 6u)
#define PHOTO_LIMIT 65535u
#define PHOTO_BLOCK 512u
#define PHOTO_RESOLUTION 7u // C328: 5 = 320x240, 7 = 640x480
/* Только указатель в статической RAM. Буферы существуют в heap во время работы.
 * Размер SETPACKSIZE включает ID, длину и verify; RX оставлен с запасом 6 байт. */
typedef struct {
  uint32_t stamp, size, written, address, deadline, started, last;
  uint32_t readAddress, offset, ackAddress, ackOffset;
  uint16_t packet, used, index, ackIndex;
  uint8_t stage, retry, syncAck, syncSeen, pending, requestId, clearOnEnd, valid;
  dataChanel_e channel;
  uint8_t rx[PHOTO_RX], frame[PHOTO_RX];
} photo_s;
static photo_s *photo;
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA };

static uint8_t photo_alloc(void)
{
  if (!photo) photo = calloc(1, sizeof(*photo));
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
static void photo_command(uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
  uint8_t p[6] = {0xAA, cmd, a, b, c, d};
  uart_send(PORT1_PORT, p, sizeof(p));
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
  uint8_t i, found = 0;
  uint32_t scan;
  photoParam_s previous;
  for (i = 0; i < 8; i++)
    if (config.rs485Periphery[i].type == 3 && config.rs485Periphery[i].address == address) found = 1;
  if (!found || config.rs485Mode != 1) return CMD_PARAMERROR;
  if (device.status.readyToUpdate || !photo_alloc()) return CMD_RUNERROR;
  if (photo->stage || photo->pending || photo->valid) return CMD_RUNERROR;
  device.countPhoto = user_getPhotoCount();
  if (device.newAddressPhoto + 8u >= photo_storage_end()) return CMD_RUNERROR;
  photo->address = device.newAddressPhoto;
  photo->stamp = device.datetime;
  if (photo->stamp == 0xFFFFFFFFu) return CMD_RUNERROR;
  // Номер фото — datetime: два снимка с одинаковым номером нельзя различить в подтверждении.
  for (scan = mem[MB_PHOTO].start; scan < device.newAddressPhoto; scan += 8u + previous.size) {
    mem_read(scan, (uint8_t*)&previous, sizeof(previous));
    if (previous.datetime == photo->stamp) {
      user_debug("Photo: duplicate datetime, retry capture later\r\n");
      return CMD_RUNERROR;
    }
    wdt_reload();
  }
  photo->size = photo->written = photo->packet = photo->used = 0;
  photo->retry = photo->syncAck = photo->syncSeen = 0;
  photo->stage = P_SYNC;
  photo->started = timers_getMillis();
  photo->last = photo->started - 50u;
  photo->deadline = photo->started;
  uart_init(PORT1_PORT, rs485_baud(), 0, photo->rx, sizeof(photo->rx));
  device.getPhoto = address;
  return CMD_SUCCESS;
}
static uint8_t photo_write(uint32_t address, uint8_t *data, uint16_t n)
{
  uint8_t check[64];
  uint16_t step;
  mem_write(address, data, n);
  while (n) {
    wdt_reload();
    step = n > sizeof(check) ? sizeof(check) : n;
    mem_read(address, check, step);
    if (memcmp(check, data, step)) return 0;
    address += step; data += step; n -= step;
  }
  return 1;
}
/* Два последних сектора MB_PHOTO: копия начала общего сектора и журнал.
 * Это позволяет повторить запись после сброса без потери предыдущего фото. */
uint32_t photo_storage_end(void)
{
  return mem[MB_PHOTO].start + (mem[MB_PHOTO].size >= 8192u ? mem[MB_PHOTO].size - 8192u : 0);
}
static uint32_t photo_backup_crc(uint32_t address, uint16_t n)
{
  uint8_t b[64], bit;
  uint16_t i, step;
  uint32_t crc = 0xFFFFFFFFu;
  while (n) {
    step = n > sizeof(b) ? sizeof(b) : n;
    mem_read(address, b, step);
    for (i = 0; i < step; i++) {
      crc ^= b[i];
      for (bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
    wdt_reload(); address += step; n -= step;
  }
  return crc;
}
uint8_t photo_recover(void)
{
  uint32_t h[4], scratch = photo_storage_end();
  uint16_t off, step;
  uint8_t b[64];
  if (mem[MB_PHOTO].size < 8192u) return 0;
  mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[3] != 0x50484F54u) return 1;
  if (h[0] < mem[MB_PHOTO].start || h[0] >= scratch || (h[0] & 4095u) ||
      !h[1] || h[1] >= MEMSECTORSIZE || photo_backup_crc(scratch, (uint16_t)h[1]) != h[2]) return 0;
  wdt_reload(); mem_erase_sector(h[0]);
  for (off = 0; off < h[1]; off += step) {
    step = h[1] - off > sizeof(b) ? sizeof(b) : (uint16_t)(h[1] - off);
    mem_read(scratch + off, b, step);
    if (!photo_write(h[0] + off, b, step)) return 0;
  }
  wdt_reload(); mem_erase_sector(scratch + MEMSECTORSIZE);
  return 1;
}
static uint8_t photo_prepare(uint32_t size)
{
  uint32_t a, eraseEnd, end = photo_storage_end(), h[4];
  uint16_t prefix = photo->address & (MEMSECTORSIZE - 1u);
  uint8_t b[64], dirty = 0;
  uint16_t off, step;
  if (!size || size > PHOTO_LIMIT || end < photo->address + 8u || size > end - photo->address - 8u) return 0;
  if (prefix) {
    // Обычное дописывание не требует стирать уже пустой хвост сектора.
    for (a = photo->address; a < photo->address - prefix + MEMSECTORSIZE; a += step) {
      step = photo->address - prefix + MEMSECTORSIZE - a > sizeof(b) ? sizeof(b) :
             (uint16_t)(photo->address - prefix + MEMSECTORSIZE - a);
      mem_read(a, b, step);
      for (off = 0; off < step; off++) if (b[off] != 0xFF) dirty = 1;
      wdt_reload();
    }
    if (dirty) {
      wdt_reload(); mem_erase_sector(end + MEMSECTORSIZE);
      wdt_reload(); mem_erase_sector(end);
      for (off = 0; off < prefix; off += step) {
        step = prefix - off > sizeof(b) ? sizeof(b) : (uint16_t)(prefix - off);
        mem_read(photo->address - prefix + off, b, step);
        if (!photo_write(end + off, b, step)) return 0;
      }
      h[0] = photo->address - prefix; h[1] = prefix;
      h[2] = photo_backup_crc(end, prefix); h[3] = 0x50484F54u;
      if (!photo_write(end + MEMSECTORSIZE, (uint8_t*)h, 12) ||
          !photo_write(end + MEMSECTORSIZE + 12u, (uint8_t*)&h[3], 4) || !photo_recover()) return 0;
    }
  }
  eraseEnd = photo->address + 8u + size;
  if (end - eraseEnd >= 8u) eraseEnd += 8u; // пустой заголовок следующего кадра
  a = photo->address - prefix;
  if (prefix) a += MEMSECTORSIZE;
  for (; a < eraseEnd; a += MEMSECTORSIZE) {
    wdt_reload(); mem_erase_sector(a);
  }
  photo->size = size;
  return 1;
}
static void photo_frame(uint8_t *p, uint16_t n)
{
  uint16_t i, length, id;
  uint8_t sum = 0;
  if (photo->stage == P_DATA) {
    id = p[0] | ((uint16_t)p[1] << 8);
    length = p[2] | ((uint16_t)p[3] << 8);
    for (i = 0; i < n - 2u; i++) sum += p[i];
    if (id != photo->packet || !length || length > PHOTO_DATA || n != length + 6u ||
        p[n-2] != sum || p[n-1] != 0 || length > photo->size - photo->written) {
      photo->used = 0; return; // повтор того же пакета по таймауту
    }
    if (!photo_write(photo->address + 8u + photo->written, p + 4, length)) { photo_stop(); return; }
    photo->written += length;
    photo->packet++;
    if (photo->written == photo->size) {
      photoParam_s h;
      h.datetime = photo->stamp; h.size = photo->size;
      // Заголовок фиксируется только после всех данных, datetime последним.
      if (photo_write(photo->address + 4u, ((uint8_t*)&h) + 4, 4) &&
          photo_write(photo->address, (uint8_t*)&h, 4)) {
        device.countPhoto = user_getPhotoCount();
        device.status.readyPhoto = device.countPhoto != 0;
      }
      photo_command(0x0E,0,0,0xF0,0xF0);
      photo_stop(); return;
    }
  } else {
    if (p[0] != 0xAA) return;
    if (p[1] == 0x0F) { photo_stop(); return; }
    if (photo->stage == P_SYNC) {
      if (p[1] == 0x0E && p[2] == 0x0D) photo->syncAck = 1;
      if (p[1] == 0x0D) photo->syncSeen = 1;
      if (!photo->syncAck || !photo->syncSeen) return;
      photo_command(0x0E,0x0D,0,0,0);
      photo->stage = P_INIT;
    } else if (photo->stage == P_GET && p[1] == 0x0A && p[2] == 1) {
      uint32_t size = p[3] | ((uint32_t)p[4] << 8) | ((uint32_t)p[5] << 16);
      if (!photo_prepare(size)) { photo_stop(); return; }
      photo->stage = P_DATA;
    } else if (p[1] == 0x0E &&
              ((photo->stage == P_INIT && p[2] == 1) ||
               (photo->stage == P_SIZE && p[2] == 6) ||
               (photo->stage == P_SNAP && p[2] == 5))) photo->stage++;
    else return;
  }
  photo->retry = 0;
  photo->deadline = timers_getMillis();
}
/* Команда 21 задаёт подтверждённый префикс очереди. Отдельный курсор
 * передачи позволяет повторить его после обрыва или запроса без аргументов. */
static cmdCode_e photo_seek(uint32_t number, uint16_t offset, uint16_t count,
                             uint32_t *address, uint16_t *index, uint8_t *clear)
{
  photoParam_s h;
  uint16_t i;
  uint32_t a = mem[MB_PHOTO].start;
  *clear = 0;
  if (!count) { *address = a; *index = 0; photo->offset = 0; return CMD_SUCCESS; }
  for (i = 0; i < count; i++) {
    mem_read(a, (uint8_t*)&h, sizeof(h));
    if (!number || number == h.datetime) break;
    a += 8u + h.size;
  }
  if (i == count || offset > h.size) {
    user_debug("Photo: invalid number or offset\r\n");
    return CMD_PARAMERROR;
  }
  if (offset == h.size) {
    a += 8u + h.size; i++; offset = 0;
    if (i == count && number == h.datetime) *clear = 1;
  }
  *address = a; *index = i; photo->offset = offset;
  return CMD_SUCCESS;
}
cmdCode_e photo_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  uint32_t address, number;
  uint16_t index, offset;
  uint8_t clear = 0;
  cmdCode_e result;
  if (size != 0 && size != 6) return CMD_PARAMERROR;
  if (device.status.readyToUpdate || channel == CH_SMS || !photo_alloc() || photo->stage) return CMD_RUNERROR;
  if (size) {
    memcpy(&number, args, 4); memcpy(&offset, args + 4, 2);
    result = photo_seek(number, offset, user_getPhotoCount(), &address, &index, &clear);
    if (result != CMD_SUCCESS) return result;
    photo->ackAddress = address; photo->ackOffset = photo->offset; photo->ackIndex = index;
  } else {
    address = photo->ackAddress ? photo->ackAddress : mem[MB_PHOTO].start;
    index = photo->ackIndex; photo->offset = photo->ackOffset;
  }
  photo->readAddress = address; photo->index = index; photo->clearOnEnd = clear;
  photo->channel = channel; photo->requestId = id; photo->pending = 1;
  return CMD_SUCCESS;
}
static void photo_send(void)
{
  uint32_t address = photo->readAddress, offset = photo->offset;
  uint16_t index = photo->index, count, n = 0, len, crc;
  uint8_t *p, ok;
  photoParam_s h;
  if (!address) address = mem[MB_PHOTO].start;
  count = user_getPhotoCount();
  // Один буфер на пакет; отказ malloc не сдвигает курсор передачи.
  p = malloc(PHOTO_BLOCK + 22u);
  if (!p) return;
  if (index < count) {
    mem_read(address, (uint8_t*)&h, sizeof(h));
    n = h.size - offset > PHOTO_BLOCK ? PHOTO_BLOCK : (uint16_t)(h.size - offset);
  }
  len = n ? n + 14u : 4u;
  memcpy(p, "SLDN", 4); p[4] = (uint8_t)len; p[5] = (uint8_t)(len >> 8);
  p[6] = 60; p[7] = photo->requestId; p[8] = 14;
  p[9] = n ? (count - index > 255u ? 255 : (uint8_t)(count - index)) : 0;
  if (n) {
    memcpy(p + 10, (uint8_t*)&h, 4); memcpy(p + 14, ((uint8_t*)&h) + 4, 4);
    p[18] = (uint8_t)offset; p[19] = (uint8_t)(offset >> 8);
    mem_read(address + 8u + offset, p + 20, n);
  }
  crc = crc16_calc(p + 4, len + 2u);
  p[len+6] = (uint8_t)crc; p[len+7] = (uint8_t)(crc >> 8);
  ok = photo->channel == CH_USB ? usb_send(p, len + 8u) : connection_send(device.connectionhandle, p, len + 8u) == CE_OK;
  free(p);
  if (!ok) return;
  photo->valid = count != 0;
  if (n) {
    offset += n;
    if (offset == h.size) { address += 8u + h.size; offset = 0; index++; }
    photo->readAddress = address; photo->offset = offset; photo->index = index;
  } else {
    photo->pending = 0;
    if (photo->clearOnEnd && count) {
      user_clearMemBlock(MB_PHOTO);
      device.countPhoto = 0; device.status.readyPhoto = 0; device.readPhotoIndex = 0;
      device.newAddressPhoto = mem[MB_PHOTO].start;
      photo->readAddress = photo->offset = photo->index = 0;
      photo->ackAddress = photo->ackOffset = photo->ackIndex = 0;
      photo->valid = 0;
    }
  }
}
void photo_connection_lost(void)
{
  if (photo && photo->channel == CH_SERVER) photo->pending = 0;
}
void photo_poll(void)
{
  uint32_t now;
  uint16_t i, n, want;
  if (!photo) return;
  now = timers_getMillis();
  if (device.status.readyToUpdate) {
    if (photo->stage) photo_stop();
    photo->pending = 0;
    return;
  }
  if (photo->stage && uart_getstate(PORT1_PORT) && !uart_isReceiving(PORT1_PORT)) {
    n = uart_received(PORT1_PORT);
    for (i = 0; i < n && photo->stage; i++) {
      if (photo->used >= sizeof(photo->frame)) { photo_stop(); break; }
      photo->frame[photo->used++] = photo->rx[i];
      want = 6;
      if (photo->stage == P_DATA && photo->used >= 4) {
        want = 6u + photo->frame[2] + ((uint16_t)photo->frame[3] << 8);
        if (want > PHOTO_PACKET) { photo_stop(); break; }
      }
      if (photo->used == want) { photo_frame(photo->frame, want); photo->used = 0; }
    }
    uart_clear(PORT1_PORT);
  }
  now = timers_getMillis(); // после записи Flash следующий пакет запрашивается в этом же проходе
  if (photo->stage && (uint32_t)(now - photo->started) >= 60000u) photo_stop();
  if (photo->stage && (int32_t)(now - photo->deadline) >= 0 && (photo->stage == P_DATA || (uint32_t)(now - photo->last) >= 50u)) {
    if (photo->retry++ >= (photo->stage == P_SYNC ? 30 : 3)) photo_stop();
    else { photo->used = 0; photo_issue(); }
  }
  if (photo->pending) {
    if (photo->channel == CH_SERVER && connection_state(device.connectionhandle) != CE_CONNECTED)
      photo->pending = 0;
    else if (photo->channel != CH_SERVER || connection_datasended(device.connectionhandle))
      photo_send();
  }
  if (!photo->stage && !photo->pending && !photo->valid) { free(photo); photo = NULL; }
}
