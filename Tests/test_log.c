// Доработка: Кряжев А., 2026
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* ТИПЫ */
static struct { uint32_t datetime, worktime; uint8_t hardwareVersion; uint16_t firmwareVersion; uint32_t buildDate; } device;
enum { MB_LOG };
static struct { uint32_t start, size; } mem[1] = {{4096, 4096}};
static uint8_t logflash[4096];
// Проверяет сценарий mem_read по независимым данным модуля.
static bool mem_read(uint32_t a, uint8_t *p, uint32_t n) { memcpy(p, logflash + a - mem[MB_LOG].start, n); return true; }
static cellLog_s last;
static unsigned added,fail_write;
static cellLog_s written[64];
// Проверяет сценарий log_addEvent по независимым данным модуля.
static bool log_addEvent(cellLog_s *cl) { if(fail_write==1||(fail_write==2&&added>=1))return false;last = *cl; if (added < 64) written[added] = *cl; added++; return true; }
static bool id_endScan; /* Используем сроки из user_fun.c, пересчитываемые после установки нового времени. */
/* ИСХОДНИК */

/* Таблица событий протокола, колонка A: номер, который сервер читает в logType. */
static const struct { int value, sheet; const char *name; } table[] = {
  {LOG_SYS_DEFCONF, 0, "factory settings"},
  {LOG_SYS_ENDIDUPDATE, 6, "ID sync done"}, {LOG_SYS_ENDCONFUPDATE, 7, "settings sync done"},
  {LOG_SYS_ENDGROUPUPDATE, 8, "groups sync done"}, {LOG_SYS_ENDSHEDULEUPDATE, 9, "schedules sync done"},
  {LOG_SYS_ENDHOLLIDAYUPDATE, 10, "holidays sync done"}, {LOG_SYS_NOTGSMREG, 11, "no network"},
  {LOG_SYS_NOTGPRSREG, 12, "no internet"}, {LOG_SYS_ERRCONNECTION, 13, "no traffic"},
  {LOG_SYS_CONNECT1, 14, "server 1"}, {LOG_SYS_CONNECT2, 15, "server 2"},
  {LOG_SYS_ERRCONNECT1, 16, "server 1 error"}, {LOG_SYS_ERRCONNECT2, 17, "server 2 error"},
  {LOG_SYS_FAILCONNECT1, 18, "server 1 lost"}, {LOG_SYS_FAILCONNECT2, 19, "server 2 lost"},
  {LOG_SYS_IN1ON, 20, "input 1 on"}, {LOG_SYS_IN1OFF, 21, "input 1 off"},
  {LOG_SYS_IN2ON, 22, "input 2 on"}, {LOG_SYS_IN2OFF, 23, "input 2 off"},
  {LOG_SYS_AUTOREGEN, 24, "autoregistration on"}, {LOG_SYS_AUTOREGDIS, 25, "autoregistration off"},
  {LOG_SYS_OPENALLEN, 26, "open all on"}, {LOG_SYS_OPENALLDIS, 27, "open all off"},
  {LOG_SYS_UNBLOCKSHEDEN, 28, "unlock by schedule on"}, {LOG_SYS_UNBLOCKSHEDDIS, 29, "unlock by schedule off"},
  {LOG_SYS_UNDLOCKHANDEN, 30, "forced unlock on"}, {LOG_SYS_UNDLOCKHANDDIS, 31, "forced unlock off"},
  {LOG_SYS_RESETONTIMER, 32, "daily reset"}, {LOG_SYS_RESETONCMD, 33, "reset by command"},
  {LOG_SYS_RESETONSHEDULE, 34, "reset by schedule"}, {LOG_SYS_RECVSMS, 35, "SMS in"},
  {LOG_SYS_SENDSMS, 36, "SMS out"}, {LOG_SYS_LOWBALANCE, 37, "low balance"},
  {LOG_SYS_LOWSIGNAL, 38, "low signal"},
  {LOG_SYS_RESETONPOWER, 39, "power reset"},
  {LOG_CON_IDNOTINBASE, 65, "ID not in base"}, {LOG_CON_IDBLOCKED, 66, "ID blocked"},
  {LOG_CON_IDADDBYSMS, 67, "ID added by SMS"}, {LOG_CON_IDADDBYAUTOREG, 68, "ID added by autoregistration"},
  {LOG_CON_OUTONBYPHONE, 69, "output by phone"}, {LOG_CON_OUTONBYRFID, 70, "output by RFID 485"},
  {LOG_CON_OUTONBYWIEGAND, 71, "output by Wiegand"}, {LOG_CON_OUTONBYRF433, 72, "output by key fob"},
  {LOG_CON_OUTONBYAVTO, 73, "output by plate"}, {LOG_CON_OUTONBYTEST, 74, "server command: admin test"},
  {LOG_CON_OUTONBYAPI, 75, "server command: API"}, {LOG_CON_OUTONBYVOICE, 76, "server command: voice assistant"},
  {LOG_CON_OUTONBYAPP, 77, "server command: mobile app"}, {LOG_CON_RECVIDONREADER, 78, "ID from reader"},
  {LOG_CON_PASSLIMIT, 79, "passes of the day used up"},
  {LOG_CON_PHOTOFIX, 129, "photo"},
  {LOG_CON_PASSCOUNT, 131, "pass counter"},
  {LOG_CON_IDTIMEEND, 138, "ID use time ended"}, {LOG_CON_IDTIMESTART, 139, "ID use time started"},
  {LOG_FW_START, 132, "FW download started"}, {LOG_FW_FAIL, 133, "FW download failed"},
  {LOG_FW_TIMEOUT, 134, "FW download time limit"}, {LOG_FW_INSTALL, 135, "FW downloaded, installing"},
  {LOG_FW_DONE, 136, "FW update done"}, {LOG_FW_ROLLBACK, 137, "FW rollback"},
};

int main(void)
{
  unsigned i;
  char id[10] = {'8','9','0','0','0','0','0','0','0','1'};

  for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    if (table[i].value != table[i].sheet) {
      fprintf(stderr, "FAIL %s: code %d, sheet %d\n", table[i].name, table[i].value, table[i].sheet);
      exit(1);
    }

  /* До серверного времени часы идут от нуля: события ждут в ОЗУ, затем получают истинное время с учётом прошедших секунд. */
  device.datetime = 31; device.worktime = 31;
  CHECK(log_addSystemEvent(LOG_SYS_CONNECT1) && added == 0);
  device.datetime = 40; device.worktime = 40;
  CHECK(log_addControlEvent(LOG_CON_OUTONBYAPP, id, 255) && added == 0);
  device.worktime = 42; log_checkPending(); CHECK(added == 0);
  log_setTime(1789600000u);
  CHECK(added == 2 && device.datetime == 1789600000u);
  CHECK(written[0].logType == 14 && written[0].datetime == 1789600000u - 11 && written[0].index == written[0].datetime);
  CHECK(written[1].logType == 77 && written[1].datetime == 1789600000u - 2 && !memcmp(written[1].ID, id, 10));
  log_setTime(1789600005u); CHECK(added == 2);
  added = 0;

  /* Запись содержит номер самого события, а не его категорию. */
  device.datetime = 1789600000u;
  CHECK(log_addSystemEvent(LOG_SYS_ENDIDUPDATE) && last.logType == 6 && last.datetime == 1789600000u && last.index == 1789600000u);
  CHECK(log_addSystemEventID(LOG_SYS_RECVSMS, id) && last.logType == 35 && !memcmp(last.ID, id, 10));
  CHECK(log_addControlEvent(LOG_CON_OUTONBYAPP, id, 3) && last.logType == 77 && last.arg2 == 0 && last.passCnt == 3 && !memcmp(last.ID, id, 10));
  /* Номер фото занимает первые 4 Б аргумента 1; аргумент 2 зарезервирован по E533/E534. */
  { static const char num[10] = {0x78,0x56,0x34,0x12};
    CHECK(log_addPhotoEvent(0x12345678u) && last.logType == 129 && !memcmp(last.ID, num, 10) && last.arg2 == 0 && last.passCnt == 0); }
  /* События прошивки: аргумент 1 содержит аппаратную версию, версию ПО и сборку; аргумент 2 — код причины. */
  { static const char ver[10] = {(char)139, 71, 0, 0x78, 0x56, 0x34, 0x12};
    device.hardwareVersion = 139; device.firmwareVersion = 71; device.buildDate = 0x12345678u;
    CHECK(log_addUpdateEvent(LOG_FW_FAIL, 8) && last.logType == 133 && !memcmp(last.ID, ver, 10) && last.arg2 == 8); }
  CHECK(added == 5);

  /* Без серверного времени полная очередь, предельное ожидание и сброс записывают события как есть. */
  log_timeKnown = 0; log_pendingCount = 0; added = 0; device.worktime = 5; device.datetime = 5;
  for (i = 0; i < LOG_PENDING_MAX; i++) log_addSystemEvent(LOG_SYS_NOTGSMREG);
  CHECK(added == 0 && log_pendingCount == LOG_PENDING_MAX);
  log_addSystemEvent(LOG_SYS_NOTGPRSREG);
  CHECK(added == LOG_PENDING_MAX && log_pendingCount == 1 && written[0].datetime == 5);
  device.worktime = LOG_TIME_WAIT - 1; log_checkPending(); CHECK(added == LOG_PENDING_MAX);
  device.worktime = LOG_TIME_WAIT; log_checkPending(); CHECK(added == LOG_PENDING_MAX + 1 && log_pendingCount == 0);
  log_addSystemEvent(LOG_SYS_NOTGSMREG); CHECK(added == LOG_PENDING_MAX + 2);
  log_timeKnown = 0; log_pendingCount = 0; added = 0; device.worktime = 7;
  log_addSystemEvent(LOG_SYS_RESETONCMD); log_flushPending(); CHECK(added == 1 && last.logType == 33);

  /* Номера растут при двух событиях за секунду, после перезапуска и после стирания. */
  { cellLog_s c; uint32_t at = mem[MB_LOG].start;
    memset(logflash, 0xff, sizeof(logflash)); log_lastIndex = 0;
    memset(&c, 0, sizeof(c)); c.index = 1000; log_uniqueIndex(&c, at); CHECK(c.index == 1000);
    memcpy(logflash, &c, sizeof(c)); log_lastIndex=c.index; at += sizeof(c);
    c.index = 1000; log_uniqueIndex(&c, at); CHECK(c.index == 1001);
    memcpy(logflash + 32, &c, sizeof(c)); log_lastIndex=c.index; at += sizeof(c);
    log_lastIndex = 0; c.index = 999; log_uniqueIndex(&c, at); CHECK(c.index == 1002);log_lastIndex=c.index;
    memset(logflash, 0xff, sizeof(logflash));
    c.index = 1002; log_uniqueIndex(&c, mem[MB_LOG].start); CHECK(c.index == 1003);
    c.index = 5000; log_uniqueIndex(&c, mem[MB_LOG].start); CHECK(c.index == 5000); }

  log_timeKnown=0;log_pendingCount=0;added=0;device.worktime=10;device.datetime=10;
  CHECK(log_addSystemEvent(LOG_SYS_NOTGSMREG));fail_write=1;log_setTime(1000);
  CHECK(log_pendingCount==1&&!added);CHECK(log_pending[0].datetime==1000);
  log_setTime(2000);CHECK(log_pending[0].datetime==1000&&log_pendingCount==1);
  fail_write=0;log_flushPending();CHECK(!log_pendingCount&&added==1&&last.datetime==1000);
  log_timeKnown=0;log_pendingCount=0;added=0;device.worktime=10;
  CHECK(log_addSystemEvent(LOG_SYS_NOTGSMREG));device.worktime=11;CHECK(log_addSystemEvent(LOG_SYS_NOTGPRSREG));
  fail_write=2;log_setTime(1000);CHECK(added==1&&log_pendingCount==1&&log_pending[0].datetime==1000);
  fail_write=0;log_flushPending();CHECK(added==2&&!log_pendingCount&&written[0].datetime==999&&written[1].datetime==1000);
  log_timeKnown=0;log_pendingCount=0;device.worktime=10;fail_write=1;
  for(i=0;i<LOG_PENDING_MAX;i++)CHECK(log_addSystemEvent(LOG_SYS_NOTGSMREG));
  CHECK(!log_addSystemEvent(LOG_SYS_NOTGPRSREG)&&log_pendingCount==LOG_PENDING_MAX);
  fail_write=0;
  puts("PASS journal: 55 event numbers match the sheet, logType is the event number, events wait for the server time, record numbers grow");
  return 0;
}
