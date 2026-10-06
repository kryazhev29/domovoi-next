// Доработка: Кряжев А., 2026
// Причины событий обновления; значения аргумента 2 задаёт Протокол2.
// Коды сохраняются при переводе логики на C и при обрыве загрузки.
// Загрузчик использует коды 20..29; неизвестная причина записывается как 30.
#ifndef USER_FW_REASON_H
#define USER_FW_REASON_H
#include "runtime_internal.h"
typedef enum {
  FW_REASON_NONE = 0,
  FW_REASON_PARAM = 1,
  FW_REASON_BUSY = 2,
  FW_REASON_OLD_BOOT = 3,
  FW_REASON_BOOT_BUSY = 4,
  FW_REASON_IN_TRIAL = 5,
  FW_REASON_NO_BACKUP = 6,
  FW_REASON_BAD_BACKUP = 7,
  FW_REASON_CRC = 8,
  FW_REASON_WRITE = 9,
  FW_REASON_CANCEL = 10,
  FW_REASON_INSTALL = 11,
  FW_REASON_HW = 12,
  FW_REASON_TRIAL_FAIL = 13,
  FW_REASON_BACKUP = 14,
  FW_REASON_TIMEOUT = 15,
  FW_REASON_BOOT_BASE = 20,
  FW_REASON_UNKNOWN = 30
} fw_reason_e;
// Записать событие с готовым числовым кодом причины.
void fw_event(logUpdate_e num, fw_reason_e reason);
// Закончить загрузку, сохранив подтверждённые позиции.
void fw_cancel(fw_reason_e reason);
// Закончить загрузку и ответить по её каналу.
void fw_stop(fw_reason_e reason, uint8_t uniqId);
// Вернуть причину отказа запуска, не меняя состояние.
fw_reason_e fw_refused(void);
// Перевести строковый ABI внешнего сервиса; удаляется вместе с его диффом.
fw_reason_e fw_legacy_reason(const char *text);
#endif
