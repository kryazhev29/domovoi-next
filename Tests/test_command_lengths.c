#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);} } while(0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
typedef uint32_t Int32U;
/* ÒÈÏÛ ÏÐÎÒÎÊÎËÀ */
#define USB_DIAG_COMMAND 111
static config_s config;
static device_s device;
static unsigned effects, replies, reply_cmd, reply_code, reply_kind, crc_calls, crc_index;
static int trial, boot_error;
/* ÊÎÍÒÐÎËÜÍÀß ÑÓÌÌÀ */
static void wdt_reload(void) {}
static uint32_t timers_getMillis(void) { return 42; }
static void user_send_answer_comand(uint8_t cmd,cmdCode_e code,dataChanel_e ch,uint8_t id)
{ CHECK((ch==CH_USB || ch==CH_SERVER) && id==42);replies++;reply_cmd=cmd;reply_code=code;reply_kind=11; }
static void user_send_anyData(uint8_t cmd,uint8_t *p,uint16_t n,dataChanel_e ch,uint8_t id)
{ CHECK((ch==CH_USB || ch==CH_SERVER) && id==42);replies++;reply_cmd=cmd;reply_kind=cmd;if(n){CHECK(n==1);reply_code=p[0];} }
static void user_send_crcMemoryID(uint8_t index,dataChanel_e ch,uint8_t id)
{ CHECK((ch==CH_USB || ch==CH_SERVER) && id==42);crc_calls++;crc_index=index;effects++; }
static void log_setTime(uint32_t n) { device.datetime=n;effects++; }
static bool app_trial_active(void) { return trial!=0; }
static void app_trial_confirm(void) { trial=0;effects++; }
static bool app_boot_error_unreported(void) { return boot_error!=0; }
static void app_boot_error_text(char *p) { strcpy(p,"TRIAL FAIL");effects++; }
static void app_boot_error_reported(void) { boot_error=0;effects++; }
static void fw_log(logUpdate_e event,const char *why) { (void)event;(void)why;effects++; }
static void status_event_sent(uint16_t n,uint32_t now) { (void)n;CHECK(now==42);effects++; }
/* Êîìàíäà 1: èñõîä ðåãèñòðàöèè ïåðåäà¸òñÿ ñåðâåðíîé ÷àñòè íîâîé ñâÿçè. */
static void comm_server_registered(uint8_t success) { (void)success;effects++; }
static void comm_channel_reopen(uint32_t pause) { CHECK(pause==0);effects++; }
static void user_factoryReset(void) { effects++; }
static void user_savereset(void) { effects++; }
static void fw_abort(const char *why) { CHECK(!strcmp(why,"CANCEL"));device.FWinProgress=0;effects++; }
/* ÊÎÄ ÌÎÄÓËß */
static void init(void)
{
    memset(&device,0,sizeof(device));memset(&config,0,sizeof(config));
    device.answerTimeout=19;device.userAutorized=1;device.connectionhandle=2;device.datetime=123;
    config.pinCode=0x31323334;effects=replies=crc_calls=0;reply_cmd=reply_code=reply_kind=255;trial=boot_error=1;
}
static void frame(uint8_t *p,uint8_t cmd,uint16_t len)
{
    memset(p,0x5a,64);p[0]=len;p[1]=len>>8;p[2]=255;p[3]=42;p[4]=cmd;
    if(cmd==1){p[5]=CMD_SUCCESS;uint32_t time=0x10203040;memcpy(p+6,&time,4);}
    else if(cmd==6){p[5]=5;p[6]=3;p[7]=7;}
    else if(cmd==23){uint32_t time=0x10203040;memcpy(p+5,&time,4);}
    else if(cmd==102)memcpy(p+5,&config.pinCode,4);
    else p[5]=2;
    uint16_t crc=crc16_calc(p,len+2);p[len+2]=crc;p[len+3]=crc>>8;
}
int main(void)
{
    static const struct { uint8_t cmd;uint16_t min; } commands[]={{1,8},{2,4},{3,4},{6,6},{18,4},{23,7},{102,7},{105,4},{108,4}};
    uint8_t p[64],before_packet[64];
    for(unsigned k=0;k<sizeof(commands)/sizeof(commands[0]);k++) {
        for(unsigned channel=0;channel<2;channel++) {
            dataChanel_e ch=channel?CH_USB:CH_SERVER;
            for(uint16_t len=3;len<commands[k].min;len++) {
                init();frame(p,commands[k].cmd,len);memcpy(before_packet,p,64);device_s before=device;
                user_parse_comand(p,ch);
                CHECK(!memcmp(&device,&before,sizeof(device)) && !effects && !crc_calls && trial==1 && boot_error==1);
                CHECK(!memcmp(p,before_packet,64));
                CHECK(replies==1 && reply_kind==11 && reply_cmd==commands[k].cmd && reply_code==CMD_PARAMERROR);
            }
        }
        for(unsigned extra=0;extra<2;extra++) {
            init();uint16_t len=commands[k].min+(extra?2:0);frame(p,commands[k].cmd,len);
            dataChanel_e ch=commands[k].cmd>=100?CH_USB:CH_SERVER;
            user_parse_comand(p,ch);
            switch(commands[k].cmd) {
            case 1:CHECK(device.registredOnServer==1 && device.datetime==0x10203040 && !trial && !boot_error);break;
            case 2:CHECK(device.timeToReset==2 && reply_code==CMD_SUCCESS);break;
            case 3:CHECK(device.timeToDefault==2 && reply_code==CMD_SUCCESS);break;
            case 6:CHECK(device.timeToGetID==5 && device.sourceGetID==3 && device.addressGetID==7 && reply_code==CMD_SUCCESS);break;
            case 18:CHECK(crc_calls==1 && crc_index==2);break;
            case 23:CHECK(device.datetime==0x10203040 && reply_code==CMD_SUCCESS);break;
            case 102:CHECK(device.userAutorized==1 && reply_kind==102 && reply_code==1);break;
            case 105:CHECK(device.moduleType==2 && effects==1 && reply_kind==105);break;
            case 108:CHECK(device.logBits==2 && reply_kind==108);break;
            default:abort();
            }
        }
    }
    for(unsigned code=1;code<=5;code++) {
        init();frame(p,1,4);p[5]=code;uint16_t crc=crc16_calc(p,6);p[6]=crc;p[7]=crc>>8;
        user_parse_comand(p,CH_SERVER);CHECK(!replies && device.answerTimeout==0);
    }
    init();frame(p,102,7);p[5]^=1;uint16_t crc=crc16_calc(p,9);p[9]=crc;p[10]=crc>>8;
    user_parse_comand(p,CH_USB);CHECK(!device.userAutorized && reply_kind==102 && reply_code==0);
    init();device.userAutorized=0;frame(p,2,4);device_s before=device;
    user_parse_comand(p,CH_USB);CHECK(!memcmp(&device,&before,sizeof(device)) && !effects && reply_code==CMD_GETPASSWORD);
    init();frame(p,2,3);p[5]^=1;before=device;
    user_parse_comand(p,CH_SERVER);CHECK(!memcmp(&device,&before,sizeof(device)) && !effects && reply_code==CMD_CRCERROR);
    for(uint16_t len=0;len<3;len++) {
        init();frame(p,1,len);before=device;user_parse_comand(p,CH_SERVER);
        CHECK(!memcmp(&device,&before,sizeof(device)) && !effects && !replies);
    }
    puts("cmdlengths: all nine commands, every short valid-CRC payload, both channels, no side effects; valid and extended frames passed");
    return 0;
}
