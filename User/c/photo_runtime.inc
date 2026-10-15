/* Доработка: Кряжев А., 2026. */
/* Автомат фото: один шаг чтения или операции W25 за проход.
* Каждый снимок владеет целыми секторами; стирание выполняется перед записью.
* Метка формата публикует фото, один байт ACK сохраняет полное подтверждение.
* Старое окно сохраняется на время пробы; подтверждённое новое кольцо не очищается по ACK. */
#include "photo_runtime.h"
#include "runtime_internal.h"
#include "usb_istr.h"
#include "../../Bootloader/app_update.h"
photo_runtime_s pr;
#define pj flash_job.data.photo
// Задача определяет ответ команды 21; фаза — следующий шаг флеш или транспорта.
enum {
  PT_STORAGE, PT_REQUEST, PT_ACK, PT_SEND, PT_RING_BOOT, PT_RING_FIND, PT_OLD_REQUEST
};
enum {
  PR_IO = 1, PR_HEAD, PR_CAMERA, PR_LAZY_DONE,
    PR_DATA_DONE, PR_PHOTO_MARK, PR_PHOTO_DONE, PR_CLEAN_END,
    PR_ACK_HEAD, PR_ACK_ADVANCE, PR_REQUEST_DONE, PR_SEND_FILL,
    PR_SEND_COMMIT, PR_IO_WAIT, PR_REQUEST_ERROR, PR_OFFSET_ERROR, PR_INTENT_DONE
};
static void photo_stop(void);
static uint8_t photo_alloc(void);
static void photo_command(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
static void photo_issue(void);
static void photo_frame(uint8_t *, uint16_t);
static void pr_tx_release(void);

// Старое окно сохраняется до конца пробы и выгрузки прежней очереди.
static uint16_t pr_head_size(void) { return pr.circular ? 32u : PHOTO_RECORD_HEAD; }
static uint32_t pr_next(uint32_t address, uint32_t size)
{
  uint32_t next=(address+pr_head_size()+size+4095u)&~4095u;
  return pr.circular && next==PHOTO_RING_END ? PHOTO_RING_START : next;
}
// CRC тела и её инверсия отделяют заголовок от данных и оборванной записи.
static uint32_t pr_header_crc(void)
{
  uint16_t crc=crc16_calcInit((uint8_t *)pj.h,8,65535);
  crc=crc16_calcInit((uint8_t *)&pj.h[4],4,crc);
  return crc | ((uint32_t)(uint16_t)~crc<<16);
}
static bool pr_header_valid(uint32_t end)
{
  if (!pj.h[0] || pj.h[0]==0xffffffffu || !pj.h[1] || pj.h[1]>65535u ||
      pj.h[1]>end-pj.scan-pr_head_size()) return false;
  if (!pr.circular) return pj.h[2]==PHOTO_FORMAT;
  return pj.h[2]==PHOTO_RING_FORMAT && pj.h[5]==pr_header_crc() &&
    pj.h[6]==~pj.h[0];
}

// Разрешаем повтор команды 21 только при очистке уже восстановленной пустой очереди.
bool photo_empty_cleanup(void)
{
  return pr.cleanup && pr.initialized;
}


// Отменяем запрос, но завершаем уже начатую публикацию или очистку.
void photo_storage_cancel(void)
{
  if (pr.tx_length) pr_tx_release();
  if (flash_job.kind == FX_PHOTO && flash_job.result == FX_PENDING) {
    pj.cancelled = 1;
    pj.quiet = 1;
    if (pj.task == PT_ACK) pr.initialized = 0;
  }
  if (photo) {
    photo->pending = 0;
    if (photo->stage) photo_stop();
  }
}

// Освобождаем буфер передачи и владение флеш после операции.
static void pr_tx_release(void)
{
  work_release(pr.tx, WORK_PHOTO_PACKET);
  pr.tx = 0;
  pr.tx_length = 0;
}
static void pr_release(flash_result_t result)
{
  pr_tx_release();
  flash_exec_finish(result);
}

// Закрываем операцию с ошибкой без повторных обращений к носителю.
static void pr_fail(void)
{
  pr.fault = 1;
  /* При ошибке носителя новые операции разрешит только перезапуск. */
  if (photo) {
    if (photo->stage) photo_stop();
    photo->pending = 0;
    if ((pj.task == PT_REQUEST || pj.task == PT_ACK) && !pj.quiet)
      user_send_answer_comand(21, CMD_RUNERROR, photo->channel, photo->requestId);
  }
  pr.initialized = 0;
  pr_release(FX_ERROR);
}

// Принимаем transient до обращения к буферу sjr.
static bool pr_begin(uint8_t step)
{
  flash_exec_args_t args = { NULL, step };
  return flash_exec_begin(FX_PHOTO, &args);
}

// Заголовок копируем в sjr; кадр камеры удерживаем до verify последней порции.
static void pr_write(uint32_t address, const uint8_t *p, uint16_t n, uint8_t after)
{
  if (flash_job.kind != FX_PHOTO && !pr_begin(PR_IO)) return;
  if (after != PR_DATA_DONE && p != sjr.data) memcpy(sjr.data, p, n);
  pj.io_address = address;
  pj.io_len = n;
  // Нулевая длина означает стирание сектора.
  pj.io_done = 0;
  pj.after = after;
  flash_job.step = PR_IO;
}

// Стираем один сектор тем же асинхронным исполнителем W25.
static void pr_erase(uint32_t address, uint8_t after)
{
  pr_write(address, sjr.data, 0, after);
}

// Начинаем проход очереди с первого заголовка.
static void pr_scan_begin(uint8_t task)
{
  flash_job.step = PR_HEAD;
  pj.task = task;
  pj.scan = (task==PT_RING_BOOT || task==PT_RING_FIND) ? PHOTO_RING_START :
    (pr.circular ? (pr.oldest ? pr.oldest : device.newAddressPhoto) : mem[MB_PHOTO].start);
  pj.queue_bytes=0;
  pj.minimum=0xffffffffu;
  pj.count = 0;
  pj.max_number = 0;
  pj.first = 0;
}

// Публикуем проверенные курсоры после OK.
static void pr_queue_publish(void)
{
  pr_release(FX_OK);
  if (!pr.circular) device.newAddressPhoto = pj.scan;
  pr.oldest=pj.first;
  pr.bytes=pj.queue_bytes;
  device.countPhoto = pj.count;
  device.status.readyPhoto = pj.count != 0;
  if (pj.max_number > pr.last_number) pr.last_number = pj.max_number;
  pr.initialized = 1;
}

// Публикуем восстановленную очередь и результат поиска запроса.
static void pr_scan_finish(void)
{
  if (pj.task==PT_OLD_REQUEST) { flash_job.step=PR_OFFSET_ERROR; return; }
  if (pj.task==PT_RING_FIND) {
    device.newAddressPhoto=pj.next_head ? pj.next_head : PHOTO_RING_START;
    pr.last_number=pj.max_number>pr.last_number ? pj.max_number : pr.last_number;
    pr_scan_begin(PT_RING_BOOT);
    return;
  }
  if (pj.task == PT_SEND) {
    flash_job.step = PR_SEND_FILL;
    return;
  }
  if (pj.task == PT_ACK || (pj.task == PT_REQUEST &&
    (pj.requested_number == 0xffffffffu || !device.countPhoto || (!pr.circular && pj.scan == mem[MB_PHOTO].start)))) {
    flash_job.step = PR_REQUEST_DONE;
    return;
  }
  if (pj.task == PT_REQUEST) flash_job.step = PR_REQUEST_ERROR;
  else pr_queue_publish();
}

// Ограничиваем ожидание транспорта, не задерживая основной цикл.
static uint8_t pr_reply_expired(uint32_t now)
{
  if (!pj.reply_wait) {
    pj.reply_wait = 1;
    pj.reply_until = now + 2000;
  }
  return (int32_t)(now - pj.reply_until) >= 0;
}

// Разрешаем новую внешнюю команду после восстановления и освобождения флеш.
static uint8_t pr_command_ready(void)
{
  return flash_exec_idle() && pr.initialized && !pr.fault &&
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
  if ((!pr.circular && !device.countPhoto && device.newAddressPhoto != mem[MB_PHOTO].start) ||
    device.newAddressPhoto >= photo_storage_end()) return CMD_RUNERROR;
  if (pr.last_number == 0xfffffffeu || device.datetime == 0xffffffffu) return CMD_RUNERROR;
  pr_tx_release();
  if (!pr_begin(PR_CAMERA)) return CMD_RUNERROR;
  pj.address = address;
  return CMD_SUCCESS;
}

// Проверяем размер от камеры до первой операции с её секторами.
static uint8_t pr_prepare(uint32_t size)
{
  uint32_t end = photo_storage_end();
  if (!size || size>65535u || device.countPhoto>=20u || pr.bytes+size>1048576u) return 0;
  if (pr.circular) {
    if (17u*4096u>end-photo->address) photo->address=PHOTO_RING_START;
    uint32_t next=photo->address+17u*4096u;
    if (device.countPhoto && photo->address<=pr.oldest && next>pr.oldest) return 0;
    if (pr.latest && photo->address<=pr.latest) pr.turns++;
  } else if (size>end-photo->address-PHOTO_RECORD_HEAD) return 0;
  photo->size = (uint16_t)size;
  photo->stage = P_DATA;
  photo_erased_to = photo->address;
  return 1;
}

// Лениво стираем сектор перед запросом данных для записи в него.
static uint8_t pr_data_ready(void)
{
  if (pr.circular && pr.latest && pr.erase_intent!=photo->address) {
    uint16_t sector=(uint16_t)((photo->address-PHOTO_RING_START)/4096u);
    uint32_t intent=sector | ((uint32_t)(uint16_t)~sector<<16);
    pr_write(pr.latest+28,(uint8_t *)&intent,4,PR_INTENT_DONE);
    return 0;
  }
  uint32_t used = photo->written + PHOTO_DATA;
  if (used > photo->size) used = photo->size;
  uint32_t end = photo->address + pr_head_size() + used;
  if (end <= photo_erased_to) return 1;
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
  pr_write(photo->address + pr_head_size() + photo->written, p + 4, len, PR_DATA_DONE);
}

// Принимаем серверный курсор команды 21 и начинаем поиск фото.
static cmdCode_e pr_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  if (size && size != 6) return CMD_PARAMERROR;
  if (channel == CH_SMS || !pr_command_ready()) return CMD_RUNERROR;
  pr_tx_release(); // Новый курсор заменяет ещё не принятый транспортом пакет.
  if (!pr_begin(PR_HEAD)) return CMD_RUNERROR;
  pj.requested_number = 0xffffffffu;
  if (size) {
    memcpy(&pj.requested_number, args, 4);
    memcpy(&pj.requested_offset, args + 4, 2);
  }
  pr.usb_epoch = usb_bus_resets;
  photo->channel = channel;
  photo->requestId = id;
  photo->pending = 0;
  pr_scan_begin(PT_REQUEST);
  if (pr.circular && pj.requested_number && pj.requested_number==pr.ack_number) {
    if (pj.requested_offset>pr.resume_offset) flash_job.step=PR_OFFSET_ERROR;
    else pj.requested_number=0xffffffffu; // Повтор последнего полного ACK не сканирует историю.
  }
  return (cmdCode_e)255;
}

// Отвечаем на команду 21, включая повтор ACK во время очистки.
void photo_request_answer(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size)
{
  /* Повтор итогового ACK допустим и пока первый сектор стирается. */
  if (photo_empty_cleanup() && (size == 0 || size == 6) && channel != CH_SMS &&
    !device.status.readyToUpdate && photo_alloc()) {
    if (pr.tx_length) pr_tx_release();
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
  if (!pr_begin(PR_HEAD)) return;
  pj.task = PT_SEND;
  pr.usb_epoch = usb_bus_resets;
  pj.scan = photo->readAddress;
}

// Удаляем первый сектор пустой очереди, остальные займёт новая запись.
static void pr_cleanup_begin(void)
{
  pj.task = PT_STORAGE;
  pj.cancelled = 0;
  pj.quiet = 0;
  pj.committed = 1;
  pr.cleanup = 1;
  pr_erase(mem[MB_PHOTO].start, PR_CLEAN_END);
}

// Выполняем один ограниченный шаг хранения или формирования пакета.
// Диспетчер вызывает только FX_PHOTO/PENDING.
void photo_storage_poll(uint32_t now)
{
  uint32_t end = photo_storage_end();
  uint16_t n;
  if (photo && photo->channel == CH_USB && pr.usb_epoch != usb_bus_resets &&
    (pj.task == PT_REQUEST || pj.task == PT_ACK || pj.task == PT_SEND)) {
    pj.quiet = 1;
    photo->pending = 0;
  }
  if (device.status.readyToUpdate) photo_storage_cancel();
  if (flash_job.step == PR_IO_WAIT) {
    uint8_t r = w25_async_poll(now);
    if (r == 1) return;
    if (r != 2) {
      pr_fail();
      return;
    }
    pj.io_done += pj.io_step;
    flash_job.step = !pj.io_len || pj.io_done == pj.io_len ? pj.after : PR_IO;
    return;
  }
  if (pj.cancelled && !pj.committed) {
    pr_release(FX_CANCELLED);
    return;
  }
  switch (flash_job.step) {
  case PR_IO: {
      uint16_t page_left = (uint16_t)(256u - ((pj.io_address + pj.io_done) & 255u));
      n = pj.io_len - pj.io_done;
      if (n > page_left) n = page_left;
      if (n > sizeof(sjr.data)) n = sizeof(sjr.data);
      pj.io_step = n;
      // FX_PHOTO не допускает разбора следующего кадра до завершения записи.
      const uint8_t *source = pj.after == PR_DATA_DONE ? photo->frame + 4 : sjr.data;
      if (!w25_async_start(pj.io_address + pj.io_done, !pj.io_len ? 0 : source + pj.io_done, n, now)) {
        pr_fail();
        break;
      }
      flash_job.step = PR_IO_WAIT;
      break;
    }
  case PR_HEAD: {
      uint16_t confirmed;
      bool finding=pj.task==PT_RING_FIND;
      bool boot=finding || pj.task==PT_RING_BOOT;
      bool physical=boot || pj.task==PT_OLD_REQUEST;
      if (pj.scan+pr_head_size()>end || (pr.circular && !physical && pj.scan==device.newAddressPhoto) ||
          (!pr.circular && pj.task==PT_SEND && pj.scan>=device.newAddressPhoto)) {
        pr_scan_finish(); break;
      }
      if ((pj.task==PT_RING_BOOT || pj.task==PT_OLD_REQUEST) && pr.erase_intent && pj.scan>=pr.erase_intent &&
          pj.scan<pr.erase_intent+17u*4096u) {
        pj.scan+=4096u; break;
      }
      if (!w25_read_data(pj.scan,(uint8_t *)pj.h,pr_head_size())) { pr_fail(); break; }
      if (!pr_header_valid(end) || (!pr.circular && pj.h[0]<=pj.max_number)) {
        if (pr.circular) {
          pj.scan+=4096u;
          if (!physical && pj.scan==end) pj.scan=PHOTO_RING_START;
        } else if (pj.h[0]==0xffffffffu || pj.h[0]<=pj.max_number) pr_scan_finish();
        else if (pj.scan==mem[MB_PHOTO].start) pr_cleanup_begin();
        else pr_scan_finish();
        break;
      }
      // Повтор старого ACK ищет прежний заголовок только при необходимости.
      // Обычный ACK обходит максимум текущие 20 снимков.
      if (pj.task==PT_OLD_REQUEST) {
        if (pj.h[0]==pj.requested_number && !((uint8_t *)pj.h)[12]) {
          if (pj.requested_offset>pj.h[1]) flash_job.step=PR_OFFSET_ERROR;
          else { pj.requested_number=0xffffffffu; pr_scan_begin(PT_REQUEST); }
        } else pj.scan=(pj.scan+32u+pj.h[1]+4095u)&~4095u;
        break;
      }
      if (pr.circular && pj.task==PT_REQUEST && pj.scan==pr.oldest &&
          pj.requested_number && pj.requested_number<pj.h[0]) {
        pj.task=PT_OLD_REQUEST; pj.scan=PHOTO_RING_START; break;
      }
      if (boot) {
        if (finding && pj.h[0]>pj.max_number) {
          uint16_t intent=(uint16_t)pj.h[7];
          pr.erase_intent=(intent<1024 && intent==(uint16_t)~(pj.h[7]>>16)) ?
            PHOTO_RING_START+(uint32_t)intent*4096u : 0;
          pj.max_number=pj.h[0]; pj.next_head=pr_next(pj.scan,pj.h[1]);
          pr.turns=pj.h[4]; pr.latest=pj.scan;
        }
        if (!finding && !((uint8_t *)pj.h)[12] && pj.h[0]>pr.ack_number) {
          pr.ack_number=pj.h[0]; pr.resume_address=pj.scan; pr.resume_offset=(uint16_t)pj.h[1];
        }
        if (!finding && ((uint8_t *)pj.h)[12]) {
          pj.count++; pj.queue_bytes+=pj.h[1];
          if (pj.h[0]<pj.minimum) { pj.minimum=pj.h[0]; pj.first=pj.scan; }
        }
        pj.scan=(pj.scan+32u+pj.h[1]+4095u)&~4095u;
        break;
      }
      pj.max_number = pj.h[0];
      confirmed = ((uint8_t*)pj.h)[PHOTO_RECORD_HEAD - 1] ? 0 : (uint16_t)pj.h[1];
      if (!confirmed && pj.scan == pr.resume_address) confirmed = pr.resume_offset;
      if (confirmed < pj.h[1]) {
        pj.count++;
        pj.queue_bytes+=pj.h[1];
        if (!pj.first) {
          pj.first = pj.scan;
          pj.first_offset = confirmed;
        }
      }
      if (pj.task == PT_REQUEST && pj.requested_number != 0xffffffffu &&
        ((pj.requested_number && pj.requested_number == pj.h[0]) || (!pj.requested_number && confirmed < pj.h[1]))) {
        if (pj.requested_offset > pj.h[1]) {
          flash_job.step = PR_OFFSET_ERROR;
          break;
        }
        pj.target = pj.scan;
        pj.target_done = confirmed;
        pj.scan = pr.circular ? pr.oldest : mem[MB_PHOTO].start;
        pj.task = PT_ACK;
        flash_job.step = PR_ACK_HEAD;
        break;
      }
      if (pj.task == PT_SEND && confirmed < pj.h[1]) {
        uint16_t off = pj.scan == photo->readAddress ? photo->offset : 0;
        if (off < confirmed) off = confirmed;
        if (off >= pj.h[1]) {
          pj.scan = pr_next(pj.scan, pj.h[1]);
          flash_job.step = PR_HEAD;
          break;
        }
        pj.send_offset = off;
        pj.send_size = (uint16_t)(pj.h[1] - off);
        if (pj.send_size > 512) pj.send_size = 512;
        flash_job.step = PR_SEND_FILL;
        break;
      }
      pj.scan = pr_next(pj.scan, pj.h[1]);
      flash_job.step = PR_HEAD;
      break;
    }
  case PR_CAMERA:
    photo->address = device.newAddressPhoto;
    photo->stamp = device.datetime ? device.datetime : 1;
    if (pr.last_number >= photo->stamp) photo->stamp = pr.last_number + 1;
    photo->written = 0;
    photo->used = 0;
    photo->packet = 0;
    photo->retry = 0;
    photo->syncSeen = 0;
    photo->syncAck = 0;
    photo->stage = P_SYNC;
    photo->started = now;
    photo->last = now - 50;
    photo->deadline = now;
    uart_init(rs485_baud(), 0, photo->rx, sizeof(photo->rx));
    device.getPhoto = pj.address;
    pr_release(FX_OK);
    break;
  case PR_INTENT_DONE:
    pr.erase_intent=photo->address;
    pr_release(FX_OK);
    break;
  case PR_LAZY_DONE: photo_erased_to += MEMSECTORSIZE;
    pr_release(FX_OK);
    break;
  case PR_DATA_DONE: {
      photo->written += pj.io_len;
      photo->packet++;
      photo->retry = 0;
      photo->deadline = now;
      if (photo->written != photo->size) {
        pr_release(FX_OK);
        break;
      }
      /* После очистки хвост старой очереди остаётся. Новый номер перекрывает
      * первый старый заголовок за снимком, даже если RTC перевели назад. */
      uint32_t next = pr_next(photo->address, photo->size);
      if (!pr.circular && next + PHOTO_RECORD_HEAD <= end) {
        if (!w25_read_data(next, (uint8_t*)pj.h, 12)) {
          pr_fail();
          break;
        }
        if (pj.h[2] == PHOTO_FORMAT && pj.h[0] != 0xffffffffu && pj.h[0] >= photo->stamp) {
          if (pj.h[0] == 0xfffffffeu) {
            pr_fail();
            break;
          }
          photo->stamp = pj.h[0] + 1;
        }
      }
      pj.h[0] = photo->stamp;
      pj.h[1] = photo->size;
      pj.h[2]=0xffffffffu;
      pj.h[3]=0xffffffffu;
      pj.h[4]=pr.turns;
      pj.h[5]=pr_header_crc();
      pj.h[6]=~pj.h[0]; pj.h[7]=0xffffffffu;
      pr_write(photo->address,(uint8_t *)pj.h,pr.circular ? 32 : 8,PR_PHOTO_MARK);
      pj.h[2]=pr.circular ? PHOTO_RING_FORMAT : PHOTO_FORMAT;
      break;
    }
  case PR_PHOTO_MARK:
    pj.committed = 1;
    pr_write(photo->address + 8, (uint8_t*)pj.h + 8, 4, PR_PHOTO_DONE);
    break;
  case PR_PHOTO_DONE:
    pr_release(FX_OK);
    if (!device.countPhoto) pr.oldest=photo->address;
    pr.bytes+=photo->size;
    pr.latest=photo->address;
    pr.erase_intent=0;
    device.countPhoto++;
    device.status.readyPhoto = 1;
    device.newAddressPhoto = pr_next(photo->address, photo->size);
    pr.last_number = photo->stamp;
    log_addPhotoEvent(photo->stamp);
    photo_command(0x0e, 0, 0, 0xf0, 0xf0);
    photo_stop();
    break;
  case PR_CLEAN_END:
    pr_release(FX_OK);
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
    break;
  case PR_ACK_HEAD:
    if (!w25_read_data(pj.scan,(uint8_t *)pj.h,pr_head_size())) { pr_fail(); break; }
    if (pr.circular && !pr_header_valid(end)) {
      pj.scan+=4096u;
      if (pj.scan==end) pj.scan=PHOTO_RING_START;
      break;
    }
    if (pj.scan != pj.target || pj.requested_offset == pj.h[1]) {
      sjr.data[0] = 0;
      pj.committed = 1;
      pr_write(pj.scan + PHOTO_RECORD_HEAD - 1, sjr.data, 1, PR_ACK_ADVANCE);
    } else flash_job.step = PR_ACK_ADVANCE;
    break;
  case PR_ACK_ADVANCE:
    if (pj.scan == pj.target) {
      pr.ack_number=pr.circular && pj.requested_offset==pj.h[1] ? pj.h[0] : 0;
      pr.resume_address = pj.target;
      pr.resume_offset = pj.requested_offset > pj.target_done ? pj.requested_offset : pj.target_done;
      pj.committed = 0;
      pr_scan_begin(PT_ACK);
    } else {
      pj.scan = pr_next(pj.scan, pj.h[1]);
      flash_job.step = PR_ACK_HEAD;
    }
    break;
  case PR_REQUEST_ERROR:
  case PR_OFFSET_ERROR:
  case PR_REQUEST_DONE: {
    uint8_t step = flash_job.step;
    if (!pj.quiet && photo->channel == CH_SERVER && comm_tx_busy()) {
      if (!pr_reply_expired(now)) break;
      pj.quiet = 1;
    }
    // Старый курсор и неверное смещение ждут TX так же, как успешный ответ.
    // При неверном смещении скан ещё не закончен: очередь не публикуем.
    if (step == PR_OFFSET_ERROR) pr_release(FX_OK);
    else pr_queue_publish();
    if (!pj.quiet) {
      user_send_answer_comand(21, step == PR_REQUEST_DONE ? CMD_SUCCESS : CMD_PARAMERROR,
                              photo->channel, photo->requestId);
      if (step == PR_REQUEST_DONE) {
        photo->readAddress = pj.first ? pj.first : device.newAddressPhoto;
        photo->offset = pj.first ? pj.first_offset : 0;
        photo->pending = 1;
        photo_pending_since = now;
      }
    }
    break;
  }
  case PR_SEND_FILL:
    if (!pr.tx) {
      pr.tx = work_acquire(PHOTO_BLOCK + 22, WORK_PHOTO_PACKET);
      if (!pr.tx) {
        pr_release(FX_OK);
        break;
      }
      uint16_t len = pj.send_size ? pj.send_size + 14 : 4;
      memcpy(pr.tx, "SLDN", 4);
      pr.tx[4] = (uint8_t)len;
      pr.tx[5] = (uint8_t)(len >> 8);
      pr.tx[6] = 60;
      pr.tx[7] = photo->requestId;
      pr.tx[8] = 14;
      pr.tx[9] = pj.send_size ? (device.countPhoto > 255 ? 255 : (uint8_t)device.countPhoto) : 0;
      if (pj.send_size) {
        memcpy(pr.tx + 10, pj.h, 8);
        memcpy(pr.tx + 18, &pj.send_offset, 2);
      }
      pj.offset = 0;
      pj.send_crc = crc16_calcInit(pr.tx + 4, pj.send_size ? 16 : 6, 65535);
    }
    n = pj.send_size - pj.offset;
    if (n > 64) n = 64;
    if (n && !w25_read_data(pj.scan + pr_head_size() + pj.send_offset + pj.offset, pr.tx + 20 + pj.offset, n)) {
      pr_fail();
      break;
    }
    if (n) pj.send_crc = crc16_calcInit(pr.tx + 20 + pj.offset, n, pj.send_crc);
    pj.offset += n;
    if (pj.offset == pj.send_size) {
      memcpy(pr.tx + (pj.send_size ? 20 + pj.send_size : 10), &pj.send_crc, 2);
      flash_job.step = PR_SEND_COMMIT;
    }
    break;
  case PR_SEND_COMMIT:
    if (pj.quiet || !photo->pending) pr_release(FX_OK);
    else {
      pr.tx_length = pj.send_size ? pj.send_size + 22 : 12;
      pr.tx_address = pj.scan;
      pr.tx_until = now + 2000u;
      // Готовый пакет владеет своей памятью; union исполнителя больше не нужен.
      flash_exec_finish(FX_OK);
    }
    break;
  default: pr_fail();
    break;
  }
}

// Передача готового куска не занимает Flash; курсор меняется только после принятия TX.
/* Проверено на плате №1 14.10.2026 (29e91ad, сборка 1791990283): фото 16 КБ по звонку
   дошло на сервер и подтверждено.
   Изменено 2026-10-15 (task/flash-wear-2): обход физического кольца — на плате не проверено. */
static void pr_send_poll(uint32_t now)
{
  if (!photo || !photo->pending || device.status.readyToUpdate ||
      (photo->channel == CH_USB && pr.usb_epoch != usb_bus_resets) ||
      (photo->channel == CH_SERVER && !comm_channel_ready())) {
    if (photo) photo->pending = 0;
  } else if (reply_send(pr.tx, pr.tx_length, photo->channel)) {
    if (pr.tx_length > 12) {
      uint32_t size;
      memcpy(&size, pr.tx + 14, 4);
      photo->offset = rt_get16(pr.tx + 18) + pr.tx_length - 22;
      photo->readAddress = pr.tx_address;
      if (photo->offset == size) {
        photo->readAddress = pr_next(photo->readAddress, size);
        photo->offset = 0;
      }
      photo_pending_since = now;
    } else photo->pending = 0;
  } else if ((int32_t)(now - pr.tx_until) < 0) return;
  pr_tx_release();
}

// Обслуживаем камеру, запуск скана, поток передачи и отложенную очистку.
void photo_poll(void)
{
  uint32_t now = timers_getMillis();
  if (pr.tx_length) { pr_send_poll(now); return; }
  if (!flash_exec_idle()) return;
  if (!pr.initialized && !pr.fault) {
    if (!pr_begin(PR_HEAD)) return;
    pr_scan_begin(PT_STORAGE);
    return;
  }
  if (photo) {
    if (photo->pending && photo->channel == CH_USB && pr.usb_epoch != usb_bus_resets) photo->pending = 0;
    if (device.status.readyToUpdate) photo_storage_cancel();
    else {
      if (photo->stage && uart_getstate() && !uart_isReceiving()) {
        uint16_t n = uart_received();
        if (n > sizeof(photo->rx)) {
          photo_stop();
          n = 0;
        }
        for (uint16_t i = 0; i < n && photo->stage && flash_job.kind != FX_PHOTO; i++) {
          if (!photo->used) photo->frame_size = 6;
          photo->frame[photo->used++] = photo->rx[i];
          if (photo->stage == P_DATA && photo->used == 4) {
            /* Размер внешнего кадра проверяем до заполнения буфера и сложения длины. */
            uint16_t length = rt_get16(photo->frame + 2);
            if (length > PHOTO_DATA) {
              photo_stop();
              break;
            }
            photo->frame_size = 6u + length;
          }
          if (photo->used == photo->frame_size) {
            photo_frame(photo->frame, photo->frame_size);
            photo->used = 0;
          }
        }
        uart_clear();
      }
      if (flash_job.kind == FX_PHOTO) return;
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
      if (flash_job.kind == FX_PHOTO) return;
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
  if (!pr.fault && !device.countPhoto && (!photo || !photo->stage) && !pr.circular &&
      mem[MB_PHOTO].size && !app_trial_active()) {
    if (pr_begin(PR_HEAD)) { pr.circular=1; pr_scan_begin(PT_RING_FIND); }
  } else if (!pr.circular && !pr.fault && !device.countPhoto &&
      device.newAddressPhoto>mem[MB_PHOTO].start && (!photo || !photo->stage) &&
      pr_begin(PR_IO)) pr_cleanup_begin();
}

void photo_storage_reset(void)
{
  photo_storage_cancel();
  work_release(pr.tx, WORK_PHOTO_PACKET);
  memset(&pr, 0, sizeof(pr));
  photo_erased_to = 0;
  photo_pending_since = 0;
}
