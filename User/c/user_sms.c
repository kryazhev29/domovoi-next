// Доработка: Кряжев А., 2026
// Команды SMS, ответы и уведомление администратора о неизвестном ID.
// Имя команды разбирается один раз, ПИН и формат проверяются на входе.
// Занятая отправка оставляет SMS в SIM; очередь уведомления существует только в RAM.
#include "user_logic_internal.h"
#include "../user_format.h"
#include "../../Modules/comm_server.h"
#include "user_comm.h"
enum { SMS_UNKNOWN, SMS_ADD, SMS_IMEI, SMS_STATUS, SMS_RESET, SMS_SETOUT, SMS_DEFAULT };
static struct { char id[10]; uint8_t due; } unreg_sms_pending;

// Преобразовать имя SMS-команды в код для одного switch.
static uint8_t sms_command(const char *name)
{
  static const char *const names[] = {"add", "imei", "status", "reset", "setout", "default"};
  uint8_t i;
  for (i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
    if (!strcmp(name, names[i])) return i + 1u;
  return SMS_UNKNOWN;
}

// Проверить ПИН и выполнить одну принятую SMS-команду.
/* Проверено на плате №1 12.10.2026 (eed4de6): #add 4321,9990001234 -> ID добавлен, событие 67, ответ Ok.
   Изменено 2026-10-15 (SMALL): удалён пустой аргумент format_status — на плате не проверено.
   Изменено 12.10.2026 (f822023): пробелы после запятой и в конце - на плате не проверено. */
bool user_smsreceived(char *num, char *data, bool *first)
{
  char *lineEnd, *pin, *arg, *reply = tempStrBuffer;
  char name[11];
  uint8_t command;
  uint32_t pinValue;
  bool validPin = false, validFormat;
  if (*first) {
    char id[10] = {0}, *tail = user_last10digitsnumber(num);
    memcpy(id, tail, strlen(tail));
    if (!log_addSystemEventID(LOG_SYS_RECVSMS, id)) return false;
    *first = false;
  }
  if (!*data) return true; /* битая PDU: событие есть, команды нет */
  if (device.triggers.smsToAdmin && *data != '#' && !user_send_smsToServer(data)) return false;
  lineEnd = skipchar('\n', data, 1);
  if (lineEnd != data) *(lineEnd - 1) = 0;
  /* Клавиатура телефона дописывает пробел в конце (плата №1, 12.10). */
  for (lineEnd = data + strlen(data); lineEnd > data && (lineEnd[-1] == ' ' || lineEnd[-1] == '\r'); ) *--lineEnd = 0;
  if (*data != '#') return true;
  pin = skipchar(' ', data, 1);
  arg = skipchar(',', data, 1);
  validFormat = pin != data && arg != data;
  if (pin != data && (arg == data || arg > pin))
    validPin = user_parse_pin(pin, arg == data ? strlen(pin) : (uint32_t)(arg - pin - 1),
                              &pinValue) && user_pin_valid(pinValue);
  while (*arg == ' ') arg++; /* пробел после запятой */
  user_strcpy(name, data + 1, ' ', 10);
  command = sms_command(name);
  strcpy(reply, name);
  for (char *c = reply; *c; ++c) if (*c >= 'a' && *c <= 'z') *c -= 'a' - 'A';
  if (!validPin) {
    strcat(reply, ": Error \"Pin not valid\"");
    user_sendsms(reply, num); return true;
  }
  if (fw_update_quiet() && (command == SMS_ADD || command == SMS_RESET ||
                           command == SMS_SETOUT || command == SMS_DEFAULT)) {
    strcat(reply, ": Error \"Update in progress\"");
    user_sendsms(reply, num); return true;
  }
  switch (command) {
  case SMS_ADD:
    if (!validFormat || strlen(arg) != 10u) goto format_error;
    for (uint8_t i = 0; i < 10; i++)
      if (arg[i] < '0' || arg[i] > '9') goto format_error;
    if (!flash_exec_idle()) return false;
    if (!user_addIdToTempBuffer(arg, CT_OTHER)) { strcat(reply, ": Error"); break; }
    log_addControlEvent(LOG_CON_IDADDBYSMS, arg, 0);
    strcat(reply, ": ");
    strcat(reply, arg);
    strcat(reply, " - Ok");
    break;
  case SMS_IMEI:
    strcat(reply, ": "); strcat(reply, device.IMEIGSM);
    break;
  case SMS_STATUS: {
    static const char *const net[] = {"Not network", "gprs", "edge", "3G", "4G", "5G"};
    format_status(reply, device.csq, net[device.netType < 6u ? device.netType : 0],
      device.connected, user_comm_server(comm_server_address()) + 1, device.firmwareVersion,
      device.triggers.handUnBlock, device.triggers.sheduleUnBlock,
      device.triggers.autoregistration, device.triggers.openAll);
    break;
  }
  case SMS_RESET:
    strcat(reply, ": Go to reset"); device.timeToReset = 5;
    break;
  case SMS_SETOUT: {
    uint8_t out;
    if (!validFormat) goto format_error;
    if ((arg[0] != '1' && arg[0] != '2') || arg[1] != 0) goto format_error;
    out = arg[0] - '0';
    if (user_activateOut(out == 1, out == 2, CT_SERVER, NULL)) {
      strcat(reply, out == 1 ? ": Out 1 enabled" : ": Out 2 enabled");
    } else strcat(reply, ": Error, Out is blocked");
    break;
  }
  case SMS_DEFAULT:
    return store_factory_request(2, 0, 0, num);
  default: return true;
  }
  user_sendsms(reply, num);
  return true;
format_error:
  strcat(reply, ": Error \"Format not correct\"");
  user_sendsms(reply, num);
  return true;
}

// Поставить SMS в очередь модема и записать событие принятия.
void user_sendsms(char *mes, char *num){
  if (user_comm_sms(num, mes))
    log_addSystemEventID(LOG_SYS_SENDSMS, user_last10digitsnumber(num));
}

// Запомнить последнее неизвестное ID для уведомления администратора.
void unreg_sms_note(const char *id)
{
  memcpy(unreg_sms_pending.id, id, 10);
  unreg_sms_pending.due = 1;
}

// Передать ожидающее уведомление, когда слот SMS свободен.
void unreg_sms_poll(void)
{
  char text[31];
  if (!unreg_sms_pending.due) return;
  memcpy(text, unreg_sms_pending.id, 10); text[10]=0;
  strcat(text, ": ID is unregistred!");
  if (user_send_smsToServer(text)) unreg_sms_pending.due = 0;
}
