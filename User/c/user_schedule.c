// Доработка: Кряжев А., 2026
// Чтение групп, расписаний и праздников для проверки доступа.
// Даты и часы используют местное время контроллера; недоступная таблица запрещает проход.
// Постоянными таблицами и восстановлением после обрыва питания владеет хранилище.
#include "user_logic_internal.h"

// Вычислить день недели из времени контроллера.
uint8_t user_getweakday (uint32_t t)
{
  // какой сейчас день по счету
  uint32_t d = t / 86400u;
  uint8_t wd = d % 7u; // день недели, 0 - Чт

  // преобразуем
  wd += 3;
  if (wd > 6) wd -= 7;

  return wd;
}

// Проверить текущую минуту по интервалу расписания.
bool shedule_time_ok(const cellShedule_s *s)
{
  uint16_t now = (uint16_t)device.hour * 60u + device.minute;
  uint16_t start = (uint16_t)s->startHour * 60u + s->startMinute;
  uint16_t stop = (uint16_t)s->stopHour * 60u + s->stopMinute;
  return start <= stop ? (now >= start && now <= stop) : (now >= start || now <= stop);
}

// Прочитать существующую группу доступа без разрешения при сбое.
bool user_getGroup(uint16_t num, cellGroup_s *group){
uint32_t offset;
  if(!num) return false;
  offset=(uint32_t)(num-1)*sizeof(cellGroup_s);
  if(offset+sizeof(cellGroup_s)>mem[MB_GROUP].size) return false;
  return mem_read(mem[MB_GROUP].start+offset,(uint8_t *)group,sizeof(cellGroup_s)) && *(uint8_t *)group==num;
}

// Собрать расписание из записи и общего хвоста.
bool user_getShedule(uint16_t num, cellShedule_s *shedule){
  return sched16_get(num,shedule);
}

// Проверить текущую дату по таблице праздников.
uint8_t user_dayIsHoliday(void){
uint8_t data[64];
  uint32_t day=device.datetime/86400u;
  for(uint32_t offset=0;offset<1024;offset+=64) {
    if(!mem_read(mem[MB_HOLIDAY].start+offset,data,64)) return 2;
    for(unsigned i=0;i<64;i+=sizeof(cellHoliday_s)) {
      uint32_t value; memcpy(&value,data+i,4); if(value/86400u==day) return 1;
    }
  }
  return 0;
}

// Проверить разрешение прохода по дню и минуте расписания.
bool user_checkShedule(uint8_t shd){
  if (!mem_tables_ready()) return false;
  cellShedule_s shedule;

  // расписание
  if (user_getShedule(shd, &shedule) == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Undefine schedule\r\n");
#endif
    return false;
  }

  // включена блокировка расписания
  if (shedule.blocked){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Shedule is blocked\r\n");
#endif
    return false;
  }

  // проверим день
  if (shedule.day[device.day] == 0)
  {
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Undefine day\r\n");
#endif
    return false;
  }

  bool flag = false;

  // проверим время
  flag = shedule_time_ok(&shedule);

  // не прошли по времени
  if (flag == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Disabled by time\r\n");
#endif
    return false;
  }

  // проверим праздничный день
  flag = false;
  switch (shedule.onHoliday){
  case 0: // 0 - Резерв

    break;
  case 1: // 1 - Игнорировать праздничные дни
    flag = true;
    break;
  case 2: // 2 - Выполнять сценарии только в праздничные дни
    flag = user_dayIsHoliday()==1;
    break;
  case 3: // 3 - Выполнять сценарии во все дни, кроме праздничных
    flag = !user_dayIsHoliday();
    break;
  }

  // не прошли по празднику
  if (flag == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Not check holiday\r\n");
#endif
    return false;
  }

  return true;
}
