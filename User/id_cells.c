#include "c/runtime_internal.h"
#include "user_fun.h"
uint16_t stage5_user_getNumbersCount(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  uint16_t cnt = 0, cell = 0, used = 0;
  uint32_t sum = 0;

  id_cells_end = 0;
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем €чейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      cell++;
      /* ѕоследн€€ €чейка, где есть хоть один записанный байт: за ней флеш стЄрта. */
      for (uint8_t b = 0; b < sizeof(cellID_s); b++)
        if (tbuffer[a + b] != 0xff) { used = cell; break; }
      // пуста€ €чейка
      if (c->index == 0xffff || c->reserve[9] == 255) continue;

      cnt++;
      id_cells_end = cell;
      sum ^= idindex_hash(cell, (const uint8_t*)c->ID);
    }
    wdt_reload();
  }
  id_used_bytes = (uint32_t)used * sizeof(cellID_s);
  idindex_scanned(sum, id_cells_end);

  return cnt;
}
bool stage5_user_updateID(cellID_s *c){
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t stored[32];
  uint16_t index;
  uint32_t adr;
  /*  оманда 13 кладЄт €чейку на место еЄ номера: сначала провер€ем это место,
     перебор - только если там друга€ €чейка. */
  if (c->index != 0 && c->index != 0xffff){
    adr = mem[MB_ID].start + (uint32_t)(c->index - 1u) * sizeof(cellID_s);
    if (adr + sizeof(cellID_s) <= endAdr){
      if (!mem_read(adr, stored, 32)) { idcrc_unknown(); return false; }
      memcpy(&index,stored,2);
      if (index == c->index && stored[31] != 255) goto update;
    }
  }
  for (adr = mem[MB_ID].start; adr + sizeof(cellID_s) <= endAdr; adr += sizeof(cellID_s)){
    if (!mem_read(adr, stored, 32)) { idcrc_unknown(); return false; }
      memcpy(&index,stored,2);
    if (index == c->index && stored[31] != 255){
      goto update;
    }
  }
  idcrc_unknown(); return false;
update: {
  bool result=mem_patch(adr,(const uint8_t*)c,sizeof(cellID_s));
  if(!result) idcrc_unknown();
  return result;
}
}
