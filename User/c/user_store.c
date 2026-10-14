// Доработка: Кряжев А., 2026
// Кооперативное сохранение device, конфигурации и намерений очистки.
// Единственный владелец флеша завершает операцию до отправки её результата.
// Маркеры публикуются до стирания и снимаются после сохранения новых курсоров.
#include "user_logic_internal.h"
#include "user_comm.h"
#include "user_fw_reason.h"
#include "../../Bootloader/app_update.h"
#include "../../Bootloader/boot_manual.h"
#include "../../Bootloader/boot_core.h"
#include "../log_ring.h"
#include "../table_runtime.h"
#include "../sched16.h"
#include "photo_runtime.h"
#include "../../Modules/comm_modem.h"
enum { US_SAVE=1, US_ACK, US_MARK, US_ERASE, US_CLEAN, US_CONFIG, US_RESET, US_REPLY,
       US_COMMAND, US_RECOVER, US_RECOVER_CLEAN, US_FACTORY };
static void store_usb_session(void);
uint8_t store_runtime, store_save_requested, store_reset_requested;
store_state_s store;
flash_exec_job_t flash_job;
static void store_poll(uint32_t now, uint8_t result);
static void factory_poll(uint32_t now);
static void store_io_poll(uint32_t now);
static uint8_t store_phase(void)
{
  return flash_job.kind == FX_STORE && flash_job.step < FS_ID ? flash_job.step : store.intent;
}
static void store_phase_set(uint8_t step)
{
  if (flash_job.kind == FX_STORE && flash_job.step < FS_ID) flash_job.step = step;
  else store.intent = step;
}
static void factory_reply(void);
static struct {
  uint32_t epoch;
  uint8_t channel, uniq, reply, code;
  char number[24];
} factory;
// Приём не выполняет I/O; отказ оставляет запрос и память у вызывающего.
bool flash_exec_idle(void)
{
  return flash_job.kind == FX_NONE && flash_job.result == FX_IDLE &&
    !w25_async_active() && w25_read_available();
}
bool flash_exec_begin(flash_kind_t kind, const flash_exec_args_t *args)
{
  if (kind == FX_NONE || kind > FX_STORE || !flash_exec_idle()) return false;
  static const uint8_t slots[] = { 0, WORK_IDCLEAR, WORK_EXPIRY, WORK_FW_CONTEXT, WORK_UPLOAD_CONTEXT, 0,
    WORK_SERVICE, WORK_TABLE, 0, WORK_FW_BLOCK, 0, 0, 0 };
  void *context = args ? args->context : NULL;
  uint8_t step = args ? args->step : 0;
  if ((kind == FX_EXPIRY || kind == FX_PHOTO || (kind == FX_CRC && !context)) &&
      (store.intent || store_reset_requested || store_save_requested || fw_event_waiting() ||
       (log_pendingCount && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)))) return false;
  uint8_t slot = slots[kind];
  if (kind == FX_CRC && !context) slot = 0;
  if (slot && !work_live(context, slot)) return false;
  if ((kind == FX_IDCLEAR || kind == FX_EXPIRY || (kind == FX_CRC && slot)) && step != 1) return false;
  if (kind == FX_PHOTO && (context || !step)) return false;
  if (kind == FX_LOG && (!context || step != 1)) return false;
  if (kind == FX_INDEX && (context || step != FX_INDEX_READ)) return false;
  if (kind == FX_CRC && !slot && step) return false;
  memset(&flash_job.data, 0, sizeof(flash_job.data));
  if (kind == FX_LOG) memcpy(&flash_job.data.log.cell, context, sizeof(cellLog_s));
  else flash_job.data.context = context;
  flash_job.step = step;
  flash_job.result = FX_PENDING;
  flash_job.kind = kind;
  return true;
}
void flash_exec_finish(flash_result_t result)
{
  if (flash_job.result != FX_PENDING) return;
  flash_job.result = result;
  if (flash_job.kind == FX_IDCLEAR || flash_job.kind == FX_EXPIRY ||
      flash_job.kind == FX_INDEX || flash_job.kind == FX_CRC) idcrc_owner_released();
}
// Конечный результат и union снимаются ровно один раз.
flash_result_t flash_exec_take_result(void)
{
  flash_result_t result = flash_job.result;
  if (result < FX_OK) return result;
  uint8_t kind = flash_job.kind;
  memset(&flash_job, 0, sizeof(flash_job));
  if ((kind >= FX_TABLE && kind <= FX_LOG) || kind == FX_STORE) idcrc_owner_released();
  return result;
}
// Единый допуск чтения: физическая команда и непубликованные таблицы закрывают NOR.
bool flash_exec_read_allowed(void)
{
  if (w25_async_active() || !w25_read_available() || !mem_storage_ready || table_fault || sj_runtime_blocked()) return false;
  if (flash_job.result == FX_PENDING &&
      (flash_job.kind == FX_IDCLEAR || flash_job.kind == FX_EXPIRY || flash_job.kind == FX_TABLE ||
       (flash_job.kind == FX_STORE && (store.clear || store_phase() == US_FACTORY || flash_job.step == FS_ID)))) return false;
  return true;
}
void flash_exec_cancel(void)
{
  switch (flash_job.kind) {
  case FX_IDCLEAR:
    idclear_runtime_cancel();
    break;
  case FX_EXPIRY:
    expiry_runtime_cancel();
    break;
  case FX_FWSERVICE:
    fw_service_cancel();
    break;
  case FX_UPLOAD:
    upload_runtime_cancel();
    break;
  case FX_CRC:
    if (flash_job.data.context) service_crc_cancel();
    else flash_exec_finish(FX_CANCELLED);
    break;
  case FX_INDEX:
    idindex_runtime_suspend();
    break;
  case FX_TABLE:
    table_runtime_cancel();
    break;
  case FX_FWBLOCK:
    fw_async_cancel();
    break;
  case FX_PHOTO:
    photo_storage_cancel();
    break;
  default:
    break;
  }
}
void flash_exec_quiet(uint8_t channel)
{
  if (store.channel == channel) store.quiet = 1;
  if (factory.channel == channel) factory.reply = 0;
  if (flash_job.kind == FX_STORE && flash_job.step == FS_ID && flash_job.data.storeio.channel == channel)
    flash_job.data.storeio.reply = 0;
  idclear_runtime_quiet(channel);
  fw_service_quiet(channel);
  upload_runtime_quiet(channel);
  fw_async_quiet(channel);
  table_runtime_quiet(channel);
  service_crc_quiet(channel);
}
// Принятый kind не вытесняется; фон заканчивает квант в этом же проходе.
static void pick_next(bool below_crc)
{
  if (flash_job.kind != FX_NONE) return;
  if (!below_crc && idcrc_background_begin()) return;
  if (idindex_runtime_begin()) return;
  if (store.intent == US_REPLY) return;
  if (!store.intent && !store_save_requested && !store_reset_requested &&
      !(log_pendingCount && (log_timeKnown || device.worktime >= LOG_TIME_WAIT))) return;
  flash_exec_args_t args = { NULL, store.intent };
  if (flash_exec_begin(FX_STORE, &args)) store.intent = 0;
}
void flash_exec_poll(uint32_t now)
{
  bool picked_idle = flash_job.kind == FX_NONE;
  pick_next(false);
dispatch:
  if (flash_job.result != FX_PENDING && flash_job.kind != FX_CONFIG) return;
  switch (flash_job.kind) {
  case FX_CRC:
    if (flash_job.data.context) service_crc_poll();
    else {
      idcrc_background_poll();
      if (picked_idle && flash_job.result >= FX_OK) {
        flash_exec_take_result();
        pick_next(true);
        picked_idle = false;
        goto dispatch;
      }
    }
    return;
  case FX_INDEX:
    idindex_poll(mem[MB_ID].start);
    return;
  case FX_EXPIRY:
    expiry_runtime_poll(now);
    return;
  case FX_UPLOAD:
    upload_runtime_poll();
    return;
  case FX_IDCLEAR:
    idclear_runtime_poll(now);
    return;
  case FX_FWSERVICE:
    if (mem_config_result() == 1) mem_config_poll(now);
    else fw_service_poll();
    return;
  case FX_TABLE:
    table_runtime_poll(now);
    return;
  case FX_PHOTO:
    photo_storage_poll(now);
    return;
  case FX_FWBLOCK:
    fw_storage_poll(now);
    return;
  case FX_LOG:
    log_storage_poll(now);
    return;
  case FX_CONFIG:
    if (log_ring_ack_poll()) return;
    if (mem_config_result() == 1) mem_config_poll(now);
    else store_poll(now, mem_config_result());
    return;
  case FX_STORE:
    if (flash_job.step >= FS_ID) {
      store_io_poll(now);
      if (flash_job.result >= FX_OK) flash_exec_take_result();
      return;
    }
    if (mem_config_result() == 1) mem_config_poll(now);
    else if (!log_ring_ack_poll()) store_poll(now, mem_config_result());
    if (flash_job.kind == FX_STORE && (!store_phase() || store_phase() == US_REPLY) && !mem_config_result() && !w25_async_active()) {
      if (store_phase() == US_REPLY) store.intent = US_REPLY;
      flash_exec_finish(FX_OK);
      flash_exec_take_result();
    }
    return;
  default:
    return;
  }
}
// Приём команды проверяет намерения и транспорт отдельно от разрешения чтения.
bool store_command_allowed(uint8_t cmd)
{
  store_usb_session();
  if (cmd == 24 || cmd == 27 || (cmd == 21 && photo_empty_cleanup())) return true;
  if (cmd == 8) return flash_exec_read_allowed();
  return !fw_event_waiting() && !store_phase() && !store_reset_requested && flash_exec_idle();
}
// Полный кадр команды 8 остаётся у источника, если доступ потребует записи ID.
/* Проверено на плате №1 14.10.2026 (2e42207, сборка 1791987129): во время съёмки и передачи
   фото USB отвечает без пропусков (457 ответов подряд). */
bool user_command_ready(const uint8_t *ptr, bool usb_input)
{
  // Ответы из RAM не зависят от владельца W25. 9/10 сохраняют только намерение.
  if (usb_input) store_usb_session();
  if (ptr[4] == 9 || ptr[4] == 10) return true;
  if (usb_input) {
    if (!device.userAutorized) return true; // Проверку ПИН выполняет разборщик.
    switch (ptr[4]) {
    case 2: case 3: case 5: case 7: case 8:
    case 12: case 13: case 14: case 15: case 16: case 17: case 18:
    case 19: case 20: case 21: case 23: case 103: case 105: case 110:
      break;
    default: return true; // Включая CMD_UNDEFINE для неподдерживаемого номера.
    }
  }
  if (!store_command_allowed(ptr[4])) return false;
  if (usb_input && ptr[4] != 8 && !(ptr[4] == 21 && photo_empty_cleanup()) && !flash_exec_idle()) return false;
  if (ptr[4] != 8 || flash_exec_idle()) return true;
  uint16_t length = rt_get16(ptr);
  if (length < 16 || !ptr[5] || (ptr[6] != 1 && ptr[7] != 1)) return true;
  char id[11];
  memcpy(id, ptr + 8, 10);
  id[10] = 0;
  return user_access_ready(id);
}
// Отменить ответы и подтверждения предыдущего поколения USB.
// Проверка поколения вызывается на каждом шаге автомата; встраивание убирает переход.
#pragma inline=forced
static inline void store_usb_session(void)
{
  if (store.usb_epoch == usb_bus_resets) return;
  store.usb_epoch = usb_bus_resets;
  flash_exec_quiet(CH_USB);
  log_unacked[0] = temp_unacked[0] = 0;
  store.seen &= 10u;
  if (store_phase() && store.channel == CH_USB) store.quiet = 1;
}
// Записать намерение очистки и её конец в резерв device.
static void store_marker(uint8_t cmd, uint32_t end)
{
  uint32_t mark = cmd ? STORE_CLEAR_MARK | cmd : 0;
  memcpy(device.reserv + DEVICE_CLEAR_MARK_OFFSET, &mark, 4);
  memcpy(device.reserv + DEVICE_CLEAR_END_OFFSET, &end, 4);
}
// Передать снимок config/device атомарному сохранению.
bool store_snapshot(void)
{
  if (flash_job.kind == FX_NONE) {
    flash_exec_args_t args = { NULL, store.intent };
    if (!flash_exec_begin(FX_STORE, &args)) return false;
    store.intent = 0;
  }
  config.crc = crc16_calc((uint8_t*)&config, sizeof(config) - 2);
  device.crc = crc16_calc((uint8_t*)&device, sizeof(device) - 2);
  return mem_config_begin((uint8_t*)&config, (uint8_t*)&device);
}
// Восстановить незавершённую долговечную очистку после старта.
void store_restore(const device_s *saved)
{
  uint32_t mark, end, start, limit;
  memcpy(&mark, saved->reserv + DEVICE_CLEAR_MARK_OFFSET, 4);
  memcpy(&end, saved->reserv + DEVICE_CLEAR_END_OFFSET, 4);
  if (mark != (STORE_CLEAR_MARK | 10u) && mark != (STORE_CLEAR_MARK | 9u)) return;
  uint8_t cmd = (uint8_t)mark;
  start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;
  limit = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;
  if (end < start || end > limit) return;
  store_marker(cmd, end);
  store.clear = store.cmd = cmd;
  store.end = end;
  store_phase_set(US_RECOVER);
  store.waiting = 0;
  store.quiet = 1;
  store.erase = start;
}
// Принять новый запрос выгрузки без повторного подтверждения.
bool store_upload_request(uint8_t cmd, dataChanel_e cch, uint8_t uniq)
{
  uint8_t slot = (cmd == 10 ? 0 : 2) + (cch == CH_USB ? 0 : 1);
  if (store_phase() || mem_config_result()) return false;
  store.cmd = cmd;
  store.channel = cch;
  store.uniq = uniq;
  store.quiet = 0;
  store.repeat = (store.seen & (1u << slot)) && store.last_id[slot] == uniq;
  store.last_id[slot] = uniq;
  store.seen |= 1u << slot;
  return true;
}
// Начать долговечное сохранение подтверждённого курсора.
bool store_ack(uint8_t cmd, dataChanel_e cch)
{
  uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;
  uint8_t channel = cch == CH_USB ? 0 : 1;
  if (!pending[channel] || store.repeat) return true;
  uint16_t old = upload_index(cmd, cch);
  store.cursor = pending[channel];
  upload_setIndex(cmd, cch, store.cursor);
  upload_store();
  bool accepted = store_snapshot();
  upload_setIndex(cmd, cch, old);
  upload_store();
  if (accepted) store_phase_set(US_ACK);
  else store.seen &= ~(1u << ((cmd == 10 ? 0 : 2) + channel));
  return false;
}
// Сохранить намерение очистки до её первого стирания.
bool store_finish(uint8_t cmd)
{
  bool active = cmd == 10 ? device.status.readyLogData || device.logsCount : device.status.readyID || device.tempIDsCount;
  if (!active && !upload_index(cmd, CH_SERVER) && !upload_index(cmd, CH_USB)) return true;
  uint32_t start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;
  uint32_t end = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;
  uint32_t used = cmd == 10 ? 0u : device.newAddressTempID;
  if (used > start && used < end) end = used;
  store.clear = cmd;
  store.erase = start & ~4095u;
  store.end = end;
  store_marker(cmd, end);
  store_phase_set(US_MARK);
  if (!store_snapshot()) store.retry_at = timers_getMillis() + 1000u;
  return false;
}
// Зафиксировать поля отложенного ответа владельца хранения.
void store_command_start(uint8_t cmd, dataChanel_e cch, uint8_t uniq)
{
  store.cmd = cmd;
  store.channel = cch;
  store.uniq = uniq;
  store.quiet = 0;
  if (store_snapshot()) { store_phase_set(US_COMMAND);
    store_save_requested = 0;
  }
  else user_send_answer_comand(cmd, CMD_RUNERROR, cch, uniq);
}
// Применить сохранённую конфигурацию к RS485 и адресам модема.
static void store_config_complete(void)
{
  rs485_set_config(mem_config_snapshot());
  comm_endpoints();
  device.datetimeConfig = config.datetime;
  device.status.defaultConfig = 0;
  // Действующая разблокировка не открывает повторно; включение новой настройкой открывает.
  user_unblock_recheck(false);
  sync_packet(SYNC_CONFIG);
  if (comm_modem_state() == SLM_IDLE) user_comm_boot();
}
// Начать сохранение временно подставленной новой конфигурации.
bool store_config_start(void)
{
  if (store_phase() || !store_snapshot()) return false;
  store_phase_set(US_CONFIG);
  return true;
}
// Передать долговечный результат по исходному каналу.
static void store_reply(void)
{
uint8_t cmd=store.cmd,id=store.uniq,quiet=store.quiet;
  dataChanel_e cch=(dataChanel_e)store.channel;
  if(!quiet && cch==CH_SERVER && comm_tx_busy()) return;
  if(!quiet && cmd==12) { if(service_crc_begin_config(cch,id)) store_phase_set(0);
    return;
  }
  store_phase_set(0);
  if(quiet) return;
  if(cmd==10) user_send_logRecords(cch,id);
  else if(cmd==9) user_send_tempIDs(cch,id);
  else user_send_answer_comand(cmd,cmd == 8 ? (cmdCode_e)store.code : CMD_SUCCESS,cch,id);
}
// Продвинуть одного владельца хранения в порядке приоритетов.
void user_storage_poll(void)
{
  store_usb_session();
  factory_reply();
  bool replied = fw_service_reply_poll();
  if (upload_reply_poll()) replied = true;
  if (store_phase() == US_REPLY) store_reply();
  upload_pending_poll();
  // Освободившийся TX завершает проход; следующая запись принимается отдельно.
  if (replied && flash_job.kind == FX_NONE) return;
  flash_exec_poll(timers_getMillis());
  uint8_t cmd = flash_job.kind == FX_TABLE ? flash_job.data.table.cmd : 0;
  if (flash_job.kind <= FX_LOG) {
    flash_result_t result = flash_exec_take_result();
    // Сцены читаются после публикации TABLE и снятия результата.
    if (result >= FX_OK && (cmd == 14 || cmd == 16)) user_unblock_recheck(false);
  }
}
static void store_poll(uint32_t now, uint8_t result)
{
  if (store_phase() == US_FACTORY) {
    factory_poll(now);
    return;
  }
  if (store_phase() == US_REPLY) {
    if (flash_job.kind == FX_STORE) {
      flash_exec_finish(FX_OK);
      flash_exec_take_result();
    }
    return;
  }
  if (store_phase() == US_RECOVER) {
    uint8_t b[64];
    uint32_t start = mem[store.clear == 10 ? MB_LOG : MB_TEMPID].start;
    if ((int32_t)(now - store.retry_at) < 0) return;
    if (!w25_read_data(store.erase, b, sizeof(b))) { store.retry_at = now + 1000u;
      return;
    }
    for (uint16_t i = 0; i < sizeof(b); ++i) if (b[i] != 255u) {
      upload_reset(store.clear);
      store_marker(0, 0);
      store_phase_set(US_RECOVER_CLEAN);
      if (!store_snapshot()) store.retry_at = now + 1000u;
      return;
    }
    store.erase += sizeof(b);
    if (!(store.erase & 4095u) || store.erase >= store.end) {
      store.erase = start & ~4095u;
      store_phase_set(US_ERASE);
    }
    return;
  }
  if (store_phase() == US_ERASE) {
    if ((int32_t)(now - store.retry_at) < 0) return;
    if (store.waiting) {
      uint8_t r = w25_async_poll(now);
      if (r == 1) return;
      store.waiting = 0;
      if (r != 2) { store.retry_at = now + 1000u;
        return;
      }
      store.erase += 4096u;
      return;
    }
    if (store.erase < store.end) {
      store.waiting = w25_async_start(store.erase, NULL, 0, now);
      if (!store.waiting) store.retry_at = now + 1000u;
      return;
    }
    upload_reset(store.clear);
    if (store.clear == 10) {
      device.logsCount = 0;
      device.status.readyLogData = 0;
      device.newAddressLog = mem[MB_LOG].start;
    } else {
      device.tempIDsCount = 0;
      device.status.readyID = 0;
      device.newAddressTempID = mem[MB_TEMPID].start;
    }
    store_marker(0, 0);
    store_phase_set(US_CLEAN);
    if (!store_snapshot()) store.retry_at = now + 1000u;
    return;
  }
  if (result == 3) {
    mem_config_reset();
    if (store_phase() == US_MARK || store_phase() == US_CLEAN || store_phase() == US_RECOVER_CLEAN) {
      store.retry_at = now + 1000u;
      return;
    }
    uint8_t job = store_phase();
    store_phase_set(0);
    if (job == US_SAVE) store_save_requested = 1;
    if (job == US_ACK) {
      store.seen &= ~(1u << ((store.cmd == 10 ? 0 : 2) + (store.channel == CH_USB ? 0 : 1)));
      if (!store.quiet) upload_pending_retry(store.channel);
    }
    if ((job == US_CONFIG || job == US_COMMAND) && !store.quiet)
      user_send_answer_comand(store.cmd, CMD_RUNERROR, (dataChanel_e)store.channel, store.uniq);
    return;
  }
  if (result == 2) {
    uint8_t job = store_phase();
    if (job == US_CONFIG) {
      store_config_complete();
    }
    mem_config_reset();
    if (job == US_ACK) {
      uint16_t *pending = store.cmd == 10 ? log_unacked : temp_unacked;
      upload_setIndex(store.cmd, (dataChanel_e)store.channel, store.cursor);
      upload_store();
      pending[store.channel == CH_USB ? 0 : 1] = 0;
      store_phase_set(US_REPLY);
    } else if (job == US_MARK) {
      upload_reset(store.clear);
      store_phase_set(US_ERASE);
      store.retry_at = now;
    } else if (job == US_CLEAN || job == US_RECOVER_CLEAN) {
      store.clear = 0;
      store_phase_set(US_REPLY);
    } else if (job == US_CONFIG || job == US_COMMAND) store_phase_set(US_REPLY);
    else if (job == US_RESET) {
      if (log_pendingCount || store_save_requested) {
        store_phase_set(0);
        store_save_requested = 0;
        return;
      }
      store_reset_requested = 0;
      store_phase_set(0);
      USB_PULLUP_OFF;
      user_comm_off();
      NVIC_SystemReset();
    } else store_phase_set(0);
    return;
  }
  if (store_phase() == US_MARK || store_phase() == US_CLEAN || store_phase() == US_RECOVER_CLEAN) {
    if ((int32_t)(now - store.retry_at) >= 0 && !store_snapshot()) store.retry_at = now + 1000u;
    return;
  }
  if (store_phase() || !w25_read_available()) return;
  if (flash_job.kind == FX_STORE) {
    flash_exec_finish(FX_OK);
    flash_exec_take_result();
  }
  if (log_pendingCount && store.clear != 10 &&
      (store_reset_requested || (!store_save_requested && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)))) {
    log_storage_poll(now);
    return;
  }
  if (store_reset_requested) {
    log_flushPending();
    if (log_pendingCount) return;
    if (store_snapshot()) {
      store_save_requested = 0;
      store_phase_set(US_RESET);
    }
  } else if (store_save_requested) {
    if (store_snapshot()) {
      store_save_requested = 0;
      store_phase_set(US_SAVE);
    }
  }
}
#include "factory_runtime.inc"
#include "store_io.inc"
