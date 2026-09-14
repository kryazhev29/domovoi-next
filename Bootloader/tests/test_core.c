#include "boot_core.h"
#include <stdlib.h>
#define assert(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
static unsigned char ext[BOOT_BACKUP+BOOT_SLOT_SIZE],flash[BOOT_FLASH_END-BOOT_APP_BASE];
static jmp_buf reset;
static unsigned mutations,cut;
static int fail_program,fail_backup,partial;
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
int boot_ext_write(uint32_t a,const void*p,uint32_t n){assert(a+n<=sizeof(ext));if(fail_backup&&a>=BOOT_BACKUP)return 0;return program(ext+a,p,n);}
int boot_ext_erase(uint32_t a){assert(!(a&4095)&&a+4096<=sizeof(ext));memset(ext+a,255,(partial&&cut&&mutations+1==cut)?2048:4096);checkpoint();return 1;}
int boot_int_erase(uint32_t a){assert(a>=BOOT_APP_BASE&&a+1024<=BOOT_FLASH_END&&!(a&1023));memset(flash+a-BOOT_APP_BASE,255,(partial&&cut&&mutations+1==cut)?512:1024);checkpoint();return 1;}
int boot_int_write(uint32_t a,const void*p,uint32_t n){assert(a>=BOOT_APP_BASE&&a+n<=BOOT_FLASH_END);if(fail_program){fail_program=0;return 0;}return program(flash+a-BOOT_APP_BASE,p,n);}
static void image(unsigned char *p,unsigned version,uint32_t size){
 BootHeader h;uint32_t i,w,crc=0xffffffffu;
 memset(&h,0,sizeof(h));h.magic=BOOT_MAGIC;h.hardware=BOOT_HW_VERSION;h.version=(uint16_t)version;h.size=size;
 memset(p,255,BOOT_HEADER_SIZE);
 for(i=0;i<size;i+=4){w=i==0?0x20005000u:(i==4?BOOT_APP_VECTOR+9u:i*17u+version);memcpy(p+BOOT_HEADER_SIZE+i,&w,4);crc=boot_crc_word(crc,w);}
 h.crc=crc;memcpy(p,&h,sizeof(h));
}
static void initial(void){memset(ext,255,sizeof(ext));memset(flash,255,sizeof(flash));image(flash,14,8192);image(ext,15,12288);mutations=cut=0;fail_program=fail_backup=partial=0;}
static int run(void){if(setjmp(reset))return -1;return boot_run();}
static unsigned version(void){BootHeader h;assert(boot_validate(0,BOOT_APP_BASE,&h));return h.version;}
int main(void){
 unsigned total,k,mode;int result;BootHeader h;
 /* A legacy header's HW byte cannot identify a physical board. Preserve the
    existing application across manual boot replacement, with CRC/vectors intact. */
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
 /* Backup cannot be written: the intact application keeps running, the request
    is dropped instead of being retried on every start, and the FW gets a code. */
 initial();fail_backup=1;assert(run()==BOOT_BACKUP_ERROR);fail_backup=0;assert(version()==14);
 {uint32_t error;assert(boot_read(1,BOOT_JOURNAL+12,&error,4)&&error==BOOT_BACKUP_ERROR);}
 total=mutations;assert(run()==BOOT_OK&&mutations==total&&version()==14);
 puts("backup failure: application kept, request dropped, error recorded");
 /* The next update erases the journal. A cut inside that erase can leave the
    completion word erased while the start or rollback word survives. The new
    candidate is not published yet; the intact application must stay. */
 for(k=0;k<2;k++){
  initial();assert(run()==BOOT_OK&&version()==15);
  image(ext,16,4096);((BootHeader*)ext)->magic=0xffffffffu;
  memset(ext+BOOT_JOURNAL+4,255,4);if(k)memcpy(ext+BOOT_JOURNAL+8,"\x0f\xff\xf0\xff",4);
  total=mutations;result=run();assert(version()==15);assert(result==BOOT_OK);assert(mutations==total);
 }
 puts("torn journal erase: running application kept, backup not restored");
 initial();ext[BOOT_HEADER_SIZE+44]^=1;assert(run()==BOOT_OK&&mutations==0&&version()==14);
 initial();ext[4]++;assert(run()==BOOT_OK&&mutations==0&&version()==14);
 initial();((BootHeader*)ext)->size=BOOT_MAX_CODE+4;assert(run()==BOOT_OK&&mutations==0);
 initial();((BootHeader*)ext)->size=9;assert(run()==BOOT_OK&&mutations==0);
 initial();memset(flash,255,sizeof(flash));assert(run()==BOOT_OK&&version()==15);
 initial();memset(flash,255,sizeof(flash));fail_program=1;assert(run()==BOOT_RECOVERY);
 /* A complete new upload can escape a previous failed recovery. */
 assert(boot_prepare()==BOOT_OK);image(ext,16,4096);assert(boot_request()==BOOT_OK);assert(run()==BOOT_OK&&version()==16);
 initial();assert(run()==BOOT_OK);image(ext,17,BOOT_MAX_CODE);assert(boot_request()==BOOT_OK);assert(run()==BOOT_OK&&version()==17);
 assert(boot_parse_filename("139.14.1726300000.bin",&h)&&h.version==14&&h.timestamp==1726300000u);
 assert(!boot_parse_filename("139.65536.1.bin",&h));assert(!boot_parse_filename("139.1.4294967296.bin",&h));
 assert(!boot_parse_filename("0.1.1.bin",&h));assert(!boot_parse_filename("139..1.bin",&h));assert(!boot_parse_filename("139.1.1.bin.bak",&h));
 puts("CRC, HW, size, empty flash, recovery retry, maximum image and filename checks passed");return 0;
}
