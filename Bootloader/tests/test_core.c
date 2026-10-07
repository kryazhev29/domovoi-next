#include "boot_core.h"
#include <stdlib.h>
#define assert(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
static unsigned char ext[BOOT_BACKUP+BOOT_SLOT_SIZE],flash[BOOT_FLASH_END-BOOT_APP_BASE];
static jmp_buf reset;
static unsigned mutations,cut;
static int fail_program,fail_backup,partial,trial_image;
static uint32_t reject_a,reject_b;
void boot_service(void){}
static void checkpoint(void){++mutations;if(cut&&mutations==cut)longjmp(reset,1);}
int boot_read(int e,uint32_t a,void *p,uint32_t n){
 if(e){assert(a+n<=sizeof(ext));memcpy(p,ext+a,n);}
 else{assert(a>=BOOT_APP_BASE&&a+n<=BOOT_FLASH_END);memcpy(p,flash+a-BOOT_APP_BASE,n);}return 1;
}
static int program(unsigned char *to,const void *from,uint32_t n){
 const unsigned char *p=from;uint32_t i,limit=n;
 if(partial&&cut&&mutations+1==cut)limit=n/2;
 for(i=0;i<limit;i++){assert((to[i]&p[i])==p[i]);to[i]&=p[i];}
 checkpoint();return 1;
}
int boot_ext_write(uint32_t a,const void*p,uint32_t n){assert(a+n<=sizeof(ext));if(fail_backup&&a>=BOOT_BACKUP)return 0;if(a&&(a==reject_a||a==reject_b))return 0;return program(ext+a,p,n);}
int boot_ext_erase(uint32_t a){assert(!(a&4095)&&a+4096<=sizeof(ext));memset(ext+a,255,(partial&&cut&&mutations+1==cut)?2048:4096);checkpoint();return 1;}
int boot_int_erase(uint32_t a){assert(a>=BOOT_APP_BASE&&a+1024<=BOOT_FLASH_END&&!(a&1023));memset(flash+a-BOOT_APP_BASE,255,(partial&&cut&&mutations+1==cut)?512:1024);checkpoint();return 1;}
int boot_int_write(uint32_t a,const void*p,uint32_t n){assert(a>=BOOT_APP_BASE&&a+n<=BOOT_FLASH_END);if(fail_program){fail_program=0;return 0;}return program(flash+a-BOOT_APP_BASE,p,n);}
static void image(unsigned char *p,unsigned version,uint32_t size){
 BootHeader h;uint32_t i,w,crc=0xffffffffu;
 memset(&h,0,sizeof(h));h.magic=BOOT_MAGIC;h.hardware=BOOT_HW_VERSION;h.version=(uint16_t)version;h.size=size;
 memset(p,255,BOOT_HEADER_SIZE);
 for(i=0;i<size;i+=4){w=i==0?0x20005000u:(i==4?BOOT_APP_VECTOR+9u:(i==BOOT_TRIAL_VECTOR&&trial_image?BOOT_TRIAL_MARKER:i*17u+version));memcpy(p+BOOT_HEADER_SIZE+i,&w,4);crc=boot_crc_word(crc,w);}
 h.crc=crc;memcpy(p,&h,sizeof(h));
}
static void initial(void){trial_image=0;reject_a=reject_b=0;memset(ext,255,sizeof(ext));memset(flash,255,sizeof(flash));image(flash,14,8192);image(ext,15,12288);mutations=cut=0;fail_program=fail_backup=partial=0;}

static void trial_initial(void){initial();trial_image=1;image(ext,15,12288);trial_image=0;}
static int run(void){if(setjmp(reset))return -1;return boot_run();}
static unsigned version(void){BootHeader h;assert(boot_validate(0,BOOT_APP_BASE,&h));return h.version;}
static uint32_t journal(uint32_t offset){uint32_t v;assert(boot_read(1,BOOT_JOURNAL+offset,&v,4));return v;}

static void app_word(uint32_t offset,uint32_t value){unsigned char b[4];unsigned i;memcpy(b,&value,4);for(i=0;i<4;i++)ext[BOOT_JOURNAL+offset+i]&=b[i];}

static void trial_settles(uint32_t first,uint32_t request){
 unsigned i,launches=0;int r,torn=partial;
 for(i=0;i<16;i++){
  if(i==first&&request)app_word(BOOT_JOURNAL_ROLLBACK,0);
  r=run();if(r==-1){cut=0;continue;}
  assert(r==BOOT_OK||r==BOOT_RESTORED);
  if(version()==15&&journal(12)==0xffffffffu)launches++;
 }

 assert(version()==14);assert(journal(12)!=0xffffffffu);assert(torn||journal(12)==BOOT_TRIAL_FAILED);
 assert(launches<=BOOT_TRIAL_LAUNCHES+1u);
}
int main(void){
 unsigned total,k,mode;int result;BootHeader h;

 initial();((BootHeader*)flash)->magic=BOOT_LEGACY_MAGIC;((BootHeader*)flash)->hardware=0;
 assert(boot_validate(0,BOOT_APP_BASE,&h));
 assert(run()==BOOT_OK);assert(boot_validate(1,BOOT_BACKUP,&h)&&h.hardware==0);
 initial();((BootHeader*)flash)->hardware=0;assert(!boot_validate(0,BOOT_APP_BASE,&h));
 initial();assert(run()==BOOT_OK);assert(version()==15);assert(boot_validate(1,BOOT_BACKUP,&h)&&h.version==14);total=mutations;
 for(mode=0;mode<2;mode++)for(k=1;k<=total;k++){
  initial();cut=k;partial=(int)mode;assert(run()==-1);cut=0;result=run();
  assert(result==BOOT_OK||result==BOOT_RESTORED);assert(version()==15||version()==14);
 }
 printf("install: %u complete/partial power-cut positions passed\n",total*2);
 initial();fail_program=1;assert(run()==BOOT_RESTORED);assert(version()==14);total=mutations;
 for(mode=0;mode<2;mode++)for(k=1;k<=total;k++){
  initial();fail_program=1;cut=k;partial=(int)mode;assert(run()==-1);cut=0;fail_program=0;
  result=run();assert(result==BOOT_OK||result==BOOT_RESTORED);assert(version()==14||version()==15);
 }
 printf("rollback: %u complete/partial power-cut positions passed\n",total*2);

 initial();fail_backup=1;assert(run()==BOOT_BACKUP_ERROR);fail_backup=0;assert(version()==14);
 {uint32_t error;assert(boot_read(1,BOOT_JOURNAL+12,&error,4)&&error==BOOT_BACKUP_ERROR);}
 total=mutations;assert(run()==BOOT_OK&&mutations==total&&version()==14);
 puts("backup failure: application kept, request dropped, error recorded");

 for(k=0;k<2;k++){
  initial();assert(run()==BOOT_OK&&version()==15);
  image(ext,16,4096);((BootHeader*)ext)->magic=0xffffffffu;
  memset(ext+BOOT_JOURNAL+4,255,4);if(k)memcpy(ext+BOOT_JOURNAL+8,"\x0f\xff\xf0\xff",4);
  total=mutations;result=run();assert(version()==15);assert(result==BOOT_OK);assert(mutations==total);
 }
 puts("torn journal erase: running application kept, backup not restored");

 trial_initial();assert(run()==BOOT_OK&&version()==15);
 assert(journal(BOOT_JOURNAL_TRIAL)==0&&journal(BOOT_JOURNAL_LAUNCHES)==0xfffffffeu);
 assert(run()==BOOT_OK&&run()==BOOT_OK&&version()==15&&journal(BOOT_JOURNAL_LAUNCHES)==0xfffffff8u);
 assert(run()==BOOT_RESTORED&&version()==14&&journal(12)==BOOT_TRIAL_FAILED);
 total=mutations;for(k=0;k<5;k++){assert(run()==BOOT_OK);assert(version()==14);}assert(mutations==total);

 trial_initial();assert(run()==BOOT_OK);app_word(BOOT_JOURNAL_CONFIRMED,0);
 total=mutations;for(k=0;k<8;k++){assert(run()==BOOT_OK);assert(version()==15);}assert(mutations==total);
 trial_initial();assert(run()==BOOT_OK);app_word(BOOT_JOURNAL_CONFIRMED,0x00ff0000u);
 assert(run()==BOOT_OK&&run()==BOOT_OK&&run()==BOOT_RESTORED&&version()==14);

 trial_initial();assert(run()==BOOT_OK);app_word(BOOT_JOURNAL_ROLLBACK,0xfffeffffu);
 assert(run()==BOOT_RESTORED&&version()==14&&journal(12)==BOOT_TRIAL_FAILED);

 initial();assert(run()==BOOT_OK&&journal(BOOT_JOURNAL_TRIAL)==0xffffffffu);
 total=mutations;for(k=0;k<8;k++){assert(run()==BOOT_OK);assert(version()==15);}assert(mutations==total);
 trial_initial();memset(flash,255,sizeof(flash));assert(run()==BOOT_OK&&version()==15);
 assert(journal(0)==0x4e4f424bu&&journal(BOOT_JOURNAL_TRIAL)==0xffffffffu);

 trial_initial();assert(run()==BOOT_OK);ext[BOOT_BACKUP+BOOT_HEADER_SIZE+40]^=1;
 for(k=0;k<6;k++){assert(run()==BOOT_OK);assert(version()==15);}
 puts("trial: launches, confirmation, torn confirmation, request, no marker, no backup, damaged backup");

 for(k=0;k<2;k++){
  trial_initial();assert(run()==BOOT_OK&&run()==BOOT_OK&&run()==BOOT_OK);app_word(BOOT_JOURNAL_CONFIRMED,0);
  assert(boot_prepare()==BOOT_OK);image(ext,16,4096);((BootHeader*)ext)->magic=0xffffffffu;
  if(k)memset(ext+BOOT_JOURNAL+BOOT_JOURNAL_CONFIRMED,255,4);else ext[BOOT_JOURNAL+BOOT_JOURNAL_CONFIRMED]|=1;
  total=mutations;result=run();assert(version()==15);assert(result==BOOT_OK);assert(mutations==total);
 }

 trial_initial();assert(run()==BOOT_OK);total=mutations;
 assert(boot_prepare()==BOOT_TRIAL_BUSY);assert(mutations==total&&((BootHeader*)ext)->magic==0);
 assert(run()==BOOT_OK&&run()==BOOT_OK&&run()==BOOT_RESTORED&&version()==14);
 assert(boot_prepare()==BOOT_OK);
 trial_initial();assert(run()==BOOT_OK);app_word(BOOT_JOURNAL_CONFIRMED,0);assert(boot_prepare()==BOOT_OK);
 trial_initial();assert(run()==BOOT_OK);memset(flash+BOOT_HEADER_SIZE+64,255,1024);assert(boot_prepare()==BOOT_OK);

 trial_initial();assert(run()==BOOT_OK);reject_a=BOOT_JOURNAL+BOOT_JOURNAL_LAUNCHES;
 assert(run()==BOOT_RESTORED&&version()==14&&journal(12)==BOOT_TRIAL_FAILED);
 trial_initial();assert(run()==BOOT_OK);reject_a=BOOT_JOURNAL+BOOT_JOURNAL_LAUNCHES;reject_b=BOOT_JOURNAL+12;
 assert(run()==BOOT_IO&&version()==14);total=mutations;
 for(k=0;k<6;k++){assert(run()==BOOT_IO);assert(version()==14);}assert(mutations==total);
 puts("trial: torn journal erase after confirmation, upload refused during trial, unrecordable launch");

 trial_initial();trial_settles(99,0);total=mutations;
 for(mode=0;mode<2;mode++)for(k=1;k<=total;k++){trial_initial();cut=k;partial=(int)mode;trial_settles(99,0);}
 printf("trial exhausted: %u complete/partial power-cut positions passed\n",total*2);
 trial_initial();trial_settles(2,1);total=mutations;
 for(mode=0;mode<2;mode++)for(k=1;k<=total;k++){trial_initial();cut=k;partial=(int)mode;trial_settles(2,1);}
 printf("trial rollback request: %u complete/partial power-cut positions passed\n",total*2);
 initial();ext[BOOT_HEADER_SIZE+44]^=1;assert(run()==BOOT_OK&&mutations==0&&version()==14);
 initial();ext[4]++;assert(run()==BOOT_OK&&mutations==0&&version()==14);
 initial();((BootHeader*)ext)->size=BOOT_MAX_CODE+4;assert(run()==BOOT_OK&&mutations==0);
 initial();((BootHeader*)ext)->size=9;assert(run()==BOOT_OK&&mutations==0);
 initial();memset(flash,255,sizeof(flash));assert(run()==BOOT_OK&&version()==15);
 initial();memset(flash,255,sizeof(flash));fail_program=1;assert(run()==BOOT_RECOVERY);

 assert(boot_prepare()==BOOT_OK);image(ext,16,4096);assert(boot_request()==BOOT_OK);assert(run()==BOOT_OK&&version()==16);
 initial();assert(run()==BOOT_OK);image(ext,17,BOOT_MAX_CODE);assert(boot_request()==BOOT_OK);assert(run()==BOOT_OK&&version()==17);
 assert(boot_parse_filename("139.14.1726300000.bin",&h)&&h.version==14&&h.timestamp==1726300000u);
 assert(!boot_parse_filename("139.65536.1.bin",&h));assert(!boot_parse_filename("139.1.4294967296.bin",&h));
 assert(!boot_parse_filename("0.1.1.bin",&h));assert(!boot_parse_filename("139..1.bin",&h));assert(!boot_parse_filename("139.1.1.bin.bak",&h));
 puts("CRC, HW, size, empty flash, recovery retry, maximum image and filename checks passed");return 0;
}
