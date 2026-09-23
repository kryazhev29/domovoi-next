// Доработка: Кряжев А., 2026
// Эмулятор C328 и NOR; исполняется производственный код.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
#include <setjmp.h>
#include <time.h>
static jmp_buf power_reset;
static int cut_after, flash_ops;
static void power_cut(void) { if(cut_after && ++flash_ops==cut_after) longjmp(power_reset,1); }
typedef enum { CMD_SUCCESS, CMD_PARAMERROR, CMD_RUNERROR } cmdCode_e;
typedef enum {CH_USB, CH_SERVER, CH_SMS} dataChanel_e;
typedef struct { uint32_t start, size; } cellMem_s;
typedef struct { uint32_t datetime, size; } photoParam_s;
typedef struct { uint8_t address,type; } periphery_s;
typedef struct { uint8_t rs485Mode; periphery_s rs485Periphery[8]; } config_s;
typedef struct {
 struct { uint8_t readyToUpdate, readyPhoto; } status;
 uint32_t datetime, newAddressPhoto; uint16_t countPhoto, readPhotoIndex; uint8_t getPhoto, connectionhandle;
} device_s;
#define PORTBUFSIZE 32
#define PORT1_PORT 0
#define MEMSECTORSIZE 4096u
#define MB_PHOTO 0
#define CE_OK 0
cellMem_s mem[16]; config_s config; device_s device;
uint8_t port_buffer[PORTBUFSIZE];
static uint8_t flash[1024*1024], *rx, wire[1100];
static uint16_t rxcap, rxcount, wirelen;
static uint32_t now, image_size, sent_commands, packet_requests, watchdog;
static uint8_t *bridge_jpeg;
static uint32_t rxready;
static int quiet, nack, bad_once, fail_send, sends, erases, fail_write;
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
 assert(a+n <= sizeof(flash)); if (fail_write) return; while(n--) flash[a++] &= *p++; power_cut();
}
static void mem_erase_sector(uint32_t a) { assert(a%4096 == 0); assert(a+4096 <= sizeof(flash)); memset(flash+a,255,4096); erases++; power_cut(); }
static void user_clearMemBlock(uint8_t block) { (void)block; for(uint32_t a=0;a<mem[0].size;a+=4096) {wdt_reload();mem_erase_sector(a);} }
static int uart_init(uint8_t u,uint32_t rate,uint8_t stop,uint8_t *p,uint16_t n) {(void)u;assert(rate==115200 && !stop);rx=p;rxcap=n;rxcount=0;return 1;}
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
static uint8_t usb_send(uint8_t *p,uint16_t n) {if(fail_send)return 0;assert(n<=sizeof(wire));memcpy(wire,p,n);wirelen=n;sends++;return 1;}
static int8_t connection_send(uint8_t h,uint8_t *p,uint16_t n) {(void)h;return usb_send(p,n)?CE_OK:-1;}
static cmdCode_e answer;
static void user_send_answer_comand(uint8_t cmd,cmdCode_e code,dataChanel_e channel,uint8_t id) {(void)cmd;(void)channel;(void)id;answer=code;}
uint32_t photo_storage_end(void);
uint8_t photo_recover(void);
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
#define malloc host_malloc
#define calloc host_calloc
#define free host_free
/* PRODUCTION */
#undef malloc
#undef calloc
#undef free

static void reset_ram(void) {host_free(photo);photo=NULL;memset(&device,0,sizeof(device));device.datetime=1789900000;uart_init(0,115200,0,port_buffer,32);device.countPhoto=user_getPhotoCount();device.status.readyPhoto=device.countPhoto!=0;}
static void clean(void) {
 reset_ram();memset(flash,255,sizeof(flash));device.status.readyPhoto=0;mem[0].start=0;mem[0].size=sizeof(flash);
 memset(&config,0,sizeof(config));config.rs485Mode=1;config.rs485Periphery[0].address=17;config.rs485Periphery[0].type=3;
 rxready=0;now=0;image_size=2507;sent_commands=packet_requests=watchdog=0;
 quiet=nack=bad_once=fail_send=sends=erases=fail_write=0;wirelen=0;
}
static void step(void){now+=1;photo_poll();}
static void capture(void){if(user_getPhotoCount())device.datetime++;assert(photo_start(17)==CMD_SUCCESS);for(int i=0;i<100000 && device.getPhoto;i++)step();assert(!device.getPhoto);}
static void check_image(uint32_t a) {
 photoParam_s h;mem_read(a,(uint8_t*)&h,8);assert(h.datetime==device.datetime && h.size==image_size);
 for(uint32_t i=0;i<image_size;i++)assert(flash[a+8+i]==pixel(i));
}
static void request_at(uint8_t id,uint32_t number,uint16_t offset,int explicit_offset) {
 uint8_t args[6];memcpy(args,&number,4);memcpy(args+4,&offset,2);
 assert(photo_request(CH_SERVER,id,args,explicit_offset?6:0)==CMD_SUCCESS);
 step();assert(!photo || !photo->pending);
}
static void request(uint8_t id){request_at(id,0,0,0);}
static void check_wire(uint16_t off,uint16_t n,uint8_t count) {
 assert(wire[6]==60 && wire[8]==14 && wire[9]==count);
 if(!count){assert(wirelen==12);return;}
 assert(wirelen==n+22);assert((wire[18]|((uint16_t)wire[19]<<8))==off);
 uint32_t stamp,size;memcpy(&stamp,wire+10,4);memcpy(&size,wire+14,4);assert(stamp==device.datetime && size==image_size);
 for(uint16_t i=0;i<n;i++)assert(wire[20+i]==pixel(off+i));
}
int main(int argc,char **argv) {
 if(argc>2 && !strcmp(argv[1],"bridge")) {
   clean();FILE *f=fopen(argv[2],"rb");assert(f);fseek(f,0,SEEK_END);image_size=(uint32_t)ftell(f);rewind(f);
   bridge_jpeg=malloc(image_size);assert(bridge_jpeg);assert(fread(bridge_jpeg,1,image_size,f)==image_size);fclose(f);
   capture();check_image(0);printf("BRIDGE_OK bytes=%u ms=%u\n",image_size,now);reset_ram();free(bridge_jpeg);return 0;
 }
 clean();command(7,4,99,1);assert(answer==CMD_PARAMERROR);command(7,3,17,1);assert(answer==CMD_PARAMERROR);
 command(21,4,0,1);assert(answer==CMD_PARAMERROR);capture();check_image(0);assert(user_getPhotoCount()==1 && device.status.readyPhoto);
 uint32_t stamp=device.datetime;request(1);check_wire(0,512,1);
 // Пакет потерян: повтор с любым uniqId не подтверждает его.
 request(2);check_wire(0,512,1);request_at(3,stamp,512,1);check_wire(512,512,1);
 request_at(4,stamp,512,1);check_wire(512,512,1);request(5);check_wire(512,512,1);
 request_at(6,stamp+1,1024,1);check_wire(0,512,1);assert(debug_reasons);
 request_at(7,stamp,3000,1);check_wire(0,512,1);
 request_at(8,stamp,1024,1);check_wire(1024,512,1);reset_ram();request_at(9,stamp,1536,1);check_wire(1536,512,1);
 request_at(10,stamp,2048,1);check_wire(2048,459,1);request(11);check_wire(2048,459,1);assert(device.status.readyPhoto);
 request_at(12,stamp,2507,1);check_wire(0,0,0);assert(!device.status.readyPhoto && !user_getPhotoCount());request_at(13,stamp,2507,1);check_wire(0,0,0);
 puts("photo: explicit offsets, lost packet, repeat/new uniqId, foreign number/range, reset, end OK");
 clean();capture();assert(photo_start(17)==CMD_RUNERROR);uint32_t first=device.datetime,next=device.newAddressPhoto;device.datetime++;capture();check_image(next);assert(user_getPhotoCount()==2);
 request_at(254,first,2507,1);check_wire(0,512,1);request_at(255,first,2507,1);check_wire(0,512,1);assert(user_getPhotoCount()==2);
 request_at(0,device.datetime,512,1);check_wire(512,512,1);request_at(1,device.datetime,2507,1);check_wire(0,0,0);assert(!user_getPhotoCount());
 puts("photo: two images, repeated completion never deletes next image OK");
 clean();capture();request(1);int sent=sends;fail_alloc=1;
 uint8_t args[6];memcpy(args,&device.datetime,4);args[4]=0;args[5]=2;
 assert(photo_request(CH_SERVER,2,args,6)==CMD_SUCCESS);step();assert(photo->pending && photo->offset==0 && sends==sent);
 fail_alloc=0;request_at(3,device.datetime,512,1);check_wire(512,512,1);
 fail_send=1;assert(photo_request(CH_SERVER,4,args,6)==CMD_SUCCESS);step();assert(photo->offset==512 && photo->pending);fail_send=0;step();check_wire(512,512,1);
 // Искусственно совмещаем оба максимальных буфера с пакетом и контекстом фото.
 void *tcp=host_calloc(2056,1),*urc=host_calloc(2056,1);assert(tcp && urc);request_at(5,device.datetime,1024,1);check_wire(1024,512,1);
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
 puts("photo: power interruption, limits, silence, NAK, checksum, write error OK");
 clean();capture();request(10);assert(photo_request(CH_SERVER,11,NULL,0)==CMD_SUCCESS);int before=sends;device.status.readyToUpdate=1;step();assert(sends==before);device.status.readyToUpdate=0;request(12);check_wire(0,512,1);
 clean();assert(photo_start(17)==CMD_SUCCESS);step();assert(rxcount==12);uint8_t rest[9];memcpy(rest,rx+3,9);rxcount=3;now=rxready;photo_poll();assert(photo->used==3);memcpy(rx,rest,9);rxcount=9;now++;photo_poll();
 for(int i=0;i<100000 && device.getPhoto;i++)step();check_image(0);
 clean();capture();uint8_t *fixture=malloc(sizeof(flash));assert(fixture);device.datetime++;assert(photo_start(17)==CMD_SUCCESS);while(photo->written<200)step();memcpy(fixture,flash,sizeof(flash));
 int completed=0,cuts=0;
 for(int cut=1;cut<120;cut++) {
  cut_after=0;reset_ram();memcpy(flash,fixture,sizeof(flash));image_size=1000;flash_ops=0;cut_after=cut;int interrupted=setjmp(power_reset);if(!interrupted)capture();
  cut_after=0;reset_ram();uint16_t count=user_getPhotoCount();assert(count==1 || count==2);assert(!memcmp(flash,fixture,2515));
  if(count==1){capture();check_image(2515);assert(user_getPhotoCount()==2);}if(!interrupted){completed=1;break;}cuts++;
 }
 assert(completed);free(fixture);printf("photo: power loss at all %d flash boundaries OK\n",cuts);reset_ram();assert(!live_heap);return 0;
}
