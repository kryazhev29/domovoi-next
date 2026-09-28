// ƒоработка:  р€жев ј., 2026
// Ёмул€тор C328 и NOR; исполн€етс€ производственный код.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
#include <setjmp.h>
#include <time.h>
static jmp_buf power_reset;
static int cut_after, flash_ops, byte_cuts;
static void power_cut(void) { if(cut_after && ++flash_ops==cut_after) longjmp(power_reset,1); }
typedef enum { CMD_SUCCESS, CMD_PARAMERROR, CMD_RUNERROR } cmdCode_e;
typedef enum {CH_USB, CH_SERVER, CH_SMS} dataChanel_e;
typedef struct { uint32_t start, size; } cellMem_s;
typedef struct { uint32_t datetime, size; } photoParam_s;
#define PHOTO_ACK_SLOTS 128u
#define PHOTO_RECORD_HEAD (sizeof(photoParam_s) + PHOTO_ACK_SLOTS * 8u + 1u)
typedef struct { uint8_t address,type; } periphery_s;
typedef struct { uint8_t rs485Mode, reserv[4]; periphery_s rs485Periphery[8]; } config_s;
typedef struct {
 struct { uint8_t readyToUpdate, readyPhoto; } status;
 uint32_t datetime, newAddressPhoto; uint16_t countPhoto, readPhotoIndex; uint8_t getPhoto, connectionhandle;
} device_s;
#define PORTBUFSIZE 32
#define PORT1_PORT 0
#define MEMSECTORSIZE 4096u
#define MB_PHOTO 0
#define CE_OK 0
#define CE_CONNECTED 1
cellMem_s mem[16]; config_s config; device_s device;
uint8_t port_buffer[PORTBUFSIZE];
static uint8_t flash[3*1024*1024], *rx, wire[1100], history[32][1100];
static uint16_t history_len[32];
static uint16_t rxcap, rxcount, wirelen;
static uint32_t now, image_size, sent_commands, packet_requests, watchdog;
static uint8_t *bridge_jpeg;
static uint32_t rxready, baudrate, send_ready_at;
static int connected, send_attempts;
static int quiet, nack, bad_once, fail_send, sends, erases, programs, fail_write;
static uint8_t pixel(uint32_t i) {
 if (bridge_jpeg) return bridge_jpeg[i];
 if (!i || i == image_size-2) return 0xFF;
 if (i == 1) return 0xD8;
 if (i == image_size-1) return 0xD9;
 return (uint8_t)(i*13u + 3u);
}
static uint32_t timers_getMillis(void) { return now; }
static void wdt_reload(void) { watchdog++; }
static void mem_read(uint32_t a,uint8_t *p,uint16_t n) { assert(a+n <= sizeof(flash)); memcpy(p,flash+a,n); }
static void mem_write(uint32_t a,uint8_t *p,uint16_t n) {
 assert(a+n <= sizeof(flash)); if (fail_write) return; programs++; while(n--) {flash[a++] &= *p++; if(byte_cuts) power_cut();} if(!byte_cuts) power_cut();
}
static void mem_erase_sector(uint32_t a) { assert(a%4096 == 0); assert(a+4096 <= sizeof(flash)); memset(flash+a,255,4096); erases++; power_cut(); }
static int uart_init(uint8_t u,uint32_t rate,uint8_t stop,uint8_t *p,uint16_t n) {(void)u;assert((rate==115200 || rate==57600 || rate==38400 || rate==19200 || rate==9600) && !stop);baudrate=rate;rx=p;rxcap=n;rxcount=0;return 1;}
static int uart_getstate(uint8_t u) {(void)u;return rxcount!=0 && now>=rxready;}
static int uart_isReceiving(uint8_t u) {(void)u;return 0;}
static uint16_t uart_received(uint8_t u) {(void)u;return rxcount;}
static void uart_clear(uint8_t u) {(void)u;rxcount=0;}
static void reply(uint8_t cmd,uint8_t a,uint8_t b,uint8_t c,uint8_t d) {
 uint8_t p[]={0xAA,cmd,a,b,c,d}; assert(rxcount+6<=rxcap);memcpy(rx+rxcount,p,6);rxcount+=6;
}
static void uart_send(uint8_t u,uint8_t *p,uint16_t n) {
 (void)u;assert(n==6 && p[0]==0xAA);sent_commands++;assert(!device.status.readyToUpdate);
 if(bridge_jpeg) {
   char line[2048];printf("TX ");for(uint16_t j=0;j<n;j++)printf("%02x",p[j]);puts("");fflush(stdout);
   assert(fgets(line,sizeof(line),stdin));rxcount=0;
   for(unsigned j=0;line[j] && line[j]!='\n';j+=2){unsigned byte;assert(sscanf(line+j,"%2x",&byte)==1);assert(rxcount<rxcap);rx[rxcount++]=(uint8_t)byte;}
   rxready=now+((n+rxcount)*10000u+115199u)/115200u+2u;return;
 }
 if(quiet)return;
 if(nack){reply(0x0F,p[1],0,0,0);return;}
 switch(p[1]) {
 case 0x0D:reply(0x0E,0x0D,0,0,0);reply(0x0D,0,0,0,0);break;
 case 1:assert(p[3]==7 && p[5]==7);reply(0x0E,1,0,0,0);break;
 case 6:assert(p[2]==8 && p[3]==64 && p[4]==0);reply(0x0E,6,0,0,0);break;
 case 5:reply(0x0E,5,0,0,0);break;
 case 4:reply(0x0E,4,0,0,0);reply(0x0A,1,(uint8_t)image_size,(uint8_t)(image_size>>8),(uint8_t)(image_size>>16));break;
 case 0x0E:
  if(p[2]==0x0D || (p[4]==0xF0 && p[5]==0xF0))break;
  {uint16_t id=p[4]|((uint16_t)p[5]<<8);uint32_t off=(uint32_t)id*58u;uint8_t sum=0;
   assert(off<image_size);uint16_t size=image_size-off>58?58:(uint16_t)(image_size-off);
   rx[0]=p[4];rx[1]=p[5];rx[2]=(uint8_t)size;rx[3]=0;
   for(uint16_t i=0;i<size;i++)rx[4+i]=pixel(off+i);
   for(uint16_t i=0;i<size+4;i++)sum+=rx[i];
   rx[4+size]=sum;rx[5+size]=0;rxcount=size+6;packet_requests++;
   if(bad_once){rx[4+size]++;bad_once=0;}
  }break;
 default:assert(0);
 }
 rxready=now+((n+rxcount)*10000u+115199u)/115200u+2u;
}
static uint16_t crc16_calc(uint8_t *p,uint16_t n) {uint16_t c=0;while(n--)c+=*p++;return c;}
static uint8_t usb_send(uint8_t *p,uint16_t n) {if(fail_send)return 0;assert(n<=sizeof(wire));memcpy(wire,p,n);wirelen=n;assert(sends<32);memcpy(history[sends],p,n);history_len[sends]=n;sends++;return 1;}
static int8_t connection_state(uint8_t h) {(void)h;return connected?CE_CONNECTED:-1;}
static int connection_datasended(uint8_t h) {(void)h;return now>=send_ready_at;}
static int8_t connection_send(uint8_t h,uint8_t *p,uint16_t n) {(void)h;send_attempts++;if(now<send_ready_at)return -1;if(!usb_send(p,n))return -1;send_ready_at=now+3;return CE_OK;}
static cmdCode_e answer;
static void user_send_answer_comand(uint8_t cmd,cmdCode_e code,dataChanel_e channel,uint8_t id) {(void)cmd;(void)channel;(void)id;answer=code;}
uint32_t photo_storage_end(void);
uint8_t photo_recover(void);
uint8_t photo_cleanup_active(void);
uint16_t photo_confirmed_offset(uint32_t address,uint16_t size);
static int fail_alloc;
static size_t live_heap, peak_heap;
static void *host_malloc(size_t n) {
 size_t use=((n+7u)&~7u)+16u;
 if(fail_alloc || live_heap+use>5120)return NULL;
 size_t *p=malloc(use);assert(p);*p=use;live_heap+=use;if(live_heap>peak_heap)peak_heap=live_heap;return p+2;
}
static void host_free(void *p){if(p){size_t *h=(size_t*)p-2;live_heap-=*h;free(h);}}
static void *host_calloc(size_t n,size_t size){void *p=host_malloc(n*size);if(p)memset(p,0,n*size);return p;}
static unsigned debug_reasons;
static void user_debug(const char *text){(void)text;debug_reasons++;}
static unsigned photo_events;static uint32_t photo_logged;
static uint8_t log_addPhotoEvent(uint32_t number){photo_events++;photo_logged=number;return 1;}
#define malloc host_malloc
#define calloc host_calloc
#define free host_free
static void uart_setRecTimeout(uint8_t p,uint16_t t){(void)p;(void)t;}
uint32_t rs485_baud(void);
/* PRODUCTION */
#undef malloc
#undef calloc
#undef free

static void reset_ram(void) {host_free(photo);photo=NULL;photo_erased_to=photo_pending_since=0;photo_copy_off=photo_prep_phase=photo_restore_phase=0;memset(&device,0,sizeof(device));device.datetime=1789900000;uart_init(0,115200,0,port_buffer,32);for(int i=0;i<100 && !photo_cleanup_active() && !photo_recover();i++){int prior=erases;now++;photo_poll();assert(erases-prior<=1);}device.countPhoto=user_getPhotoCount();device.status.readyPhoto=device.countPhoto!=0;}
static void clean(void) {
 reset_ram();memset(flash,255,sizeof(flash));device.status.readyPhoto=0;mem[0].start=0;mem[0].size=1024*1024;
 memset(&config,0,sizeof(config));config.rs485Mode=1;config.rs485Periphery[0].address=17;config.rs485Periphery[0].type=3;
 rxready=0;now=0;image_size=2507;sent_commands=packet_requests=watchdog=0;
 quiet=nack=bad_once=fail_send=sends=erases=programs=fail_write=0;wirelen=0;connected=1;send_ready_at=0;send_attempts=0;
}
static void step(void){int prior=erases;now+=1;photo_poll();assert(erases-prior<=1);}
static void capture(void){if(user_getPhotoCount())device.datetime++;cmdCode_e result=photo_start(17);if(result==CMD_RUNERROR && photo_cleanup_active()){for(int i=0;i<300 && photo_cleanup_active();i++)step();assert(!photo_cleanup_active());result=photo_start(17);}assert(result==CMD_SUCCESS);for(int i=0;i<100000 && device.getPhoto;i++)step();assert(!device.getPhoto);}
static void check_image(uint32_t a) {
 photoParam_s h;mem_read(a,(uint8_t*)&h,8);assert(h.datetime==device.datetime && h.size==image_size);
 for(uint32_t i=0;i<image_size;i++)assert(flash[a+PHOTO_RECORD_HEAD+i]==pixel(i));
}
static void request_at(uint8_t id,uint32_t number,uint16_t offset,int explicit_offset) {
 uint8_t args[6];memcpy(args,&number,4);memcpy(args+4,&offset,2);
 int prior=erases;assert(photo_request(CH_SERVER,id,args,explicit_offset?6:0)==CMD_SUCCESS);assert(erases==prior);
}
static void drain(void){for(int i=0;i<1000 && photo && photo->pending;i++)step();assert(!photo || !photo->pending);}
static void request(uint8_t id){request_at(id,0,0,0);}
static void check_wire(uint16_t off,uint16_t n,uint8_t count) {
 assert(wire[6]==60 && wire[8]==14 && wire[9]==count);
 if(!count){assert(wirelen==12);return;}
 assert(wirelen==n+22);assert((wire[18]|((uint16_t)wire[19]<<8))==off);
 uint32_t stamp,size;memcpy(&stamp,wire+10,4);memcpy(&size,wire+14,4);assert(stamp==device.datetime && size==image_size);
 for(uint16_t i=0;i<n;i++)assert(wire[20+i]==pixel(off+i));
}
static void check_saved(int i,uint16_t off,uint16_t n,uint8_t count,uint32_t number) {
 assert(i<sends);wirelen=history_len[i];memcpy(wire,history[i],wirelen);uint32_t old=device.datetime;device.datetime=number;check_wire(off,n,count);device.datetime=old;
}
int main(int argc,char **argv) {
 if(argc>2 && !strcmp(argv[1],"bridge")) {
   clean();FILE *f=fopen(argv[2],"rb");assert(f);fseek(f,0,SEEK_END);image_size=(uint32_t)ftell(f);rewind(f);
   bridge_jpeg=malloc(image_size);assert(bridge_jpeg);assert(fread(bridge_jpeg,1,image_size,f)==image_size);fclose(f);
   capture();check_image(0);printf("BRIDGE_OK bytes=%u ms=%u\n",image_size,now);reset_ram();free(bridge_jpeg);return 0;
 }
 clean();uint8_t empty_args[6];uint32_t arbitrary=0xA5A5A5A5u;
 memcpy(empty_args,&arbitrary,4);empty_args[4]=0xFF;empty_args[5]=0xFF;
 user_send_answer_comand(21,photo_request(CH_SERVER,90,empty_args,6),CH_SERVER,90);
 assert(answer==CMD_SUCCESS && !programs && !erases && !device.status.readyPhoto);
 drain();assert(sends==1);check_saved(0,0,0,0,0);
 clean();command(7,4,99,1);assert(answer==CMD_PARAMERROR);command(7,3,17,1);assert(answer==CMD_PARAMERROR);
 command(21,4,0,1);assert(answer==CMD_PARAMERROR);unsigned events=photo_events;capture();check_image(0);assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 uint32_t stamp=device.datetime;assert(photo_events==events+1 && photo_logged==stamp); /* journal 129: number of the stored photo */
 request_at(1,0,0,1);drain();assert(sends==6 && user_getPhotoCount()==1 && device.status.readyPhoto);
 for(int i=0;i<5;i++)check_saved(i,512*i,i==4?459:512,1,stamp);
 check_saved(5,0,0,0,stamp);
 request(2);drain();assert(sends==12);check_saved(6,0,512,1,stamp);
 request_at(22,0,2507,1);drain();assert(sends==13 && !user_getPhotoCount() && !device.status.readyPhoto);check_saved(12,0,0,0,stamp);
 // ќбрыв останавливает поток, а €вна€ команда начинает его с прин€того смещени€.
 clean();capture();stamp=device.datetime;request_at(3,stamp,0,1);step();assert(sends==1);
 photo_connection_lost();for(int i=0;i<5;i++)step();assert(sends==1 && (!photo || !photo->pending));
 request_at(4,stamp,512,1);drain();assert(sends==6);check_saved(1,512,512,1,stamp);check_saved(5,0,0,0,stamp);
 // Ќова€ команда внутри потока замен€ет старый курсор.
 clean();capture();stamp=device.datetime;request_at(5,stamp,0,1);step();assert(sends==1);
 request_at(6,stamp,1024,1);drain();assert(sends==5);check_saved(1,1024,512,1,stamp);check_saved(4,0,0,0,stamp);
 uint8_t args[6];memcpy(args,&stamp,4);args[4]=0;args[5]=2;
 assert(photo_request(CH_SERVER,7,args,6)==CMD_SUCCESS);now=send_ready_at;fail_send=1;step();assert(sends==5 && photo->offset==1024 && photo->pending);
 fail_send=0;step();assert(sends==6);check_saved(5,1024,512,1,stamp);
 // «ан€тый модем удерживает блок; отсутствие QISACK не даЄт отправить следующий.
 int sent=sends, attempts=send_attempts;step();assert(sends==sent && send_attempts==attempts);drain();
 memcpy(args,&stamp,4);args[4]=0;args[5]=0;uint32_t foreign=stamp+1;memcpy(args,&foreign,4);assert(photo_request(CH_SERVER,8,args,6)==CMD_PARAMERROR);memcpy(args,&stamp,4);args[4]=0xB8;args[5]=0x0B;assert(photo_request(CH_SERVER,9,args,6)==CMD_PARAMERROR);assert(debug_reasons);
 clean();capture();stamp=device.datetime;request_at(8,stamp,0,1);step();assert(photo && photo->pending);
 uint32_t prior_address=photo->readAddress;uint16_t prior_offset=photo->offset;
 foreign=stamp+1u;memcpy(args,&foreign,4);args[4]=args[5]=0;
 user_send_answer_comand(21,photo_request(CH_SERVER,8,args,6),CH_SERVER,8);
 assert(answer==CMD_PARAMERROR && photo->pending && photo->readAddress==prior_address && photo->offset==prior_offset);
 memcpy(args,&stamp,4);args[4]=0xB8;args[5]=0x0B;
 user_send_answer_comand(21,photo_request(CH_SERVER,9,args,6),CH_SERVER,9);
 assert(answer==CMD_PARAMERROR && photo->pending && photo->readAddress==prior_address && photo->offset==prior_offset);
 drain();
 clean();capture();uint32_t first=device.datetime,next=device.newAddressPhoto;assert(photo_start(17)==CMD_SUCCESS);
 for(int i=0;i<100000 && device.getPhoto;i++)step();assert(!device.getPhoto);
 photoParam_s repeated;mem_read(next,(uint8_t*)&repeated,sizeof(repeated));assert(repeated.datetime==first+1u);
 device.datetime=repeated.datetime;check_image(next);assert(user_getPhotoCount()==2);
 request_at(10,0,0,1);drain();assert(sends==11 && user_getPhotoCount()==2);
 check_saved(0,0,512,2,first);check_saved(5,0,512,2,device.datetime);check_saved(10,0,0,0,device.datetime);
 request_at(11,first,2507,1);drain();assert(sends==17);check_saved(11,0,512,1,device.datetime);assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 uint32_t last=device.datetime;request_at(12,last,2507,1);assert(!device.status.readyPhoto);drain();assert(sends==18);check_saved(17,0,0,0,last);assert(!user_getPhotoCount() && !device.status.readyPhoto);int erased=erases;
 int programmed=programs;uint8_t stale[6];memcpy(stale,&last,4);stale[4]=0xCB;stale[5]=9;
 user_send_answer_comand(21,photo_request(CH_SERVER,13,stale,6),CH_SERVER,13);
 assert(answer==CMD_SUCCESS && erases==erased && programs==programmed && !device.status.readyPhoto);
 drain();assert(sends==19);check_saved(18,0,0,0,last);
 memcpy(stale,&arbitrary,4);stale[4]=0xFF;stale[5]=0xFF;
 user_send_answer_comand(21,photo_request(CH_SERVER,14,stale,6),CH_SERVER,14);
 assert(answer==CMD_SUCCESS && erases==erased && programs==programmed);
 drain();assert(sends==20);check_saved(19,0,0,0,last);
 clean();capture();uint32_t streamed=device.datetime;
 request(40);send_ready_at=now+20000;capture();uint32_t appended=device.datetime;
 assert(photo && photo->pending && user_getPhotoCount()==2 && device.status.readyPhoto);
 send_ready_at=now;drain();assert(sends==11);check_saved(0,0,512,2,streamed);check_saved(5,0,512,2,appended);
 request_at(41,streamed,2507,1);drain();assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 check_saved(11,0,512,1,appended);reset_ram();assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 request(42);drain();check_saved(17,0,512,1,appended);
 request_at(43,streamed,2507,1);drain();assert(user_getPhotoCount()==1);
 request_at(44,appended,2507,1);drain();assert(!user_getPhotoCount() && !device.status.readyPhoto);
 capture();check_image(0);
 // ¬о врем€ восстановлени€ общего сектора выгрузка читает только готовые данные.
 clean();image_size=1000;capture();uint32_t guarded=device.datetime;
 assert(photo_start(17)==CMD_SUCCESS);while(photo && !photo->written)step();
 device.status.readyToUpdate=1;step();device.status.readyToUpdate=0;
 request(97);send_ready_at=now+120000u;
 assert(photo_start(17)==CMD_SUCCESS);
 for(int i=0;i<1000 && photo_restore_phase!=1;i++)step();
 assert(photo_restore_phase==1 && photo && photo->pending);
 int guarded_sends=sends;send_ready_at=now;step();assert(sends==guarded_sends);
 for(int i=0;i<100000 && device.getPhoto;i++)step();
 drain();assert(sends && (history[0][9]==1 || history[0][9]==2));check_saved(0,0,512,history[0][9],guarded);
 puts("photo: capture during upload, durable prefix, reclaim OK"); clean();capture();uint32_t partial=device.datetime;
 request_at(45,partial,1024,1);drain();assert(photo_confirmed_offset(0,2507)==1024);
 reset_ram();assert(user_getPhotoCount()==1 && device.status.readyPhoto);request_at(46,0,0,1);drain();check_saved(4,1024,512,1,partial);
 request_at(47,partial,1536,1);drain();reset_ram();assert(photo_confirmed_offset(0,2507)==1536);
 request(48);drain();check_saved(11,1536,512,1,partial);
 puts("photo: partial acknowledgement survives reset OK"); clean();capture();uint32_t journal_first=device.datetime;capture();uint32_t journal_last=device.datetime;
 for(uint16_t i=1;i<=PHOTO_ACK_SLOTS;i++)request_at(51,journal_first,i,1);
 assert(photo_confirmed_offset(0,2507)==PHOTO_ACK_SLOTS);
 uint8_t full_journal[6];memcpy(full_journal,&journal_first,4);full_journal[4]=129;full_journal[5]=0;
 assert(photo_request(CH_SERVER,52,full_journal,6)==CMD_RUNERROR && user_getPhotoCount()==2);
 request_at(53,journal_first,2507,1);assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 reset_ram();assert(user_getPhotoCount()==1);request(54);drain();check_saved(0,0,512,1,journal_last);
 puts("photo: full confirmation after 128 partial offsets OK"); clean();mem[0].start=0x200000u;mem[0].size=1024*1024;user_getPhotoCount();
 image_size=50000;for(int i=0;i<20;i++)capture();
 assert(user_getPhotoCount()==20 && device.status.readyPhoto);
 uint32_t tail_number=device.datetime, tail_address=mem[0].start+19u*(PHOTO_RECORD_HEAD+50000u);
 assert(device.newAddressPhoto==mem[0].start+20u*(PHOTO_RECORD_HEAD+50000u));check_image(tail_address);
 uint32_t head_number=tail_number-19u;request_at(49,head_number,50000,1);photo_connection_lost();
 assert(user_getPhotoCount()==19 && device.status.readyPhoto);
 unsigned no_space=debug_reasons;capture();assert(user_getPhotoCount()==19 && debug_reasons>no_space);
 reset_ram();device.datetime=tail_number;check_image(tail_address);assert(user_getPhotoCount()==19);
 request_at(50,tail_number,50000,1);drain();assert(!user_getPhotoCount() && !device.status.readyPhoto);
 reset_ram();assert(device.newAddressPhoto==mem[0].start);image_size=2507;capture();check_image(mem[0].start);
 puts("photo: twenty images, blocked prefix, full reclaim OK"); clean();image_size=3;for(int i=0;i<20;i++)capture();uint32_t stream_first=device.datetime-19u;
 request(55);drain();assert(sends==21);
 for(int i=0;i<20;i++)check_saved(i,0,3,20,stream_first+(uint32_t)i);
 check_saved(20,0,0,0,device.datetime);
 request_at(56,stream_first+9u,3,1);drain();assert(user_getPhotoCount()==10 && device.status.readyPhoto);
 check_saved(21,0,3,10,stream_first+10u);reset_ram();assert(user_getPhotoCount()==10);
 puts("photo: twenty-image stream and unconfirmed packet count OK"); puts("photo: stream, resume, preemption, busy modem, two photos, explicit erase OK");
 clean();capture();request_at(21,device.datetime,512,1);fail_alloc=1;step();assert(!sends && photo->pending && photo->offset==512);fail_alloc=0;drain();check_saved(0,512,512,1,device.datetime);
 clean();capture();request_at(19,device.datetime,1024,1);drain();int again=sends;request(20);drain();check_saved(again,1024,512,1,device.datetime);
 clean();capture();request_at(14,device.datetime,0,1);step();int before=sends;device.status.readyToUpdate=1;step();assert(sends==before && (!photo || !photo->pending));device.status.readyToUpdate=0;
 request_at(15,device.datetime,512,1);drain();check_saved(before,512,512,1,device.datetime);
 clean();capture();request_at(16,device.datetime,0,1);step();before=sends;connected=0;step();assert(sends==before && (!photo || !photo->pending));connected=1;request_at(17,device.datetime,512,1);drain();check_saved(before,512,512,1,device.datetime);
 // Heap: один буфер 14 р€дом с двум€ буферами приЄма.
 clean();capture();void *tcp=host_calloc(2056,1),*urc=host_calloc(2056,1);assert(tcp && urc);request_at(18,device.datetime,0,1);drain();
 printf("photo: heap upper bound (8-byte alignment +16 per allocation)=%zu/5120 OK\n",peak_heap);host_free(tcp);host_free(urc);
 clean();assert(photo_start(17)==CMD_SUCCESS);
 tcp=host_calloc(2056,1);urc=host_calloc(2056,1);assert(tcp && urc);
 command(21,3,0,50);assert(answer==CMD_RUNERROR && device.getPhoto && !sends);
 command(7,4,17,51);assert(answer==CMD_RUNERROR && device.getPhoto);
 host_free(tcp);host_free(urc);while(device.getPhoto)step();check_image(0);
 clean();fail_alloc=1;assert(photo_start(17)==CMD_RUNERROR);fail_alloc=0;
 clean();assert(photo_start(99)==CMD_PARAMERROR && !sent_commands);device.status.readyToUpdate=1;assert(photo_start(17)==CMD_RUNERROR);assert(photo_request(CH_SERVER,1,NULL,0)==CMD_RUNERROR);step();assert(!sent_commands);
 clean();assert(photo_start(17)==CMD_SUCCESS);step();uint32_t old=sent_commands;device.status.readyToUpdate=1;step();assert(sent_commands==old && !device.getPhoto && !user_getPhotoCount());
 puts("photo: allocation/transport failure, wrong camera, update mode OK");
 clean();assert(photo_start(17)==CMD_SUCCESS);while(!photo->written)step();assert(device.getPhoto && !user_getPhotoCount());reset_ram();image_size=1000;capture();check_image(0);
 clean();image_size=65536;capture();assert(!user_getPhotoCount() && !packet_requests && !erases);
 clean();mem[0].size=12288;image_size=4090;capture();assert(!user_getPhotoCount() && !packet_requests && !erases);
 clean();image_size=65535;capture();check_image(0);assert(user_getPhotoCount()==1 && packet_requests>255);
 clean();image_size=15*1024;capture();check_image(0);printf("photo: 15 KiB wire+2ms silence+1ms main model: %u ms\n",now);
 clean();image_size=50*1024;capture();check_image(0);printf("photo: 50 KiB wire+2ms silence+1ms main model: %u ms\n",now);assert(now<15000);
 clean();quiet=1;capture();assert(!user_getPhotoCount() && sent_commands==30);
 clean();nack=1;capture();assert(!user_getPhotoCount());
 clean();bad_once=1;capture();check_image(0);assert(packet_requests==(image_size+57)/58+1);
 clean();fail_write=1;capture();assert(!user_getPhotoCount() && !device.status.readyPhoto);
 // —ектор стираетс€ по ходу записи, а не во врем€ команды 7.
 clean();int command_erases=erases;command(7,4,17,98);assert(answer==CMD_SUCCESS && erases==command_erases);
 for(int i=0;i<100000 && device.getPhoto;i++)step();check_image(0);
 clean();image_size=50*1024;int before_erase=erases;
 assert(photo_start(17)==CMD_SUCCESS && erases==before_erase);
 for(int i=0;i<100000 && device.getPhoto;i++)step();
 assert(!device.getPhoto && erases>before_erase);check_image(0);
 // ƒо получени€ времени номер нулевого кадра начинаетс€ с единицы.
 clean();device.datetime=0;capture();photoParam_s zero_time;
 mem_read(0,(uint8_t*)&zero_time,sizeof(zero_time));assert(zero_time.datetime==1);
 capture();mem_read(PHOTO_RECORD_HEAD+image_size,(uint8_t*)&zero_time,sizeof(zero_time));
 assert(zero_time.datetime==2);
 // ”спешный блок продлевает врем€ ожидани€ следующего блока.
 clean();capture();request(95);uint32_t stream_begin=photo_pending_since;
 step();assert(photo && photo->pending);
 now=stream_begin+59000u;send_ready_at=now;step();assert(photo && photo->pending);
 now=stream_begin+60000u;send_ready_at=now+120000u;step();
 assert(photo && photo->pending && photo->offset==1024);
 // «а 60 секунд без продвижени€ поток снимаетс€, подтверждени€ остаютс€.
 clean();capture();request_at(91,device.datetime,512,1);uint32_t held=photo->readAddress;
 uint16_t held_offset=photo->offset;unsigned old_debug=debug_reasons;
 fail_alloc=1;now=photo_pending_since+59998u;step();assert(photo && photo->pending);
 step();assert(!photo || !photo->pending);fail_alloc=0;
 assert(debug_reasons>old_debug && user_getPhotoCount()==1);
 request_at(92,device.datetime,512,1);assert(photo->readAddress==held && photo->offset==held_offset);
 send_ready_at=now+120000u;old_debug=debug_reasons;now=photo_pending_since+59998u;
 step();assert(photo && photo->pending);step();assert(!photo || !photo->pending);
 assert(debug_reasons>old_debug && user_getPhotoCount()==1);
 // ѕосле старого длинного фото короткое не открывает остатки старой записи.
 clean();image_size=50000;capture();uint32_t old_number=device.datetime;
 before_erase=erases;uint8_t final_request[6];memcpy(final_request,&old_number,4);final_request[4]=0x50;final_request[5]=0xC3;
 user_send_answer_comand(21,photo_request(CH_SERVER,93,final_request,6),CH_SERVER,93);
 assert(answer==CMD_SUCCESS && erases==before_erase);
 assert(photo_cleanup_active() && erases==before_erase && !device.status.readyPhoto);
 assert(photo_start(17)==CMD_RUNERROR && erases==before_erase);
 reset_ram();assert(photo_cleanup_active() && !user_getPhotoCount());
 for(int i=0;i<300 && photo_cleanup_active();i++)step();
 assert(!photo_cleanup_active() && erases-before_erase<=15);
 image_size=1000;capture();assert(user_getPhotoCount()==1 && device.newAddressPhoto==PHOTO_RECORD_HEAD+1000u);
 reset_ram();assert(user_getPhotoCount()==1);
 puts("photo: one erase per poll, number 1, stalled upload and shorter image OK");
 puts("photo: power interruption, limits, silence, NAK, checksum, write error OK");
 clean();assert(photo_start(17)==CMD_SUCCESS);step();assert(rxcount==12);uint8_t rest[9];memcpy(rest,rx+3,9);rxcount=3;now=rxready;photo_poll();assert(photo->used==3);memcpy(rx,rest,9);rxcount=9;now++;photo_poll();
 for(int i=0;i<100000 && device.getPhoto;i++)step();check_image(0);
 clean();capture();uint8_t *fixture=malloc(sizeof(flash));assert(fixture);device.datetime++;assert(photo_start(17)==CMD_SUCCESS);while(photo->written<200)step();memcpy(fixture,flash,sizeof(flash));
 int completed=0,cuts=0;
 for(int cut=1;cut<400;cut++) {
  cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));image_size=1000;flash_ops=0;cut_after=cut;int interrupted=setjmp(power_reset);if(!interrupted)capture();
  cut_after=0;reset_ram();uint16_t count=user_getPhotoCount();assert(count==1 || count==2);assert(!memcmp(flash,fixture,(PHOTO_RECORD_HEAD+2507)));
  if(count==1){capture();check_image((PHOTO_RECORD_HEAD+2507));assert(user_getPhotoCount()==2);}if(!interrupted){completed=1;break;}cuts++;
 }
 assert(completed);free(fixture);printf("photo: power loss at all %d flash boundaries OK\n",cuts); clean();capture();uint32_t marked_first=device.datetime;capture();
 request_at(69,marked_first,2507,1);photo_connection_lost();assert(user_getPhotoCount()==1);
 device.datetime++;assert(photo_start(17)==CMD_SUCCESS);while(photo->written<200)step();
 fixture=malloc(sizeof(flash));assert(fixture);memcpy(fixture,flash,sizeof(flash));
 int append_cuts=0,append_complete=0;
 for(int cut=1;cut<400;cut++) {
  cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));image_size=1000;flash_ops=0;cut_after=cut;
  int interrupted=setjmp(power_reset);if(!interrupted)capture();
  cut_after=0;reset_ram();uint16_t count=user_getPhotoCount();assert(count==1 || count==2);
  assert(photo_confirmed_offset(0,2507)==2507);
  assert(!memcmp(flash+PHOTO_RECORD_HEAD+2507u,fixture+PHOTO_RECORD_HEAD+2507u,PHOTO_RECORD_HEAD+2507u));
  if(!interrupted){append_complete=1;break;}append_cuts++;
 }
 assert(append_complete);free(fixture);printf("photo: marked prefix survives %d append cuts OK\n",append_cuts); clean();capture();uint32_t ack_first=device.datetime;capture();
 fixture=malloc(sizeof(flash));assert(fixture);memcpy(fixture,flash,sizeof(flash));
 int ack_cuts=0;
 for(int mode=0;mode<2;mode++) for(int cut=1;cut<40;cut++) {
  cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));flash_ops=0;byte_cuts=1;cut_after=cut;
  int interrupted=setjmp(power_reset);if(!interrupted)request_at(70,ack_first,mode?2507:1024,1);
  cut_after=byte_cuts=0;reset_ram();uint16_t count=user_getPhotoCount();assert(mode ? (count==1 || count==2) : count==2);
  if(!mode)assert(photo_confirmed_offset(0,2507)==0 || photo_confirmed_offset(0,2507)==1024);
  assert(!memcmp(flash+PHOTO_RECORD_HEAD,fixture+PHOTO_RECORD_HEAD,2507));
  assert(!memcmp(flash+2u*PHOTO_RECORD_HEAD+2507u,fixture+2u*PHOTO_RECORD_HEAD+2507u,2507));
  if(!interrupted)break;ack_cuts++;
 }
 assert(ack_cuts>0);free(fixture);
 clean();capture();capture();uint32_t ack_last=device.datetime;
 fixture=malloc(sizeof(flash));assert(fixture);memcpy(fixture,flash,sizeof(flash));
 int clear_cuts=0,completed_clear=0,checked_physical_empty=0;
 for(int cut=1;cut<300;cut++) {
  cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));flash_ops=0;byte_cuts=1;cut_after=cut;
  int interrupted=setjmp(power_reset);if(!interrupted){request_at(71,ack_last,2507,1);for(int i=0;i<300 && photo_cleanup_active();i++)step();assert(!photo_cleanup_active());}
  cut_after=byte_cuts=0;reset_ram();uint16_t count=user_getPhotoCount();assert(count<=1);
  if(count)assert(!memcmp(flash+PHOTO_RECORD_HEAD,fixture+PHOTO_RECORD_HEAD,2507));
  else if(device.newAddressPhoto) {int old_erases=erases,old_programs=programs;request_at(72,0xA5A5A5A5u,65535,1);
    assert(erases==old_erases && programs==old_programs && device.newAddressPhoto!=0 && !device.status.readyPhoto);
    if(!checked_physical_empty){int at=sends;drain();assert(sends==at+1);check_saved(at,0,0,0,0);checked_physical_empty=1;}
    else photo_connection_lost();}
  if(!interrupted){completed_clear=1;break;}clear_cuts++;
 }
 assert(completed_clear && checked_physical_empty);free(fixture);
 printf("photo: power loss at %d acknowledgement bytes and %d clear boundaries OK\n",ack_cuts,clear_cuts);
 // ¬се точки стирани€ заполненной области: после сброса очередь остаЄтс€ пустой.
 clean();image_size=50000;for(int i=0;i<20;i++)capture();
 uint32_t clear_bound=device.newAddressPhoto;uint32_t clear_last=device.datetime;
 request_at(94,clear_last,50000,1);assert(photo_cleanup_active());
 fixture=malloc(sizeof(flash));assert(fixture);memcpy(fixture,flash,sizeof(flash));
 int full_cuts=0,full_done=0;
 for(int cut=1;cut<600;cut++) {
   cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));
   flash_ops=0;byte_cuts=1;cut_after=cut;
   int interrupted=setjmp(power_reset);
   if(!interrupted){for(int i=0;i<300 && photo_cleanup_active();i++)step();assert(!photo_cleanup_active());}
   cut_after=byte_cuts=0;reset_ram();
   assert(!user_getPhotoCount() && !device.status.readyPhoto);
   if(photo_cleanup_active()) {
     int old_erases=erases;assert(photo_start(17)==CMD_RUNERROR && erases==old_erases);
     for(int i=0;i<300 && photo_cleanup_active();i++)step();
   }
   assert(!photo_cleanup_active() && !user_getPhotoCount());
   if(!interrupted){full_done=1;break;}full_cuts++;
 }
 assert(full_done && full_cuts>=500);
 for(uint32_t a=0;a<clear_bound+sizeof(photoParam_s);a++)assert(flash[a]==0xFF);
 free(fixture);printf("photo: full-area cleanup survives %d flash cuts OK\n",full_cuts);
 // —брос между последним подтверждением и маркером тоже запускает очистку.
 clean();capture();capture();
 assert(photo_ack_request(device.datetime,2507)==CMD_SUCCESS);
 reset_ram();assert(!photo_cleanup_active() && !device.countPhoto && device.newAddressPhoto);
 step();assert(photo_cleanup_active() && !device.status.readyPhoto);
 for(int i=0;i<300 && photo_cleanup_active();i++)step();
 assert(!photo_cleanup_active() && !user_getPhotoCount() && !device.newAddressPhoto);
 // ƒлинный кадр пересекает 13 секторов; провер€ем каждую запись и стирание.
 clean();image_size=50000;capture();fixture=malloc(sizeof(flash));assert(fixture);
 memcpy(fixture,flash,sizeof(flash));int lazy_cuts=0,lazy_done=0;
 for(int cut=1;cut<1100;cut++) {
   cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));
   flash_ops=0;cut_after=cut;int interrupted=setjmp(power_reset);
   if(!interrupted)capture();
   cut_after=0;reset_ram();uint16_t n=user_getPhotoCount();assert(n==1 || n==2);
   assert(!memcmp(flash,fixture,PHOTO_RECORD_HEAD+50000u));
   if(n==1){capture();assert(user_getPhotoCount()==2);}
   if(!interrupted){lazy_done=1;break;}lazy_cuts++;
 }
 assert(lazy_done && lazy_cuts>850);free(fixture);
 printf("photo: lazy sector erase survives %d flash cuts OK\n",lazy_cuts);
 reset_ram();assert(!live_heap);return 0;
}
