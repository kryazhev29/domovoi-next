// Доработка: Кряжев А., 2026
// Граница со строковым ABI fw_service_runtime и загрузчика.
// Собственная логика использует только числовые причины из user_fw_reason.h.
// Эти переходники нужны до применения чужих диффов из отчёта ЛОГИКА.
#include "user_fw_reason.h"
// Преобразовать причину на единственной внешней границе.
fw_reason_e fw_legacy_reason(const char *text)
{
  static const char *const names[] = {"PARAM", "BUSY", "OLD BOOT", "BOOT BUSY", "IN TRIAL",
    "NO BACKUP", "BAD BACKUP", "CRC", "WRITE", "CANCEL", "INSTALL", "HW", "TRIAL FAIL", "BACKUP",
    "TIMEOUT"};
  uint8_t i;
  if (text == NULL) return 0;
  for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    if (strcmp(text, names[i]) == 0) return i + 1u;
  if (strncmp(text, "BOOT ", 5) == 0 && text[5] >= '0' && text[5] <= '9') return 20u + (uint32_t)(text[5] - '0');
  return 30;
}
// Сохранить ABI журнала чужого сервиса обновления.
void fw_log(logUpdate_e num, const char *reason)
{ fw_event(num, fw_legacy_reason(reason)); }
// Сохранить ABI отмены чужого сервиса обновления.
void fw_abort(const char *reason)
{ fw_cancel(fw_legacy_reason(reason)); }
// Сохранить ABI проверки отказа чужого сервиса обновления.
const char *fw_start_refused(void)
{
  switch (fw_refused()) {
  case FW_REASON_OLD_BOOT: return "OLD BOOT";
  case FW_REASON_BOOT_BUSY: return "BOOT BUSY";
  case FW_REASON_IN_TRIAL: return "IN TRIAL";
  case FW_REASON_BUSY: return "BUSY";
  default: return NULL;
  }
}
