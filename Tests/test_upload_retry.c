#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* TYPES */
enum {MB_LOG,MB_TEMPID,MB_ID,MB_GROUP,MB_SHEDULE,MB_HOLIDAY,MB_PHOTO};
enum {CE_OK=0,CE_BUSY=5};
#define _1B(x) ((uint8_t)(x))
#define _2B(x) ((uint8_t)((x)>>8))
static struct {uint32_t start,size;} mem[2]={{0,16384},{16384,16384}};
typedef struct {uint16_t readLogIDIndex,readTempIDIndex,logsCount,tempIDsCount;uint32_t newAddressLog,newAddressTempID;
 int8_t connectionhandle;uint8_t reserv[8],FWinProgress;uint16_t updateTimeout;struct {uint8_t readyLogData,readyID,readyToUpdate;} status;} device_s;
static device_s device,saved;
static uint8_t flash[32768],last_frame[1100],failed_frame[1100];
static unsigned records,fail_transport,fail_alloc,fail_read,fail_save,fail_clear,attempts,clears,frame_size,saves;
static void upload_reset(uint8_t);
static void *test_malloc(size_t n){return fail_alloc?NULL:malloc(n);}
#define malloc test_malloc
static void wdt_reload(void){}
static uint16_t log_getRecordsCount(void){return records;}
static uint16_t user_getTempIDRecordsCount(void){return records;}
static bool user_writeConfigAndDevice(void){if(fail_save)return false;saves++;saved=device;return true;}
static bool log_clearLogBuffer(void){CHECK(!saved.readLogIDIndex&&!memcmp(saved.reserv+4,"\0\0",2));if(fail_clear)return false;clears++;memset(flash,255,16384);records=0;device.logsCount=0;device.status.readyLogData=0;upload_reset(10);return true;}
static bool user_clearTempIdBuffer(void){CHECK(!saved.readTempIDIndex&&!memcmp(saved.reserv+6,"\0\0",2));if(fail_clear)return false;clears++;memset(flash+16384,255,16384);records=0;device.tempIDsCount=0;device.status.readyID=0;upload_reset(9);return true;}
static void reboot(void);
static void connection_deleteAll(void){}
static bool user_clearMemBlock(unsigned mb){(void)mb;return true;}
static void user_setdefault(void){(void)user_writeConfigAndDevice();}
static void user_initvariables(void){reboot();}
static void usergsm_restart(void){}
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){CHECK(a+n<=sizeof(flash));if(fail_read)return false;memcpy(p,flash+a,n);return true;}
static bool send_frame(uint8_t *p,uint16_t n){CHECK(n<=sizeof(last_frame));attempts++;memcpy(last_frame,p,n);frame_size=n;return !fail_transport;}
static uint8_t usb_send(uint8_t *p,uint16_t n){return send_frame(p,n);}
static int8_t connection_send(int8_t h,uint8_t *p,uint16_t n){CHECK(h==2);return send_frame(p,n)?CE_OK:CE_BUSY;}
/* PRODUCTION */
static void upload(unsigned cmd,dataChanel_e ch){if(cmd==10)user_send_logRecords(ch,42);else user_send_tempIDs(ch,42);}
static unsigned cursor(unsigned cmd,dataChanel_e ch){return upload_index(cmd,ch);}
static unsigned pending(unsigned cmd,dataChanel_e ch){return (cmd==10?log_unacked:temp_unacked)[ch==CH_USB?0:1];}
static void setup(unsigned cmd){unsigned i;memset(&device,0,sizeof(device));log_usbReadIndex=temp_usbReadIndex=0;
 memset(log_unacked,0,sizeof(log_unacked));memset(temp_unacked,0,sizeof(temp_unacked));device.connectionhandle=2;
 records=cmd==10?65:201;device.logsCount=device.tempIDsCount=records;device.status.readyID=device.status.readyLogData=1;
 device.newAddressLog=records*32;device.newAddressTempID=16384+records*32;saved=device;
 memset(flash,255,sizeof(flash));for(i=0;i<records;i++){cellLog_s log;cellID_s id;memset(&log,0,sizeof(log));memset(&id,0,sizeof(id));
 log.index=i+1;log.logType=33;id.index=i+1;memcpy(id.ID,"7900000007",10);memcpy(flash+i*32,&log,32);memcpy(flash+16384+i*32,&id,32);}
 attempts=clears=saves=fail_read=fail_alloc=fail_transport=fail_save=fail_clear=0;}
static void reboot(void){device=saved;log_usbReadIndex=temp_usbReadIndex=0;memset(log_unacked,0,sizeof(log_unacked));memset(temp_unacked,0,sizeof(temp_unacked));upload_restore(&saved);}
int main(void){unsigned cmd,ch,f,p;(void)user_writeConfigAndDevice;for(cmd=9;cmd<=10;cmd++)for(ch=CH_USB;ch<=CH_SERVER;ch++)for(f=0;f<3;f++){
 unsigned cap=cmd==10?32:100,total;setup(cmd);total=records;
 for(p=0;p<3;p++){unsigned start=p*cap,end=start+cap;if(end>total)end=total;
  if(p==f){fail_transport=1;upload(cmd,ch);CHECK(cursor(cmd,ch)==start&&!pending(cmd,ch)&&!clears);
   memcpy(failed_frame,last_frame,frame_size);fail_transport=0;upload(cmd,ch);CHECK(!memcmp(failed_frame,last_frame,frame_size));
  }else upload(cmd,ch);
  CHECK(cursor(cmd,ch)==start&&pending(cmd,ch)==end&&!clears);
 }
 fail_transport=1;upload(cmd,ch);CHECK(clears==1&&!records&&!cursor(cmd,ch)&&!pending(cmd,ch));
 CHECK(!last_frame[9]&&!last_frame[10]);if(cmd==10)CHECK(!last_frame[11]);
 CHECK(cmd==10?!device.status.readyLogData:!device.status.readyID);
 }
 for(cmd=9;cmd<=10;cmd++)for(ch=CH_USB;ch<=CH_SERVER;ch++){
  unsigned cap=cmd==10?32:100;setup(cmd);upload(cmd,ch);CHECK(!cursor(cmd,ch));
  memcpy(failed_frame,last_frame,frame_size);reboot();upload(cmd,ch);CHECK(!cursor(cmd,ch)&&!memcmp(failed_frame,last_frame,frame_size));
  upload(cmd,ch);CHECK(cursor(cmd,ch)==cap);memcpy(failed_frame,last_frame,frame_size);reboot();upload(cmd,ch);
  CHECK(cursor(cmd,ch)==cap&&!memcmp(failed_frame,last_frame,frame_size)&&!clears);
  setup(cmd);fail_alloc=1;upload(cmd,ch);CHECK(!cursor(cmd,ch)&&!pending(cmd,ch)&&!attempts&&!clears);
  setup(cmd);fail_read=1;upload(cmd,ch);CHECK(!cursor(cmd,ch)&&!pending(cmd,ch)&&!attempts&&!clears);
  setup(cmd);upload(cmd,ch);fail_save=1;attempts=0;upload(cmd,ch);
  CHECK(!cursor(cmd,ch)&&pending(cmd,ch)==cap&&!attempts&&!clears);fail_save=0;upload(cmd,ch);CHECK(cursor(cmd,ch)==cap);
  setup(cmd);upload(cmd,CH_SMS);CHECK(!attempts&&!cursor(cmd,CH_SERVER)&&!clears);
  setup(cmd);upload(cmd,CH_USB);upload(cmd,CH_SERVER);CHECK(!cursor(cmd,CH_USB)&&!cursor(cmd,CH_SERVER));
  upload(cmd,CH_USB);CHECK(cursor(cmd,CH_USB)==cap&&!cursor(cmd,CH_SERVER));
  setup(cmd);upload(cmd,ch);upload(cmd,ch);upload(cmd,ch);fail_clear=1;attempts=0;upload(cmd,ch);
  CHECK(!attempts&&!clears&&(cmd==10?device.status.readyLogData:device.status.readyID));
  fail_clear=0;
  setup(cmd);upload(cmd,ch);upload(cmd,ch);CHECK(cursor(cmd,ch)==cap);user_factoryReset();
  CHECK(!cursor(9,CH_USB)&&!cursor(9,CH_SERVER)&&!cursor(10,CH_USB)&&!cursor(10,CH_SERVER));
 }
 puts("uploadretry: real cmd9/10, Q5 next-request ACK, failures on every packet/last, durable cursors/reboot, USB/server, erase only after ACK passed");return 0;}
