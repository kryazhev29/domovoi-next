// Доработка: Кряжев А., 2026
// Единое заполнение событий и очередь до получения времени сервера.
// Неизвестное время заменяется ровно один раз; владелец журнала выполняет запись.
// Очередь RAM теряется при сбросе, опубликованные записи восстанавливает кольцо журнала.
#include "user_logic_internal.h"
#include "../log_ring.h"
#include "../user_format.h"
uint32_t log_lastIndex;
cellLog_s log_pending[LOG_PENDING_MAX];
uint8_t log_pendingCount, log_timeKnown;

// Определить стёртую запись журнала.
bool log_blank(const cellLog_s *cell){
  const uint8_t *p = (const uint8_t*)cell;
  for (uint16_t i = 0; i < sizeof(*cell); i++) if (p[i] != 0xFF) return false;
  return true;
}

// Передать первое ожидающее событие владельцу записи журнала.
void log_storage_poll(uint32_t now)
{
  if (flash_job.kind != FX_LOG) {
    if (log_pendingCount)
      log_async_begin(&log_pending[0]);
    return;
  }
  if (log_async_step(now) != 2) return;
  --log_pendingCount;
  memmove(log_pending, log_pending + 1, log_pendingCount * sizeof(log_pending[0]));
}

// Сохранить накопленные стартовые события синхронным путём до запуска.
void log_flushPending(void){
  if (store_runtime) return;
  while (log_pendingCount) {
    if (!log_addEvent(&log_pending[0])) return;
    log_pendingCount--;
    memmove(log_pending, log_pending + 1, log_pendingCount * sizeof(log_pending[0]));
  }
}

// Перевести время ожидающих событий из uptime в время сервера.
void log_setTime(uint32_t now){
  device.datetime = now;
  id_endScan = true;
  if (!log_timeKnown) {
    for (uint8_t i = (flash_job.kind == FX_LOG && flash_job.result == FX_PENDING) ? 1u : 0u; i < log_pendingCount; i++){
      log_pending[i].datetime = now - (device.worktime - log_pending[i].datetime);
      log_pending[i].index = log_pending[i].datetime;
    }
  }
  log_timeKnown = 1;
  user_unblock_recheck(false);
  log_flushPending();
}

// Разрешить запись очереди после появления времени или истечения ожидания.
void log_checkPending(void){
  if (log_pendingCount && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)) log_flushPending();
}

// Принять событие в ограниченную очередь журнала.
static bool log_put(cellLog_s *cell){
  /* Запись во флеш делает только владелец журнала (log_storage_poll): здесь только очередь. */
  if (log_pendingCount == LOG_PENDING_MAX) return false;
  log_pending[log_pendingCount] = *cell;
  // до времени сервера datetime хранит uptime, затем переводится один раз
  if (!log_timeKnown) log_pending[log_pendingCount].datetime = device.worktime;
  log_pendingCount++;
  return true;
}

// Единожды заполнить общие поля записи события.
static bool log_event(uint8_t type, const void *id, uint8_t passes, uint32_t argument)
{
  cellLog_s cell;
  memset(&cell, 0, sizeof(cell));
  cell.index = cell.datetime = device.datetime;
  cell.logType = type;
  cell.passCnt = passes;
  cell.arg2 = argument;
  if (id) memcpy(cell.ID, id, sizeof(cell.ID));
  return log_put(&cell);
}

// Записать системное событие с нулевыми аргументами.
bool log_addSystemEvent(logSystem_e num)
{ return log_event((uint8_t)num, NULL, 0, 0); }

// Преобразовать числовое значение системного события в поле ID.
bool log_addSystemValue(logSystem_e num, int32_t value)
{
  char id[12] = {0};
  format_i32(id, NULL, value);
  return log_addSystemEventID(num, id);
}

// Записать системное событие с указанным ID.
bool log_addSystemEventID(logSystem_e num, char *id)
{ return log_event((uint8_t)num, id, 0, 0); }

// Записать событие доступа с ID и остатком проходов.
bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt)
{ return log_event((uint8_t)num, id, cnt, 0); }

// Записать событие обновления с версией, билдом и числовой причиной.
bool log_addUpdateEvent(logUpdate_e num, uint32_t reason)
{
  uint8_t id[10] = {0};
  id[0] = device.hardwareVersion;
  rt_put16(id + 1, device.firmwareVersion);
  rt_put32(id + 3, device.buildDate);
  return log_event((uint8_t)num, id, 0, reason);
}

// Записать номер сохранённой фотографии в событие журнала.
bool log_addPhotoEvent(uint32_t number)
{
  uint8_t id[10] = {0};
  rt_put32(id, number);
  return log_event((uint8_t)LOG_CON_PHOTOFIX, id, 0, 0);
}
