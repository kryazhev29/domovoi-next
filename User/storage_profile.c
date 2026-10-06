/* Capacity from the 0x90 manufacturer/device response: high byte 0x14
 * is 16 Mbit (2 MiB), 0x15 and larger supported parts have room for IX_LOG.
 * Unknown/smaller media keep the mandatory linear ID lookup. */
#include "c/runtime_internal.h"
bool storage_has_index(void)
{
  uint8_t code=(uint8_t)(device.memID>>8);
  return code>=0x15u && code<=0x18u;
}
extern void idindex_impl_scanned(uint32_t sum, uint16_t end);
void idindex_scanned(uint32_t sum, uint16_t end)
{
  if(!storage_has_index()) { return; }
  idindex_impl_scanned(sum,end);
}
extern void idindex_impl_table_commit(uint32_t delta, uint16_t end);
void idindex_table_commit(uint32_t delta, uint16_t end)
{
  if(!storage_has_index()) { return; }
  idindex_impl_table_commit(delta,end);
}
extern int8_t idindex_impl_find(uint32_t base,const uint8_t *id,uint8_t *cell);
int8_t idindex_find(uint32_t base,const uint8_t *id,uint8_t *cell)
{
  if(!storage_has_index()) { return -1; }
  return idindex_impl_find(base,id,cell);
}
extern bool idindex_impl_put(uint16_t index,const uint8_t *id);
bool idindex_put(uint16_t index,const uint8_t *id)
{
  if(!storage_has_index()) { return true; }
  return idindex_impl_put(index,id);
}
extern void idindex_impl_commit(uint32_t delta,uint16_t end);
void idindex_commit(uint32_t delta,uint16_t end)
{
  if(!storage_has_index()) { return; }
  idindex_impl_commit(delta,end);
}
extern void idindex_impl_poll(uint32_t base);
void idindex_poll(uint32_t base)
{
  if(!storage_has_index()) { return; }
  idindex_impl_poll(base);
}

/* runtime_active only observes RAM; gate the operations that touch NOR. */
