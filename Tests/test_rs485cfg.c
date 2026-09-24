// Доработка: Кряжев А., 2026
#include <stddef.h>
#define SHTAMP_CONFIG 4
#define MB_CONFIG 1
static unsigned saved_config, config_ack;
static void user_writeConfigAndDevice(void){saved_config++;}
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n){(void)a;(void)n;return 0;}
static void user_send_timeshtamp(int k,uint32_t t,uint16_t crc,dataChanel_e ch,uint8_t id){(void)k;(void)t;(void)crc;(void)ch;(void)id;config_ack++;}
static void receive_config(dataChanel_e cch, uint8_t code) {
 uint8_t frame[sizeof(config_s)+5]={0},*ptr=frame,uniqId=7;
 uint16_t len=sizeof(config_s)+3;
 memcpy(ptr+5,&config,sizeof(config));ptr[4]=12;ptr[13]='s';ptr[142]='7';ptr[5+174]=code;
 switch(ptr[4]) { /* COMMAND12 */ }
}
static void test_rs485cfg(void) {
 const uint32_t rates[]={115200,57600,38400,19200,9600};
 assert(sizeof(config_s)==512 && offsetof(config_s,reserv)==174);
 clean();
 for(unsigned i=0;i<256;i++) {
   config.reserv[0]=(uint8_t)i;unsigned before=debug_reasons;rs485_init();
   assert(baudrate==rates[i<=4?i:0]);
   if(i>4)assert(config.reserv[0]==0 && debug_reasons>before);
   assert(photo_start(17)==CMD_SUCCESS && baudrate==rates[i<=4?i:0]);
   photo_stop();assert(baudrate==rates[i<=4?i:0]);
 }
 for(unsigned ch=CH_USB;ch<=CH_SERVER;ch++) {
   for(unsigned i=0;i<5;i++) {receive_config((dataChanel_e)ch,(uint8_t)i);assert(baudrate==rates[i]);}
 }
 assert(saved_config==10 && config_ack==10);
 clean();config.reserv[1]=51;config.reserv[2]=52;config.reserv[3]=53;
 config.rs485Periphery[1].address=0;config.rs485Periphery[1].type=255;
 config.rs485Mode=255;receive_config(CH_USB,255);
 assert(config.rs485Mode==0 && config.rs485Periphery[1].type==255 && config.reserv[1]==51 && config.reserv[2]==52 && config.reserv[3]==53);
 // Сохранённый снимок и незавершённый второй: отмена не удаляет первый.
 clean();capture();device.datetime++;assert(photo_start(17)==CMD_SUCCESS);
 while(photo->stage && photo->written<100)step();assert(photo->stage);
 receive_config(CH_SERVER,4);assert(!device.getPhoto && photo->stage==0 && baudrate==9600);
 assert(user_getPhotoCount()==1);uint32_t stamp=device.datetime;device.datetime--;check_image(0);device.datetime=stamp;
 // Изменение состава на той же скорости тоже прерывает съёмку.
 assert(photo_start(17)==CMD_SUCCESS);config_s next=config;next.rs485Periphery[1].type=6;
 rs485_set_config((const uint8_t*)&next);assert(!device.getPhoto);
 // Выгрузка, включая ещё не отправленный запрос, сохраняет свой курсор.
 request(1);request_at(2,device.datetime-1,512,1);step();
 assert(photo_request(CH_SERVER,3,NULL,0)==CMD_SUCCESS);
 uint32_t cursor=photo->offset;next=config;next.reserv[0]=2;next.rs485Periphery[1].type=2;
 rs485_set_config((const uint8_t*)&next);assert(photo->pending && photo->offset==cursor && device.status.readyPhoto);
 int at=sends;step();assert(photo->offset==cursor && user_getPhotoCount()==1);drain();check_saved(at,512,512,1,device.datetime-1);
 // Камера в смешанном списке: никакого трафика для других типов.
 clean();config.rs485Periphery[1].type=2;config.rs485Periphery[1].address=99;
 unsigned logs=debug_reasons;rs485_init();assert(debug_reasons>logs);
 assert(photo_start(99)==CMD_PARAMERROR && sent_commands==0);capture();assert(sent_commands>0);
 // Адрес 0 допустим; тип 3 не подменяем и не ограничиваем индексом INI.
 clean();config.rs485Periphery[0].address=0;assert(photo_start(0)==CMD_SUCCESS);
 next=config;next.rs485Periphery[0].type=255;rs485_set_config((const uint8_t*)&next);
 assert(photo_start(0)==CMD_PARAMERROR);reset_ram();
 puts("rs485cfg: 256 codes, USB/server command12, reserved bytes, cancel capture, preserve upload, exclusive C328 OK");
}
