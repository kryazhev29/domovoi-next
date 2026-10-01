#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
typedef uint32_t Int32U;
typedef uint16_t Int16U;
typedef uint8_t *pInt8U;
#define mem_check_power() ((void)0)
#define wdt_reload() ((void)0)
#define LEGACY 1889280u
static uint8_t flash[2097152],snapshot[2097152],olddata[4096],newdata[4096],scratch[4096],got[4096];
static unsigned operations,cut,partial,oplength[256],full_write;
typedef struct {uint8_t bytes[510];uint16_t crc;} saved_s;
static saved_s config,device;
#define MB_CONFIG 0
static struct {uint32_t start;} mem[1]={{LEGACY}};
static jmp_buf poweroff;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint16_t crc16_calc(uint8_t *p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
bool w25_read_data(uint32_t a,uint8_t*p,uint16_t n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);return true;}
static unsigned limit(unsigned n){oplength[operations]=n;return cut==operations+1 && partial<n ? partial:n;}
static void checkpoint(void){operations++;if(cut==operations)longjmp(poweroff,1);}
bool w25_erase_sector(uint32_t a){a &= ~4095u;CHECK(a+4096<=sizeof(flash));memset(flash+a,255,limit(4096));checkpoint();return true;}
bool w25_write_data(uint32_t a,uint8_t*p,uint16_t n){while(n){unsigned i,k=256-(a&255),m;if(k>n)k=n;m=limit(k);CHECK(a+k<=sizeof(flash));for(i=0;i<m;i++){flash[a+i]&=p[i];}a+=k;p+=k;n-=k;checkpoint();}return true;}
static unsigned heap_used,heap_peak,heap_failures,legacy_defaulted;
static void *heap_block;
static void *host_malloc(size_t n){size_t *p;if(heap_used+n>0x1400){heap_failures++;return NULL;}CHECK(heap_block==NULL);p=malloc(n+sizeof(*p));CHECK(p!=NULL);heap_block=p;*p=n;heap_used+=(unsigned)n;if(heap_used>heap_peak)heap_peak=heap_used;return p+1;}
static void host_free(void *v){if(v){size_t *p=(size_t*)v-1;heap_used-=(unsigned)*p;free(p);heap_block=NULL;}}
static void host_boot(void){free(heap_block);heap_block=NULL;heap_used=0;}
#define malloc host_malloc
#define free host_free
#define delay_ms(n) ((void)(n))
static void legacy_user_setdefault(void){legacy_defaulted=1;}
static void legacy_user_saveindex(void){}
static void legacy_mem_read(uint32_t a,uint8_t*p,uint16_t n){w25_read_data(a,p,n);}
static void legacy_mem_write(uint32_t a,uint8_t*p,uint16_t n){w25_write_data(a,p,n);}
static void legacy_mem_erase_sector(uint32_t a){w25_erase_sector(a);}
uint16_t crc16_calcInit(uint8_t*p,uint16_t n,uint16_t c){unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
/* PRODUCTION */
static void seed(void){host_boot();TEST_STORAGE_BOOT();unsigned i;uint16_t crc;memset(flash,255,sizeof(flash));for(i=0;i<4096;i++)olddata[i]=(uint8_t)(i*31+7);crc=crc16_calc(olddata,510);memcpy(olddata+510,&crc,2);memcpy(flash+LEGACY,olddata,4096);memcpy(newdata,olddata,4096);newdata[600]^=0x55;operations=cut=0;partial=4096;}
static void readback(void){host_boot();TEST_STORAGE_BOOT();mem_read(LEGACY,got,4096);}
static void update(void){if(full_write)user_writeConfigAndDevice();else mem_rewrite(LEGACY+600,newdata+600,1,scratch);}
static void combined(void){unsigned total,k,p;seed();readback();memcpy(&config,olddata,512);memcpy(&device,olddata+512,512);config.bytes[30]^=0x21;device.bytes[31]^=0x73;config.crc=crc16_calc((uint8_t*)&config,510);device.crc=crc16_calc((uint8_t*)&device,510);memcpy(newdata,&config,512);memcpy(newdata+512,&device,512);memcpy(snapshot,flash,sizeof(flash));operations=0;full_write=1;update();total=operations;CHECK(total>0);for(k=1;k<=total;k++)for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff))update();CHECK(operations>=k);cut=0;readback();CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));}full_write=0;printf("combined config+device: %u operations with every byte cut position passed\n",total);}

static void campaign(int migration){unsigned total,k,p;seed();if(!migration)readback();memcpy(snapshot,flash,sizeof(flash));operations=0;if(migration)readback();else update();total=operations;CHECK(total>0);for(k=1;k<=total;k++){for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff)){if(migration)readback();else update();}CHECK(operations>=k);cut=0;readback();CHECK(!memcmp(got,olddata,4096)||(!migration&&!memcmp(got,newdata,4096)));if(migration)CHECK(!memcmp(flash+LEGACY,olddata,4096));}}printf("%s: %u operations with every byte cut position passed\n",migration?"migration":"update",total);}
static void integrity(void){
#ifdef MEM_CONFIG_SLOT0
 uint32_t h[4];unsigned errors;
 seed();readback();update();flash[MEM_CONFIG_SLOT1+100]^=1;readback();CHECK(!memcmp(got,olddata,4096));
 seed();readback();update();flash[MEM_CONFIG_SLOT1+4096+4]^=1;readback();CHECK(!memcmp(got,olddata,4096));
 seed();readback();h[0]=CFG_MAGIC;h[1]=0xffffffffu;CHECK(cfg_image_crc(MEM_CONFIG_SLOT0,h[1],&h[2]));h[3]=~h[2];memcpy(flash+MEM_CONFIG_SLOT0+4096,h,16);update();readback();CHECK(!memcmp(got,newdata,4096));
 errors=mem_config_errors;CHECK(errors==0);
 puts("corrupt data/generation rejected, generation wrap selected: passed");
#endif
}
#define EXPECT(x,msg) do{if(!(x)){fprintf(stderr,"FAIL %s\n",msg);exit(1);}}while(0)
static void fix_crcs(uint8_t *p){uint16_t c=crc16_calc(p,510);memcpy(p+510,&c,2);c=crc16_calc(p+512,510);memcpy(p+1022,&c,2);}
static void valid_seed(void){seed();fix_crcs(olddata);memcpy(flash+LEGACY,olddata,4096);memcpy(newdata,olddata,4096);newdata[30]^=0x51;newdata[600]^=0xa5;fix_crcs(newdata);}
static void old_write(const uint8_t *p){memcpy(&config,p,512);memcpy(&device,p+512,512);legacy_user_writeConfigAndDevice();}
static void old_read(void){legacy_defaulted=0;legacy_user_readconfig();}
static void compat_test(void){valid_seed();readback();old_write(newdata);readback();EXPECT(!memcmp(got,newdata,4096),"legacy update ignored after rollback");puts("PASS legacy update accepted after rollback (non-FF reserve preserved)");}
static void rollback_test(void){valid_seed();readback();mem_write(LEGACY,newdata,4096);old_read();EXPECT(!legacy_defaulted&&!memcmp(&config,newdata,512),"old firmware reads stale config after new commit");EXPECT(!memcmp(flash+LEGACY,newdata,4096),"legacy mirror differs from committed image");puts("PASS rollback reads new commit");}
static void heap_test(void){uint8_t *p;valid_seed();heap_failures=0;p=malloc(4096);CHECK(p!=NULL);mem_rewrite(LEGACY,olddata,512,p);free(p);EXPECT(heap_failures==0,"nested 4096-byte allocation exceeds 0x1400 heap");puts("PASS migration inside caller's 4096-byte allocation");}
static void cycles_test(void){unsigned i;valid_seed();readback();for(i=0;i<12;i++){old_write(newdata);readback();EXPECT(!memcmp(got,newdata,4096),"new/old cycles accepted stale slot");memcpy(olddata,newdata,4096);newdata[30]^=(uint8_t)(i+1);newdata[700]^=(uint8_t)(i+5);fix_crcs(newdata);mem_write(LEGACY,newdata,4096);old_read();CHECK(!legacy_defaulted&&!memcmp(&config,newdata,512));readback();CHECK(!memcmp(got,newdata,4096));operations=0;readback();CHECK(operations==0);memcpy(olddata,newdata,4096);newdata[31]^=(uint8_t)(i+3);fix_crcs(newdata);}puts("PASS 12 new/old cycles without stale acceptance or redundant writes");}
static void mirror_power_test(void){unsigned k,p,total,lens[256],cases=0;valid_seed();readback();/* Change reserve too: partial mirrors must not be accepted as full images. */newdata[1024]^=0x23;newdata[3500]^=0xb1;memcpy(snapshot,flash,sizeof(flash));operations=0;mem_write(LEGACY,newdata,4096);total=operations;memcpy(lens,oplength,sizeof(lens));EXPECT(!memcmp(flash+LEGACY,newdata,4096),"commit does not mirror legacy before publication");for(k=1;k<=total;k++)for(p=0;p<=lens[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;TEST_STORAGE_BOOT();if(!setjmp(poweroff))mem_write(LEGACY,newdata,4096);cut=0;old_read();EXPECT(legacy_defaulted||!memcmp(flash+LEGACY,olddata,4096)||!memcmp(flash+LEGACY,newdata,4096),"old reader accepted mixed mirror");readback();EXPECT(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096),"new reader accepted mixed mirror/slot");cases++;}printf("PASS mirror+slot power cuts: %u operations, %u byte boundaries\n",total,cases);}
int main(int argc,char**argv){if(argc>1){if(!strcmp(argv[1],"compat")){compat_test();return 0;}if(!strcmp(argv[1],"rollback")){rollback_test();return 0;}if(!strcmp(argv[1],"heap")){heap_test();return 0;}if(!strcmp(argv[1],"cycles")){cycles_test();return 0;}if(!strcmp(argv[1],"mirror_power")){mirror_power_test();return 0;}}seed();if(argc>1&&!strcmp(argv[1],"power")){campaign(0);return 0;}readback();operations=0;mem_rewrite(LEGACY+600,olddata+600,1,scratch);CHECK(operations==0);puts("unchanged data: no erase/program");integrity();campaign(1);campaign(0);combined();/* Reuse an occupied older slot, including torn erases. */seed();readback();update();memcpy(olddata,newdata,4096);newdata[600]^=0x33;memcpy(snapshot,flash,sizeof(flash));{unsigned total,k,p;operations=0;update();total=operations;for(k=1;k<=total;k++)for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff))update();CHECK(operations>=k);cut=0;readback();CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));}printf("slot reuse: %u operations with every byte cut position passed\n",total);}return 0;}
