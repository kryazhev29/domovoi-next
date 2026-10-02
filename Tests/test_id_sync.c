/* Доработка: Кряжев А., 2026. Сквозной тест базы ID: команда 13 из прошивки, user_findID,
   user_getNumbersCount и индекс по корзинам вместе, против перебора всей базы. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int stage;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL stage %d line %d: %s\n",stage,__LINE__,#x);exit(1);}}while(0)
#define IDINDEX_HOST_TEST 1
/* TYPES */
#define MEMSECTORSIZE 4096u
static cellMem_s mem[16];
static struct {uint32_t datetimeID,datetimeShedule,datetimeGroup,datetimeHoliday;uint16_t numbersCount;} device;
static uint8_t flash[0x342000];
static unsigned answers;
static cmdCode_e answer;
static unsigned long read_bytes;
static bool mem_read(uint32_t a,void *p,unsigned n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);read_bytes+=n;return true;}
static bool mem_write(uint32_t a,const void *p,unsigned n){CHECK(a+n<=sizeof(flash));for(unsigned i=0;i<n;i++)flash[a+i]&=((const uint8_t*)p)[i];return true;}
static bool mem_erase_sector(uint32_t a){a&=~4095u;CHECK(a+4096<=sizeof(flash));memset(flash+a,255,4096);return true;}
static void wdt_reload(void){}
static void user_send_answer_comand(unsigned c,cmdCode_e code,dataChanel_e ch,unsigned id){(void)c;(void)ch;(void)id;answers++;answer=code;}
static void user_send_timeshtamp(shtamp_e s,uint32_t t,uint16_t crc,dataChanel_e ch,unsigned id){(void)s;(void)t;(void)crc;(void)ch;(void)id;}
static uint16_t id_today(void){return 10;}
static void id_set_pass_day(cellID_s *c,uint16_t day){memcpy(c->reserve+4,&day,2);}
static uint32_t id_time_end(cellID_s *c){return c->blockTime;}
static void id_watch_end(uint32_t t){(void)t;}
static bool mem_tables_ready(void){return true;}
static bool mem_replace_sector(uint32_t a,const uint8_t *p){return mem_erase_sector(a)&&mem_write(a,p,MEMSECTORSIZE);}
static bool mem_replace_pages(uint32_t a,uint8_t (*reader)(const void*,uint16_t,uint8_t*),const void *ctx){static uint8_t image[4096];for(uint16_t off=0;off<4096;off+=256)if(!reader(ctx,off,image+off))return false;return mem_replace_sector(a,image);}
/* PRODUCTION */
static uint32_t rnd=777;
static uint32_t next(void){rnd=rnd*1103515245u+12345u;return rnd>>8;}
static void make_id(char *id,uint32_t k){for(int i=0;i<10;i++)id[i]=(char)('0'+(k>>(i*3))%10);id[9]=(char)(k|1u);}
static int linear(const char *id){for(uint32_t i=0;i<mem[MB_ID].size/32u;i++){const uint8_t *c=flash+mem[MB_ID].start+i*32u;if(!(c[0]==0xFF&&c[1]==0xFF)&&!memcmp(c+2,id,10))return 1;}return 0;}
static void packet13(uint8_t *p,unsigned n,const uint16_t *idx,char (*ids)[10]){
  memset(p,0,2048);p[4]=13;{uint32_t t=1234;memcpy(p+5,&t,4);}p[9]=(uint8_t)n;
  for(unsigned i=0;i<n;i++){uint8_t *c=p+10+i*32u;memcpy(c,&idx[i],2);memcpy(c+2,ids[i],10);c[12]=1;}}
static void sync_random(unsigned packets,uint32_t *k){
  uint8_t p[2048];uint16_t idx[30];char ids[30][10];
  for(unsigned q=0;q<packets;q++){unsigned n=1+next()%30;
    for(unsigned i=0;i<n;i++){idx[i]=(uint16_t)(1+next()%12000);make_id(ids[i],(*k)++%9000);}
    if(n>3&&q%5==0)idx[2]=idx[1];
    packet13(p,n,idx,ids);answers=0;command(p,(uint16_t)(8+n*32),CH_SERVER,42);CHECK(!answers);}
}
static void check_find(void){
  char id[10];cellID_s cell;
  for(uint32_t k=0;k<11000;k+=7){make_id(id,k);int r=user_findID(id,&cell);CHECK(r==linear(id));if(r)CHECK(!memcmp(cell.ID,id,10));}
}
static uint16_t count_ref(void){uint16_t n=0;for(uint32_t i=0;i<mem[MB_ID].size/32u;i++)if(!(flash[mem[MB_ID].start+i*32u]==0xFF&&flash[mem[MB_ID].start+i*32u+1]==0xFF))n++;return n;}
static void reboot(void){memset(&ix,0,sizeof(ix));id_used_bytes=0xFFFFFFFFu;device.numbersCount=user_getNumbersCount();}
static void settle(void){for(int i=0;i<300000&&ix.state!=IX_READY&&ix.state!=IX_OFF;i++)idindex_poll(mem[MB_ID].start);}
int main(void){
  uint32_t k=0;char id[10];cellID_s cell;
  memset(flash,0xFF,sizeof(flash));mem[MB_ID]=(cellMem_s){266240,1601536};
  stage=1;reboot();CHECK(device.numbersCount==0);settle();CHECK(ix.state==IX_READY);
  stage=2;sync_random(300,&k);CHECK(ix.state==IX_READY);CHECK(device.numbersCount==count_ref());check_find();
  /* Поиск по готовому индексу не читает базу. */
  make_id(id,12345);read_bytes=0;(void)user_findID(id,&cell);CHECK(read_bytes<=64+4*32);
  stage=3;reboot();CHECK(ix.state==IX_READY);CHECK(device.numbersCount==count_ref());check_find();
  /* Запись мимо индекса (как старой прошивкой) - при старте пересборка, поиск верен всё время. */
  stage=4;make_id(id,999999);memcpy(flash+mem[MB_ID].start+(11999u-1u)*32u+2,id,10);flash[mem[MB_ID].start+(11999u-1u)*32u]=0x5F;flash[mem[MB_ID].start+(11999u-1u)*32u+1]=0x2E;
  reboot();CHECK(ix.state!=IX_READY);CHECK(user_findID(id,&cell));check_find();
  for(int i=0;i<40;i++)idindex_poll(mem[MB_ID].start);sync_random(20,&k);check_find();
  settle();CHECK(ix.state==IX_READY);check_find();CHECK(user_findID(id,&cell));
  stage=5;reboot();CHECK(ix.state==IX_READY);CHECK(device.numbersCount==count_ref());check_find();
  puts("idsync: command 13 + user_findID + index equal full scan over 320 packets, restart, foreign write and sync during rebuild passed");
  return 0;
}
