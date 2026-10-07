// Доработка: Кряжев А., 2026
// Wiegand и звонок: проверка ID, групп, расписаний и управление реле.
// При ошибке чтения разрешение не выдаётся; ограничения выходов действуют для всех путей.
// Счётчики проходов сохраняет модуль ID, событие принимает очередь журнала.
#include "user_logic_internal.h"
#include "../user_format.h"
#include "../sched16.h"
logControl_e server_out_event = LOG_CON_OUTONBYAPI;
uint8_t unblock_active;
static uint8_t unblock_relays, unblock_pending, unblock_restart, unblock_offline;
static uint32_t unblock_offline_since, unblock_minute;

// Пересчитать намерение; занятое хранилище откладывает только чтение сцен.
void user_unblock_recheck(bool restart)
{
  uint8_t next = 0, relays = 0;
  device.status.unblockHand = device.triggers.handUnBlock;
  device.status.unblockOnScene = device.triggers.sheduleUnBlock;
  unblock_pending = 1;
  unblock_restart |= restart;
  if (device.registredOnServer) {
    unblock_offline_since = device.worktime;
    unblock_offline = 0;
  } else if (device.worktime - unblock_offline_since >= 1800u) unblock_offline = 1;
  if (!unblock_offline) {
    if (device.triggers.handUnBlock) next = 1;
    else if (device.triggers.sheduleUnBlock && log_timeKnown && mem_tables_ready()) {
      uint8_t scenes[8];
      if (!user_storage_flash_ready()) return;
      device.day = user_getweakday(device.datetime);
      device.hour = device.datetime % 86400u / 3600u;
      device.minute = device.datetime % 3600u / 60u;
      if (mem_read(mem[MB_SHEDULE].start + SCHED16_TAIL_OFFSET, scenes, sizeof scenes))
        for (uint8_t i = 0; i < sizeof scenes; ++i)
          if (user_checkShedule(scenes[i])) { next = 2; break; }
    }
  }
  for (uint8_t i = 0; i < 2; ++i) {
    if (config.rele[i].useInBlock) relays |= 1u << i;
    else if (unblock_relays & (1u << i)) out_load(i, OUT_OFF, 0, 0, 0);
  }
  if (unblock_restart || (!unblock_active && next)) {
    // Штатное открытие не замыкает уже разомкнутую линию разблокировки.
    if (next && relays)
      for (uint8_t i = 0; i < 2; ++i)
        if (!(relays & (1u << i))) {
          if (config.rele[i].mode == 0)
            out_load(i, OUT_IMP, config.rele[i].tImpulse, config.rele[i].tPause, config.rele[i].cnt);
          else out_load(i, OUT_ON, 0, 0, 0);
        }
  }
  // Разорвать линию только после запуска реле открытия; снятие замыкает её.
  if (unblock_restart || (!!next != !!unblock_active) || relays != unblock_relays)
    for (uint8_t i = 0; i < 2; ++i)
      if (relays & (1u << i)) out_load(i, next ? OUT_OFF : OUT_ON, 0, 0, 0);
  if (next != unblock_active) {
    if (unblock_active) log_addSystemEvent(unblock_active == 1 ? LOG_SYS_UNDLOCKHANDDIS : LOG_SYS_UNBLOCKSHEDDIS);
    if (next) log_addSystemEvent(next == 1 ? LOG_SYS_UNDLOCKHANDEN : LOG_SYS_UNBLOCKSHEDEN);
  }
  unblock_active = next;
  unblock_relays = relays;
  unblock_minute = device.datetime / 60u;
  unblock_pending = unblock_restart = 0;
}

// Оффлайн считается по uptime; сцены — при смене минуты часов контроллера.
void user_unblock_poll(void)
{
  if (device.registredOnServer) {
    unblock_offline_since = device.worktime;
    if (unblock_offline) unblock_pending = 1;
    unblock_offline = 0;
  } else if (!unblock_offline && device.worktime - unblock_offline_since >= 1800u) {
    unblock_offline = unblock_pending = 1;
  }
  if (unblock_pending || unblock_minute != device.datetime / 60u)
    user_unblock_recheck(false);
}

// Превратить код Wiegand в десятибайтовый ID и проверить доступ.
void WG_readedID(uint32_t id){
  char tb[11];

  format_id(tb, NULL, id);
  if (device.sourceGetID == 1) {
    device.timeToGetID = device.sourceGetID = 0;
    log_addControlEvent(LOG_CON_RECVIDONREADER, tb, 0);
    user_send_readedIDtoServer((uint8_t *)tb, 1);
  }
  user_checkIdAndControl(tb, CT_WIEGAND);
}

// Нормализовать номер звонящего и проверить доступ.
bool user_incomingcall (char* num, uint8_t typ)
{

  if (typ == CT_PHONE){

    // номер звонящего: длиннее поля не пишем
    strncpy(device.callingnumber, num, sizeof(device.callingnumber) - 1u);
    device.callingnumber[sizeof(device.callingnumber) - 1u] = 0;
  }

  // 10 цифр номера
  char *ldn = user_last10digitsnumber(num);

  // проверить номер
  user_checkIdAndControl(ldn, (codeType_e)typ);

  return true;
}

// Проверить все условия доступа ID до включения выходов.
void user_checkIdAndControl(char *id, codeType_e tp){
  if (!mem_tables_ready()) return;

  if (unblock_active) return;

  bool onOut[2] = {false, false};

  // проверим можно ли управлять этим способом: настройки реле действуют для любого пути открытия
  for (uint8_t i = 0; i < 2; i++){
    if (config.rele[i].useInBlock == 0){
      switch (tp){
      case CT_PHONE:
        onOut[i] = true;
        break;
      case CT_WIEGAND:
        if (config.rele[i].onWiegand) onOut[i] = true;
        break;
      case CT_AUTO:
        if (config.rele[i].onCamera) onOut[i] = true;
        break;
      case CT_SERVER:
        if (config.rele[i].onCmdServer) onOut[i] = true;
        break;
      default:
        if (config.rele[i].onOther) onOut[i] = true;
        break;
      }
    }
  }

  // проверяем ID из временного буфера
  if (user_findTempID(id)){
    user_activateOut(onOut[0], onOut[1], tp, id);
    return;
  }

  // авторегистрация
  if (device.status.autoRegistration){
    user_addIdToTempBuffer(id, tp);
    log_addControlEvent(LOG_CON_IDADDBYAUTOREG, id, 0);
  }

  // открывать всем без разбора
  if (device.status.openAnyUser){
    user_activateOut(onOut[0], onOut[1], tp, id);
    return;
  }

  // открывать всем по расписанию
  if (device.triggers.openAllByShedule){
    // проверяем рассписание
    // проходим по расписанию
    if (user_checkShedule(device.triggers.openAllByShedule) == true){
      user_activateOut(onOut[0], onOut[1], tp, id);
    }
    return;
  }

  // можно, ищем ID
  if (onOut[0] || onOut[1]){
    cellID_s cellId;

    // ID найден
    uint8_t found=user_findID(id,&cellId);
    if(found>1) return;
    if (found){
      // проверим разрешение по дате запрета
      if ((cellId.blockTime != 0) && (cellId.blockTime < device.datetime))
      {
        return;
      }
      // время действия истекло (строка 284): не открываем; об окончании уже сообщило событие 138
      if (id_expired(&cellId)){
        return;
      }

      cellGroup_s group;

      // принадлежность к группе
      if (user_getGroup(cellId.group, &group) == false)
      {
        return;
      }

      // включена блокировка группы
      if (group.blocked){
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0);
        return;
      }

      // разрешение на управление данным типом
      switch (tp){
      case CT_PHONE:
        if (group.onPhone == 0) return;
        break;
      case CT_WIEGAND:
        if (group.onWiegand == 0) return;
        break;
      case CT_AUTO:
        if (group.onAuto == 0) return;
        break;
      default:
        break;
      }

      // расписание не задано, круглосуточно - управляем
      if (group.sheduleNumber == 0){
        user_activateOut(onOut[0], onOut[1], tp, id);
        return;
      }

      cellShedule_s shedule;

      // расписание
      if (user_getShedule(group.sheduleNumber, &shedule) == false)
      {
        return;
      }

      // включена блокировка расписания
      if (shedule.blocked){
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0);
        return;
      }

      if (!shedule_window_ok(&shedule)) return;

      user_activateOut(onOut[0], onOut[1], tp, id);
    } else{
      // ID не найден - включено информирование админа
      log_addControlEvent(LOG_CON_IDNOTINBASE, id, 0);
      if (device.triggers.smsUnreg){
        unreg_sms_note(id);
      }
    }
  }
}

// Сохранить счётчики проходов, включить разрешённые реле и записать событие.
bool user_activateOut(bool out1, bool out2, codeType_e tp, char *id){

  if (unblock_active) return false;

  bool run[2];
  uint8_t out;

  run[0] = out1 && !config.rele[0].useInBlock && !out_getmodestate(0);
  run[1] = out2 && !config.rele[1].useInBlock && !out_getmodestate(1);
  if (!run[0] && !run[1])
  {
    return false;
  }

  // проверим счетчик проходов
  uint8_t cnt = 255;
  bool started = false;
  cellID_s cellId;
  /* Проходы считаются только по заданному ID. Команда 8 без ID (короткий пакет) открывает
     по настройкам реле: пустой ID совпадал с ячейкой базы, где ID из нулей, и по её счётчику
     команда молча не выполнялась или списывала проходы чужому абоненту (18.09.2026). */
  uint8_t found=(id!=NULL && id[0]!=0)?user_findID(id,&cellId):0;
  if(found>1) return false;
  if (found){
    bool save = false;
    // задано кол-во проходов
    if (cellId.cntPassOnDay != 255){
      if (id_new_day(&cellId)) save = true;
      // есть проходы
      if (cellId.cntPass != 0){
        cellId.cntPass -= 1;
        cnt = cellId.cntPass;
        save = true;
      }
      else{
        if (save) user_updateID(&cellId); // день отмечен: завтра остаток восстановится
        return false;
      }
    }
    // первый проход ID со временем действия - начало отсчёта
    if (cellId.useTime != 0 && id_first_pass(&cellId) == 0 && device.datetime >= ID_TIME_VALID){
      id_set_first_pass(&cellId, device.datetime);
      id_watch_end(id_time_end(&cellId));
      started = save = true;
    }
    if (save && !user_updateID(&cellId)) return false;
  }

  /* Реле - сразу после решения (счётчик проходов уже сохранён, B25), журнал - после:
     запись событий не задерживает щелчок. */
  for (out = 0; out < 2; out++){
    if (!run[out]) continue;
    // импульс
    if (config.rele[out].mode == 0){
      out_load(out, OUT_IMP, config.rele[out].tImpulse, config.rele[out].tPause, config.rele[out].cnt);
    } else{
      // уровень: проход ID переключает выход, команда сервера 8 включает (строка 186)
      out_load(out, (tp == CT_SERVER) ? OUT_ON : OUT_INV, 0, 0, 0);
    }
  }
  switch (tp){
  case CT_PHONE:
    log_addControlEvent(LOG_CON_OUTONBYPHONE, id, cnt);
    break;
  case CT_WIEGAND:
    log_addControlEvent(LOG_CON_OUTONBYWIEGAND, id, cnt);
    break;
  case CT_AUTO:
    log_addControlEvent(LOG_CON_OUTONBYAVTO, id, cnt);
    break;
  case CT_SERVER:
    log_addControlEvent(server_out_event, id, cnt);
    break;
  default:
    break;
  }
  // проходы ограничены: сервер раздаёт остаток остальным контроллерам объекта
  if (cnt != 255) log_addControlEvent(LOG_CON_PASSCOUNT, id, cnt);
  if (cnt == 0) log_addControlEvent(LOG_CON_PASSLIMIT, id, 0); // 79: последний проход суток
  if (started) log_addControlEvent(LOG_CON_IDTIMESTART, id, 255);
  return true;
}

// Вернуть последние десять символов телефонного номера.
char* user_last10digitsnumber (char* num)
{
  uint16_t l = strlen(num);

  return (l <= 10)?num:(num + (l-10));
}
