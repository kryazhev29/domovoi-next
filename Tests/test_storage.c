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
static unsigned operations,cut,partial,oplength[64],full_write;
typedef struct {uint8_t bytes[510];uint16_t crc;} saved_s;
static saved_s config,device;
#define MB_CONFIG 0
static struct {uint32_t start;} mem[1]={{LEGACY}};
static jmp_buf poweroff;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint16_t crc16_calc(uint8_t *p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
void w25_read_data(uint32_t a,uint8_t*p,uint16_t n){CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);}
static unsigned limit(unsigned n){oplength[operations]=n;return cut==operations+1 && partial<n ? partial:n;}
static void checkpoint(void){operations++;if(cut==operations)longjmp(poweroff,1);}
void w25_erase_sector(uint32_t a){a &= ~4095u;CHECK(a+4096<=sizeof(flash));memset(flash+a,255,limit(4096));checkpoint();}
void w25_write_data(uint32_t a,uint8_t*p,uint16_t n){while(n){unsigned i,k=256-(a&255),m;if(k>n)k=n;m=limit(k);CHECK(a+k<=sizeof(flash));for(i=0;i<m;i++){CHECK((flash[a+i]&p[i])==p[i]);flash[a+i]&=p[i];}a+=k;p+=k;n-=k;checkpoint();}}
/* PRODUCTION */
static void seed(void){unsigned i;uint16_t crc;memset(flash,255,sizeof(flash));for(i=0;i<4096;i++)olddata[i]=(uint8_t)(i*31+7);crc=crc16_calc(olddata,510);memcpy(olddata+510,&crc,2);memcpy(flash+LEGACY,olddata,4096);memcpy(newdata,olddata,4096);newdata[600]^=0x55;operations=cut=0;partial=4096;}
static void readback(void){mem_read(LEGACY,got,4096);}
static void update(void){if(full_write)user_writeConfigAndDevice();else mem_rewrite(LEGACY+600,newdata+600,1,scratch);}
static void combined(void){unsigned total,k,p;seed();readback();memcpy(&config,olddata,512);memcpy(&device,olddata+512,512);config.bytes[30]^=0x21;device.bytes[31]^=0x73;config.crc=crc16_calc((uint8_t*)&config,510);device.crc=crc16_calc((uint8_t*)&device,510);memcpy(newdata,&config,512);memcpy(newdata+512,&device,512);memcpy(snapshot,flash,sizeof(flash));operations=0;full_write=1;update();total=operations;CHECK(total>0);for(k=1;k<=total;k++)for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff))update();cut=0;readback();CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));}full_write=0;printf("combined config+device: %u operations with every byte cut position passed\n",total);}

static void campaign(int migration){unsigned total,k,p;seed();if(!migration)readback();memcpy(snapshot,flash,sizeof(flash));operations=0;if(migration)readback();else update();total=operations;CHECK(total>0);for(k=1;k<=total;k++){for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff)){if(migration)readback();else update();}cut=0;readback();CHECK(!memcmp(got,olddata,4096)||(!migration&&!memcmp(got,newdata,4096)));CHECK(!memcmp(flash+LEGACY,olddata,4096));}}printf("%s: %u operations with every byte cut position passed\n",migration?"migration":"update",total);}
static void integrity(void){
#ifdef MEM_CONFIG_SLOT0
 uint32_t h[4];unsigned errors;
 seed();readback();update();flash[MEM_CONFIG_SLOT1+100]^=1;readback();CHECK(!memcmp(got,olddata,4096));
 seed();readback();update();flash[MEM_CONFIG_SLOT1+4096+4]^=1;readback();CHECK(!memcmp(got,olddata,4096));
 seed();readback();h[0]=CFG_MAGIC;h[1]=0xffffffffu;h[2]=cfg_image_crc(MEM_CONFIG_SLOT0,h[1]);h[3]=~h[2];memcpy(flash+MEM_CONFIG_SLOT0+4096,h,16);update();readback();CHECK(!memcmp(got,newdata,4096));
 errors=mem_config_errors;CHECK(errors==0);
 puts("corrupt data/generation rejected, generation wrap selected: passed");
#endif
}
int main(int argc,char**argv){seed();if(argc>1&&!strcmp(argv[1],"power")){campaign(0);return 0;}readback();operations=0;mem_rewrite(LEGACY+600,olddata+600,1,scratch);CHECK(operations==0);puts("unchanged data: no erase/program");integrity();campaign(1);campaign(0);combined();/* Reuse an occupied older slot, including torn erases. */seed();readback();update();memcpy(olddata,newdata,4096);newdata[600]^=0x33;memcpy(snapshot,flash,sizeof(flash));{unsigned total,k,p;operations=0;update();total=operations;for(k=1;k<=total;k++)for(p=0;p<=oplength[k-1];p++){memcpy(flash,snapshot,sizeof(flash));operations=0;cut=k;partial=p;if(!setjmp(poweroff))update();cut=0;readback();CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));}printf("slot reuse: %u operations with every byte cut position passed\n",total);}return 0;}
