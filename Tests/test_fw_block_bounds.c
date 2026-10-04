#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)
/* TYPES */
/* DEFINES */
#define DEBUG_USER
#define MB_FIRMWARE 0
static struct { uint32_t start; } mem[1];
static struct { uint8_t cchFWupdate,hardwareVersion,firmwarePercent;uint16_t FWblockIdx,FWblockCount;uint32_t startWaitFWtime; } device;
static const uint8_t *input;
static size_t input_size;
static unsigned writes,reads,aborts,logs,status_count,debug_lines;
static uint8_t status_code;
static int manual,written,foreign;
static char tempStrBuffer[128];
static bool boot_manual_busy(void) { return manual!=0; }
static void wdt_reload(void) {}
static bool fw_image_foreign(const uint8_t *p,uint8_t hw) { CHECK(p!=NULL && hw==139);return foreign!=0; }
static void fw_abort(const char *why) { (void)why;aborts++;device.startWaitFWtime=0; }
static void fw_log(logUpdate_e event,const char *why) { CHECK(event==LOG_FW_FAIL && !strcmp(why,"HW"));logs++; }
static bool mem_write(uint32_t address,uint8_t *p,uint16_t n)
{ CHECK(n==1024 && p==input+2 && address==FWBLOCK_OFFSET+(uint32_t)device.FWblockIdx*1024);writes++; return true; }
static unsigned erases;static uint32_t erased[8];static int erase_fail;
static bool mem_erase_sector(uint32_t a) { CHECK(a%4096u==0);if(erase_fail)return false;if(erases<8)erased[erases]=a;erases++;return true; }
static bool fw_block_written(uint32_t address,const uint8_t *p) { (void)address;CHECK(p==input+2);return written!=0; }
static bool mem_read(uint32_t address,uint8_t *p,uint16_t n) { CHECK(address==0 && n==sizeof(bootFWparam_s));memset(p,0,n);reads++; return true; }
static bool user_checkFirmware(uint32_t address,uint32_t n,uint32_t crc) { CHECK(address==FWBLOCK_OFFSET && !n && !crc);return true; }
static uint8_t fw_blocks_done(bool ok) { CHECK(ok);return FW_BLOCK_OK; }
static uint8_t fw_write_errors;
static void user_send_statusUpdateFW(uint8_t cmd,uint8_t code,dataChanel_e ch,uint8_t id)
{ CHECK(cmd==20 && ch==CH_SERVER && id==42);status_count++;status_code=code; }
static void user_debug(char *p) { CHECK(strstr(p,"cmd 20")!=NULL);CHECK(input_size>=2);debug_lines++; }
static void *checked_memcpy(void *dest,const void *source,size_t n)
{ if(source==input)CHECK(n<=input_size);CHECK(source!=NULL);return memcpy(dest,source,n); }
#define memcpy checked_memcpy
/* PRODUCTION */
#undef memcpy
static void init(void)
{
    memset(&device,0,sizeof(device));device.cchFWupdate=CH_SERVER;device.hardwareVersion=139;
    device.FWblockIdx=2;device.FWblockCount=4;device.startWaitFWtime=12345;
    writes=reads=aborts=logs=status_count=debug_lines=fw_write_errors=0;manual=foreign=0;written=1;
}
static void test_lengths(void)
{
    static const unsigned lengths[]={0,1,2,1025,1026};
    for(unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++) {
        unsigned n=lengths[i];init();uint8_t *p=n?calloc(n,1):NULL;CHECK(!n || p!=NULL);
        if(n)p[0]=2;input=p;input_size=n;uint8_t code=fw_take_block(p,n,CH_SERVER);
        if(n==1026)CHECK(code==FW_BLOCK_OK && device.startWaitFWtime==0 && writes==1 && device.FWblockIdx==3);
        else CHECK(code==FW_BLOCK_BADFRAME && device.startWaitFWtime==12345 && !writes && !reads && !aborts && device.FWblockIdx==2);
        free(p);
    }
    puts("fwblockbounds: length checked before the two-byte read; malformed frames keep the retry clock passed");
}
static void test_errors(void)
{
    uint8_t p[1026]={0};input=p;input_size=sizeof(p);
    init();p[0]=3;CHECK(fw_take_block(p,sizeof(p),CH_SERVER)==FW_BLOCK_BADFRAME && device.startWaitFWtime==12345 && !writes);
    init();p[0]=1;CHECK(fw_take_block(p,sizeof(p),CH_SERVER)==FW_BLOCK_OK && device.startWaitFWtime==12345 && !writes);
    init();p[0]=2;written=0;CHECK(fw_take_block(p,sizeof(p),CH_SERVER)==FW_BLOCK_BADWRITE && device.startWaitFWtime==12345 && device.FWblockIdx==2);
    init();manual=1;input=NULL;input_size=0;CHECK(fw_take_block(NULL,0,CH_SERVER)==FW_BLOCK_END && device.startWaitFWtime==12345);
    init();CHECK(fw_take_block(NULL,0,CH_USB)==FW_BLOCK_END && device.startWaitFWtime==12345);
    init();device.FWblockIdx=4;CHECK(fw_take_block(NULL,0,CH_SERVER)==FW_BLOCK_END && device.startWaitFWtime==12345);
    puts("fwblockbounds: future/duplicate/write error/foreign channel/manual/done preserve existing reply codes and timers passed");
}
static void test_command(void)
{
    static const unsigned lengths[]={0,1,2,1025,1026};uint8_t frame[1035];
    for(unsigned i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++) {
        unsigned n=lengths[i];init();memset(frame,0,sizeof(frame));frame[4]=20;frame[5]=2;
        input=frame+5;input_size=n;command(frame,n+3,CH_SERVER,42);
        CHECK(status_count==1 && status_code==(n==1026?FW_BLOCK_OK:FW_BLOCK_BADFRAME));
        CHECK(debug_lines==(n>=2));
        if(n!=1026)CHECK(device.startWaitFWtime==12345 && !writes);
    }
    puts("fwblockbounds command20: DEBUG_USER respects payload boundaries; one unchanged command4 response passed");
}
/* A sector is erased right before the first block written into it: blocks 3 and 7 cross 0x1000 and 0x2000. */
static void test_lazy_erase(void)
{
    uint8_t p[1026]={0};input=p;input_size=sizeof(p);
    init();device.FWblockIdx=0;device.FWblockCount=8;erases=0;
    for(uint16_t k=0;k<8;k++){memcpy(p,&k,2);CHECK(fw_take_block(p,sizeof(p),CH_SERVER)==FW_BLOCK_OK);}
    CHECK(erases==2&&erased[0]==0x1000&&erased[1]==0x2000);
    init();device.FWblockIdx=3;device.FWblockCount=8;erases=0;erase_fail=1;p[0]=3;p[1]=0;
    CHECK(fw_take_block(p,sizeof(p),CH_SERVER)==FW_BLOCK_BADWRITE&&device.FWblockIdx==3&&!writes);erase_fail=0;
    puts("fwblockbounds: each sector erased before its first block, erase failure is a write failure passed");
}
int main(int argc,char **argv)
{
    if(argc>1) { CHECK(!strcmp(argv[1],"errors"));test_errors(); }
    else {test_lengths();test_errors();test_command();test_lazy_erase();}
    return 0;
}
