#include "boot_core.h"
#include <string.h>
static uint32_t buffer[64], verify[64];
typedef char header_size_must_be_32[(sizeof(BootHeader)==32)?1:-1];
uint32_t boot_crc_word(uint32_t crc,uint32_t word) {
 unsigned i; crc^=word;
 for(i=0;i<32;i++) crc=(crc<<1)^((crc&0x80000000u)?0x04c11db7u:0u);
 return crc;
}
static int read_word(uint32_t address,uint32_t *value) {
 return boot_read(1,address,value,4);
}
static int write_word(uint32_t address,uint32_t value) {
 uint32_t actual;
 return boot_ext_write(address,&value,4)&&read_word(address,&actual)&&actual==value;
}
static int header_ok(const BootHeader *h) {
 return (h->magic==BOOT_MAGIC||h->magic==BOOT_LEGACY_MAGIC)&&
 (h->magic==BOOT_LEGACY_MAGIC||h->hardware==BOOT_HW_VERSION)&&h->size>=8&&h->size<=BOOT_MAX_CODE&&!(h->size&3);
}
int boot_validate(int external,uint32_t base,BootHeader *h) {
 uint32_t pos,n,i,crc=0xffffffffu,sp,pc;
 if(!boot_read(external,base,h,sizeof(*h))||!header_ok(h)) return 0;
 if(!boot_read(external,base+BOOT_HEADER_SIZE,buffer,8)) return 0;
 sp=buffer[0];pc=buffer[1];
 if(sp<0x20000008u||sp>0x20005000u||(sp&7)||!(pc&1)||
    (pc&~1u)<BOOT_APP_VECTOR||(pc&~1u)>=BOOT_APP_VECTOR+h->size) return 0;
 for(pos=0;pos<h->size;pos+=n) {
  n=h->size-pos;if(n>sizeof(buffer)) n=sizeof(buffer);
  if(!boot_read(external,base+BOOT_HEADER_SIZE+pos,buffer,n)) return 0;
  for(i=0;i<n/4;i++) crc=boot_crc_word(crc,buffer[i]);
  boot_service();
 }
 return crc==h->crc;
}
static int snapshot(void) {
 BootHeader h,check; uint32_t pos,n;
 if(!boot_validate(0,BOOT_APP_BASE,&h)) return 0;
 for(pos=0;pos<h.size+BOOT_HEADER_SIZE;pos+=4096)
  if(!boot_ext_erase(BOOT_BACKUP+pos)) return 0;
 /* Copy the existing header at offset zero, not before an extra header. */
 for(pos=0;pos<h.size+BOOT_HEADER_SIZE;pos+=n) {
  n=h.size+BOOT_HEADER_SIZE-pos;if(n>sizeof(buffer)) n=sizeof(buffer);
  if(!boot_read(0,BOOT_APP_BASE+pos,buffer,n)||
     !boot_ext_write(BOOT_BACKUP+pos,buffer,n)||
     !boot_read(1,BOOT_BACKUP+pos,verify,n)||memcmp(buffer,verify,n)) return 0;
  boot_service();
 }
 return boot_validate(1,BOOT_BACKUP,&check);
}
static int install(uint32_t base) {
 BootHeader h,check; uint32_t pos,n;
 if(!boot_validate(1,base,&h)) return 0;
 for(pos=0;pos<h.size+BOOT_HEADER_SIZE;pos+=1024)
  if(!boot_int_erase(BOOT_APP_BASE+pos)) return 0;
 /* Header is written last: incomplete images never become bootable. */
 for(pos=BOOT_HEADER_SIZE;pos<h.size+BOOT_HEADER_SIZE;pos+=n) {
  n=h.size+BOOT_HEADER_SIZE-pos;if(n>sizeof(buffer)) n=sizeof(buffer);
  if(!boot_read(1,base+pos,buffer,n)||!boot_int_write(BOOT_APP_BASE+pos,buffer,n)) return 0;
  boot_service();
 }
 for(pos=0;pos<BOOT_HEADER_SIZE;pos+=sizeof(buffer))
  if(!boot_read(1,base+pos,buffer,sizeof(buffer))||
     !boot_int_write(BOOT_APP_BASE+pos,buffer,sizeof(buffer))) return 0;
 return boot_validate(0,BOOT_APP_BASE,&check);
}
/* Start a new transaction only while a valid application is still intact.
 * A recovery session may also replace a damaged candidate, keeping the journal
 * and last known good backup until it has a complete replacement. */
int boot_prepare(void) {
 uint32_t pos;
 for(pos=0;pos<BOOT_SLOT_SIZE;pos+=4096)
  if(!boot_ext_erase(BOOT_CANDIDATE+pos)) return BOOT_IO;
 return BOOT_OK;
}
int boot_stage_header(const BootHeader *h) {
 BootHeader stored=*h;
 if(!header_ok(h)) return BOOT_BAD_IMAGE;
 stored.magic=0xffffffffu;
 return boot_ext_write(BOOT_CANDIDATE,&stored,sizeof(stored))?BOOT_OK:BOOT_IO;
}
int boot_request(void) {
 BootHeader h;
 /* Clear the previous transaction before publishing a new candidate.
    On reset in between, the current application/backup remains intact. */
 if(!boot_ext_erase(BOOT_JOURNAL)) return BOOT_IO;
 if(!write_word(BOOT_CANDIDATE,BOOT_MAGIC)||!boot_validate(1,BOOT_CANDIDATE,&h)) return BOOT_BAD_IMAGE;
 return BOOT_OK;
}

static void cleanup_failed_candidate(void) {
 uint32_t error,clean,pos;
 if(!read_word(BOOT_JOURNAL+12,&error)||error!=BOOT_PROGRAM_ERROR||
    !read_word(BOOT_JOURNAL+16,&clean)||clean==0) return;
 for(pos=0;pos<BOOT_SLOT_SIZE;pos+=4096)
  if(!boot_ext_erase(BOOT_CANDIDATE+pos)) return;
 write_word(BOOT_JOURNAL+16,0);
}
int boot_run(void) {
 BootHeader h; uint32_t started,done,rollback;
 int candidate,app,backup;
 if(!read_word(BOOT_JOURNAL,&started)||!read_word(BOOT_JOURNAL+4,&done)||
    !read_word(BOOT_JOURNAL+8,&rollback)) return BOOT_IO;
 app=boot_validate(0,BOOT_APP_BASE,&h);
 if(done!=0xffffffffu) {
  /* Any torn completion marker follows a fully verified installation. */
  if(app) { cleanup_failed_candidate();return BOOT_OK; }
  return BOOT_RECOVERY;
 }
 candidate=boot_validate(1,BOOT_CANDIDATE,&h)&&h.magic==BOOT_MAGIC;
 if(started==0xffffffffu) {
  if(!candidate) return app?BOOT_OK:BOOT_RECOVERY;
  if(app&&!snapshot()) return BOOT_BACKUP_ERROR;
  backup=app||boot_validate(1,BOOT_BACKUP,&h);
  /* Record whether a backup is available before touching internal flash. */
  if(!write_word(BOOT_JOURNAL,backup?0x494e5354u:0x4e4f424bu)) return BOOT_IO;
  started=backup?0x494e5354u:0x4e4f424bu;
 }
 if(rollback==0xffffffffu&&candidate&&install(BOOT_CANDIDATE)) {
  if(!write_word(BOOT_JOURNAL+4,0)) return BOOT_IO;
  write_word(BOOT_CANDIDATE,0);
  return BOOT_OK;
 }
 /* A torn start marker has not allowed an erase, but restoring a validated
    backup is also safe. Never overwrite a backup while recovering. */
 if(!write_word(BOOT_JOURNAL+8,0)) return BOOT_IO;
 if(started!=0x4e4f424bu&&install(BOOT_BACKUP)) {
  if(!write_word(BOOT_JOURNAL+12,BOOT_PROGRAM_ERROR)||!write_word(BOOT_JOURNAL+4,0)) return BOOT_IO;
  cleanup_failed_candidate();
  return BOOT_RESTORED;
 }
 return BOOT_RECOVERY;
}
int boot_parse_filename(const char *name,BootHeader *h) {
 uint32_t v[3]={0,0,0},limit;unsigned i;
 if(!name||!h) return 0;
 for(i=0;i<3;i++) {
  limit=i==0?255u:(i==1?65535u:0xffffffffu);
  if(*name<'0'||*name>'9') return 0;
  while(*name>='0'&&*name<='9') {
   unsigned digit=(unsigned)(*name++-'0');
   if(v[i]>(limit-digit)/10u) return 0;
   v[i]=v[i]*10u+digit;
  }
  if(*name++!='.') return 0;
 }
 if(strcmp(name,"bin")||!v[0]||v[0]!=BOOT_HW_VERSION) return 0;
 memset(h,0,sizeof(*h));h->magic=BOOT_MAGIC;h->hardware=(uint8_t)v[0];
 h->version=(uint16_t)v[1];h->timestamp=v[2];return 1;
}
