#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t boot_ver=0x32;
#define BOOT_VER boot_ver
#define FWBLOCK_OFFSET (uint32_t)0x200
#define CPU_ENDMEMORYADDRESS (uint32_t)0x0801ffff
#define APLICATION_ADDRESS (uint32_t)(0x08008000 + FWBLOCK_OFFSET)
/* TYPES */
typedef enum {MB_FIRMWARE=0, MB_BACKUP=1} memBlock_e;
/* The application's map (user_fun.c): image area, then the backup slot. */
static struct {uint32_t start,size;} mem[2]={{0,131072},{135168,131072}};
static uint8_t flash[135168+131072];
static unsigned saves,wdt,torn,sent,sent_code,sent_cch,trial,manual;
static uint32_t stuck=0xffffffffu;
static uint8_t bf[128];
static void mem_read(uint32_t a,uint8_t*p,uint16_t n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);}
/* NOR programming clears bits only. A torn program leaves the second half
 * erased; a stuck address never programs. */
static void mem_write(uint32_t a,uint8_t*p,uint16_t n){uint32_t i;CHECK(a+n<=sizeof(flash));
 for(i=0;i<n;i++){if(torn&&i>=n/2u)break;if(a+i==stuck)continue;flash[a+i]&=p[i];}if(torn)torn--;}
static void user_clearMemBlock(memBlock_e mb){memset(flash+mem[mb].start,0xff,mem[mb].size);}
static uint32_t crc_word(uint32_t c,uint32_t v){unsigned i;c^=v;for(i=0;i<32;i++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0);return c;}
static uint32_t stm_crc(const uint8_t*p,uint32_t n){uint32_t c=0xffffffffu,w,i;for(i=0;i<n;i+=4){memcpy(&w,p+i,4);c=crc_word(c,w);}return c;}
static bool user_checkFirmware(uint32_t adr,uint32_t size,uint32_t crc){return adr+size<=sizeof(flash)&&stm_crc(flash+adr,size)==crc;}
static void user_saveindex(void){saves++;}
static void wdt_reload(void){wdt++;}
static int app_trial_active(void){return trial;}
static int boot_manual_busy(void){return manual;}
static struct{int num;char id[11];}logs[64];static unsigned nlogs;
static bool log_addSystemEvent(logSystem_e num){CHECK(nlogs<64);logs[nlogs].num=num;logs[nlogs].id[0]=0;nlogs++;return true;}
static bool log_addSystemEventID(logSystem_e num,char*id){CHECK(nlogs<64);logs[nlogs].num=num;memcpy(logs[nlogs].id,id,10);logs[nlogs].id[10]=0;nlogs++;return true;}
static struct{uint8_t FWinProgress,rebootToUpdate,firmwarePercent,cchFWupdate,hardwareVersion,registredOnServer;
 uint16_t updateTimeout,FWblockIdx,FWblockCount;uint32_t startWaitFWtime;struct{uint16_t readyToUpdate:1;}status;}device;
static void user_send_statusUpdateFW(uint8_t cmd,uint8_t code,dataChanel_e cch,uint8_t uniqId){(void)cmd;(void)uniqId;sent++;sent_code=code;sent_cch=cch;}
static void fw_resume_after_boot(void);
/* PRODUCTION */

static uint8_t image[96*1024];
static void reset(void){memset(flash,0xff,sizeof(flash));memset(&device,0,sizeof(device));device.hardwareVersion=139;device.registredOnServer=1;
 saves=torn=sent=trial=manual=nlogs=0;stuck=0xffffffffu;boot_ver=0x32;}
static int last_log(int num,const char*id){return nlogs&&logs[nlogs-1].num==num&&!strcmp(logs[nlogs-1].id,id);}
static cmdCode_e start(uint16_t number,uint32_t build,uint32_t size,uint32_t crc,uint16_t blocks,dataChanel_e cch){
 uint8_t p[16];memcpy(p,&number,2);memcpy(p+2,&build,4);memcpy(p+6,&size,4);memcpy(p+10,&crc,4);memcpy(p+14,&blocks,2);return fw_start(p,cch);}
static uint8_t block(uint16_t k,uint16_t n,dataChanel_e cch){static uint8_t p[1030];uint32_t i;memcpy(p,&k,2);
 for(i=0;i<1024;i++)p[2+i]=(uint32_t)k*1024u+i<sizeof(image)?image[(uint32_t)k*1024u+i]:0xff;return fw_take_block(p,n,cch);}
static void make_image(uint32_t size){uint32_t seed=777u,i,w=0x4857008Bu;memset(image,0xff,sizeof(image));
 for(i=0;i<size;i++){seed=seed*1103515245u+12345u;image[i]=(uint8_t)(seed>>16);}
 memcpy(image+0x20,&w,4);}/* board marker of HW 139 in reserved vector 8 */

static void start_tests(void){uint32_t size=66716,crc;uint16_t blocks=66;
 make_image(size);crc=stm_crc(image,size);
 /* Field checks: number 0 reserved, rollback with zero fields, each broken field. */
 CHECK(fw_start_rejected(0,1,4,1,1));CHECK(!fw_start_rejected(1,0,0,0,0));CHECK(!fw_start_rejected(1,0xffffffff,3,0,0xffff));
 CHECK(!fw_start_rejected(64,1789454349,65016,0x57d1c3a7,64));CHECK(!fw_start_rejected(2,1,4,1,1));
 CHECK(!fw_start_rejected(64,1,67584,1,66));/* the table's padding to 1024 */
 CHECK(fw_start_rejected(64,0,65016,0x57d1c3a7,64));CHECK(fw_start_rejected(64,1789454349,0,0x57d1c3a7,64));
 CHECK(fw_start_rejected(64,1789454349,65015,0x57d1c3a7,64));CHECK(fw_start_rejected(64,1789454349,65016,0,64));
 CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,0));CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,63));
 CHECK(!fw_start_rejected(64,1,0x17e00,1,0x60));CHECK(fw_start_rejected(64,1,0x17e04,1,0x60));CHECK(fw_start_rejected(64,1,4,1,128));
 /* Server start: answer 0, bit 11, one 10-minute timeout, header staged, event 2 with the number, saved. */
 reset();flash[5000]=0;
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS);
 CHECK(device.FWinProgress&&device.status.readyToUpdate&&device.updateTimeout==600&&device.cchFWupdate==CH_SERVER);
 CHECK(device.FWblockIdx==0&&device.FWblockCount==blocks&&device.firmwarePercent==0&&saves==1&&flash[5000]==0xff);
 {bootFWparam_s h;memcpy(&h,flash,sizeof(h));CHECK(h.appIdent==0xffffffffu&&h.firmwareVersion==71&&h.buildDate==1789600000&&h.firmwareSize==size&&h.firmwareCRC==crc&&h.hardwareVersion==139);}
 CHECK(last_log(LOG_SYS_GOTOUPDATE,"71"));
 /* The same file again on the same channel carries on; the block is asked for at once. */
 device.FWblockIdx=9;device.startWaitFWtime=1234;saves=0;
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS&&device.FWblockIdx==9&&!device.startWaitFWtime&&!saves);
 /* Another file, or the same file over USB, during the download: 2 "BUSY", nothing touched. */
 CHECK(start(72,1789600000,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"BUSY")&&device.FWblockIdx==9);
 CHECK(start(71,1789600000,size,crc,blocks,CH_USB)==CMD_RUNERROR&&device.FWinProgress&&device.cchFWupdate==CH_SERVER);
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"BUSY"));
 /* Bad fields: 1 "PARAM". */
 CHECK(start(0,1,4,1,1,CH_SERVER)==CMD_PARAMERROR&&last_log(LOG_SYS_ERRUPDATE,"PARAM"));
 /* Not now: 2 with the reason, nothing erased. */
 reset();flash[5000]=0;boot_ver=0x21;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"OLD BOOT"));
 boot_ver=0x32;manual=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"BOOT BUSY"));
 manual=0;trial=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"IN TRIAL"));
 trial=0;device.rebootToUpdate=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"BUSY"));
 CHECK(flash[5000]==0&&!device.FWinProgress&&!device.status.readyToUpdate);
 /* USB start: NextConfig sends the blocks; no bit 11, no timeout, nothing saved. */
 reset();CHECK(start(71,1,size,crc,blocks,CH_USB)==CMD_SUCCESS&&!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!saves&&device.cchFWupdate==CH_USB);
 puts("command 19: fields, answer 0 with bit 11 and the 10-minute timeout, carry on with the same file, 2 with its reason, 1 PARAM, USB");}

static void rollback_tests(void){uint32_t size=66716;bootFWparam_s h;
 make_image(size);
 /* No backup: 2 "NO BACKUP". A backup failing its CRC: 2 "BAD BACKUP". */
 reset();CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"NO BACKUP")&&!device.rebootToUpdate);
 memset(&h,0xff,sizeof(h));h.appIdent=0x4E465744u;h.hardwareVersion=139;h.firmwareVersion=70;h.firmwareSize=size;h.firmwareCRC=stm_crc(image,size)^1u;h.buildDate=1789000000u;
 memcpy(flash+135168,&h,sizeof(h));memcpy(flash+135168+0x200,image,size);
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_SYS_ERRUPDATE,"BAD BACKUP")&&!device.rebootToUpdate);
 /* A good backup is staged in the image area with its magic erased, bit 11 set, restart requested. */
 h.firmwareCRC^=1u;memcpy(flash+135168,&h,sizeof(h));flash[100]=0;
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_SUCCESS&&device.rebootToUpdate&&device.status.readyToUpdate&&device.cchFWupdate==CH_SERVER);
 CHECK(!memcmp(flash+4,flash+135168+4,size+0x200-4)&&!memcmp(flash,"\xff\xff\xff\xff",4)&&last_log(LOG_SYS_GOTOUPDATE,"ROLLBACK"));
 puts("rollback: no backup, bad backup, staged copy with the magic erased");}

static void download(uint32_t size,uint16_t blocks,uint32_t crc){reset();
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS);}

static void block_tests(void){uint32_t size=66716,crc;uint16_t blocks=66,k;
 make_image(size);crc=stm_crc(image,size);
 /* A whole download: 0 for every block, percent follows, the last one requests the install. */
 download(size,blocks,crc);
 for(k=0;k<blocks;k++){device.startWaitFWtime=5;device.updateTimeout=5;CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);CHECK(!device.startWaitFWtime);
  /* Every accepted block restarts the limit; the last one ends the download. */
  CHECK(device.updateTimeout==(k+1u<blocks?600:0));
  CHECK(device.firmwarePercent==(k+1u)*100u/blocks);CHECK((k+1u<blocks)==(device.FWinProgress!=0));}
 CHECK(device.rebootToUpdate&&!device.status.readyToUpdate&&!device.updateTimeout&&device.firmwarePercent==100);
 CHECK(!memcmp(flash+0x200,image,(uint32_t)blocks*1024u));
 /* After the end a late block is confirmed with 0; one past the image gets 253. */
 CHECK(block(blocks-1,1026,CH_SERVER)==FW_BLOCK_OK&&block(blocks,1026,CH_SERVER)==FW_BLOCK_END);
 /* The table's padding to 1024 works the same way. */
 make_image(67584);download(67584,66,stm_crc(image,67584));for(k=0;k<66;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);CHECK(device.rebootToUpdate);
 make_image(size);
 /* Wrong length or a block ahead: 255, asked for again at once; a late copy: 0, nothing written. */
 download(size,blocks,crc);CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK&&block(1,1026,CH_SERVER)==FW_BLOCK_OK);
 device.startWaitFWtime=5;device.updateTimeout=5;CHECK(block(2,1025,CH_SERVER)==FW_BLOCK_BADFRAME&&!device.startWaitFWtime&&device.FWblockIdx==2);
 /* A rejected block does not extend the limit; a late copy does not either. */
 CHECK(device.updateTimeout==5);CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK&&device.updateTimeout==5);
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_BADFRAME&&device.FWblockIdx==2&&flash[0x200+3*1024]==0xff);
 CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK&&device.FWblockIdx==2);
 /* A block on the other channel: 253, the download goes on. */
 CHECK(block(2,1026,CH_USB)==FW_BLOCK_END&&device.FWinProgress&&device.FWblockIdx==2&&flash[0x200+2*1024]==0xff);
 /* A torn program: 254, the same block again completes it. */
 torn=1;CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_BADWRITE&&device.FWblockIdx==2);CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_OK&&device.FWblockIdx==3);
 CHECK(!memcmp(flash+0x200+2*1024,image+2*1024,1024));
 /* A byte that never programs: 254 twice, then 253 and "WRITE", the download is over. */
 stuck=0x200+3*1024+10;nlogs=0;
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_BADWRITE&&block(3,1026,CH_SERVER)==FW_BLOCK_BADWRITE);
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_END&&last_log(LOG_SYS_ERRUPDATE,"WRITE")&&!device.FWinProgress&&!device.status.readyToUpdate);
 CHECK(block(4,1026,CH_SERVER)==FW_BLOCK_END);
 /* Errors do not add up across good blocks. */
 download(size,blocks,crc);torn=1;CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK);
 torn=1;CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK);
 torn=1;CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_OK);
 /* A foreign board in block 0: 253 and event 6, nothing written. */
 download(size,blocks,crc);image[0x20]=0x0B;CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_END);image[0x20]=0x8B;
 CHECK(nlogs&&logs[nlogs-1].num==LOG_SYS_HWERRUPDATE&&!device.FWinProgress&&!device.status.readyToUpdate&&flash[0x200+0x20]==0xff);
 /* A bad image CRC after the last block: 253 and "CRC", no install. */
 download(size,blocks,crc^1u);for(k=0;k+1<blocks;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
 CHECK(block(blocks-1,1026,CH_SERVER)==FW_BLOCK_END&&last_log(LOG_SYS_ERRUPDATE,"CRC")&&!device.rebootToUpdate&&!device.status.readyToUpdate);
 /* USB: blocks from NextConfig, the same codes. */
 reset();CHECK(start(71,1,size,crc,blocks,CH_USB)==CMD_SUCCESS);for(k=0;k<blocks;k++)CHECK(block(k,1026,CH_USB)==FW_BLOCK_OK&&!device.updateTimeout);
 CHECK(device.rebootToUpdate&&!saves);
 /* No download after a start: blocks are not taken. */
 reset();device.FWblockCount=66;fw_resume_after_boot();
 CHECK(block(0,1026,CH_USB)==FW_BLOCK_END&&block(0,1026,CH_SERVER)==FW_BLOCK_END&&flash[0x200]==0xff);
 puts("command 20: codes 0, 253, 254, 255, late copies, other channel, read-back, foreign board, image CRC, USB, no download");}

static void end_tests(void){
 /* The controller ends a download: 253 on its channel, reason in the journal, bit 11 cleared, saved. */
 download(66716,66,1);saves=0;sent=0;fw_end("TIMEOUT",7);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!device.FWblockIdx&&!device.FWblockCount&&saves==1);
 CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_END);
 CHECK(sent==1&&sent_code==FW_BLOCK_END&&sent_cch==CH_SERVER&&last_log(LOG_SYS_ERRUPDATE,"TIMEOUT"));
 download(66716,66,1);device.registredOnServer=0;sent=0;fw_end("TIMEOUT",7);CHECK(!sent&&!device.FWinProgress);
 /* Bit 11 is the update mode. */
 device.status.readyToUpdate=0;CHECK(!fw_update_quiet());device.status.readyToUpdate=1;CHECK(fw_update_quiet());
 /* Board marker in reserved vector 8, little-endian. */
 {uint8_t b[1024];uint32_t w;memset(b,0xA5,sizeof b);
  w=0x4857008Bu;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,139));CHECK(fw_image_foreign(b,11));CHECK(!fw_image_foreign(b,0));
  w=0;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,11));w=0x4857018Bu;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,11));}
 /* Command 8 source to the journal event (E184). */
 CHECK(server_out_event==LOG_CON_OUTONBYAPI);
 CHECK(cmd8_event(1)==52&&cmd8_event(2)==50&&cmd8_event(3)==49&&cmd8_event(4)==51&&cmd8_event(0)==50&&cmd8_event(5)==50&&cmd8_event(255)==50);
 puts("controller ends a download, update mode, board marker, command 8 sources");}

static void stage(uint16_t blocks,uint16_t version){bootFWparam_s h;reset();memset(&h,0xff,sizeof(h));h.hardwareVersion=139;h.firmwareVersion=version;
 h.firmwareSize=(uint32_t)blocks*1024u-8u;h.firmwareCRC=0x12345678u;h.buildDate=1789454349u;memcpy(flash,&h,sizeof(h));
 device.FWinProgress=1;device.FWblockCount=blocks;device.cchFWupdate=0;saves=0;}
static void resume_tests(void){uint32_t seed=12345u,i;uint16_t blocks=37,k,idx;unsigned cuts=0;
 static const uint16_t offs[]={0,1,63,64,255,256,700,1023,1024};
 for(i=0;i<sizeof(image);i++){seed=seed*1103515245u+12345u;image[i]=(uint8_t)(seed>>16);}
 memset(image+5*1024,0xff,1024);memset(image+6*1024,0xff,300);memset(image+36*1024+1016,0xff,8);/* legitimate FF block, FF prefix, FF tail */
 for(k=0;k<blocks;k++){unsigned o;for(o=0;o<sizeof(offs)/sizeof(offs[0]);o++){uint32_t base=0x200u+(uint32_t)k*1024u;
  stage(blocks,64);
  memcpy(flash+0x200,image,(uint32_t)k*1024u);/* blocks before k complete */
  memcpy(flash+base,image+base-0x200,offs[o]);/* block k torn after offs[o] bytes */
  if(offs[o]<1024)flash[base+offs[o]]&=(uint8_t)(image[base-0x200+offs[o]]|0x5a);/* one byte with bits still set */
  fw_resume_after_boot();idx=device.FWblockIdx;
  CHECK(device.FWinProgress&&device.status.readyToUpdate&&device.updateTimeout==600&&device.cchFWupdate==CH_SERVER&&!saves);
  CHECK(idx<=k);/* never past a torn or missing block */
  for(i=(uint32_t)idx*1024u;i<(uint32_t)blocks*1024u;i++)flash[0x200+i]&=image[i];/* server sends idx..end */
  CHECK(!memcmp(flash+0x200,image,(uint32_t)blocks*1024u));cuts++;}}
 /* Nothing written: start from 0. Everything written: request the last block again. */
 stage(blocks,64);fw_resume_after_boot();CHECK(device.FWblockIdx==0&&device.firmwarePercent==0);
 stage(blocks,64);memcpy(flash+0x200,image,(uint32_t)blocks*1024u);fw_resume_after_boot();CHECK(device.FWblockIdx==blocks-1);
 /* No download in progress: nothing read back, nothing saved, a stale timeout is dropped, no block is taken. */
 stage(blocks,64);device.FWinProgress=0;device.updateTimeout=900;fw_resume_after_boot();
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!saves&&!device.updateTimeout&&!device.FWblockIdx&&!device.FWblockCount);
 /* The saved remaining time is kept; missing or impossible values restart at 10 minutes. */
 stage(blocks,64);device.updateTimeout=150;fw_resume_after_boot();CHECK(device.FWinProgress&&device.updateTimeout==150);
 stage(blocks,64);device.updateTimeout=3000;fw_resume_after_boot();CHECK(device.updateTimeout==600);
 /* A header that is not a download ends the update and saves that. */
 stage(blocks,1);fw_resume_after_boot();CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&saves==1&&!device.FWblockIdx&&!device.FWblockCount);
 stage(blocks,64);{uint32_t magic=0x32544F42u;memcpy(flash,&magic,4);}fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(blocks,64);device.FWblockCount=1;fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(0xffff,64);memset(flash,0xff,sizeof(flash));fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(blocks,64);boot_ver=0x21;fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);boot_ver=0x32;
 printf("resume: %u power cuts over %u blocks restore the image byte for byte\n",cuts,blocks);}

/* Доработка: Кряжев А., 2026. Проверка реального цикла при медленном сервере. */
static uint32_t audit_ms;
static unsigned audit_requests;
static uint8_t uniqIDCnt;
static uint32_t timers_getMillis(void){return audit_ms;}
static void user_getFWblockFromServer(dataChanel_e ch,uint16_t idx){
 CHECK(ch==CH_SERVER&&idx==device.FWblockIdx);audit_requests++;
}
/* AUDIT LOOP */
static void audit_seconds(unsigned n){while(n--){audit_ms+=1000;audit_poll();audit_tick();}}
static void slow_server_tests(void){
 uint16_t k;uint32_t size=68784,crc;unsigned requests;
 reset();make_image(size);crc=stm_crc(image,size);audit_ms=1000;audit_requests=0;
 CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 for(k=0;k<68;k++){
  audit_seconds(15);CHECK(device.FWinProgress&&device.FWblockIdx==k);
  if(k==20){
   device.registredOnServer=0;requests=audit_requests;audit_seconds(120);
   CHECK(audit_requests==requests&&device.FWblockIdx==20&&device.cchFWupdate==CH_SERVER);
   device.registredOnServer=1;audit_seconds(2);
  }
  CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
  if(k<67){
   audit_seconds(1);requests=audit_requests;
   CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
   CHECK(device.FWblockIdx==k+1&&device.updateTimeout==599);
   audit_poll();CHECK(audit_requests==requests);
  }
 }
 CHECK(device.rebootToUpdate&&!device.updateTimeout&&!device.FWinProgress);
 CHECK(!memcmp(flash+FWBLOCK_OFFSET,image,size));
 reset();CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 audit_seconds(599);CHECK(device.FWinProgress&&device.updateTimeout==1&&saves==10);
 audit_seconds(1);CHECK(!device.FWinProgress&&!device.updateTimeout&&sent_code==FW_BLOCK_END&&sent==1);
 puts("audit: slow 68-block download, connection pause, duplicates, periodic saves and silence timeout passed");
}

int main(void){slow_server_tests();start_tests();rollback_tests();block_tests();end_tests();resume_tests();
 puts("PASS firmware update of sheet Protokol2: commands 19, 20 and 4, rollback, end of a download, resume after a reset");return 0;}
