// Доработка: Кряжев А., 2026
// Загрузка прошивки: проверка блока, запись порциями и завершение по числовой причине.
// W25 проверяет запись; продвижение номера блока происходит только после её завершения.
// Обрыв восстанавливается по заголовку и сохранённому device; чужая прошивка не устанавливается.
#include "user_logic_internal.h"
#include "../fw_crc.h"
static bool fw_async_begin(const uint8_t *data, uint32_t address, dataChanel_e channel);
uint8_t fw_write_errors;
uint32_t fw_rollback_pos, fw_rollback_end;
typedef struct {
  uint8_t *data;
  uint32_t address, position, remaining, crc, expected_crc, usb_epoch;
  uint16_t offset, block;
  uint8_t phase, waiting, channel, id, quiet, cancelled, code;
} fw_block_job_s;
static fw_block_job_s fw_job;
enum { FA_ERASE=1, FA_WRITE, FA_HEADER, FA_CRC, FA_REPLY };

// Вернуть запрет управляющих действий во время обновления.
uint8_t fw_update_quiet(void)
{
  return device.status.readyToUpdate;
}

// Записать событие обновления с готовой числовой причиной.
void fw_event(logUpdate_e num, fw_reason_e reason)
{ log_addUpdateEvent(num, (uint32_t)reason); }

// Отменить владельцев загрузки и сохранить состояние завершения.
void fw_cancel(fw_reason_e reason)
{
  fw_service_cancel();
  fw_async_cancel();
  fw_rollback_end = 0;
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;
  device.FWblockIdx = device.FWblockCount = 0;
  fw_write_errors = 0;
  if (reason != FW_REASON_NONE) fw_event(reason == FW_REASON_TIMEOUT ? LOG_FW_TIMEOUT : LOG_FW_FAIL, reason);
  if (device.cchFWupdate != CH_USB) user_saveindex();
}

// Завершить загрузку по причине и передать ответ её каналу.
void fw_stop(fw_reason_e reason, uint8_t uniqId)
{
  fw_cancel(reason);
  if (device.cchFWupdate == CH_USB || device.registredOnServer)
    user_send_statusUpdateFW(20, FW_BLOCK_END, (dataChanel_e)device.cchFWupdate, uniqId);
}

// Определить несовместимое железо по метке первого блока.
static uint8_t fw_image_foreign(const uint8_t *block0, uint8_t hw)
{
  uint32_t w;
  memcpy((uint8_t*)&w, block0 + FW_HW_MARKER_OFFSET, 4);
  return hw != 0 && (w & 0xFFFFFF00u) == FW_HW_MARKER_TAG && (uint8_t)w != hw;
}

// Проверить внешние параметры образа прошивки.
uint8_t fw_start_rejected(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks)
{
  if (number == 0) return 1;
  if (number == 1) return 0;
  return (size > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) ||
         ((size & 3u) != 0) ||
         (blocks < (size + 1023u) / 1024u) ||
         ((FWBLOCK_OFFSET + (uint32_t)blocks * 1024u) > mem[MB_FIRMWARE].size) ||
         (build == 0) || (size == 0) || (crc == 0) || (blocks == 0);
}

// Выбрать причину запрета запуска обновления.
fw_reason_e fw_refused(void)
{
  if (BOOT_VER < 0x30) return FW_REASON_OLD_BOOT;
  if (boot_manual_busy()) return FW_REASON_BOOT_BUSY;
  if (app_trial_active()) return FW_REASON_IN_TRIAL;
  if (device.rebootToUpdate || device.FWinProgress || fw_rollback_end) return FW_REASON_BUSY;
  return FW_REASON_NONE;
}

// Распознать повтор команды начала уже идущей загрузки.
uint8_t fw_same_download(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks, dataChanel_e cch)
{
  bootFWparam_s h;
  if (!device.FWinProgress || device.cchFWupdate != cch || blocks != device.FWblockCount) return 0;
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))) return 0;
  return h.firmwareVersion == number && h.buildDate == build &&
         h.firmwareSize == size && h.firmwareCRC == crc;
}

// Опубликовать завершение загрузки после окончательной CRC образа.
static uint8_t fw_blocks_done(uint8_t crc_ok)
{
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;
  if (crc_ok) device.rebootToUpdate = 1;
  else fw_event(LOG_FW_FAIL, FW_REASON_CRC);
  if (device.cchFWupdate != CH_USB) user_saveindex();
  return crc_ok ? FW_BLOCK_OK : FW_BLOCK_END;
}

// Проверить номер блока и передать его асинхронной записи.
uint8_t fw_take_block(const uint8_t *p, uint16_t n, dataChanel_e cch)
{
  uint16_t block;
  uint32_t adr;

  if (boot_manual_busy() || cch != (dataChanel_e)device.cchFWupdate) return FW_BLOCK_END;
  if (n != 1026u) return device.FWblockIdx >= device.FWblockCount ? FW_BLOCK_END : FW_BLOCK_BADFRAME;
  memcpy((uint8_t*)&block, p, 2);
  if (block < device.FWblockIdx) return FW_BLOCK_OK;
  if (device.FWblockIdx >= device.FWblockCount) return FW_BLOCK_END;
  if (block != device.FWblockIdx) return FW_BLOCK_BADFRAME;
  if (block == 0 && fw_image_foreign(&p[2], device.hardwareVersion)){
    fw_cancel(FW_REASON_NONE);
    fw_event(LOG_FW_FAIL, FW_REASON_HW);
    return FW_BLOCK_END;
  }
  /* Сектор стирается перед первым блоком, который в него пишет (команда 19 стирает только
     первый): стирание всей области сразу останавливало цикл на секунды и рвало связь.
     Повтор блока стирает свой сектор снова - там только этот блок - и дописывает оборванное. */
  adr = mem[MB_FIRMWARE].start + FWBLOCK_OFFSET + (uint32_t)block * 1024u;
  return fw_async_begin(&p[2], adr, cch) ? FW_BLOCK_PENDING : FW_BLOCK_BADWRITE;
}

// Выбрать блок продолжения после перезапуска.
static uint16_t fw_resume_block(void)
{
  return device.FWblockIdx < device.FWblockCount ? device.FWblockIdx : UINT16_MAX;
}

// Восстановить допустимую загрузку из заголовка и сохранённого device.
void fw_resume_after_boot(void)
{
  bootFWparam_s h;
  if (!device.FWinProgress) {
    device.updateTimeout = 0;
    device.FWblockIdx = device.FWblockCount = 0;
    return;
  }
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) || BOOT_VER < 0x30 || h.appIdent != 0xFFFFFFFFu || h.firmwareVersion < 2 ||
      fw_start_rejected(h.firmwareVersion, h.buildDate, h.firmwareSize, h.firmwareCRC, device.FWblockCount)) {
    device.FWinProgress = 0;
    device.status.readyToUpdate = 0;
    device.updateTimeout = 0;
    device.FWblockIdx = device.FWblockCount = 0;
    user_saveindex();
    return;
  }
  device.cchFWupdate = CH_SERVER;
  device.FWblockIdx = fw_resume_block();
  if (device.FWblockIdx == UINT16_MAX) { fw_cancel(FW_REASON_UNKNOWN); return; }
  device.startWaitFWtime = 0;
  if (device.updateTimeout == 0 || device.updateTimeout > FW_TIMEOUT_S) device.updateTimeout = FW_TIMEOUT_S;
  device.status.readyToUpdate = 1;
  device.firmwarePercent = device.FWblockIdx * 100 / device.FWblockCount;
}

// Сообщить занятость владельца записи блока прошивки.
bool fw_async_active(void) { return fw_job.phase != 0; }

// Пометить текущую запись отменённой без раннего освобождения флеша.
void fw_async_cancel(void)
{
  if (fw_async_active()) fw_job.cancelled = fw_job.quiet = 1;
}

// Подавить ответ записи при закрытии её канала.
void fw_async_quiet(dataChanel_e channel)
{
  if (fw_async_active() && fw_job.channel == channel) fw_job.quiet = 1;
}

// Зафиксировать уникальный номер ответа текущему блоку.
void fw_async_reply(uint8_t id) { fw_job.id = id; }

// Скопировать проверенный блок и захватить флеш для фоновой записи.
static bool fw_async_begin(const uint8_t *data, uint32_t address, dataChanel_e channel)
{
  if (flash_owner || store.job || mem_config_result() || w25_async_active()) return false;
  fw_job.data = work_acquire(1024u, WORK_FW_BLOCK);
  if (!fw_job.data) return false;
  memcpy(fw_job.data, data, 1024u);
  fw_job.address = address; fw_job.block = device.FWblockIdx;
  fw_job.offset = 0; fw_job.waiting = fw_job.quiet = fw_job.cancelled = 0;
  fw_job.channel = channel; fw_job.usb_epoch = usb_bus_resets;
  flash_claim(FO_FWBLOCK);
  fw_job.phase = ((address + 4095u) & ~4095u) < address + 1024u ? FA_ERASE : FA_WRITE;
  return true;
}

// Освободить данные блока и перейти к ответу результата.
static void fw_async_complete(uint8_t code)
{
  work_release(fw_job.data, WORK_FW_BLOCK); fw_job.data = NULL;
  fw_job.code = code; fw_job.phase = FA_REPLY;
}

// Продвинуть проверенную запись блока и CRC полного образа.
void fw_storage_poll(uint32_t now)
{
  uint8_t r;
  uint16_t n;
  bootFWparam_s header;
  if (fw_job.channel == CH_USB && fw_job.usb_epoch != usb_bus_resets) fw_job.quiet = 1;
  if (fw_job.cancelled) {
    if (fw_job.waiting && w25_async_poll(now) == 1) return;
    work_release(fw_job.data, WORK_FW_BLOCK); fw_job.data = NULL; fw_job.phase = fw_job.waiting = 0; flash_release(FO_FWBLOCK);
    return;
  }
  if (fw_job.phase == FA_REPLY) {
    if (!fw_job.quiet && fw_job.channel == CH_SERVER && comm_tx_busy()) return;
    fw_job.phase = 0; flash_release(FO_FWBLOCK);
    if (!fw_job.quiet) user_send_statusUpdateFW(20, fw_job.code, (dataChanel_e)fw_job.channel, fw_job.id);
    return;
  }
  if (fw_job.waiting) {
    r = w25_async_poll(now);
    if (r == 1) return;
    fw_job.waiting = 0;
    if (r != 2) goto write_error;
    if (fw_job.phase == FA_ERASE) fw_job.phase = FA_WRITE;
    else fw_job.offset += 64u;
    return;
  }
  if (!w25_read_available()) return;
  if (fw_job.phase == FA_ERASE) {
    fw_job.waiting = w25_async_start((fw_job.address + 4095u) & ~4095u, NULL, 0, now);
    if (!fw_job.waiting) goto write_error;
    return;
  }
  if (fw_job.phase == FA_WRITE) {
    if (fw_job.offset < 1024u) {
      fw_job.waiting = w25_async_start(fw_job.address + fw_job.offset, fw_job.data + fw_job.offset, 64u, now);
      if (!fw_job.waiting) goto write_error;
      return;
    }

    fw_write_errors = 0; device.startWaitFWtime = 0;
    device.FWblockIdx = fw_job.block + 1u;
    device.firmwarePercent = (uint8_t)((uint32_t)device.FWblockIdx * 100u / device.FWblockCount);
    if (device.FWblockIdx < device.FWblockCount) { fw_async_complete(FW_BLOCK_OK); return; }
    fw_job.phase = FA_HEADER; return;
  }
  if (fw_job.phase == FA_HEADER) {
    if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&header, sizeof(header))) goto read_error;
    fw_job.remaining = header.firmwareSize; fw_job.expected_crc = header.firmwareCRC;
    fw_job.position = mem[MB_FIRMWARE].start + FWBLOCK_OFFSET; fw_job.crc = fw_crc_begin();
    fw_job.phase = FA_CRC; return;
  }
  if (fw_job.phase == FA_CRC) {
    n = fw_job.remaining > 64u ? 64u : (uint16_t)fw_job.remaining;

    if (!mem_read(fw_job.position, fw_job.data, n)) goto read_error;
    for (uint16_t i = 0; i < n; i += 4u) {
      uint32_t word; memcpy(&word, fw_job.data + i, 4);
      fw_job.crc = fw_crc_next(fw_job.crc, word);
    }
    fw_job.position += n; fw_job.remaining -= n;
    if (!fw_job.remaining) fw_async_complete(fw_blocks_done(fw_job.crc == fw_job.expected_crc));
    return;
  }
  return;
write_error:
  r = ++fw_write_errors < FW_WRITE_TRIES ? FW_BLOCK_BADWRITE : FW_BLOCK_END;
  fw_async_complete(r);
  if (r == FW_BLOCK_END) { fw_job.phase = 0; flash_release(FO_FWBLOCK); fw_cancel(FW_REASON_WRITE); flash_claim(FO_FWBLOCK); fw_job.phase = FA_REPLY; }
  return;
read_error:
  fw_async_complete(FW_BLOCK_END);
  fw_job.phase = 0; flash_release(FO_FWBLOCK); fw_cancel(FW_REASON_UNKNOWN); flash_claim(FO_FWBLOCK); fw_job.phase = FA_REPLY;
}
