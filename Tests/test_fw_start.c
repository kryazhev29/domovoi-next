// Доработка: Кряжев А., 2026
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t boot_ver=0x32;
#define BOOT_VER boot_ver
#define FWBLOCK_OFFSET (uint32_t)0x200
#define CPU_ENDMEMORYADDRESS (uint32_t)0x0801ffff
#define APLICATION_ADDRESS (uint32_t)(0x08008000 + FWBLOCK_OFFSET)
/* ТИПЫ */
typedef enum {MB_FIRMWARE=0, MB_BACKUP=1} memBlock_e;
/* Карта приложения: область образа, затем резервная копия прошивки. */
static struct {uint32_t start,size;} mem[2]={{0,131072},{135168,131072}};
static uint8_t flash[135168+131072];
static unsigned saves,wdt,torn,sent,sent_code,sent_cch,trial,manual,fail_flash;
static uint32_t stuck=0xffffffffu;
static uint32_t bf_words[32];
#define bf ((uint8_t *)bf_words)
#define bf32 bf_words
static uint32_t read_fail_at=UINT32_MAX;
static unsigned read_fail_copy;
// Проверяет сценарий mem_read по независимым данным модуля.
static bool mem_read(uint32_t a,uint8_t*p,uint16_t n){CHECK(a+n<=sizeof(flash));if(a==read_fail_at){if(read_fail_copy)memcpy(p,flash+a,n);return false;}memcpy(p,flash+a,n);return true;}
/* NOR только сбрасывает биты. Обрыв оставляет вторую половину стёртой; неисправный адрес не записывается. */
static bool mem_write(uint32_t a,uint8_t*p,uint16_t n){uint32_t i;CHECK(a+n<=sizeof(flash));
 if(fail_flash==2)return false;
 for(i=0;i<n;i++){if(torn&&i>=n/2u)break;if(a+i==stuck)continue;flash[a+i]&=p[i];}if(torn)torn--;return true;}
static unsigned erases;
// Проверяет сценарий mem_erase_sector по независимым данным модуля.
static bool mem_erase_sector(uint32_t a){CHECK(a%4096u==0&&a+4096u<=sizeof(flash));if(fail_flash==1)return false;memset(flash+a,0xff,4096);erases++;return true;}
// Проверяет сценарий crc_word по независимым данным модуля.
static uint32_t crc_word(uint32_t c,uint32_t v){unsigned i;c^=v;for(i=0;i<32;i++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0);return c;}
// Проверяет сценарий stm_crc по независимым данным модуля.
static uint32_t stm_crc(const uint8_t*p,uint32_t n){uint32_t c=0xffffffffu,w,i;for(i=0;i<n;i+=4){memcpy(&w,p+i,4);c=crc_word(c,w);}return c;}
/* CRC ПРОШИВКИ */
static uint16_t saved_timeout;
static void user_saveindex(void);
// Проверяет сценарий wdt_reload по независимым данным модуля.
static void wdt_reload(void){wdt++;}
// Проверяет сценарий app_trial_active по независимым данным модуля.
static int app_trial_active(void){return trial;}
// Проверяет сценарий boot_manual_busy по независимым данным модуля.
static int boot_manual_busy(void){return manual;}
static struct{int num;uint32_t reason;}logs[64];static unsigned nlogs;
// Проверяет сценарий log_addUpdateEvent по независимым данным модуля.
static bool log_addUpdateEvent(logUpdate_e num,uint32_t reason){CHECK(nlogs<64);logs[nlogs].num=num;logs[nlogs].reason=reason;nlogs++;return true;}
static struct{uint8_t FWinProgress,rebootToUpdate,firmwarePercent,cchFWupdate,hardwareVersion,registredOnServer;
 uint16_t updateTimeout,FWblockIdx,FWblockCount;uint32_t startWaitFWtime;struct{uint16_t readyToUpdate:1;}status;}device;
// Проверяет сценарий user_saveindex по независимым данным модуля.
static void user_saveindex(void){saves++;saved_timeout=device.updateTimeout;}
// Проверяет сценарий user_send_statusUpdateFW по независимым данным модуля.
static void user_send_statusUpdateFW(uint8_t cmd,uint8_t code,dataChanel_e cch,uint8_t uniqId){(void)cmd;(void)uniqId;sent++;sent_code=code;sent_cch=cch;}
static void fw_resume_after_boot(void);
/* ИСХОДНИК */

static uint8_t image[96*1024];
// Проверяет сценарий reset по независимым данным модуля.
static void reset(void){memset(flash,0xff,sizeof(flash));memset(&device,0,sizeof(device));device.hardwareVersion=139;device.registredOnServer=1;
 fw_rollback_pos=fw_rollback_end=0;saves=torn=sent=trial=manual=nlogs=fail_flash=0;stuck=0xffffffffu;boot_ver=0x32;read_fail_at=UINT32_MAX;read_fail_copy=0;}
/* Эталон события E534 переводит имя причины в код; NULL означает ноль. */
static int last_log(int num,const char*reason){return nlogs&&logs[nlogs-1].num==num&&logs[nlogs-1].reason==fw_reason_code(reason);}
static cmdCode_e start(uint16_t number,uint32_t build,uint32_t size,uint32_t crc,uint16_t blocks,dataChanel_e cch){
 uint8_t p[16];memcpy(p,&number,2);memcpy(p+2,&build,4);memcpy(p+6,&size,4);memcpy(p+10,&crc,4);memcpy(p+14,&blocks,2);return fw_start(p,cch);}
// Проверяет сценарий block по независимым данным модуля.
static uint8_t block(uint16_t k,uint16_t n,dataChanel_e cch){static uint8_t p[1030];uint32_t i;memcpy(p,&k,2);
 for(i=0;i<1024;i++)p[2+i]=(uint32_t)k*1024u+i<sizeof(image)?image[(uint32_t)k*1024u+i]:0xff;return fw_take_block(p,n,cch);}
// Проверяет сценарий make_image по независимым данным модуля.
static void make_image(uint32_t size){uint32_t seed=777u,i,w=0x4857008Bu;memset(image,0xff,sizeof(image));
 for(i=0;i<size;i++){seed=seed*1103515245u+12345u;image[i]=(uint8_t)(seed>>16);}
 memcpy(image+0x20,&w,4);}/* Маркер платы 139 находится в резервном векторе 8. */

// Проверяет сценарий start_tests по независимым данным модуля.
static void start_tests(void){uint32_t size=66716,crc;uint16_t blocks=66;
 make_image(size);crc=stm_crc(image,size);
 /* Проверяем нулевой номер, откат с нулевыми полями и каждое повреждённое поле. */
 CHECK(fw_start_rejected(0,1,4,1,1));CHECK(!fw_start_rejected(1,0,0,0,0));CHECK(!fw_start_rejected(1,0xffffffff,3,0,0xffff));
 CHECK(!fw_start_rejected(64,1789454349,65016,0x57d1c3a7,64));CHECK(!fw_start_rejected(2,1,4,1,1));
 CHECK(!fw_start_rejected(64,1,67584,1,66));/* Выравнивание таблицы прошивки до 1024 Б. */
 CHECK(fw_start_rejected(64,0,65016,0x57d1c3a7,64));CHECK(fw_start_rejected(64,1789454349,0,0x57d1c3a7,64));
 CHECK(fw_start_rejected(64,1789454349,65015,0x57d1c3a7,64));CHECK(fw_start_rejected(64,1789454349,65016,0,64));
 CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,0));CHECK(fw_start_rejected(64,1789454349,65016,0x57d1c3a7,63));
 CHECK(!fw_start_rejected(64,1,0x17e00,1,0x60));CHECK(fw_start_rejected(64,1,0x17e04,1,0x60));CHECK(fw_start_rejected(64,1,4,1,128));
 /* Серверный старт: ответ 0, бит 11, таймаут 10 минут, заголовок, событие и сохранение. */
 reset();flash[5000]=0;
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS);
 CHECK(device.FWinProgress&&device.status.readyToUpdate&&device.updateTimeout==600&&device.cchFWupdate==CH_SERVER);
 CHECK(device.FWblockIdx==0&&device.FWblockCount==blocks&&device.firmwarePercent==0&&saves==1&&flash[5000]==0);
 {bootFWparam_s h;memcpy(&h,flash,sizeof(h));CHECK(h.appIdent==0xffffffffu&&h.firmwareVersion==71&&h.buildDate==1789600000&&h.firmwareSize==size&&h.firmwareCRC==crc&&h.hardwareVersion==139);}
 CHECK(last_log(LOG_FW_START,NULL));
 /* Повтор того же файла на том же канале продолжается с немедленным запросом блока. */
 device.FWblockIdx=9;device.startWaitFWtime=1234;saves=0;
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS&&device.FWblockIdx==9&&!device.startWaitFWtime&&!saves);
 /* Другой файл или тот же файл по USB во время загрузки даёт код 2 без изменения данных. */
 CHECK(start(72,1789600000,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"BUSY")&&device.FWblockIdx==9);
 CHECK(start(71,1789600000,size,crc,blocks,CH_USB)==CMD_RUNERROR&&device.FWinProgress&&device.cchFWupdate==CH_SERVER);
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"BUSY"));
 /* Неверные поля дают код 1. */
 CHECK(start(0,1,4,1,1,CH_SERVER)==CMD_PARAMERROR&&last_log(LOG_FW_FAIL,"PARAM"));
 /* Временный отказ даёт код 2 с причиной, без стирания. */
 reset();flash[5000]=0;boot_ver=0x21;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"OLD BOOT"));
 boot_ver=0x32;manual=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"BOOT BUSY"));
 manual=0;trial=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"IN TRIAL"));
 trial=0;device.rebootToUpdate=1;CHECK(start(71,1,size,crc,blocks,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"BUSY"));
 CHECK(flash[5000]==0&&!device.FWinProgress&&!device.status.readyToUpdate);
 /* USB: NextConfig передаёт блоки без бита 11, таймаута и сохранения. */
 reset();CHECK(start(71,1,size,crc,blocks,CH_USB)==CMD_SUCCESS&&!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!saves&&device.cchFWupdate==CH_USB);
 /* Команда 19 стирает только сектор 0; следующие сектора стираются перед первым блоком, чтобы не остановить связь на секунды. */
 reset();memset(flash,0x5a,0x20000);erases=0;CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS&&erases==1&&flash[5000]==0x5a);
 {uint16_t k;for(k=0;k<blocks;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);}
 CHECK(!memcmp(flash+0x200,image,size)&&device.rebootToUpdate&&erases==1u+(0x200u+blocks*1024u-1u)/4096u);
 puts("command 19: fields, answer 0 with bit 11 and the 10-minute timeout, carry on with the same file, 2 with its reason, 1 PARAM, USB");}

// Проверяет сценарий rollback_tests по независимым данным модуля.
static void rollback_tests(void){uint32_t size=66716;bootFWparam_s h;
 make_image(size);
 /* Отсутствие резерва или неверная CRC резерва дают код 2 с соответствующей причиной. */
 reset();CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"NO BACKUP")&&!device.rebootToUpdate);
 memset(&h,0xff,sizeof(h));h.appIdent=0x4E465744u;h.hardwareVersion=139;h.firmwareVersion=70;h.firmwareSize=size;h.firmwareCRC=stm_crc(image,size)^1u;h.buildDate=1789000000u;
 memcpy(flash+135168,&h,sizeof(h));memcpy(flash+135168+0x200,image,size);
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR&&last_log(LOG_FW_FAIL,"BAD BACKUP")&&!device.rebootToUpdate);
 /* Целый резерв копируется в область приложения без магии; ставится бит 11 и запрос перезапуска. */
 h.firmwareCRC^=1u;memcpy(flash+135168,&h,sizeof(h));flash[100]=0;
 {unsigned before=erases,steps=0;
 CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_SUCCESS&&!device.rebootToUpdate&&device.status.readyToUpdate&&device.cchFWupdate==CH_SERVER);
 CHECK(erases==before);CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_RUNERROR);
 while(!device.rebootToUpdate&&steps++<1000){before=erases;fw_rollback_poll();CHECK(erases-before<=1);}
 CHECK(device.rebootToUpdate&&steps<1000);}
 CHECK(!memcmp(flash+4,flash+135168+4,size+0x200-4)&&!memcmp(flash,"\xff\xff\xff\xff",4)&&last_log(LOG_FW_ROLLBACK,NULL));
 /* Ошибка позднего шага запрещает установку и снимает бит 11. */
 for(unsigned failure=1;failure<=2;failure++){
  device.rebootToUpdate=0;device.status.readyToUpdate=0;fail_flash=failure;
  CHECK(start(1,0,0,0,0,CH_SERVER)==CMD_SUCCESS);fw_rollback_poll();
  CHECK(!device.rebootToUpdate&&!device.status.readyToUpdate&&!fw_rollback_end&&sent_code==FW_BLOCK_END);
  CHECK(last_log(LOG_FW_FAIL,"WRITE"));
  fail_flash=0;
 }
 puts("rollback: no backup, bad backup, staged copy with the magic erased");}

// Проверяет сценарий download по независимым данным модуля.
static void download(uint32_t size,uint16_t blocks,uint32_t crc){reset();
 CHECK(start(71,1789600000,size,crc,blocks,CH_SERVER)==CMD_SUCCESS);}

// Проверяет сценарий block_tests по независимым данным модуля.
static void block_tests(void){uint32_t size=66716,crc;uint16_t blocks=66,k;
 make_image(size);crc=stm_crc(image,size);
 /* Полная загрузка: код 0 каждому блоку, процент растёт, последний блок запрашивает установку. */
 download(size,blocks,crc);
 for(k=0;k<blocks;k++){device.startWaitFWtime=5;device.updateTimeout=5;CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);CHECK(!device.startWaitFWtime);
  /* Ни один блок не пересекает границу; последний завершает загрузку. */
  CHECK(device.updateTimeout==(k+1u<blocks?5:0));
  CHECK(device.firmwarePercent==(k+1u)*100u/blocks);CHECK((k+1u<blocks)==(device.FWinProgress!=0));}
 CHECK(device.rebootToUpdate&&!device.status.readyToUpdate&&!device.updateTimeout&&device.firmwarePercent==100);
 CHECK(!memcmp(flash+0x200,image,(uint32_t)blocks*1024u));
 /* После конца поздний блок подтверждается кодом 0; выход за образ даёт 253. */
 CHECK(block(blocks-1,1026,CH_SERVER)==FW_BLOCK_OK&&block(blocks,1026,CH_SERVER)==FW_BLOCK_END);
 /* Выравнивание таблицы до 1024 Б сохраняет то же поведение. */
 make_image(67584);download(67584,66,stm_crc(image,67584));for(k=0;k<66;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);CHECK(device.rebootToUpdate);
 make_image(size);
 /* Неверная длина или будущий блок: 255, таймер ожидания сохранён; поздний повтор: 0, без записи. */
 download(size,blocks,crc);CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK&&block(1,1026,CH_SERVER)==FW_BLOCK_OK);
 device.startWaitFWtime=5;device.updateTimeout=5;CHECK(block(2,1025,CH_SERVER)==FW_BLOCK_BADFRAME&&device.startWaitFWtime==5&&device.FWblockIdx==2);
 /* Отклонённый блок и поздняя копия не продлевают таймаут. */
 CHECK(device.updateTimeout==5);CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK&&device.updateTimeout==5);
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_BADFRAME&&device.FWblockIdx==2&&flash[0x200+3*1024]==0xff);
 CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK&&device.FWblockIdx==2);
 /* Блок другого канала даёт 253, текущая загрузка продолжается. */
 CHECK(block(2,1026,CH_USB)==FW_BLOCK_END&&device.FWinProgress&&device.FWblockIdx==2&&flash[0x200+2*1024]==0xff);
 /* Обрыв записи даёт 254; повтор того же блока завершает запись. */
 torn=1;CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_BADWRITE&&device.FWblockIdx==2);CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_OK&&device.FWblockIdx==3);
 CHECK(!memcmp(flash+0x200+2*1024,image+2*1024,1024));
 /* Незаписываемый байт дважды даёт 254, затем 253 с причиной ошибки записи. */
 stuck=0x200+3*1024+10;nlogs=0;
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_BADWRITE&&block(3,1026,CH_SERVER)==FW_BLOCK_BADWRITE);
 CHECK(block(3,1026,CH_SERVER)==FW_BLOCK_END&&last_log(LOG_FW_FAIL,"WRITE")&&!device.FWinProgress&&!device.status.readyToUpdate);
 CHECK(block(4,1026,CH_SERVER)==FW_BLOCK_END);
 /* Успешный блок сбрасывает накопленные ошибки записи. */
 download(size,blocks,crc);torn=1;CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK);
 torn=1;CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_OK);
 torn=1;CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_BADWRITE);CHECK(block(2,1026,CH_SERVER)==FW_BLOCK_OK);
 /* Чужая плата в блоке 0 даёт 253 и событие 6 без записи во флеш. */
 download(size,blocks,crc);image[0x20]=0x0B;CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_END);image[0x20]=0x8B;
 CHECK(last_log(LOG_FW_FAIL,"HW")&&!device.FWinProgress&&!device.status.readyToUpdate&&flash[0x200+0x20]==0xff);
 /* Неверная CRC образа после последнего блока даёт 253 без установки. */
 download(size,blocks,crc^1u);for(k=0;k+1<blocks;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
 CHECK(block(blocks-1,1026,CH_SERVER)==FW_BLOCK_END&&last_log(LOG_FW_FAIL,"CRC")&&!device.rebootToUpdate&&!device.status.readyToUpdate);
 /* USB-блоки NextConfig используют те же коды. */
 reset();CHECK(start(71,1,size,crc,blocks,CH_USB)==CMD_SUCCESS);for(k=0;k<blocks;k++)CHECK(block(k,1026,CH_USB)==FW_BLOCK_OK&&!device.updateTimeout);
 CHECK(device.rebootToUpdate&&!saves);
 /* Без начатой загрузки блоки не принимаются. */
 reset();device.FWblockCount=66;fw_resume_after_boot();
 CHECK(block(0,1026,CH_USB)==FW_BLOCK_END&&block(0,1026,CH_SERVER)==FW_BLOCK_END&&flash[0x200]==0xff);
 puts("command 20: codes 0, 253, 254, 255, late copies, other channel, read-back, foreign board, image CRC, USB, no download");}

// Проверяет сценарий end_tests по независимым данным модуля.
static void end_tests(void){
 /* Отмена контроллером даёт 253 на его канале, причину в журнале и сохраняет сброс бита 11. */
 download(66716,66,1);saves=0;sent=0;fw_end("TIMEOUT",7);
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!device.updateTimeout&&!device.FWblockIdx&&!device.FWblockCount&&saves==1);
 CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_END);
 CHECK(sent==1&&sent_code==FW_BLOCK_END&&sent_cch==CH_SERVER&&last_log(LOG_FW_TIMEOUT,"TIMEOUT"));
 download(66716,66,1);device.registredOnServer=0;sent=0;fw_end("TIMEOUT",7);CHECK(!sent&&!device.FWinProgress);
 /* Бит 11 означает режим обновления. */
 device.status.readyToUpdate=0;CHECK(!fw_update_quiet());device.status.readyToUpdate=1;CHECK(fw_update_quiet());
 /* Маркер платы в резервном векторе 8 записан младшим байтом вперёд. */
 {uint8_t b[1024];uint32_t w;memset(b,0xA5,sizeof b);
  w=0x4857008Bu;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,139));CHECK(fw_image_foreign(b,11));CHECK(!fw_image_foreign(b,0));
  w=0;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,11));w=0x4857018Bu;memcpy(b+0x20,&w,4);CHECK(!fw_image_foreign(b,11));}
 /* Команда 8 передаёт источник в событие E184. */
 CHECK(server_out_event==LOG_CON_OUTONBYAPI);
 CHECK(cmd8_event(1)==77&&cmd8_event(2)==75&&cmd8_event(3)==74&&cmd8_event(4)==76&&cmd8_event(0)==75&&cmd8_event(5)==75&&cmd8_event(255)==75);
 puts("controller ends a download, update mode, board marker, command 8 sources");}

// Проверяет сценарий stage по независимым данным модуля.
static void stage(uint16_t blocks,uint16_t version){bootFWparam_s h;reset();memset(&h,0xff,sizeof(h));h.hardwareVersion=139;h.firmwareVersion=version;
 h.firmwareSize=(uint32_t)blocks*1024u-8u;h.firmwareCRC=0x12345678u;h.buildDate=1789454349u;memcpy(flash,&h,sizeof(h));
 device.FWinProgress=1;device.FWblockCount=blocks;device.cchFWupdate=0;saves=0;}
// Проверяет сценарий resume_tests по независимым данным модуля.
static void resume_tests(void){uint32_t seed=12345u,i;uint16_t blocks=37,k,idx;unsigned cuts=0;
 static const uint16_t offs[]={0,1,63,64,255,256,700,1023,1024};
 for(i=0;i<sizeof(image);i++){seed=seed*1103515245u+12345u;image[i]=(uint8_t)(seed>>16);}
 memset(image+5*1024,0xff,1024);memset(image+6*1024,0xff,300);memset(image+36*1024+1016,0xff,8);/* Допустимый блок FF, префикс FF и хвост FF. */
 for(k=0;k<blocks;k++){unsigned o;for(o=0;o<sizeof(offs)/sizeof(offs[0]);o++){uint32_t base=0x200u+(uint32_t)k*1024u;
  stage(blocks,64);device.FWblockIdx=k>3?k-3:0;
  memcpy(flash+0x200,image,(uint32_t)k*1024u);/* Блоки до k целиком записаны. */
  memcpy(flash+base,image+base-0x200,offs[o]);/* Блок k оборван после offs[o] байтов. */
  if(offs[o]<1024)flash[base+offs[o]]&=(uint8_t)(image[base-0x200+offs[o]]|0x5a);/* Один байт сохранил установленные биты. */
  fw_resume_after_boot();idx=device.FWblockIdx;
  CHECK(device.FWinProgress&&device.status.readyToUpdate&&device.updateTimeout==600&&device.cchFWupdate==CH_SERVER&&!saves);
  CHECK(idx<=k);/* Не пропускаем оборванный или отсутствующий блок. */
  for(i=(uint32_t)idx*1024u;i<(uint32_t)blocks*1024u;i++)flash[0x200+i]&=image[i];/* Сервер передаёт блоки от idx до конца. */
  CHECK(!memcmp(flash+0x200,image,(uint32_t)blocks*1024u));cuts++;}}
 /* Если ничего не записано, начинаем с 0; если всё записано, повторяем последний блок. */
 stage(blocks,64);fw_resume_after_boot();CHECK(device.FWblockIdx==0&&device.firmwarePercent==0);
 stage(blocks,64);device.FWblockIdx=blocks-1;memcpy(flash+0x200,image,(uint32_t)blocks*1024u);fw_resume_after_boot();CHECK(device.FWblockIdx==blocks-1);
 /* Без загрузки нет чтений и сохранений; старый таймаут снимается, блоки не принимаются. */
 stage(blocks,64);device.FWinProgress=0;device.updateTimeout=900;fw_resume_after_boot();
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&!saves&&!device.updateTimeout&&!device.FWblockIdx&&!device.FWblockCount);
 /* Сохранённое оставшееся время удерживается; отсутствующее или невозможное даёт 10 минут. */
 stage(blocks,64);device.updateTimeout=150;fw_resume_after_boot();CHECK(device.FWinProgress&&device.updateTimeout==150);
 stage(blocks,64);device.updateTimeout=3000;fw_resume_after_boot();CHECK(device.updateTimeout==600);
 /* Заголовок вне загрузки завершает обновление и сохраняет это состояние. */
 stage(blocks,1);fw_resume_after_boot();CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&saves==1&&!device.FWblockIdx&&!device.FWblockCount);
 stage(blocks,64);{uint32_t magic=0x32544F42u;memcpy(flash,&magic,4);}fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(blocks,64);device.FWblockCount=1;fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(0xffff,64);memset(flash,0xff,sizeof(flash));fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);
 stage(blocks,64);boot_ver=0x21;fw_resume_after_boot();CHECK(!device.FWinProgress&&saves==1);boot_ver=0x32;
 printf("resume: %u power cuts over %u blocks restore the image byte for byte\n",cuts,blocks);}

/* Доработка: Кряжев А., 2026. Проверка реального цикла при медленном сервере. */
static void read_error_tests(void){
 uint32_t crc;unsigned k;
 /* Отказ чтения оставляет в буфере прежний фрагмент CRC; одинаковые фрагменты образа не должны скрыть отказ. */
 reset();memset(flash+FWBLOCK_OFFSET,0xA5,256);crc=stm_crc(flash+FWBLOCK_OFFSET,256);
 read_fail_at=FWBLOCK_OFFSET+128;
 CHECK(!user_checkFirmware(FWBLOCK_OFFSET,256,crc));
 read_fail_at=UINT32_MAX;CHECK(user_checkFirmware(FWBLOCK_OFFSET,256,crc));
 CHECK(!user_checkFirmware(FWBLOCK_OFFSET,0,UINT32_MAX));
 CHECK(!user_checkFirmware(FWBLOCK_OFFSET,130,stm_crc(flash+FWBLOCK_OFFSET,128)));
 /* SPI может передать ожидаемые байты и одновременно сообщить отказ периферии. */
 make_image(2048);crc=stm_crc(image,2048);download(2048,2,crc);
 read_fail_at=FWBLOCK_OFFSET;read_fail_copy=1;
 CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_BADWRITE&&device.FWblockIdx==0);
 read_fail_at=UINT32_MAX;CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK);
 read_fail_at=0;
 CHECK(block(1,1026,CH_SERVER)==FW_BLOCK_END&&!device.rebootToUpdate&&!device.FWinProgress);
 CHECK(last_log(LOG_FW_FAIL,"READ"));
 download(2048,2,crc);read_fail_at=0;read_fail_copy=1;
 CHECK(start(71,1789600000,2048,crc,2,CH_SERVER)==CMD_RUNERROR&&device.FWinProgress);
 stage(2,71);read_fail_at=0;read_fail_copy=1;fw_resume_after_boot();
 CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&saves==1);
 /* Повреждённый сохранённый курсор не пропускает загрузку. */
 for(k=2;k<5;++k){stage(2,71);device.FWblockIdx=k;
  fw_resume_after_boot();CHECK(!device.FWinProgress&&!device.status.readyToUpdate&&saves==1);
 }
 puts("firmware read faults: CRC, read-back, final header, repeated start and resume fail closed");
}

static uint32_t audit_ms;
static unsigned audit_requests,audit_busy;
static uint8_t uniqIDCnt;
// Проверяет сценарий timers_getMillis по независимым данным модуля.
static uint32_t timers_getMillis(void){return audit_ms;}
// Проверяет сценарий user_getFWblockFromServer по независимым данным модуля.
static bool user_getFWblockFromServer(dataChanel_e ch,uint16_t idx){
 CHECK(ch==CH_SERVER&&idx==device.FWblockIdx);
 if(audit_busy){audit_busy--;return false;}audit_requests++;return true;
}
/* ПРОВЕРКА ЦИКЛА */
static void audit_seconds(unsigned n){while(n--){audit_ms+=1000;audit_poll();audit_tick();}}
// Проверяет сценарий slow_server_tests по независимым данным модуля.
static void slow_server_tests(void){
 uint16_t k;uint32_t size=68784,crc;unsigned requests;
 /* Занятый слот не запускает двадцатисекундное ожидание. */
 reset();make_image(size);crc=stm_crc(image,size);audit_ms=1000;audit_requests=0;
 CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 for(k=0;k<68;k++){
  audit_busy=3;requests=audit_requests;
  while(audit_busy){audit_ms+=50;audit_poll();CHECK(!device.startWaitFWtime&&audit_requests==requests);}
  audit_ms+=50;audit_poll();CHECK(audit_requests==requests+1);
  CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
 }
 CHECK(device.rebootToUpdate&&audit_ms<600000u);
 /* Старый образ за сохранённым курсором не является частью новой загрузки. */
 reset();memset(flash,0x55,mem[MB_FIRMWARE].size);
 CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK);
 device.FWblockIdx=0;fw_resume_after_boot();CHECK(device.FWblockIdx==0);
 for(k=0;k<68;k++)CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
 CHECK(device.rebootToUpdate&&!memcmp(flash+FWBLOCK_OFFSET,image,size));
 /* Темп сервера 15 с на блок: 600 секунд кончаются на середине, загрузка прекращается. */
 reset();make_image(size);crc=stm_crc(image,size);audit_ms=1000;audit_requests=0;
 CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 for(k=0;k<68&&device.FWinProgress;k++){
  audit_seconds(15);
  if(!device.FWinProgress) break;
  CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);
 }
 CHECK(!device.FWinProgress&&!device.rebootToUpdate&&!device.status.readyToUpdate);
 /* fw_abort() обнуляет счётчик блоков, поэтому считаем принятые сами. */
 CHECK(sent_code==FW_BLOCK_END&&last_log(LOG_FW_TIMEOUT,"TIMEOUT"));
 printf("slow server: %u of 68 blocks in 600 s at 15 s each\n",k);CHECK(k==39);
 /* Тот же образ при быстрой отдаче укладывается в срок с большим запасом. */
 reset();audit_ms=1000;CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 for(k=0;k<68;k++){audit_seconds(1);CHECK(block(k,1026,CH_SERVER)==FW_BLOCK_OK);}
 CHECK(device.rebootToUpdate&&!device.FWinProgress&&!device.updateTimeout);
 CHECK(!memcmp(flash+FWBLOCK_OFFSET,image,size));
 /* Пауза связи: блоки не запрашиваются, но срок идёт - он один на всю загрузку. */
 reset();audit_ms=1000;audit_requests=0;
 CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 audit_seconds(5);CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK);
 device.registredOnServer=0;requests=audit_requests;audit_seconds(120);
 CHECK(audit_requests==requests&&device.FWblockIdx==1&&device.cchFWupdate==CH_SERVER);
 CHECK(device.updateTimeout==600-125);
 /* Запоздавший дубль ничего не двигает и лишнего запроса не рождает. */
 device.registredOnServer=1;audit_seconds(1);requests=audit_requests;
 CHECK(block(0,1026,CH_SERVER)==FW_BLOCK_OK&&device.FWblockIdx==1&&device.updateTimeout==600-126);
 audit_poll();CHECK(audit_requests==requests);
 /* Остаток срока пишется раз в минуту и переживает сброс. */
 reset();audit_ms=1000;CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 audit_seconds(300);CHECK(saved_timeout==300&&saves==6);
 device.updateTimeout=saved_timeout;fw_resume_after_boot();CHECK(device.updateTimeout==300);
 /* Срок молчания выходит целиком и заканчивает загрузку кодом 253. */
 reset();audit_ms=1000;CHECK(start(71,1789922853,size,crc,68,CH_SERVER)==CMD_SUCCESS);
 audit_seconds(599);CHECK(device.FWinProgress&&device.updateTimeout==1&&saves==10);
 audit_seconds(1);CHECK(!device.FWinProgress&&!device.updateTimeout&&sent_code==FW_BLOCK_END&&sent==1);
 puts("audit: one 600 s limit for the whole download - a 15 s/block server does not fit, a fast one does");
}

// Запускает проверки модуля и сохраняет результаты в заданный файл.
int main(void){unsigned failure;for(failure=1;failure<=2;failure++){reset();fail_flash=failure;
 CHECK(start(71,1789600000,1024,1,1,CH_SERVER)==CMD_RUNERROR);CHECK(!device.FWinProgress&&!device.rebootToUpdate&&!saves);}
 read_error_tests();slow_server_tests();start_tests();rollback_tests();block_tests();end_tests();resume_tests();
 puts("PASS firmware update of sheet Protokol2: commands 19, 20 and 4, rollback, end of a download, resume after a reset");return 0;}
