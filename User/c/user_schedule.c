// Доработка: Кряжев А., 2026
// Чтение групп, расписаний и праздников для проверки доступа.
// Даты и часы используют местное время контроллера; недоступная таблица запрещает проход.
// Постоянными таблицами и восстановлением после обрыва питания владеет хранилище.
#include "user_logic_internal.h"
#include "../sched16.h"

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

// Проверить день, время и праздничный режим прочитанного расписания.
bool shedule_window_ok(const cellShedule_s *s)
{
  if (!s->day[device.day] || !shedule_time_ok(s)) return false;
  switch (s->onHoliday) {
  case 1: return true;
  case 2: return user_dayIsHoliday() == 1;
  case 3: return user_dayIsHoliday() == 0;
  default: return false;
  }
}

// Прочитать разрешённое расписание и проверить его текущее окно.
bool user_checkShedule(uint8_t number)
{
  cellShedule_s shedule;
  return mem_tables_ready() && user_getShedule(number, &shedule) &&
    !shedule.blocked && shedule_window_ok(&shedule);
}
