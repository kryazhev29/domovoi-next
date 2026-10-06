#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
typedef uint32_t Int32U;
typedef uint8_t *pInt8U;
/* ÒÈÏÛ ÏÐÎÒÎÊÎËÀ */
#define PROTOCOL_VERSION 1
static config_s config;
static struct { uint8_t before[16]; device_s value; uint8_t after[16]; } dev;
#define device dev.value
static struct { uint32_t start, size; } mem[16];
static uint8_t flash[sizeof(config_s)+sizeof(device_s)];
static unsigned reads, saves, defaults, resumes, acks, stamps;
/* ÊÎÍÒÐÎËÜÍÀß ÑÓÌÌÀ */
static void wdt_reload(void) {}
static void delay_ms(unsigned n) { CHECK(n == 10); }
static void mem_read(uint32_t address, uint8_t *p, uint16_t n)
{ CHECK(address >= 4096 && address-4096+n <= sizeof(flash)); memcpy(p,flash+address-4096,n); reads++; }
static void user_setdefault(void)
{ memset(&config,0,sizeof(config)); strcpy(config.server1,"default"); config.crc=crc16_calc((uint8_t *)&config,sizeof(config)-2); defaults++; }
static void user_saveindex(void) { saves++; }
static void fw_resume_after_boot(void) { resumes++; }
static void rs485_set_config(const uint8_t *p) { memcpy(&config,p,sizeof(config)); }
static bool user_writeConfigAndDevice(void) { saves++; return true; }
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n) { CHECK(a==4096 && n==1024); return config.crc; }
static void user_send_timeshtamp(shtamp_e kind,uint32_t time,uint16_t crc,dataChanel_e ch,uint8_t id)
{ CHECK(kind==SHTAMP_CONFIG && time==config.datetime && crc==config.crc && ch==CH_SERVER && id==42); stamps++; }
static void user_send_answer_comand(uint8_t cmd,cmdCode_e code,dataChanel_e ch,uint8_t id)
{ CHECK(cmd==12 && code==CMD_PARAMERROR && ch==CH_SERVER && id==42); acks++; }
static char *checked_strcpy(char *dest, size_t capacity, const char *p, size_t source_capacity)
{
    size_t n=0; while(n<source_capacity && p[n])n++;
    CHECK(n<source_capacity && n<capacity); memcpy(dest,p,n+1); return dest;
}
#define strcpy(p,s) checked_strcpy(p,__builtin_object_size(p,1),s,__builtin_object_size(s,1))
enum { SLM_IDLE };
static uint8_t comm_modem_state(void) { return 2; }
static void user_comm_boot(void) { CHECK(0); }
/* ÊÎÄ ÌÎÄÓËß */
#undef strcpy
static void init(void)
{
    memset(&config,0,sizeof(config)); memset(&dev,0,sizeof(dev));
    memset(dev.before,0xa7,sizeof(dev.before)); memset(dev.after,0xa7,sizeof(dev.after));
    reads=saves=defaults=resumes=acks=stamps=0; mem[MB_CONFIG].start=4096;mem[MB_CONFIG].size=1024;
}
static void prepare(config_s *next, bool malformed)
{
    memset(next,0x51,sizeof(*next));
    if(!malformed) {
        strcpy(next->server1,"ok");strcpy(next->server2,"ok");strcpy(next->NTPserver,"ok");
        strcpy(next->netOperator,"ok");strcpy(next->countryCode,"+7");strcpy(next->simPhone,"ok");strcpy(next->ussdBalance,"ok");
    }
    next->crc=crc16_calc((uint8_t *)next,sizeof(*next)-2);
}
static void check_config(void)
{
    CHECK(config.server1[31]==0 && config.server2[31]==0 && config.NTPserver[31]==0 && config.netOperator[31]==0);
    CHECK(config.countryCode[7]==0 && config.simPhone[15]==0 && config.ussdBalance[7]==0);
    CHECK(config.server1[0]=='Q' && config.server2[0]=='Q' && config.NTPserver[0]=='Q' && config.netOperator[0]=='Q');
    CHECK(config.crc==crc16_calc((uint8_t *)&config,sizeof(config)-2));
    CHECK(config.pinCode==0x51515151u && config.datetime==0x51515151u && config.reserv2[0]==0x51);
}
static void boot_test(void)
{
    config_s next;init();prepare(&next,true);memcpy(flash,&next,sizeof(next));
    user_readconfig();CHECK(reads==1 && !saves && !defaults);check_config();
    CHECK(!memcmp(flash,&next,sizeof(next)));
    init();prepare(&next,false);memcpy(flash,&next,sizeof(next));user_readconfig();CHECK(!memcmp(&config,&next,sizeof(config)));
    init();prepare(&next,true);next.crc^=1;memcpy(flash,&next,sizeof(next));user_readconfig();
    CHECK(reads==20 && defaults==1 && saves==1 && !strcmp(config.server1,"default"));
    puts("cfgstrings boot: valid CRC verified before termination; valid strings preserved; bad CRC rejected passed");
}
static void device_test(void)
{
    device_s saved;init();memset(&saved,0x51,sizeof(saved));saved.crc=crc16_calc((uint8_t *)&saved,sizeof(saved)-2);
    memcpy(flash+sizeof(config),&saved,sizeof(saved));user_readsavedinformation();
    CHECK(reads==1 && resumes==1 && device.IMEIGSM[15]==0);
    CHECK(!memcmp(device.IMEIGSM,saved.IMEIGSM,15) && device.datetimeConfig==saved.datetimeConfig && device.protocolVersion==1);
    for(unsigned i=0;i<16;i++)CHECK(dev.before[i]==0xa7 && dev.after[i]==0xa7);
    init();strcpy(saved.IMEIGSM,"123456789012345");saved.crc=crc16_calc((uint8_t *)&saved,sizeof(saved)-2);
    memcpy(flash+sizeof(config),&saved,sizeof(saved));user_readsavedinformation();CHECK(!strcmp(device.IMEIGSM,saved.IMEIGSM));
    init();saved.crc^=1;memcpy(flash+sizeof(config),&saved,sizeof(saved));strcpy(device.IMEIGSM,"old");user_readsavedinformation();
    CHECK(reads==10 && resumes==1 && !strcmp(device.IMEIGSM,"old"));
    puts("cfgstrings device: unterminated IMEI with valid CRC bounded; invalid saved device rejected passed");
}
static void command_test(void)
{
    config_s next;uint8_t frame[sizeof(config)+5];init();prepare(&next,true);
    frame[4]=12;memcpy(frame+5,&next,sizeof(next));command(frame,sizeof(config)+3,CH_SERVER,42);check_config();
    CHECK(saves==1 && stamps==1 && !acks && device.datetimeConfig==config.datetime);
    memcpy(flash,&config,sizeof(config));memset(&config,0,sizeof(config));user_readconfig();check_config();
    puts("cfgstrings command12: normalized before save and reply; reboot keeps bounded strings passed");
}
int main(int argc,char **argv)
{
    if(argc>1) { if(!strcmp(argv[1],"device")) device_test(); else {CHECK(!strcmp(argv[1],"command"));command_test();} }
    else { boot_test();device_test();command_test(); }
    return 0;
}
