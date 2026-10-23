// Доработка: Кряжев А., 2026
// Команды SMS, ответы и уведомление администратора о неизвестном ID.
// Имя команды разбирается один раз, ПИН и формат проверяются на входе.
// Занятая отправка оставляет SMS в SIM; очередь уведомления существует только в RAM.
#include "user_logic_internal.h"
#include "../user_format.h"
#include "../../Modules/comm_server.h"
#include "user_comm.h"
#include "../../Modules/comm_at.h"
enum { SMS_UNKNOWN, SMS_ADD, SMS_IMEI, SMS_STATUS, SMS_RESET, SMS_SETOUT, SMS_DEFAULT };
static struct { char id[10]; uint8_t due; } unreg_sms_pending;

// Преобразовать имя SMS-команды в код для одного switch.
static uint8_t sms_command(const char *name)
{
  return comm_at_choice((const uint8_t *)name, (uint16_t)strlen(name), "add\0imei\0status\0reset\0setout\0default\0");
}

// Дописать текст к ответу и вернуть его новый конец.
static char *sms_append(char *end, const char *text)
{
  size_t n = strlen(text);
  memcpy(end, text, n + 1u);
  return end + n;
}

// Проверить ПИН и выполнить одну принятую SMS-команду.
/* Проверено на плате №1 12.10.2026 (eed4de6): #add 4321,9990001234 -> ID добавлен, событие 67, ответ Ok.
   Изменено 2026-10-15 (b46ed55): удалён пустой аргумент format_status — на плате не проверено.
   Изменено 12.10.2026 (f822023): пробелы после запятой и в конце - на плате не проверено.
   Изменено 2026-10-17: пересылка на сервер только при связи с ним — на плате не проверено. */
bool user_smsreceived(char *num, char *data, bool *first)
{
  char *lineEnd, *pin, *arg, *reply = tempStrBuffer, *end;
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
  /* Без сервера пересылку пропускаем: иначе одна чужая SMS останавливала бы всю очередь SMS. */
  if (device.triggers.smsToAdmin && *data != '#' && comm_server_online() && !user_send_smsToServer(data)) return false;
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
  end = sms_append(reply, name);
  for (char *c = reply; *c; ++c) if (*c >= 'a' && *c <= 'z') *c -= 'a' - 'A';
  if (!validPin) {
    sms_append(end, ": Error \"Pin not valid\"");
    user_sendsms(reply, num); return true;
  }
  if (fw_update_quiet() && (command == SMS_ADD || command == SMS_RESET ||
                           command == SMS_SETOUT || command == SMS_DEFAULT)) {
    sms_append(end, ": Error \"Update in progress\"");
    user_sendsms(reply, num); return true;
  }
  switch (command) {
  case SMS_ADD:
    if (!validFormat || strlen(arg) != 10u) goto format_error;
    for (uint8_t i = 0; i < 10; i++)
      if (arg[i] < '0' || arg[i] > '9') goto format_error;
    if (!flash_exec_idle()) return false;
    if (!user_addIdToTempBuffer(arg, CT_OTHER)) { sms_append(end, ": Error"); break; }
    log_addControlEvent(LOG_CON_IDADDBYSMS, arg, 0);
    sms_append(sms_append(sms_append(end, ": "), arg), " - Ok");
    break;
  case SMS_IMEI:
    sms_append(sms_append(end, ": "), device.IMEIGSM);
    break;
  case SMS_STATUS: {
    format_status(reply, device.csq, comm_at_nth("Not network\0gprs\0edge\0" "3G\0" "4G\0" "5G", device.netType < 6u ? device.netType : 0),
      device.connected, user_comm_server(comm_server_address()) + 1, device.firmwareVersion,
      device.triggers.handUnBlock, device.triggers.sheduleUnBlock,
      device.triggers.autoregistration, device.triggers.openAll);
    break;
  }
  case SMS_RESET:
    sms_append(end, ": Go to reset"); device.timeToReset = 5;
    break;
  case SMS_SETOUT: {
    uint8_t out;
    if (!validFormat) goto format_error;
    if ((arg[0] != '1' && arg[0] != '2') || arg[1] != 0) goto format_error;
    out = arg[0] - '0';
    if (user_activateOut(out == 1, out == 2, CT_SERVER, NULL)) {
      sms_append(end, out == 1 ? ": Out 1 enabled" : ": Out 2 enabled");
    } else sms_append(end, ": Error, Out is blocked");
    break;
  }
  case SMS_DEFAULT:
    return store_factory_request(2, 0, 0, num);
  default: return true;
  }
  user_sendsms(reply, num);
  return true;
format_error:
  sms_append(end, ": Error \"Format not correct\"");
  user_sendsms(reply, num);
  return true;
}

// Поставить SMS в очередь модема и записать событие принятия.
bool user_sendsms(char *mes, char *num){
  if (!user_comm_sms(num, mes)) return false;
  log_addSystemEventID(LOG_SYS_SENDSMS, user_last10digitsnumber(num));
  return true;
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
  memcpy(text, unreg_sms_pending.id, 10);
  memcpy(text + 10, ": ID is unregistred!", sizeof ": ID is unregistred!");
  if (user_send_smsToServer(text)) unreg_sms_pending.due = 0;
}
