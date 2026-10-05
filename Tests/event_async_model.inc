#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
enum {MB_LOG};
static struct {uint32_t start,size;} mem[]={{4096,98304}};
static struct {uint32_t datetime,worktime,newAddressLog;struct {unsigned readyLogData;} status;} device;
static uint32_t log_lastIndex,now;
static uint8_t flash[98304];
static bool id_endScan,store_runtime=true;
static unsigned reads,starts,polls,read_ops,start_ops,poll_ops,fail_read,fail_start,fail_poll;
static unsigned held,busy,unavailable,cut_count,written_bytes,max_reads,max_starts,max_polls;
static int cut_at=-1;
static jmp_buf power;
static uint32_t address;
static uint16_t length;
static const uint8_t *source;
static uint8_t source_copy[64];
static void wdt_reload(void){}
static bool log_addEvent(cellLog_s *p){(void)p;CHECK(!store_runtime);return true;}
static bool w25_read_available(void){return !unavailable;}
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){
  CHECK(a>=4096&&a+n<=102400);reads++;read_ops++;
  if(read_ops==fail_read)return false;
  memcpy(p,flash+a-4096,n);return true;
}
static bool w25_async_start(uint32_t a,const uint8_t *p,uint16_t n,uint32_t t){
  (void)t;CHECK(!held&&p&&n&&n<=64&&(a&255)+n<=256);starts++;start_ops++;
  if(start_ops==fail_start)return false;
  CHECK(a>=4096&&a+n<=102400);held=1;busy=3;address=a;length=n;source=p;memcpy(source_copy,p,n);return true;
}
static uint8_t w25_async_poll(uint32_t t){
  (void)t;CHECK(held);polls++;poll_ops++;CHECK(!memcmp(source,source_copy,length));
  if(busy){busy--;return 1;}
  held=0;
  for(unsigned i=0;i<length;i++){
    if(cut_at>=0&&(int)written_bytes==cut_at)longjmp(power,1);
    flash[address-4096+i]&=source[i];written_bytes++;
  }
  /* Simulates an error after the device accepted every byte. */
  return poll_ops==fail_poll?3:2;
}
/* PRODUCTION */
static void reset(void){
  memset(flash,255,sizeof(flash));memset(&log_job,0,sizeof(log_job));
  memset(&device,0,sizeof(device));device.newAddressLog=4096;
  held=unavailable=read_ops=start_ops=poll_ops=fail_read=fail_start=fail_poll=0;
  log_lastIndex=now=log_pendingCount=0;log_timeKnown=1;written_bytes=0;cut_at=-1;
}
static void step(void){
  reads=starts=polls=0;log_storage_poll(now++);
  CHECK(reads<=1&&starts<=1&&polls<=1);
  if(reads>max_reads)max_reads=reads;if(starts>max_starts)max_starts=starts;if(polls>max_polls)max_polls=polls;
}
static void drain(void){unsigned i=0;while(log_pendingCount){step();CHECK(++i<30000);}}
static cellLog_s event(unsigned id){cellLog_s c;memset(&c,0,sizeof(c));c.datetime=c.index=id;c.logType=33;return c;}
static void queue(unsigned id){cellLog_s c=event(id);CHECK(log_put(&c));}
static unsigned records(void){fail_read=0;unsigned n=log_getRecordsCount();CHECK(n!=65535);return n;}
int main(void){
  reset();queue(100);CHECK(log_pendingCount==1&&!device.status.readyLogData);step();
  for(unsigned i=0;i<30&&log_pendingCount;i++){
    uint32_t before=device.newAddressLog;step();
    if(log_pendingCount)CHECK(device.newAddressLog==before&&!device.status.readyLogData);
  }
  CHECK(!log_pendingCount&&device.newAddressLog==4128&&records()==1);
  /* All 16 cells retain order, unique IDs and owned sources during later enqueue. */
  reset();for(unsigned i=0;i<16;i++)queue(100);cellLog_s c=event(100);CHECK(!log_put(&c));
  log_flushPending();log_checkPending();CHECK(!held&&!read_ops&&!start_ops);
  drain();CHECK(records()==16);
  for(unsigned i=0;i<16;i++){memcpy(&c,flash+i*32,32);CHECK(c.index==100+i&&log_valid(&c));}
  /* Each possible read/start/poll failure: no early pop; exact late commit dedup. */
  for(unsigned kind=0;kind<3;kind++)for(unsigned fail=1;fail<=10;fail++){
    reset();if(kind==0)fail_read=fail;if(kind==1)fail_start=fail;if(kind==2)fail_poll=fail;
    queue(100);drain();CHECK(records()==1&&device.newAddressLog==4128);
  }
  /* Stop power before each programmed byte, including complete marker with lost RAM ACK. */
  for(int cut=0;cut<=32;cut++){
    reset();queue(100);cut_at=cut;
    if(!setjmp(power))drain();
    held=0;cut_at=-1;memset(&log_job,0,sizeof(log_job));log_pendingCount=0;log_lastIndex=0;
    device.newAddressLog=4096;
    CHECK(records()==(cut==32?1u:0u));
    queue(200);drain();CHECK(records()==(cut==32?2u:1u));cut_count++;
  }
  /* A read error after marker acceptance must not duplicate a queued event. */
  reset();queue(100);while(log_job.phase!=LA_VERIFY)step();fail_read=read_ops+1;
  step();CHECK(log_pendingCount==1&&log_job.result==3&&device.newAddressLog==4096);
  drain();CHECK(records()==1&&start_ops==2);
  /* Incompatible torn cell is skipped; stable cursor still waits for success. */
  reset();queue(100);while(log_job.phase!=LA_BODY)step();fail_start=start_ops+1;step();
  flash[0]=0;drain();CHECK(records()==1&&device.newAddressLog==4160);
  /* Reconciliation finishes any compatible partial body/marker in the same slot. */
  for(unsigned part=0;part<32;part++){
    reset();queue(100);while(log_job.phase!=LA_BODY)step();fail_start=1;step();
    memcpy(flash,&log_job.cell,part);drain();CHECK(records()==1&&device.newAddressLog==4128);
  }
  reset();queue(100);while(!held)step();unavailable=1;
  for(unsigned i=0;i<4;i++)step();CHECK(log_pendingCount==1);
  unavailable=0;drain();CHECK(records()==1);
  /* Mid-write time update changes only later queue cells, never the owned head. */
  reset();log_timeKnown=0;device.worktime=700;queue(100);queue(101);while(!held)step();
  cellLog_s saved=log_job.cell;log_setTime(10000);CHECK(!memcmp(&saved,&log_job.cell,32)&&log_pending[0].datetime==700&&log_pending[1].datetime==10000);
  drain();CHECK(records()==2&&id_endScan);
  /* 3072 occupied slots are visited one per pass; full journal releases the writer. */
  reset();memset(flash,0,sizeof(flash));queue(100);
  for(unsigned i=0;i<3075;i++)step();CHECK(log_pendingCount==1&&!log_async_active()&&log_job.result==3);
  CHECK(device.newAddressLog==4096&&!held);
  /* Clearing can now make room, without popping the pending event on full/error. */
  memset(flash,255,sizeof(flash));now+=1000;drain();CHECK(records()==1);
  printf("PASS async events: bounded reads/starts/polls %u/%u/%u; 33 byte cuts, 30 read/start/poll faults, 32 torn retries, queue/order/time/full/dedup\n",max_reads,max_starts,max_polls);
  return 0;
}
