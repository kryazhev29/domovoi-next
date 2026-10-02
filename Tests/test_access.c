#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
enum {MB_ID = 0};
enum {OUT_IMP = 1, OUT_INV = 2, OUT_ON = 3};
static struct {uint32_t start, size;} mem[1] = {{0, 4096}};   /* 128 cells */
static uint8_t flash[4096];
static struct {releMode_s rele[2];} config;
#define ONE_DAY (3600*24)
static struct {uint8_t hour, minute; uint32_t datetime;} device;
static logControl_e server_out_event = LOG_CON_OUTONBYAPI;
static unsigned wdt, loads[2], load_mode[2], nlogs, npass, busy[2],fail_save;
static int log_num, log_cnt, pass_cnt;
static void wdt_reload(void) { wdt++; }
static unsigned fail_read;
static bool mem_read(uint32_t a, uint8_t *p, uint16_t n) { CHECK(a + n <= sizeof(flash)); memcpy(p, flash + a, n); return !fail_read; }
static uint8_t out_getmodestate(uint8_t out) { return (uint8_t)busy[out]; }
static void out_load(uint8_t out, int mode, uint8_t t, uint8_t p, uint8_t c) { (void)t; (void)p; (void)c; loads[out]++; load_mode[out] = (unsigned)mode; }
static bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt) { (void)id; if (num == LOG_CON_PASSCOUNT) { npass++; pass_cnt = cnt; return true; } nlogs++; log_num = num; log_cnt = cnt; return true; }
static bool user_updateID(cellID_s *c)
{
  uint32_t a;
  if(fail_save)return false;
  for (a = 0; a < sizeof(flash); a += sizeof(cellID_s))
    if (((cellID_s*)(flash + a))->index == c->index) { memcpy(flash + a, c, sizeof(*c)); return true; }
  CHECK(0);return false;
}
/* PRODUCTION */

static void put(uint16_t index, const char *id, uint8_t per_day, uint8_t left)
{
  cellID_s c;
  memset(&c, 0, sizeof(c));
  c.index = index; memcpy(c.ID, id, 10); c.group = 1; c.cntPassOnDay = per_day; c.cntPass = left;
  memcpy(flash + (uint32_t)(index - 1) * sizeof(cellID_s), &c, sizeof(c));
}
static uint8_t left_of(uint16_t index) { return ((cellID_s*)(flash + (uint32_t)(index - 1) * sizeof(cellID_s)))->cntPass; }
static void at(uint8_t h, uint8_t m) { device.hour = h; device.minute = m; }
static void clear_outs(void) { loads[0] = loads[1] = 0; nlogs = npass = 0; busy[0] = busy[1] = 0; }

int main(void)
{
  cellShedule_s s;
  cellID_s found;

  /* Schedule window in minutes of the day. */
  memset(&s, 0, sizeof(s));
  s.startHour = 8; s.startMinute = 30; s.stopHour = 17; s.stopMinute = 15;
  at(9, 0); CHECK(shedule_time_ok(&s));      /* the old code refused: minute 0 < 30 */
  at(12, 10); CHECK(shedule_time_ok(&s));
  at(8, 30); CHECK(shedule_time_ok(&s)); at(17, 15); CHECK(shedule_time_ok(&s));
  at(8, 29); CHECK(!shedule_time_ok(&s)); at(17, 16); CHECK(!shedule_time_ok(&s)); at(18, 0); CHECK(!shedule_time_ok(&s));
  s.startHour = 22; s.startMinute = 0; s.stopHour = 6; s.stopMinute = 0;   /* past midnight */
  at(23, 0); CHECK(shedule_time_ok(&s)); at(5, 59); CHECK(shedule_time_ok(&s)); at(0, 0); CHECK(shedule_time_ok(&s));
  at(6, 1); CHECK(!shedule_time_ok(&s)); at(21, 59); CHECK(!shedule_time_ok(&s)); at(12, 0); CHECK(!shedule_time_ok(&s));
  s.startHour = 10; s.startMinute = 10; s.stopHour = 10; s.stopMinute = 50;
  at(10, 30); CHECK(shedule_time_ok(&s)); at(10, 5); CHECK(!shedule_time_ok(&s));

  /* The base with holes: cells 1, 2 and 7 written, an empty ID at 9. */
  memset(flash, 0xff, sizeof(flash));
  put(1, "7900000001", 255, 0); put(2, "7900000002", 255, 0); put(7, "7900000007", 5, 3);
  { cellID_s e; memset(&e, 0xff, sizeof(e)); e.index = 9; memcpy(flash + 8 * sizeof(cellID_s), &e, sizeof(e)); }
  CHECK(user_getNumbersCount() == 4 && id_cells_end == 9);
  CHECK(user_findID("7900000007", &found) && found.index == 7);   /* the old code stopped at cell 3 */
  CHECK(user_findID("7900000001", &found) && found.index == 1);
  fail_read=1;memset(&found,0x5A,sizeof(found));
  CHECK(!user_findID("7900000001", &found) && found.index==0x5A5A);fail_read=0;
  CHECK(!user_findID("7900000009", &found));
  { char ff[10]; memset(ff, 0xff, 10); CHECK(!user_findID(ff, &found)); }
  memset(flash, 0xff, sizeof(flash)); CHECK(user_getNumbersCount() == 0 && !user_findID("7900000001", &found));
  put(128, "7900000128", 255, 0); CHECK(user_getNumbersCount() == 1 && user_findID("7900000128", &found));

  /* Outputs: only the requested relays, counter and journal once per pass. */
  memset(flash, 0xff, sizeof(flash)); put(7, "7900000007", 5, 3); user_getNumbersCount();
  config.rele[0].mode = 0; config.rele[1].mode = 1;
  clear_outs(); user_activateOut(true, false, CT_WIEGAND, "7900000007");
  CHECK(loads[0] == 1 && loads[1] == 0 && load_mode[0] == OUT_IMP && nlogs == 1 && log_num == LOG_CON_OUTONBYWIEGAND && log_cnt == 2 && left_of(7) == 2);
  CHECK(npass == 1 && pass_cnt == 2); /* 131 after every pass of a limited ID */
  clear_outs(); user_activateOut(true, true, CT_PHONE, "7900000007");
  CHECK(loads[0] == 1 && loads[1] == 1 && load_mode[1] == OUT_INV && nlogs == 1 && log_cnt == 1 && left_of(7) == 1);
  /* A running relay or one used for unlocking is left out; nothing to switch means no pass. */
  clear_outs(); busy[1] = 1; user_activateOut(true, true, CT_PHONE, "7900000007");
  CHECK(loads[0] == 1 && loads[1] == 0 && nlogs == 1 && left_of(7) == 0);
  put(7, "7900000007", 5, 3);
  clear_outs(); config.rele[0].useInBlock = 1; user_activateOut(true, true, CT_RFID, "7900000007");
  CHECK(loads[0] == 0 && loads[1] == 1 && nlogs == 1 && left_of(7) == 2);
  clear_outs(); user_activateOut(true, false, CT_RFID, "7900000007");
  CHECK(loads[0] == 0 && loads[1] == 0 && nlogs == 0 && left_of(7) == 2);
  config.rele[0].useInBlock = 0;
  clear_outs(); user_activateOut(false, false, CT_PHONE, "7900000007"); CHECK(!loads[0] && !loads[1] && !nlogs && left_of(7) == 2);
  /* No passes left today: nothing opens, nothing is written. */
  put(7, "7900000007", 5, 0);
  clear_outs(); user_activateOut(true, true, CT_PHONE, "7900000007"); CHECK(!loads[0] && !loads[1] && !nlogs);
  /* An ID outside the base opens without a counter; a server command logs its source event. */
  clear_outs(); server_out_event = LOG_CON_OUTONBYAPP; user_activateOut(false, true, CT_SERVER, "0000000000");
  CHECK(loads[1] == 1 && nlogs == 1 && log_num == LOG_CON_OUTONBYAPP && log_cnt == 255 && npass == 0);

  /* Command 8 without an ID (short packet): a base cell whose ID is all zeros must not match it -
     neither block the command by its empty counter nor lose a pass. */
  { static const char none[10] = {0}; put(9, none, 5, 0); user_getNumbersCount();
    clear_outs(); user_activateOut(true, false, CT_SERVER, (char*)none);
    CHECK(loads[0] == 1 && nlogs == 1 && log_cnt == 255 && left_of(9) == 0);
    put(9, none, 5, 3); user_getNumbersCount(); clear_outs(); user_activateOut(true, false, CT_SERVER, (char*)none);
    CHECK(loads[0] == 1 && left_of(9) == 3);
    CHECK(!user_findID((char*)none, &found)); }
  /* The answer to command 8 follows the result: true only when an output was switched. */
  put(7, "7900000007", 5, 3); user_getNumbersCount();
  clear_outs(); CHECK(user_activateOut(true, false, CT_SERVER, "7900000007"));
  clear_outs(); CHECK(!user_activateOut(false, false, CT_SERVER, "7900000007"));
  put(7, "7900000007", 5, 0);
  clear_outs(); CHECK(!user_activateOut(true, true, CT_PHONE, "7900000007") && !loads[0]);
  /* Use time (Protokol2 row 284): the first pass starts it, only with the server time. */
  { cellID_s c; memset(flash, 0xff, sizeof(flash)); put(7, "7900000007", 255, 255);
    ((cellID_s*)flash)[6].useTime = 2; memset(((cellID_s*)flash)[6].reserve, 0, 10); user_getNumbersCount();
    device.datetime = 1000; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007"));
    CHECK(user_findID("7900000007", &c) && id_first_pass(&c) == 0);
    device.datetime = 1789700000u; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007"));
    CHECK(user_findID("7900000007", &c) && id_first_pass(&c) == 1789700000u && !id_expired(&c));
    device.datetime = 1789700100u; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007"));
    CHECK(user_findID("7900000007", &c) && id_first_pass(&c) == 1789700000u && !id_expired(&c));
    device.datetime = 1789700119u; CHECK(!id_expired(&c));
    device.datetime = 1789700120u; CHECK(id_expired(&c));
    c.useTime = 0; CHECK(!id_expired(&c)); }
  /* 139 on the first pass, 138 once at the end if it falls on the day of the first pass;
     an end after 23:59:59 or on an earlier day is left to the server (29.09.2026). */
  { cellID_s c; const uint32_t day = 1789700000u / ONE_DAY * ONE_DAY, t0 = day + 3600;
    cellID_s *f = &((cellID_s*)flash)[6];
    memset(flash, 0xff, sizeof(flash)); put(7, "7900000007", 255, 255);
    f->useTime = 2; memset(f->reserve, 0, 10); user_getNumbersCount();
    device.datetime = t0; id_endScan = false; id_nextEnd = 0;
    clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && nlogs == 2 && log_num == LOG_CON_IDTIMESTART);
    CHECK(id_nextEnd == t0 + 120);
    clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && nlogs == 1 && log_num == LOG_CON_OUTONBYPHONE);
    clear_outs(); device.datetime = t0 + 119; user_checkIdTimeEnd(); CHECK(nlogs == 0);
    clear_outs(); device.datetime = t0 + 120; user_checkIdTimeEnd(); CHECK(nlogs == 1 && log_num == LOG_CON_IDTIMEEND && f->reserve[6] == ID_TIME_DONE && id_nextEnd == 0);
    clear_outs(); id_endScan = true; device.datetime = t0 + 121; user_checkIdTimeEnd(); CHECK(nlogs == 0);
    /* restart the same day before the end was reported: the scan finds it */
    f->reserve[6] = 0; clear_outs(); id_endScan = true; user_checkIdTimeEnd(); CHECK(nlogs == 1 && log_num == LOG_CON_IDTIMEEND);
    /* restart on a later day: nothing, nothing written */
    f->reserve[6] = 0; clear_outs(); id_endScan = true; device.datetime = t0 + ONE_DAY; user_checkIdTimeEnd();
    CHECK(nlogs == 0 && f->reserve[6] == 0 && id_nextEnd == 0);
    /* the first pass at 23:59: the end is tomorrow, no 138, the ID still stops */
    memset(f->reserve, 0, 10); device.datetime = day + ONE_DAY - 60; id_endScan = false; id_nextEnd = 0;
    clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && log_num == LOG_CON_IDTIMESTART && id_nextEnd == 0);
    clear_outs(); id_endScan = true; device.datetime = day + ONE_DAY + 60; user_checkIdTimeEnd(); CHECK(nlogs == 0);
    CHECK(user_findID("7900000007", &c) && id_expired(&c));
    /* no server time: no scan */
    clear_outs(); id_endScan = true; device.datetime = 1000; user_checkIdTimeEnd(); CHECK(nlogs == 0 && id_endScan);
    device.datetime = 0; }
  /* Passes per day: the first pass of a new day restores the limit, the day of the last pass is kept
     in bytes 4-5 of the reserve. An unknown day (no server time, or the record from the server)
     takes the counter as it is, a refused pass still marks the day (28.09.2026). */
  { cellID_s c; const uint32_t d1 = 1789700000u;
    memset(flash, 0xff, sizeof(flash)); put(7, "7900000007", 5, 0); user_getNumbersCount();
    device.datetime = 1000; clear_outs(); CHECK(!user_activateOut(true, false, CT_PHONE, "7900000007") && !loads[0]);
    CHECK(user_findID("7900000007", &c) && id_pass_day(&c) == 0);
    device.datetime = d1; clear_outs(); CHECK(!user_activateOut(true, false, CT_PHONE, "7900000007") && !loads[0]);
    CHECK(user_findID("7900000007", &c) && id_pass_day(&c) == d1 / ONE_DAY && c.cntPass == 0);
    device.datetime = d1 + ONE_DAY; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007"));
    CHECK(left_of(7) == 4 && npass == 1 && pass_cnt == 4);
    clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && left_of(7) == 3);
    device.datetime = 1000; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && left_of(7) == 2);
    device.datetime = d1 + 3 * ONE_DAY; clear_outs(); CHECK(user_activateOut(true, false, CT_PHONE, "7900000007") && left_of(7) == 4);
    device.datetime = 0; }
  /* Level relay: an ID pass toggles it, server command 8 switches it on (Protokol2 row 186). */
  put(7, "7900000007", 255, 255); user_getNumbersCount();
  clear_outs(); CHECK(user_activateOut(false, true, CT_SERVER, "7900000007") && load_mode[1] == OUT_ON);
  clear_outs(); CHECK(user_activateOut(false, true, CT_PHONE, "7900000007") && load_mode[1] == OUT_INV);

  put(7,"7900000007",5,2);user_getNumbersCount();clear_outs();fail_save=1;
  CHECK(!user_activateOut(true,true,CT_PHONE,"7900000007"));CHECK(!loads[0]&&!loads[1]&&!nlogs&&!npass&&left_of(7)==2);
  fail_save=0;CHECK(user_activateOut(true,true,CT_PHONE,"7900000007")&&left_of(7)==1);
  puts("PASS access: schedule window by minutes, ID base with holes, requested relays only, one counter step and one record per pass");
  return 0;
}
