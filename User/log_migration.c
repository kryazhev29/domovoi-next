/* One-time, backward copy of the legacy unaligned 96 KiB log. The target is
 * 0x1D0000..0x1E7FFF (same size). Source 0x1CF400 starts 3072 bytes lower;
 * even after dropping at most 24 oldest slots, backward copy cannot overwrite
 * an unread source. The existing sector journal commits each destination. */
#include "c/runtime_internal.h"
#include "log_ring.h"
#include "sector_journal.h"
#include "fast_kernels.h"
#define LEGACY_LOG 0x1CF400u
#define MIG_META 0x1FD000u
#define MIG_MAGIC 0x354D474Cu
extern bool log_valid(const cellLog_s *);
static struct { uint32_t header[8]; uint16_t used,drop,part; } migration;
static bool valid(const uint32_t *h)
{
  return h[0]==LOG_RING_MAGIC && h[1] && h[2]==~h[1] && h[4]==~h[3];
}
static uint8_t page(const void *context,uint16_t offset,uint8_t *data)
{
  (void)context;
  memset(data,255,256);
  for(uint16_t off=0;off<256;off+=32) {
    uint16_t local=offset+off;
    if(!local) memcpy(data,migration.header,32);
    else {
      uint32_t slot=(uint32_t)migration.part*127u+local/32u-1u+migration.drop;
      if(slot<migration.used && !w25_read_data(LEGACY_LOG+32u*slot,data+off,32)) return 0;
    }
  }
  return 1;
}
bool log_ring_migrate(void)
{
  uint32_t meta[4],h[8]; cellLog_s cell; uint16_t used=0,drop=0;
  if(mem[MB_LOG].start!=0x1D0000u || mem[MB_LOG].size!=98304u) return true;
  if(!w25_read_data(MIG_META,(uint8_t *)meta,16)) return false;
  bool active=meta[0]==MIG_MAGIC && meta[2]==~meta[1] && meta[3]==~MIG_MAGIC;
  if(active) {
    used=(uint16_t)meta[1]; drop=(uint16_t)(meta[1]>>16);
    if(!used || used>3072 || drop!=(used>3048?used-3048:0)) return false;
  } else {
    if(!w25_read_data(LEGACY_LOG,(uint8_t *)&cell,32)) return false;
    if(fast_all_ff((uint8_t *)&cell,32)) return true;
    for(uint8_t s=0;s<24;s++) {
      if(!w25_read_data(mem[MB_LOG].start+4096u*s,(uint8_t *)h,32)) return false;
      if(valid(h)) return true; /* Already converted; legacy prefix is obsolete. */
    }
    for(uint16_t i=0;i<3072;i++) {
      if(!w25_read_data(LEGACY_LOG+32u*i,(uint8_t *)&cell,32)) return false;
      if(!fast_all_ff((uint8_t *)&cell,32)) used=i+1;
    }
    if(!used) return true;
    drop=used>3048?used-3048:0;
    meta[0]=MIG_MAGIC; meta[1]=used|((uint32_t)drop<<16); meta[2]=~meta[1]; meta[3]=~MIG_MAGIC;
    if(!w25_erase_sector(MIG_META) || !w25_write_data(MIG_META+4,(uint8_t *)meta+4,12) ||
       !w25_write_data(MIG_META,(uint8_t *)meta,4)) return false;
  }
  migration.used=used; migration.drop=drop;
  uint16_t sectors=(uint16_t)((used-drop+126u)/127u);
  for(uint16_t part=sectors;part;part--) {
    uint32_t target=mem[MB_LOG].start+4096u*(part-1u),previous=0; uint16_t count=0;
    if(!w25_read_data(target,(uint8_t *)h,32)) return false;
    if(valid(h) && h[1]==part) continue; /* sj_recover completed this sector. */
    migration.part=part-1;
    uint16_t first=drop+(part-1u)*127u;
    for(uint16_t i=first;i;i--) {
      if(!w25_read_data(LEGACY_LOG+32u*(i-1u),(uint8_t *)&cell,32)) return false;
      if(log_valid(&cell)) { previous=cell.index; break; }
    }
    for(uint16_t i=first;i<used && i<first+127u;i++) {
      if(!w25_read_data(LEGACY_LOG+32u*i,(uint8_t *)&cell,32)) return false;
      if(log_valid(&cell)) count++;
    }
    migration.header[0]=LOG_RING_MAGIC; migration.header[1]=part; migration.header[2]=~(uint32_t)part;
    migration.header[3]=previous; migration.header[4]=~previous;
    migration.header[5]=part==sectors?0xffffffffu:count|((uint32_t)(uint16_t)~count<<16);
    migration.header[6]=migration.header[7]=0xffffffffu;
    if(!sj_replace_pages(target,page,0)) return false;
  }
  if(!w25_erase_sector(LEGACY_LOG&~4095u) || !w25_erase_sector(MIG_META)) return false;
  return true;
}

uint8_t stage5_sj_destination(uint32_t address)
{
  if(address & 4095u) return 0;
  return (address>=0x41000u && address<0x1C8000u) ||
    address==0x1C9000u || address==0x1CA000u || address==0x1CC000u ||
    (address>=0x1D0000u && address<0x1F5000u);
}
