// Доработка: Кряжев А., 2026
// Камеры по RS485: C328/OV528 (съёмка пакетами) и Interceptor (номер и снимок по запросу), поток пакетов 14.
// Флеш обслуживает асинхронный автомат; каждое фото начинается с сектора.
// Заголовок публикуется после данных, частичная докачка живёт только в RAM.
#include "photo.h"
#include "../../Modules/comm_channel.h"
#include "../sector_journal.h"
#include "photo_context.h"
#define PHOTO_BLOCK 512u
#define PHOTO_RESOLUTION 7u // Разрешение C328: 640 на 480 точек.
photo_s *photo;
// Граница уже стёртых секторов и срок ожидания следующей отправки.
uint32_t photo_erased_to, photo_pending_since;
// Фазы C328 идут в порядке команд камеры; P_ERASE и P_IDC — снимок камеры номеров.
enum { P_IDLE, P_SYNC, P_INIT, P_SIZE, P_SNAP, P_GET, P_DATA, P_ERASE, P_IDC };
// Камера номеров (camera.docx, режим loop 0): ответ на запрос номера за 0,7–2,6 с, иногда с паузой
// внутри строки; снимок отдаётся одним потоком ~42 КБ без пауз и только пока номер в кадре.
#define ANPR_POLL_MS 1000u   // не чаще одного запроса номера в секунду
#define ANPR_WAIT_MS 3000u   // ожидание ответа на запрос номера
#define ANPR_PLATE_MS 5000u  // номер считается в кадре после последнего ответа с номером
#define ANPR_FIRST_MS 5000u  // начало потока снимка (камера готовит его ~2,7 с)
#define ANPR_GAP_MS 1000u    // тишина внутри потока снимка — обрыв
// Размер снимка камера сообщает в конце потока: место — 12 секторов, наибольший снимок 49 139 Б.
#define ANPR_RECORD (12u * MEMSECTORSIZE)
#define PHOTO_ANPR_MAX (ANPR_RECORD - PHOTO_RECORD_HEAD)
// Опрос: ждём ответа камеры из строки slot; plate — когда камера plate_addr последний раз видела номер.
static struct { uint32_t sent, plate; uint8_t slot, wait, plate_addr; } anpr;
#include "photo_runtime.inc"

// Выделяем контекст камеры только на время работы.
static uint8_t photo_alloc(void)
{
  if (!photo) photo = work_acquire(sizeof(*photo), WORK_PHOTO_CONTEXT);
  return photo != NULL;
}

// Не включает просто сохранённые снимки: без запроса 21 журнал не ждёт бесконечно.
bool photo_busy(void) { return photo && (photo->stage || photo->pending); }

// Завершаем съёмку и возвращаем линию 485 её хозяину вне съёмки.
static void photo_stop(void)
{
  rs485_init();
  photo->stage = P_IDLE;
  device.getPhoto = 0;
}

// Выбираем общую скорость RS485 по сохранённой настройке.
uint32_t rs485_baud(void)
{
  if (device.wifiModem || comm_channel_wifi()) return 115200;
  switch (config.reserv[0]) {
  case 1: return 57600;
  case 2: return 38400;
  case 3: return 19200;
  case 4: return 9600;
  default: return 115200;
  }
}

// Настраиваем RS485, устраняем недопустимые значения конфигурации и назначаем единственного хозяина
// линии вне съёмки: мост (модем выбран или ещё выключается) берёт буфер сам, камера — на время снимка;
// иначе линию слушает свободная строка моста wf_line: в пассивном режиме приёмник ID,
// в активном — опрос камер номеров. Запрос номера, оставшийся без ответа, снимается.
void rs485_init(void)
{
  if (config.rs485Mode > 1) config.rs485Mode = 0;
  if (config.reserv[0] > 4) config.reserv[0] = 0;
  bool listen = !device.wifiModem && !comm_channel_wifi();
  uart_init(rs485_baud(), 0, listen ? wf_line : NULL, listen ? sizeof(wf_line) : 0u);
  uart_setRecTimeout(10);
  anpr.wait = 0;
}

// Прерываем текущую съёмку при изменении параметров её шины.
void rs485_set_config(const uint8_t *data)
{
  const config_s *next = (const config_s *)data;
  uint8_t changed = config.reserv[0] != next->reserv[0] ||
    config.rs485Mode != next->rs485Mode ||
    memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery)) != 0;
  // Не трогаем курсор и запросы выгрузки уже сохранённых фотографий.
  if (changed) {
    memset(device.peripheryState, 0, sizeof(device.peripheryState)); // ответы опроса — по новой таблице
    if (photo && photo->stage) photo_stop();
  }
  memcpy(&config, data, sizeof(config));
  if ((!photo || !photo->stage) && !comm_channel_wifi()) rs485_init();
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
    if (flash_job.kind == FX_PHOTO && flash_job.result == FX_PENDING) flash_job.quiet = 1;
  }
}

// Команда камере номеров FT+<команда><адрес>CRLF; адрес — байт-символ (serialAddr камеры 48 = «0»).
static void anpr_send(const char *cmd, uint8_t address)
{
  uint8_t p[9] = { 'F', 'T', '+', (uint8_t)cmd[0], (uint8_t)cmd[1], (uint8_t)cmd[2], address, '\r', '\n' };
  uart_send(p, sizeof(p));
}

// Камеры номеров (тип 8) по очереди, один запрос номера на линии. Ответ ++FT<адрес>IDN<номер 10 Б>CRLF
// ждём хвостом приёма, а не тишиной; 10 нулей — номера в кадре нет. Ответ отмечает камеру в статусе 2.
void anpr_poll(void)
{
  uint32_t now = timers_getMillis();
  if (anpr.wait) {
    uint16_t n = uart_received();
    const uint8_t *r = wf_line + (n >= 20u ? n - 20u : 0u);
    uint8_t a = config.rs485Periphery[anpr.slot].address;
    if (n >= 20u && !memcmp(r, "++FT", 4) && r[4] == a && !memcmp(r + 5, "IDN", 3) &&
        r[18] == '\r' && r[19] == '\n') {
      char id[11];
      bool plate = false;
      anpr.wait = 0;
      device.peripheryState[anpr.slot] = 1;
      memcpy(id, r + 8, 10);
      id[10] = 0;
      for (uint8_t i = 0; i < 10u; ++i) plate |= id[i] != 0;
      if (plate) {
        anpr.plate = now; // до проверки доступа: снимок прохода спросит, есть ли номер в кадре
        anpr.plate_addr = a;
        camera_readedID(id, a);
      }
      return;
    }
    if (now - anpr.sent < ANPR_WAIT_MS) return;
    anpr.wait = 0;
    device.peripheryState[anpr.slot] = 0;
  }
  if (now - anpr.sent < ANPR_POLL_MS) return;
  anpr.sent = now;
  uart_clear();
  for (uint8_t k = 1; k <= 8u; ++k) {
    uint8_t i = (uint8_t)((anpr.slot + k) & 7u);
    if (config.rs485Periphery[i].type == RS485_ANPR && config.rs485Periphery[i].address) {
      anpr_send("IDN", config.rs485Periphery[i].address);
      anpr.slot = i;
      anpr.wait = 1;
      return;
    }
  }
}

// Снимок камеры номеров: поток идёт без пауз, а стирание сектора длится до 0,4 с — место под снимок
// стирается до запроса IDC. Поток ++FT<адрес>IDC<номер 10 Б>:<JPEG><CRC32><размер>CRLF пишется
// во флеш порциями до 506 Б; последние 10 Б приёма не пишутся: если CRC32 и размер в них сходятся
// с принятым, это хвост, и снимок публикуется. Запрос номера, ушедший до съёмки, сначала отвечается.
static void photo_anpr(uint32_t now)
{
  uint16_t n = uart_received();
  if (photo->stage == P_ERASE) {
    if (photo_erased_to < photo->address + ANPR_RECORD + MEMSECTORSIZE) // снимок и сектор впереди
      pr_erase(photo_erased_to == photo_storage_end() ? mem[MB_PHOTO].start : photo_erased_to, PR_LAZY_DONE);
    else if (!anpr.wait || now - anpr.sent >= ANPR_WAIT_MS) {
      uart_clear();
      anpr_send("IDC", device.getPhoto);
      photo->stage = P_IDC;
      photo->deadline = now + ANPR_FIRST_MS;
    }
    return;
  }
  if (!photo->packet && n >= 19u) {
    if (memcmp(photo->rx, "++FT", 4) || photo->rx[4] != device.getPhoto ||
        memcmp(photo->rx + 5, "IDC", 3) || photo->rx[18] != ':') {
      photo_stop();
      return;
    }
    uart_rebuffer(photo->rx, sizeof(photo->rx), 19u);
    n -= 19u;
    photo->packet = 1;
    photo->crc = 0xFFFFFFFFu;
    photo->deadline = now + ANPR_GAP_MS;
  }
  if (photo->packet && !photo->used && n > 10u) {
    uint16_t k = n - 10u;
    if (k > PHOTO_DATA) k = PHOTO_DATA;
    if (k > photo->size - photo->written) {
      photo_stop(); // снимок больше места под него
      return;
    }
    memcpy(photo->frame + 4, photo->rx, k);
    photo->crc = crc32_update(photo->crc, photo->frame + 4, k);
    uart_rebuffer(photo->rx, sizeof(photo->rx), k);
    photo->used = k;
    photo->deadline = now + ANPR_GAP_MS;
  }
  if (photo->used) {
    pr_write(photo->address + PHOTO_RECORD_HEAD + photo->written, photo->frame + 4, photo->used, PR_STREAM_DONE);
    return;
  }
  if (photo->written && n == 10u && rt_get32(photo->rx) == ~photo->crc && rt_get32(photo->rx + 4) == photo->written &&
      photo->rx[8] == '\r' && photo->rx[9] == '\n') {
    uint32_t h[2] = { photo->stamp, photo->written };
    photo->size = photo->written;
    pr_write(photo->address, (uint8_t *)h, 8, PR_STREAM_HEAD);
    return;
  }
  if ((int32_t)(now - photo->deadline) >= 0) photo_stop();
}
