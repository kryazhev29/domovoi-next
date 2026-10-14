// Доработка: Кряжев А., 2026
// Курсоры подтверждённой выгрузки журнала и временных ID.
// Только подтверждённые позиции записываются в device; отправку ведёт upload_runtime.
// Подтверждение дописывается перед продолжением, сектора занимают только писатели колец.
#include "user_logic_internal.h"
#include "../log_ring.h"
uint16_t temp_usbReadIndex, temp_unacked[2];

// Курсоры временных ID: поколение и физическая позиция, независимо по каналам.
uint16_t temp_index(dataChanel_e cch)
{ return cch==CH_USB?temp_usbReadIndex:device.readTempIDIndex; }
void temp_set_index(dataChanel_e cch,uint16_t value)
{ if(cch==CH_USB) temp_usbReadIndex=value; else device.readTempIDIndex=value; }

// Поместить подтверждённые курсоры USB в резерв device.
void upload_store(void){
  uint32_t mark = DEVICE_UPLOAD_MARK;
  memcpy(device.reserv + DEVICE_UPLOAD_MARK_OFFSET, &mark, 4);
  memcpy(device.reserv + DEVICE_TEMP_USB_OFFSET, &temp_usbReadIndex, 2);
  log_ring_checkpoint();
}

// Восстановить подтверждённые курсоры из сохранённого device.
void upload_restore(const device_s *saved){
  log_ring_restore(saved);
  uint32_t mark;
  memcpy(&mark, saved->reserv + DEVICE_UPLOAD_MARK_OFFSET, 4);
  if (mark != DEVICE_UPLOAD_MARK) return; // старые версии подтверждения не сохраняли
  device.readTempIDIndex = saved->readTempIDIndex;
  memcpy(&temp_usbReadIndex, saved->reserv + DEVICE_TEMP_USB_OFFSET, 2);
  upload_store();
}

// Обнулить курсоры и неподтверждённые позиции выбранной таблицы.
void upload_reset(uint8_t cmd){
  if (cmd == 10) log_ring_reset();
  else {
    temp_usbReadIndex=device.readTempIDIndex=temp_unacked[0]=temp_unacked[1]=temp_next=0;
  }
  upload_store();
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
