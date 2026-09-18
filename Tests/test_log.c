#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
static struct { uint32_t datetime, worktime; } device;
enum { MB_LOG };
static struct { uint32_t start, size; } mem[1] = {{4096, 4096}};
static uint8_t logflash[4096];
static void mem_read(uint32_t a, uint8_t *p, uint32_t n) { memcpy(p, logflash + a - mem[MB_LOG].start, n); }
static cellLog_s last;
static unsigned added;
static cellLog_s written[64];
static bool log_addEvent(cellLog_s *cl) { last = *cl; if (added < 64) written[added] = *cl; added++; return true; }
/* PRODUCTION */

/* Event journal sheet of the protocol table, column A: the number the server reads in logType. */
static const struct { int value, sheet; const char *name; } table[] = {
  {LOG_SYS_DEFCONF, 1, "factory settings"}, {LOG_SYS_GOTOUPDATE, 2, "update mode"},
  {LOG_SYS_RUNUPDATE, 3, "update started"}, {LOG_SYS_ENDUPDATE, 4, "update done"},
  {LOG_SYS_ERRUPDATE, 5, "update error"}, {LOG_SYS_HWERRUPDATE, 6, "wrong HW"},
  {LOG_SYS_ENDIDUPDATE, 7, "ID sync done"}, {LOG_SYS_ENDCONFUPDATE, 8, "settings sync done"},
  {LOG_SYS_ENDGROUPUPDATE, 9, "groups sync done"}, {LOG_SYS_ENDSHEDULEUPDATE, 10, "schedules sync done"},
  {LOG_SYS_ENDHOLLIDAYUPDATE, 11, "holidays sync done"}, {LOG_SYS_NOTGSMREG, 12, "no network"},
  {LOG_SYS_NOTGPRSREG, 13, "no internet"}, {LOG_SYS_ERRCONNECTION, 14, "no traffic"},
  {LOG_SYS_CONNECT1, 15, "server 1"}, {LOG_SYS_CONNECT2, 16, "server 2"},
  {LOG_SYS_ERRCONNECT1, 17, "server 1 error"}, {LOG_SYS_ERRCONNECT2, 18, "server 2 error"},
  {LOG_SYS_FAILCONNECT1, 19, "server 1 lost"}, {LOG_SYS_FAILCONNECT2, 20, "server 2 lost"},
  {LOG_SYS_IN1ON, 21, "input 1 on"}, {LOG_SYS_IN1OFF, 22, "input 1 off"},
  {LOG_SYS_IN2ON, 23, "input 2 on"}, {LOG_SYS_IN2OFF, 24, "input 2 off"},
  {LOG_SYS_AUTOREGEN, 25, "autoregistration on"}, {LOG_SYS_AUTOREGDIS, 26, "autoregistration off"},
  {LOG_SYS_OPENALLEN, 27, "open all on"}, {LOG_SYS_OPENALLDIS, 28, "open all off"},
  {LOG_SYS_UNBLOCKSHEDEN, 29, "unlock by schedule on"}, {LOG_SYS_UNBLOCKSHEDDIS, 30, "unlock by schedule off"},
  {LOG_SYS_UNDLOCKHANDEN, 31, "forced unlock on"}, {LOG_SYS_UNDLOCKHANDDIS, 32, "forced unlock off"},
  {LOG_SYS_RESETONTIMER, 33, "daily reset"}, {LOG_SYS_RESETONCMD, 34, "reset by command"},
  {LOG_SYS_RESETONSHEDULE, 35, "reset by schedule"}, {LOG_SYS_RECVSMS, 36, "SMS in"},
  {LOG_SYS_SENDSMS, 37, "SMS out"}, {LOG_SYS_LOWBALANCE, 38, "low balance"},
  {LOG_SYS_LOWSIGNAL, 39, "low signal"},
  {LOG_CON_IDNOTINBASE, 40, "ID not in base"}, {LOG_CON_IDBLOCKED, 41, "ID blocked"},
  {LOG_CON_IDADDBYSMS, 42, "ID added by SMS"}, {LOG_CON_IDADDBYAUTOREG, 43, "ID added by autoregistration"},
  {LOG_CON_OUTONBYPHONE, 44, "output by phone"}, {LOG_CON_OUTONBYRFID, 45, "output by RFID 485"},
  {LOG_CON_OUTONBYWIEGAND, 46, "output by Wiegand"}, {LOG_CON_OUTONBYRF433, 47, "output by key fob"},
  {LOG_CON_OUTONBYAVTO, 48, "output by plate"}, {LOG_CON_OUTONBYTEST, 49, "server command: admin test"},
  {LOG_CON_OUTONBYAPI, 50, "server command: API"}, {LOG_CON_OUTONBYVOICE, 51, "server command: voice assistant"},
  {LOG_CON_OUTONBYAPP, 52, "server command: mobile app"}, {LOG_CON_RECVIDONREADER, 53, "ID from reader"},
  {LOG_CON_PHOTOFIX, 54, "photo"}, {LOG_CON_IDEXPIRED, 55, "ID use time expired"},
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

  /* Before the server time arrives the clock runs from zero: events wait in memory and are
     written with the real time minus the seconds passed since each of them (18.09.2026). */
  device.datetime = 31; device.worktime = 31;
  CHECK(log_addSystemEvent(LOG_SYS_CONNECT1) && added == 0);
  device.datetime = 40; device.worktime = 40;
  CHECK(log_addControlEvent(LOG_CON_OUTONBYAPP, id, 0, 255) && added == 0);
  device.worktime = 42; log_checkPending(); CHECK(added == 0);
  log_setTime(1789600000u);
  CHECK(added == 2 && device.datetime == 1789600000u);
  CHECK(written[0].logType == 15 && written[0].datetime == 1789600000u - 11 && written[0].index == written[0].datetime);
  CHECK(written[1].logType == 52 && written[1].datetime == 1789600000u - 2 && !memcmp(written[1].ID, id, 10));
  log_setTime(1789600005u); CHECK(added == 2);
  added = 0;

  /* Every record carries the event number itself, not a category. */
  device.datetime = 1789600000u;
  CHECK(log_addSystemEvent(LOG_SYS_ERRUPDATE) && last.logType == 5 && last.datetime == 1789600000u && last.index == 1789600000u);
  CHECK(log_addSystemEventID(LOG_SYS_RECVSMS, id) && last.logType == 36 && !memcmp(last.ID, id, 10));
  CHECK(log_addControlEvent(LOG_CON_OUTONBYAPP, id, 77, 3) && last.logType == 52 && last.photoNum == 77 && last.passCnt == 3 && !memcmp(last.ID, id, 10));
  CHECK(log_addControlEvent(LOG_CON_PHOTOFIX, NULL, 0, 0) && last.logType == 54 && last.ID[0] == 0);
  CHECK(added == 4);

  /* No server time: a full queue, the time limit and a reset write the events as they are. */
  log_timeKnown = 0; log_pendingCount = 0; added = 0; device.worktime = 5; device.datetime = 5;
  for (i = 0; i < LOG_PENDING_MAX; i++) log_addSystemEvent(LOG_SYS_NOTGSMREG);
  CHECK(added == 0 && log_pendingCount == LOG_PENDING_MAX);
  log_addSystemEvent(LOG_SYS_NOTGPRSREG);
  CHECK(added == LOG_PENDING_MAX && log_pendingCount == 1 && written[0].datetime == 5);
  device.worktime = LOG_TIME_WAIT - 1; log_checkPending(); CHECK(added == LOG_PENDING_MAX);
  device.worktime = LOG_TIME_WAIT; log_checkPending(); CHECK(added == LOG_PENDING_MAX + 1 && log_pendingCount == 0);
  log_addSystemEvent(LOG_SYS_NOTGSMREG); CHECK(added == LOG_PENDING_MAX + 2);
  log_timeKnown = 0; log_pendingCount = 0; added = 0; device.worktime = 7;
  log_addSystemEvent(LOG_SYS_RESETONCMD); log_flushPending(); CHECK(added == 1 && last.logType == 34);

  /* Record numbers only grow: two events in one second, after a restart and after the erase. */
  { cellLog_s c; uint32_t at = mem[MB_LOG].start;
    memset(logflash, 0xff, sizeof(logflash)); log_lastIndex = 0;
    memset(&c, 0, sizeof(c)); c.index = 1000; log_uniqueIndex(&c, at); CHECK(c.index == 1000);
    memcpy(logflash, &c, sizeof(c)); at += sizeof(c);
    c.index = 1000; log_uniqueIndex(&c, at); CHECK(c.index == 1001);
    memcpy(logflash + 32, &c, sizeof(c)); at += sizeof(c);
    log_lastIndex = 0; c.index = 999; log_uniqueIndex(&c, at); CHECK(c.index == 1002);
    memset(logflash, 0xff, sizeof(logflash));
    c.index = 1002; log_uniqueIndex(&c, mem[MB_LOG].start); CHECK(c.index == 1003);
    c.index = 5000; log_uniqueIndex(&c, mem[MB_LOG].start); CHECK(c.index == 5000); }

  puts("PASS journal: 54 event numbers match the sheet, logType is the event number, events wait for the server time, record numbers grow");
  return 0;
}
