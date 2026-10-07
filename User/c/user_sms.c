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
bool user_smsreceived(char *num, char *data)
{
  char *lineEnd, *pin, *arg, *reply = tempStrBuffer;
  char name[11];
  uint8_t command;
  uint32_t pinValue;
  bool validPin = false, validFormat;
  if (device.triggers.smsToAdmin && *data != '#' && !user_send_smsToServer(data)) return false;
  log_addSystemEventID(LOG_SYS_RECVSMS, user_last10digitsnumber(num));
  lineEnd = skipchar('\n', data, 1);
  if (lineEnd != data) *(lineEnd - 1) = 0;
  if (*data != '#') return true;
  pin = skipchar(' ', data, 1);
  arg = skipchar(',', data, 1);
  validFormat = pin != data && arg != data;
  if (pin != data && (arg == data || arg > pin))
    validPin = user_parse_pin(pin, arg == data ? strlen(pin) : (uint32_t)(arg - pin - 1),
                              &pinValue) && user_pin_valid(pinValue);
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
    if (!validFormat) goto format_error;
    strncpy(tempStrBuffer + 96, arg, 10); tempStrBuffer[106] = 0;
    user_addIdToTempBuffer(tempStrBuffer + 96, CT_OTHER);
    log_addControlEvent(LOG_CON_IDADDBYSMS, tempStrBuffer + 96, 0);
    strcat(reply, ": "); strcat(reply, tempStrBuffer + 96); strcat(reply, " - Ok");
    break;
  case SMS_IMEI:
    strcat(reply, ": "); strcat(reply, device.IMEIGSM);
    break;
  case SMS_STATUS: {
    static const char *const net[] = {"Not network", "gprs", "edge", "3G", "4G", "5G"};
    format_status(reply, NULL, device.csq, net[device.netType < 6u ? device.netType : 0],
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
    out = (uint8_t)user_decimal(arg);
    if (out != 1 && out != 2) goto format_error;
    if (config.rele[out - 1u].onCmdServer) {
      out_load(out == 1 ? OUT1 : OUT2, OUT_ON, 0, 0, 0);
      strcat(reply, out == 1 ? ": Out 1 enabled" : ": Out 2 enabled");
    } else strcat(reply, ": Error, Out is blocked");
    break;
  }
  case SMS_DEFAULT:
    user_factoryReset(); strcat(reply, ": Complete");
    break;
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
