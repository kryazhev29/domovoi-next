// Доработка: Кряжев А., 2026
// Фоновая запись таблиц, пересчёт ID и ответы о результате.
// Данные кадра копируются при запуске; внешние указатели не переживают приём.
// CRC читается по 64 Б, до четырёх фрагментов и одной миллисекунды за проход.
enum {
  TABLE_SECTOR=1, TABLE_JOURNAL=2, TABLE_CRC=3,
    TABLE_REPLY=4, TABLE_APPEND_CHECK=5, TABLE_APPEND_BODY=6,
    TABLE_APPEND_BODY_WAIT=7, TABLE_APPEND_COMMIT=8, TABLE_APPEND_COMMIT_WAIT=9
};
#define table_job flash_job.data.table
uint8_t table_fault;

// Подавляет ответ команды для закрытого канала.
void table_runtime_quiet(dataChanel_e channel)
{
  if (flash_job.kind == FX_TABLE && flash_job.result == FX_PENDING && table_job.channel==channel) table_job.quiet=1;
}
// Запрашивает отмену записи таблиц и незавершённого журнала.
void table_runtime_cancel(void)
{
  if (flash_job.kind != FX_TABLE || flash_job.result != FX_PENDING) return;
  if(table_job.cmd==13) idcrc_unknown();
  table_job.cancelled = 1;
  table_job.quiet = 1;
  sj_runtime_cancel();
}
// Накладывает записи команды на фрагмент исходного сектора.
static uint8_t table_chunk(const void *context, uint16_t off, uint8_t *page)
{
  (void)context;
  if (!w25_read_data(table_job.sector+off,page,64)) return 0;
  if (table_job.cmd==16) {
    for (uint16_t i=0;i<64 && off+i<mem[MB_HOLIDAY].size;i++)
      page[i]=off+i<table_job.bytes ? table_job.records[off+i] : 255;
    return 1;
  }
  if (table_job.cmd==14) {
    // Кадр задаёт все расписания; отсутствующие записи стираются.
    uint32_t rel=table_job.sector+off-table_job.start;
    if (rel<SCHED16_TAIL_OFFSET) memset(page,255,64);
    else if (rel==SCHED16_TAIL_OFFSET)
      memcpy(page,table_job.records+table_job.count*SCHED16_RECORD_SIZE,SCHED16_TAIL_SIZE);
  }
  for (uint16_t i=0;i<table_job.batch;i++) {
    const uint8_t *record=table_job.records+(uint32_t)(table_job.position+i)*table_job.cell_size;
    uint16_t index=record[0];
    if (table_job.cmd==13) index|=(uint16_t)record[1]<<8;
    uint32_t address=table_job.start+(uint32_t)(index-1)*table_job.cell_size;
    if (address<table_job.sector+off || address>=table_job.sector+off+64) continue;
    uint16_t local=(uint16_t)(address-table_job.sector-off);
    if (table_job.cmd==13) {
      cellID_s *old=(cellID_s*)(page+local);
      uint8_t first[4],done=old->reserve[6];
      // Источник читается при сравнении и записи; изменение суммы считаем
      // только в первом проходе. Публикуем сумму после завершения всех секторов.
      if (sjr.phase==SJ_PHASE_COMPARE) {
        if (old->index==0xffff || old->reserve[9]==255) table_job.numbers++;
        else table_job.sum^=idindex_hash(index,(uint8_t*)old->ID);
        table_job.sum^=idindex_hash(index,record+2);
        if (index>table_job.last) table_job.last=index;
        if ((uint32_t)index*32>table_job.used) table_job.used=(uint32_t)index*32;
      }
      bool keep=old->index!=0xffff && old->reserve[9]!=255 && !memcmp(old->ID,record+2,10);
      memcpy(first,old->reserve,4);
      memcpy(old,record,table_job.cell_size);
      if (keep) {
        memcpy(old->reserve,first,4);
        old->reserve[6]=done;
      }
      id_set_pass_day(old,table_job.day);
      if (sjr.phase==SJ_PHASE_COMPARE) id_watch_end(id_time_end(old));
    } else memcpy(page+local,record,table_job.cell_size);
  }
  return 1;
}
// Копирует уже проверенные записи кадра и начинает атомарную запись.
bool table_runtime_begin(const uint8_t *ptr, dataChanel_e channel, uint8_t uniq)
{
  uint8_t cmd=ptr[4],mb=cmd==13 ? MB_ID : cmd==14 ? MB_SHEDULE : cmd==15 ? MB_GROUP : MB_HOLIDAY;
  uint16_t size=cmd==13 ? sizeof(cellID_s) : cmd==14 ? SCHED16_RECORD_SIZE :
  cmd==15 ? sizeof(cellGroup_s) : sizeof(cellHoliday_s);
  uint16_t bytes=(uint16_t)ptr[9]*size;
  if (cmd==14) bytes+=SCHED16_TAIL_SIZE;
  if (!flash_exec_idle() || !flash_exec_read_allowed()) return false;
  uint8_t *snapshot=work_acquire(bytes, WORK_TABLE);
  if (!snapshot) return false;
  memcpy(snapshot,ptr+10,bytes);
  flash_exec_args_t args = { snapshot, TABLE_SECTOR };
  if (!flash_exec_begin(FX_TABLE, &args)) {
    work_release(snapshot, WORK_TABLE);
    return false;
  }
  table_job.count=ptr[9];
  table_job.cell_size=size;
  table_job.bytes=bytes;
  memcpy(&table_job.stamp,ptr+5,4);
  table_job.cmd=cmd;
  table_job.mb=mb;
  table_job.channel=channel;
  table_job.uniq=uniq;
  table_job.start=mem[mb].start;
  table_job.day=id_today();
  if (cmd==13) {
    table_job.last=id_cells_end;
    table_job.used=id_used_bytes;
    idindex_runtime_suspend();
    flash_job.step=TABLE_APPEND_CHECK;
    for (uint16_t i=0;i<table_job.count;i++) {
      if (snapshot[32u*i+31]==255) {
        flash_job.step=TABLE_SECTOR;
        id_used_bytes=UINT32_MAX;
      }
      for (uint16_t j=0;j<i;j++)
        if (!memcmp(snapshot+32u*i,snapshot+32u*j,2)) flash_job.step=TABLE_SECTOR;
    }
  }
  return true;
}
// Готовит пересчёт ID после успешной записи таблиц.
static void table_scan_begin(void)
{
  flash_job.step=TABLE_CRC;
  table_job.offset=0;
  table_job.crc=0xffff;
  table_job.crc_limit=table_job.cmd==14 ? sched16_crc_size() : mem[table_job.mb].size;
  // Пересчёт использует отдельные границу чтения и флаг полного сканирования ID.
  table_job.scan_full=0;
  if (table_job.cmd==13) {
    if(table_job.failed || table_job.cancelled) idcrc_unknown();
    if (!table_job.failed && !table_job.cancelled && id_used_bytes!=UINT32_MAX) {
      table_job.crc_limit=table_job.used;
    } else {
      table_job.scan_full=1;
      id_used_bytes=UINT32_MAX;
      id_nextEnd=0;
      table_job.numbers = 0;
      table_job.last = 0;
      table_job.sum = 0;
      table_job.used = 0;
    }
  } else if (table_job.cmd==16 && !table_job.failed && !table_job.cancelled)
    table_job.crc_limit=table_job.bytes;
  if (table_job.cmd==13 && !table_job.scan_full && !table_job.failed && !table_job.cancelled &&
    idcrc_get(&table_job.crc)) flash_job.step=TABLE_REPLY;
}
// Завершает запись, обновляет CRC и готовит ответ команды.
static void table_runtime_finish(void)
{
  uint8_t cmd=table_job.cmd;
  flash_result_t result = table_job.failed ? FX_ERROR : table_job.cancelled ? FX_CANCELLED : FX_OK;
  flash_exec_finish(result);
  if (cmd == 13 && (table_job.scan_full == 2 || result == FX_OK)) {
    if (table_job.scan_full == 2) {
      device.numbersCount = table_job.numbers;
      idindex_scanned(table_job.sum, table_job.last);
    } else {
      device.numbersCount += table_job.numbers;
      idindex_table_commit(table_job.sum, table_job.last);
    }
    id_cells_end = table_job.last;
    id_used_bytes = table_job.used;
  }
  if (result == FX_OK) {
    uint8_t kind,bit;
    if (cmd==13) {
      device.datetimeID=table_job.stamp;
      kind=SHTAMP_ID;
      bit=SYNC_ID;
    } else if (cmd==14) {
      device.datetimeShedule=table_job.stamp;
      kind=SHTAMP_SHEDULE;
      bit=SYNC_SHEDULE;
    } else if (cmd==15) {
      device.datetimeGroup=table_job.stamp;
      kind=SHTAMP_GROUP;
      bit=SYNC_GROUP;
    } else {
      device.datetimeHoliday=table_job.stamp;
      kind=SHTAMP_HOLIDAY;
      bit=SYNC_HOLIDAY;
    }
    sync_packet(bit);
    if (!table_job.quiet) user_send_timeshtamp((shtamp_e)kind,table_job.stamp,table_job.crc,(dataChanel_e)table_job.channel,table_job.uniq);
  } else if (!table_job.quiet)
    user_send_answer_comand(cmd,CMD_RUNERROR,(dataChanel_e)table_job.channel,table_job.uniq);
  work_release(table_job.records, WORK_TABLE);
  table_job.records=0;
}
// Делает шаг журнальной записи, пересчёта либо передачи результата.
// Диспетчер вызывает только FX_TABLE/PENDING.
void table_runtime_poll(uint32_t now)
{
  // Проверяем весь кадр до первой записи во флеш.
  if (flash_job.step==TABLE_APPEND_CHECK) {
    uint16_t index;
    uint8_t old[32];
    memcpy(&index,table_job.records+32u*table_job.position,2);
    if (!w25_read_data(table_job.start+32u*(index-1),old,32)) {
      table_job.failed=1;
      table_scan_begin();
      return;
    }
    if (!fast_all_ff(old,32)) {
      table_job.position=0;
      flash_job.step=TABLE_SECTOR;
      return;
    }
    if (++table_job.position==table_job.count) {
      table_job.position=0;
      flash_job.step=TABLE_APPEND_BODY;
    }
    return;
  }
  if (flash_job.step>=TABLE_APPEND_BODY && flash_job.step<=TABLE_APPEND_COMMIT_WAIT) {
    uint8_t *record=table_job.records+32u*table_job.position;
    uint16_t index;
    uint8_t result;
    memcpy(&index,record,2);
    if (flash_job.step==TABLE_APPEND_BODY_WAIT || flash_job.step==TABLE_APPEND_COMMIT_WAIT) {
      result=w25_async_poll(now);
      if (result==1) return;
      if (result!=2) {
        table_job.failed=1;
        table_scan_begin();
        return;
      }
      if (flash_job.step==TABLE_APPEND_BODY_WAIT) {
        flash_job.step=TABLE_APPEND_COMMIT;
        return;
      }
      table_job.numbers++;
      table_job.sum^=idindex_hash(index,record+2);
      if (index>table_job.last) table_job.last=index;
      if (32u*index>table_job.used) table_job.used=32u*index;
      id_watch_end(id_time_end((cellID_s *)record));
      if (++table_job.position==table_job.count) table_scan_begin();
      else flash_job.step=TABLE_APPEND_BODY;
      return;
    }
    if (flash_job.step==TABLE_APPEND_BODY && table_job.cancelled) {
      table_scan_begin();
      return;
    }
    if (!w25_async_start(table_job.start+32u*(index-1)+(flash_job.step==TABLE_APPEND_COMMIT?31:0),
      record+(flash_job.step==TABLE_APPEND_COMMIT?31:0),flash_job.step==TABLE_APPEND_COMMIT?1:31,now)) {
      table_job.failed=1;
      table_scan_begin();
      return;
    }
    flash_job.step=flash_job.step==TABLE_APPEND_BODY?TABLE_APPEND_BODY_WAIT:TABLE_APPEND_COMMIT_WAIT;
    return;
  }
  if (flash_job.step==TABLE_SECTOR) {
    if (table_job.cancelled) {
      table_scan_begin();
      return;
    }
    uint16_t index=table_job.records[(uint32_t)table_job.position*table_job.cell_size];
    if (table_job.cmd==13) memcpy(&index,table_job.records+(uint32_t)table_job.position*table_job.cell_size,2);
    table_job.sector=table_job.cmd==16 ? table_job.start :
    (table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u;
    table_job.batch=1;
    if (table_job.cmd==16) table_job.batch=table_job.count;
    else while (table_job.position+table_job.batch<table_job.count) {
      const uint8_t *next=table_job.records+(uint32_t)(table_job.position+table_job.batch)*table_job.cell_size;
      index=next[0];
      if (table_job.cmd==13) index|=(uint16_t)next[1]<<8;
      if (((table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u)!=table_job.sector) break;
      table_job.batch++;
    }
    uint8_t prepared=counter_prepare(table_job.sector,now);
    if (prepared==1) return;
    if (prepared!=2 || !sj_runtime_begin(table_job.sector,table_chunk,0)) {
      table_job.failed=1;
      table_scan_begin();
    } else flash_job.step=TABLE_JOURNAL;
    return;
  }
  if (flash_job.step==TABLE_JOURNAL) {
    uint8_t result=sj_runtime_poll(now);
    if (result==1) return;
    sj_runtime_reset();
    if (result!=2) {
      table_job.failed=1;
      if (sj_runtime_blocked()) {
        table_fault=1;
        flash_job.step=TABLE_REPLY;
      }
      else table_scan_begin();
      return;
    }
    table_job.position+=table_job.batch;
    if (table_job.position==table_job.count || table_job.cancelled) table_scan_begin();
    else flash_job.step=TABLE_SECTOR;
    return;
  }
  if (flash_job.step==TABLE_CRC) {
    uint8_t *b=sjr.data, chunks=4;
    // Журнал свободен: используем его буфер.
    uint32_t started=timers_getMillis();
    do {
      uint16_t n=64;
      if (table_job.offset==table_job.crc_limit) {
        uint32_t total=table_job.cmd==14 ? sched16_crc_size() : mem[table_job.mb].size;
        table_job.crc=crc16_ffInit(total-table_job.offset,table_job.crc);
        if (table_job.scan_full) table_job.scan_full = 2;
        flash_job.step=TABLE_REPLY;
        break;
      }
      if (table_job.crc_limit-table_job.offset<n) n=(uint16_t)(table_job.crc_limit-table_job.offset);
      if (!w25_read_data(table_job.start+table_job.offset,b,n) ||
          !counter_overlay(table_job.start+table_job.offset,b,n)) {
        table_fault = 1;
        table_job.failed = 1;
        flash_job.step=TABLE_REPLY;
        return;
      }
      table_job.crc=crc16_calcInit(b,n,table_job.crc);
      if (table_job.scan_full) for (uint16_t i=0;i<n;i+=32) {
        cellID_s *cell=(cellID_s*)(b+i);
        if (!fast_all_ff(b+i,32)) table_job.used=table_job.offset+i+32;
        if (cell->index==0xffff || cell->reserve[9]==255) continue;
        uint16_t index=(uint16_t)((table_job.offset+i)/32+1);
        table_job.numbers++;
        table_job.last=index;
        table_job.sum^=idindex_hash(index,(uint8_t*)cell->ID);
        id_watch_end(id_time_end(cell));
      }
      table_job.offset+=n;
    }
    while (--chunks && timers_getMillis()-started<1u);
    return;
  }
  if (flash_job.step==TABLE_REPLY) {
    if (!table_job.quiet && table_job.channel==CH_SERVER && comm_tx_busy()) {
      if (!table_job.reply_wait) {
        table_job.reply_wait=1;
        table_job.reply_until=now+2000u;
      }
      if ((int32_t)(now-table_job.reply_until)<0) return;
      table_job.quiet=1;
    }
    table_runtime_finish();
  }
}
