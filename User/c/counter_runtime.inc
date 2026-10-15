// EEPROM по схеме AN2594: данные, затем адрес; перенос последних значений.
// Адреса резервных страниц не пересекаются с базовыми областями 2 МиБ.
#include "../counter.h"
#include "../id_crc_cache.h"
#define CT_PAGE0 0x1FB000u
#define CT_HEAD 12u
#define CT_ROWS ((4096u-CT_HEAD)/10u)
typedef struct { uint8_t value[8]; uint16_t key; } ct_row;
static struct { uint32_t page,generation; uint16_t used; } ct;
static uint32_t ct_address(uint16_t pos) { return ct.page+CT_HEAD+10u*pos; }
static bool ct_read(uint16_t pos,ct_row *row)
{
  return w25_read_data(ct_address(pos),(uint8_t *)row,sizeof(*row));
}
// Проверка номера относится только к данным, прочитанным с NOR.
static bool ct_valid(const ct_row *row) { return row->key<mem[MB_ID].size/32u; }
static int ct_find(uint16_t key,ct_row *row)
{
  for (uint16_t n=ct.used;n;) {
    if (!ct_read(--n,row)) return -1;
    if (row->key==key) return n+1;
  }
  return 0;
}
// Номер записывается последним: оборванное тело не подставляется в ячейку.
static bool ct_append(uint32_t address,const ct_row *row)
{
  return w25_write_data(address,(uint8_t *)row->value,8) &&
    w25_write_data(address+8,(uint8_t *)&row->key,2);
}
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size)
{
  if (!ct.page || address<mem[MB_ID].start || address>=mem[MB_ID].start+mem[MB_ID].size) return true;
  while (size) {
    uint16_t off=(address-mem[MB_ID].start)%32u,n=32u-off;
    ct_row row;
    if (n>size) n=size;
    int found=ct_find((uint16_t)((address-mem[MB_ID].start)/32u),&row);
    if (found<0) return false;
    if (found) for (uint16_t i=0;i<n;i++)
      if (off+i>=21 && off+i<29) data[i]=row.value[off+i-21];
    address+=n;data+=n;size-=n;
  }
  return true;
}
static uint8_t ct_chunk(const void *context,uint16_t offset,uint8_t *data)
{
  uint32_t address=*(const uint32_t *)context+offset;
  return w25_read_data(address,data,SJ_PAGE) && counter_overlay(address,data,SJ_PAGE);
}
// Каждый затронутый сектор базы переносится ровно одной транзакцией SJ.
static bool ct_materialize(void)
{
  idcrc_unknown();
  uint32_t after=0;
  for (;;) {
    uint32_t next=0xffffffffu;
    ct_row row;
    for (uint16_t n=0;n<ct.used;n++) {
      if (!ct_read(n,&row)) return false;
      uint32_t sector=(mem[MB_ID].start+32u*row.key)&~4095u;
      if (ct_valid(&row) && sector>after && sector<next) next=sector;
    }
    if (next==0xffffffffu) return true;
    if (!sj_replace_pages(next,ct_chunk,&next)) return false;
    after=next;
  }
}
// Поколение с инверсией проверяется один раз при старте: прерванное стирание
// прежнего заголовка не может превратить его в более новое поколение.
static bool ct_transfer(bool flush)
{
  uint32_t target=(2u*CT_PAGE0+4096u)-ct.page,h[3]={ct.generation+1u,~(ct.generation+1u),0xffffffffu};
  uint16_t count=0;
  ct_row row,last;
  if (!w25_write_data(target,(uint8_t *)h,8)) return false;
  if (!flush) for (uint16_t n=0;n<ct.used;n++) {
    if (!ct_read(n,&row)) return false;
    if (!ct_valid(&row)) continue;
    int found=ct_find(row.key,&last);
    if (found<0) return false;
    if (found==n+1) {
      if (!ct_append(target+CT_HEAD+10u*count,&row)) return false;
      count++;
    }
  }
  if (flush || count==CT_ROWS) {
    if (!ct_materialize()) return false;
    // Полная страница живых адресов: база опубликована, новая страница пуста.
    if (count && (!w25_erase_sector(target) || !w25_write_data(target,(uint8_t *)h,8))) return false;
    count=0;
  }
  h[2]=0;
  if (!w25_write_data(target+8,(uint8_t *)&h[2],4)) return false;
  uint32_t old=ct.page;
  ct.page=target;ct.generation=h[0];ct.used=count;
  return w25_erase_sector(old);
}
bool counter_boot(void)
{
  uint32_t h[3];
  memset(&ct,0,sizeof(ct));
  for (uint32_t page=CT_PAGE0;page<CT_PAGE0+8192u;page+=4096u) {
    if (!w25_read_data(page,(uint8_t *)h,sizeof(h))) return false;
    if (!h[2] && h[1]==~h[0] && (!ct.page || (int32_t)(h[0]-ct.generation)>0)) {
      ct.page=page;ct.generation=h[0];
    }
  }
  if (!ct.page) {
    ct.page=CT_PAGE0;ct.generation=0;
    h[0]=0;h[1]=0xffffffffu;h[2]=0;
    return w25_erase_sector(CT_PAGE0+4096u) && w25_erase_sector(ct.page) && w25_write_data(ct.page,(uint8_t *)h,sizeof(h));
  }
  // Только невыбранная страница: незавершённый RECEIVE или остаток старой VALID.
  uint32_t other=(2u*CT_PAGE0+4096u)-ct.page;
  for (uint16_t off=0;off<4096u;off+=64) {
    uint8_t b[64];
    if (!w25_read_data(other+off,b,sizeof(b))) return false;
    if (!fast_all_ff(b,sizeof(b))) {
      if (!w25_erase_sector(other)) return false;
      break;
    }
  }
  for (uint16_t n=0;n<CT_ROWS;n++) {
    ct_row row;
    if (!ct_read(n,&row)) return false;
    if (!fast_all_ff((uint8_t *)&row,sizeof(row))) ct.used=n+1;
  }
  return true;
}
bool counter_flush(void)
{
  if (!ct.used || ct_transfer(true)) return true;
  mem_storage_ready=false;
  return false;
}
// 0 — меняется сама ячейка; 1 — записаны лишь поля состояния; -1 — ошибка NOR.
int counter_write(uint32_t address,const cellID_s *cell)
{
  uint8_t old[32];ct_row row;
  if (!ct.page && !counter_boot()) return -1;
  if (!w25_read_data(address,old,32)) return -1;
  if (memcmp(old,cell,21) || memcmp(old+29,(const uint8_t *)cell+29,3)) return 0;
  if (!counter_overlay(address,old,32)) return -1;
  if (!memcmp(old+21,(const uint8_t *)cell+21,8)) return 1;
  if (ct.used==CT_ROWS && !ct_transfer(false)) return -1;
  memcpy(row.value,(const uint8_t *)cell+21,8);
  row.key=(uint16_t)((address-mem[MB_ID].start)/32u);
  uint32_t next=ct_address(ct.used++);
  if (!ct_append(next,&row)) return -1;
  idcrc_counter(address,old+21,row.value);
  return 1;
}
