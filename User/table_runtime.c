// Доработка: Кряжев А., 2026
// Обработка таблиц и переход на расписания по 16 Б.
// Изменения секторов выполняются журналом; маркер перехода публикуется последним.
// Формат кадров проверяется до записи, CRC расписаний охватывает 1040 Б.
#include "c/runtime_internal.h"
#include "table_runtime.h"
#include "fast_kernels.h"
#include "user_fun.h"
#include "c/user_logic_internal.h"
#include "sched16.h"
#include "sector_journal.h"
extern uint32_t id_nextEnd;
extern uint32_t id_time_end(cellID_s *);
extern void id_watch_end(uint32_t);
enum {
  SYNC_ID=1,SYNC_GROUP=4,SYNC_SHEDULE=8,SYNC_HOLIDAY=16
};
#include "c/table_runtime.inc"

// Все варианты и команда 17 используют одну область CRC.
uint32_t sched16_crc_size(void) { return 1040u; }

// Проверяет длину, количество и номера расписаний входящего кадра.
bool sched16_valid(const uint8_t *packet, uint16_t length)
{
  if (length < 24u) return false;
  uint16_t count=packet[9];
  if (!count || count>SCHED16_COUNT || length!=24u+count*SCHED16_RECORD_SIZE) return false;
  for (uint16_t i=0;i<count;i++)
    if (!packet[10u+i*SCHED16_RECORD_SIZE] || packet[10u+i*SCHED16_RECORD_SIZE]>SCHED16_COUNT) return false;
  return true; // Сохраняем прежние правила допустимости полей команды.
}

// Читает расписание и общий хвост в прежнее представление исполнителя.
uint8_t sched16_get(uint16_t number, cellShedule_s *schedule)
{
  if (!number || number>SCHED16_COUNT) return 0;
  uint32_t base=mem[MB_SHEDULE].start;
  if (!mem_read(base+(number-1u)*SCHED16_RECORD_SIZE,(uint8_t *)schedule,16)) return 2;
  if (schedule->index!=number) return 0;
  if (!mem_read(base+SCHED16_TAIL_OFFSET,(uint8_t *)schedule+16,16)) return 2;
  // Сохраняем размещение scene и onHoliday для исполнителя доступа.
  memmove(schedule->scene,(uint8_t *)schedule+16,9);
  memset(schedule->reserve,255,sizeof schedule->reserve);
  return 1;
}

#define SCHED16_MARK_OFFSET 32u
#define SCHED16_MAGIC 0x36314853u
static const uint32_t sched16_mark[2]={SCHED16_MAGIC,~SCHED16_MAGIC};
// Возвращает маркер перехода из сохранённого состояния устройства.
void sched16_restore(const device_s *saved)
{ memcpy(device.reserv+SCHED16_MARK_OFFSET,saved->reserv+SCHED16_MARK_OFFSET,8); }

// Заводская очистка сохраняет отсутствие маркера до завершения таблицы.
void sched16_reset_mark(bool complete)
{
  device.datetimeShedule = 0;
  if (complete) memcpy(device.reserv + SCHED16_MARK_OFFSET, sched16_mark, 8);
  else memset(device.reserv + SCHED16_MARK_OFFSET, 0, 8);
}

// Готовит пустую страницу расписаний для журнальной очистки.
static uint8_t sched16_empty_page(const void *context,uint16_t offset,uint8_t *page)
{
  (void)context;
  uint32_t address=(mem[MB_SHEDULE].start&~4095u)+offset;
  if (address<mem[MB_SHEDULE].start) return w25_read_data(address,page,256);
  memset(page,255,256); return 1;
}

// Завершает переход формата расписаний после стартового восстановления.
bool sched16_boot(void)
{
  if (!memcmp(device.reserv+SCHED16_MARK_OFFSET,sched16_mark,8)) return true;
  device.datetimeShedule=0;
  // Отсутствие маркера означает незавершённый переход; после восстановления
  // журнала очищаем расписания и только затем публикуем маркер.
  if (!flash_exec_read_allowed() ||
      !mem_replace_pages(mem[MB_SHEDULE].start&~4095u,sched16_empty_page,0)) goto bad;
  memcpy(device.reserv+SCHED16_MARK_OFFSET,sched16_mark,8);
  if (user_writeConfigAndDevice()) return true;
bad:
  memset(device.reserv+SCHED16_MARK_OFFSET,0,8);
  table_fault=1;
  return false;
}

