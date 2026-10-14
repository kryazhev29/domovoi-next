// Доработка: Кряжев А., 2026
// Общая рабочая память и совместимость её владельцев.
// Автоматы получают фиксированные участки без кучи и сохраняют их до завершения.
// USB берёт буфер взаймы; память освобождается после окончания передачи.
#include "work_memory.h"
#include "includes.h"
#include "photo_context.h"
#include <string.h>
typedef struct {
  uint8_t photo[(sizeof(photo_s) + 7u) & ~7u];
  uint8_t shared[2032];
  uint16_t owned;
  uint16_t usb_slot, usb_released;
} work_memory_s;
#pragma data_alignment=8
static work_memory_s work_memory;
static uint8_t usb_reply[523];

#define BIT(slot) ((uint16_t)(1u << ((slot)-1u)))
#define PHOTO BIT(WORK_PHOTO_CONTEXT)
#define PACKET BIT(WORK_PHOTO_PACKET)
#define FW (BIT(WORK_FW_CONTEXT)|BIT(WORK_FW_DATA)|BIT(WORK_CONFIG))
#define UPLOAD (BIT(WORK_UPLOAD_CONTEXT)|BIT(WORK_UPLOAD_PACKET))
static const uint16_t capacities[] = {0,sizeof(photo_s),534,132,128,1024,1024,40,248,32,1040,2032,2028,1024,2032};
// Выбирает фиксированный участок рабочей памяти по владельцу.
static void *address(uint32_t slot)
{
  if(slot==WORK_PHOTO_CONTEXT) return work_memory.photo;
  if(slot==WORK_PHOTO_PACKET) return work_memory.shared+1152;
  if(slot==WORK_FW_DATA || slot==WORK_CONFIG) return work_memory.shared+128;
  if(slot==WORK_UPLOAD_PACKET) return work_memory.shared+40;
  return work_memory.shared;
}
// Возвращает допустимую вместимость рабочего слота.
uint32_t work_capacity(uint32_t slot)
{ return slot>0 && slot<=WORK_TABLE ? capacities[slot] : 0; }
bool work_live(const void *pointer, uint32_t slot)
{
  return slot > 0 && slot <= WORK_TABLE && pointer == address(slot) &&
    (work_memory.owned & BIT(slot));
}
// Прерывание USB владеет дескриптором; рабочей памятью управляет главный цикл.
extern uint8_t USB_Tx_SourceBusy(void);
uint8_t *work_usb_reply(uint16_t length)
{
  return length <= sizeof usb_reply && !USB_Tx_SourceBusy() ? usb_reply : NULL;
}
// Возвращает память после завершения заимствованной USB-передачи.
static void work_usb_collect(void)
{
  if(work_memory.usb_slot && !USB_Tx_SourceBusy()) {
    if(work_memory.usb_released) work_memory.owned&=(uint16_t)~BIT(work_memory.usb_slot);
    work_memory.usb_slot=work_memory.usb_released=0;
  }
}
// Возвращает занятые слоты, учитывая завершённую USB-передачу.

// Выдаёт свободный слот с учётом совместимости остальных владельцев.
void *work_acquire(uint32_t size, uint32_t slot)
{
  uint16_t bit, live, compatible;
  void *p;
  if(!size) { work_usb_collect(); return NULL; }
  if(size>work_capacity(slot)) return NULL;
retry:
  bit=BIT(slot); live=work_memory.owned;
  if(live&bit) goto blocked;
  compatible=PHOTO;
  if(slot==WORK_PHOTO_CONTEXT) compatible=0xffffu;
  else if(slot==WORK_PHOTO_PACKET) compatible|=(uint16_t)~(BIT(WORK_REPLY)|BIT(WORK_SMS)|BIT(WORK_TABLE));
  else {
    if(slot!=WORK_REPLY && slot!=WORK_SMS && slot!=WORK_TABLE) compatible|=PACKET;
    if(slot==WORK_FW_CONTEXT) compatible|=FW;
    if(slot==WORK_FW_DATA) compatible|=BIT(WORK_FW_CONTEXT);
    if(slot==WORK_CONFIG) compatible|=BIT(WORK_FW_CONTEXT);
    if(slot==WORK_UPLOAD_CONTEXT || slot==WORK_UPLOAD_PACKET) compatible|=UPLOAD;
  }
  if(live & (uint16_t)~compatible) goto blocked;
  work_memory.owned=live|bit;
  p=address(slot);
  if(slot==WORK_PHOTO_CONTEXT) memset(p,0,size);
  return p;
blocked:
  // Повторяем выдачу только после возврата памяти завершённой USB-передачей.
  // Обычный путь не требует обслуживания транспортного состояния.
  work_usb_collect();
  if(live!=work_memory.owned) goto retry;
  return NULL;
}
// Освобождает слот сразу либо после завершения передачи USB.
uint8_t work_release(void *pointer, uint32_t slot)
{
  if(slot>0 && slot<=WORK_TABLE && pointer==address(slot)) {
    if(work_memory.usb_slot==slot) {
      work_usb_collect();
      if(work_memory.usb_slot) { work_memory.usb_released=1; return 0; }
    }
    work_memory.owned&=(uint16_t)~BIT(slot); return 1;
  }
  return 0;
}
// Вызывается с запрещёнными прерываниями до публикации дескриптора USB.
// Владелец кадра отдаёт неизменяемый источник взаймы; временный ответ копируется.
// Адрес рабочей памяти без владельца нельзя заимствовать или копировать.
const uint8_t *work_usb_source(const uint8_t *source, uint16_t length)
{
  uint32_t slot, start=(uint32_t)source, end=start+length;
  uint8_t *copy; uint16_t valid=0; uint32_t copy_slot=WORK_REPLY;
  work_usb_collect();
  if(work_memory.usb_slot || end<start) return NULL;
  if (source == usb_reply && length <= sizeof usb_reply) return source;
  for(slot=1;slot<=WORK_TABLE;slot++) {
    uint32_t base=(uint32_t)address(slot);
    if((work_memory.owned&BIT(slot)) && start>=base && end<=base+work_capacity(slot)) {
      valid=1;
      if(slot==WORK_REPLY || slot==WORK_PHOTO_PACKET || slot==WORK_UPLOAD_PACKET ||
         slot==WORK_SERVICE || slot==WORK_IDCLEAR) {
        work_memory.usb_slot=(uint16_t)slot; return source;
      }
    }
  }
  if(!valid && start<(uint32_t)&work_memory+sizeof work_memory && end>(uint32_t)&work_memory) return NULL;
  copy=work_acquire(length,WORK_REPLY);
  if(!copy && length<=work_capacity(WORK_PHOTO_PACKET)) {
    copy_slot=WORK_PHOTO_PACKET; copy=work_acquire(length,copy_slot);
  }
  if(!copy) {
    copy = work_usb_reply(length);
    if (!copy) return NULL;
    memcpy(copy, source, length);
    return copy;
  }
  memcpy(copy,source,length);
  work_memory.usb_slot=(uint16_t)copy_slot; work_memory.usb_released=1; return copy;
}
