#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
static struct { uint32_t datetime; } device;
static cellLog_s last;
static unsigned added;
static bool log_addEvent(cellLog_s *cl) { last = *cl; added++; return true; }
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
  {LOG_CON_PHOTOFIX, 54, "photo"},
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

  /* Every record carries the event number itself, not a category. */
  device.datetime = 1789600000u;
  CHECK(log_addSystemEvent(LOG_SYS_ERRUPDATE) && last.logType == 5 && last.datetime == 1789600000u && last.index == 1789600000u);
  CHECK(log_addSystemEventID(LOG_SYS_RECVSMS, id) && last.logType == 36 && !memcmp(last.ID, id, 10));
  CHECK(log_addControlEvent(LOG_CON_OUTONBYAPP, id, 77, 3) && last.logType == 52 && last.photoNum == 77 && last.passCnt == 3 && !memcmp(last.ID, id, 10));
  CHECK(log_addControlEvent(LOG_CON_PHOTOFIX, NULL, 0, 0) && last.logType == 54 && last.ID[0] == 0);
  CHECK(added == 4);

  puts("PASS journal: 54 event numbers match the sheet, logType is the event number");
  return 0;
}
