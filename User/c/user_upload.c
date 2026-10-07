// Доработка: Кряжев А., 2026
// Курсоры подтверждённой выгрузки журнала и временных ID.
// Только подтверждённые позиции записываются в device; отправку ведёт upload_runtime.
// Намерение очистки сохраняется до стирания, поэтому обрыв допускает повтор, не пропуск.
#include "user_logic_internal.h"
#include "../log_ring.h"
uint16_t log_usbReadIndex, temp_usbReadIndex;
uint16_t log_unacked[2], temp_unacked[2];

// Вернуть подтверждённый курсор заданной выгрузки и канала.
uint16_t upload_index(uint8_t cmd, dataChanel_e cch){
  if (cmd == 10) return cch == CH_USB ? log_usbReadIndex : device.readLogIDIndex;
  return cch == CH_USB ? temp_usbReadIndex : device.readTempIDIndex;
}

// Установить рабочий курсор заданной выгрузки и канала.
void upload_setIndex(uint8_t cmd, dataChanel_e cch, uint16_t value){
  if (cmd == 10) {
    if (cch == CH_USB) log_usbReadIndex = value; else device.readLogIDIndex = value;
  } else {
    if (cch == CH_USB) temp_usbReadIndex = value; else device.readTempIDIndex = value;
  }
}

// Поместить подтверждённые курсоры USB в резерв device.
void upload_store(void){
  uint32_t mark = DEVICE_UPLOAD_MARK;
  memcpy(device.reserv + DEVICE_UPLOAD_MARK_OFFSET, &mark, 4);
  memcpy(device.reserv + DEVICE_LOG_USB_OFFSET, &log_usbReadIndex, 2);
  memcpy(device.reserv + DEVICE_TEMP_USB_OFFSET, &temp_usbReadIndex, 2);
  log_ring_checkpoint();
}

// Восстановить подтверждённые курсоры из сохранённого device.
void upload_restore(const device_s *saved){
  log_ring_restore(saved);
  uint32_t mark;
  memcpy(&mark, saved->reserv + DEVICE_UPLOAD_MARK_OFFSET, 4);
  if (mark != DEVICE_UPLOAD_MARK) return; // старые версии подтверждения не сохраняли
  device.readLogIDIndex = saved->readLogIDIndex;
  device.readTempIDIndex = saved->readTempIDIndex;
  memcpy(&log_usbReadIndex, saved->reserv + DEVICE_LOG_USB_OFFSET, 2);
  memcpy(&temp_usbReadIndex, saved->reserv + DEVICE_TEMP_USB_OFFSET, 2);
  upload_store();
}

// Обнулить курсоры и неподтверждённые позиции выбранной таблицы.
void upload_reset(uint8_t cmd){
  if (cmd == 10) log_ring_reset();
  upload_setIndex(cmd, CH_USB, 0);
  upload_setIndex(cmd, CH_SERVER, 0);
  uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;
  pending[0] = pending[1] = 0;
  upload_store();
}

// Передать подтверждение выгрузки единственному владельцу сохранения.
bool upload_ack(uint8_t cmd, dataChanel_e cch)
{
  return store_ack(cmd, cch);
}

// Передать завершение выгрузки владельцу долговечной очистки.
bool upload_finish(uint8_t cmd)
{
  return store_finish(cmd);
}

// Запустить фоновую выгрузку журнала.
void user_send_logRecords(dataChanel_e cch, uint8_t uniqId){
  upload_runtime_begin(10,cch,uniqId);
}

// Запустить фоновую выгрузку временных ID.
void user_send_tempIDs(dataChanel_e cch, uint8_t uniqId)
{
  upload_runtime_begin(9, cch, uniqId);
}
