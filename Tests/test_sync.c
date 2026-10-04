#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}} while(0)
/* TYPES */
#define MEMSECTORSIZE 4096u
static cellMem_s mem[16];
static struct {uint32_t datetimeID,datetimeShedule,datetimeGroup,datetimeHoliday;uint16_t numbersCount;} device;
static uint8_t flash[32768],before[32768];
static unsigned answers,stamps,erases,writes,allocations,diag_heap_fail,fail_alloc,fail_erase,fail_write,fail_read;
static cmdCode_e answer;
static void *test_malloc(size_t n){allocations++;return fail_alloc?NULL:malloc(n);}
#define malloc test_malloc
static void wdt_reload(void){}
static bool mem_read(uint32_t a,void *p,unsigned n){CHECK(a+n<=sizeof(flash));if(fail_read)return false;memcpy(p,flash+a,n);return true;}
static bool mem_write(uint32_t a,const void *p,unsigned n){CHECK(a+n<=sizeof(flash));writes++;if(fail_write)return false;memcpy(flash+a,p,n);return true;}
static bool mem_erase_sector(uint32_t a){CHECK(a+4096<=sizeof(flash));erases++;if(fail_erase)return false;memset(flash+a,255,4096);return true;}
static void user_send_answer_comand(unsigned c,cmdCode_e code,dataChanel_e ch,unsigned id){CHECK(c>=13&&c<=16&&ch==CH_SERVER&&id==42);answers++;answer=code;}
static void user_send_timeshtamp(shtamp_e s,uint32_t t,uint16_t crc,dataChanel_e ch,unsigned id){(void)s;CHECK(t==1234&&crc==0x1234&&ch==CH_SERVER&&id==42);stamps++;}
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n){CHECK(a+n<=sizeof(flash));return 0x1234;}
static unsigned rescans;
static uint16_t user_getNumbersCount(void){rescans++;return 1;}
static uint16_t id_today(void){return 10;}
static void id_set_pass_day(cellID_s *c,uint16_t day){memcpy(c->reserve+4,&day,2);}
static uint32_t id_time_end(cellID_s *c){return c->blockTime;}
static void id_watch_end(uint32_t t){(void)t;}
/* Journal events 6-10 (end of sync); sync_refuse - journal busy, retried next second. */
static unsigned sync_events[16], sync_refuse;
static bool log_addSystemEvent(logSystem_e e){if(sync_refuse){sync_refuse--;return false;}CHECK(e>=6&&e<=10);sync_events[e]++;return true;}
static bool mem_replace_sector(uint32_t a,const uint8_t *p) { return mem_erase_sector(a) && mem_write(a,p,MEMSECTORSIZE); }
static bool mem_replace_pages(uint32_t a,uint8_t (*reader)(const void*,uint16_t,uint8_t*),const void *ctx){uint8_t image[4096];for(uint16_t off=0;off<4096;off+=256)if(!reader(ctx,off,image+off))return false;return mem_replace_sector(a,image);}
/* PRODUCTION */
static unsigned cell_size(unsigned c){return c==13?sizeof(cellID_s):c==14?sizeof(cellShedule_s):c==15?sizeof(cellGroup_s):sizeof(cellHoliday_s);}
static memBlock_e block(unsigned c){return c==13?MB_ID:c==14?MB_SHEDULE:c==15?MB_GROUP:MB_HOLIDAY;}
static void reset(void){memset(flash,0xA5,sizeof(flash));memcpy(before,flash,sizeof(flash));memset(&device,0,sizeof(device));answers=stamps=erases=writes=allocations=0;fail_alloc=fail_erase=fail_write=fail_read=0;}
static void packet(uint8_t *p,unsigned c,unsigned count,unsigned a,unsigned b){unsigned z=cell_size(c);memset(p,0,2048);p[4]=c;{uint32_t time=1234;memcpy(p+5,&time,4);}p[9]=count;p[10]=a;p[11]=c==13?a>>8:0;p[10+z]=b;p[11+z]=c==13?b>>8:0;if(c==13){p[22]=1;p[22+z]=1;}}
static void ticks(unsigned n){while(n--)sync_end_tick();}
static void sync_end_tests(void){uint8_t p[2048];
 /* 6 ID, 8 groups, 9 schedules, 10 holidays: once, 30 s after the last sync packet. */
 memset(sync_events,0,sizeof(sync_events));sync_pending=sync_quiet=0;
 reset();packet(p,14,1,1,0);command(p,8+cell_size(14),CH_SERVER,42);packet(p,15,1,1,0);command(p,8+cell_size(15),CH_SERVER,42);
 ticks(29);CHECK(!sync_events[8]&&!sync_events[9]);
 packet(p,14,1,1,0);command(p,8+cell_size(14),CH_SERVER,42);ticks(29);CHECK(!sync_events[9]);
 ticks(1);CHECK(sync_events[8]==1&&sync_events[9]==1&&!sync_events[6]&&!sync_events[10]);
 ticks(100);CHECK(sync_events[8]==1&&sync_events[9]==1);
 /* A packet that was not written does not count. */
 reset();fail_write=1;packet(p,16,1,1,0);command(p,8+cell_size(16),CH_SERVER,42);fail_write=0;ticks(40);CHECK(!sync_events[10]);
 reset();packet(p,16,1,1,0);command(p,8+cell_size(16),CH_SERVER,42);ticks(30);CHECK(sync_events[10]==1);
 /* Journal busy: retried a second later, written once. */
 reset();memset(flash,255,sizeof(flash));packet(p,13,1,1,0);command(p,8+cell_size(13),CH_SERVER,42);sync_refuse=1;
 ticks(30);CHECK(!sync_events[6]);ticks(1);CHECK(sync_events[6]==1);ticks(60);CHECK(sync_events[6]==1);
 puts("sync end: events 6-10 once after 30 s quiet, failed packet ignored, busy journal retried passed");}
/* Holidays: a list of dates without cell numbers, replaced as a whole (Protocol2 rows 359-362). */
static void holiday_tests(void){uint8_t p[2048];uint32_t d[3]={1767225600u,1767312000u,1778198400u},s=mem[MB_HOLIDAY].start;unsigned i,f;
 memset(p,0,sizeof(p));p[4]=16;{uint32_t t=1234;memcpy(p+5,&t,4);}p[9]=3;memcpy(p+10,d,12);
 /* 1 Jan 00:00 has a zero low byte: it used to be taken as cell number 0 and refused. */
 reset();command(p,8+12,CH_SERVER,42);CHECK(!answers&&stamps==1&&device.datetimeHoliday==1234&&!memcmp(flash+s,d,12));
 for(i=12;i<1024;i++)CHECK(flash[s+i]==0xff);
 CHECK(!memcmp(flash+s+1024,before+s+1024,4096-1024));
 /* A shorter list replaces the longer one. */
 p[9]=1;command(p,8+4,CH_SERVER,42);CHECK(stamps==2&&!memcmp(flash+s,d,4)&&flash[s+4]==0xff&&flash[s+8]==0xff);
 /* Wrong length or no dates: 1, nothing written. */
 reset();p[9]=2;command(p,8+4,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!writes&&!erases);
 reset();p[9]=0;command(p,8,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!writes);
 /* Storage not ready, read/erase/write failure: 2, timestamp unchanged. */
 reset();p[9]=1;table_storage_ready=false;command(p,12,CH_SERVER,42);CHECK(answers==1&&answer==CMD_RUNERROR&&!writes);table_storage_ready=true;
 for(f=0;f<3;f++){reset();fail_read=f==0;fail_erase=f==1;fail_write=f==2;command(p,12,CH_SERVER,42);
  CHECK(answers==1&&answer==CMD_RUNERROR&&!stamps&&!device.datetimeHoliday);}
 puts("holidays: dates in order from cell 1, zero low byte accepted, list replaced as a whole, errors keep the timestamp passed");}
int main(int argc,char **argv){uint8_t p[2048];unsigned c,mode,first,last_cmd;CHECK(argc<=3);mode=argc>1?(unsigned)atoi(argv[1]):0;first=argc==3?(unsigned)atoi(argv[2]):13;last_cmd=argc==3?first:15;
 (void)test_malloc;(void)wdt_reload;(void)diag_heap_fail;
 mem[MB_ID]=(cellMem_s){4096,8192};mem[MB_SHEDULE]=(cellMem_s){12288,2048};mem[MB_GROUP]=(cellMem_s){16384,2048};mem[MB_HOLIDAY]=(cellMem_s){20480,1024};
 for(c=first;c<=last_cmd;c++){unsigned z=cell_size(c),last=mem[block(c)].size/z;
  reset();packet(p,c,2,1,mode==1&&c!=16?last+1:0);command(p,8+2*z,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!stamps);CHECK(!erases&&!writes&&!memcmp(flash,before,sizeof(flash)));CHECK(!device.datetimeID&&!device.datetimeShedule&&!device.datetimeGroup&&!device.datetimeHoliday);
  reset();table_storage_ready=false;packet(p,c,1,1,0);command(p,8+z,CH_SERVER,42);CHECK(answers==1&&answer==CMD_RUNERROR&&!stamps&&!allocations&&!erases&&!writes);table_storage_ready=true;
  reset();table_storage_ready=false;packet(p,c,2,1,0);command(p,8+2*z,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!stamps&&!allocations&&!erases&&!writes);table_storage_ready=true;
  reset();packet(p,c,1,1,0);command(p,8+z,CH_SERVER,42);CHECK(!answers&&stamps==1&&erases==1&&writes==1);CHECK(flash[mem[block(c)].start]==1);
  reset();packet(p,c,1,1,0);fail_alloc=1;command(p,8+z,CH_SERVER,42);CHECK(!answers&&stamps==1&&writes==1&&!allocations);
  reset();packet(p,c,1,1,0);command(p,8+z-1,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!stamps&&!writes);
  {unsigned failure;for(failure=0;failure<3;failure++){
   reset();packet(p,c,1,1,0);fail_read=failure==0;fail_erase=failure==1;fail_write=failure==2;
   command(p,8+z,CH_SERVER,42);CHECK(answers==1&&answer==CMD_RUNERROR&&!stamps);
   CHECK(!device.datetimeID&&!device.datetimeShedule&&!device.datetimeGroup&&!device.datetimeHoliday);
   if(failure<2)CHECK(!writes);if(failure==0)CHECK(!erases);
  }}
 }
 reset();packet(p,13,2,129,0);command(p,72,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!stamps&&!erases&&!writes);
 reset();packet(p,13,1,1,0);p[22]=0;p[23]=255;command(p,40,CH_SERVER,42);CHECK(answers==1&&answer==CMD_PARAMERROR&&!stamps&&!writes);
 /* Счёт ID после команды 13 на ходу: пустые ячейки прибавляются, повтор номера и уже
    занятая ячейка - нет, вся база не перечитывается. */
 reset();memset(flash,255,sizeof(flash));rescans=0;device.numbersCount=7;
 packet(p,13,2,3,5);command(p,72,CH_SERVER,42);CHECK(!answers&&device.numbersCount==9&&!rescans&&id_cells_end==5);
 packet(p,13,2,3,5);command(p,72,CH_SERVER,42);CHECK(device.numbersCount==9&&!rescans);
 packet(p,13,2,4,4);command(p,72,CH_SERVER,42);CHECK(device.numbersCount==10&&!rescans&&id_cells_end==5);
 packet(p,13,1,2,0);command(p,40,CH_SERVER,42);CHECK(device.numbersCount==11&&id_cells_end==5);
 fail_write=1;packet(p,13,1,9,0);command(p,40,CH_SERVER,42);CHECK(rescans==1&&answer==CMD_RUNERROR);
 if(argc==1){sync_end_tests();holiday_tests();}
 puts("sync: invalid second cell, bounds, allocation failure and valid writes, ID count without rescan passed");return 0;}
