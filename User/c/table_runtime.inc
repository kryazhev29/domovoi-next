/* Доработка: Кряжев А., 2026. */
/* C oracle for the ASM runtime table command machine; included only by export.
 * CRC: 64-byte reads, <=4 chunks and one millisecond per poll.
 * No incoming pointer survives table_runtime_begin. */
struct {
  uint8_t *records;
  uint32_t stamp, sector, start, offset, sum, used, reply_until;
  uint16_t count, position, batch, cell_size, bytes, day, crc, numbers, last;
  uint8_t phase, mb, cmd, channel, uniq, quiet, cancelled, failed, reply_wait;
} table_job;
uint8_t table_fault;

bool table_runtime_active(void) { return table_job.phase!=0; }
void table_runtime_quiet(dataChanel_e channel)
{
  if (table_job.phase && table_job.channel==channel) table_job.quiet=1;
}
void table_runtime_cancel(void)
{
  if (!table_job.phase) return;
  table_job.cancelled=table_job.quiet=1; sj_runtime_cancel();
}
static uint8_t table_chunk(const void *context, uint16_t off, uint8_t *page)
{
  (void)context;
  if (!w25_read_data(table_job.sector+off,page,64)) return 0;
  if (table_job.cmd==16) {
    for (uint16_t i=0;i<64 && off+i<mem[MB_HOLIDAY].size;i++)
      page[i]=off+i<table_job.bytes ? table_job.records[off+i] : 255;
    return 1;
  }
  if (table_job.cmd==14 && table_job.sector+off==table_job.start+SCHED16_TAIL_OFFSET)
    memcpy(page,table_job.records+table_job.count*SCHED16_RECORD_SIZE,SCHED16_TAIL_SIZE);
  for (uint16_t i=0;i<table_job.batch;i++) {
    const uint8_t *record=table_job.records+(uint32_t)(table_job.position+i)*table_job.cell_size;
    uint16_t index=record[0];
    if (table_job.cmd==13) index|=(uint16_t)record[1]<<8;
    uint32_t address=table_job.start+(uint32_t)(index-1)*table_job.cell_size;
    if (address<table_job.sector+off || address>=table_job.sector+off+64) continue;
    uint16_t local=(uint16_t)(address-table_job.sector-off);
    if (table_job.cell_size>64-local) return 0;
    if (table_job.cmd==13) {
      cellID_s *old=(cellID_s*)(page+local);
      uint8_t first[4],done=old->reserve[6];
      /* The reader runs twice: count only the journal preflight, once per
       * sector. Publish the delta only after all sectors are verified. */
      if (sjr.phase==16) {
        if (old->index==0xffff || old->reserve[9]==255) table_job.numbers++;
        else table_job.sum^=idindex_hash(index,(uint8_t*)old->ID);
        table_job.sum^=idindex_hash(index,record+2);
        if (index>table_job.last) table_job.last=index;
        if ((uint32_t)index*32>table_job.used) table_job.used=(uint32_t)index*32;
      }
      bool keep=old->index!=0xffff && old->reserve[9]!=255 && !memcmp(old->ID,record+2,10);
      memcpy(first,old->reserve,4); memcpy(old,record,table_job.cell_size);
      if (keep) { memcpy(old->reserve,first,4); old->reserve[6]=done; }
      id_set_pass_day(old,table_job.day);
      if (sjr.phase==16) id_watch_end(id_time_end(old));
    } else memcpy(page+local,record,table_job.cell_size);
  }
  return 1;
}
bool table_runtime_begin(const uint8_t *ptr, dataChanel_e channel, uint8_t uniq)
{
  uint8_t cmd=ptr[4],mb=cmd==13 ? MB_ID : cmd==14 ? MB_SHEDULE : cmd==15 ? MB_GROUP : MB_HOLIDAY;
  uint16_t size=cmd==13 ? sizeof(cellID_s) : cmd==14 ? SCHED16_RECORD_SIZE :
                cmd==15 ? sizeof(cellGroup_s) : sizeof(cellHoliday_s);
  uint16_t bytes=(uint16_t)ptr[9]*size;
  if (cmd==14) bytes+=SCHED16_TAIL_SIZE;
  if (!user_storage_command_ready() || !bytes || bytes>2048 || !mem_tables_ready()) return false;
  uint8_t *snapshot=work_acquire(bytes, WORK_TABLE);
  if (!snapshot) return false;
  memcpy(snapshot,ptr+10,bytes);
  memset(&table_job,0,sizeof(table_job));
  table_job.records=snapshot; table_job.count=ptr[9]; table_job.cell_size=size; table_job.bytes=bytes;
  memcpy(&table_job.stamp,ptr+5,4); table_job.cmd=cmd; table_job.mb=mb;
  table_job.channel=channel; table_job.uniq=uniq; table_job.start=mem[mb].start;
  table_job.day=id_today(); table_job.phase=1; flash_claim(FO_TABLE);
  if (cmd==13) {
    table_job.last=id_cells_end; table_job.used=id_used_bytes;
    idindex_runtime_suspend();
    table_job.phase=5;
    for (uint16_t i=0;i<table_job.count;i++) {
      if (snapshot[32u*i+31]==255) { table_job.phase=1; id_used_bytes=UINT32_MAX; }
      for (uint16_t j=0;j<i;j++)
        if (!memcmp(snapshot+32u*i,snapshot+32u*j,2)) table_job.phase=1;
    }
  }
  return true;
}
static void table_scan_begin(void)
{
  table_job.phase=3; table_job.offset=0; table_job.crc=0xffff;
  table_job.sector=table_job.cmd==14 ? sched16_crc_size() : mem[table_job.mb].size;
  /* batch/sector are dead after the journal: reuse as recount flag/read end. */
  table_job.batch=0;
  if (table_job.cmd==13) {
    if (!table_job.failed && !table_job.cancelled && id_used_bytes!=UINT32_MAX) {
      device.numbersCount+=table_job.numbers; id_cells_end=table_job.last;
      id_used_bytes=table_job.used; table_job.sector=table_job.used;
      idindex_table_commit(table_job.sum,table_job.last);
    } else {
      table_job.batch=1; id_used_bytes=UINT32_MAX; id_nextEnd=0;
      table_job.numbers=table_job.last=0; table_job.sum=table_job.used=0;
    }
  } else if (table_job.cmd==16 && !table_job.failed && !table_job.cancelled)
    table_job.sector=table_job.bytes;
  if (table_job.cmd==13 && !table_job.batch && !table_job.failed && !table_job.cancelled &&
      idcrc_get(0,mem[MB_ID].size,&table_job.crc)) table_job.phase=4;
}
static void table_runtime_finish(void)
{
  uint8_t cmd=table_job.cmd;
  if (!table_job.failed && !table_job.cancelled) {
    uint8_t kind,bit;
    if (cmd==13) { device.datetimeID=table_job.stamp; kind=SHTAMP_ID; bit=SYNC_ID; }
    else if (cmd==14) { device.datetimeShedule=table_job.stamp; kind=SHTAMP_SHEDULE; bit=SYNC_SHEDULE; }
    else if (cmd==15) { device.datetimeGroup=table_job.stamp; kind=SHTAMP_GROUP; bit=SYNC_GROUP; }
    else { device.datetimeHoliday=table_job.stamp; kind=SHTAMP_HOLIDAY; bit=SYNC_HOLIDAY; }
    sync_packet(bit);
    if (!table_job.quiet) user_send_timeshtamp((shtamp_e)kind,table_job.stamp,table_job.crc,(dataChanel_e)table_job.channel,table_job.uniq);
  } else if (!table_job.quiet)
    user_send_answer_comand(cmd,CMD_RUNERROR,(dataChanel_e)table_job.channel,table_job.uniq);
  work_release(table_job.records, WORK_TABLE); table_job.records=0; table_job.phase=0; flash_release(FO_TABLE);
}
void table_runtime_poll(uint32_t now)
{
  /* Preflight the complete packet before programming any target. */
  if (table_job.phase==5) {
    uint16_t index; uint8_t old[32];
    memcpy(&index,table_job.records+32u*table_job.position,2);
    if (!w25_read_data(table_job.start+32u*(index-1),old,32)) {
      table_job.failed=1; table_scan_begin(); return;
    }
    if (!fast_all_ff(old,32)) { table_job.position=0; table_job.phase=1; return; }
    if (++table_job.position==table_job.count) { table_job.position=0; table_job.phase=6; }
    return;
  }
  if (table_job.phase>=6 && table_job.phase<=9) {
    uint8_t *record=table_job.records+32u*table_job.position;
    uint16_t index; uint8_t result;
    memcpy(&index,record,2);
    if (table_job.phase==7 || table_job.phase==9) {
      result=w25_async_poll(now); if (result==1) return;
      if (result!=2) { table_job.failed=1; table_scan_begin(); return; }
      if (table_job.phase==7) { table_job.phase=8; return; }
      table_job.numbers++; table_job.sum^=idindex_hash(index,record+2);
      if (index>table_job.last) table_job.last=index;
      if (32u*index>table_job.used) table_job.used=32u*index;
      id_watch_end(id_time_end((cellID_s *)record));
      if (++table_job.position==table_job.count) table_scan_begin(); else table_job.phase=6;
      return;
    }
    if (table_job.phase==6 && table_job.cancelled) { table_scan_begin(); return; }
    if (table_job.phase==6 && !idindex_put(index,record+2)) {
      table_job.failed=1; table_scan_begin(); return;
    }
    if (!w25_async_start(table_job.start+32u*(index-1)+(table_job.phase==8?31:0),
                        record+(table_job.phase==8?31:0),table_job.phase==8?1:31,now)) {
      table_job.failed=1; table_scan_begin(); return;
    }
    table_job.phase++; return;
  }
  if (table_job.phase==1) {
    if (table_job.cancelled) { table_scan_begin(); return; }
    uint16_t index=table_job.records[(uint32_t)table_job.position*table_job.cell_size];
    if (table_job.cmd==13) memcpy(&index,table_job.records+(uint32_t)table_job.position*table_job.cell_size,2);
    table_job.sector=table_job.cmd==16 ? table_job.start :
      (table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u;
    table_job.batch=1;
    if (table_job.cmd==16) table_job.batch=table_job.count;
    else while (table_job.position+table_job.batch<table_job.count) {
      const uint8_t *next=table_job.records+(uint32_t)(table_job.position+table_job.batch)*table_job.cell_size;
      index=next[0]; if (table_job.cmd==13) index|=(uint16_t)next[1]<<8;
      if (((table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u)!=table_job.sector) break;
      table_job.batch++;
    }
    if (!sj_runtime_begin(table_job.sector,table_chunk,0)) { table_job.failed=1; table_scan_begin(); }
    else table_job.phase=2;
    return;
  }
  if (table_job.phase==2) {
    uint8_t result=sj_runtime_poll(now);
    if (result==1) return;
    sj_runtime_reset();
    if (result!=2) {
      table_job.failed=1;
      if (sj_runtime_blocked()) { table_fault=1; table_job.phase=4; }
      else table_scan_begin();
      return;
    }
    table_job.position+=table_job.batch;
    if (table_job.position==table_job.count || table_job.cancelled) table_scan_begin();
    else table_job.phase=1;
    return;
  }
  if (table_job.phase==3) {
    uint8_t *b=sjr.data, chunks=4; /* journal idle; no additional RAM */
    uint32_t started=timers_getMillis();
    do {
      uint16_t n=64;
      if (table_job.offset==table_job.sector) {
        uint32_t total=table_job.cmd==14 ? sched16_crc_size() : mem[table_job.mb].size;
        table_job.crc=crc16_ffInit(total-table_job.offset,table_job.crc);
        if (table_job.batch) {
          device.numbersCount=table_job.numbers; id_cells_end=table_job.last; id_used_bytes=table_job.used;
          idindex_scanned(table_job.sum,table_job.last);
        }
        table_job.phase=4; break;
      }
      if (table_job.sector-table_job.offset<n) n=(uint16_t)(table_job.sector-table_job.offset);
      if (!w25_read_data(table_job.start+table_job.offset,b,n)) {
        table_fault=table_job.failed=1; table_job.phase=4; return;
      }
      table_job.crc=crc16_calcInit(b,n,table_job.crc);
      if (table_job.batch) for (uint16_t i=0;i<n;i+=32) {
        cellID_s *cell=(cellID_s*)(b+i);
        if (!fast_all_ff(b+i,32)) table_job.used=table_job.offset+i+32;
        if (cell->index==0xffff || cell->reserve[9]==255) continue;
        uint16_t index=(uint16_t)((table_job.offset+i)/32+1);
        table_job.numbers++; table_job.last=index;
        table_job.sum^=idindex_hash(index,(uint8_t*)cell->ID);
        id_watch_end(id_time_end(cell));
      }
      table_job.offset+=n;
    } while (--chunks && timers_getMillis()-started<1u);
    return;
  }
  if (table_job.phase==4) {
    if (!table_job.quiet && table_job.channel==CH_SERVER && comm_tx_busy()) {
      if (!table_job.reply_wait) { table_job.reply_wait=1; table_job.reply_until=now+2000u; }
      if ((int32_t)(now-table_job.reply_until)<0) return;
      table_job.quiet=1;
    }
    table_runtime_finish();
  }
}
