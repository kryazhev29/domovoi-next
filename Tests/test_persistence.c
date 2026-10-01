#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef uint8_t *pInt8U;
/* TYPES */
#define MEMSECTORSIZE 4096
enum {MB_CONFIG, MB_ID};
static struct {uint32_t start,size;} mem[2]={{8192,4096},{0,4096}};
static struct {uint8_t bytes[510];uint16_t crc;} config,device;
static uint8_t flash[16384];
static unsigned failure,allocs,frees,writes,diag_heap_fail;
static void *test_malloc(size_t n){if(failure==1)return NULL;allocs++;return malloc(n);}
static void test_free(void *p){if(p)frees++;free(p);}
#define malloc test_malloc
#define free test_free
static void wdt_reload(void){}
static uint16_t crc16_calc(uint8_t *p,uint16_t n){(void)p;(void)n;return 123;}
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){if(failure==2)return false;CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);return true;}
static bool mem_write(uint32_t a,uint8_t *p,uint16_t n){writes++;if(failure==3)return false;CHECK(a+n<=sizeof(flash));memcpy(flash+a,p,n);return true;}
static bool mem_patch2(uint32_t a,const uint8_t *p,uint16_t n,const uint8_t *q,uint16_t k){uint8_t image[4096];uint32_t start=a&~4095u;if(!mem_read(start,image,4096))return false;memcpy(image+a-start,p,n);if(k)memcpy(image+a-start+n,q,k);return mem_write(start,image,4096);}
static bool mem_patch(uint32_t a,const uint8_t *p,uint16_t n){return mem_patch2(a,p,n,NULL,0);}
/* PRODUCTION */
static bool save_result(void){
#if HAVE_RESULT
 return user_writeConfigAndDevice();
#else
 user_writeConfigAndDevice();return true;
#endif
}
static bool id_result(cellID_s *c){
#if HAVE_RESULT
 return user_updateID(c);
#else
 user_updateID(c);return true;
#endif
}
int main(void){cellID_s c;unsigned f;memset(&c,0,sizeof(c));c.index=7;c.cntPass=1;
 (void)test_malloc;(void)test_free;(void)diag_heap_fail;
 failure=1;CHECK(save_result());memset(flash,255,sizeof(flash));memcpy(flash,&c,sizeof(c));CHECK(id_result(&c));CHECK(!allocs&&!frees);
 for(f=2;f<=3;f++){failure=f;allocs=frees=writes=0;CHECK(!save_result());CHECK(allocs==frees);if(f<3)CHECK(!writes);
  memset(flash,255,sizeof(flash));memcpy(flash,&c,sizeof(c));allocs=frees=writes=0;CHECK(!id_result(&c));CHECK(allocs==frees);if(f<3)CHECK(!writes);}
 failure=0;allocs=frees=0;CHECK(save_result()&&id_result(&c)&&allocs==frees);
 c.index=8;CHECK(!id_result(&c));
 puts("persistence: real save/update allocation/read/write errors, no leaks passed");return 0;}
