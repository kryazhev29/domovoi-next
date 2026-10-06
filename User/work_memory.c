/* Доработка: Кряжев А., 2026. Shared 2224-byte storage, without an allocator. */
#include "work_memory.h"
#include "includes.h"
#include <string.h>
typedef struct {
  uint8_t photo[184];
  uint8_t shared[2032];
  uint16_t owned;
  uint16_t usb_slot, usb_released;
  uint8_t reserved[2];
} work_memory_s;
typedef char work_memory_size[(sizeof(work_memory_s)==2224)?1:-1];
#pragma data_alignment=8
static work_memory_s work_memory;

#define BIT(slot) ((uint16_t)(1u << ((slot)-1u)))
#define PHOTO BIT(WORK_PHOTO_CONTEXT)
#define PACKET BIT(WORK_PHOTO_PACKET)
#define FW (BIT(WORK_FW_CONTEXT)|BIT(WORK_FW_DATA)|BIT(WORK_CONFIG))
#define UPLOAD (BIT(WORK_UPLOAD_CONTEXT)|BIT(WORK_UPLOAD_PACKET))
static const uint16_t capacities[] = {0,180,534,1092,128,1024,1024,56,312,36,1040,2032,2028,1024,2032};
static void *address(uint32_t slot)
{
  if(slot==WORK_PHOTO_CONTEXT) return work_memory.photo;
  if(slot==WORK_PHOTO_PACKET) return work_memory.shared+1152;
  if(slot==WORK_FW_DATA || slot==WORK_CONFIG) return work_memory.shared+128;
  if(slot==WORK_UPLOAD_PACKET) return work_memory.shared+40;
  return work_memory.shared;
}
uint32_t work_capacity(uint32_t slot)
{ return slot>0 && slot<=WORK_TABLE ? capacities[slot] : 0; }
/* USB IRQ owns only the transport descriptor. Workspace state stays in main. */
extern uint8_t USB_Tx_SourceBusy(void);
static void work_usb_collect(void)
{
  if(work_memory.usb_slot && !USB_Tx_SourceBusy()) {
    if(work_memory.usb_released) work_memory.owned&=(uint16_t)~BIT(work_memory.usb_slot);
    work_memory.usb_slot=work_memory.usb_released=0;
  }
}
uint16_t work_owned(void) { work_usb_collect(); return work_memory.owned; }
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
  /* Retry only if a completed USB loan released the conflicting owner.
   * The ordinary allocation path pays no transport bookkeeping. */
  work_usb_collect();
  if(live!=work_memory.owned) goto retry;
  return NULL;
}
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
/* Called with PRIMASK set, before publishing the complete USB descriptor.
 * Existing packet owners lend immutable data; transient/stack replies become
 * WORK_REPLY packets. No unowned workspace address can be borrowed or copied. */
const uint8_t *work_usb_source(const uint8_t *source, uint16_t length)
{
  uint32_t slot, start=(uint32_t)source, end=start+length;
  uint8_t *copy; uint16_t valid=0; uint32_t copy_slot=WORK_REPLY;
  work_usb_collect();
  if(work_memory.usb_slot || end<start) return NULL;
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
  if(!copy) return NULL;
  memcpy(copy,source,length);
  work_memory.usb_slot=(uint16_t)copy_slot; work_memory.usb_released=1; return copy;
}
extern bool upload_ack(uint8_t command, dataChanel_e channel);
uint8_t work_upload_ack(uint8_t command, uint8_t channel, void *context)
{
  /* The preflight reservation protects ACK against a busy workspace. It has
   * no initialized payload yet. Release it BEFORE the durable config snapshot.
   * A true ACK result starts no config job; no IRQ touches this workspace, so
   * re-acquisition is guaranteed to have the same addresses as the preflight. */
  work_release(*(void **)context, WORK_UPLOAD_PACKET);
  work_release(context, WORK_UPLOAD_CONTEXT);
  if(!upload_ack(command, (dataChanel_e)channel)) return 0;
  context=work_acquire(36,WORK_UPLOAD_CONTEXT);
  *(void **)context=work_acquire(1040,WORK_UPLOAD_PACKET);
  return 1;
}
