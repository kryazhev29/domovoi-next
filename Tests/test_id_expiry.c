#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
#define ONE_DAY 86400u
#define ID_TIME_VALID 1600000000u
#define ID_TIME_DONE 0x38u
static struct {uint32_t datetime;} device;
/* PRODUCTION */
int main(void){
 const uint32_t firsts[]={1600000000u,2147483600u,UINT32_MAX-120u,UINT32_MAX-30u};
 const uint16_t durations[]={0,1,2,65535};
 const uint32_t times[]={0,1600000000u,2147483647u,UINT32_MAX-121u,UINT32_MAX-60u,UINT32_MAX};
 for(unsigned f=0;f<4;f++)for(unsigned d=0;d<4;d++){
  cellID_s c={0};c.useTime=durations[d];memcpy(c.reserve,firsts+f,4);
  uint64_t end=(uint64_t)firsts[f]+(uint32_t)c.useTime*60u;
  for(unsigned t=0;t<6;t++){device.datetime=times[t];CHECK(id_expired(&c)==(c.useTime&&device.datetime>=end));}
  uint32_t expected=c.useTime&&end<=UINT32_MAX&&end/ONE_DAY==firsts[f]/ONE_DAY?(uint32_t)end:0;
  CHECK(id_time_end(&c)==expected);c.reserve[6]=ID_TIME_DONE;CHECK(!id_time_end(&c));
 }
 cellID_s c={0};c.useTime=1;memset(c.reserve,255,4);device.datetime=UINT32_MAX;CHECK(!id_expired(&c)&&!id_time_end(&c));
 puts("B30: unsigned expiry matches 64-bit reference, no premature end on overflow PASS");
}
