/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
typedef uint16_t Int16U;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* ТИПЫ ПРОТОКОЛА */
/* КОНСТАНТЫ МОДУЛЯ */
#define FWBLOCK_OFFSET 512u
enum {MB_FIRMWARE};
static struct {uint32_t start,size;} mem[]={{0,131072}};
static device_s device;
static bool store_runtime=true;
static struct {unsigned job,clear;} store;
static uint8_t store_reset_requested;
static uint32_t usb_bus_resets,fw_rollback_end,now;
static uint8_t fw_write_errors,flash[131072],payload[1031],original[1024];
static unsigned reads,starts,polls,read_ops,start_ops,poll_ops,fail_read,fail_start,fail_poll;
static unsigned held,busy,allocated,alloc_fail,unavailable,txbusy,acks,last_code,last_id,saves,logs;
static uint32_t address;
static uint16_t length;
static const uint8_t *source;
static uint8_t source_copy[64];
static int cut_at=-1,changed;
static jmp_buf power;
static bool fw_async_begin(const uint8_t*,uint32_t,dataChanel_e);
static bool fw_async_active(void);
static void fw_async_cancel(void);
static void store_usb_session(void){}
static bool log_async_active(void){return false;}
static uint8_t mem_config_result(void){return 0;}
static bool w25_async_active(void){return held!=0;}
static bool w25_read_available(void){return !unavailable;}
static bool comm_tx_busy(void){return txbusy!=0;}
static bool boot_manual_busy(void){return false;}
static bool fw_image_foreign(const uint8_t*p,uint8_t hw){(void)p;(void)hw;return false;}
static void user_saveindex(void){saves++;}
static void wdt_reload(void){}
static void fw_log(logUpdate_e type,const char *reason){(void)type;(void)reason;logs++;}
static void user_send_statusUpdateFW(uint8_t cmd,uint8_t code,dataChanel_e ch,uint8_t id){
 CHECK(cmd==20&&(ch==CH_SERVER||ch==CH_USB)&&code!=FW_BLOCK_PENDING);acks++;last_code=code;last_id=id;
}
static bool mem_read(uint32_t a,uint8_t*p,uint16_t n){
 CHECK(a+n<=sizeof(flash));reads++;read_ops++;if(read_ops==fail_read)return false;memcpy(p,flash+a,n);return true;
}
static bool mem_write(uint32_t a,uint8_t*p,uint16_t n){(void)a;(void)p;(void)n;CHECK(0);return false;}
static bool mem_erase_sector(uint32_t a){(void)a;CHECK(0);return false;}
static bool fw_block_written(uint32_t a,const uint8_t*p){(void)a;(void)p;CHECK(0);return false;}
static bool user_checkFirmware(uint32_t a,uint32_t n,uint32_t c){(void)a;(void)n;(void)c;CHECK(0);return false;}
static void *take(size_t n){CHECK(n==1024);if(alloc_fail)return NULL;allocated++;return malloc(n);}
static void release(void *p){if(p){CHECK(!held&&allocated);allocated--;free(p);}}
static bool w25_async_start(uint32_t a,const uint8_t*p,uint16_t n,uint32_t t){
 (void)t;CHECK(!held&&a<sizeof(flash));starts++;start_ops++;if(start_ops==fail_start)return false;
 if(p)CHECK(n==64&&(a&255)+n<=256);else CHECK(!n&&!(a&4095)&&a+4096<=sizeof(flash));
 held=1;busy=3;address=a;length=p?n:4096;source=p;if(p)memcpy(source_copy,p,n);return true;
}
static uint8_t w25_async_poll(uint32_t t){
 (void)t;CHECK(held);polls++;poll_ops++;if(source)CHECK(!memcmp(source,source_copy,length));
 if(busy){busy--;return 1;}
 held=0;
 for(unsigned i=0;i<length;i++){
   if(cut_at>=0&&changed==cut_at)longjmp(power,1);
   if(source)flash[address+i]&=source[i];else flash[address+i]=255;changed++;
 }
 return poll_ops==fail_poll?3:2;
}
#define malloc take
#define free release
/* КОД МОДУЛЯ */
#undef malloc
#undef free
static uint32_t reference_crc(const uint8_t *p,unsigned n){
 uint32_t c=0xffffffffu;for(unsigned i=0;i<n;i+=4){uint32_t v;memcpy(&v,p+i,4);c^=v;
 for(unsigned j=0;j<32;j++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0);}return c;
}
static void reset(unsigned block,unsigned blocks){
 CHECK(!allocated);memset(&fw_job,0,sizeof(fw_job));memset(&device,0,sizeof(device));memset(flash,255,sizeof(flash));
 device.cchFWupdate=CH_SERVER;device.FWblockIdx=block;device.FWblockCount=blocks;device.FWinProgress=1;device.updateTimeout=600;
 device.startWaitFWtime=123;device.hardwareVersion=139;held=reads=starts=polls=read_ops=start_ops=poll_ops=0;
 fail_read=fail_start=fail_poll=unavailable=txbusy=acks=logs=saves=alloc_fail=usb_bus_resets=store.job=0;fw_write_errors=0;
 cut_at=-1;changed=0;now=0;
 memset(payload,0,sizeof(payload));payload[4]=20;payload[5]=block;payload[6]=block>>8;
 for(unsigned i=0;i<1024;i++)original[i]=payload[7+i]=(uint8_t)(i*37+block);
}
static void start(void){command(payload,1029,device.cchFWupdate,42);}
static void step(void){reads=starts=polls=0;fw_storage_poll(now++);CHECK(reads<=1&&starts<=1&&polls<=1);}
static void drain(void){unsigned i=0;while(fw_async_active()){step();CHECK(++i<10000);}}
static void header(unsigned size,bool good){bootFWparam_s h;memset(&h,255,sizeof(h));h.firmwareSize=size;
 h.firmwareCRC=reference_crc(flash+512,size)^(good?0u:1u);memcpy(flash,&h,sizeof(h));}
int main(void){
 reset(0,2);start();CHECK(fw_async_active()&&allocated==1&&!acks);
 CHECK(!user_storage_command_ready()&&!user_storage_flash_ready());memset(payload+7,0,1024);
 while(fw_job.phase!=FA_REPLY){step();if(fw_job.offset<1024)CHECK(!device.FWblockIdx&&!acks);}
 CHECK(!allocated&&device.FWblockIdx==1&&!memcmp(flash+512,original,1024));txbusy=1;
 for(unsigned i=0;i<10;i++)step();CHECK(!acks&&fw_async_active());txbusy=0;drain();CHECK(acks==1&&last_code==0&&last_id==42);
 start();CHECK(acks==2&&!fw_async_active()); /* Повтор блока ниже подтверждённого курсора. */
 reset(3,5);memset(flash+4096,0,4096);start();drain();CHECK(device.FWblockIdx==4&&!memcmp(flash+3584,original,1024));
 for(unsigned i=4608;i<8192;i++)CHECK(flash[i]==255);
 /* На каждом отказе ожидание не превращается в отказ или подтверждение. */
 for(unsigned fail=1;fail<=17;fail++){
   reset(3,5);fail_start=fail;start();drain();CHECK(last_code==FW_BLOCK_BADWRITE&&device.FWblockIdx==3&&!allocated);
   fail_start=0;start();drain();CHECK(last_code==0&&device.FWblockIdx==4&&acks==2);
 }
 for(unsigned fail=4;fail<=68;fail+=4){
   reset(3,5);fail_poll=fail;start();drain();CHECK(last_code==FW_BLOCK_BADWRITE&&device.FWblockIdx==3);
   fail_poll=0;start();drain();CHECK(last_code==0&&device.FWblockIdx==4);
 }
 reset(0,2);for(unsigned i=0;i<3;i++){fail_start=start_ops+1;start();drain();}
 CHECK(last_code==FW_BLOCK_END&&!device.FWinProgress&&!allocated);
 reset(0,2);alloc_fail=1;start();CHECK(!allocated&&!fw_async_active()&&last_code==FW_BLOCK_BADWRITE&&!device.FWblockIdx);
 /* CRC всего образа считается частями; размер заголовка проверяется до чтения. */
 for(unsigned good=0;good<2;good++){
   reset(1,2);for(unsigned i=0;i<1024;i++)flash[512+i]=(uint8_t)i;
   memcpy(flash+1536,original,1024);header(2048,good);memset(flash+1536,255,1024);start();
   while(fw_job.phase!=FA_CRC)step();CHECK(!acks&&!device.rebootToUpdate);
   drain();CHECK(acks==1&&last_code==(good?0:FW_BLOCK_END)&&device.rebootToUpdate==good&&!allocated);
 }
 for(unsigned fail=1;fail<=33;fail++){
   reset(0,1);memcpy(flash+512,original,1024);header(1024,true);memset(flash+512,255,1024);
   fail_read=fail;start();drain();CHECK(!allocated&&acks==1);
   CHECK((fail<=17)==(last_code==FW_BLOCK_END));
 }
 reset(0,1);header(0,true);start();drain();CHECK(last_code==FW_BLOCK_END&&!device.rebootToUpdate);
 /* Закрытие канала подавляет старые ответы; отмена ждёт освобождения источника данных NOR. */
 reset(0,2);start();while(!held)step();fw_async_quiet(CH_SERVER);drain();CHECK(!acks&&device.FWblockIdx==1);
 reset(0,2);device.cchFWupdate=CH_USB;start();while(!held)step();usb_bus_resets++;drain();CHECK(!acks&&device.FWblockIdx==1);
 reset(0,2);start();while(!held)step();fw_abort("TIMEOUT");CHECK(allocated&&held);drain();CHECK(!allocated&&!acks&&!device.FWblockIdx);
 for(unsigned phase=FA_ERASE;phase<=FA_CRC;phase++){
   reset(3,4);memcpy(flash+3584,original,1024);header(4096,true);memset(flash+3584,255,1024);start();
   while(fw_job.phase!=phase)step();fw_abort("CANCEL");drain();CHECK(!allocated&&!acks&&!device.FWblockIdx);
 }
 /* Обрыв на каждой границе записываемого байта и на выбранных границах стирания. */
 for(int cut=0;cut<=1024;cut++){
   reset(0,2);cut_at=cut;
   if(!setjmp(power)){start();drain();}
   held=0;free(fw_job.data);fw_job.data=NULL;allocated=0;fw_job.phase=0;device.FWblockIdx=0;cut_at=-1;
   start();drain();CHECK(device.FWblockIdx==1&&!memcmp(flash+512,original,1024));
 }
 for(int cut=0;cut<=4096;cut+=64){
   reset(3,5);memset(flash+4096,0,4096);cut_at=cut;
   if(!setjmp(power)){start();drain();}
   held=0;free(fw_job.data);fw_job.data=NULL;allocated=0;fw_job.phase=0;device.FWblockIdx=3;cut_at=-1;
   start();drain();CHECK(device.FWblockIdx==4&&!memcmp(flash+3584,original,1024));
 }
 puts("PASS async FW: 1090 power cuts, 51 start/poll/read faults + 16 controls, exact image/CRC, ownership, cursor/ACK, cancel/reconnect/USB, busy/heap/duplicates");
 return 0;
}
