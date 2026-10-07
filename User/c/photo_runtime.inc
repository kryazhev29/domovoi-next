/* Доработка: Кряжев А., 2026. */
/* Автомат фото: один шаг чтения или операции W25 за проход.
 * Каждый снимок владеет целыми секторами; стирание выполняется перед записью.
 * Метка формата публикует фото, один байт ACK сохраняет полное подтверждение.
 * После итогового ACK стирается первый сектор, остальные — лениво. */
#include "photo_runtime.h"
#include "runtime_internal.h"
#include "usb_istr.h"
photo_runtime_s pr;
// Задача определяет ответ команды 21; фаза — следующий шаг флеш или транспорта.
enum { PT_STORAGE, PT_REQUEST, PT_ACK, PT_SEND };
enum {
  PR_IO = 1, PR_HEAD, PR_CAMERA, PR_LAZY_DONE,
  PR_DATA_DONE, PR_PHOTO_MARK, PR_PHOTO_DONE, PR_CLEAN_END,
  PR_ACK_HEAD, PR_ACK_ADVANCE, PR_REQUEST_DONE, PR_SEND_FILL,
  PR_SEND_COMMIT
};
static void photo_stop(void);
static uint8_t photo_alloc(void);
static void photo_command(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void photo_issue(void);
static void photo_frame(uint8_t *, uint16_t);

// Переходим к следующему сектору: чужих данных в секторе записи нет.
static uint32_t pr_next(uint32_t address, uint32_t size)
{
  return (address + PHOTO_RECORD_HEAD + size + (MEMSECTORSIZE - 1u))&~(MEMSECTORSIZE - 1u);
}

// Разрешаем повтор команды 21 только при очистке уже восстановленной пустой очереди.
bool photo_empty_cleanup(void) {
  return pr.cleanup && pr.initialized;
}


// Отменяем запрос, но завершаем уже начатую публикацию или очистку.
void photo_storage_cancel(void)
{
  pr.cancelled = pr.quiet = 1;
  if (pr.task == PT_ACK) pr.initialized = 0;
  if (photo) {
    photo->pending = 0;
    if (photo->stage) photo_stop();
  }
}

// Освобождаем буфер передачи и владение флеш после операции.
static void pr_release(void)
{
  work_release(pr.tx, WORK_PHOTO_PACKET);
  pr.tx = 0;
  pr.phase = pr.committed = 0;
  flash_release(FO_PHOTO);
}

// Закрываем операцию с ошибкой без повторных обращений к носителю.
static void pr_fail(void)
{
  pr.fault = 1;
  /* При ошибке носителя новые операции разрешит только перезапуск. */
  if (photo) {
    if (photo->stage) photo_stop();
    photo->pending = 0;
    if ((pr.task == PT_REQUEST || pr.task == PT_ACK) && !pr.quiet)
      user_send_answer_comand(21, CMD_RUNERROR, photo->channel, photo->requestId);
  }
  pr.initialized = 0;
  pr_release();
}

// Копируем данные в буфер владельца и запускаем запись по страницам.
static void pr_write(uint32_t address, const uint8_t *p, uint16_t n, uint8_t after)
{
  if (p != sjr.data) memcpy(sjr.data, p, n);
  pr.io_address = address;
  pr.io_len = n; // Нулевая длина означает стирание сектора.
  pr.io_done = 0;
  pr.after = after;
  pr.phase = PR_IO;
  flash_claim(FO_PHOTO);
}

// Стираем один сектор тем же асинхронным исполнителем W25.
static void pr_erase(uint32_t address, uint8_t after)
{
  pr_write(address, sjr.data, 0, after);
}

// Начинаем проход очереди с первого заголовка.
static void pr_scan_begin(uint8_t task)
{
  pr.task = task;
  pr.scan = mem[MB_PHOTO].start;
  pr.count = 0;
  pr.max_number = 0;
  pr.first = 0;
  pr.phase = PR_HEAD;
  flash_claim(FO_PHOTO);
}

// Публикуем восстановленную очередь и результат поиска запроса.
static void pr_scan_finish(void)
{
  if (pr.task == PT_SEND) {
    pr.send_size = 0;
    pr.phase = PR_SEND_FILL;
    return;
  }
  device.newAddressPhoto = pr.scan;
  device.countPhoto = pr.count;
  device.status.readyPhoto = pr.count != 0;
  if (pr.max_number > pr.last_number) pr.last_number = pr.max_number;
  pr.initialized = 1;
  if (pr.task == PT_REQUEST && pr.requested_number != 0xffffffffu && pr.scan != mem[MB_PHOTO].start) {
    if (!pr.quiet) user_send_answer_comand(21, CMD_PARAMERROR, photo->channel, photo->requestId);
    pr_release();
    return;
  }
  if (pr.task == PT_REQUEST || pr.task == PT_ACK) {
    pr.phase = PR_REQUEST_DONE;
    return;
  }
  pr_release();
}

// Ограничиваем ожидание транспорта, не задерживая основной цикл.
static uint8_t pr_reply_expired(uint32_t now)
{
  if (!pr.reply_wait) {
    pr.reply_wait = 1;
    pr.reply_until = now + 2000;
  }
  return (int32_t)(now - pr.reply_until) >= 0;
}

// Разрешаем новую внешнюю команду после восстановления и освобождения флеш.
static uint8_t pr_command_ready(void)
{
  return user_storage_command_ready() && pr.initialized && !pr.fault &&
    !device.status.readyToUpdate && photo_alloc() && !photo->stage;
}

// Проверяем внешнюю команду 7 и начинаем съёмку.
cmdCode_e photo_start(uint8_t address)
{
  uint8_t found = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (config.rs485Periphery[i].type == 3 && config.rs485Periphery[i].address == address) {
      found = 1;
      break;
    }
  }
  if (!found || config.rs485Mode != 1) return CMD_PARAMERROR;
  if (!pr_command_ready()) return CMD_RUNERROR;
  // Курсор уже проверен сканом и выровнен по сектору: остаток не меньше 4096 Б.
  if ((!device.countPhoto && device.newAddressPhoto != mem[MB_PHOTO].start) ||
      device.newAddressPhoto >= photo_storage_end()) return CMD_RUNERROR;
  if (pr.last_number == 0xfffffffeu || device.datetime == 0xffffffffu) return CMD_RUNERROR;
  pr.address = address;
  pr.cancelled = pr.quiet = 0;
  pr.task = PT_STORAGE;
  pr.phase = PR_CAMERA;
  flash_claim(FO_PHOTO);
  return CMD_SUCCESS;
}

// Проверяем размер от камеры до первой операции с её секторами.
static uint8_t pr_prepare(uint32_t size)
{
  uint32_t end = photo_storage_end();
  if (!size || size > 65535 || size > end - photo->address - PHOTO_RECORD_HEAD) return 0;
  photo->size = (uint16_t)size;
  photo->stage = P_DATA;
  photo_erased_to = photo->address;
  return 1;
}

// Лениво стираем сектор перед запросом данных для записи в него.
static uint8_t pr_data_ready(void)
{
  uint32_t used = photo->written + PHOTO_DATA;
  if (used > photo->size) used = photo->size;
  uint32_t end = photo->address + PHOTO_RECORD_HEAD + used;
  if (end <= photo_erased_to) return 1;
  pr.task = PT_STORAGE;
  pr_erase(photo_erased_to, PR_LAZY_DONE);
  return 0;
}

// Проверяем номер, контрольную сумму и остаток размера пакета камеры.
static void pr_frame(uint8_t *p, uint16_t n)
{
  uint16_t id = p[0] | ((uint16_t)p[1] << 8), len = n - 6;
  uint8_t sum = 0;
  for (uint16_t i = 0; i < n - 2;i++) sum += p[i];
  if (id != photo->packet || !len || p[n - 2] != sum || p[n - 1] || len > photo->size - photo->written) return;
  pr.task = PT_STORAGE;
  pr_write(photo->address + PHOTO_RECORD_HEAD + photo->written, p + 4, len, PR_DATA_DONE);
}

// Принимаем серверный курсор команды 21 и начинаем поиск фото.
static cmdCode_e pr_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  if (size && size != 6) return CMD_PARAMERROR;
  if (channel == CH_SMS || !pr_command_ready()) return CMD_RUNERROR;
  pr.requested_number = 0xffffffffu;
  pr.requested_offset = 0;
  if (size) {
    memcpy(&pr.requested_number, args, 4);
    memcpy(&pr.requested_offset, args + 4, 2);
  }
  pr.quiet = pr.cancelled = pr.reply_wait = 0;
  pr.usb_epoch = usb_bus_resets;
  photo->channel = channel;
  photo->requestId = id;
  photo->pending = 0;
  pr_scan_begin(PT_REQUEST);
  return (cmdCode_e)255;
}

// Отвечаем на команду 21, включая повтор ACK во время очистки.
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  /* Повтор итогового ACK допустим и пока первый сектор стирается. */
  if (photo_empty_cleanup() && (size == 0 || size == 6) && channel != CH_SMS &&
      !device.status.readyToUpdate && photo_alloc()) {
    photo->channel = channel;
    photo->requestId = id;
    photo->readAddress = mem[MB_PHOTO].start;
    photo->offset = 0;
    photo->pending = 1;
    photo_pending_since = timers_getMillis();
    pr.usb_epoch = usb_bus_resets;
    user_send_answer_comand(21, CMD_SUCCESS, channel, id);
    return;
  }
  cmdCode_e r = pr_request(channel, id, args, size);
  if ((uint8_t)r != 255) user_send_answer_comand(21, r, channel, id);
}

// Начинаем следующий пакет 14 с текущего курсора потока.
static void pr_send_begin(void)
{
  pr.task = PT_SEND;
  pr.quiet = pr.cancelled = 0;
  pr.usb_epoch = usb_bus_resets;
  pr.max_number = 0;
  pr.scan = photo->readAddress;
  pr.phase = PR_HEAD;
  flash_claim(FO_PHOTO);
  pr.reply_wait = 0;
}

// Удаляем первый сектор пустой очереди, остальные займёт новая запись.
static void pr_cleanup_begin(void)
{
  pr.cancelled = pr.quiet = 0;
  pr.committed = pr.cleanup = 1;
  pr.task = PT_STORAGE;
  pr_erase(mem[MB_PHOTO].start, PR_CLEAN_END);
}

// Выполняем один ограниченный шаг хранения или формирования пакета.
void photo_storage_poll(uint32_t now)
{
  uint32_t end = photo_storage_end();
  uint16_t n;
  if (!pr.phase) return;
  if (photo && photo->channel == CH_USB && pr.usb_epoch != usb_bus_resets &&
      (pr.task == PT_REQUEST || pr.task == PT_ACK || pr.task == PT_SEND)) {
    pr.quiet = 1;
    photo->pending = 0;
  }
  if (device.status.readyToUpdate) photo_storage_cancel();
  if (pr.waiting) {
    uint8_t r = w25_async_poll(now);
    if (r == 1) return;
    pr.waiting = 0;
    if (r != 2) {
      pr_fail();
      return;
    }
    pr.io_done += pr.io_step;
    if (!pr.io_len || pr.io_done == pr.io_len) pr.phase = pr.after;
    return;
  }
  if (pr.cancelled && !pr.committed) {
    pr_release();
    return;
  }
  switch (pr.phase) {
  case PR_IO: {
    uint16_t page_left = (uint16_t)(256u - ((pr.io_address + pr.io_done) & 255u));
    n = pr.io_len - pr.io_done;
    if (n > page_left) n = page_left;
    pr.io_step = n;
    if (!w25_async_start(pr.io_address + pr.io_done, !pr.io_len ? 0 : sjr.data + pr.io_done, n, now)) {
      pr_fail();
      break;
    }
    pr.waiting = 1;
    break;
  }
  case PR_HEAD: {
    uint16_t confirmed;
    if (pr.scan + PHOTO_RECORD_HEAD > end || (pr.task == PT_SEND && pr.scan >= device.newAddressPhoto)) {
      pr_scan_finish();
      break;
    }
    if (!w25_read_data(pr.scan, (uint8_t*)pr.h, PHOTO_RECORD_HEAD)) {
      pr_fail();
      break;
    }
    if (pr.h[0] == 0xffffffffu || pr.h[0] <= pr.max_number) {
      pr_scan_finish();
      break;
    }
    if (pr.h[2] != PHOTO_FORMAT || !pr.h[1] || pr.h[1] > 65535 || pr.h[1] > end - pr.scan - PHOTO_RECORD_HEAD) {
      /* Неверный первый заголовок очищаем; у конца очереди сохраняем предыдущие фото. */
      if (pr.scan == mem[MB_PHOTO].start) pr_cleanup_begin();
      else pr_scan_finish();
      break;
    }
    pr.max_number = pr.h[0];
    confirmed = ((uint8_t*)pr.h)[PHOTO_RECORD_HEAD - 1] ? 0 : (uint16_t)pr.h[1];
    if (!confirmed && pr.scan == pr.resume_address) confirmed = pr.resume_offset;
    if (confirmed < pr.h[1]) {
      pr.count++;
      if (!pr.first) {
        pr.first = pr.scan;
        pr.first_offset = confirmed;
      }
    }
    if (pr.task == PT_REQUEST && pr.requested_number != 0xffffffffu &&
        ((pr.requested_number && pr.requested_number == pr.h[0]) || (!pr.requested_number && confirmed < pr.h[1]))) {
      if (pr.requested_offset > pr.h[1]) {
        if (!pr.quiet) user_send_answer_comand(21, CMD_PARAMERROR, photo->channel, photo->requestId);
        pr_release();
        break;
      }
      pr.target = pr.scan;
      pr.target_done = confirmed;
      pr.scan = mem[MB_PHOTO].start;
      pr.task = PT_ACK;
      pr.phase = PR_ACK_HEAD;
      break;
    }
    if (pr.task == PT_SEND && confirmed < pr.h[1]) {
      uint16_t off = pr.scan == photo->readAddress ? photo->offset : 0;
      if (off < confirmed) off = confirmed;
      if (off >= pr.h[1]) {
        pr.scan = pr_next(pr.scan, pr.h[1]);
        pr.phase = PR_HEAD;
        break;
      }
      pr.send_offset = off;
      pr.send_size = (uint16_t)(pr.h[1] - off);
      if (pr.send_size > 512) pr.send_size = 512;
      pr.phase = PR_SEND_FILL;
      break;
    }
    pr.scan = pr_next(pr.scan, pr.h[1]);
    pr.phase = PR_HEAD;
    break;
  }
  case PR_CAMERA:
    photo->address = device.newAddressPhoto;
    photo->stamp = device.datetime ? device.datetime : 1;
    if (pr.last_number >= photo->stamp) photo->stamp = pr.last_number + 1;
    photo->written = photo->packet = photo->used = 0;
    photo->retry = photo->syncAck = photo->syncSeen = 0;
    photo->stage = P_SYNC;
    photo->started = now;
    photo->last = now - 50;
    photo->deadline = now;
    uart_init(rs485_baud(), 0, photo->rx, sizeof(photo->rx));
    device.getPhoto = pr.address;
    pr_release();
    break;
  case PR_LAZY_DONE: photo_erased_to += MEMSECTORSIZE;
  pr_release();
  break;
  case PR_DATA_DONE: {
    photo->written += pr.io_len;
    photo->packet++;
    photo->retry = 0;
    photo->deadline = now;
    if (photo->written != photo->size) {
      pr_release();
      break;
    }
    /* После очистки хвост старой очереди остаётся. Новый номер перекрывает
     * первый старый заголовок за снимком, даже если RTC перевели назад. */
    uint32_t next = pr_next(photo->address, photo->size);
    if (next + PHOTO_RECORD_HEAD <= end) {
      if (!w25_read_data(next, (uint8_t*)pr.h, 12)) {
        pr_fail();
        break;
      }
      if (pr.h[2] == PHOTO_FORMAT && pr.h[0] != 0xffffffffu && pr.h[0] >= photo->stamp) {
        if (pr.h[0] == 0xfffffffeu) {
          pr_fail();
          break;
        }
        photo->stamp = pr.h[0] + 1;
      }
    }
    pr.h[0] = photo->stamp;
    pr.h[1] = photo->size;
    pr.h[2] = PHOTO_FORMAT;
    pr_write(photo->address, (uint8_t*)pr.h, 8, PR_PHOTO_MARK);
    break;
  }
  case PR_PHOTO_MARK:
    pr.committed = 1;
    pr_write(photo->address + 8, (uint8_t*)pr.h + 8, 4, PR_PHOTO_DONE);
    break;
  case PR_PHOTO_DONE:
    device.countPhoto++;
    device.status.readyPhoto = 1;
    device.newAddressPhoto = pr_next(photo->address, photo->size);
    pr.last_number = photo->stamp;
    log_addPhotoEvent(photo->stamp);
    photo_command(0x0e, 0, 0, 0xf0, 0xf0);
    photo_stop();
    pr_release();
    break;
  case PR_CLEAN_END:
    pr.cleanup = 0;
    pr.initialized = 1;
    pr.resume_address = 0;
    pr.resume_offset = 0;
    device.newAddressPhoto = mem[MB_PHOTO].start;
    device.countPhoto = 0;
    device.status.readyPhoto = 0;
    device.readPhotoIndex = 0;
    if (photo) {
      photo->readAddress = mem[MB_PHOTO].start;
      photo->offset = 0;
    }
    pr_release();
    break;
  case PR_ACK_HEAD:
    /* Заголовки до цели уже проверены при поиске запроса. */
    if (!w25_read_data(pr.scan + 4, (uint8_t*) & pr.h[1], 4)) {
      pr_fail();
      break;
    }
    if (pr.scan < pr.target || pr.requested_offset == pr.h[1]) {
      sjr.data[0] = 0;
      pr.committed = 1;
      pr_write(pr.scan + PHOTO_RECORD_HEAD - 1, sjr.data, 1, PR_ACK_ADVANCE);
    } else pr.phase = PR_ACK_ADVANCE;
    break;
  case PR_ACK_ADVANCE:
    if (pr.scan == pr.target) {
      pr.resume_address = pr.target;
      pr.resume_offset = pr.requested_offset > pr.target_done ? pr.requested_offset : pr.target_done;
      pr.committed = 0;
      pr_scan_begin(PT_ACK);
    } else {
      pr.scan = pr_next(pr.scan, pr.h[1]);
      pr.phase = PR_ACK_HEAD;
    }
    break;
  case PR_REQUEST_DONE:
    if (!pr.quiet && photo->channel == CH_SERVER && comm_tx_busy()) {
      if (!pr_reply_expired(now)) break;
      pr.quiet = 1;
    }
    if (!pr.quiet) {
      user_send_answer_comand(21, CMD_SUCCESS, photo->channel, photo->requestId);
      photo->readAddress = pr.first ? pr.first : device.newAddressPhoto;
      photo->offset = pr.first ? pr.first_offset : 0;
      photo->pending = 1;
      photo_pending_since = now;
    }
    pr_release();
    break;
  case PR_SEND_FILL:
    if (!pr.tx) {
      pr.tx = work_acquire(PHOTO_BLOCK + 22, WORK_PHOTO_PACKET);
      if (!pr.tx) {
        pr_release();
        break;
      }
      uint16_t len = pr.send_size ? pr.send_size + 14 : 4;
      memcpy(pr.tx, "SLDN", 4);
      pr.tx[4] = (uint8_t)len;
      pr.tx[5] = (uint8_t)(len >> 8);
      pr.tx[6] = 60;
      pr.tx[7] = photo->requestId;
      pr.tx[8] = 14;
      pr.tx[9] = pr.send_size ? (device.countPhoto > 255 ? 255 : (uint8_t)device.countPhoto) : 0;
      if (pr.send_size) {
        memcpy(pr.tx + 10, pr.h, 8);
        memcpy(pr.tx + 18, &pr.send_offset, 2);
      }
      pr.offset = 0;
      pr.send_crc = crc16_calcInit(pr.tx + 4, pr.send_size ? 16 : 6, 65535);
    }
    n = pr.send_size - pr.offset;
    if (n > 64) n = 64;
    if (n && !w25_read_data(pr.scan + PHOTO_RECORD_HEAD + pr.send_offset + pr.offset, pr.tx + 20 + pr.offset, n)) {
      pr_fail();
      break;
    }
    if (n) pr.send_crc = crc16_calcInit(pr.tx + 20 + pr.offset, n, pr.send_crc);
    pr.offset += n;
    if (pr.offset == pr.send_size) {
      memcpy(pr.tx + (pr.send_size ? 20 + pr.send_size : 10), &pr.send_crc, 2);
      pr.phase = PR_SEND_COMMIT;
    }
    break;
  case PR_SEND_COMMIT: {
    if (pr.quiet || !photo->pending) {
      pr_release();
      break;
    }
    n = pr.send_size ? pr.send_size + 22 : 12;
    uint8_t ok = reply_send(pr.tx, n, photo->channel);
    if (!ok) {
      if (!pr_reply_expired(now)) break;
      pr_release();
      break;
    }
    if (pr.send_size) {
      photo->offset = pr.send_offset + pr.send_size;
      photo->readAddress = pr.scan;
      if (photo->offset == pr.h[1]) {
        photo->readAddress = pr_next(photo->readAddress, pr.h[1]);
        photo->offset = 0;
      }
      photo_pending_since = now;
    } else photo->pending = 0;
    pr_release();
    break;
  }
  default: pr_fail();
  break;
  }
}

// Обслуживаем камеру, запуск скана, поток передачи и отложенную очистку.
void photo_poll(void)
{
  if (pr.phase || !user_storage_background_ready()) return;
  uint32_t now = timers_getMillis();
  if (!pr.initialized && !pr.fault) {
    pr.cancelled = pr.quiet = 0;
    pr_scan_begin(PT_STORAGE);
    return;
  }
  if (photo) {
    if (photo->pending && photo->channel == CH_USB && pr.usb_epoch != usb_bus_resets) photo->pending = 0;
    if (device.status.readyToUpdate) photo_storage_cancel();
    else {
      if (photo->stage && uart_getstate() && !uart_isReceiving()) {
        uint16_t n = uart_received();
        if (n > 64) {
          photo_stop();
          n = 0;
        }
        for (uint16_t i = 0; i < n && photo->stage && !pr.phase; i++) {
          if (!photo->used) photo->frame_size = 6;
          photo->frame[photo->used++] = photo->rx[i];
          if (photo->stage == P_DATA && photo->used == 4) {
            /* Размер внешнего кадра проверяем до заполнения буфера и сложения длины. */
            if (photo->frame[3] || photo->frame[2] > PHOTO_DATA) {
              photo_stop();
              break;
            }
            photo->frame_size = (uint8_t)(6u + photo->frame[2]);
          }
          if (photo->used == photo->frame_size) {
            photo_frame(photo->frame, photo->frame_size);
            photo->used = 0;
          }
        }
        uart_clear();
      }
      if (pr.phase) return;
      if (photo->stage && (uint32_t)(now - photo->started) >= 60000u) {
        photo_stop();
      }
      if (photo->stage && (int32_t)(now - photo->deadline) >= 0 &&
          (photo->stage == P_DATA || (uint32_t)(now - photo->last) >= 50u)) {
        if (photo->stage != P_DATA || pr_data_ready()) {
          if (photo->retry++ >= (photo->stage == P_SYNC ? 30 : 3)) {
            photo_stop();
          }
          else {
            photo->used = 0;
            photo_issue();
          }
        }
      }
      if (pr.phase) return;
      if (photo->pending) {
        if ((uint32_t)(now - photo_pending_since) >= 60000 ||
            (photo->channel == CH_SERVER && !comm_channel_ready())) photo->pending = 0;
        else {
          pr_send_begin();
          return;
        }
      }
    }
    if (!photo->stage && !photo->pending) {
      work_release(photo, WORK_PHOTO_CONTEXT);
      photo = 0;
    }
  }
  if (!pr.fault && !device.countPhoto && device.newAddressPhoto > mem[MB_PHOTO].start &&
      (!photo || !photo->stage)) pr_cleanup_begin();
}
