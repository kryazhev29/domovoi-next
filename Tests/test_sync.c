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
static uint16_t user_getNumbersCount(void){return 1;}
static uint16_t id_today(void){return 10;}
static void id_set_pass_day(cellID_s *c,uint16_t day){memcpy(c->reserve+4,&day,2);}
static uint32_t id_time_end(cellID_s *c){return c->blockTime;}
static void id_watch_end(uint32_t t){(void)t;}
static bool mem_replace_sector(uint32_t a,const uint8_t *p) { return mem_erase_sector(a) && mem_write(a,p,MEMSECTORSIZE); }
static bool mem_replace_pages(uint32_t a,uint8_t (*reader)(const void*,uint16_t,uint8_t*),const void *ctx){uint8_t image[4096];for(uint16_t off=0;off<4096;off+=256)if(!reader(ctx,off,image+off))return false;return mem_replace_sector(a,image);}
/* PRODUCTION */
static unsigned cell_size(unsigned c){return c==13?sizeof(cellID_s):c==14?sizeof(cellShedule_s):c==15?sizeof(cellGroup_s):sizeof(cellHoliday_s);}
static memBlock_e block(unsigned c){return c==13?MB_ID:c==14?MB_SHEDULE:c==15?MB_GROUP:MB_HOLIDAY;}
static void reset(void){memset(flash,0xA5,sizeof(flash));memcpy(before,flash,sizeof(flash));memset(&device,0,sizeof(device));answers=stamps=erases=writes=allocations=0;fail_alloc=fail_erase=fail_write=fail_read=0;}
static void packet(uint8_t *p,unsigned c,unsigned count,unsigned a,unsigned b){unsigned z=cell_size(c);memset(p,0,2048);p[4]=c;{uint32_t time=1234;memcpy(p+5,&time,4);}p[9]=count;p[10]=a;p[11]=c==13?a>>8:0;p[10+z]=b;p[11+z]=c==13?b>>8:0;if(c==13){p[22]=1;p[22+z]=1;}}
int main(int argc,char **argv){uint8_t p[2048];unsigned c,mode,first,last_cmd;CHECK(argc<=3);mode=argc>1?(unsigned)atoi(argv[1]):0;first=argc==3?(unsigned)atoi(argv[2]):13;last_cmd=argc==3?first:16;
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
 puts("sync: invalid second cell, bounds, allocation failure and valid writes passed");return 0;}
