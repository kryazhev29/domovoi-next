// Доработка: Кряжев А., 2026
// Кооперативное сохранение device, конфигурации и подтверждений выгрузки.
// Единственный владелец флеша завершает операцию до отправки её результата.
// Курсор публикуется после verify; запись колец сама освобождает место.
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
enum { US_SAVE=1, US_ACK, US_CONFIG=6, US_RESET, US_REPLY,
       US_COMMAND, US_FACTORY = 12 };
static void store_usb_session(void);
uint8_t store_runtime, store_save_requested, store_reset_requested;
// Последний проход с готовым хранилищем без запроса перезагрузки; 0 — такого ещё не было.
static uint32_t store_ready_at;
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
// Приоритет записи одинаков для приёма фото и запуска фонового владельца W25.
static bool store_priority_pending(void)
{
  return store.intent || store_reset_requested || store_save_requested || fw_event_waiting() ||
    (log_pendingCount && log_write_ready() && (log_timeKnown || device.worktime >= LOG_TIME_WAIT));
}
// Вид работы, контекст и шаг задаёт сам вызывающий константами; контекст только что взят
// из work_acquire и проверен. Проверяются только занятость и очерёдность (23.10.2026).
bool flash_exec_begin(flash_kind_t kind, const flash_exec_args_t *args)
{
  if (!flash_exec_idle()) return false;
  void *context = args ? args->context : NULL;
  uint8_t step = args ? args->step : 0;
  if (counter_busy() && !(kind==FX_STORE && step==FS_ID)) return false;
  if ((kind == FX_EXPIRY || kind == FX_PHOTO || (kind == FX_CRC && !context)) &&
      store_priority_pending()) return false;
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
// Проверка сроков закрывает чтение только на своей записи: её проход по базе дверь не держит.
bool flash_exec_read_allowed(void)
{
  if (w25_async_active() || !w25_read_available() || !mem_storage_ready || table_fault || sj_runtime_blocked()) return false;
  if (flash_job.result == FX_PENDING &&
      (flash_job.kind == FX_IDCLEAR || (flash_job.kind == FX_EXPIRY && expiry_runtime_writing()) || flash_job.kind == FX_TABLE ||
       (flash_job.kind == FX_STORE && (store_phase() == US_FACTORY || flash_job.step == FS_ID)))) return false;
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
// Полуполная очередь журнала идёт раньше CRC и индекса: пересборка индекса
// по ячейке за проход иначе держала бы журнал, и события после 16-го терялись.
static void pick_next(bool below_crc)
{
  if (flash_job.kind != FX_NONE || counter_busy()) return;
  bool log_due = log_pendingCount && log_write_ready() && (log_timeKnown || device.worktime >= LOG_TIME_WAIT);
  if (!log_due || log_pendingCount < LOG_PENDING_MAX / 2u) {
    if (!below_crc && idcrc_background_begin()) return;
    if (idindex_runtime_begin()) return;
  }
  if (store.intent == US_REPLY) return;
  if (!store_priority_pending() && counter_pending()) {
    if (!counter_drain_begin()) mem_storage_ready=false;
    return;
  }
  if (!store.intent && !store_save_requested && !store_reset_requested && !log_due) return;
  flash_exec_args_t args = { NULL, store.intent };
  if (flash_exec_begin(FX_STORE, &args)) store.intent = 0;
}
void flash_exec_poll(uint32_t now)
{
  if (counter_busy() && (sjr.waiting || flash_job.kind==FX_NONE ||
      (flash_job.kind==FX_STORE && flash_job.data.storeio.mode==2))) {
    counter_poll(now);
    return;
  }
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
  if (id_scan_cell && (cmd==3 || cmd==13 || cmd==17 || cmd==18 || cmd==103)) return false;
  if (cmd == 24 || cmd == 27) return true;
  if (cmd == 8) return flash_exec_read_allowed();
  if (cmd == 21) return !store_priority_pending() && flash_exec_idle();
  return !fw_event_waiting() && !store_phase() && !store_reset_requested && flash_exec_idle();
}
// Полный кадр команды 8 остаётся у источника, если доступ потребует записи ID.
/* Проверено на плате №1 14.10.2026 (2e42207, сборка 1791987129): во время съёмки и передачи
   фото USB отвечает без пропусков (457 ответов подряд).
   Изменено 2026-10-15 (cefcd36): готовность проверяется только источником кадра — на плате не проверено.
   Изменено 2026-10-15 (23e9519): удалён допуск команды 21 во время полной очистки — на плате не проверено.
   Изменено 2026-10-16 (a5f1efc): команда 8 ждёт продолжаемый поиск ID — на плате не проверено.
   Изменено 2026-10-21 (task/wifi-one): USB114 ждёт готовности хранилища — на плате не проверено. */
bool user_command_ready(const uint8_t *ptr, bool usb_input)
{
  // Ответы из RAM не зависят от владельца W25. 9/10 сохраняют только намерение.
  if (usb_input) store_usb_session();
  if (ptr[4] == 101) return !id_scan_cell; // удержать запрос до полного счёта ID
  if (ptr[4] == 9 || ptr[4] == 10) return true;
  if (usb_input) {
    if (!device.userAutorized) return true; // Проверку ПИН выполняет разборщик.
    switch (ptr[4]) {
    case 2: case 3: case 5: case 7: case 8:
    case 12: case 13: case 14: case 15: case 16: case 17: case 18:
    case 19: case 20: case 21: case 23: case 103: case 105: case 110: case 114:
      break;
    default: return true; // Включая CMD_UNDEFINE для неподдерживаемого номера.
    }
  }
  if (!store_command_allowed(ptr[4])) return false;
  if (usb_input && ptr[4] != 8 && !flash_exec_idle()) return false;
  if (ptr[4] != 8) return true;
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
  temp_unacked[0] = 0;
  store.seen &= 10u;
  if (store_phase() && store.channel == CH_USB) store.quiet = 1;
}
// Передать снимок config/device атомарному сохранению.
static bool store_save(bool cursor)
{
  if (flash_job.kind == FX_NONE) {
    flash_exec_args_t args = { NULL, store.intent };
    if (!flash_exec_begin(FX_STORE, &args)) return false;
    store.intent = 0;
  }
  if(cursor) upload_store();
  config.crc = crc16_calc((uint8_t*)&config, sizeof(config) - 2);
  device.crc = crc16_calc((uint8_t*)&device, sizeof(device) - 2);
  return cursor ? mem_cursor_begin((uint8_t*)&config, (uint8_t*)&device) :
                  mem_config_begin((uint8_t*)&config, (uint8_t*)&device);
}
bool store_snapshot(void) { return store_save(false); }
bool store_cursor_snapshot(void) { return store_save(true); }
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
// Отставший канал не держит поколение уже перезаписанного оборота.
// Расстояние курсоров остаётся меньше половины uint16_t и после 32 оборотов.
static bool temp_ack_cursor(dataChanel_e channel, uint16_t cursor)
{
  uint16_t other=temp_index((dataChanel_e)(1u-channel));
  temp_set_index(channel,cursor);
  if(cursor==temp_next || (int16_t)(cursor-other)>2048) {
    temp_set_index((dataChanel_e)(1u-channel),cursor==temp_next?cursor:(uint16_t)(cursor-2048));
    return true;
  }
  return false;
}
// Начать долговечное сохранение подтверждённого курсора.
bool store_temp_ack(dataChanel_e cch)
{
  uint16_t *pending = temp_unacked;
  uint8_t channel = cch == CH_USB ? 0 : 1;
  if (!pending[channel] || store.repeat) return true;
  uint16_t old = temp_index(cch), other = temp_index((dataChanel_e)(1u-channel));
  if((int16_t)(pending[channel]-old)<=0) { pending[channel]=0; return true; }
  store.cursor = pending[channel];
  temp_ack_cursor(cch, store.cursor);
  bool accepted = store_cursor_snapshot();
  temp_set_index(cch, old);
  temp_set_index((dataChanel_e)(1u-channel), other);
  upload_store();
  if (accepted) store_phase_set(US_ACK);
  else store.seen &= ~(1u << (2u + channel));
  return false;
}
// Пустая выгрузка снимает готовность; физическое место освободит следующий оборот.
void store_finish(uint8_t cmd)
{
  if (cmd == 10) device.status.readyLogData = 0;
  else device.status.readyID=0;
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
  uint8_t wiegand = config.wiegandType;
  const config_s *next = (const config_s *)mem_config_snapshot();
  bool endpoint = memcmp(config.server1,next->server1,sizeof(config.server1)+sizeof(config.server2)) ||
    memcmp(&config.useServer1,&next->useServer1,2);
  rs485_set_config((const uint8_t *)next);
  // Считыватель включается и выключается новой настройкой, а не только при старте.
  if (!config.wiegandType) WG_stop();
  else if (!wiegand) WG_init();
  comm_endpoints();
  if (endpoint) comm_channel_wifi_changed();
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
  uint32_t now = timers_getMillis();
  store_usb_session();
  // Хранилище, отказавшее в работе, через 30 с перезагружает контроллер: старт его восстанавливает.
  if (!store_reset_requested) {
    if (mem_storage_ready) store_ready_at = now | 1u;
    else if (store_ready_at && now - store_ready_at >= 30000u) user_savereset();
  }
  factory_reply();
  bool replied = fw_service_reply_poll();
  if (upload_reply_poll()) replied = true;
  if (store_phase() == US_REPLY) store_reply();
  upload_pending_poll();
  // Освободившийся TX завершает проход; следующая запись принимается отдельно.
  if (replied && flash_job.kind == FX_NONE) return;
  flash_exec_poll(now);
  uint8_t cmd = flash_job.kind == FX_TABLE ? flash_job.data.table.cmd : 0;
  if (flash_job.kind <= FX_LOG) {
    flash_result_t result = flash_exec_take_result();
    // Сцены читаются после публикации TABLE и снятия результата.
    if (result >= FX_OK && (cmd == 14 || cmd == 16)) user_unblock_recheck(false);
  }
}
// Перезагрузка ждёт журнал не дольше 10 с от запроса и только при готовом хранилище.
static bool store_reset_waits_log(uint32_t now)
{
  return log_pendingCount && log_write_ready() && mem_storage_ready && now - store_ready_at < 10000u;
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
  if (result == 3) {
    mem_config_reset();
    uint8_t job = store_phase();
    store_phase_set(0);
    if (job == US_SAVE) store_save_requested = 1;
    if (job == US_ACK) {
      store.seen &= ~(1u << (2u + store.channel));
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
    } else if (job == US_COMMAND && store.cmd == 114) {
      const device_s *saved = (const device_s *)(mem_config_snapshot() + sizeof(config));
      photo_storage_cancel();
      memcpy(&device.wifiModem, &saved->wifiModem, 4);
      if (device.wifiModem) rs485_init();
      comm_channel_wifi_changed();
    }
    mem_config_reset();
    if (job == US_ACK) {
      uint16_t *pending = temp_unacked;
      if (temp_ack_cursor((dataChanel_e)store.channel, store.cursor)) {
        pending[1u-store.channel]=0;
      }
      upload_store();
      pending[store.channel == CH_USB ? 0 : 1] = 0;
      store_phase_set(US_REPLY);
    } else if (job == US_CONFIG || job == US_COMMAND) store_phase_set(US_REPLY);
    else if (job == US_RESET) {
      if (store_reset_waits_log(now) || store_save_requested) {
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
  if (store_phase() || !w25_read_available()) return;
  if (flash_job.kind == FX_STORE) {
    flash_exec_finish(FX_OK);
    flash_exec_take_result();
  }
  if (store_reset_requested) {
    if (store_reset_waits_log(now)) {
      if (log_write_ready()) log_storage_poll(now);
      return;
    }
    if (store_snapshot()) {
      store_save_requested = 0;
      store_phase_set(US_RESET);
    }
    return;
  }
  if (log_pendingCount && log_write_ready() && !store_save_requested &&
      (log_timeKnown || device.worktime >= LOG_TIME_WAIT)) {
    log_storage_poll(now);
    return;
  }
  if (store_save_requested && store_snapshot()) {
    store_save_requested = 0;
    store_phase_set(US_SAVE);
  }
}
#include "factory_runtime.inc"
#include "store_io.inc"
