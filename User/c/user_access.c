// Доработка: Кряжев А., 2026
// Wiegand и звонок: проверка ID, групп, расписаний и управление реле.
// При ошибке чтения разрешение не выдаётся; ограничения выходов действуют для всех путей.
// Счётчики проходов сохраняет модуль ID, событие принимает очередь журнала.
#include "user_logic_internal.h"
#include "../user_format.h"
#include "../sched16.h"
#include "../photo.h"
logControl_e server_out_event = LOG_CON_OUTONBYAPI;
uint8_t unblock_active;
static uint8_t unblock_relays, unblock_pending, unblock_offline;
static uint32_t unblock_offline_since, unblock_minute;
/* Ожидание разрыва ИК: unblock_wait — бит 0/1 реле управления, бит 2 — выдержка уровня 1 с
   от unblock_wait_since. unblock_level — бит 0/1 реле уровня, чьё состояние запомнено,
   бит 2 — это состояние до разблокировки; 0 — ничего не запомнено. */
static uint32_t unblock_wait_since;
static uint8_t unblock_wait, unblock_level;
/* Бит 0 — предыдущий опрос датчика, бит 1 — закрытие уже обработано. */
static uint8_t unblock_gate;

static void unblock_set(uint8_t i, uint8_t state)
{
  out_load(i, state, 0, 0, 0);
}

// При включении ждём штатное открытие; датчик даёт один импульс при разорванной ИК.
/* Проверено на плате №1 12.10.2026 (ee0979b): включение - один щелчок реле 1, ИК разорвана через 4 с;
   перезагрузка командой 2 - реле 1 молчит, ИК разорвана сразу; выключение - ИК замкнута. */
static void unblock_open(uint8_t relays, bool entry)
{
  if (!relays || relays == 3) return;
  uint8_t i = relays & 1u;
  if (entry) {
    unblock_set(i ^ 1u, OUT_ON);
    unblock_wait = i + 1;
  }
  if (config.rele[i].mode == 0)
    out_load(i, OUT_IMP, config.rele[i].tImpulse,
             entry ? config.rele[i].tPause : 0, entry ? config.rele[i].cnt : 1);
  else {
    if (!(unblock_level & (1u << i)))
      unblock_level = (i + 1) | (out_getstate(i) << 2);
    unblock_set(i, OUT_ON);
    if (entry) {
      unblock_wait |= 4;
      unblock_wait_since = timers_getMillis();
    }
  }
}

// Два одинаковых успешных опроса АЦП; повтор разрешён после двух опросов «открыто».
void user_unblock_input_poll(void)
{
  uint8_t closed = (config.inputMode[0] == INPUT_GATE_CLOSED && device.status.stateIn1) ||
                   (config.inputMode[1] == INPUT_GATE_CLOSED && device.status.stateIn2);
  if (!unblock_active) unblock_gate &= 1u;
  if (closed != (unblock_gate & 1u)) unblock_gate ^= 1u;
  else if (!closed) unblock_gate = 0;
  else if (unblock_active && !(unblock_gate & 2u)) {
    unblock_gate |= 2u;
    unblock_open(unblock_relays, false);
  }
}

// Пересчитать намерение; занятое хранилище откладывает только чтение сцен.
/* Проверено на плате №1 12.10.2026 (ee0979b): включение - один щелчок реле 1, ИК разорвана через 4 с;
   перезагрузка командой 2 - реле 1 молчит, ИК разорвана сразу; выключение - ИК замкнута. */
void user_unblock_recheck(bool restore)
{
  uint8_t next = 0, relays = 0;
  device.status.unblockHand = device.triggers.handUnBlock;
  device.status.unblockOnScene = device.triggers.sheduleUnBlock;
  // Бит 1 сохраняет восстановление при отложенном чтении сцен.
  unblock_pending |= restore ? 2u : 1u;
  if (device.registredOnServer) {
    unblock_offline_since = device.worktime;
    unblock_offline = 0;
  } else if (device.worktime - unblock_offline_since >= 1800u) unblock_offline = 1;
  if (!unblock_offline) {
    if (device.triggers.handUnBlock) next = 1;
    else if (device.triggers.sheduleUnBlock && log_timeKnown) {
      uint8_t scenes[8], r;
      if (!flash_exec_read_allowed()) return;
      device.day = user_getweakday(device.datetime);
      device.hour = device.datetime % 86400u / 3600u;
      device.minute = device.datetime % 3600u / 60u;
      if (!mem_read(mem[MB_SHEDULE].start + SCHED16_TAIL_OFFSET, scenes, sizeof scenes)) return;
      for (uint8_t i = 0; i < sizeof scenes; ++i) {
        r = scenes[i] ? user_checkShedule(scenes[i]) : 0;
        if (r == 2) return; // Отказ чтения не доказывает отсутствие сцены.
        if (r) { next = 2; break; }
      }
    }
  }
  relays = !!config.rele[0].useInBlock | (!!config.rele[1].useInBlock << 1);
  if (!(unblock_pending & 2u) && !unblock_active && next) {
    unblock_open(relays, true);
    unblock_gate |= 2u; // Рабочее включение уже открыло: датчик не дублирует его.
  }
  if (!next || (unblock_pending & 2u)) unblock_wait = 0;
  // Сначала ИК, затем возврат уровня; смена назначения не обрывает импульс.
  if ((unblock_pending & 2u) || (!!next != !!unblock_active) || relays != unblock_relays)
    for (uint8_t n = 0; n < 2; ++n) {
      uint8_t i = (relays & 1u ? 0 : 1) ^ n, state;
      if (unblock_wait & (1u << i)) continue;
      if (relays & (1u << i)) state = !next || unblock_wait;
      else if (!next && (unblock_level & (1u << i))) state = unblock_level >> 2;
      else if (unblock_relays & (1u << i)) state = OUT_OFF;
      else continue;
      unblock_set(i, state);
    }
  if (!next) unblock_level = 0;
  if (next != unblock_active) {
    if (unblock_active) log_addSystemEvent(unblock_active == 1 ? LOG_SYS_UNDLOCKHANDDIS : LOG_SYS_UNBLOCKSHEDDIS);
    if (next) log_addSystemEvent(next == 1 ? LOG_SYS_UNDLOCKHANDEN : LOG_SYS_UNBLOCKSHEDEN);
  }
  unblock_active = next;
  unblock_relays = relays;
  unblock_minute = device.datetime / 60u;
  unblock_pending = 0;
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
  // 1/2 — номер реле + 1; бит 2 — секундная выдержка уровня.
  if (unblock_wait && ((unblock_wait & 4u)
      ? timers_getMillis() - unblock_wait_since >= 1000u
      : !out_getmodestate(unblock_wait - 1))) {
    unblock_wait = 0;
    for (uint8_t i = 0; i < 2; ++i)
      if (unblock_relays & (1u << i)) unblock_set(i, OUT_OFF);
  }
}

// Превратить код Wiegand в десятибайтовый ID и проверить доступ.
void WG_readedID(uint32_t id){
  char tb[11];

  format_id(tb, id);
  if (device.timeToGetID) {
    if (device.sourceGetID == 1) {
      device.timeToGetID = 0;
      device.sourceGetID = 0;
      log_addControlEvent(LOG_CON_RECVIDONREADER, tb, 0);
      user_send_readedIDtoServer((uint8_t *)tb, 1);
    }
    return;
  }
  user_checkIdAndControl(tb, CT_WIEGAND);
}

// Нормализовать номер звонящего и проверить доступ.
bool user_incomingcall (char* num, uint8_t typ)
{

  if (typ == CT_PHONE){

    // номер звонящего: длиннее поля не пишем
    memset(device.callingnumber, 0, sizeof(device.callingnumber));
    user_strcpy(device.callingnumber, num, 0, sizeof(device.callingnumber) - 1u);
  }

  // 10 цифр номера
  char *ldn = user_last10digitsnumber(num);

  // проверить номер
  user_checkIdAndControl(ldn, (codeType_e)typ);

  return true;
}

// Проверить все условия доступа ID до включения выходов.
void user_checkIdAndControl(char *id, codeType_e tp){
  if (!flash_exec_read_allowed()) return;

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
  uint8_t temp = user_findTempID(id);
  if (temp > 1) return;
  if (temp == 1){
    user_activateOut(onOut[0], onOut[1], tp, id);
    return;
  }

  cellID_s cellId;
  uint8_t found = 0;
  // Авторегистрация не переносит известные ID в обход их ограничений.
  if (device.status.autoRegistration){
    found = user_findID(id, &cellId);
    if (found > 1) return;
    if (found == 0 && user_addIdToTempBuffer(id, tp))
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
    if (user_checkShedule(device.triggers.openAllByShedule) == 1){
      user_activateOut(onOut[0], onOut[1], tp, id);
    }
    return;
  }

  // можно, ищем ID
  if (onOut[0] || onOut[1]){
    // ID найден
    if (!device.status.autoRegistration) found = user_findID(id, &cellId);
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

      if (shedule_window(&shedule) != 1) return;

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

  bool run[2] = {out1, out2};
  uint8_t out;

  for (out = 0; out < 2; out++)
    run[out] = run[out] && !config.rele[out].useInBlock &&
      (tp != CT_SERVER || config.rele[out].onCmdServer) && !out_getmodestate(out);
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
    if (save) {
      if (!user_updateID(&cellId)) return false;
      store_io_s *p = &flash_job.data.storeio;
      p->outputs = (uint8_t)(run[0] | (run[1] << 1));
      p->type = (uint8_t)tp;
      p->count = cnt;
      p->started = started;
      p->event = (uint16_t)server_out_event;
      return true;
    }
  }

  user_access_complete((uint8_t)(run[0] | (run[1] << 1)), tp, id, cnt, started);
  return true;
}

// Реле и события публикуются после долговечной записи счётчика проходов.
void user_access_complete(uint8_t outputs, codeType_e tp, char *id, uint8_t cnt, bool started)
{
  uint8_t out;
  /* Реле - сразу после решения (счётчик проходов уже сохранён, B25), журнал - после:
     запись событий не задерживает щелчок. */
  for (out = 0; out < 2; out++){
    if (!(outputs & (1u << out))) continue;
    // импульс
    if (config.rele[out].mode == 0){
      out_load(out, OUT_IMP, config.rele[out].tImpulse, config.rele[out].tPause, config.rele[out].cnt);
    } else{
      // уровень: проход ID переключает выход, команда сервера 8 включает (строка 186)
      out_load(out, (tp == CT_SERVER) ? OUT_ON : OUT_INV, 0, 0, 0);
    }
  }
  // Штатное фото не задерживает реле; занятая или недоступная камера пропускается.
  if (outputs && config.rs485Mode == 1)
    for (out = 0; out < 8; out++)
      if (config.rs485Periphery[out].type == 3) {
        photo_start(config.rs485Periphery[out].address);
        break;
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
    if (id != NULL) log_addControlEvent(server_out_event, id, cnt);
    break;
  default:
    break;
  }
  // проходы ограничены: сервер раздаёт остаток остальным контроллерам объекта
  if (cnt != 255) log_addControlEvent(LOG_CON_PASSCOUNT, id, cnt);
  if (cnt == 0) log_addControlEvent(LOG_CON_PASSLIMIT, id, 0); // 79: последний проход суток
  if (started) log_addControlEvent(LOG_CON_IDTIMESTART, id, 255);

}

// Вернуть последние десять символов телефонного номера.
char* user_last10digitsnumber (char* num)
{
  uint16_t l = strlen(num);

  return (l <= 10)?num:(num + (l-10));
}

// Чистый доступ идёт между шагами; доступ с изменением ID ждёт свободного исполнителя.
bool user_access_ready(const char *id)
{
  if (!flash_exec_read_allowed()) return false;
  if (flash_exec_idle() || unblock_active) return true;
  if (device.status.autoRegistration) return false;
  cellID_s cell;
  uint8_t found = user_findID((char *)id, &cell);
  if (found > 1) return false;
  if (found && (cell.cntPassOnDay != 255 ||
      (cell.useTime && !id_first_pass(&cell) && device.datetime >= ID_TIME_VALID))) return false;
  return true;
}
