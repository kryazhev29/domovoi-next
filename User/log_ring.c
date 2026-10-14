// Доработка: Кряжев А., 2026
// Кольцевой журнал: 24 сектора по 127 записей и заголовок.
// Заголовок хранит поколение, число записей и намерение удалить старый хвост.
// W25 проверяет запись; после обрыва целостность определяется заголовком и CRC.
#include "usb_log.h"
#include "c/runtime_internal.h"
#include "log_ring.h"
#include "user_fun.h"
#include "fast_kernels.h"
enum {
  LOG_INIT=1, LOG_PREPARE=2, LOG_BODY=3,
    LOG_COMMIT=4, LOG_PUBLISH=5, LOG_REPAIR=6,
    LOG_CLOSE=10, LOG_INTENT=11, LOG_ERASE=12,
    LOG_HEADER=13, LOG_HEADER_COMMIT=14, LOG_ROTATED=15,
    LOG_WRITE_WAIT=16, LOG_RETRY_INIT=17, LOG_RETRY_REPAIR=18
};
extern bool log_blank(const cellLog_s *);
extern uint32_t log_lastIndex;
extern uint8_t store_save_requested;
// Проверяет CRC и метку записи на границе чтения из флеша.
bool log_valid(const cellLog_s *cell)
{
  // Буферы записей выровнены по слову: поля заголовка читаются без копирования.
  const uint32_t *words=(const uint32_t *)cell;
  uint32_t value=words[6];
  return words[0]!=0xffffffffu && words[7]==0x32474F4Cu &&
    (uint16_t)value==(uint16_t)~(value>>16) && (uint16_t)value==crc16_calc((uint8_t *)cell,24);
}
#define log_job flash_job.data.log
static struct {
  uint32_t live,maximum,header[8],word;
  uint8_t seq[24],count[24];
  uint8_t position,low,high,scan;
  uint8_t head,target,forced,part,init,ready,after;
} lr;
#define NONE 255u
// Возвращает начало сектора кольца.
static uint32_t sector(uint8_t n)
{
  return mem[MB_LOG].start+4096u*n;
}
// Проверяет поколение и инверсии заголовка после сброса.
static bool header_valid(const uint32_t *h)
{
  return h[0]==LOG_RING_MAGIC && h[1] && h[1]!=0xffffffffu &&
    h[2]==~h[1] && h[4]==~h[3];
}
// Проверяет пару из 16-битного значения и его инверсии.
static bool pair_valid(uint32_t v)
{
  return (uint16_t)v==(uint16_t)~(v>>16);
}
// Упаковывает 16-битное значение с инверсией для записи во флеш.
static uint32_t pair(uint16_t v)
{
  return v|((uint32_t)(uint16_t)~v<<16);
}
// Полная очистка уже проверена STORE: публикуем пустой журнал без повторного erase.
void log_ring_cleared(void)
{
  memset(&lr, 0, sizeof(lr));
  lr.head = NONE;
  lr.forced = NONE;
  lr.ready = 1;
  device.logsCount = 0;
  device.status.readyLogData = 0;
  device.newAddressLog = mem[MB_LOG].start + 32;
}
// Требует повторного чтения заголовков перед следующей операцией.
void log_ring_invalidate(void)
{
  lr.init = 0;
  lr.ready = 0;
}
// Восстанавливает кольцо либо форматирует старую область по одной операции.
uint8_t log_ring_init_step(void)
{
  uint32_t h[8];
  cellLog_s cell;
  if(lr.ready) return 2;
  if(!lr.init) {
    memset(&lr,0,sizeof(lr));
    lr.head = NONE;
    lr.forced = NONE;
    lr.init=1;
  }
  if(lr.init==1) {
    if(!w25_read_data(sector(lr.part),(uint8_t *)h,32)) return 3;
    if(header_valid(h)) {
      lr.live|=1u<<lr.part;
      lr.seq[lr.part]=(uint8_t)h[1];
      if(pair_valid(h[5]) && (uint16_t)h[5]<=127) lr.count[lr.part]=(uint16_t)h[5];
      if(h[1]>lr.maximum) {
        lr.maximum=h[1];
        lr.head=lr.part;
        memcpy(lr.header,h,32);
      }
    }
    if(++lr.part<24) return 1;
    if(lr.head==NONE) {
      lr.part=0;
      lr.scan=0;
      lr.init=4;
      return 1;
    }
    // Намерение удаляет старый хвост даже при целом заголовке после обрыва стирания.
    if(pair_valid(lr.header[6]) && (uint16_t)lr.header[6]<24 && (uint16_t)lr.header[6]!=lr.head) {
      lr.forced=(uint8_t)lr.header[6];
      lr.live&=~(1u<<lr.forced);
      lr.count[lr.forced]=0;
    }
    lr.low=0;
    lr.high=127;
    lr.init=2;
    return 1;
  }
  // До окончания очистки заголовков нет: сброс повторяет всё форматирование.
  if(lr.init==4) {
    if(lr.scan) {
      uint8_t r=w25_async_poll(timers_getMillis());
      if(r==1) return 1;
      lr.scan=0;
      if(r!=2) return 3;
      if(++lr.part<24) return 1;
      log_ring_reset();
      store_save_requested=1;
      lr.position=0;
      goto ready;
    }
    if(!w25_async_start(sector(lr.part),0,0,timers_getMillis())) return 3;
    lr.scan=1;
    return 1;
  }
  if(lr.init==2) {
    if(lr.low<lr.high) {
      uint16_t middle=(uint16_t)((lr.low+lr.high)/2);
      if(!w25_read_data(sector(lr.head)+32u+32u*middle,(uint8_t *)&cell,32)) return 3;
      if(log_blank(&cell)) lr.high=middle;
      else lr.low=middle+1;
      return 1;
    }
    lr.position=lr.low;
    lr.scan=0;
    lr.count[lr.head]=0;
    if(log_lastIndex<lr.header[3]) log_lastIndex=lr.header[3];
    lr.init=3;
    return 1;
  }
  // Закрытые сектора уже имеют счётчики; головной проверяем, пропуская оборванные записи.
  if(lr.scan<lr.position) {
    if(!w25_read_data(sector(lr.head)+32u+32u*lr.scan++,(uint8_t *)&cell,32)) return 3;
    if(log_blank(&cell)) lr.position=lr.scan-1u;
    if(log_valid(&cell)) {
      lr.count[lr.head]++;
      if(log_lastIndex<cell.index) log_lastIndex=cell.index;
    }
    return 1;
  }
  ready:
  device.logsCount=0;
  for(uint8_t i=0;i<24;i++) device.logsCount+=lr.count[i];
  device.newAddressLog=sector(lr.head==NONE?0:lr.head)+32u+32u*lr.position;
  device.status.readyLogData=device.logsCount!=0;
  lr.ready=1;
  return 2;
}
// Живут не более 24 соседних поколений: в ОЗУ хватает младшего байта, полный номер остаётся во флеше.
static uint8_t next_sector(uint8_t current)
{
  uint8_t found=NONE;
  uint8_t age=current==NONE?255u:(uint8_t)(lr.maximum-lr.seq[current]);
  for(uint8_t i=0;i<24;i++) {
    uint8_t candidate=(uint8_t)(lr.maximum-lr.seq[i]);
    if((lr.live&(1u<<i)) && candidate<age &&
      (found==NONE || candidate>(uint8_t)(lr.maximum-lr.seq[found]))) found=i;
  }
  return found;
}
// Возвращает начало самого старого живого сектора.
uint32_t log_ring_first(void)
{
  uint8_t n=next_sector(NONE);
  return n==NONE?0:sector(n)+32;
}
// Продвигает адрес, полученный из кольца, до следующей записи.
uint32_t log_ring_next(uint32_t address)
{
  uint8_t n=(uint8_t)((address-mem[MB_LOG].start)/4096u);
  uint32_t offset=(address-mem[MB_LOG].start)&4095u;
  if(offset+32u<4096u && (n!=lr.head || offset/32u<lr.position)) return address+32u;
  n=next_sector(n);
  return n==NONE?0:sector(n)+32u;
}
// Завершает стартовую операцию с обслуживанием сторожевого таймера.
static uint16_t log_wait(void)
{
  uint8_t r;
  do {
    wdt_reload();
    r=log_ring_init_step();
  }
  while(r==1);
  return r==2?device.logsCount:0xffff;
}
// Восстанавливает кольцо и возвращает число действующих записей.
uint16_t log_getRecordsCount(void)
{
  log_ring_invalidate();
  return log_wait();
}

// Копирует событие и занимает флеш до окончания записи.
void log_async_begin(const cellLog_s *cell)
{
  flash_exec_args_t args = { (void *)cell, LOG_INIT };
  flash_exec_begin(FX_LOG, &args);
}
// Назначает повтор после ошибки, перечитывая кольцо при неясной ротации.
static uint8_t fail(uint32_t now,bool rotation)
{
  log_job.retry_at=now+1000u;
  if(rotation) {
    log_ring_invalidate();
    flash_job.step=LOG_RETRY_INIT;
  } else flash_job.step=LOG_RETRY_REPAIR;
  return 3;
}
// Запускает проверяемую команду W25, сохраняя её следующий шаг.
static bool start(uint32_t a,const uint8_t *p,uint16_t n,uint32_t now,uint8_t after)
{
  if(!w25_async_start(a,p,n,now)) return false;
  log_job.after = after;
  flash_job.step = LOG_WRITE_WAIT;
  return true;
}
// Выбирает свободный сектор либо самый старый хвост кольца.
static void target_select(void)
{
  uint8_t n=lr.forced;
  if(n==NONE) {
    for(n=0;n<24;n++) if(!(lr.live&(1u<<n))) break;
    if(n==24) n=next_sector(NONE);
  }
  lr.target=n;
}
// Публикует только полностью проверенную запись и освобождает владельца журнала.
static uint8_t log_publish(void)
{
  flash_exec_finish(FX_OK);
  log_lastIndex=log_job.cell.index;
  lr.position++;
  lr.count[lr.head]++;
  device.logsCount++;
  device.newAddressLog=log_job.address+32;
  device.status.readyLogData=1;
  if (usb_log_on(4)) usb_log(32, (const uint8_t *)&log_job.cell, sizeof(log_job.cell));
  return 2;
}
// Записывает событие или поворачивает кольцо одной командой за проход.
uint8_t log_async_step(uint32_t now)
{
  cellLog_s actual;
  uint8_t result;
  uint16_t crc;
  if (flash_job.kind != FX_LOG) return 0;
  if (flash_job.result != FX_PENDING) return flash_job.result == FX_OK ? 2 : 3;
  if (flash_job.step == LOG_RETRY_INIT || flash_job.step == LOG_RETRY_REPAIR) {
    if ((int32_t)(now - log_job.retry_at) < 0) return 3;
    flash_job.step = flash_job.step == LOG_RETRY_INIT ? LOG_INIT : LOG_REPAIR;
  }
  if (flash_job.step == LOG_WRITE_WAIT) {
    result=w25_async_poll(now);
    if(result==1) return 1;
    if(result!=2) return fail(now,log_job.after>=LOG_CLOSE);
    flash_job.step=log_job.after;
    return 1;
  }
  if(!w25_read_available() && !(flash_job.step==LOG_INIT && lr.init==4 && lr.scan)) return 1;
  switch(flash_job.step) {
  case LOG_INIT:
    result=log_ring_init_step();
    if(result==3) return fail(now,true);
    if(result!=2) return 1;
    flash_job.step=LOG_PREPARE;
    return 1;
  case LOG_PREPARE:
    if(lr.head==NONE || lr.position==127 || lr.forced!=NONE) {
      target_select();
      flash_job.step=lr.head==NONE?LOG_ERASE:LOG_CLOSE;
      return 1;
    }
    log_job.address=sector(lr.head)+32u+32u*lr.position;
    // Только владелец журнала продвигает свободное место; результат записи проверяет W25.
    if(log_lastIndex && log_job.cell.index<=log_lastIndex) log_job.cell.index=log_lastIndex+1u;
    crc=crc16_calc((uint8_t *)&log_job.cell,24);
    memcpy(log_job.cell.reserve,&crc,2);
    crc=(uint16_t)~crc;
    memcpy(log_job.cell.reserve+2,&crc,2);
    lr.word=0x32474f4cu;
    memcpy(log_job.cell.reserve+4,&lr.word,4);
    flash_job.step=LOG_BODY;
    return 1;
  case LOG_BODY:
    if(!start(log_job.address,(uint8_t *)&log_job.cell,28,now,LOG_COMMIT)) return fail(now,false);
    return 1;
  case LOG_COMMIT:
    if(!start(log_job.address+28,(uint8_t *)&log_job.cell+28,4,now,LOG_PUBLISH)) return fail(now,false);
    return 1;
  case LOG_PUBLISH: return log_publish();
  case LOG_REPAIR:
    if(!w25_read_data(log_job.address,(uint8_t *)&actual,32)) return fail(now,false);
    // После отказа W25 проверяем, успела ли целиком записаться исходная запись.
    if(!memcmp(&actual,&log_job.cell,32) && actual.index!=0xffffffffu) return log_publish();
    for(uint8_t i=0;i<32;i++) if((((uint8_t *)&actual)[i]&((uint8_t *)&log_job.cell)[i])!=((uint8_t *)&log_job.cell)[i]) {
      if(log_valid(&actual)) {
        if(log_lastIndex<actual.index) log_lastIndex=actual.index;
        lr.count[lr.head]++;
        device.logsCount++;
      }
      lr.position++;
      flash_job.step=LOG_PREPARE;
      return 1;
    }
    flash_job.step=LOG_BODY;
    return 1;
  case LOG_CLOSE:
    lr.word=pair(lr.count[lr.head]);
    if(!start(sector(lr.head)+20,(uint8_t *)&lr.word,4,now,LOG_INTENT)) return fail(now,true);
    return 1;
  case LOG_INTENT:
    lr.word=pair(lr.target);
    if(!start(sector(lr.head)+24,(uint8_t *)&lr.word,4,now,LOG_ERASE)) return fail(now,true);
    return 1;
  case LOG_ERASE:
    if(!start(sector(lr.target),0,0,now,LOG_HEADER)) return fail(now,true);
    device.logsCount-=lr.count[lr.target];
    lr.count[lr.target]=0;
    lr.live&=~(1u<<lr.target);
    return 1;
  case LOG_HEADER:
    lr.header[0]=LOG_RING_MAGIC;
    lr.header[1]=lr.maximum+1u;
    lr.header[2]=~lr.header[1];
    lr.header[3]=log_lastIndex;
    lr.header[4]=~log_lastIndex;
    lr.header[5]=lr.header[6]=lr.header[7]=0xffffffffu;
    if(!start(sector(lr.target)+4,(uint8_t *)lr.header+4,16,now,LOG_HEADER_COMMIT)) return fail(now,true);
    return 1;
  case LOG_HEADER_COMMIT:
    if(!start(sector(lr.target),(uint8_t *)lr.header,4,now,LOG_ROTATED)) return fail(now,true);
    return 1;
  case LOG_ROTATED:
    lr.head=lr.target;
    lr.maximum=lr.header[1];
    lr.seq[lr.head]=(uint8_t)lr.maximum;
    lr.live|=1u<<lr.head;
    lr.position=0;
    lr.count[lr.head]=0;
    lr.forced=NONE;
    flash_job.step=LOG_PREPARE;
    return 1;
  default: return fail(now,true);
  }
}
// Записывает стартовое событие синхронно тем же автоматом.
bool log_addEvent(cellLog_s *cell)
{
  if(!flash_exec_read_allowed()) return false;
  log_async_begin(cell);
  if (flash_job.kind != FX_LOG || flash_job.result != FX_PENDING) return false;
  while (flash_job.result == FX_PENDING) {
    wdt_reload();
    uint8_t r=log_async_step(timers_getMillis());
    if (r == 3) {
      flash_exec_finish(FX_ERROR);
      log_ring_invalidate();
    }
  }
  bool ok = flash_job.result == FX_OK;
  if (ok) cell->index=log_job.cell.index;
  flash_exec_take_result();
  return ok;
}
