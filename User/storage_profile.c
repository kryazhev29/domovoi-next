// Доработка: Кряжев А., 2026
// Доступность индекса определяется ёмкостью установленной W25.
// Код 0x14 означает 2 МиБ: индекс выше 3 МиБ на такой памяти недоступен.
// При неизвестной ёмкости или меньшей памяти остаётся линейный поиск ID.
#include "c/runtime_internal.h"
// Разрешает область индекса только для W25 ёмкостью от 4 МиБ.
bool storage_has_index(void)
{
  uint8_t code=(uint8_t)(device.memID>>8);
  return code>=0x15u && code<=0x18u;
}
extern void idindex_impl_scanned(uint32_t sum, uint16_t end);
// Передаёт итог сканирования индексу доступной ёмкости.
void idindex_scanned(uint32_t sum, uint16_t end)
{
  if(!storage_has_index()) { return; }
  idindex_impl_scanned(sum,end);
}
extern void idindex_impl_table_commit(uint32_t delta, uint16_t end);
// Передаёт сумму изменённых ячеек индексу доступной ёмкости.
void idindex_table_commit(uint32_t delta, uint16_t end)
{
  if(!storage_has_index()) { return; }
  idindex_impl_table_commit(delta,end);
}
extern int8_t idindex_impl_find(uint32_t base,const uint8_t *id,uint8_t *cell);
// Использует индекс либо разрешает вызывающему линейный поиск.
int8_t idindex_find(uint32_t base,const uint8_t *id,uint8_t *cell)
{
  if(!storage_has_index()) { return -1; }
  return idindex_impl_find(base,id,cell);
}
extern void idindex_impl_poll(uint32_t base);
// Продвигает пересборку индекса только на подходящей W25.
void idindex_poll(uint32_t base)
{
  if(!storage_has_index()) { return; }
  idindex_impl_poll(base);
}
