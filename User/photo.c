// Доработка: Кряжев А., 2026
// C328/OV528: последовательность команд из старого Modules/jpeg.c.
#include "photo.h"
extern device_s device;
extern config_s config;
extern uint8_t port_buffer[PORTBUFSIZE];
#define PHOTO_PACKET 64u
#define PHOTO_RX PHOTO_PACKET
#define PHOTO_DATA (PHOTO_PACKET - 6u)
#define PHOTO_LIMIT 65535u
#define PHOTO_BLOCK 512u
#define PHOTO_RESOLUTION 7u // C328: 5 = 320x240, 7 = 640x480
/* Только указатель в статической RAM. Буферы существуют в heap во время работы.
 * Размер SETPACKSIZE включает ID, длину и verify; RX оставлен с запасом 6 байт. */
typedef struct {
  uint32_t stamp, address, deadline, started, last;
  uint32_t readAddress;
  uint16_t size, written, offset, packet, used;
  uint8_t stage, retry, syncAck, syncSeen, pending, requestId;
  dataChanel_e channel;
  uint8_t rx[PHOTO_RX], frame[PHOTO_RX];
} photo_s;
static photo_s *photo;
// Служебное состояние хранится в статической RAM и не расходует heap.
static uint32_t photo_erased_to, photo_pending_since;
static uint16_t photo_copy_off;
static uint8_t photo_prep_phase, photo_restore_phase;
static uint8_t photo_clear_confirmed(void);
uint8_t photo_cleanup_active(void);
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA, P_PREP };

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
  if (photo->stage) return CMD_RUNERROR;
  device.countPhoto = user_getPhotoCount();
  if (photo_cleanup_active()) {
    user_debug("Photo: clearing storage\r\n");
    return CMD_RUNERROR;
  }
  if (!photo_recover()) return CMD_RUNERROR;
  if (!device.countPhoto && device.newAddressPhoto != mem[MB_PHOTO].start) {
    if (!photo_clear_confirmed()) return CMD_RUNERROR;
    user_debug("Photo: clearing storage\r\n");
    return CMD_RUNERROR;
  }
  if (device.newAddressPhoto + PHOTO_RECORD_HEAD >= photo_storage_end()) {
    user_debug("Photo: no space for next image\r\n");
    return CMD_RUNERROR;
  }
  photo->address = device.newAddressPhoto;
  photo->stamp = device.datetime ? device.datetime : 1u;
  // Номер уникален среди фото в памяти и растёт по очереди.
  // Если дататайм не больше предыдущего номера, берём наибольший имеющийся + 1.
  for (scan = mem[MB_PHOTO].start; scan < device.newAddressPhoto; scan += PHOTO_RECORD_HEAD + previous.size) {
    mem_read(scan, (uint8_t*)&previous, sizeof(previous));
    if (previous.datetime >= photo->stamp) photo->stamp = previous.datetime + 1u;
    wdt_reload();
  }
  if (photo->stamp == 0xFFFFFFFFu) return CMD_RUNERROR;
  photo->size = photo->written = photo->packet = photo->used = 0;
  photo_erased_to = 0; photo_prep_phase = 0;
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
uint8_t photo_cleanup_active(void)
{
  uint32_t marker, scratch = photo_storage_end();
  if (mem[MB_PHOTO].size < 8192u) return 0;
  mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4);
  return marker == 0u;
}
uint8_t photo_recover(void)
{
  uint32_t h[4], scratch = photo_storage_end();
  if (mem[MB_PHOTO].size < 8192u || photo_cleanup_active()) return 0;
  mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[3] != 0x50484F54u) return 1;
  if (h[0] < mem[MB_PHOTO].start || h[0] >= scratch || (h[0] & 4095u) ||
      !h[1] || h[1] >= MEMSECTORSIZE || (!photo_restore_phase && photo_backup_crc(scratch, (uint16_t)h[1]) != h[2])) return 0;
  return 0; // Копия восстанавливается по шагам из photo_poll.
}
static uint8_t photo_restore_step(void)
{
  uint32_t h[4], scratch = photo_storage_end();
  uint8_t b[64];
  uint16_t step;
  mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h));
  if (h[3] != 0x50484F54u) { photo_restore_phase = 0; return 0; }
  if (h[0] < mem[MB_PHOTO].start || h[0] >= scratch || (h[0] & 4095u) ||
      !h[1] || h[1] >= MEMSECTORSIZE || (!photo_restore_phase && photo_backup_crc(scratch, (uint16_t)h[1]) != h[2])) return 0;
  if (!photo_restore_phase) {
    wdt_reload(); mem_erase_sector(h[0]);
    photo_restore_phase = 1; photo_copy_off = 0;
    return 1;
  }
  if (photo_restore_phase == 1) {
    step = h[1] - photo_copy_off > sizeof(b) ? sizeof(b) : (uint16_t)(h[1] - photo_copy_off);
    mem_read(scratch + photo_copy_off, b, step);
    if (!photo_write(h[0] + photo_copy_off, b, step)) return 0;
    photo_copy_off += step;
    if (photo_copy_off == h[1]) photo_restore_phase = 2;
    return 0;
  }
  wdt_reload(); mem_erase_sector(scratch + MEMSECTORSIZE);
  photo_restore_phase = 0;
  user_getPhotoCount();
  return 1;
}
static uint8_t photo_prepare(uint32_t size)
{
  uint32_t a, end = photo_storage_end();
  uint16_t prefix = photo->address & (MEMSECTORSIZE - 1u);
  uint8_t b[64], dirty = 0;
  uint16_t off, step;
  if (!size || size > PHOTO_LIMIT) {
    user_debug("Photo: invalid image size\r\n");
    return 0;
  }
  if (end < photo->address + PHOTO_RECORD_HEAD ||
      size > end - photo->address - PHOTO_RECORD_HEAD) {
    user_debug("Photo: no space for image, queue retained\r\n");
    return 0;
  }
  if (prefix) {
    // Уже пустой хвост общего сектора можно дописать без стирания.
    for (a = photo->address; a < photo->address - prefix + MEMSECTORSIZE; a += step) {
      step = photo->address - prefix + MEMSECTORSIZE - a > sizeof(b) ? sizeof(b) :
             (uint16_t)(photo->address - prefix + MEMSECTORSIZE - a);
      mem_read(a, b, step);
      for (off = 0; off < step; off++) if (b[off] != 0xFF) dirty = 1;
      wdt_reload();
    }
  }
  photo->size = (uint16_t)size;
  photo_copy_off = 0;
  if (dirty) { photo_prep_phase = 1; photo->stage = P_PREP; }
  else {
    photo_erased_to = prefix ? photo->address - prefix + MEMSECTORSIZE : photo->address;
    photo->stage = P_DATA;
  }
  return 1;
}
static uint8_t photo_prepare_step(void)
{
  uint32_t end = photo_storage_end(), h[4];
  uint16_t prefix = photo->address & (MEMSECTORSIZE - 1u), step;
  uint8_t b[64];
  if (photo_prep_phase == 1) {
    wdt_reload(); mem_erase_sector(end + MEMSECTORSIZE);
    photo_prep_phase = 2; return 1;
  }
  if (photo_prep_phase == 2) {
    wdt_reload(); mem_erase_sector(end);
    photo_prep_phase = 3; return 1;
  }
  if (photo_prep_phase == 3) {
    step = prefix - photo_copy_off > sizeof(b) ? sizeof(b) : (uint16_t)(prefix - photo_copy_off);
    mem_read(photo->address - prefix + photo_copy_off, b, step);
    if (!photo_write(end + photo_copy_off, b, step)) { photo_stop(); return 0; }
    photo_copy_off += step;
    if (photo_copy_off == prefix) photo_prep_phase = 4;
    return 0;
  }
  if (photo_prep_phase == 4) {
    h[0] = photo->address - prefix; h[1] = prefix;
    h[2] = photo_backup_crc(end, prefix); h[3] = 0x50484F54u;
    if (!photo_write(end + MEMSECTORSIZE, (uint8_t*)h, 12) ||
        !photo_write(end + MEMSECTORSIZE + 12u, (uint8_t*)&h[3], 4)) { photo_stop(); return 0; }
    photo_prep_phase = 5;
    return 0;
  }
  if (photo_prep_phase == 5 && photo_recover()) {
    photo_erased_to = photo->address - prefix + MEMSECTORSIZE;
    photo->stage = P_DATA;
    photo->deadline = timers_getMillis();
  }
  return 0;
}
// Перед запросом пакета стираем только сектор, в который попадут его данные.
static uint8_t photo_data_ready(void)
{
  uint32_t end, used = photo->written + PHOTO_DATA;
  if (used > photo->size) used = photo->size;
  end = photo->address + PHOTO_RECORD_HEAD + used;
  if (used == photo->size && photo_storage_end() - end >= sizeof(photoParam_s))
    end += sizeof(photoParam_s); // Заголовок следующей записи должен быть пустым.
  if (end <= photo_erased_to) return 1;
  if (photo_erased_to >= photo_storage_end()) { photo_stop(); return 0; }
  wdt_reload(); mem_erase_sector(photo_erased_to);
  photo_erased_to += MEMSECTORSIZE;
  return 0;
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
    if (!photo_write(photo->address + PHOTO_RECORD_HEAD + photo->written, p + 4, length)) { photo_stop(); return; }
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
        log_addPhotoEvent(photo->stamp); // номер фото = его datetime
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
  mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1);
  if (full == 0) return size;
  for (i = 0; i < PHOTO_ACK_SLOTS; i++) {
    mem_read(address + sizeof(photoParam_s) + 8u * i, (uint8_t*)entry, sizeof(entry));
    if (entry[0] == 0xFFFFu && entry[1] == 0xFFFFu &&
        entry[2] == 0xFFFFu && entry[3] == 0xFFFFu) break;
    if (entry[2] == 0 && entry[3] == 0 &&
        (uint16_t)(entry[0] ^ entry[1]) == 0xFFFFu &&
        entry[0] <= size && entry[0] > done) done = entry[0];
  }
  return done;
}
static uint32_t photo_ack_slot(uint32_t address)
{
  uint16_t i, entry[4];
  for (i = 0; i < PHOTO_ACK_SLOTS; i++) {
    uint32_t at = address + sizeof(photoParam_s) + 8u * i;
    mem_read(at, (uint8_t*)entry, sizeof(entry));
    if (entry[0] == 0xFFFFu && entry[1] == 0xFFFFu &&
        entry[2] == 0xFFFFu && entry[3] == 0xFFFFu) return at;
  }
  return 0;
}
static uint8_t photo_ack_write(uint32_t address, uint16_t size, uint16_t offset)
{
  uint16_t value[2];
  uint32_t at, commit = 0;
  if (offset <= photo_confirmed_offset(address, size)) return 1;
  if (offset == size) {
    uint8_t full = 0;
    return photo_write(address + PHOTO_RECORD_HEAD - 1u, &full, 1);
  }
  at = photo_ack_slot(address);
  if (!at) return 0;
  value[0] = offset; value[1] = (uint16_t)~offset;
  return photo_write(at, (uint8_t*)value, sizeof(value)) &&
         photo_write(at + 4u, (uint8_t*)&commit, sizeof(commit));
}
/* Сектор фиксации стирается последним. После обрыва питания нулевой маркер
 * заставляет завершить очистку; до неё все записи уже подтверждены. */
static uint8_t photo_clear_confirmed(void)
{
  uint32_t marker[3], at = photo_storage_end() + MEMSECTORSIZE + 32u;
  uint32_t first = mem[MB_PHOTO].start, end = photo_storage_end();
  if (photo_cleanup_active()) return 1;
  if (device.newAddressPhoto <= first) return 1;
  if (device.newAddressPhoto > end || end - device.newAddressPhoto < sizeof(photoParam_s))
    marker[1] = end;
  else marker[1] = device.newAddressPhoto + sizeof(photoParam_s);
  marker[1] = (marker[1] + MEMSECTORSIZE - 1u) & ~(MEMSECTORSIZE - 1u);
  if (marker[1] > end || marker[1] <= first) return 0;
  marker[0] = 0; marker[2] = ~marker[1];
  // Сначала граница и дополнение, последним фиксируется признак очистки.
  if (!photo_write(at + 4u, (uint8_t*)&marker[1], 8) ||
      !photo_write(at, (uint8_t*)marker, 4)) return 0;
  device.newAddressPhoto = first;
  device.countPhoto = 0; device.status.readyPhoto = 0; device.readPhotoIndex = 0;
  if (photo) { photo->readAddress = first; photo->offset = 0; }
  return 1;
}
static void photo_cleanup_step(void)
{
  uint32_t marker[3], at = photo_storage_end() + MEMSECTORSIZE + 32u;
  uint32_t first = mem[MB_PHOTO].start;
  uint8_t done[256], zero = 0;
  uint16_t i, sectors;
  mem_read(at, (uint8_t*)marker, sizeof(marker));
  if (marker[0] != 0) return;
  if (marker[1] != ~marker[2] || marker[1] > photo_storage_end() ||
      marker[1] <= first || ((marker[1] - first) & (MEMSECTORSIZE - 1u))) return;
  sectors = (uint16_t)((marker[1] - first) / MEMSECTORSIZE);
  if (sectors > sizeof(done)) return;
  mem_read(at + sizeof(marker), done, sectors);
  for (i = 0; i < sectors; i++) if (done[i] != 0) {
    wdt_reload(); mem_erase_sector(first + (uint32_t)i * MEMSECTORSIZE);
    photo_write(at + sizeof(marker) + i, &zero, 1);
    return;
  }
  // Журнал стирается последним: до этого очередь после сброса пуста.
  wdt_reload(); mem_erase_sector(photo_storage_end() + MEMSECTORSIZE);
  device.newAddressPhoto = first;
  device.countPhoto = 0; device.status.readyPhoto = 0;
}
static uint8_t photo_first_pending(uint32_t *address, uint32_t *offset)
{
  uint32_t a;
  photoParam_s h;
  for (a = mem[MB_PHOTO].start; a < device.newAddressPhoto; a += PHOTO_RECORD_HEAD + h.size) {
    uint16_t confirmed;
    mem_read(a, (uint8_t*)&h, sizeof(h));
    confirmed = photo_confirmed_offset(a, (uint16_t)h.size);
    if (confirmed < h.size) { *address = a; *offset = confirmed; return 1; }
  }
  *address = device.newAddressPhoto; *offset = 0;
  return 0;
}
/* Сначала проверяем весь запрос и свободные ячейки, затем фиксируем префикс.
 * При сбое питания подтверждённым может стать только уже запрошенный префикс. */
static cmdCode_e photo_ack_request(uint32_t number, uint16_t offset)
{
  uint32_t a, target = 0;
  photoParam_s h;
  uint8_t found = 0;
  for (a = mem[MB_PHOTO].start; a < device.newAddressPhoto; a += PHOTO_RECORD_HEAD + h.size) {
    mem_read(a, (uint8_t*)&h, sizeof(h));
    if ((!number && photo_confirmed_offset(a, (uint16_t)h.size) < h.size) ||
        (number && h.datetime == number)) { target = a; found = 1; break; }
  }
  if (!found || offset > h.size) {
    user_debug("Photo: invalid number or offset\r\n");
    return CMD_PARAMERROR;
  }
  for (a = mem[MB_PHOTO].start; a <= target; a += PHOTO_RECORD_HEAD + h.size) {
    uint16_t want, done;
    mem_read(a, (uint8_t*)&h, sizeof(h));
    done = photo_confirmed_offset(a, (uint16_t)h.size);
    want = a == target ? offset : (uint16_t)h.size;
    if (want > done && want != h.size && !photo_ack_slot(a)) {
      user_debug("Photo: confirmation journal full\r\n");
      return CMD_RUNERROR;
    }
  }
  for (a = mem[MB_PHOTO].start; a <= target; a += PHOTO_RECORD_HEAD + h.size) {
    uint16_t want;
    mem_read(a, (uint8_t*)&h, sizeof(h));
    want = a == target ? offset : (uint16_t)h.size;
    if (!photo_ack_write(a, (uint16_t)h.size, want)) return CMD_RUNERROR;
  }
  return CMD_SUCCESS;
}
cmdCode_e photo_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  uint32_t number, address, offset;
  uint16_t requested;
  cmdCode_e result;
  if (size != 0 && size != 6) return CMD_PARAMERROR;
  if (device.status.readyToUpdate || channel == CH_SMS || !photo_alloc() || photo->stage) return CMD_RUNERROR;
  user_getPhotoCount();
  if (!photo_cleanup_active() && !photo_recover()) return CMD_RUNERROR;
  if (size && device.countPhoto) {
    memcpy(&number, args, 4); memcpy(&requested, args + 4, 2);
    result = photo_ack_request(number, requested);
    if (result != CMD_SUCCESS) return result;
    user_getPhotoCount();
    if (!device.countPhoto && !photo_clear_confirmed()) return CMD_RUNERROR;
  }
  photo_first_pending(&address, &offset);
  photo->readAddress = address; photo->offset = offset;
  photo->channel = channel; photo->requestId = id; photo->pending = 1;
  photo_pending_since = timers_getMillis();
  return CMD_SUCCESS;
}
static uint8_t photo_next(uint32_t *address, uint32_t *offset, photoParam_s *head)
{
  uint32_t a, first = *address;
  photoParam_s h;
  for (a = first; a < device.newAddressPhoto; a += PHOTO_RECORD_HEAD + h.size) {
    uint16_t confirmed;
    mem_read(a, (uint8_t*)&h, sizeof(h));
    confirmed = photo_confirmed_offset(a, (uint16_t)h.size);
    if (confirmed == h.size) continue;
    *address = a; *head = h;
    if (a != first || *offset < confirmed) *offset = confirmed;
    return 1;
  }
  return 0;
}
static void photo_send(void)
{
  uint32_t address = photo->readAddress, offset = photo->offset;
  uint16_t count, n = 0, len, crc;
  uint8_t *p, ok;
  photoParam_s h;
  count = photo_next(&address, &offset, &h);
  p = malloc(PHOTO_BLOCK + 22u);
  if (!p) return;
  if (count) n = h.size - offset > PHOTO_BLOCK ? PHOTO_BLOCK : (uint16_t)(h.size - offset);
  len = n ? n + 14u : 4u;
  memcpy(p, "SLDN", 4); p[4] = (uint8_t)len; p[5] = (uint8_t)(len >> 8);
  p[6] = 60; p[7] = photo->requestId; p[8] = 14;
  p[9] = n ? (device.countPhoto > 255u ? 255 : (uint8_t)device.countPhoto) : 0;
  if (n) {
    memcpy(p + 10, (uint8_t*)&h, 4); memcpy(p + 14, ((uint8_t*)&h) + 4, 4);
    p[18] = (uint8_t)offset; p[19] = (uint8_t)(offset >> 8);
    mem_read(address + PHOTO_RECORD_HEAD + offset, p + 20, n);
  }
  crc = crc16_calc(p + 4, len + 2u);
  p[len+6] = (uint8_t)crc; p[len+7] = (uint8_t)(crc >> 8);
  ok = photo->channel == CH_USB ? usb_send(p, len + 8u) : connection_send(device.connectionhandle, p, len + 8u) == CE_OK;
  free(p);
  if (!ok) return;
  if (n) {
    offset += n;
    if (offset == h.size) { address += PHOTO_RECORD_HEAD + h.size; offset = 0; }
    photo->readAddress = address; photo->offset = offset;
    photo_pending_since = timers_getMillis();
  } else photo->pending = 0;
}
void photo_connection_lost(void)
{
  if (photo && photo->channel == CH_SERVER) photo->pending = 0;
}
void photo_poll(void)
{
  uint32_t now;
  uint16_t i, n, want;
  uint8_t erased = 0;
  // После сброса завершаем подтверждённую очередь без команды сервера.
  if (!photo_cleanup_active() && photo_recover() && !device.countPhoto &&
      device.newAddressPhoto > mem[MB_PHOTO].start && (!photo || !photo->stage))
    photo_clear_confirmed();
  // После сброса журнал восстанавливается без ожидания команды камеры.
  if (!photo_cleanup_active() && !photo_recover()) erased = photo_restore_step();
  if (photo) {
    now = timers_getMillis();
    if (device.status.readyToUpdate) {
      if (photo->stage) photo_stop();
      photo->pending = 0;
    } else {
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
      if (photo->stage == P_PREP && photo_recover() && !erased)
        erased = photo_prepare_step();
      now = timers_getMillis();
      if (photo->stage && (uint32_t)(now - photo->started) >= 60000u) photo_stop();
      if (photo->stage && photo->stage != P_PREP &&
          (int32_t)(now - photo->deadline) >= 0 &&
          (photo->stage == P_DATA || (uint32_t)(now - photo->last) >= 50u)) {
        if (photo->stage != P_DATA || (erased || photo_data_ready())) {
          if (photo->retry++ >= (photo->stage == P_SYNC ? 30 : 3)) photo_stop();
          else { photo->used = 0; photo_issue(); }
        } else erased = 1;
      }
      if (photo->pending) {
        if ((uint32_t)(now - photo_pending_since) >= 60000u) {
          photo->pending = 0;
          user_debug("Photo: upload stalled for 60 seconds\r\n");
        } else if (photo->channel == CH_SERVER && connection_state(device.connectionhandle) != CE_CONNECTED)
          photo->pending = 0;
        else if ((photo_cleanup_active() || photo_recover()) &&
                 (photo->channel != CH_SERVER || connection_datasended(device.connectionhandle)))
          photo_send();
      }
    }
    if (!photo->stage && !photo->pending) { free(photo); photo = NULL; }
  }
  if (!erased && photo_cleanup_active()) photo_cleanup_step();
}
