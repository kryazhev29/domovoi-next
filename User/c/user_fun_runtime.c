// Доработка: Кряжев А., 2026
#include "fast_kernels.h"
#include "runtime_internal.h"
#include "prof.h"
#include "../user_format.h"
#include "../hybrid/string_select.h"
#include "stm32_it.h"
// Пользовательские функции

#include "user_fun.h"
#include "../../Modules/comm_channel.h"
#include "../../Modules/comm_server.h"
#include "photo.h"
#include "user_comm.h"
#include "../../Modules/comm_recover.h"
#include "../../Modules/comm_info.h"
#include "../../Modules/comm_uart.h"
#include "../../Modules/comm_modem.h"
#include "../../Modules/comm_call.h"
#include "idindex.h"
#include "usb_istr.h"
#include "../../Bootloader/app_update.h"
#include "../../Bootloader/boot_manual.h"

// ************** Включить режим отладки **********
#ifndef RELEASE_BUILD
#define DEBUG_USER
#endif

#ifdef HYBRID_CARD
uint32_t id_time_end(cellID_s *c);
void id_watch_end(uint32_t end);
#endif

#define UNIPASS_INITVALUE       32768
#define CRYPT_START             (uint8_t)0x5A
#define TIMETOREGRESPONSE    (uint8_t)5 // время на получение ответа от сервера
#define TIMETOSTATUSRESPONSE    (uint8_t)15 
#define STATUSTIMERINTERVAL     (uint8_t)15 // интервал отправки статуса
#define MAXSTATUSANSWERERROR    (uint8_t)3 // пропущенных подряд ответов на статус до переподключения (лист «Проверка связи контроллером»: один пропуск - ничего не делать)
#define PROTOCOL_VERSION        (uint8_t)1 // версия протокола в регистрации (Протокол2 E424)
/* Ответ сервера на регистрацию (Протокол2, строка 135): 4 - такого IMEI нет в системе,
   5 - не привязан ни к одному объекту; в обоих случаях контроллер делает заводской сброс. */
#define REG_UNKNOWN_IMEI        4
#define REG_NOT_BOUND           5

// команда 7 запрос фото
// если getPhoto не равно нулю то сделать фото
// сервер обновления и передачи фото
// алгоритм переключения на второй сервер

// таблица адресов областей памяти контроллера
#pragma data_alignment=4
#ifdef HYBRID_CARD
extern cellMem_s mem[10];
#else
cellMem_s mem[10];
#endif
#ifndef HYBRID_CARD
static bool log_readRecords(uint16_t first, uint8_t *data, uint16_t count);
#endif

// конфигурация
#pragma data_alignment=4
#ifdef HYBRID_CARD
extern config_s config;
#else
config_s config;
#endif

// оперативные переменные
#pragma data_alignment=4
#ifdef HYBRID_CARD
extern device_s device;
#else
device_s device;
#endif

/* >>> Cooperative storage callers */
#ifdef ASM_RUNTIME
#include "sector_runtime.h"
#include "photo_runtime.h"
#ifndef HYBRID_CARD
static bool table_runtime_active(void);
#endif

#ifndef HYBRID_CARD
static void table_runtime_poll(uint32_t now);
#endif
#ifndef HYBRID_CARD
static void table_runtime_cancel(void);
#endif
#ifndef HYBRID_CARD
static void table_runtime_quiet(dataChanel_e channel);
#endif
void idindex_runtime_suspend(void);
#else
#define photo_storage_busy() 0
#define photo_storage_cancel() ((void)0)
#define photo_storage_poll(now) ((void)0)
#define table_runtime_active() 0
#define table_runtime_blocks_reads() 0
#define table_runtime_poll(now) ((void)0)
#define table_runtime_cancel() ((void)0)
#define table_runtime_quiet(channel) ((void)0)
#endif
#ifndef HYBRID_CARD
static bool fw_async_active(void);
#endif
#ifndef HYBRID_CARD
static bool fw_async_begin(const uint8_t *data, uint32_t address, dataChanel_e channel);
#endif
#ifndef HYBRID_CARD
static void fw_async_reply(uint8_t id);
#endif
#ifndef HYBRID_CARD
static void fw_async_cancel(void);
#endif
#ifndef HYBRID_CARD
static void fw_async_quiet(dataChanel_e channel);
#endif
#ifndef HYBRID_CARD
static void fw_storage_poll(uint32_t now);
#endif
#ifndef HYBRID_CARD
static bool log_async_active(void);
#endif
#ifndef HYBRID_CARD
static void log_storage_poll(uint32_t now);
#endif
#ifdef HYBRID_CARD
extern uint8_t store_runtime, store_save_requested, store_reset_requested;
#else
uint8_t store_runtime, store_save_requested, store_reset_requested;
#endif
#ifdef HYBRID_CARD
extern struct {
  uint32_t erase, end, retry_at, usb_epoch;
  uint16_t cursor;
  uint8_t job, clear, waiting, cmd, channel, uniq, quiet, repeat;
  uint8_t last_id[4], seen;
} store;
#else
struct {
  uint32_t erase, end, retry_at, usb_epoch;
  uint16_t cursor;
  uint8_t job, clear, waiting, cmd, channel, uniq, quiet, repeat;
  uint8_t last_id[4], seen;
} store;
#endif
enum { US_SAVE=1, US_ACK, US_MARK, US_ERASE, US_CLEAN, US_CONFIG, US_RESET, US_REPLY, US_COMMAND, US_RECOVER, US_RECOVER_CLEAN };
#ifndef HYBRID_CARD
static bool store_ack(uint8_t cmd, dataChanel_e cch);
#endif
#ifndef HYBRID_CARD
static void store_usb_session(void);
#endif
#ifndef HYBRID_CARD
static bool store_finish(uint8_t cmd);
#endif
bool store_upload_request(uint8_t cmd, dataChanel_e cch, uint8_t uniq);
#ifndef HYBRID_CARD
static bool store_config_start(void);
#endif
#ifndef HYBRID_CARD
static void store_command_start(uint8_t cmd, dataChanel_e cch, uint8_t uniq);
#endif
#ifndef HYBRID_CARD
static void store_restore(const device_s *saved);
#endif
void user_storage_poll(void);
#ifndef HYBRID_CARD
static bool storage_idle(void) { return !flash_owner && !w25_async_active() && w25_read_available(); }
#endif
#ifndef HYBRID_CARD
bool user_storage_command_ready(void) {
store_usb_session(); return !store.job && !store_reset_requested && !mem_config_result() && storage_idle();
}
#endif
#ifndef HYBRID_CARD
static bool store_command_allowed(uint8_t cmd) { return cmd == 24 || cmd == 27 || user_storage_command_ready(); }
#endif
#ifndef HYBRID_CARD
bool user_storage_flash_ready(void) {
return store.clear!=9 && storage_idle();
}
#endif
/* <<< Cooperative storage callers */

/* >>> подтверждение выгрузки */
#ifdef HYBRID_CARD
extern uint16_t log_usbReadIndex, temp_usbReadIndex;
#else
uint16_t log_usbReadIndex, temp_usbReadIndex;
#endif
#ifdef HYBRID_CARD
extern uint16_t log_unacked[2], temp_unacked[2];
#else
uint16_t log_unacked[2], temp_unacked[2];
#endif

#ifndef HYBRID_CARD
uint16_t upload_index(uint8_t cmd, dataChanel_e cch){
  if (cmd == 10) return cch == CH_USB ? log_usbReadIndex : device.readLogIDIndex;
  return cch == CH_USB ? temp_usbReadIndex : device.readTempIDIndex;
}
#endif
#ifndef HYBRID_CARD
static void upload_setIndex(uint8_t cmd, dataChanel_e cch, uint16_t value){
  if (cmd == 10) {
    if (cch == CH_USB) log_usbReadIndex = value; else device.readLogIDIndex = value;
  } else {
    if (cch == CH_USB) temp_usbReadIndex = value; else device.readTempIDIndex = value;
  }
}
#endif
// Только подтверждённые позиции, в ранее неиспользуемом резерве device.
#ifndef HYBRID_CARD
static void upload_store(void){
  uint32_t mark = 0x314C5055u;
  memcpy(device.reserv, &mark, 4);
  memcpy(device.reserv + 4, &log_usbReadIndex, 2);
  memcpy(device.reserv + 6, &temp_usbReadIndex, 2);
}
#endif
#ifndef HYBRID_CARD
static void upload_restore(const device_s *saved){
  uint32_t mark;
  memcpy(&mark, saved->reserv, 4);
  if (mark != 0x314C5055u) return; // старые версии подтверждения не сохраняли
  device.readLogIDIndex = saved->readLogIDIndex;
  device.readTempIDIndex = saved->readTempIDIndex;
  memcpy(&log_usbReadIndex, saved->reserv + 4, 2);
  memcpy(&temp_usbReadIndex, saved->reserv + 6, 2);
  upload_store();
}
#endif
#ifndef HYBRID_CARD
static void upload_reset(uint8_t cmd){
  upload_setIndex(cmd, CH_USB, 0);
  upload_setIndex(cmd, CH_SERVER, 0);
  uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;
  pending[0] = pending[1] = 0;
  upload_store();
}
#endif
#ifndef HYBRID_CARD
bool upload_ack(uint8_t cmd, dataChanel_e cch){
  if (store_runtime) return store_ack(cmd, cch);
  uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;
  uint8_t channel = cch == CH_USB ? 0 : 1;
  if (!pending[channel]) return true;
  uint16_t old = upload_index(cmd, cch);
  upload_setIndex(cmd, cch, pending[channel]);
  upload_store();
  if (!user_writeConfigAndDevice()) {
    upload_setIndex(cmd, cch, old);
    upload_store();
    return false;
  }
  pending[channel] = 0;
  return true;
}
#endif
#ifndef HYBRID_CARD
bool upload_finish(uint8_t cmd){
  if (store_runtime) return store_finish(cmd);
  uint16_t server = upload_index(cmd, CH_SERVER), usb = upload_index(cmd, CH_USB);
  bool active = cmd == 10 ? device.status.readyLogData || device.logsCount != 0 :
                           device.status.readyID || device.tempIDsCount != 0;
  if (server || usb || active) {
    // Нулевая позиция должна стать долговечной ДО стирания.
    // Сброс посреди стирания допускает повтор, но не пропуск новых записей.
    upload_reset(cmd);
    if (!user_writeConfigAndDevice()) {
      upload_setIndex(cmd, CH_SERVER, server);
      upload_setIndex(cmd, CH_USB, usb);
      upload_store();
      return false;
    }
    if (active && !(cmd == 10 ? log_clearLogBuffer() : user_clearTempIdBuffer())) return false;
  }
  if (cmd == 10) device.status.readyLogData = 0; else device.status.readyID = 0;
  return true;
}
#endif
/* <<< подтверждение выгрузки */



#ifndef HYBRID_CARD
static void fw_resume_after_boot(void);
#endif
#ifndef HYBRID_CARD
static void fw_rollback_poll(void);
#endif
#ifndef HYBRID_CARD
static void fw_status_poll(void);
#endif
#ifndef HYBRID_CARD
static void unreg_sms_poll(void);
#endif
#ifndef HYBRID_CARD
static void server_rx_reset(void);
#endif
/* Диагностика стенда, команда 111, только по USB, только чтение. */
#define USB_DIAG_COMMAND 111u
/* Диагностика связи, команда 112 (предложение для Протокол2), только по USB, только чтение. */
#define USB_COMM_DIAG_COMMAND 112u
extern unsigned int loop_max_ms, loop_stalls;
#ifdef HYBRID_CARD
extern uint32_t stack_free_bytes(void);
#else
uint32_t stack_free_bytes(void);
#endif // main.c: нетронутый снизу шаблон стека, байт
#ifndef HYBRID_CARD
static uint32_t diag_gsm_pwron = 0, diag_gsm_notanswer = 0, diag_heap_fail = 0;
#endif
#ifndef HYBRID_CARD
static void user_send_usbDiag(uint8_t uniqId);
#endif
#ifndef HYBRID_CARD
static void user_send_commDiag(uint8_t uniqId);
#endif
#ifndef HYBRID_CARD
static void reader_reply_poll(void);
#endif
/* Порядок обновления по листу "Протокол2": статус 1 вне очереди (E95), режим обновления - бит
 * 11 (E440), команда 19 (E122), блоки 13 и 20 (E96, E123), команда 4 (E476, E477). Объявлено до
 * команды 19. */
#define FW_TIMEOUT_S            600u    /* вся загрузка, от команды 19 */
#define FW_BLOCK_RETRY_MS       20000u  /* время актуальности команды 13 */
#define FW_WRITE_TRIES          3u      /* один блок не прошёл проверку после записи */
#define FW_BLOCK_PENDING        252u /* internal only: never sent on the wire */
#define FW_BLOCK_OK             0u      /* коды команды 4 */
#define FW_BLOCK_END            253u
#define FW_BLOCK_BADWRITE       254u
#define FW_BLOCK_BADFRAME       255u
#ifndef HYBRID_CARD
static void status_event_sent(uint16_t status, uint32_t now_ms);
#endif
#ifndef HYBRID_CARD
static uint8_t status_event_due(uint16_t status, uint32_t now_ms);
#endif
#ifndef HYBRID_CARD
static uint8_t fw_update_quiet(void);
#endif
void fw_end(const char *reason, uint8_t uniqId);
void fw_log(logUpdate_e num, const char *reason);

// буфер под прием данных от внешнего порта
#pragma data_alignment=4
#ifdef HYBRID_CARD
extern uint8_t port_buffer[PORTBUFSIZE];
#else
uint8_t port_buffer[PORTBUFSIZE];
#endif

#ifndef HYBRID_CARD
union{
  char tempStrBuffer[128]; // временный строковый буфер
  uint32_t bf32[32];
  uint8_t bf[128];
};
#else
extern char _A_tempStrBuffer[128];
#define tempStrBuffer _A_tempStrBuffer
#endif

#ifdef HYBRID_CARD
extern uint8_t uniqIDCnt;
#else
uint8_t uniqIDCnt = 0;
#endif

// >>> переопределение функций --------------------------
#ifndef HYBRID_CARD
void gsm_user_debug (char* ptr)
{
  if (device.logBits & 1)
    usb_sendstr(ptr);
}
#endif

#ifndef HYBRID_CARD
void user_debug(char* ptr)
{
#ifdef DEBUG_USER
  if (device.logBits & 4)
    usb_sendstr(ptr);
#else
  (void)ptr;
#endif
}
#endif

#ifndef HYBRID_CARD
void timers_user_handle(void)
{
  usb_timer1ms();
  uart_timer1ms();
}
#endif

// обработка данных от usb
#ifndef HYBRID_CARD
void usb_data_reading(uint8_t* ptr, uint16_t len)
{
  uint16_t body;
  if (len < 11 || memcmp(ptr, "SLDN", 4)) return;
  body = (uint16_t)ptr[4] | ((uint16_t)ptr[5] << 8);
  if (body < 3 || body != len - 8) return;
  /* Сборщик USB передаёт ровно один целый кадр с проверенной CRC. */
  user_parse_comand(ptr + 4, CH_USB);
}
#endif

// обработчик событий
/* ID со считывателя: ждал ли его сервер (команда 6: источник 1 Wiegand, 2 RF433, 3 RFID
   с адресом) - событие 78 и пакет 8; затем обычная проверка доступа. */
static void reader_id(char *tb, uint8_t source, uint8_t address, codeType_e type)
{
  if (device.sourceGetID == source && (source != 3 || device.addressGetID == address)){
    device.timeToGetID = 0;
    device.sourceGetID = 0;
    log_addControlEvent(LOG_CON_RECVIDONREADER, tb, 0);
    user_send_readedIDtoServer((uint8_t*)tb, 1);
  }
  user_checkIdAndControl(tb, type);
}

// получен идентификатор по wiegand
void WG_readedID(uint32_t id){
  char tb[11];

  format_id(tb, NULL, id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U Wiegand ID: %s\r\n", tb);
  user_debug(tempStrBuffer);
#endif
  reader_id(tb, 1, 0, CT_WIEGAND);
}

// получен идентификатор по rf433
void RF433_readedID(uint32_t id){
  char tb[11];
  //777
  format_id(tb, NULL, id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U RF433 ID: %s\r\n", tb);
  user_debug(tempStrBuffer); 
#endif
  
  reader_id(tb, 2, 0, CT_RF433);
}

// получен идентификатор по RFID
void RFID_readedID(uint32_t id, uint8_t address){
  char tb[11];
  //777
  format_id(tb, NULL, id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U RFID ID: %s\r\n", tb);
  user_debug(tempStrBuffer); 
#endif
  
  reader_id(tb, 3, address, CT_RFID);
}
// <<< переопределение функций --------------------------

// >>> вспомогательные ---------------------------------------------------------
// валиден ли IMEI
#ifndef HYBRID_CARD
Int8U user_imeivalid(void)
{
  if (chkdigitstr(device.IMEIGSM, 15)) 
    return 1;
  return 0;
}
#endif

// универсальный пароль на конфигурацию
#ifndef HYBRID_CARD
Int16U user_unipassword (void)
{
  Int16U p = UNIPASS_INITVALUE;
  Int16U a,b,c,d;
  char i,br[4];

  if (user_imeivalid() == 0) return p; // не валидный

  a = atoi((char*)&device.IMEIGSM[11]); // последние 4-ре цифры

  for (i=0; i<3; i++)
    br[i] = device.IMEIGSM[8+i];
  br[i] = 0;
  b = atoi(br); // 3-ри цифры с 8-й

  for (i=0; i<3; i++)
    br[i] = device.IMEIGSM[i];
  br[i] = 0;
  c = atoi(br); // 3-ри первые цифры

  for (i=0; i<3; i++)
    br[i] = device.IMEIGSM[3+i];
  br[i] = 0;
  d = atoi(br); // 3-ри цифры c 3-й

  p = UNIPASS_INITVALUE + (2*a) + c - (3*b) - d;

  return p;
}
#endif

// ожидание пока напряжение питания не станет выше порога
#ifndef HYBRID_CARD
void user_wait_stable_power(void)
{

}
#endif

// Преобразует время в целочисленное
#ifndef HYBRID_CARD
uint32_t user_timeToInt(uint16_t year, uint8_t mon, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
  uint32_t timedate, temp;

  timedate = (uint32_t)3600*hour;
  timedate += (uint32_t)60*min;
  timedate += sec;
  // считаем дату с учетом кол-ва дней в месяце и весокосных дней
  timedate += (uint32_t)86400*(day-1);
  timedate += (uint32_t)2678400*(mon-1);
  switch (mon) // отнимаем лишние дни
   {
   case 3:
   case 4: timedate -= 259200; break; // 3 дня
   case 5:
   case 6: timedate -= 345600; break; // 4 дня
   case 7:
   case 8:
   case 9: timedate -= 432000; break; // 5 дней
   case 10:
   case 11: timedate -= 518400; break; // 6 дней
   case 12: timedate -= 604800; break; // 7 дней
   }
  temp = year - 1970;
  timedate += ((uint32_t)31536000*temp) + ((uint32_t)86400*div((temp+2),4).quot); // добавляем високосные дни
     
  if (div((temp+2),4).rem == 0) // текущий год високосный
  {
    if (mon < 3) timedate -= 86400; // январь, февраль - отнимаем один день
  }
  
  return timedate;
}
#endif
// <<< вспомогательные ---------------------------------------------------------

// ошибка подключения
#ifndef HYBRID_CARD
void user_errorconnection (int8_t handle){
#ifdef DEBUG_USER
  user_debug(">>U Error connection ...\r\n"); 
#endif
  device.registredOnServer = 0;
  device.answerTimeout = 0;
  
  log_addSystemEvent(LOG_SYS_ERRCONNECTION);
}
#endif

/* TCP не сохраняет границы пакетов. Буфер хранит один кадр до проверки длины и CRC.
 * Неполный кадр живёт не дольше 30 с; новое соединение всегда начинается с чистого буфера. */
#define SERVER_RX_CAPACITY 2048u
#define SERVER_RX_TIMEOUT_MS 30000u
#define SERVER_RX_WORK 256u
#ifdef HYBRID_CARD
extern uint8_t server_rx_buffer[SERVER_RX_CAPACITY];
#else
uint8_t server_rx_buffer[SERVER_RX_CAPACITY];
#endif
#ifdef HYBRID_CARD
extern uint16_t server_rx_count;
#else
uint16_t server_rx_count;
#endif
#ifdef HYBRID_CARD
extern uint32_t server_rx_last;
#else
uint32_t server_rx_last;
#endif
#ifdef HYBRID_CARD
extern uint8_t server_rx_defer, server_rx_busy, server_rx_provisional;
#else
uint8_t server_rx_defer, server_rx_busy, server_rx_provisional;
#endif
#ifdef HYBRID_CARD
extern uint16_t server_rx_ready;
#else
uint16_t server_rx_ready;
#endif
/* Only begin_pass replenishes these, never receive/process/reset. Search and
 * CRC share credits, including repeated byte callbacks and nested reception.
 * At most one scan compaction and one command/consume per service pass. */
#ifdef HYBRID_CARD
extern uint16_t server_rx_work;
#else
uint16_t server_rx_work;
#endif
#ifdef HYBRID_CARD
extern uint8_t server_rx_compacted, server_rx_commands, server_rx_cancelled;
#else
uint8_t server_rx_compacted, server_rx_commands, server_rx_cancelled;
#endif
#ifdef HYBRID_CARD
extern uint16_t server_rx_offset, server_rx_total, server_rx_pos, server_rx_crc;
#else
uint16_t server_rx_offset, server_rx_total, server_rx_pos, server_rx_crc;
#endif
void server_rx_process(void);

#ifndef HYBRID_CARD
static void server_rx_reset(void)
{
  if (server_rx_busy) server_rx_cancelled = 1;
  else server_rx_count = 0;
  server_rx_ready = server_rx_offset = server_rx_total = server_rx_pos = 0;
  server_rx_provisional = 0;
}
#endif

#ifndef HYBRID_CARD
void server_rx_begin_pass(void)
{
  if (server_rx_busy) return;
  server_rx_work = SERVER_RX_WORK;
  server_rx_compacted = 0;
  server_rx_commands = 1;
  if (server_rx_count && (uint32_t)(timers_getMillis() - server_rx_last) >= SERVER_RX_TIMEOUT_MS)
    server_rx_reset();
}
#endif

#ifndef HYBRID_CARD
static void server_rx_consume(uint16_t n)
{
  server_rx_count -= n;
  memmove(server_rx_buffer, server_rx_buffer + n, server_rx_count);
}
#endif

#ifndef HYBRID_CARD
static void server_rx_scan(void)
{
  uint16_t left, n, found, body;
  uint8_t *p;
  if (server_rx_ready || server_rx_busy || !server_rx_work) return;
  while (server_rx_offset < server_rx_count && server_rx_work >= 8u) {
    p = server_rx_buffer + server_rx_offset;
    left = server_rx_count - server_rx_offset;
    if (!server_rx_total) {
      /* Bound the kernel itself; its partial suffix is retried next pass. */
      n = left > 64u ? 64u : left;
      if (n > server_rx_work - 8u) n = server_rx_work - 8u;
      if (!n) break;
      found = (uint16_t)fast_sldn(p, n);
      server_rx_work -= found + 4u < n ? found + 4u : n;
      server_rx_offset += found;
      if (found == n) continue;
      if (n - found < 4u && left > n) continue;
      p += found; left -= found;
      server_rx_work -= 8u; /* header/length and candidate transition */
      if (left < 6u) break;
      body = (uint16_t)p[4] | ((uint16_t)p[5] << 8);
      if (body < 3u || body > SERVER_RX_CAPACITY - 8u) {
        ++server_rx_offset; continue;
      }
      if (left < body + 8u) break;
      server_rx_total = body + 8u;
      server_rx_pos = 4u;
      server_rx_crc = 0xffffu;
    }
    n = server_rx_total - 2u - server_rx_pos;
    if (n > server_rx_work) n = server_rx_work;
    if (n) {
      server_rx_crc = crc16_calcInit(p + server_rx_pos, n, server_rx_crc);
      server_rx_pos += n; server_rx_work -= n;
    }
    if (server_rx_pos != server_rx_total - 2u) break;
    if (server_rx_crc == ((uint16_t)p[server_rx_total - 2u] |
                         ((uint16_t)p[server_rx_total - 1u] << 8))) {
      /* A verified frame may wait for next pass's compaction quota. */
      if (server_rx_offset && server_rx_compacted) break;
      server_rx_ready = server_rx_total;
      server_rx_total = server_rx_pos = 0;
      break;
    }
    server_rx_total = server_rx_pos = 0;
    ++server_rx_offset;
  }
  if (server_rx_offset && !server_rx_compacted) {
    server_rx_consume(server_rx_offset);
    server_rx_offset = 0;
    server_rx_compacted = 1;
  }
}
#endif

#ifndef HYBRID_CARD
void user_dataconnection(int8_t handle, uint8_t *data, uint16_t siz)
{
  uint32_t now = timers_getMillis();
  uint16_t n;
  if (handle != device.connectionhandle || server_rx_cancelled) return;
  if (!server_rx_busy && server_rx_count && (uint32_t)(now - server_rx_last) >= SERVER_RX_TIMEOUT_MS)
    server_rx_reset();
  if (siz) server_rx_last = now;
  while (siz) {
    if (server_rx_count == SERVER_RX_CAPACITY) {
      uint8_t defer = server_rx_defer;
      server_rx_process(); server_rx_defer = defer;
    }
    n = SERVER_RX_CAPACITY - server_rx_count;
    if (!n) return; /* QIRD uses capacity/backpressure; legacy drops excess. */
    if (n > siz) n = siz;
    memcpy(server_rx_buffer + server_rx_count, data, n);
    server_rx_count += n; data += n; siz -= n;
    server_rx_scan();
    if (!server_rx_defer) server_rx_process();
  }
}
#endif

#ifndef HYBRID_CARD
void server_rx_process(void)
{
  uint16_t total;
  if (server_rx_provisional || server_rx_busy) return;
  server_rx_defer = 0;
  server_rx_scan(); /* Also resume when the input stream has stopped. */
  if (!server_rx_ready || !server_rx_commands || comm_tx_busy()) return;
  if (!store_command_allowed(server_rx_buffer[8])) return;
  --server_rx_commands;
  total = server_rx_ready; server_rx_ready = 0;
  server_rx_busy = 1;
  user_parse_comand(server_rx_buffer + 4, CH_SERVER);
  server_rx_busy = 0;
  if (server_rx_cancelled) {
    server_rx_cancelled = 0; server_rx_reset(); return;
  }
  server_rx_consume(total);
  server_rx_scan();
}
#endif

/* QIRD копирует прямо в прежний сборщик. До финала команды выполнение отложено. */
#ifndef HYBRID_CARD
static bool server_rx_verified(const uint8_t *p)
{ return server_rx_busy && p == server_rx_buffer + 4; }
#endif

#ifndef HYBRID_CARD
uint16_t user_comm_rx_capacity(void)
{ return server_rx_cancelled ? 0 : (uint16_t)(sizeof(server_rx_buffer) - server_rx_count); }
#endif
#ifndef HYBRID_CARD
uint16_t user_comm_rx(const uint8_t *data, uint16_t count)
{
  uint16_t capacity = user_comm_rx_capacity();
  if (count > capacity) count = capacity;
  server_rx_defer = server_rx_provisional = 1;
  user_dataconnection(device.connectionhandle, (uint8_t *)data, count);
  return count;
}
#endif
#ifndef HYBRID_CARD
void user_comm_rx_done(uint16_t count, uint8_t reason)
{
  (void)count; server_rx_provisional = 0;
  if (reason) server_rx_reset();
}
#endif
#ifndef HYBRID_CARD
void user_comm_session(bool open)
{
  server_rx_reset();
  fw_service_quiet(CH_SERVER); upload_runtime_quiet(CH_SERVER);
  fw_async_quiet(CH_SERVER);
  table_runtime_quiet(CH_SERVER); service_crc_quiet(CH_SERVER);
  device.connectionhandle = 0; device.registredOnServer = 0; device.answerTimeout = 0;
  /* Поколение TCP отменяет неподтверждённый пакет; сохранённые курсоры и USB не трогаем. */
  log_unacked[1] = temp_unacked[1] = 0;
  if (store.job && store.channel == CH_SERVER) store.quiet = 1;
  log_unacked[1] = temp_unacked[1] = 0; store.seen &= 5u;
  if (!open) photo_connection_lost(); /* «подключён» пишет user_comm_link после регистрации */
}
#endif

// принято СМС
// формат команды [Символ – #][Команда][Пробел][ПИН код контроллера][Запятая][Аргументы команды]
#ifndef HYBRID_CARD
bool user_smsreceived (char* num, char* data)
{
  /* SIM хранит SMS, пока единственный слот передачи занят. Событие пишем лишь при приёме. */
  if (device.triggers.smsToAdmin && *data != '#' && !user_send_smsToServer(data)) return false;
  log_addSystemEventID(LOG_SYS_RECVSMS, user_last10digitsnumber(num));
  
  //777 в какой момент utc конвертить в текст?
  /*if ((dt[0] == '0') && ((dt[1] == '0') || (dt[1] == '4'))){ // ucs2 code
    if ((dtEnd != dt) && ((dtEnd - dt) <= 512)){
      decodeUCS2toText(tempStrBuffer, dt, (uint16_t)(dtEnd - dt - 1));
      dt = tempStrBuffer;
    }
  }*/
  
#ifdef DEBUG_USER
  // Оставляем место для префикса, перевода строки и завершающего нуля.
  sprintf(tempStrBuffer, ">>U SMS recv: %.*s\r\n",
          (int)(sizeof(tempStrBuffer) - sizeof(">>U SMS recv: \r\n")), data);
  user_debug(tempStrBuffer); 
#endif 
  
  char *dt, *dtEnd;
  
  dt = data;
  dtEnd = skipchar(0x0a, dt, 1); // перевод каретки (конец строки)
  
  if (dtEnd != dt){ // перевод каретки заменяем на ноль
    *(dtEnd - 1) = 0x00;
  }
  
  if (*dt == '#'){ // решетка перед командой в СМС обязательна
    bool validPin = false;
    bool validFormat = true;
    char *v, *arg;
    
    // ПИН код
    v = skipchar(' ', dt, 1);
    if (v != dt){ // есть пробел
      user_strcpy(tempStrBuffer, v, ',', 4);
      if (config.pinCode == *(uint32_t*)tempStrBuffer){
        validPin = true;
      }
    } else{ // не верный формат
      validFormat = false;
    }

    // аргумент
    arg = skipchar(',', dt, 1);
    if (arg == dt){
      validFormat = false;
    }
    
    dt++;
    v = &tempStrBuffer[0];
    // команда
    user_strcpy(v, dt, ' ', 10);
    /* Лист «SMS»: команда строчными, ответ начинается с неё заглавными («ADD: ...»). */
    char cmd[11];
    strcpy(cmd, v);
    for (char *c = v; *c; ++c) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');

    if (validPin == true){
      /* Режим обновления (таблица E437): управляющие команды ждут окончания обновления. */
      if (fw_update_quiet() && (strcmp(cmd, "add") == 0 || strcmp(cmd, "reset") == 0 ||
                                strcmp(cmd, "setout") == 0 || strcmp(cmd, "default") == 0)){
        strcat(v, ": Error \"Update in progress\"");
        user_sendsms(v, num);
      } else
      // добавить ID
      if (strcmp(cmd, "add") == 0){
        if (validFormat){
          strncpy(&tempStrBuffer[96], arg, 10);
          tempStrBuffer[106] = 0; // ID завершён; короткий аргумент дополнен нулями.
          user_addIdToTempBuffer(&tempStrBuffer[96], CT_OTHER);
          log_addControlEvent(LOG_CON_IDADDBYSMS, &tempStrBuffer[96], 0);
          strcat(v, ": ");
          strcat(v, &tempStrBuffer[96]);
          strcat(v, " - Ok");
          user_sendsms(v, num);
        } else{
          strcat(v, ": Error \"Format not correct\""); 
          user_sendsms(v, num);
        }
      } else
      
      // запросить IMEI
      if (strcmp(cmd, "imei") == 0){
        strcat(v, ": ");
        strcat(v, device.IMEIGSM);
        user_sendsms(v, num);
      } else
        
      // запросить статус
      if (strcmp(cmd, "status") == 0){
        /* -NET: тип сети comm_info (0 нет, 1 GSM/GPRS, 3 3G, 4 LTE); -CLOUD: связь/номер сервера;
           -TRIG: E1-E4 - первые четыре триггера команды 5. */
        static const char *const net[] = {"Not network", "gprs", "edge", "3G", "4G", "5G"};
        format_status(v, NULL,
                device.csq, net[device.netType < 6u ? device.netType : 0], device.connected,
                user_comm_server(comm_server_address()) + 1, device.firmwareVersion,
                device.triggers.handUnBlock, device.triggers.sheduleUnBlock,
                device.triggers.autoregistration, device.triggers.openAll);
        user_sendsms(v, num);
      } else
        
      // перезагрузить
      if (strcmp(cmd, "reset") == 0){
        strcat(v, ": Go to reset");
        user_sendsms(v, num);
        device.timeToReset = 5;
      } else
        
      // включить выход N
      if (strcmp(cmd, "setout") == 0){
        if (validFormat){
          uint8_t out = atoi(arg);

          if (out == 1 || out == 2){
            if (config.rele[out - 1u].onCmdServer){
              out_load (out == 1 ? OUT1 : OUT2, OUT_ON, 0, 0, 0);
              strcat(v, out == 1 ? ": Out 1 enabled" : ": Out 2 enabled");
            } else{
              strcat(v, ": Error, Out is blocked");
            }
          } else{ // выхода с таким номером нет
            strcat(v, ": Error \"Format not correct\"");
          }
          user_sendsms(v, num);
        } else{
          strcat(v, ": Error \"Format not correct\""); 
          user_sendsms(v, num);
        }  
      } else
        
      // обновить прошивку N
      if (strcmp(cmd, "update") == 0){
        //777
      } else
        
      // заводские настройки
      if (strcmp(cmd, "default") == 0){
        user_factoryReset();
        strcat(v, ": Complete");
        user_sendsms(v, num);
      }
      
    } else{
      strcat(v, ": Error \"Pin not valid\""); 
      user_sendsms(v, num);
    }
  }

  return true;
}
#endif

// отправить СМС
#ifndef HYBRID_CARD
void user_sendsms(char *mes, char *num){  
#ifdef DEBUG_USER
  user_debug(">>U Sending SMS ...\r\n"); 
#endif
  if (user_comm_sms(num, mes))
    log_addSystemEventID(LOG_SYS_SENDSMS, user_last10digitsnumber(num));
}
#endif

// СМС отправлено
#ifndef HYBRID_CARD
void user_smssended (void)
{
#ifdef DEBUG_USER
  user_debug(">>U SMS sended ...\r\n"); 
#endif  
}
#endif

// входящий вызов
#ifndef HYBRID_CARD
bool user_incomingcall (char* num, uint8_t typ)
{
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U InCalling: %s\r\n", num);
  user_debug(tempStrBuffer); 
#endif
  
  if (typ == CT_PHONE){
    
    // номер звонящего: длиннее поля не пишем
    strncpy(device.callingnumber, num, sizeof(device.callingnumber) - 1u);
    device.callingnumber[sizeof(device.callingnumber) - 1u] = 0;
  }  

  // 10 цифр номера
  char *ldn = user_last10digitsnumber(num);  
  
  // проверить номер
  user_checkIdAndControl(ldn, (codeType_e)typ);  

  return true;
}
#endif

// ошибка команды
#ifndef HYBRID_CARD
void user_comanderror (int16_t er)
{
#ifdef DEBUG_USER
  user_debug(">>U Comand error ...\r\n"); 
#endif
}
#endif

// организовать подключения
// парсер USSD
#ifndef HYBRID_CARD
void user_parseUSSD(char* ptr)
{
#ifdef DEBUG_USER
  user_debug(">>U Parse USSD ...\r\n"); 
#endif  
}
#endif
// <<< переопределение функций --------------------------


// инициализация
#ifndef HYBRID_CARD
void user_init(void)
{
  // Определяем тип памяти
  device.memID = w25_read_devID();
  
  // константы адресов памяти
  mem[MB_FIRMWARE].start = 0;
  mem[MB_FIRMWARE].size = 131072;
  mem[MB_BACKUP].start = 135168;
  mem[MB_BACKUP].size = 131072;
  mem[MB_ID].start = 266240;
  mem[MB_ID].size = 1601536;
  mem[MB_GROUP].start = 1871872;
  mem[MB_GROUP].size = 2048;
  mem[MB_SHEDULE].start = 1878016;
  mem[MB_SHEDULE].size = 2048;
  mem[MB_HOLIDAY].start = 1884160;
  mem[MB_HOLIDAY].size = 1024;
  /* Логический адрес старого места. memory.c хранит новые образы в резерве 0x1F5000..0x1F8FFF
     (два сектора данных и два сектора фиксации). */
  mem[MB_CONFIG].start = 1889280;
  mem[MB_CONFIG].size = 4096;
  mem[MB_LOG].start = 1897472;
  mem[MB_LOG].size = 98304;
  mem[MB_TEMPID].start = 1999872;
  mem[MB_TEMPID].size = 49152;

  if (device.memID == 0x17C2){ // память 16 MBit
    mem[MB_PHOTO].start = 2097152;
    mem[MB_PHOTO].size = 1048576;
  } else{
    mem[MB_PHOTO].start = 0;
    mem[MB_PHOTO].size = 0;
  }

  //user_setdefault();
  
  // Считываем конфигурацию
  // Ошибка recovery блокирует таблицы; конфигурация и связь используют отдельные области.
  (void)mem_recover();
  user_readconfig();

  // инициализация переменных
  user_initvariables();
  // Сброс по питанию: одна запись; при отказе журнала сохраняем метку.
  if (BKP->DR1 == PVD_RESET_MARK && log_addSystemEvent(LOG_SYS_RESETONPOWER))
    BKP->DR1 = 0;
  app_boot_metadata();

  // Виганд
  if (config.wiegandType != 0)
    WG_init(); 
  
  // внешний порт
  rs485_init(); // порт 485
  uart_setRecTimeout(PORT1_PORT, 10); // время заполнения/ожидания приемного буфера 10 мс  
  store_runtime = 1;
}
#endif

// очистить блок памяти
#ifndef HYBRID_CARD
bool user_clearMemBlock(memBlock_e mb){
  
  if (mem[mb].size == 0) return true;

#ifdef DEBUG_USER
  user_debug(">>U Clear mem block ...\r\n"); 
#endif
  
  uint32_t adr;
  uint32_t endAdr = mem[mb].start + mem[mb].size;

  // по секторно стираем
  for (adr = mem[mb].start; adr < endAdr; adr += 4096){
    if (!mem_erase_sector(adr)) return false;
    wdt_reload();
  }
  if (mb == MB_ID) idindex_scanned(0, 0); /* база пуста - индекс пересобирается в фоне */
  return true;
}
#endif

// циклический обработчик
/* >>> Конец синхронизации (лист «Журнал событий», 6-10)
   Признака последнего пакета в протоколе нет: синхронизация идёт подряд командами 12-16
   (буфер 3 сервера), её конец - SYNC_QUIET_S секунд без них. Бит i - событие 6+i. */
#define SYNC_QUIET_S 30u
enum { SYNC_ID = 1u, SYNC_CONFIG = 2u, SYNC_GROUP = 4u, SYNC_SHEDULE = 8u, SYNC_HOLIDAY = 16u };
#ifdef HYBRID_CARD
extern uint8_t sync_pending, sync_quiet;
#else
uint8_t sync_pending, sync_quiet;
#endif
#ifndef HYBRID_CARD
static void sync_packet(uint8_t bit) { sync_pending |= bit; sync_quiet = SYNC_QUIET_S; }
#endif
/* Раз в секунду; журнал не принял запись - повтор через секунду. */
#ifndef HYBRID_CARD
static void sync_end_tick(void)
{
  if (!sync_quiet || --sync_quiet) return;
  for (uint8_t i = 0; i < 5u; ++i)
    if ((sync_pending & (1u << i)) && log_addSystemEvent((logSystem_e)(LOG_SYS_ENDIDUPDATE + i)))
      sync_pending &= (uint8_t)~(1u << i);
  if (sync_pending) sync_quiet = 1;
}
#endif
/* <<< Конец синхронизации */

#ifndef HYBRID_CARD
void user_loop(void)
{
  /* Карта - первой и на каждом проходе: конец кадра ловится сразу после паузы,
     а не на следующем 10-мс такте. */
  if (config.wiegandType != 0 && user_storage_flash_ready())
    WG_process();
  if (user_storage_command_ready() && user_storage_flash_ready()) idindex_poll(mem[MB_ID].start); /* фоновая пересборка индекса ID, короткий шаг */
  if (user_storage_command_ready() && user_storage_flash_ready()) fw_rollback_poll();
  fw_status_poll();
  unreg_sms_poll();
  reader_reply_poll(); /* пакет 8 ждёт свободного слота канала */

  // каждые 50 мс
  if (timers_get(1) >= 50)
  {
    timers_set(1,0);
    // запущен процесс обновления
    if (device.registredOnServer && device.FWinProgress){
      if (device.startWaitFWtime == 0){
        /* Срок ожидания - только для ушедшего запроса: канал занят (ушёл ответ 4) - повтор
           через 50 мс, а не через 20 с (было: блок в 20 с, 74 блока не успевали за 10 минут). */
        if (user_getFWblockFromServer((dataChanel_e)device.cchFWupdate, device.FWblockIdx))
          device.startWaitFWtime = timers_getMillis();
      } else{
        // не дождались блока прошивки
        if ((timers_getMillis() - device.startWaitFWtime) >= FW_BLOCK_RETRY_MS){
          // повторяем запрос
          device.startWaitFWtime = 0;
        }
      }
    }
  }

  // каждые 100 мс
  if (timers_get(2) >= 100)
  {
    timers_set(2,0);
    wdt_reload();

    led_fun100ms(); // обработка индикации
    user_readinputs(); // входы
    // состояние выходов
    device.status.stateOut1 = out_getstate(OUT1);
    device.status.stateOut2 = out_getstate(OUT2);
    /* Статус 1 вне очереди после изменения входа, выхода, триггера или бита 11. */
    if (device.registredOnServer){
      uint16_t st;
      memcpy((uint8_t*)&st, (uint8_t*)&device.status, 2);
      if (status_event_due(st, timers_getMillis()))
        user_send_statusPack(CH_SERVER);
    }
    // состояние выхода отображаем светодиодом
    if (device.status.stateOut1 || device.status.stateOut2){
      LED3_ON;
    } else{
      LED3_OFF;
    }
  }

  /* Выходы - раз в 100 мс (outs.h): импульс и пауза с точностью 0,1 с. */
  uint32_t outTicks = timers_take(0, 100);
  while (outTicks != 0) { --outTicks; out_process(); }

  // каждую 1 сек
  // воспроизводим накопленные секунды; новые тики остаются на следующий проход
  uint32_t elapsedSeconds = timers_take(3, 1000);
  while (elapsedSeconds != 0)
  {
    --elapsedSeconds;
    wdt_reload();
    
    
    // есть подключение по USB но не настроено
    if (USB_DETECT)
    {
      if (usb_get_state() != ON)
        usb_init();
    }
    else
      device.userAutorized = 0; // кабель вынут - ПИН вводится заново
    
    user_indication(); // индикация
        
    device.worktime++; // время работы от момента включения, в секундах
    device.datetime++; // внутренние часы
    log_checkPending(); // события, ждущие времени от сервера
    if (user_storage_flash_ready()) user_checkIdTimeEnd(); // окончание времени действия ID - событие 138
    // вычислим день недели, час и минуту
    device.day = user_getweakday(device.datetime);
    device.hour = device.datetime % 86400u / 3600u;
    device.minute = device.datetime % 3600u / 60u;
    // уровень сигнала
    device.connected = comm_channel_ready();
    /* Статусы 1/2 и NextConfig: сигнал, регистрация CS/PS, тип сети (как у прежней связи). */
    device.csq = comm_info()->csq;
    device.creg = comm_info()->creg;
    device.cgreg = comm_info()->cgreg;
    device.netType = comm_info()->net;
    
    // перезагрузка через заданный таймаут
    if (device.timeToReset){
      if (--device.timeToReset == 0){
        log_addSystemEvent(LOG_SYS_RESETONCMD);
#ifdef DEBUG_USER
  user_debug(">>U Reset on time\r\n"); 
#endif         
        user_savereset();
      }
    }
    
    // сброс на заводские
    if (device.timeToDefault){
      if (--device.timeToDefault == 0){
#ifdef DEBUG_USER
  user_debug(">>U Factory on time\r\n"); 
#endif        
        user_factoryReset();
      }
    }
    
    // таймаут на отправку ID
    if (device.timeToGetID){
      // ID будет передан при условии что получен или выход таймауту
      if (--device.timeToGetID == 0){
        // ошибка на Запрос идентификаторов
        device.sourceGetID = 0;
        user_send_readedIDtoServer(0, 255);
      }
    }
    sync_end_tick(); // события 6-10
    
    // время получения ответа от сервера при подключении
    if (device.answerTimeout){
      if (--device.answerTimeout == 0){
        switch (device.answerID){
        case 1:
#ifdef DEBUG_USER
  user_debug(">>U Not answer to reg pack\r\n"); 
#endif
          device.registredOnServer = 0;
        comm_channel_reopen(0);
          break;
          
        case 2:
          if (++device.lmCount >= MAXSTATUSANSWERERROR){
#ifdef DEBUG_USER
  user_debug(">>U Not answer to status\r\n"); 
#endif            
            device.registredOnServer = 0;
        comm_channel_reopen(0);
          }
          break;
        }
      }
    }
    
    // отправить live message
    if (device.registredOnServer){
      // отправка статуса
      if (device.lmTimer){
        if (--device.lmTimer == 0){
          device.lmTimer = STATUSTIMERINTERVAL;
          user_send_statusPack(CH_SERVER);
        }
      }
    }
    
    // перезагрузиться в режим обновления FW
    if (device.rebootToUpdate){
      device.rebootToUpdate = 0;
      if (app_boot_request()) {
        fw_log(LOG_FW_INSTALL, NULL);
        user_savereset();
      } else {
        fw_end("INSTALL", uniqIDCnt++);
      }
    }

    // таймаут обновления
    if (device.updateTimeout){
      if (--device.updateTimeout == 0){
        fw_end("TIMEOUT", uniqIDCnt++);
      } else if (device.FWinProgress && device.updateTimeout % 60u == 0){
        user_saveindex(); /* после сброса докачка продолжает с оставшимся временем */
      }
    }

    // пробная прошивка после обновления так и не зарегистрировалась на сервере - откат
    if (app_trial_expired()){
#ifdef DEBUG_USER
  user_debug(">>U Trial expired, rollback\r\n");
#endif
      app_trial_rollback();
      user_savereset();
    }
  }

  // каждые 10 сек
  if (timers_get(4) >= 10000)
  {
    timers_set(4,0);
  }

  // каждую 1 минуту
  if (timers_get(5) >= 60000)
  {
    timers_set(5,0);

    // использовать время NTP - синхронизировать
    
    // задано время на перезагрузку
    if (config.restartTime != 0){
      if (device.worktime >= ((uint32_t)config.restartTime*3600)){
        log_addSystemEvent(LOG_SYS_RESETONTIMER);
#ifdef DEBUG_USER
  user_debug(">>U Reset on work time\r\n"); 
#endif
        user_savereset();
      }
    }
    
  } // 1 минута
}
#endif

// инициализация переменных
#ifndef HYBRID_CARD
void user_initvariables (void)
{
  // чистим
  memset((pInt8U)&device, 0, sizeof(device));

  device.connectionhandle = -1;
  device.memID = w25_read_devID();
  device.freeBufferSize = 2048;
 
  // чтение сохранённой информации
  user_readsavedinformation();
  device.protocolVersion = PROTOCOL_VERSION;
  
  user_comm_boot();
  
  // смотрим сколько в памяти номеров
  device.numbersCount = user_getNumbersCount();
  device.logsCount = log_getRecordsCount();
  if (device.logsCount)
    device.status.readyLogData = 1;
  device.tempIDsCount = user_getTempIDRecordsCount();
  if (device.tempIDsCount)
    device.status.readyID = 1;
  device.countPhoto = user_getPhotoCount();
  if (device.countPhoto)
    device.status.readyPhoto = 1; 
}
#endif

// Чтение конфигурации с памяти
#ifndef HYBRID_CARD
static void config_exchange(uint8_t *data)
{
  uint8_t *p = (uint8_t*)&config;
  for (uint16_t i = 0; i < sizeof(config); i++) {
    uint8_t b = p[i];
    p[i] = data[i];
    data[i] = b;
  }
}
#endif

#ifndef HYBRID_CARD
static void config_terminate_strings(void)
{
  bool changed = false;
  // CRC проверена до вызова; завершаем только строки без нуля внутри поля.
#define CONFIG_TERM(field) \
  if (memchr(config.field, 0, sizeof(config.field)) == NULL){ \
    config.field[sizeof(config.field) - 1] = 0; changed = true; \
  }
  CONFIG_TERM(server1);
  CONFIG_TERM(server2);
  CONFIG_TERM(NTPserver);
  CONFIG_TERM(netOperator);
  CONFIG_TERM(countryCode);
  CONFIG_TERM(simPhone);
  CONFIG_TERM(ussdBalance);
#undef CONFIG_TERM
  if (changed) config.crc = crc16_calc((uint8_t*)&config, sizeof(config) - 2);
}
#endif

#ifndef HYBRID_CARD
void user_readconfig(void)
{
  uint8_t read_count = 20;

#ifdef DEBUG_USER
  user_debug(">>U Read config ...\r\n"); 
#endif  
  
  // читаем
  while (read_count){
    wdt_reload();
    mem_read(mem[MB_CONFIG].start, (pInt8U)&config, sizeof(config));
    if (config.crc == crc16_calc((pInt8U)&config, sizeof(config)-2)) // контрольная сумма совпала
      break;
    delay_ms(10);
    read_count--;
  }
  
  // исчерпали попытки
  if (read_count == 0){
    user_setdefault(); // грузим блок по умолчанию
    user_saveindex();
  }
  else config_terminate_strings();
}
#endif

// запись конфигурации в память
#ifndef HYBRID_CARD
void user_writeconfig(void)
{
  if (store_runtime) { store_save_requested = 1; return; }
  wdt_reload();
  config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);
  (void)mem_patch(mem[MB_CONFIG].start, (const uint8_t*)&config, sizeof(config));
  wdt_reload();
}
#endif

// Загрузить настройки по умолчанию
#ifndef HYBRID_CARD
void user_setdefault (void)
{
  wdt_reload();
  
#ifdef DEBUG_USER
  user_debug(">>U Set default params ...\r\n"); 
#endif
  
  // чистим
  memset((pInt8U)&config, 0, sizeof(config));

  config.pinCode = 0x31323334;
  strcpy(config.server1, "gate.gsmgate.ru:15010");
  strcpy(config.server2, "gate.gsmgate.ru:15020");
  strcpy(config.countryCode, "+7");
  strcpy(config.netOperator, "internet,gdata,gdata");
  config.netType = 0; // автоматически: плата с модемом 4G
  config.useServer1 = 1;
  
  config.rele[0].mode = 0;
  config.rele[0].onCmdServer = 1;
  config.rele[0].onOther = 1;
  config.rele[0].tImpulse = 1;
  config.rele[0].tPause = 3;
  config.rele[0].cnt = 1;
  
  device.loadDefaultConfig = 1;
  device.moduleType = MODEM_TYPE_AUTO;
  device.status.defaultConfig = 1;
  log_addSystemEvent(LOG_SYS_DEFCONF);
  
  user_writeConfigAndDevice();
  //user_writeconfig();
}
#endif

// Сброс всего
#ifndef HYBRID_CARD
void user_factoryReset (void){
  fw_async_cancel();
#ifdef DEBUG_USER
  user_debug(">>U Factory reset\r\n"); 
#endif  
  
  // закрываем соединение с сервером
  upload_reset(10);
  upload_reset(9);
  if (!user_writeConfigAndDevice()) return; // не стираем с сохранённым старым курсором
  
  // чистим память абонентов 
  user_clearMemBlock(MB_ID);
  // чистим память групп 
  user_clearMemBlock(MB_GROUP);
  // чистим память расписаний 
  user_clearMemBlock(MB_SHEDULE);
  // чистим память праздников 
  user_clearMemBlock(MB_HOLIDAY);
  // чистим память журнала 
  user_clearMemBlock(MB_LOG);
  // чистим память временные ID 
  user_clearMemBlock(MB_TEMPID);
  // чистим память фото 
  user_clearMemBlock(MB_PHOTO);
  
  /* Заводской сброс отменяет загрузку: user_setdefault сохраняет это состояние, и после
     перезапуска ничего не продолжается. */
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;

  // чистим конфиг
  user_setdefault();
  // реинициализируем переменные
  user_initvariables();
  
  // перезагружаем gsm
  user_comm_boot();
}
#endif

// чтение сохраненной информации
#ifndef HYBRID_CARD
void user_readsavedinformation (void)
{
  device_s DV;
  uint8_t read_count = 10;
  
  while (read_count--){
    wdt_reload();
    // Блок DEVICE
    mem_read (mem[MB_CONFIG].start + sizeof(config), (pInt8U)&DV, sizeof(DV));
    if (DV.crc == crc16_calc((pInt8U)&DV, sizeof(DV)-2)){ // контрольная сумма совпала
      DV.IMEIGSM[sizeof(DV.IMEIGSM) - 1] = 0; // Завершаем после проверки CRC.
      strcpy(device.IMEIGSM, DV.IMEIGSM); // восстанавливаем IMEI GSM

      device.protocolVersion = PROTOCOL_VERSION; // всегда версия из таблицы (строка 426), не сохранённая
      device.hardwareVersion = DV.hardwareVersion;
      device.firmwareVersion = DV.firmwareVersion;
      device.bootVersion = DV.bootVersion;
      device.buildDate = DV.buildDate;
      
      device.datetimeConfig = DV.datetimeConfig;
      device.datetimeID = DV.datetimeID;
      device.datetimeShedule = DV.datetimeShedule;
      device.datetimeGroup = DV.datetimeGroup;
      device.datetimeHoliday = DV.datetimeHoliday;
      
      //device.objNumber = DV.objNumber;
      //strcpy(device.objName, DV.objName);
      //strcpy(device.controllerName, DV.controllerName);
      //strcpy(device.adminNumber, DV.adminNumber);
      
      device.triggers = DV.triggers;
      
      device.firmwareSize = DV.firmwareSize;
      device.firmwareCRC = DV.firmwareCRC;
      device.FWblockCount = DV.FWblockCount;
      device.FWblockIdx = DV.FWblockIdx;
      device.firmwarePercent = DV.firmwarePercent;
      device.FWinProgress = DV.FWinProgress;
      device.updateTimeout = DV.updateTimeout;
      
      device.moduleType = DV.moduleType;
      upload_restore(&DV);
      store_restore(&DV);
       
      break;
    }
    delay_ms(10);
  }
  fw_resume_after_boot();
}
#endif

// Безопастное завершение работы прибора
#ifndef HYBRID_CARD
void user_savereset (void)
{
  if (store_runtime) { idclear_runtime_cancel(); expiry_runtime_cancel(); fw_service_cancel(); upload_runtime_cancel(); service_crc_cancel(); table_runtime_cancel(); photo_storage_cancel(); store_reset_requested = 1; return; }
  log_flushPending(); // события, не дождавшиеся времени, - в журнал до перезагрузки
  user_saveindex(); // сохраняем индексы и т.п.
  wdt_reload();
  
  // Отключаем USB
  USB_PULLUP_OFF; // выключаем подтягивающий резистор

  // выключаем gsm
  comm_modem_off();
  
  // перезагружаем
  NVIC_SystemReset();
}
#endif

// Сохранение указателей чтения/записи в памяти
/* Настройки попадают в сектор из ОЗУ, где их проверили при старте, а не из чтения сектора:
 * чтение, испорченное на провале питания (включение модема), публиковалось в слот с верной CRC
 * слота, и следующий старт находил неверную CRC настроек и записывал заводские (18.09.2026). */
#ifndef HYBRID_CARD
void user_saveindex (void)
{
  if (store_runtime) { store_save_requested = 1; return; }
  user_writeConfigAndDevice();
}
#endif

// запись конфигурации и параметров device в память
#ifndef HYBRID_CARD
bool user_writeConfigAndDevice(void)
{
  wdt_reload();
  config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);
  device.crc = crc16_calc((pInt8U)&device, sizeof(device)-2);
  bool ok = mem_patch2(mem[MB_CONFIG].start, (const uint8_t*)&config, sizeof(config),
                      (const uint8_t*)&device, sizeof(device));
  wdt_reload();
  return ok;
}
#endif

#ifndef HYBRID_CARD
void port_check_data(void)
{
  if (!user_storage_flash_ready()) return;
  photo_poll();
  if (!device.getPhoto && uart_getstate(PORT1_PORT) && !uart_isReceiving(PORT1_PORT))
    uart_clear(PORT1_PORT);
}
#endif

// проверка входящих данных от модуля
#ifndef HYBRID_CARD
void gsm_check_data (void)
{
  /* Каждый проход главного цикла: порт, AT, модем, канал, сервер, восстановление. */
  user_comm_poll(timers_getMillis());
}
#endif

// расчет контрольной суммы структуры данных во flash
#ifndef HYBRID_CARD
uint16_t user_calcCRC16InFlash(uint32_t address, uint32_t siz){
  uint16_t crc = 0xFFFF;
  uint8_t bfr[1024];
  uint16_t block;
  
  while (siz){
    if (siz > 1024)
      block = 1024;
    else
      block = siz;
    
    mem_read(address, bfr, block);
    crc = crc16_calcInit(bfr, block, crc);
    
    address += block;
    siz -= block;
  }
  
  return crc;
}
#endif

/* Конец записанной части базы ID, байт от начала области; дальше флеш стёрта (0xFF).
   UINT32_MAX - ещё не известно, тогда читается вся область. */
#ifdef HYBRID_CARD
extern uint32_t id_used_bytes;
#else
uint32_t id_used_bytes = UINT32_MAX;
#endif

/* CRC-16 части базы ID [off, off+n): читается только записанная часть, стёртый хвост
   досчитывается без чтения. Результат тот же, что при чтении всей области. */
#ifndef HYBRID_CARD
static uint16_t id_crc16(uint32_t off, uint32_t n){
  uint32_t real = 0;
  if (off < id_used_bytes) real = id_used_bytes - off < n ? id_used_bytes - off : n;
  uint16_t crc = real ? user_calcCRC16InFlash(mem[MB_ID].start + off, real) : 0xFFFF;
  return crc16_ffInit(n - real, crc);
}
#endif

// Отправка данных в формате протокола
/* Назначение кадра серверу — единственный слот канала, без копии/heap. */
#ifndef HYBRID_CARD
uint8_t *user_comm_acquire(uint16_t needed)
{
  uint16_t capacity; uint8_t *p = comm_tx_acquire(&capacity);
  if (p && needed > capacity) { comm_tx_release(); return NULL; }
  return p;
}
#endif
#define user_comm_free(p, cch) do { if ((cch) == CH_SERVER) comm_tx_release(); else work_release(p, WORK_REPLY); } while (0)

#ifndef HYBRID_CARD
void user_send_anyData(uint8_t id, uint8_t* data, uint16_t size, dataChanel_e cch, uint8_t uniqId){
  // весь кадр должен помещаться в 16-битную длину транспорта
  if (size > 65524u || (size && data == NULL)) return;
  uint16_t crc;
  uint16_t len = size + 3; // timeout comand + uniqId + id + payload  
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);
  
  if (send_buffer != NULL){

#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U Send any data (cmd %d)\r\n", id);
  user_debug(tempStrBuffer);
#endif    
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len); // длина
    send_buffer[5] = _2B(len);
    
    // ------- меняемый блок ---------
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqId; // uniqId
    send_buffer[8] = id; // id

    if (size) memcpy(&send_buffer[9], data, size); // imei
    // ------- меняемый блок ---------
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      comm_tx_commit(len + 8);
      break;
      
    case CH_SMS:
      
      break;
    } 
    
    user_comm_free(send_buffer, cch);
  }
}
#endif

// Пакет 1 - пакет регистрации контроллера на сервере
#ifndef HYBRID_CARD
void user_send_registrationData(dataChanel_e cch){
  
  uint16_t crc;
  uint16_t len = 40; // timeout comand + uniqIDCnt + id + payload
  uint8_t send_local[48];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;
  
#ifdef DEBUG_USER
  user_debug(">>U Send reg data (cmd 1)\r\n"); 
#endif   
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = TIMETOREGRESPONSE; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++; // uniqIDCnt
  send_buffer[8] = 1; // id

  memcpy(&send_buffer[9], (uint8_t*)&device.IMEIGSM, 15); // imei
  
  send_buffer[24] = device.protocolVersion; // версия протокола
  send_buffer[25] = device.hardwareVersion; // версия железа
  
  tmp32 = device.datetimeID;
  memcpy(&send_buffer[26], (uint8_t*)&tmp32, 4); // таймштамп ИД
  tmp32 = device.datetimeGroup;
  memcpy(&send_buffer[30], (uint8_t*)&tmp32, 4); // таймштамп групп
  tmp32 = device.datetimeShedule;
  memcpy(&send_buffer[34], (uint8_t*)&tmp32, 4); // таймштамп засписаний
  tmp32 = device.datetimeConfig;
  memcpy(&send_buffer[38], (uint8_t*)&tmp32, 4); // таймштамп конфига
  tmp32 = device.datetimeHoliday;
  memcpy(&send_buffer[42], (uint8_t*)&tmp32, 4); // таймштамп празников  
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc 
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    device.answerID = 1;
    device.answerTimeout = TIMETOREGRESPONSE;
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }    
}
#endif

// Пакет 2 - пакет статуса 1
#ifndef HYBRID_CARD
void user_send_statusPack(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 18; // timeout comand + uniqIDCnt + id + payload
  uint8_t send_local[26];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;
  
#ifdef DEBUG_USER
  user_debug(">>U Send status (cmd 2)\r\n"); 
#endif    
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = TIMETOSTATUSRESPONSE; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++; // uniqIDCnt
  send_buffer[8] = 2; // id

  tmp32 = device.datetime;
  memcpy(&send_buffer[9], (uint8_t*)&tmp32, 4); // время
  memcpy(&send_buffer[13], (uint8_t*)&device.status, 2); // статус
  tmp32 = device.freeBufferSize;
  memcpy(&send_buffer[15], (uint8_t*)&tmp32, 2); // Размер буфера входящих данных 
  
  memset(&send_buffer[17], 0, 7); // резерв  
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    /* Внеочередной статус (таблица E92) не должен перезапускать уже идущее ожидание
     * подтверждения: ожидание держит предел 15 с из таблицы A86. */
    if (device.answerID != 2 || device.answerTimeout == 0){
      device.answerID = 2;
      device.answerTimeout = TIMETOSTATUSRESPONSE;
    }
    comm_server_status_sent(send_buffer[7]);
    status_event_sent((uint16_t)(send_buffer[13] | (send_buffer[14] << 8)), timers_getMillis());
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 3 - пакет статуса 2
#ifndef HYBRID_CARD
void user_send_statusPackSecond(dataChanel_e cch, uint8_t uniqId){
    
  uint16_t crc;
  uint16_t len = 24; // timeout comand + uniqId + id + payload
  uint8_t send_local[32];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;

#ifdef DEBUG_USER
  user_debug(">>U Send status2 (cmd 3)\r\n"); 
#endif  
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId; // uniqId 
  send_buffer[8] = 3; // id

  tmp32 = device.datetime;
  memcpy(&send_buffer[9], (uint8_t*)&tmp32, 4); // время
  send_buffer[13] = device.csq; // уровень сигнала
  tmp32 = device.balance;
  memcpy(&send_buffer[14], (uint8_t*)&tmp32, 2); // баланс
  send_buffer[16] = device.firmwareVersion; // версия прошивки
  tmp32 = device.buildDate;
  memcpy(&send_buffer[17], (uint8_t*)&tmp32, 4); // билд
  send_buffer[21] = device.bootVersion; // версия загрузчика
  memcpy(&send_buffer[22], (uint8_t*)&device.peripheryState, 8); // состояние периферии
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 4 - пакет статуса обновления FW
/* >>> ход обновления */
#ifdef HYBRID_CARD
extern struct { uint8_t due, percent, code, id; } fw_status_pending;
#else
struct { uint8_t due, percent, code, id; } fw_status_pending;
#endif
#ifndef HYBRID_CARD
static void fw_status_poll(void)
{
  uint16_t capacity, crc;
  uint8_t *b;
  if (!fw_status_pending.due || !comm_server_online()) return;
  b = comm_tx_acquire(&capacity);
  if (!b) return;
  memcpy(b, "SLDN", 4); b[4]=5; b[5]=0; b[6]=255; b[7]=fw_status_pending.id; b[8]=4;
  b[9]=fw_status_pending.percent; b[10]=fw_status_pending.code;
  crc=crc16_calc(b+4,7); b[11]=(uint8_t)crc; b[12]=(uint8_t)(crc>>8);
  if (comm_tx_commit(13)) fw_status_pending.due=0;
}
#endif
/* <<< ход обновления */
#ifndef HYBRID_CARD
void user_send_statusUpdateFW(uint8_t cmd, uint8_t code, dataChanel_e cch, uint8_t uniqId){
  (void)cmd;
  uint16_t crc;
  uint16_t len = 5; // timeout comand + uniqId + id + payload
  uint8_t send_local[13];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) {
    if (cch == CH_SERVER) {
      fw_status_pending.due=1; fw_status_pending.percent=device.firmwarePercent;
      fw_status_pending.code=code; fw_status_pending.id=uniqId;
    }
    return;
  }
  
#ifdef DEBUG_USER
  user_debug(">>U Send status update (cmd 4)\r\n"); 
#endif    
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);  // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId; // uniqId
  send_buffer[8] = 4; // id
  //send_buffer[9] = 20; // Отправка блока прошивки
  send_buffer[9] = device.firmwarePercent; // Процент хода обновления или код ошибки
  send_buffer[10] = code; // 0 блок принят, 253 загрузка прекращена, 254 и 255 блок запрошен снова
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 5 - контрольные суммы данных структур
#ifndef HYBRID_CARD
void user_send_crcPack(dataChanel_e cch, uint8_t uniqId){
  if (store_runtime) { service_crc_begin_pack(cch,uniqId); return; }
  uint16_t crc;
  uint16_t len = 13; // timeout comand + uniqId + id + payload
  uint16_t crcData;
  uint8_t send_local[21];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  
#ifdef DEBUG_USER
  user_debug(">>U Send crc data (cmd 5)\r\n"); 
#endif    
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);  // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId; // uniqId
  send_buffer[8] = 5; // id
  
  crcData = id_crc16(0, mem[MB_ID].size);
  memcpy(&send_buffer[9], (uint8_t*)&crcData, 2); // crc id
  
  crcData = user_calcCRC16InFlash(mem[MB_GROUP].start, mem[MB_GROUP].size);
  memcpy(&send_buffer[11], (uint8_t*)&crcData, 2); // crc group
  
  crcData = user_calcCRC16InFlash(mem[MB_SHEDULE].start, mem[MB_SHEDULE].size);
  memcpy(&send_buffer[13], (uint8_t*)&crcData, 2); // crc shedule
  
  crcData = user_calcCRC16InFlash(mem[MB_CONFIG].start, mem[MB_CONFIG].size);
  memcpy(&send_buffer[15], (uint8_t*)&crcData, 2); // crc conf
  crcData = user_calcCRC16InFlash(mem[MB_HOLIDAY].start, mem[MB_HOLIDAY].size);
  memcpy(&send_buffer[17], (uint8_t*)&crcData, 2); // crc holiday
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }

}
#endif

// Пакет 6,15 -  пакет контрольные суммы стуктуры ID абонентов
#ifndef HYBRID_CARD
void user_send_crcMemoryID(uint8_t idx, dataChanel_e cch, uint8_t uniqId){
  if (store_runtime) { service_crc_begin_id(idx,cch,uniqId); return; }
  uint16_t crc;
  uint16_t len; // timeout comand + , uint8_t uniqId + id + payload
  uint16_t crcData;
  uint32_t memAddress = mem[MB_ID].start;
  uint32_t sizeBlock;
  
  /* Протокол2 E363-E365, E562-E566: 0 - пакет 6 со 100 CRC; 1-100 - пакет 15:
     номер блока и CRC16 блока N (ячейки (N-1)*500+1 ... N*500). */
  if (idx == 0){
    len = 203;
  } else{
    len = 6;
  }
  
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);
  
  if (send_buffer != NULL){
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    
    // ------- меняемый блок ---------
    sizeBlock = 500 * sizeof(cellID_s);
    
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqId; // uniqId
    
    if (idx == 0){ // все id
      send_buffer[8] = 6; // id
 
#ifdef DEBUG_USER
  user_debug(">>U Send crc data (cmd 6)\r\n"); 
#endif        
      
      for (uint16_t i = 0; i < 100; i++){
        crcData = id_crc16(memAddress - mem[MB_ID].start, sizeBlock);
        memcpy(&send_buffer[9 + (i*2)], (uint8_t*)&crcData, 2); // crc id
        memAddress += sizeBlock;
      }
    } else{ // конкретный id
      send_buffer[8] = 15; // id
      
#ifdef DEBUG_USER
  user_debug(">>U Send crc data (cmd 16)\r\n"); 
#endif  
      
      crcData = id_crc16(sizeBlock * (uint32_t)(idx - 1u), sizeBlock);
      send_buffer[9] = idx; // номер блока
      memcpy(&send_buffer[10], (uint8_t*)&crcData, 2); // crc блока
    }
    // ------- меняемый блок ---------
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      comm_tx_commit(len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }
    user_comm_free(send_buffer, cch);
  }
}
#endif

// Пакет 7 -  трансляция SMS
#ifndef HYBRID_CARD
bool user_send_smsToServer(char* sms){
  if (!comm_server_online()) return false;
  uint16_t crc;
  uint16_t capacity, size = 0;
  uint8_t *send_buffer = comm_tx_acquire(&capacity);
  if (!send_buffer) return false;
  if (!sms) { comm_tx_release(); return false; }
  /* Не дать strlen/16-битной длине обойти вместимость единственного слота. */
  while (size + 11u <= capacity && sms[size]) ++size;
  if (size + 11u > capacity) { comm_tx_release(); return false; }
  uint16_t len = size + 3u;

  if (send_buffer != NULL){
    
#ifdef DEBUG_USER
  user_debug(">>U Send sms to server (cmd 7)\r\n"); 
#endif
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    
    // ------- меняемый блок ---------
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqIDCnt++; // uniqIdCnt
    send_buffer[8] = 7; // id
    strcpy((char*)&send_buffer[9], sms);
    // ------- меняемый блок ---------
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    if (comm_tx_commit(len + 8)) return true;
    user_comm_free(send_buffer, CH_SERVER);
  }
  return false;
}
#endif

/* >>> уведомление об ID */
/* Последний неизвестный ID ждёт слота; все отказы остаются в журнале (событие 65). */
#ifdef HYBRID_CARD
extern struct { char id[10]; uint8_t due; } unreg_sms_pending;
#else
struct { char id[10]; uint8_t due; } unreg_sms_pending;
#endif
static void unreg_sms_note(const char *id)
{
  memcpy(unreg_sms_pending.id, id, 10);
  unreg_sms_pending.due = 1;
}
#ifndef HYBRID_CARD
static void unreg_sms_poll(void)
{
  char text[31];
  if (!unreg_sms_pending.due) return;
  memcpy(text, unreg_sms_pending.id, 10); text[10]=0;
  strcat(text, ": ID is unregistred!");
  if (user_send_smsToServer(text)) unreg_sms_pending.due = 0;
}
#endif
/* <<< уведомление об ID */

// Пакет 8 - трансляция ID со считывателей
// cmdState: 1 - норм, 255 - таймаут
/* >>> ответ считывателя (пакет 8)
   Ответ на команду 6 ждёт свободного слота канала: раньше при занятом слоте (статус,
   ответ на команду) он молча терялся и сервер не узнавал ни ID, ни конец ожидания. */
#ifdef HYBRID_CARD
extern struct { uint8_t state; char id[10]; } reader_out;
#else
struct { uint8_t state; char id[10]; } reader_out;
#endif
#ifndef HYBRID_CARD
void user_send_readedIDtoServer(uint8_t* id, uint8_t cmdState){
  reader_out.state = cmdState; // 1 - ID считан, 255 - время вышло
  if (id == 0) memset(reader_out.id, 0, 10);
  else memcpy(reader_out.id, id, 10);
}
#endif
#ifndef HYBRID_CARD
static void reader_reply_poll(void)
{
  if (!reader_out.state) return;
  uint8_t *b = user_comm_acquire(22);
  if (!b) return;
  b[0] = 'S'; b[1] = 'L'; b[2] = 'D'; b[3] = 'N'; b[4] = 14; b[5] = 0; // длина: 3 + код + 10 байт ID
  b[6] = 255; b[7] = uniqIDCnt++; b[8] = 8; b[9] = reader_out.state;
  memcpy(&b[10], reader_out.id, 10);
  uint16_t crc = crc16_calc(&b[4], 16);
  b[20] = (uint8_t)crc; b[21] = (uint8_t)(crc >> 8);
  if (comm_tx_commit(22)) reader_out.state = 0;
}
#endif
/* <<< ответ считывателя */

// Пакет 11 - подтверждение приема команды от сервера
#ifndef HYBRID_CARD
void user_send_answer_comand(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId){
  
  uint16_t crc;
  uint16_t len = 11; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_local[19];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;
  
#ifdef DEBUG_USER
  user_debug(">>U Send answer (cmd 11)\r\n"); 
#endif    
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);  // длина
  send_buffer[5] = _2B(len);
  send_buffer[6] = (cmd == 7 || cmd == 21) ? 5 : 255; // таймаут актуальности команды
  send_buffer[7] = uniqId; // uniqId
  send_buffer[8] = 11; // id
  send_buffer[9] = cmd; // На какую именно команду выдается подтверждение
  send_buffer[10] = cc; // Код выполнения или ошибки
  tmp32 = device.datetime;
  memcpy(&send_buffer[11], (uint8_t*)&tmp32, 4); // Время контроллера
  tmp32 = device.freeBufferSize;
  memcpy(&send_buffer[15], (uint8_t*)&tmp32, 2); // Размер буфера входящих данных 

  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 12 - таймштамп структур
#ifndef HYBRID_CARD
void user_send_timeshtamp(shtamp_e sht, uint32_t time, uint16_t crcData, dataChanel_e cch, uint8_t uniqId){
  
  uint16_t crc;
  uint16_t len = 10; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_local[18];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  
#ifdef DEBUG_USER
  user_debug(">>U Send timeshtamp (cmd 12)\r\n"); 
#endif    
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);  // длина
  send_buffer[5] = _2B(len);
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId; // uniqId
  send_buffer[8] = 12; // id
  send_buffer[9] = sht; // к каким данным относится таймштамп
  memcpy(&send_buffer[10], (uint8_t*)&time, 4); // таймштамп
  memcpy(&send_buffer[14], (uint8_t*)&crcData, 2); // Контрольная сумма структуры
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 17 - запрос информации о привязки контроллера к объекту
// Пакет 100 - идентификация
#ifndef HYBRID_CARD
void user_send_identPackToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 19; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_local[27];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = 0; // таймаут актуальности команды
  send_buffer[7] = 0; // uniqIdCnt
  send_buffer[8] = 100; // id
  memcpy(&send_buffer[9], (uint8_t*)&device.IMEIGSM, 15);
  if (config.pinCode == 0x31323334)
    send_buffer[24] = 1;
  else
    send_buffer[24] = 0;
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 101 - пакет статуса 3
#ifndef HYBRID_CARD
void user_send_statusPackToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 34; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_local[42 + sizeof(fault_record)];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); // длина
  send_buffer[5] = _2B(len);
  
  // ------- меняемый блок ---------
  send_buffer[6] = 0; // таймаут актуальности команды
  send_buffer[7] = 0; // uniqIdCnt
  send_buffer[8] = 101; // id

  tmp32 = device.datetime;
  memcpy(&send_buffer[9], (uint8_t*)&tmp32, 4); // время
  memcpy(&send_buffer[13], (uint8_t*)&device.status, 2); // статус
  send_buffer[15] = device.firmwareVersion; // версия прошивки
  tmp32 = device.buildDate;
  memcpy(&send_buffer[16], (uint8_t*)&tmp32, 4); // билд
  send_buffer[20] = device.bootVersion; // версия загрузчика
  send_buffer[21] = device.hardwareVersion; // версия железа
  send_buffer[22] = device.csq; // уровень сигнала
  tmp32 = device.balance;
  memcpy(&send_buffer[23], (uint8_t*)&tmp32, 2); // баланс
  send_buffer[25] = device.creg;
  send_buffer[26] = device.cgreg; 
  send_buffer[27] = device.connected;
  send_buffer[28] = device.netType;
  memcpy(&send_buffer[29], (uint8_t*)&device.peripheryState, 8); // состояние периферии
  send_buffer[37] = _1B(device.numbersCount); // использовано памяти, кол-во ID
  send_buffer[38] = _2B(device.numbersCount);
  send_buffer[39] = device.moduleType;
  // ------- меняемый блок ---------
  
  /* Необязательное расширение FLT1 после исходных данных статуса. */
  if (fault_record[0] == FAULT_RECORD_MAGIC) {
    uint16_t i;
    for (i = 0; i < sizeof(fault_record); i++)
      send_buffer[40 + i] = ((volatile uint8_t*)fault_record)[i];
    len += sizeof(fault_record);
    send_buffer[4] = _1B(len);
    send_buffer[5] = _2B(len);
  }

  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    comm_tx_commit(len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}
#endif

// Пакет 106 - параметры конфигурации
#ifndef HYBRID_CARD
void user_send_configToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = sizeof(config_s) + 3; // timeout comand + uniqIdCnt + id + payload
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);
  
  if (send_buffer != NULL){
    
#ifdef DEBUG_USER
  user_debug(">>U Send sms to server (cmd 7)\r\n"); 
#endif
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    
    // ------- меняемый блок ---------
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = 0; // uniqIdCnt
    send_buffer[8] = 106; // id
    memcpy(&send_buffer[9], (uint8_t*)&config, sizeof(config_s));
    // ------- меняемый блок ---------
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;

    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      comm_tx_commit(len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    user_comm_free(send_buffer, cch);
  }
}
#endif

/* USB читает журнал со своей позиции и не стирает его - стирает только выгрузка сервером.
   Раньше позиция была общей, и чтение по USB забирало события у сервера (18.09.2026). */
#ifndef HYBRID_CARD
void user_send_logRecords(dataChanel_e cch, uint8_t uniqId){
  if (upload_runtime_begin(10,cch,uniqId)) return;
  if (cch != CH_USB && cch != CH_SERVER) return;
  if (store_runtime && !store_upload_request(10, cch, uniqId)) return;
  if (!upload_ack(10, cch)) return;
  uint16_t readIdx = upload_index(10, cch);

 
  // читаем кол-во записей в памяти
  device.logsCount = log_getRecordsCount();
  if (device.logsCount == 0xFFFFu) return; // чтение не завершилось, журнал не пустой

  /* Пустой ответ по таблице (пакет 10): осталось записей 0 (2 байта) и записей
     в пакете 0 (1 байт). Двухбайтовый ответ с остатком не 0 сервер принимал, но
     не закрывал выгрузку журнала и переставал отвечать на статус (18.09.2026). */
  uint8_t empty[3] = {0, 0, 0};

  if (device.logsCount == 0 || readIdx >= device.logsCount){
    if (!upload_finish(10)) return;
    user_send_anyData(10, empty, sizeof(empty), cch, uniqId);
    return; // пустой ответ данных не несёт; его доставки не ждём
  }

  uint16_t crc;  
  uint16_t cntId = device.logsCount - readIdx;

  if (cntId > 32) cntId = 32;
  // выделим память 1 кБ под записи журнала (32 * 32)
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire((cntId * sizeof(cellLog_s)) + 16) : work_acquire((cntId * sizeof(cellLog_s)) + 16, WORK_REPLY);
  uint16_t len = (cntId * sizeof(cellLog_s)) + 6;
  uint16_t cntId2 = device.logsCount - readIdx - cntId;
  
  if (send_buffer != NULL){
    
#ifdef DEBUG_USER
  user_debug(">>U Send log data (cmd 10)\r\n"); 
#endif
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqId; // uniqId   
    send_buffer[8] = 10; // id 
    send_buffer[9] = _1B(cntId2);  // остаток записей
    send_buffer[10] = _2B(cntId2);
    send_buffer[11] = cntId; // записей в текущем пакете
    // читаем из памяти
    if (!log_readRecords(readIdx, &send_buffer[12], cntId)) { user_comm_free(send_buffer, cch); return; }
    
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    bool sent = false;
    switch (cch){
    case CH_USB:
      sent = usb_send(send_buffer, len + 8) != 0;
      break;
      
    case CH_SERVER:
      sent = comm_tx_commit(len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    log_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;
    if (cch != CH_USB) device.status.readyLogData = 1;
    
    user_comm_free(send_buffer, cch);
  }
}
#endif

// Пакет 9 - выгрузка ID из временного буфера
#ifndef HYBRID_CARD
void user_send_tempIDs(dataChanel_e cch, uint8_t uniqId){
  if (upload_runtime_begin(9,cch,uniqId)) return;
  if (cch != CH_USB && cch != CH_SERVER) return;
  if (store_runtime && !store_upload_request(9, cch, uniqId)) return;
  if (!upload_ack(9, cch)) return;
  uint16_t readIdx = upload_index(9, cch);
 
  // читаем кол-во записей в памяти
  device.tempIDsCount = user_getTempIDRecordsCount();
  if (device.tempIDsCount == 0xFFFFu) return;

  if (device.tempIDsCount == 0 || readIdx >= device.tempIDsCount){
    if (!upload_finish(9)) return;
    uint8_t empty[3] = {0, 0, 0}; /* Protocol2: left (2) + count (1), as answer 10 */
    user_send_anyData(9, empty, sizeof(empty), cch, uniqId);
    return;
  }
  uint32_t readAdr = mem[MB_TEMPID].start + (uint32_t)readIdx * sizeof(cellID_s);

  uint16_t crc;  
  uint16_t cntId = device.tempIDsCount - readIdx;

  if (cntId > 100) cntId = 100;
  // выделим память 1 кБ под ID (100 * 10)
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire((cntId * 10) + 16) : work_acquire((cntId * 10) + 16, WORK_REPLY);
  uint16_t len = (cntId * 10) + 6;
  uint16_t cntId2 = device.tempIDsCount - readIdx - cntId;
  
  if (send_buffer != NULL){
    
#ifdef DEBUG_USER
  user_debug(">>U Send temp IDs (cmd 9)\r\n"); 
#endif
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqId; // uniqId      
    send_buffer[8] = 9; // id
    send_buffer[9] = _1B(cntId2);  // остаток записей
    send_buffer[10] = _2B(cntId2);
    send_buffer[11] = cntId; // записей в текущем пакете
    // читаем из памяти
    for (uint16_t i = 0; i < cntId; i++){
      if (!mem_read(readAdr + 2, &send_buffer[12 + (i*10)], 10)) { user_comm_free(send_buffer, cch); return; }
      readAdr += sizeof(cellID_s);
    }

    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    bool sent = false;
    switch (cch){
    case CH_USB:
      sent = usb_send(send_buffer, len + 8) != 0;
      break;
      
    case CH_SERVER:
      sent = comm_tx_commit(len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    temp_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;
    device.status.readyID = 1;
    
    user_comm_free(send_buffer, cch);
  }
}
#endif

// Пакет 13 - Запрос конкретного блока прошивки
#ifndef HYBRID_CARD
bool user_getFWblockFromServer(dataChanel_e cch, uint16_t block){
  if (fw_service_busy()) return false;
  if (fw_async_active()) return false;
  uint8_t send_local[16];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return false;
  uint16_t len = 5;
  uint16_t crc;
  
#ifdef DEBUG_USER
  if (cch != CH_USB){
    sprintf(tempStrBuffer,">>U Get FW block %d (cmd 13)\r\n", block);
    user_debug(tempStrBuffer);
  }
#endif     
  
  send_buffer[0] = 'S'; // идент
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);  // длина
  send_buffer[5] = _2B(len);
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++; // uniqId   
  send_buffer[8] = 13; // id 
  send_buffer[9] = _1B(block);  // номер блока
  send_buffer[10] = _2B(block);
    
  crc = crc16_calc(&send_buffer[4], len + 2);
    
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
    
  if (cch == CH_SERVER) return comm_tx_commit(len + 8);
  if (cch == CH_USB) usb_send(send_buffer, len + 8);
  return true;
}
#endif

// Пакет 14 - передача фотографии
#ifndef HYBRID_CARD
void user_send_Photos(dataChanel_e cch, uint8_t uniqId)
{

#ifdef ASM_RUNTIME
  photo_request_answer(cch,uniqId,NULL,0);
#else
  user_send_answer_comand(21, photo_request(cch, uniqId, NULL, 0), cch, uniqId);
#endif
}
#endif

/* >>> время действия ID
 * Протокол2, строка 284: через useTime минут после первого прохода ID помечается на удаление -
 * больше не открывает. Время первого прохода контроллер хранит в первых 4 байтах резерва записи
 * ID (сервер шлёт там нули) и отсчитывает только по времени сервера (18.09.2026).
 * Первый проход пишет событие 139. Если время закончилось в те же сутки, в момент окончания
 * пишется 138, один раз: отметка в байте 6 резерва. Закончилось позже 23:59:59 - события нет,
 * ID удаляет сервер (Алексей, 29.09.2026). Ближайшее окончание держится в ОЗУ, база
 * перечитывается только к нему, при старте и при новом времени от сервера. */
#define ID_TIME_VALID 1600000000u // раньше этого времени часы ещё не получены от сервера
#define ID_TIME_DONE  0x38u       // резерв[6]: событие 138 записано
#ifdef HYBRID_CARD
extern uint32_t id_nextEnd;
#else
uint32_t id_nextEnd;
#endif       // ближайшее окончание в сутки первого прохода, 0 - нет
#ifndef HYBRID_CARD
bool id_endScan = true;
#else
extern bool id_endScan;
#endif    // перечитать базу: старт, новое время от сервера
static uint32_t id_first_pass(cellID_s *c){
  uint32_t t;
  memcpy(&t, c->reserve, 4);
  return (t == 0xffffffff || t < ID_TIME_VALID) ? 0 : t;
}
static void id_set_first_pass(cellID_s *c, uint32_t t){
  memcpy(c->reserve, &t, 4);
}
static bool id_expired(cellID_s *c){
  uint32_t first = id_first_pass(c);
  return c->useTime != 0 && first != 0 && device.datetime >= first &&
    device.datetime - first >= (uint32_t)c->useTime * 60u;
}
// окончание времени действия, если оно в сутки первого прохода и 138 ещё не записано; иначе 0
#ifndef HYBRID_CARD
uint32_t id_time_end(cellID_s *c){
  uint32_t first = id_first_pass(c), end;
  if (c->useTime == 0 || first == 0 || c->reserve[6] == ID_TIME_DONE) return 0;
  // за пределами представимого Unix-времени нет раннего окончания
  if (first > 0xffffffffu - (uint32_t)c->useTime * 60u) return 0;
  end = first + (uint32_t)c->useTime * 60u;
  return (end / ONE_DAY == first / ONE_DAY) ? end : 0;
}
#endif
#ifndef HYBRID_CARD
void id_watch_end(uint32_t end){
  if (end != 0 && (id_nextEnd == 0 || end < id_nextEnd)) id_nextEnd = end;
}
#endif
/* <<< время действия ID */

/* >>> проходы в сутки
 * Остаток восстанавливается до «проходов в сутки» первым проходом новых суток: день последнего
 * списания хранится в байтах 4-5 резерва записи ID. Прежний сброс всей базы в полночь выходил
 * до записи последней страницы, писал Flash без стирания и пропускал сутки после перезагрузки
 * около полуночи (28.09.2026). День 0 - неизвестен: остаток, пришедший до времени сервера,
 * берётся как есть. */
static uint16_t id_today(void){
  return device.datetime >= ID_TIME_VALID ? (uint16_t)(device.datetime / ONE_DAY) : 0;
}
static uint16_t id_pass_day(cellID_s *c){
  uint16_t d;
  memcpy(&d, &c->reserve[4], 2);
  return d == 0xffff ? 0 : d;
}
static void id_set_pass_day(cellID_s *c, uint16_t day){
  memcpy(&c->reserve[4], &day, 2);
}
// новые сутки: остаток = лимит; true - запись ID изменилась
static bool id_new_day(cellID_s *c){
  uint16_t today = id_today(), last = id_pass_day(c);
  if (today == 0 || last == today) return false;
  if (last != 0) c->cntPass = c->cntPassOnDay;
  id_set_pass_day(c, today);
  return true;
}
/* <<< проходы в сутки */

/* USB без верного ПИНа: только опознание, статус, проверка ПИНа и диагностика USB. */
#ifndef HYBRID_CARD
static bool usb_cmd_without_pin(uint8_t cmd)
{
  return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_DIAG_COMMAND || cmd == USB_COMM_DIAG_COMMAND;
}
#endif

/* Статус 1 вне очереди (таблица E92, A57, A58): входы, выходы и триггеры (биты 0-7) и
 * готовность к обновлению (бит 11). Биты 12-15 следуют за журналом и выгрузками и идут по
 * расписанию раз в 15 с. Входы читаются без антидребезга, поэтому внеочередные статусы - не
 * чаще раза в секунду. */
#define STATUS_EVENT_MASK       0x08FFu
#define STATUS_EVENT_GAP_MS     1000u
#ifdef HYBRID_CARD
extern uint16_t status_reported;
#else
uint16_t status_reported;
#endif
#ifdef HYBRID_CARD
extern uint32_t status_reported_ms;
#else
uint32_t status_reported_ms;
#endif

#ifndef HYBRID_CARD
static uint8_t status_event_due(uint16_t status, uint32_t now_ms)
{
  return ((status ^ status_reported) & STATUS_EVENT_MASK) != 0 &&
         (uint32_t)(now_ms - status_reported_ms) >= STATUS_EVENT_GAP_MS;
}
#endif

#ifndef HYBRID_CARD
static void status_event_sent(uint16_t status, uint32_t now_ms)
{
  status_reported = status;
  status_reported_ms = now_ms;
}
#endif

/* Режим обновления, бит 11 статуса 1 (Протокол2 E440): команда 8 и управляющие SMS отвергаются.
 * Звонки, карты и входы работают: у звонка высший приоритет. */
#ifndef HYBRID_CARD
static uint8_t fw_update_quiet(void)
{
  return device.status.readyToUpdate;
}
#endif

/* Код причины для аргумента 2 событий обновления (Протокол2 E534; E122: причина отказа пишется
 * в журнал). Номер в списке + 1; «BOOT n» - код загрузчика n + 20. */
#ifndef HYBRID_CARD
static uint32_t fw_reason_code(const char *text)
{
  static const char *const names[] = {"PARAM", "BUSY", "OLD BOOT", "BOOT BUSY", "IN TRIAL",
    "NO BACKUP", "BAD BACKUP", "CRC", "WRITE", "CANCEL", "INSTALL", "HW", "TRIAL FAIL", "BACKUP",
    "TIMEOUT"};
  uint8_t i;
  if (text == NULL) return 0;
  for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    if (strcmp(text, names[i]) == 0) return i + 1u;
  if (strncmp(text, "BOOT ", 5) == 0 && text[5] >= '0' && text[5] <= '9') return 20u + (uint32_t)(text[5] - '0');
  return 30;
}
#endif

#ifndef HYBRID_CARD
void fw_log(logUpdate_e num, const char *reason)
{
  log_addUpdateEvent(num, fw_reason_code(reason));
}
#endif

#ifdef HYBRID_CARD
extern uint8_t fw_write_errors;
#else
uint8_t fw_write_errors;
#endif
#ifdef HYBRID_CARD
extern uint32_t fw_rollback_pos, fw_rollback_end;
#else
uint32_t fw_rollback_pos, fw_rollback_end;
#endif

/* Загрузка заканчивается без установки. Блоки получают 253 до следующей команды 19. */
#ifndef HYBRID_CARD
void fw_abort(const char *reason)
{
  fw_service_cancel();
  fw_async_cancel();
  fw_rollback_end = 0;
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;
  device.FWblockIdx = device.FWblockCount = 0;
  fw_write_errors = 0;
  if (reason != NULL) fw_log(strcmp(reason, "TIMEOUT") == 0 ? LOG_FW_TIMEOUT : LOG_FW_FAIL, reason);
  if (device.cchFWupdate != CH_USB) user_saveindex();
}
#endif

/* Контроллер сам прекращает загрузку (E96): команда 4 с кодом 253 по каналу загрузки, причина -
 * в журнал. */
#ifndef HYBRID_CARD
void fw_end(const char *reason, uint8_t uniqId)
{
  fw_abort(reason);
  if (device.cchFWupdate == CH_USB || device.registredOnServer)
    user_send_statusUpdateFW(20, FW_BLOCK_END, (dataChanel_e)device.cchFWupdate, uniqId);
}
#endif

/* В зарезервированном векторе 8 образа - 0x4857 и номер железа в младшем байте
 * (startup_stm32f10x_md.s, DOMOVOI_APP_HW). Чужая плата заканчивает загрузку событием 6.
 * Образы, собранные до появления метки, принимаются. */
#define FW_HW_MARKER_OFFSET     0x20u
#define FW_HW_MARKER_TAG        0x48570000u
#ifndef HYBRID_CARD
static uint8_t fw_image_foreign(const uint8_t *block0, uint8_t hw)
{
  uint32_t w;
  memcpy((uint8_t*)&w, block0 + FW_HW_MARKER_OFFSET, 4);
  return hw != 0 && (w & 0xFFFFFF00u) == FW_HW_MARKER_TAG && (uint8_t)w != hw;
}
#endif

/* Поля команды 19 (строки 368-372). Номер 0 зарезервирован; номер 1 - откат на резервную копию,
 * остальные поля могут быть нулевыми. Обновлению нужны билд, CRC и размер, помещающийся в
 * область образа. Таблица дописывает файл до кратного 1024; размер, кратный только 4, тоже
 * принимается, если блоков по 1024 байта хватает. */
#ifndef HYBRID_CARD
uint8_t fw_start_rejected(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks)
{
  if (number == 0) return 1;
  if (number == 1) return 0;
  return (size > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) ||
         ((size & 3u) != 0) ||
         (blocks < (size + 1023u) / 1024u) ||
         ((FWBLOCK_OFFSET + (uint32_t)blocks * 1024u) > mem[MB_FIRMWARE].size) ||
         (build == 0) || (size == 0) || (crc == 0) || (blocks == 0);
}
#endif

/* Ответ 2, "сейчас нельзя" (E122): причина, или NULL, если команду 19 можно выполнять. */
#ifndef HYBRID_CARD
const char *fw_start_refused(void)
{
  if (BOOT_VER < 0x30) return "OLD BOOT";
  if (boot_manual_busy()) return "BOOT BUSY";
  if (app_trial_active()) return "IN TRIAL";
  if (device.rebootToUpdate || device.FWinProgress || fw_rollback_end) return "BUSY";
  return NULL;
}
#endif

/* Идущая по этому каналу загрузка начата с этими полями. */
#ifndef HYBRID_CARD
uint8_t fw_same_download(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks, dataChanel_e cch)
{
  bootFWparam_s h;
  if (!device.FWinProgress || device.cchFWupdate != cch || blocks != device.FWblockCount) return 0;
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))) return 0;
  return h.firmwareVersion == number && h.buildDate == build &&
         h.firmwareSize == size && h.firmwareCRC == crc;
}
#endif

/* Номер 1: подготовить резервную копию для загрузчика и перезапуститься. Возможно, пока копия
 * цела, иначе ответ 2. */
#ifndef HYBRID_CARD
static cmdCode_e fw_rollback(dataChanel_e cch)
{
  bootFWparam_s fwParam;
  if (!mem_read(mem[MB_BACKUP].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))) return CMD_RUNERROR;
  if ((fwParam.buildDate == 0xffffffff) || (fwParam.firmwareSize == 0xffffffff) ||
      (fwParam.firmwareCRC == 0xffffffff) || (fwParam.firmwareVersion == 0xffff)){
    fw_log(LOG_FW_FAIL, "NO BACKUP");
    return CMD_RUNERROR;
  }
  if ((fwParam.firmwareSize > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) ||
      !user_checkFirmware(mem[MB_BACKUP].start + FWBLOCK_OFFSET, fwParam.firmwareSize, fwParam.firmwareCRC)){
    fw_log(LOG_FW_FAIL, "BAD BACKUP");
    return CMD_RUNERROR;
  }
  fw_rollback_pos = 0;
  fw_rollback_end = fwParam.firmwareSize + FWBLOCK_OFFSET;
  device.cchFWupdate = cch;
  device.status.readyToUpdate = 1;
  return CMD_SUCCESS;
}
#endif

/* Один фрагмент за проход: стирание не больше одного сектора, без нового буфера. */
#ifndef HYBRID_CARD
static void fw_rollback_poll(void)
{
  if (store_runtime) return;
  uint16_t n;
  bootFWparam_s h;
  if (!fw_rollback_end) return;
  n = fw_rollback_end - fw_rollback_pos > 128u ? 128u : (uint16_t)(fw_rollback_end - fw_rollback_pos);
  if ((fw_rollback_pos & 4095u) == 0 && !mem_erase_sector(mem[MB_FIRMWARE].start + fw_rollback_pos)) {
    fw_end("WRITE", 0); return;
  }
  if (!mem_read(mem[MB_BACKUP].start + fw_rollback_pos, bf, n)) { fw_end("READ", 0); return; }
  if (fw_rollback_pos == 0) memset(bf, 0xff, 4);
  if (!mem_write(mem[MB_FIRMWARE].start + fw_rollback_pos, bf, n)) { fw_end("WRITE", 0); return; }
  fw_rollback_pos += n;
  wdt_reload();
  if (fw_rollback_pos < fw_rollback_end) return;
  fw_rollback_end = 0;
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) ||
      !user_checkFirmware(mem[MB_FIRMWARE].start + FWBLOCK_OFFSET, h.firmwareSize, h.firmwareCRC)) {
    fw_end("CRC", 0); return;
  }
  fw_log(LOG_FW_ROLLBACK, NULL);
  device.rebootToUpdate = 1;
}
#endif

/* Команда 19 (E122): ответ 0 принято, 1 неверные поля, 2 сейчас нельзя. Номер с установленным
 * не сверяется: что ставить, решает сервер. Тот же файл во время его загрузки - загрузка
 * продолжается. */
#ifndef HYBRID_CARD
static cmdCode_e fw_start(const uint8_t *p, dataChanel_e cch)
{
  uint16_t number, blocks;
  uint32_t build, size, crc;
  const char *busy;
  bootFWparam_s fwParam;

  memcpy((uint8_t*)&number, &p[0], 2);
  memcpy((uint8_t*)&build, &p[2], 4);
  memcpy((uint8_t*)&size, &p[6], 4);
  memcpy((uint8_t*)&crc, &p[10], 4);
  memcpy((uint8_t*)&blocks, &p[14], 2);

  if (fw_start_rejected(number, build, size, crc, blocks)){
    fw_log(LOG_FW_FAIL, "PARAM");
    return CMD_PARAMERROR;
  }
  if (number != 1 && fw_same_download(number, build, size, crc, blocks, cch)){
    device.startWaitFWtime = 0; /* запросить недостающий блок сразу */
    return CMD_SUCCESS;
  }
  busy = fw_start_refused();
  if (busy != NULL){
    fw_log(LOG_FW_FAIL, busy);
    return CMD_RUNERROR;
  }
  if (number == 1) return fw_rollback(cch);

  /* Только первый сектор (заголовок и начало блока 0): остальные стирает fw_take_block. */
  if (!mem_erase_sector(mem[MB_FIRMWARE].start)) return CMD_RUNERROR;
  memset(&fwParam, 0xff, sizeof(fwParam));
  fwParam.hardwareVersion = device.hardwareVersion;
  fwParam.firmwareVersion = number;
  fwParam.firmwareSize = size;
  fwParam.firmwareCRC = crc;
  fwParam.buildDate = build;
  if (!mem_write(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))) return CMD_RUNERROR;
  wdt_reload();

  device.FWblockCount = blocks;
  device.FWblockIdx = 0;
  device.firmwarePercent = 0;
  device.startWaitFWtime = 0;
  device.cchFWupdate = cch;
  fw_write_errors = 0;
  fw_log(LOG_FW_START, NULL);
  /* Сервер: бит 11, блоки запрашивает контроллер, один срок на всю загрузку, всё переживает
     сброс. USB: блоки шлёт NextConfig. */
  if (cch != CH_USB){
    device.FWinProgress = 1;
    device.updateTimeout = FW_TIMEOUT_S;
    device.status.readyToUpdate = 1;
    user_saveindex();
  }
  return CMD_SUCCESS;
}
#endif

/* Блок читается обратно: оборванная или неудачная запись видна здесь (код 254). */
#ifndef HYBRID_CARD
static uint8_t fw_block_written(uint32_t adr, const uint8_t *data)
{
  uint8_t b[64];
  uint16_t off;
  for (off = 0; off < 1024u; off += sizeof(b)){
    if (!mem_read(adr + off, b, sizeof(b))) return 0;
    if (memcmp(b, data + off, sizeof(b)) != 0) return 0;
  }
  return 1;
}
#endif

/* Все блоки приняты. Исправный образ - перезапуск в загрузчик на установку (событие 3 перед
 * перезапуском); неверная CRC заканчивает загрузку. Бит 11 снимается в обоих случаях. */
#ifndef HYBRID_CARD
static uint8_t fw_blocks_done(uint8_t crc_ok)
{
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;
  if (crc_ok) device.rebootToUpdate = 1;
  else fw_log(LOG_FW_FAIL, "CRC");
  if (device.cchFWupdate != CH_USB) user_saveindex();
  return crc_ok ? FW_BLOCK_OK : FW_BLOCK_END;
}
#endif

/* Команда 20 (E96, E123, E477): номер блока и 1024 байта. Возвращает код команды 4: 0 принят,
 * 254 не прошёл чтение после записи, 255 неверная длина или номер (оба сразу запрашиваются
 * заново), 253 загрузка окончена. Команда 4 подтверждает каждую команду 20 (сервер снимает
 * команду только по подтверждению): запоздавший повтор уже записанного блока получает 0,
 * повторно ничего не пишется. */
#ifndef HYBRID_CARD
static uint8_t fw_take_block(const uint8_t *p, uint16_t n, dataChanel_e cch)
{
  uint16_t block;
  uint32_t adr;
  bootFWparam_s fwParam;

  if (boot_manual_busy() || cch != (dataChanel_e)device.cchFWupdate) return FW_BLOCK_END;
  if (n != 1026u) return device.FWblockIdx >= device.FWblockCount ? FW_BLOCK_END : FW_BLOCK_BADFRAME;
  memcpy((uint8_t*)&block, p, 2);
  if (block < device.FWblockIdx) return FW_BLOCK_OK;
  if (device.FWblockIdx >= device.FWblockCount) return FW_BLOCK_END;
  if (block != device.FWblockIdx) return FW_BLOCK_BADFRAME;
  if (block == 0 && fw_image_foreign(&p[2], device.hardwareVersion)){
    fw_abort(NULL);
    fw_log(LOG_FW_FAIL, "HW");
    return FW_BLOCK_END;
  }
  /* Сектор стирается перед первым блоком, который в него пишет (команда 19 стирает только
     первый): стирание всей области сразу останавливало цикл на секунды и рвало связь.
     Повтор блока стирает свой сектор снова - там только этот блок - и дописывает оборванное. */
  adr = mem[MB_FIRMWARE].start + FWBLOCK_OFFSET + (uint32_t)block * 1024u;
  if (store_runtime) return fw_async_begin(&p[2], adr, cch) ? FW_BLOCK_PENDING : FW_BLOCK_BADWRITE;
  uint32_t sector = (adr + 4095u) & ~4095u;
  bool written = (sector >= adr + 1024u || mem_erase_sector(sector)) && mem_write(adr, (uint8_t*)&p[2], 1024);
  wdt_reload();
  if (!written || !fw_block_written(adr, &p[2])){
    if (++fw_write_errors < FW_WRITE_TRIES) return FW_BLOCK_BADWRITE;
    fw_abort("WRITE");
    return FW_BLOCK_END;
  }
  fw_write_errors = 0;
  device.startWaitFWtime = 0; // Ожидание завершено только после проверенной записи блока.
  device.FWblockIdx++;
  device.firmwarePercent = (uint8_t)((uint32_t)device.FWblockIdx * 100u / device.FWblockCount);
  if (device.FWblockIdx < device.FWblockCount) return FW_BLOCK_OK;
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))) {
    fw_abort("READ");
    return FW_BLOCK_END;
  }
  return fw_blocks_done(user_checkFirmware(mem[MB_FIRMWARE].start + FWBLOCK_OFFSET, fwParam.firmwareSize, fwParam.firmwareCRC));
}
#endif

/* После отложенного стирания хвост содержит старый образ. Продолжаем с сохранённого
 * курсора: повтор уже записанных блоков безопасен для NOR, хвост стирается при записи. */
#ifndef HYBRID_CARD
static uint16_t fw_resume_block(void)
{
  return device.FWblockIdx < device.FWblockCount ? device.FWblockIdx : UINT16_MAX;
}
#endif

/* После сброса продолжить загрузку с сервера, если она шла (бит 11 переживает сброс, E440).
 * Подготовленный заголовок должен по-прежнему описывать обновление: стёртая метка, номер
 * прошивки от 2 и поля, которые приняла бы команда 19. Остаток времени сохраняется раз в
 * минуту. Без загрузки любой блок получает 253. */
#ifndef HYBRID_CARD
static void fw_resume_after_boot(void)
{
  bootFWparam_s h;
  if (!device.FWinProgress) {
    device.updateTimeout = 0;
    device.FWblockIdx = device.FWblockCount = 0;
    return;
  }
  if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) || BOOT_VER < 0x30 || h.appIdent != 0xFFFFFFFFu || h.firmwareVersion < 2 ||
      fw_start_rejected(h.firmwareVersion, h.buildDate, h.firmwareSize, h.firmwareCRC, device.FWblockCount)) {
    device.FWinProgress = 0;
    device.status.readyToUpdate = 0;
    device.updateTimeout = 0;
    device.FWblockIdx = device.FWblockCount = 0;
    user_saveindex();
    return;
  }
  device.cchFWupdate = CH_SERVER;
  device.FWblockIdx = fw_resume_block();
  if (device.FWblockIdx == UINT16_MAX) { fw_abort("READ"); return; }
  device.startWaitFWtime = 0;
  if (device.updateTimeout == 0 || device.updateTimeout > FW_TIMEOUT_S) device.updateTimeout = FW_TIMEOUT_S;
  device.status.readyToUpdate = 1;
  device.firmwarePercent = device.FWblockIdx * 100 / device.FWblockCount;
}
#endif

/* Источник команды 8 (E184) -> событие журнала: 1 мобильное приложение 52, 2 API 50, 3 тест из
 * админки 49, 4 голосовой помощник 51. Источники, которых нет в таблице, пишутся как API. */
#ifndef HYBRID_CARD
static logControl_e cmd8_event(uint8_t source)
{
  switch (source){
  case 1: return LOG_CON_OUTONBYAPP;
  case 3: return LOG_CON_OUTONBYTEST;
  case 4: return LOG_CON_OUTONBYVOICE;
  default: return LOG_CON_OUTONBYAPI;
  }
}
#endif
#ifdef HYBRID_CARD
extern logControl_e server_out_event;
#else
logControl_e server_out_event = LOG_CON_OUTONBYAPI;
#endif

// Разбор данных
/* Все ячейки проверяются до выделения памяти, изменения таймштампа и записи Flash.
 * Ошибка в хвосте пакета не должна сохранять его начало и отправлять второй ответ. */
#ifndef HYBRID_CARD
static bool sync_cells_valid(const uint8_t *ptr, uint16_t len, memBlock_e mb, uint16_t cell_size)
{
  uint16_t count, index;
  if (len < 8) return false;
  count = ptr[9];
  if (count == 0 || (uint32_t)(len - 8) != (uint32_t)count * cell_size) return false;
  for (uint16_t i = 0; i < count; i++){
    const uint8_t *cell = ptr + 10 + (uint32_t)i * cell_size;
    index = cell[0];
    if (mb == MB_ID) index |= (uint16_t)cell[1] << 8;
    if (index == 0 || index > mem[mb].size / cell_size) return false;
    if (mb == MB_ID && cell[12] == 0) return false;
  }
  return true;
}
#endif

#ifndef HYBRID_CARD
static bool command_args_valid(const uint8_t *ptr, uint16_t len)
{
  // Проверяем обязательные аргументы до любых действий команды.
  switch (ptr[4]){
    case 1:
      if (len < 4) return false;
      return ptr[5] != CMD_SUCCESS || len >= 8;
    case 2: case 3: case 18: case 105: case 108:
      return len >= 4;
    case 6: return len >= 6;
    case 23: case 102: return len >= 7;
    default: return true;
  }
}
#endif

/* >>> Сборка таблиц страницами: исходный сектор читается до commit журнала. */
typedef struct {
  const uint8_t *records;
  uint32_t sector, start;
  uint16_t count, size;
  uint8_t ids;
} sync_pages_s;

#ifndef ASM_RUNTIME
#ifndef HYBRID_CARD
static uint8_t sync_page(const void *context, uint16_t off, uint8_t *page)
{
  const sync_pages_s *s = context;
  if (!mem_read(s->sector + off, page, 256)) return 0;
  for (uint16_t i = 0; i < s->count; i++) {
    const uint8_t *record = s->records + (uint32_t)i * s->size;
    uint16_t index = record[0];
    if (s->ids) memcpy(&index, record, 2);
    uint32_t address = s->start + (uint32_t)(index - 1u) * s->size;
    if (address < s->sector + off || address >= s->sector + off + 256u) continue;
    uint16_t local = (uint16_t)(address - s->sector - off);
    if (s->size > 256u - local) return 0;
    if (s->ids) {
      cellID_s *old = (cellID_s*)(page + local);
      uint8_t first[4], done = old->reserve[6];
      bool keep = old->index != 0xffff && memcmp(old->ID, record + 2, 10) == 0;
      memcpy(first, old->reserve, 4);
      memcpy(old, record, s->size);
      if (keep) { memcpy(old->reserve, first, 4); old->reserve[6] = done; }
      id_set_pass_day(old, id_today());
      id_watch_end(id_time_end(old));
    } else memcpy(page + local, record, s->size);
  }
  return 1;
}
#endif
#endif

/* Номер последней записанной ячейки базы ID: граница поиска (user_findID). */
#ifdef HYBRID_CARD
extern uint16_t id_cells_end;
#else
uint16_t id_cells_end;
#endif
/* Сколько ID пакета команды 13 займут пустые ячейки (повторы номера считаются раз) и
 * наибольший номер. Читается по 2 байта на запись, а не вся база. UINT16_MAX - ошибка
 * чтения, тогда считаем по всей базе. */
#ifndef ASM_RUNTIME
#ifndef HYBRID_CARD
static uint16_t sync_new_ids(const uint8_t *records, uint16_t count, uint16_t *last, uint32_t *delta)
{
  uint16_t added = 0, index;
  for (uint16_t i = 0; i < count; i++){
    const uint8_t *r = records + (uint32_t)i * sizeof(cellID_s);
    uint8_t old[12];
    memcpy(&index, r, 2);
    if (index > *last) *last = index;
    /* Номер в индекс до записи ячейки: лишний номер безвреден, пропавший - нет. */
    (void)idindex_put(index, r + 2);
    /* Прежнее содержимое: последняя ранняя запись пакета с тем же номером или флеш. */
    int16_t j = (int16_t)i - 1;
    while (j >= 0 && memcmp(records + (uint32_t)j * sizeof(cellID_s), &index, 2)) j--;
    if (j >= 0) memcpy(old, records + (uint32_t)j * sizeof(cellID_s), 12);
    else if (!mem_read(mem[MB_ID].start + (uint32_t)(index - 1u) * sizeof(cellID_s), old, 12))
      return UINT16_MAX;
    if (old[0] == 0xff && old[1] == 0xff) { if (j < 0) added++; }
    else *delta ^= idindex_hash(index, old + 2);
    *delta ^= idindex_hash(index, r + 2);
  }
  return added;
}
#endif
#endif

#ifndef ASM_RUNTIME
#ifndef HYBRID_CARD
static bool sync_write_cells(const uint8_t *records, uint16_t count, memBlock_e mb, uint16_t size)
{
  while (count) {
    uint16_t index = records[0], n = 1;
    if (mb == MB_ID) memcpy(&index, records, 2);
    uint32_t sector = (mem[mb].start + (uint32_t)(index - 1u) * size) & ~4095u;
    while (n < count) {
      const uint8_t *next = records + (uint32_t)n * size;
      index = next[0];
      if (mb == MB_ID) memcpy(&index, next, 2);
      if (((mem[mb].start + (uint32_t)(index - 1u) * size) & ~4095u) != sector) break;
      n++;
    }
    sync_pages_s image = {records, sector, mem[mb].start, n, size, mb == MB_ID};
    if (!mem_replace_pages(sector, sync_page, &image)) return false;
    records += (uint32_t)n * size;
    count -= n;
  }
  return true;
}
#endif
#endif
/* <<< Сборка таблиц страницами */

/* >>> Праздники целиком: даты подряд с первой ячейки, остаток списка - 0xFF. */
typedef struct { const uint8_t *dates; uint16_t bytes; } holiday_pages_s;
#ifndef ASM_RUNTIME
#ifndef HYBRID_CARD
static uint8_t holiday_page(const void *context, uint16_t off, uint8_t *page)
{
  const holiday_pages_s *h = context;
  if (!mem_read(mem[MB_HOLIDAY].start + off, page, 256)) return 0;
  for (uint16_t i = 0; i < 256u && off + i < mem[MB_HOLIDAY].size; i++)
    page[i] = off + i < h->bytes ? h->dates[off + i] : 0xFFu;
  return 1;
}
#endif
#endif
#ifndef ASM_RUNTIME
#ifndef HYBRID_CARD
static bool holiday_write(const uint8_t *dates, uint8_t count)
{
  holiday_pages_s h = {dates, (uint16_t)(count * sizeof(cellHoliday_s))};
  return mem_replace_pages(mem[MB_HOLIDAY].start, holiday_page, &h);
}
#endif
#endif
/* <<< Праздники целиком */

#ifdef ASM_RUNTIME
#ifndef HYBRID_CARD
#include "table_runtime.inc"
#endif
#endif

#ifndef HYBRID_CARD
void user_parse_comand(uint8_t* ptr, dataChanel_e cch){
  
  uint16_t len, crc, i;
  //uint8_t cmd_timeout;
  uint8_t uniqId;
  uint32_t tmp32;

  len = ptr[1]; // длина
  len = (len<<8) | ptr[0];
  if (len < 3) return; // В неполном заголовке нет команды и uniqId.
  crc = ptr[len + 3];  // crc
  crc = (crc<<8) | ptr[len + 2];
  i = (cch == CH_SERVER && server_rx_verified(ptr)) ? crc : crc16_calc(ptr, len + 2);

  //cmd_timeout = ptr[2];
  uniqId = ptr[3];
  wdt_reload();  
  
  if (crc != i) // ошибка контрольной суммы
  {
    user_send_answer_comand(ptr[4], CMD_CRCERROR, cch, uniqId);
    return;
  }

  if (!command_args_valid(ptr, len)){
    user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
    return;
  }

  /* ПИН (команда 102) проверялся, но нигде не требовался: без него по USB стирались база ID
     и настройки. Теперь остальные команды по USB - только после верного ПИНа, иначе ответ 4
     «нужен пароль» (18.09.2026). */
  if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4])){
    user_send_answer_comand(ptr[4], CMD_GETPASSWORD, cch, uniqId);
    return;
  }

#ifdef ASM_RUNTIME
  if (store_runtime && !store_command_allowed(ptr[4])) {
    user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId); return;
  }
#endif
  switch (ptr[4]) // команда
  {
    case 1: // Ответ контроллеру на регистрацию
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd reg (cmd 1)\r\n"); 
#endif        
      if (cch != CH_SERVER) break; // ответ на регистрацию бывает только от сервера
      device.answerTimeout = 0;
      comm_server_registered(ptr[5] == CMD_SUCCESS);
      switch ((cmdCode_e)ptr[5]){
      case CMD_SUCCESS:
        memcpy((uint8_t*)&tmp32, &ptr[6], 4); // время сервера
        log_setTime(tmp32);
        device.registredOnServer = 1;
        if (app_trial_active()){
          app_trial_confirm();
          if (!app_trial_active()) fw_log(LOG_FW_DONE, NULL);
        } // прошивка дошла до сервера - обновление подтверждено
        /* Неудачная установка или проба до этого старта: только журнал, версию сервер видит в
           статусе 2 (E122). */
        if (app_boot_error_unreported()){
          char reason[11];
          app_boot_error_text(reason);
          fw_log(strcmp(reason, "TRIAL FAIL") == 0 ? LOG_FW_ROLLBACK : LOG_FW_FAIL, reason);
          app_boot_error_reported();
        }
        device.lmTimer = 5;
        device.lmCount = 0;
        {
          uint16_t st;
          memcpy((uint8_t*)&st, (uint8_t*)&device.status, 2);
          status_event_sent(st, timers_getMillis());
        }
        break;
        
      case CMD_PARAMERROR:
      case CMD_RUNERROR:
      case CMD_UNDEFRESULT:
        device.registredOnServer = 0;
        comm_channel_reopen(0);
        break;
        
      case REG_UNKNOWN_IMEI:
      case REG_NOT_BOUND:
        // пробная прошивка могла сама прочитать IMEI неверно: не подтверждаем и базу не стираем,
        // при повторении откат по таймауту вернёт прежнюю прошивку
        if (!app_trial_active())
          user_factoryReset();
        device.registredOnServer = 0;
        comm_channel_reopen(0);
        break;
       
      default:
        break;
      }
      break;

    case 2: // перезагрузить
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd reset (cmd 2)\r\n");
#endif
      /* Бит 11 (E440): перезагрузка отменяет загрузку; откат или установка, уже идущие в
         загрузчик, не трогаются. */
      if (device.rebootToUpdate){
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
      if (device.FWinProgress) fw_abort("CANCEL");
      if (ptr[5] == 0){
        /* Reset now = in 1-2 s by timeToReset (user_loop writes 33): a blocking pause kept the modem from sending answer 11. */
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        device.timeToReset = 2;
      } else{
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); // кода 5 в ответе 11 нет (строка 537)
        device.timeToReset = ptr[5];
      }
      break;
    
    case 3: // команда установить заводские настройки
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd factory (cmd 3)\r\n"); 
#endif 
      if (ptr[5] == 0){
        user_factoryReset();
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      } else{
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); // кода 5 в ответе 11 нет (строка 537)
        device.timeToDefault = ptr[5];
      }
      break;      

    case 5: // Установка состояния триггерных функций
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd triggers (cmd 5)\r\n"); 
#endif 
      if ((len - 3) == 16){
        memcpy((uint8_t*)&device.triggers, &ptr[5], 16);
        
        if (device.status.unblockHand != device.triggers.handUnBlock){
          if (device.triggers.handUnBlock)
            log_addSystemEvent(LOG_SYS_UNDLOCKHANDEN);
          else
            log_addSystemEvent(LOG_SYS_UNDLOCKHANDDIS);
        }
          
        if (device.status.unblockOnScene != device.triggers.sheduleUnBlock){
          if (device.triggers.sheduleUnBlock)
            log_addSystemEvent(LOG_SYS_UNBLOCKSHEDEN);
          else
            log_addSystemEvent(LOG_SYS_UNBLOCKSHEDDIS);
        }
        
        if (device.status.autoRegistration != device.triggers.autoregistration){
          if (device.triggers.autoregistration)
            log_addSystemEvent(LOG_SYS_AUTOREGEN);
          else
            log_addSystemEvent(LOG_SYS_AUTOREGDIS);      
        }
        
        if (device.status.openAnyUser != device.triggers.openAll){
          if (device.triggers.openAll)
            log_addSystemEvent(LOG_SYS_OPENALLEN);
          else
            log_addSystemEvent(LOG_SYS_OPENALLDIS);        
        }
            
        // дублируем в статусе
        device.status.unblockHand = device.triggers.handUnBlock;
        device.status.unblockOnScene = device.triggers.sheduleUnBlock;
        device.status.autoRegistration = device.triggers.autoregistration;
        device.status.openAnyUser = device.triggers.openAll;
        // сохранить состояние в памяти
        store_command_start(5, cch, uniqId);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      

    case 6: //  запрос ID с аппаратных считывателей
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get IDs (cmd 6)\r\n"); 
#endif
      if ((ptr[5] == 0) || (ptr[6] == 0) || ((ptr[6] == 3) && (ptr[7] == 0))){
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      else{
        device.timeToGetID = ptr[5];
        if (device.timeToGetID){
          device.sourceGetID = ptr[6];
          device.addressGetID = ptr[7];
        }
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);        
      }
      break;      

    case 7: // запрос фото с камеры
      user_send_answer_comand(7, len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR, cch, uniqId);
      break;

    case 8: // управление выходами
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd set out (cmd 8)\r\n"); 
#endif 
      if (fw_update_quiet()){ /* table E437, F108 */
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
      if (ptr[5] != 0 && (len - 3) >= 3){
        cmdCode_e res8 = CMD_SUCCESS;
        //ptr[5] - источник события, от него зависит номер события в журнале (E184)
        //ptr[8] - ID абонента для передачи контроллеру информации для какого ID нужно сделать декримент счетчика проходов в сутки.
        /* ID абонента есть только в полном пакете (13 байт); в коротком ID пустой,
           за концом пакета ничего не читается. */
        memset(tempStrBuffer, 0, 11);
        if ((len - 3) >= 13) strncpy(tempStrBuffer, (char*)&ptr[8], 10);
        if (ptr[6] > 1 || ptr[7] > 1){
          res8 = CMD_PARAMERROR;
        } else{
          /* Протокол2, строки 185-188: 1 - включить, 0 - выключить; источник задаёт только номер
             события в журнале (тест из админки - тоже). Управляются только выходы, которым
             настройки реле разрешают команды сервера (onCmdServer); включение - по режиму реле
             и счётчику проходов ID. */
          server_out_event = cmd8_event(ptr[5]);
          if (ptr[6] == 0 && config.rele[0].onCmdServer) out_load(OUT1, OUT_OFF, 0, 0, 0);
          if (ptr[7] == 0 && config.rele[1].onCmdServer) out_load(OUT2, OUT_OFF, 0, 0, 0);
          /* Запрошенный выход не включился (не разрешён, занят, кончились проходы) - ответ 2,
             а не «выполнено»: сервер должен знать, что дверь не открылась (18.09.2026). */
          if ((ptr[6] == 1 || ptr[7] == 1) &&
              !user_activateOut((ptr[6] == 1) && config.rele[0].onCmdServer,
                           (ptr[7] == 1) && config.rele[1].onCmdServer, CT_SERVER, tempStrBuffer))
            res8 = CMD_RUNERROR;
        }
        user_send_answer_comand(ptr[4], res8, cch, uniqId);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      

    case 9: // запрос журнала событий
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get log (cmd 9)\r\n"); 
#endif 
      user_send_logRecords(cch, uniqId);
      break;      

    case 10: // запрос ID добавленных через SMS или Авторегистрацию
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get IDs (cmd 10)\r\n"); 
#endif       
      user_send_tempIDs(cch, uniqId);
      break;      
      
    case 11: // отправить смс на заданный номер с заданным текстом
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd send sms (cmd 11)\r\n"); 
#endif 
      /* Номер занимает 10 байт. Текст ограничен длиной кадра, NUL на проводе необязателен.
       * Собственная копия не позволяет захватить CRC или соседний пакет в текст SMS. */
      if (len >= 14 && ptr[5] != 0 && ptr[15] != 0 &&
          memchr(config.countryCode, 0, sizeof(config.countryCode)) != NULL){
        uint16_t text_size = len - 13;
        char number[sizeof(config.countryCode) + 11];
        char *text = work_acquire(text_size + 1u, WORK_SMS);
        if (text == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        strcpy(number, config.countryCode);
        strncat(number, (char*)&ptr[5], 10);
        memcpy(text, &ptr[15], text_size);
        text[text_size] = 0;
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        user_sendsms(text, number);
        work_release(text, WORK_SMS);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;       
      
    case 12: // установка настроек контроллера
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd settings (cmd 12)\r\n"); 
#endif 
      if ((len - 3) == sizeof(config)){
        uint8_t* p = &ptr[5];

        /* Сервер 1 и код страны обязательны; сервер 2 может быть пустым, его использование
           задаёт useServer2. */
        if ((p[8] == 0) || (p[137] == 0)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        }
        else{
          uint32_t oldtime = device.datetimeConfig;
          uint16_t oldcrc = device.crc;
          uint8_t olddefault = device.status.defaultConfig;
          config_exchange(&ptr[5]);
          config_terminate_strings();
          device.datetimeConfig = config.datetime;
          device.status.defaultConfig = 0;
          store.channel = cch; store.uniq = uniqId; store.cmd = 12; store.quiet = 0;
          bool saved = store_config_start();
          config_exchange(&ptr[5]);
          device.datetimeConfig = oldtime;
          device.crc = oldcrc;
          device.status.defaultConfig = olddefault;
          if (!saved) user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        }
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 13: // отправить блок ID абонентов в контроллер
      {
        // определяем сектор, в который будем писать, читаем, стираем и пишем.
        if (!sync_cells_valid(ptr, len, MB_ID, sizeof(cellID_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
      if (!mem_tables_ready()) {
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
#ifdef ASM_RUNTIME
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
#else
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        {
        uint16_t last = id_cells_end;
        uint32_t delta = 0;
        uint16_t added = sync_new_ids(&ptr[10], ptr[9], &last, &delta);
        if (!sync_write_cells(&ptr[10], ptr[9], MB_ID, sizeof(cellID_s))) {
          device.numbersCount = user_getNumbersCount(); /* часть секторов могла записаться */
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        device.datetimeID = tmp32;
        sync_packet(SYNC_ID);
        /* Команда 13 только заполняет ячейки: счёт и конец базы - на ходу, без чтения 1,6 МБ. */
        if (added == UINT16_MAX) device.numbersCount = user_getNumbersCount();
        else {
          device.numbersCount += added; id_cells_end = last;
          idindex_commit(delta, last);
          if (id_used_bytes != UINT32_MAX && id_used_bytes < (uint32_t)last * sizeof(cellID_s))
            id_used_bytes = (uint32_t)last * sizeof(cellID_s);
        }
        }
        user_send_timeshtamp(SHTAMP_ID, device.datetimeID,
                           id_crc16(0, mem[MB_ID].size), cch, uniqId);
#endif

      }
      break;
      
    case 14: // синхро. расписаний
      {
        if (!sync_cells_valid(ptr, len, MB_SHEDULE, sizeof(cellShedule_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
      if (!mem_tables_ready()) {
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
#ifdef ASM_RUNTIME
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
#else
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        if (!sync_write_cells(&ptr[10], ptr[9], MB_SHEDULE, sizeof(cellShedule_s))) {
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        device.datetimeShedule = tmp32;
        sync_packet(SYNC_SHEDULE);
        user_send_timeshtamp(SHTAMP_SHEDULE, device.datetimeShedule,
                           user_calcCRC16InFlash(mem[MB_SHEDULE].start, mem[MB_SHEDULE].size), cch, uniqId);

#endif

      }      
      break;
      
    case 15: // синхро групп 
      {
        if (!sync_cells_valid(ptr, len, MB_GROUP, sizeof(cellGroup_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
      if (!mem_tables_ready()) {
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
#ifdef ASM_RUNTIME
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
#else
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        if (!sync_write_cells(&ptr[10], ptr[9], MB_GROUP, sizeof(cellGroup_s))) {
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        device.datetimeGroup = tmp32;
        sync_packet(SYNC_GROUP);
        user_send_timeshtamp(SHTAMP_GROUP, device.datetimeGroup,
                           user_calcCRC16InFlash(mem[MB_GROUP].start, mem[MB_GROUP].size), cch, uniqId);

#endif

      }
      break;
      
    case 16: // синхро. праздников
      {
        /* Праздники - список дат без номера ячейки (Протокол2, строки 359-362), синхронизируются
           целиком. Раньше номером ячейки считался младший байт даты: полночь отвергалась. */
        if (len < 8 || ptr[9] == 0 || (uint32_t)(len - 8) != (uint32_t)ptr[9] * sizeof(cellHoliday_s)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
      if (!mem_tables_ready()) {
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
#ifdef ASM_RUNTIME
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
#else
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        if (!holiday_write(&ptr[10], ptr[9])) {
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        device.datetimeHoliday = tmp32;
        sync_packet(SYNC_HOLIDAY);
        user_send_timeshtamp(SHTAMP_HOLIDAY, device.datetimeHoliday,
                           user_calcCRC16InFlash(mem[MB_HOLIDAY].start, mem[MB_HOLIDAY].size), cch, uniqId);

#endif

      }
      break;
          
    case 17: // CRC всех 5-ти блоков данных
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get crc (cmd 17)\r\n"); 
#endif 
      user_send_crcPack(cch, uniqId);
      break;      
      
    case 18: // запрос CRC базы абонентов поблочно
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd crc ID (cmd 18)\r\n"); 
#endif       
      if (ptr[5] > 100) { // блоков 100: номер больше - чтение мимо базы
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        break;
      }
      user_send_crcMemoryID(ptr[5], cch, uniqId);
      break;      
      
    case 19: // Обновление прошивки
#ifdef DEBUG_USER
  if (cch != CH_USB)
    user_debug(">>U Recv cmd fw update (cmd 19)\r\n");
#endif
      if ((len - 3) == 16){
        if (fw_service_begin(ptr+5,cch,uniqId)) break;
        cmdCode_e started = fw_start(&ptr[5], cch);
        if (started == CMD_SUCCESS && store_save_requested) store_command_start(19, cch, uniqId);
        else user_send_answer_comand(ptr[4], started, cch, uniqId);
      } else{
        fw_log(LOG_FW_FAIL, "PARAM");
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 20: // Блок прошивки, ответ командой 4
#ifdef DEBUG_USER
if (cch != CH_USB && len >= 5){
  sprintf(tempStrBuffer, ">>U Recv FW block %u (cmd 20)\r\n", (unsigned)(ptr[5] | (ptr[6] << 8)));
  user_debug(tempStrBuffer);
}
#endif
      {
        uint8_t code = fw_take_block(&ptr[5], (uint16_t)(len - 3), cch);
        if (code == FW_BLOCK_PENDING) fw_async_reply(uniqId);
        else user_send_statusUpdateFW(ptr[4], code, cch, uniqId);
      }
      break;

    case 21: // запрос фотографий 
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get photo (cmd 21)\r\n"); 
#endif 
      if (len == 3 || len == 9)

#ifdef ASM_RUNTIME
        photo_request_answer(cch,uniqId,ptr+5,(uint8_t)(len-3));
#else
        user_send_answer_comand(21, photo_request(cch, uniqId, ptr + 5, (uint8_t)(len - 3)), cch, uniqId);
#endif
      else user_send_answer_comand(21, CMD_PARAMERROR, cch, uniqId);
      break;      
      
    case 22: // запрос статуса 2
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get status (cmd 22)\r\n"); 
#endif 
      user_send_statusPackSecond(cch, uniqId);
      break;      
      
    case 23: // установить время контроллера
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd set time (cmd 23)\r\n"); 
#endif 
      memcpy((uint8_t*)&tmp32, &ptr[5], 4);
      log_setTime(tmp32);
      user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); 
      break;      
       
    case 24: // команда подтверждения приема команды от контроллера
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd answer (cmd 24)\r\n"); 
#endif 
      /* Ответ на статус 1 заканчивает ожидание и счёт пропущенных ответов. ptr[5] - уникальный
         номер подтверждаемого сообщения, а не команда: он не должен заканчивать ожидание ответа
         на регистрацию. */
      if (cch == CH_SERVER) comm_server_status_ack(ptr[5]);
      if (device.answerID == 2){
        device.answerTimeout = 0;
        device.lmCount = 0;
      }
      break;      

    case 27: // произвольная команда
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd (cmd 27)\r\n"); 
#endif 
      user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      break;      

    case 100: // идентификация для конфигуратора
      if (cch == CH_USB)
        user_send_identPackToConfigurator(CH_USB);
      break;
      
    case 101: // статус для конфигуратора
      if (cch == CH_USB)
        user_send_statusPackToConfigurator(CH_USB);
      break;
      
    case 102: // проверка ПИН кода
      if (cch == CH_USB){
        if (*(uint32_t*)&ptr[5] == config.pinCode){
          device.userAutorized = 1;
        } else{
          device.userAutorized = 0;
        }
        user_send_anyData(102, &device.userAutorized, 1, cch, uniqId);
      }
      break;
      
    case 103: // очистить память идентификаторов
      if (cch==CH_USB && !idclear_runtime_begin(cch,uniqId))
        user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
      break;

    /*case 104: // информация о привязке к объекту
      if (cch == CH_USB){
        user_send_getInfoObject();
        user_send_anyData(104, NULL, 0, cch, uniqId);
      }
      break;*/
      
    case 105: // задать тип модуля
      if (cch == CH_USB){
        user_send_anyData(105, NULL, 0, cch, uniqId);
        device.moduleType = ptr[5];
        user_savereset(); 
      }
      break;
       
    case 106: // считать конфигурацию
      if (cch == CH_USB)
        user_send_configToConfigurator(CH_USB);
      break;
      
    case 107: // режим прозрачной передачи gsm
      user_send_anyData(107, NULL, 0, cch, uniqId);
      break;
      
    case USB_DIAG_COMMAND:
      if (cch == CH_USB)
        user_send_usbDiag(uniqId);
      break;
    case USB_COMM_DIAG_COMMAND:
      if (cch == CH_USB) { user_send_commDiag(uniqId); prof_send(uniqId); }
      break;

    case MANUAL_BOOT_COMMAND: { /* Файл загрузчика вручную, только по USB. */
      uint8_t response[8];
      if (cch != CH_USB || len < 4) {
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        break;
      }
      if (device.FWinProgress || device.rebootToUpdate || app_trial_active()) {
        user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        break;
      }
      boot_manual_command(1, &ptr[5], (uint16_t)(len-3), response);
      user_send_anyData(MANUAL_BOOT_COMMAND, response, sizeof(response), cch, uniqId);
      if (response[0] == 2 && response[1] == MANUAL_OK) {
        /* Дать USB отправить подтверждение. Успех проверяется после перезапуска. */
        delay_ms(200);
        response[1] = (uint8_t)boot_manual_apply();
        user_send_anyData(MANUAL_BOOT_COMMAND, response, sizeof(response), cch, uniqId);
      }
      break;
    }

    case 108: // включить лог
      user_send_anyData(108, NULL, 0, cch, uniqId);
      if (cch == CH_USB)
        device.logBits = ptr[5];
      break;
      
    default: // команда не поддерживается 
      user_send_answer_comand(ptr[4], CMD_UNDEFINE, cch, uniqId);
      break;
  }
}
#endif

// индикация
#ifndef HYBRID_CARD
void user_indication (void)
{
  // NET LED: на сервере - горит, модем в сети - мигает, выходит на связь - мигает часто
  if (comm_server_online()) led_load(NET_LED, LED_ON);
  else if (modem_slm336_ready()) led_load(NET_LED, LED_FL1);
  else led_load(NET_LED, LED_FL2);
}
#endif

// опрос входов
#ifndef HYBRID_CARD
void user_readinputs (void){
  // при отказе АЦП сохраняем последние состояния входов
  if (!adc_startInjectedConvertion()) return;
  
  // дискретное состояние входов
  device.status.stateIn1 = (adc_getinjectedvalue_mV(IN1_CHANEL) > 3000)?1:0;
  device.status.stateIn2 = (adc_getinjectedvalue_mV(IN2_CHANEL) > 3000)?1:0;
  
  // вход 1
 
  if (device.laststatus.stateIn1 != device.status.stateIn1){
    if (config.inputMode[0]){
      switch (config.inputMode[0]){
        case 1:
          //777
          break;

        case 2:
          break;
      }
    }
    if (device.status.stateIn1){
      log_addSystemEvent(LOG_SYS_IN1ON);
    }
    else
      log_addSystemEvent(LOG_SYS_IN1OFF);
    device.laststatus.stateIn1 = device.status.stateIn1;
  }
  
  // вход 2
  if (device.laststatus.stateIn2 != device.status.stateIn2){
    if (config.inputMode[1]){  
      switch (config.inputMode[1]){
        case 1:
          //777
          break;

        case 2:
          break;
      }
    }
    if (device.status.stateIn2)
      log_addSystemEvent(LOG_SYS_IN2ON);
    else
      log_addSystemEvent(LOG_SYS_IN2OFF);
    device.laststatus.stateIn2 = device.status.stateIn2;
  }
}
#endif

// вернуть указатель на 10ть последних цифр номера при номере более 10
#ifndef HYBRID_CARD
char* user_last10digitsnumber (char* num)
{
  uint16_t l = strlen(num);

  return (l <= 10)?num:(num + (l-10));
}
#endif

// узнать текущий день недели 
// 0 - Пн, 1 - Вт ...
#ifndef HYBRID_CARD
uint8_t user_getweakday (uint32_t t)
{
  // какой сейчас день по счету
  uint32_t d = t / 86400u;
  uint8_t wd = d % 7u; // день недели, 0 - Чт

  // преобразуем
  wd += 3;
  if (wd > 6) wd -= 7;

  return wd;
}
#endif


// ********************** Работа с ID **************************

/* Команда 13 пишет ячейку на место её номера, поэтому в базе бывают пропуски. Поиск идёт до
 * последней записанной ячейки (считается при старте и после каждой синхронизации) и пропускает
 * пустые. */

// поиск ID в памяти, если есть то возвращаются параметры ячейки
uint8_t user_findID(char *id, cellID_s *cell){

  uint32_t adr;
  uint16_t idsum = 0;
  for (uint8_t i = 0; i < 10; i++) idsum += (uint8_t)id[i];
  if (idsum == 2550 || idsum == 0) return false; // пустой ID в базе не ищется
  {
    cellID_s found;
    int8_t r = idindex_find(mem[MB_ID].start, (const uint8_t*)id, (uint8_t*)&found);
    if (r >= 0) {
      if (r) memcpy(cell, &found, sizeof(found));
      return r == 1;
    }
  }
  /* Индекс не готов (пересборка) - перебор по базе. */
  uint32_t endAdr = mem[MB_ID].start + (uint32_t)id_cells_end * sizeof(cellID_s);
  uint8_t tbuffer[1024];
  cellID_s *c;

  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    if (!mem_read(adr, tbuffer, 1024)) return 2;
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024 && adr + a < endAdr; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff) continue;
      /* The query is already nonempty: an equal stored ID is nonempty too. */
      if (memcmp(c->ID, id, 10) == 0){
        memcpy(cell, c, sizeof(cellID_s));
        return true;
      }
    }
  }
  // не нашли  
  return false;
}

// поиск ID во временном буфере
#ifndef HYBRID_CARD
bool user_findTempID(char *id){
  
  uint32_t adr;
  //uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;
  
  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;
  
  // проверим, что такого ID нет в буфере
  for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return false;
    // такой ID есть
    if (memcmp(cellId.ID, id, 10) == 0){
      return true;
    }
  }
  // не нашли  
  return false;
}
#endif

// добавляет ID во временный буфер
#ifndef HYBRID_CARD
void user_addIdToTempBuffer(char *id, codeType_e tp){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;
  
  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;
  
  // проверим, что такого ID нет в буфере
  for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return;
    // такой ID есть
    if (memcmp(cellId.ID, id, 10) == 0){
      return;
    }
  }
  
  // по странично 
  for (adr = startAddress; adr < endAdr; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return;
    // ячейка пустая, она должна быть стерта
    if (cellId.index == 0xffff){
      memset(&cellId, 0, sizeof(cellId)); // новая запись без чужих флагов и резерва
      memcpy(cellId.ID, id, 10);
      cellId.group = 0;
      cellId.blockTime = 0;
      switch (tp){
        case CT_PHONE: cellId.isPhone = 1; break;
        case CT_RFID: cellId.isRFID = 1; break;
        case CT_AUTO: cellId.isAuto = 1; break;
        case CT_RF433: cellId.isRF433 = 1; break;
        case CT_WIEGAND: cellId.isWiegand = 1; break;
        default: break;
      }
      if (!mem_write(adr, (uint8_t*)&cellId, sizeof(cellID_s))) return;
      device.newAddressTempID = adr + sizeof(cellID_s);
      device.status.readyID = 1;
      return;
    }
  }
}
#endif

/* Окно расписания в минутах суток: 08:30-17:15 включает 09:00, окно через полночь (22:00-06:00)
 * переходит через ноль. */
static bool shedule_time_ok(const cellShedule_s *s)
{
  uint16_t now = (uint16_t)device.hour * 60u + device.minute;
  uint16_t start = (uint16_t)s->startHour * 60u + s->startMinute;
  uint16_t stop = (uint16_t)s->stopHour * 60u + s->stopMinute;
  return start <= stop ? (now >= start && now <= stop) : (now >= start || now <= stop);
}

// проверка ID по параметрам и управление выходом
// через нее проходят все ID
void user_checkIdAndControl(char *id, codeType_e tp){
  if (!mem_tables_ready()) return;
#ifdef DEBUG_USER
    switch (tp)
    {
    case CT_PHONE: user_debug(">>U Check ID: Phone\r\n"); break;
    case CT_WIEGAND: user_debug(">>U Check ID: Wiegand\r\n"); break;
    case CT_RF433: user_debug(">>U Check ID: RF433\r\n"); break;
    case CT_RFID: user_debug(">>U Check ID: RFID\r\n"); break;
    case CT_AUTO: user_debug(">>U Check ID: AUTO\r\n"); break;
    case CT_SERVER: user_debug(">>U Check ID: Server\r\n"); break;
    }
#endif  
  
  // проверяем что блокировка не включена
  if (device.status.unblockHand)
  {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Unblock hand\r\n"); 
#endif     
    return; // ручная разблокировка включена
  }
  if (device.status.unblockOnScene)
  {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Unblock on scene\r\n"); 
#endif    
    return; // разблокировка по сценарию включена
  }

  bool onOut[2] = {false, false};

  // проверим можно ли управлять этим способом: настройки реле действуют для любого пути открытия
  for (uint8_t i = 0; i < 2; i++){
    if (config.rele[i].useInBlock == 0){
      switch (tp){
      case CT_PHONE:
        onOut[i] = true;
        break;
      case CT_WIEGAND:
        if (config.rele[i].onWiegand) onOut[i] = true;
        break;
      case CT_RF433:
        if (config.rele[i].onRF433) onOut[i] = true;
        break;
      case CT_RFID:
        if (config.rele[i].onRFID) onOut[i] = true;
        break;
      case CT_AUTO:
        if (config.rele[i].onCamera) onOut[i] = true;
        break;
      case CT_SERVER:
        if (config.rele[i].onCmdServer) onOut[i] = true;
        break;
      default:
        if (config.rele[i].onOther) onOut[i] = true;
        break;
      }
    }
  }

  // проверяем ID из временного буфера
  if (user_findTempID(id)){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Open by temp ID\r\n"); 
#endif
    user_activateOut(onOut[0], onOut[1], tp, id);
    return;
  }
  
  // авторегистрация
  if (device.status.autoRegistration){
    user_addIdToTempBuffer(id, tp);
    log_addControlEvent(LOG_CON_IDADDBYAUTOREG, id, 0);
  }
  
  // открывать всем без разбора
  if (device.status.openAnyUser){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Open any user\r\n"); 
#endif
    user_activateOut(onOut[0], onOut[1], tp, id);
    return;
  }

  // открывать всем по расписанию
  if (device.triggers.openAllByShedule){
    // проверяем рассписание
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Open all by schedule\r\n"); 
#endif
    // проходим по расписанию
    if (user_checkShedule(device.triggers.openAllByShedule) == true){
      user_activateOut(onOut[0], onOut[1], tp, id);
    }
    return;
  }
  
  // можно, ищем ID
  if (onOut[0] || onOut[1]){
    cellID_s cellId;
    
    // ID найден
    uint8_t found=user_findID(id,&cellId);
    if(found>1) return;
    if (found){
      // проверим разрешение по дате запрета
      if ((cellId.blockTime != 0) && (cellId.blockTime < device.datetime))
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Blocked on date\r\n"); 
#endif         
        return;
      }
      // время действия истекло (строка 284): не открываем; об окончании уже сообщило событие 138
      if (id_expired(&cellId)){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Use time expired\r\n");
#endif
        return;
      }
            
      cellGroup_s group;
          
      // принадлежность к группе
      if (user_getGroup(cellId.group, &group) == false)
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Undefine group\r\n"); 
#endif 
        return;
      }
      
      // включена блокировка группы
      if (group.blocked){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Group is blocked\r\n"); 
#endif 
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0);
        return;
      }
      
      // разрешение на управление данным типом
      switch (tp){
      case CT_PHONE:
        if (group.onPhone == 0) return;
        break;
      case CT_WIEGAND:
        if (group.onWiegand == 0) return;
        break;
      case CT_RF433:
        if (group.onRF433 == 0) return;
        break;
      case CT_RFID:
        if (group.onRFID == 0) return;
        break;
      case CT_AUTO:
        if (group.onAuto == 0) return;
        break;
      default:
        break;
      }
          
      // расписание не задано, круглосуточно - управляем
      if (group.sheduleNumber == 0){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Any time\r\n"); 
#endif 
        user_activateOut(onOut[0], onOut[1], tp, id);
        return;
      }
      
      cellShedule_s shedule;
            
      // расписание
      if (user_getShedule(group.sheduleNumber, &shedule) == false)
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Undefine schedule\r\n"); 
#endif         
        return;
      }
      
      // включена блокировка расписания
      if (shedule.blocked){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Shedule is blocked\r\n"); 
#endif 
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0);
        return;
      }
      
      // проверим день
      if (shedule.day[device.day] == 0)
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Undefine day\r\n"); 
#endif 
        return;
      }
      
      bool flag = false;
      
      // проверим время
      flag = shedule_time_ok(&shedule);
      
      // не прошли по времени
      if (flag == false)
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Disabled by time\r\n"); 
#endif         
        return;
      }
      
      // проверим праздничный день
      flag = false;
      switch (shedule.onHoliday){
      case 0: // 0 - Резерв
        
        break;
      case 1: // 1 - Игнорировать праздничные дни
        flag = true;
        break;
      case 2: // 2 - Выполнять сценарии только в праздничные дни
        flag = user_dayIsHoliday()==1;
        break;
      case 3: // 3 - Выполнять сценарии во все дни, кроме праздничных
        flag = !user_dayIsHoliday();
        break;
      }
      
      // не прошли по празднику
      if (flag == false)
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Not check holiday\r\n"); 
#endif         
        return;
      }
      
      user_activateOut(onOut[0], onOut[1], tp, id);
    } else{
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Not founded\r\n"); 
#endif
      // ID не найден - включено информирование админа
      log_addControlEvent(LOG_CON_IDNOTINBASE, id, 0);
      if (device.triggers.smsUnreg){
        unreg_sms_note(id);
      }
    }
  }
  else
  {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Control out disabled to current type\r\n"); 
#endif
  }
}

/* Один проход ID открывает запрошенные выходы по их настройкам. Реле для разблокировки и уже
 * работающие пропускаются; счётчик проходов уменьшается и в журнал пишется одна запись на
 * проход, а не на каждое реле (Д-07). */

/* Возвращает true, если включён хотя бы один выход. */
bool user_activateOut(bool out1, bool out2, codeType_e tp, char *id){

  bool run[2];
  uint8_t out;

  run[0] = out1 && !config.rele[0].useInBlock && !out_getmodestate(0);
  run[1] = out2 && !config.rele[1].useInBlock && !out_getmodestate(1);
  if (!run[0] && !run[1])
  {
#ifdef DEBUG_USER
    user_debug(">>U Act Out: No output to switch\r\n");
#endif
    return false;
  }

  // проверим счетчик проходов
  uint8_t cnt = 255;
  bool started = false;
  cellID_s cellId;
  /* Проходы считаются только по заданному ID. Команда 8 без ID (короткий пакет) открывает
     по настройкам реле: пустой ID совпадал с ячейкой базы, где ID из нулей, и по её счётчику
     команда молча не выполнялась или списывала проходы чужому абоненту (18.09.2026). */
  uint8_t found=(id!=NULL && id[0]!=0)?user_findID(id,&cellId):0;
  if(found>1) return false;
  if (found){
    bool save = false;
    // задано кол-во проходов
    if (cellId.cntPassOnDay != 255){
      if (id_new_day(&cellId)) save = true;
      // есть проходы
      if (cellId.cntPass != 0){
        cellId.cntPass -= 1;
        cnt = cellId.cntPass;
        save = true;
      }
      else{
        if (save) user_updateID(&cellId); // день отмечен: завтра остаток восстановится
        return false;
      }
    }
    // первый проход ID со временем действия - начало отсчёта
    if (cellId.useTime != 0 && id_first_pass(&cellId) == 0 && device.datetime >= ID_TIME_VALID){
      id_set_first_pass(&cellId, device.datetime);
      id_watch_end(id_time_end(&cellId));
      started = save = true;
    }
    if (save && !user_updateID(&cellId)) return false;
  }

  /* Реле - сразу после решения (счётчик проходов уже сохранён, B25), журнал - после:
     запись событий не задерживает щелчок. */
  for (out = 0; out < 2; out++){
    if (!run[out]) continue;
#ifdef DEBUG_USER
    user_debug(out == 0 ? ">>U Act Out: Out 1\r\n" : ">>U Act Out: Out 2\r\n");
#endif
    // импульс
    if (config.rele[out].mode == 0){
      out_load(out, OUT_IMP, config.rele[out].tImpulse, config.rele[out].tPause, config.rele[out].cnt);
    } else{
      // уровень: проход ID переключает выход, команда сервера 8 включает (строка 186)
      out_load(out, (tp == CT_SERVER) ? OUT_ON : OUT_INV, 0, 0, 0);
    }
  }
  switch (tp){
  case CT_PHONE:
    log_addControlEvent(LOG_CON_OUTONBYPHONE, id, cnt);
    break;
  case CT_WIEGAND:
    log_addControlEvent(LOG_CON_OUTONBYWIEGAND, id, cnt);
    break;
  case CT_RF433:
    log_addControlEvent(LOG_CON_OUTONBYRF433, id, cnt);
    break;
  case CT_RFID:
    log_addControlEvent(LOG_CON_OUTONBYRFID, id, cnt);
    break;
  case CT_AUTO:
    log_addControlEvent(LOG_CON_OUTONBYAVTO, id, cnt);
    break;
  case CT_SERVER:
    log_addControlEvent(server_out_event, id, cnt);
    break;
  default:
    break;
  }
  // проходы ограничены: сервер раздаёт остаток остальным контроллерам объекта
  if (cnt != 255) log_addControlEvent(LOG_CON_PASSCOUNT, id, cnt);
  if (cnt == 0) log_addControlEvent(LOG_CON_PASSLIMIT, id, 0); // 79: последний проход суток
  if (started) log_addControlEvent(LOG_CON_IDTIMESTART, id, 255);
  return true;
}

// считать параметры группы
#ifndef HYBRID_CARD
bool user_getGroup(uint16_t num, cellGroup_s *group){
uint32_t offset;
  if(!num) return false;
  offset=(uint32_t)(num-1)*sizeof(cellGroup_s);
  if(offset+sizeof(cellGroup_s)>mem[MB_GROUP].size) return false;
  return mem_read(mem[MB_GROUP].start+offset,(uint8_t *)group,sizeof(cellGroup_s)) && *(uint8_t *)group==num;
}
#endif

// считать параметры расписания
#ifndef HYBRID_CARD
bool user_getShedule(uint16_t num, cellShedule_s *shedule){
uint32_t offset;
  if(!num) return false;
  offset=(uint32_t)(num-1)*sizeof(cellShedule_s);
  if(offset+sizeof(cellShedule_s)>mem[MB_SHEDULE].size) return false;
  return mem_read(mem[MB_SHEDULE].start+offset,(uint8_t *)shedule,sizeof(cellShedule_s)) && *(uint8_t *)shedule==num;
}
#endif

// проверить праздничный день
#ifndef HYBRID_CARD
uint8_t user_dayIsHoliday(void){
uint8_t data[64];
  uint32_t day=device.datetime/86400u;
  for(uint32_t offset=0;offset<1024;offset+=64) {
    if(!mem_read(mem[MB_HOLIDAY].start+offset,data,64)) return 2;
    for(unsigned i=0;i<64;i+=sizeof(cellHoliday_s)) {
      uint32_t value; memcpy(&value,data+i,4); if(value/86400u==day) return 1;
    }
  }
  return 0;
}
#endif

// очистить буфер временных ID
#ifndef HYBRID_CARD
bool user_clearTempIdBuffer(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  
  // по секторно стираем
  for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 4096){
    if (!mem_erase_sector(adr)) return false;
    wdt_reload();
  }
  
  device.status.readyID = 0;
  device.tempIDsCount = 0;
  upload_reset(9);
  device.newAddressTempID = mem[MB_TEMPID].start;
  return true;
}
#endif

// сколько номеров в памяти
/* Записанные ячейки по всей области, с пропусками; заодно находит конец базы для user_findID. */
#ifndef HYBRID_CARD
uint16_t user_getNumbersCount(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  uint16_t cnt = 0, cell = 0, used = 0;
  uint32_t sum = 0;

  id_cells_end = 0;
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      cell++;
      /* Последняя ячейка, где есть хоть один записанный байт: за ней флеш стёрта. */
      for (uint8_t b = 0; b < sizeof(cellID_s); b++)
        if (tbuffer[a + b] != 0xff) { used = cell; break; }
      // пустая ячейка
      if (c->index == 0xffff) continue;

      cnt++;
      id_cells_end = cell;
      sum ^= idindex_hash(cell, (const uint8_t*)c->ID);
    }
    wdt_reload();
  }
  id_used_bytes = (uint32_t)used * sizeof(cellID_s);
  idindex_scanned(sum, id_cells_end);

  return cnt;
}
#endif

/* Окончание времени действия ID (Протокол2, строка 284): событие 138, если время вышло в сутки
 * первого прохода. Окончание прошлых суток (контроллер был выключен) не сообщается - такой ID
 * уже удаляет сервер. */
#ifndef HYBRID_CARD
void user_checkIdTimeEnd(void){
expiry_runtime_begin();
}
#endif

// получаем кол-во ID во временном буфере и тут же обновляем адрес записи
#ifndef HYBRID_CARD
uint16_t user_getTempIDRecordsCount(void){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  uint16_t cnt = 0;
  
  // по странично читаем
  for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 1024){
    if (!mem_read(adr, tbuffer, 1024)) return 0xFFFFu;
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff){
        device.newAddressTempID = adr + a;
        return cnt;
      }
      
      cnt++;
    }
  }
  
  device.newAddressTempID = endAdr;
  return cnt;
}
#endif

// проверить расписание
#ifndef HYBRID_CARD
bool user_checkShedule(uint8_t shd){
  if (!mem_tables_ready()) return false;
  cellShedule_s shedule;
            
  // расписание
  if (user_getShedule(shd, &shedule) == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Undefine schedule\r\n"); 
#endif         
    return false;
  }
      
  // включена блокировка расписания
  if (shedule.blocked){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Shedule is blocked\r\n"); 
#endif 
    return false;
  }
      
  // проверим день
  if (shedule.day[device.day] == 0)
  {
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Undefine day\r\n"); 
#endif 
    return false;
  }
      
  bool flag = false;
  
  // проверим время
  flag = shedule_time_ok(&shedule);
      
  // не прошли по времени
  if (flag == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Disabled by time\r\n"); 
#endif         
    return false;
  }
      
  // проверим праздничный день
  flag = false;
  switch (shedule.onHoliday){
  case 0: // 0 - Резерв
    
    break;
  case 1: // 1 - Игнорировать праздничные дни
    flag = true;
    break;
  case 2: // 2 - Выполнять сценарии только в праздничные дни
    flag = user_dayIsHoliday()==1;
    break;
  case 3: // 3 - Выполнять сценарии во все дни, кроме праздничных
    flag = !user_dayIsHoliday();
    break;
  }
      
  // не прошли по празднику
  if (flag == false){
#ifdef DEBUG_USER
    user_debug(">>U Check Shedule: Not check holiday\r\n"); 
#endif         
    return false;
  }
      
  return true;
}
#endif

// обновить данные ID
#ifndef HYBRID_CARD
bool user_updateID(cellID_s *c){
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint16_t index;
  /* Команда 13 кладёт ячейку на место её номера: сначала проверяем это место,
     перебор - только если там другая ячейка. */
  if (c->index != 0 && c->index != 0xffff){
    uint32_t adr = mem[MB_ID].start + (uint32_t)(c->index - 1u) * sizeof(cellID_s);
    if (adr + sizeof(cellID_s) <= endAdr){
      if (!mem_read(adr, (uint8_t*)&index, 2)) return false;
      if (index == c->index) return mem_patch(adr, (const uint8_t*)c, sizeof(cellID_s));
    }
  }
  for (uint32_t adr = mem[MB_ID].start; adr + sizeof(cellID_s) <= endAdr; adr += sizeof(cellID_s)){
    if (!mem_read(adr, (uint8_t*)&index, 2)) return false;
    if (index == c->index){
      return mem_patch(adr, (const uint8_t*)c, sizeof(cellID_s));
    }
  }
  return false;
}
#endif

// ********************** Работа с log **************************

// очистить буфер журнала
#ifndef HYBRID_CARD
bool log_clearLogBuffer(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;

  /* Стираются только сектора с записями: весь буфер - 24 сектора и около
     секунды простоя главного цикла, а выгрузка журнала идёт часто. Сектора
     считаются от выровненного начала, чтобы не пропустить сектор на стыке. */
  if (device.newAddressLog > mem[MB_LOG].start && device.newAddressLog < endAdr)
    endAdr = device.newAddressLog;
  // по секторно стираем
  for (adr = mem[MB_LOG].start & ~(uint32_t)4095; adr < endAdr; adr += 4096){
    if (!mem_erase_sector(adr)) return false;
    wdt_reload();
  }
  
  device.readLogIDIndex = 0;
  upload_reset(10);
  device.logsCount = 0;
  device.status.readyLogData = 0;
  device.newAddressLog = mem[MB_LOG].start;
  return true;
}
#endif

// Полностью стёртая ячейка; незавершённую запись повторно не используем.
#ifndef HYBRID_CARD
bool log_blank(const cellLog_s *cell){
  const uint8_t *p = (const uint8_t*)cell;
  for (uint16_t i = 0; i < sizeof(*cell); i++) if (p[i] != 0xFF) return false;
  return true;
}
#endif

/* >>> номер записи журнала
 * Номер - время события, но два события в одну секунду получали одинаковый «уникальный»
 * номер. Теперь он всегда больше предыдущего: по последней записи во флешке (переживает
 * перезапуск) и по последнему выданному (переживает стирание журнала) (18.09.2026). */
// Старые записи с нулевым резервом читаются без миграции.
#ifndef HYBRID_CARD
bool log_valid(const cellLog_s *cell){
  uint32_t mark;
  uint16_t crc, inverse;
  if (cell->index == 0xFFFFFFFFu) return false;
  memcpy(&mark, cell->reserve + 4, 4);
  if (mark == 0) {
    for (uint8_t i = 0; i < sizeof(cell->reserve); i++) if (cell->reserve[i] != 0) return false;
    return true;
  }
  if (mark != 0x32474F4Cu) return false;
  memcpy(&crc, cell->reserve, 2);
  memcpy(&inverse, cell->reserve + 2, 2);
  return inverse == (uint16_t)~crc &&
    crc == crc16_calc((uint8_t*)cell, sizeof(*cell) - sizeof(cell->reserve));
}
#endif

#ifdef HYBRID_CARD
extern uint32_t log_lastIndex;
#else
uint32_t log_lastIndex;
#endif
#ifndef HYBRID_CARD
static void log_uniqueIndex(cellLog_s *cl, uint32_t adr){
  cellLog_s prev;
  uint32_t last = log_lastIndex;
  if (adr >= mem[MB_LOG].start + sizeof(cellLog_s)){
    if (mem_read(adr - sizeof(cellLog_s), (uint8_t*)&prev, sizeof(prev)) &&
        log_valid(&prev) && prev.index > last) last = prev.index;
  }
  if (last != 0 && cl->index <= last) cl->index = last + 1;
}
#endif
/* <<< номер записи журнала */

// добавить событие в буфер
#ifndef HYBRID_CARD
bool log_addEvent(cellLog_s *cl){
  if (store.clear == 10 || !user_storage_flash_ready()) return false;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  cellLog_s cell;
  uint32_t startAddress = device.newAddressLog;
  if (startAddress < mem[MB_LOG].start) startAddress = mem[MB_LOG].start;
  for (uint32_t adr = startAddress; adr + sizeof(cell) <= endAdr; adr += sizeof(cell)){
    wdt_reload();
    if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell))) return false;
    if (log_blank(&cell)){
      log_uniqueIndex(cl, adr);
      cell = *cl;
      uint16_t crc = crc16_calc((uint8_t*)&cell, sizeof(cell) - sizeof(cell.reserve));
      uint16_t inverse = (uint16_t)~crc;
      uint32_t mark = 0x32474F4Cu;
      memcpy(cell.reserve, &crc, 2);
      memcpy(cell.reserve + 2, &inverse, 2);
      memset(cell.reserve + 4, 0xFF, 4);
      // Сначала данные и CRC, затем отдельная запись признака завершения.
      if (!mem_write(adr, (uint8_t*)&cell, sizeof(cell) - 4) ||
          !mem_write(adr + sizeof(cell) - 4, (uint8_t*)&mark, 4)) return false;
      if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell)) || !log_valid(&cell)) return false;
      log_lastIndex = cl->index;
      device.newAddressLog = adr + sizeof(cell);
      device.status.readyLogData = 1;
      return true;
    }
  }
  return false;
}
#endif

// получаем кол-во записей в журнале
#ifndef HYBRID_CARD
uint16_t log_getRecordsCount(void){
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  uint8_t tbuffer[1024];
  uint16_t cnt = 0;
  for (uint32_t adr = mem[MB_LOG].start; adr < endAdr; adr += sizeof(tbuffer)){
    uint16_t n = endAdr - adr > sizeof(tbuffer) ? sizeof(tbuffer) : endAdr - adr;
    wdt_reload();
    if (!mem_read(adr, tbuffer, n)) return 0xFFFFu;
    for (uint16_t a = 0; a + sizeof(cellLog_s) <= n; a += sizeof(cellLog_s)){
      cellLog_s *cell = (cellLog_s*)&tbuffer[a];
      if (log_blank(cell)){
        device.newAddressLog = adr + a;
        return cnt;
      }
      if (log_valid(cell)){
        cnt++;
        if (cell->index > log_lastIndex) log_lastIndex = cell->index;
      }
    }
  }
  device.newAddressLog = endAdr;
  return cnt;
}
#endif

// Номер выгружаемой записи логический: оборванные ячейки пропускаются.
#ifndef HYBRID_CARD
static bool log_readRecords(uint16_t first, uint8_t *data, uint16_t count){
  cellLog_s cell;
  uint16_t index = 0, copied = 0;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  for (uint32_t adr = mem[MB_LOG].start; adr + sizeof(cell) <= endAdr && copied < count; adr += sizeof(cell)){
    wdt_reload();
    if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell))) return false;
    if (log_blank(&cell)) break;
    if (!log_valid(&cell)) continue;
    if (index++ < first) continue;
    memset(cell.reserve, 0, sizeof(cell.reserve)); // внутренние CRC/признак в кадр не входят
    memcpy(data + copied * sizeof(cell), &cell, sizeof(cell));
    copied++;
  }
  return copied == count;
}
#endif

/* >>> отложенные события журнала
 * После запуска время платы неизвестно, пока сервер не пришлёт его в ответе на регистрацию
 * или командой 23: часы идут от нуля, и события получали дату 1970 года (18.09.2026). Такие
 * события ждут в памяти и пишутся, когда время придёт, с поправкой на прошедшие секунды.
 * Если время не пришло за LOG_TIME_WAIT секунд, очередь заполнена или плата уходит в
 * перезагрузку, события пишутся как есть, чтобы не пропасть. */
#define LOG_PENDING_MAX 16
#define LOG_TIME_WAIT   600
#ifdef HYBRID_CARD
extern cellLog_s log_pending[LOG_PENDING_MAX];
#else
cellLog_s log_pending[LOG_PENDING_MAX];
#endif
#ifdef HYBRID_CARD
extern uint8_t log_pendingCount;
#else
uint8_t log_pendingCount;
#endif
#ifdef HYBRID_CARD
extern uint8_t log_timeKnown;
#else
uint8_t log_timeKnown;
#endif
#ifdef ASM_RUNTIME
/* Pending foreground persistence runs before another photo background job. */
#ifndef HYBRID_CARD
bool user_storage_background_ready(void)
{
  return user_storage_command_ready() && !store_save_requested &&
    !(log_pendingCount && (log_timeKnown || device.worktime >= LOG_TIME_WAIT));
}
#endif
#endif


/* >>> Cooperative firmware block writer */
#ifdef HYBRID_CARD
extern struct {
  uint8_t *data;
  uint32_t address, position, remaining, crc, expected_crc, usb_epoch;
  uint16_t offset, block;
  uint8_t phase, waiting, channel, id, quiet, cancelled, code;
} fw_job;
#else
struct {
  uint8_t *data;
  uint32_t address, position, remaining, crc, expected_crc, usb_epoch;
  uint16_t offset, block;
  uint8_t phase, waiting, channel, id, quiet, cancelled, code;
} fw_job;
#endif
enum { FA_ERASE=1, FA_WRITE, FA_HEADER, FA_CRC, FA_REPLY };
#ifndef HYBRID_CARD
static bool fw_async_active(void) { return fw_job.phase != 0; }
#endif
#ifndef HYBRID_CARD
static void fw_async_cancel(void)
{
  if (fw_async_active()) fw_job.cancelled = fw_job.quiet = 1;
}
#endif
#ifndef HYBRID_CARD
static void fw_async_quiet(dataChanel_e channel)
{
  if (fw_async_active() && fw_job.channel == channel) fw_job.quiet = 1;
}
#endif
#ifndef HYBRID_CARD
static void fw_async_reply(uint8_t id) { fw_job.id = id; }
#endif
#ifndef HYBRID_CARD
static bool fw_async_begin(const uint8_t *data, uint32_t address, dataChanel_e channel)
{
  if (flash_owner || store.job || mem_config_result() || w25_async_active()) return false;
  if (address < mem[MB_FIRMWARE].start + FWBLOCK_OFFSET ||
      address > mem[MB_FIRMWARE].start + mem[MB_FIRMWARE].size - 1024u) return false;
  fw_job.data = work_acquire(1024u, WORK_FW_BLOCK);
  if (!fw_job.data) return false;
  memcpy(fw_job.data, data, 1024u);
  fw_job.address = address; fw_job.block = device.FWblockIdx;
  fw_job.offset = 0; fw_job.waiting = fw_job.quiet = fw_job.cancelled = 0;
  fw_job.channel = channel; fw_job.usb_epoch = usb_bus_resets;
  flash_claim(FO_FWBLOCK);
  fw_job.phase = ((address + 4095u) & ~4095u) < address + 1024u ? FA_ERASE : FA_WRITE;
  return true;
}
#endif
#ifndef HYBRID_CARD
static void fw_async_complete(uint8_t code)
{
  work_release(fw_job.data, WORK_FW_BLOCK); fw_job.data = NULL;
  fw_job.code = code; fw_job.phase = FA_REPLY;
}
#endif
#ifndef HYBRID_CARD
static void fw_storage_poll(uint32_t now)
{
  uint8_t r;
  uint16_t n;
  bootFWparam_s header;
  if (fw_job.channel == CH_USB && fw_job.usb_epoch != usb_bus_resets) fw_job.quiet = 1;
  if (fw_job.cancelled) {
    if (fw_job.waiting && w25_async_poll(now) == 1) return;
    work_release(fw_job.data, WORK_FW_BLOCK); fw_job.data = NULL; fw_job.phase = fw_job.waiting = 0; flash_release(FO_FWBLOCK);
    return; /* A timed-out operation cannot publish progress or a late ACK. */
  }
  if (fw_job.phase == FA_REPLY) {
    if (!fw_job.quiet && fw_job.channel == CH_SERVER && comm_tx_busy()) return;
    fw_job.phase = 0; flash_release(FO_FWBLOCK);
    if (!fw_job.quiet) user_send_statusUpdateFW(20, fw_job.code, (dataChanel_e)fw_job.channel, fw_job.id);
    return;
  }
  if (fw_job.waiting) {
    r = w25_async_poll(now);
    if (r == 1) return;
    fw_job.waiting = 0;
    if (r != 2) goto write_error;
    if (fw_job.phase == FA_ERASE) fw_job.phase = FA_WRITE;
    else fw_job.offset += 64u;
    return;
  }
  if (!w25_read_available()) return;
  if (fw_job.phase == FA_ERASE) {
    fw_job.waiting = w25_async_start((fw_job.address + 4095u) & ~4095u, NULL, 0, now);
    if (!fw_job.waiting) goto write_error;
    return;
  }
  if (fw_job.phase == FA_WRITE) {
    if (fw_job.offset < 1024u) {
      fw_job.waiting = w25_async_start(fw_job.address + fw_job.offset, fw_job.data + fw_job.offset, 64u, now);
      if (!fw_job.waiting) goto write_error;
      return;
    }
    /* Every byte was read back by W25 before the block cursor/percent moves. */
    fw_write_errors = 0; device.startWaitFWtime = 0;
    device.FWblockIdx = fw_job.block + 1u;
    device.firmwarePercent = (uint8_t)((uint32_t)device.FWblockIdx * 100u / device.FWblockCount);
    if (device.FWblockIdx < device.FWblockCount) { fw_async_complete(FW_BLOCK_OK); return; }
    fw_job.phase = FA_HEADER; return;
  }
  if (fw_job.phase == FA_HEADER) {
    if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&header, sizeof(header))) goto read_error;
    if (!header.firmwareSize || (header.firmwareSize & 3u) ||
        header.firmwareSize > (uint32_t)device.FWblockCount * 1024u ||
        header.firmwareSize > mem[MB_FIRMWARE].size - FWBLOCK_OFFSET) {
      fw_async_complete(fw_blocks_done(0)); return;
    }
    fw_job.remaining = header.firmwareSize; fw_job.expected_crc = header.firmwareCRC;
    fw_job.position = mem[MB_FIRMWARE].start + FWBLOCK_OFFSET; fw_job.crc = 0xffffffffu;
    fw_job.phase = FA_CRC; return;
  }
  if (fw_job.phase == FA_CRC) {
    n = fw_job.remaining > 64u ? 64u : (uint16_t)fw_job.remaining;
    /* Reuse the owned, aligned heap snapshot after its last program completed. */
    if (!mem_read(fw_job.position, fw_job.data, n)) goto read_error;
    for (uint16_t i = 0; i < n; i += 4u) {
      uint32_t word; memcpy(&word, fw_job.data + i, 4);
      fw_job.crc = CRC32(fw_job.crc, word); /* existing verified ASM kernel */
    }
    fw_job.position += n; fw_job.remaining -= n;
    if (!fw_job.remaining) fw_async_complete(fw_blocks_done(fw_job.crc == fw_job.expected_crc));
    return;
  }
  return;
write_error:
  r = ++fw_write_errors < FW_WRITE_TRIES ? FW_BLOCK_BADWRITE : FW_BLOCK_END;
  fw_async_complete(r);
  if (r == FW_BLOCK_END) { fw_job.phase = 0; flash_release(FO_FWBLOCK); fw_abort("WRITE"); flash_claim(FO_FWBLOCK); fw_job.phase = FA_REPLY; }
  return;
read_error:
  fw_async_complete(FW_BLOCK_END);
  fw_job.phase = 0; flash_release(FO_FWBLOCK); fw_abort("READ"); flash_claim(FO_FWBLOCK); fw_job.phase = FA_REPLY;
}
#endif
/* <<< Cooperative firmware block writer */

/* >>> Cooperative event writer */
/* Snapshot owns both async program sources, never the caller or queue storage.
 * result: 1 pending, 2 verified, 3 failed (retained for delayed reconciliation).
 * Each step reads one 32-byte cell, starts one command, or polls one command. */
#ifdef HYBRID_CARD
extern struct {
  cellLog_s cell;
  uint32_t address, previous, retry_at;
  uint8_t phase, waiting, result;
} log_job;
#else
struct {
  cellLog_s cell;
  uint32_t address, previous, retry_at;
  uint8_t phase, waiting, result;
} log_job;
#endif
enum { LA_PREVIOUS=1, LA_FIND, LA_BODY, LA_MARK, LA_VERIFY, LA_RECONCILE };
#ifndef HYBRID_CARD
static bool log_async_active(void) { return log_job.phase != 0; }
#endif

#ifndef HYBRID_CARD
static void log_async_begin(const cellLog_s *cell)
{
  log_job.cell = *cell;
  log_job.address = device.newAddressLog;
  if (log_job.address < mem[MB_LOG].start) log_job.address = mem[MB_LOG].start;
  log_job.previous = log_lastIndex;
  flash_claim(FO_LOG); log_job.phase = LA_PREVIOUS; log_job.waiting = 0; log_job.result = 1;
}
#endif

#ifndef HYBRID_CARD
static uint8_t log_async_step(uint32_t now)
{
  cellLog_s actual;
  uint8_t r;
  uint16_t crc, inverse;
  uint32_t mark = 0x32474f4cu;
  if (!log_job.phase) return log_job.result;
  if (log_job.result == 3 && (int32_t)(now - log_job.retry_at) < 0) return 3;
  if (log_job.waiting) {
    r = w25_async_poll(now);
    if (r == 1) return 1;
    log_job.waiting = 0;
    if (r != 2) { log_job.phase = LA_RECONCILE; goto failed; }
    ++log_job.phase; /* BODY -> MARK -> VERIFY */
    return log_job.result = 1;
  }
  if (!w25_read_available()) return log_job.result;
  if (log_job.address < mem[MB_LOG].start ||
      log_job.address > mem[MB_LOG].start + mem[MB_LOG].size - sizeof(actual)) {
    log_job.phase = 0; goto failed; /* Full: allow upload/clear to make room. */
  }
  switch (log_job.phase) {
  case LA_PREVIOUS:
    if (log_job.address >= mem[MB_LOG].start + sizeof(actual)) {
      if (!mem_read(log_job.address - sizeof(actual), (uint8_t*)&actual, sizeof(actual))) goto failed;
      if (log_valid(&actual) && actual.index > log_job.previous) log_job.previous = actual.index;
    }
    log_job.phase = LA_FIND; break;
  case LA_FIND:
    if (!mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual))) goto failed;
    if (!log_blank(&actual)) {
      if (log_valid(&actual) && actual.index > log_job.previous) log_job.previous = actual.index;
      log_job.address += sizeof(actual); break;
    }
    if (log_job.previous && log_job.cell.index <= log_job.previous) log_job.cell.index = log_job.previous + 1u;
    crc = crc16_calc((uint8_t*)&log_job.cell, sizeof(actual) - sizeof(actual.reserve));
    inverse = (uint16_t)~crc;
    memcpy(log_job.cell.reserve, &crc, 2); memcpy(log_job.cell.reserve + 2, &inverse, 2);
    memcpy(log_job.cell.reserve + 4, &mark, 4);
    log_job.phase = LA_BODY; break;
  case LA_BODY:
  case LA_MARK:
    r = log_job.phase == LA_BODY;
    log_job.waiting = w25_async_start(log_job.address + (r ? 0u : sizeof(actual) - 4u),
      (uint8_t*)&log_job.cell + (r ? 0u : sizeof(actual) - 4u), r ? sizeof(actual) - 4u : 4u, now);
    if (!log_job.waiting) { log_job.phase = LA_RECONCILE; goto failed; }
    break;
  case LA_VERIFY:
  case LA_RECONCILE:
    if (!mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual))) goto failed;
    if (!memcmp(&actual, &log_job.cell, sizeof(actual)) && log_valid(&actual)) {
      /* Only this verified path publishes the cursor and releases the queue. */
      log_lastIndex = log_job.cell.index;
      device.newAddressLog = log_job.address + sizeof(actual);
      device.status.readyLogData = 1;
      log_job.phase = 0; flash_release(FO_LOG); return log_job.result = 2;
    }
    if (log_job.phase == LA_VERIFY) { log_job.phase = LA_RECONCILE; goto failed; }
    /* A failed write may have reached NOR. Re-read before retrying: acknowledge
     * the exact committed record once, or finish compatible torn bytes in place.
     * Never overwrite an unrelated/over-cleared cell. */
    for (uint16_t i = 0; i < sizeof(actual); ++i) {
      if ((((uint8_t*)&actual)[i] & ((uint8_t*)&log_job.cell)[i]) != ((uint8_t*)&log_job.cell)[i]) {
        if (log_valid(&actual) && actual.index > log_job.previous) log_job.previous = actual.index;
        log_job.address += sizeof(actual); log_job.phase = LA_FIND;
        return log_job.result = 1;
      }
    }
    log_job.phase = LA_BODY; break;
  }
  return log_job.result = 1;
failed:
  if (log_job.phase == LA_PREVIOUS || log_job.phase == LA_FIND) { log_job.phase = 0; flash_release(FO_LOG); }
  log_job.retry_at = now + 1000u;
  return log_job.result = 3;
}
#endif

#ifndef HYBRID_CARD
static void log_storage_poll(uint32_t now)
{
  if (!log_async_active()) {
    if (log_pendingCount && (log_job.result != 3 || (int32_t)(now - log_job.retry_at) >= 0))
      log_async_begin(&log_pending[0]);
    return;
  }
  if (log_async_step(now) != 2) return;
  --log_pendingCount;
  memmove(log_pending, log_pending + 1, log_pendingCount * sizeof(log_pending[0]));
}
#endif
/* <<< Cooperative event writer */

// записать отложенные события как есть
#ifndef HYBRID_CARD
void log_flushPending(void){
  if (store_runtime) return; /* Only user_storage_poll advances runtime writes. */
  while (log_pendingCount) {
    if (!log_addEvent(&log_pending[0])) return;
    log_pendingCount--;
    memmove(log_pending, log_pending + 1, log_pendingCount * sizeof(log_pending[0]));
  }
}
#endif

#ifndef HYBRID_CARD
void log_setTime(uint32_t now){
  device.datetime = now;
  id_endScan = true;
  if (!log_timeKnown) {
    for (uint8_t i = log_async_active() ? 1u : 0u; i < log_pendingCount; i++){
      log_pending[i].datetime = now - (device.worktime - log_pending[i].datetime);
      log_pending[i].index = log_pending[i].datetime;
    }
  }
  log_timeKnown = 1;
  log_flushPending();
}
#endif

#ifndef HYBRID_CARD
void log_checkPending(void){
  if (log_pendingCount && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)) log_flushPending();
}
#endif

#ifndef HYBRID_CARD
static bool log_put(cellLog_s *cell){
  if (!store_runtime && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)){
    log_flushPending();
    if (!log_pendingCount && log_addEvent(cell)) return true;
  }
  if (!store_runtime && log_pendingCount == LOG_PENDING_MAX) log_flushPending();
  if (log_pendingCount == LOG_PENDING_MAX) return false;
  log_pending[log_pendingCount] = *cell;
  // до времени сервера datetime хранит uptime, затем переводится один раз
  if (!log_timeKnown) log_pending[log_pendingCount].datetime = device.worktime;
  log_pendingCount++;
  return true;
}
#endif
/* <<< отложенные события журнала */

// добавить системное событие
#ifndef HYBRID_CARD
bool log_addSystemEvent(logSystem_e num){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)num; // номер события листа «Журнал событий»
  
  return log_put(&cell);
}
#endif

/* Событие с числом текстом в поле ID (37 - баланс, 38 - сигнал), остаток поля нули. */
#ifndef HYBRID_CARD
static bool log_addSystemValue(logSystem_e num, int32_t value)
{
  char id[12] = {0};
  format_i32(id, NULL, value);
  return log_addSystemEventID(num, id);
}
#endif

// добавить системное событие
#ifndef HYBRID_CARD
bool log_addSystemEventID(logSystem_e num, char *id){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)num; // номер события листа «Журнал событий»
  
  if (id != NULL)
    memcpy(cell.ID, id, 10);
    
  return log_put(&cell);
}
#endif

// добавить событие доступа
#ifndef HYBRID_CARD
bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)num; // номер события листа «Журнал событий»
  cell.passCnt = cnt;
  
  if (id != NULL)
    memcpy(cell.ID, id, 10);
  
  return log_put(&cell);
}
#endif

// обновление FW: в аргументе 1 текущая версия - HW 1 байт, FW 2 байта, билд 4 байта,
// в аргументе 2 код причины (Протокол2 E533, E534)
#ifndef HYBRID_CARD
bool log_addUpdateEvent(logUpdate_e num, uint32_t reason){
  cellLog_s cell;
  uint16_t fw = device.firmwareVersion; // device упакована: копируем через переменные
  uint32_t build = device.buildDate;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)num;
  cell.ID[0] = (char)device.hardwareVersion;
  memcpy(&cell.ID[1], &fw, 2);
  memcpy(&cell.ID[3], &build, 4);
  cell.arg2 = reason;
  
  return log_put(&cell);
}
#endif

// фотофиксация: номер фото в первых 4 байтах аргумента 1 (Протокол2 E533)
#ifndef HYBRID_CARD
bool log_addPhotoEvent(uint32_t number){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LOG_CON_PHOTOFIX;
  memcpy(cell.ID, &number, 4);
  
  return log_put(&cell);
}
#endif


// ********************** Память фото **************

// получаем кол-во фото в памяти
#ifndef HYBRID_CARD
uint16_t user_getPhotoCount(void)
{
  uint32_t adr = mem[MB_PHOTO].start;
  uint32_t end = photo_storage_end();
  uint16_t count = 0;
  photoParam_s h;
  uint8_t cleanup = photo_cleanup_active();
  if (cleanup) {
    device.newAddressPhoto = cleanup == 1 ? adr : UINT32_MAX; device.countPhoto = 0; device.status.readyPhoto = 0;
    return 0;
  }
  if (!photo_recover()) { device.newAddressPhoto = UINT32_MAX; device.countPhoto = 0; device.status.readyPhoto = 0; return 0; }
  while (adr + sizeof(h) <= end) {
    if (!mem_read(adr, (uint8_t*)&h, sizeof(h))) { adr = UINT32_MAX; count = 0; break; }
    if (h.datetime == 0xFFFFFFFFu) break;
    if (!h.size || h.size > 65535u || end - adr < PHOTO_RECORD_HEAD ||
        h.size > end - adr - PHOTO_RECORD_HEAD) {
      adr = end; break; // повреждённую запись не обходим за границу области
    }
    if (photo_confirmed_offset(adr, (uint16_t)h.size) < h.size) count++;
    adr += PHOTO_RECORD_HEAD + h.size;
  }
  device.newAddressPhoto = adr;
  device.countPhoto = count; device.status.readyPhoto = count != 0;
  return count;
}
#endif


// ************************ Прошивка ***********************
#ifndef HYBRID_CARD
bool user_checkFirmware (uint32_t adr, uint32_t size, uint32_t crc){

  if (!size || (size & 3u)) return false;

  uint32_t crcvalue = 0xffffffff;
  uint32_t readed;
  uint32_t i;

  // подаём тактирование на модуль CRC
  /*RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
  
  CRC_ResetDR();
  
  while (size > 0){
    readed = (size > 128) ? 128 : size; // сколько считывать байт
    mem_read (adr, bf, readed);
    // аппаратный подсчёт контрольной суммы
    crcvalue = CRC_CalcBlockCRC(bf32, readed/4);

    adr += readed;
    size -= readed;
  }*/
  
  while (size > 0){
    readed = (size > 128) ? 128 : size; // сколько считывать байт
    if (!mem_read (adr, bf, readed)) return false;
    
    for (i = 0; i < (readed/4); i++)
      crcvalue = CRC32(crcvalue, bf32[i]);

    adr += readed;
    size -= readed;
  }
  
  // проверяем
  if (crcvalue != crc) 
    return false;
  else 
    return true;
}
#endif

#ifndef ASM_CRC
#ifndef HYBRID_CARD
uint32_t CRC32(uint32_t crc, uint32_t data){
  crc = crc ^ data;
  for (uint8_t i = 0; i < 32; i++){
    if (crc&0x80000000)
      crc = (crc<<1)^0x4C11DB7;
    else
      crc <<= 1;
  }
  return crc;
}
#endif
#endif /* !ASM_CRC */

/* Команда 111, раскладка версии 4: 57 значений uint32, младшим байтом вперёд.
 *  0 версия, 1 время работы с, 2 миллисекунды,
 *  3 принудительные сбросы ESOF, 4 таймауты сброса ESOF, 5 сбросы шины,
 *  6 восстановления USB, 7 сторож: IN не забран, 8 сторож: OUT при ненастроенном USB,
 *  9..15 последнее восстановление: причина, bDeviceState, TX в пути, байт в очереди, мс с IN/OUT/SOF,
 *  16..21 сейчас: bDeviceState, TX в пути, байт в очереди, мс с IN/OUT/SOF,
 *  22 ошибки TX, 23 ошибки RX, 24 потерянные байты отладки, 25 OUT при ненастроенном USB,
 *  26 самый долгий проход главного цикла мс, 27 проходов >= 1 с, 28 ошибки хранилища настроек,
 *  29 включения GSM, 30 GSM без ответа, 31 отказы кучи 4 КиБ, 32 есть запись аварии,
 *  33..41 на момент последнего восстановления, 42..50 сейчас: EP1R, EP3R, счётчик TX EP1, ISTR, CNTR, FNR,
 *  USB_Tx_State, выставлено пакетов IN, забрано пакетов IN,
 *  51 ошибки шины USB (прерывания ERR) сейчас, 52 ошибки шины на момент последнего восстановления,
 *  53 DADDR и 54 EP0R на момент последнего восстановления, 55 DADDR и 56 EP0R сейчас,
 *  57 минимальный запас стека за работу, байт (версия 5, 01.10.2026).
 */
#ifndef HYBRID_CARD
static void user_send_usbDiag(uint8_t uniqId)
{
  uint32_t v[58];
  uint8_t send_buffer[8 + 3 + sizeof(v)];
  uint16_t len = 3 + sizeof(v), crc;

  v[0] = 5;
  v[1] = device.worktime;
  v[2] = timers_getMillis();
  usb_diag_fill(&v[3]);
  v[26] = loop_max_ms;
  v[27] = loop_stalls;
  v[28] = mem_config_errors;
  v[29] = diag_gsm_pwron;
  v[30] = diag_gsm_notanswer;
  v[31] = diag_heap_fail;
  v[32] = fault_record[0] == FAULT_RECORD_MAGIC;
  usb_diag_fill_regs(&v[33]);
  usb_diag_fill_errors(&v[51]);
  usb_diag_fill_addr(&v[53]);
  v[57] = stack_free_bytes();

  send_buffer[0] = 'S';
  send_buffer[1] = 'L';
  send_buffer[2] = 'D';
  send_buffer[3] = 'N';
  send_buffer[4] = _1B(len);
  send_buffer[5] = _2B(len);
  send_buffer[6] = 0;
  send_buffer[7] = uniqId;
  send_buffer[8] = USB_DIAG_COMMAND;
  memcpy(&send_buffer[9], (uint8_t*)v, sizeof(v));
  crc = crc16_calc(&send_buffer[4], len + 2);
  send_buffer[len + 6] = crc & 0xff;
  send_buffer[len + 7] = (crc >> 8) & 0xff;
  usb_send(send_buffer, len + 8);
}
#endif

/* Команда 112, версия 1: uint32 младшим байтом вперёд.
 *  0 версия, 1 мс,
 *  2 модем: состояние | шаг<<8 | фаза<<16 | причина<<24, 3 код модема | сбой AT<<16, 4 пройденные шаги (биты),
 *  5 канал (comm_channel_diag), 6 сервер (comm_server_diag), 7 восстановление (comm_recover_diag), 8 остаток паузы мс,
 *  9..13 UART: ошибки, переполнения, ORE, FE, NE,
 *  14 сбоев всего | следующий<<8 | последний код<<16,
 *  15..24 кольцо сбоев: шаг | причина<<8 | секунды<<16,
 *  25.. длительности шагов последней попытки, 10 мс (SLM_STEP_COUNT значений). */
#ifndef HYBRID_CARD
static void user_send_commDiag(uint8_t uniqId)
{
  uint32_t v[25 + SLM_STEP_COUNT];
  uint8_t send_buffer[8 + 3 + sizeof(v)];
  uint16_t len = 3 + sizeof(v), crc;
  const slm_status_t *s = modem_slm336_status();
  const slm_diag_t *d = modem_slm336_diag();
  comm_uart_stats_t u;
  uint32_t now = timers_getMillis();
  comm_uart_stats(&u);
  v[0] = 1; v[1] = now;
  v[2] = s->state | (uint32_t)s->step << 8 | (uint32_t)s->phase << 16 | (uint32_t)s->reason << 24;
  v[3] = s->modem_code | (uint32_t)s->at_fault << 16; v[4] = s->completed_steps;
  v[5] = comm_channel_diag(); v[6] = comm_server_diag(); v[7] = comm_recover_diag(now, &v[8]);
  v[9] = u.errors; v[10] = u.overflow; v[11] = u.ore; v[12] = u.fe; v[13] = u.ne;
  v[14] = d->failure_count | (uint32_t)d->failure_next << 8 | (uint32_t)d->last_code << 16;
  for (uint8_t i = 0; i < 10u; i++)
    v[15 + i] = d->failure_step[i] | (uint32_t)d->failure_reason[i] << 8 | (uint32_t)d->failure_seconds[i] << 16;
  for (uint8_t i = 0; i < SLM_STEP_COUNT; i++) v[25 + i] = d->steps[i];
  send_buffer[0] = 'S'; send_buffer[1] = 'L'; send_buffer[2] = 'D'; send_buffer[3] = 'N';
  send_buffer[4] = _1B(len); send_buffer[5] = _2B(len);
  send_buffer[6] = 0; send_buffer[7] = uniqId; send_buffer[8] = USB_COMM_DIAG_COMMAND;
  memcpy(&send_buffer[9], (uint8_t*)v, sizeof(v));
  crc = crc16_calc(&send_buffer[4], len + 2);
  send_buffer[len + 6] = crc & 0xff; send_buffer[len + 7] = (crc >> 8) & 0xff;
  usb_send(send_buffer, len + 8);
}
#endif

/* Часы прошивки - местное время (как у суток ID); до установки - недостоверно. */
#ifndef HYBRID_CARD
uint32_t user_comm_local_time(void)
{ return device.datetime >= ID_TIME_VALID ? device.datetime : UINT32_MAX; }
#endif
/* Ночной перезапуск модема не рвёт обновление прошивки. */
/* Баланс SIM (статус 2): USSD из настроек (команда 12) и разобранный ответ, рубли. */
#ifndef HYBRID_CARD
const char *user_comm_ussd(void) { return config.ussdBalance; }
#endif
#ifndef HYBRID_CARD
uint8_t user_comm_volte(void) { return config.useVoLTE; }
#endif
#ifndef HYBRID_CARD
void user_comm_balance(int32_t rubles)
{
  device.balance = rubles < 1 ? 1u : rubles > 65000 ? 65000u : (uint16_t)rubles; /* статус 2: 0 - не определён */
  /* 37 «Баланс ниже нормы»: после каждого запроса, порог 100 р (триггер «баланс менее 100»). */
  if (rubles < 100) log_addSystemValue(LOG_SYS_LOWBALANCE, rubles);
}
#endif
#ifndef HYBRID_CARD
void user_comm_low_signal(uint8_t csq) { log_addSystemValue(LOG_SYS_LOWSIGNAL, csq); }
#endif
/* Журнал связи: 14/15 подключён, 16/17 ошибка соединения, 18/19 потеря связи с сервером 1/2;
   11 нет регистрации в сети, 12 нет интернета. */
#ifndef HYBRID_CARD
bool user_comm_link(uint8_t kind, uint8_t address)
{
  static const uint8_t first[] = {LOG_SYS_CONNECT1, LOG_SYS_ERRCONNECT1, LOG_SYS_FAILCONNECT1, LOG_SYS_NOTGSMREG, LOG_SYS_NOTGPRSREG};
  if (kind >= sizeof(first)) return true;
  uint8_t event = first[kind];
  if (kind <= COMM_LINK_LOST) event = (uint8_t)(event + user_comm_server(address));
  return log_addSystemEvent((logSystem_e)event);
}
#endif
/* Ночной перезапуск модема ждёт конца загрузки FW и вызова. */
#ifndef HYBRID_CARD
bool user_comm_busy(void) { return device.FWinProgress != 0 || comm_call_active(); }
#endif

/* Адрес сервера в настройках - "домен:порт" (как у старой связи). */
#ifdef HYBRID_CARD
extern char comm_host[2][32];
#else
char comm_host[2][32];
#endif
#ifdef HYBRID_CARD
extern uint16_t comm_port[2];
#else
uint16_t comm_port[2];
#endif
#ifdef HYBRID_CARD
extern char comm_credential[sizeof(config.netOperator)];
#else
char comm_credential[sizeof(config.netOperator)];
#endif
#ifndef HYBRID_CARD
static void comm_split(uint8_t n, const char *server)
{
  const char *colon = strrchr(server, ':');
  size_t len = colon ? (size_t)(colon - server) : strlen(server);
  if (len >= sizeof(comm_host[n])) len = sizeof(comm_host[n]) - 1u;
  memcpy(comm_host[n], server, len); comm_host[n][len] = 0;
  comm_port[n] = colon ? (uint16_t)atoi(colon + 1) : 0;
}
#endif
/* Адрес 0 - сервер 1, если он включён или выключены оба (как в comm_endpoint); адрес 1 - сервер 2. */
#ifndef HYBRID_CARD
uint8_t user_comm_server(uint8_t address) { return address || (!config.useServer1 && config.useServer2); }
#endif
/* Читаем текущие настройки: команда 12 применяется без перезагрузки платы.
   Указатель живёт до следующего запроса поля; сборщик AT копирует его сразу. */
#ifndef HYBRID_CARD
static void comm_endpoint(uint8_t n)
{
  comm_split(n, n || (!config.useServer1 && config.useServer2) ? config.server2 : config.server1);
}
#endif
#ifndef HYBRID_CARD
static const char *comm_text(uint8_t field, uint16_t *capacity)
{
  static const char empty[1] = "";
  switch (field) {
  case SLM_CFG_HOST: comm_endpoint(0); *capacity = sizeof(comm_host[0]); return comm_host[0];
  case SLM_CFG_HOST2: comm_endpoint(1); *capacity = sizeof(comm_host[1]); return comm_host[1];
  case SLM_CFG_APN: case SLM_CFG_LOGIN: case SLM_CFG_PASSWORD: {
    const char *p = config.netOperator, *end = p + sizeof(config.netOperator);
    uint8_t part = (uint8_t)(field - SLM_CFG_APN), n = 0;
    while (part && p < end && *p) { if (*p++ == ',') --part; }
    if (part) { *capacity = sizeof(empty); return empty; }
    while (p < end && *p && *p != ',' && n + 1u < sizeof(comm_credential)) comm_credential[n++] = *p++;
    comm_credential[n] = 0; *capacity = sizeof(comm_credential); return comm_credential;
  }
  default: *capacity = 0; return NULL;
  }
}
#endif
#ifndef HYBRID_CARD
static uint16_t comm_number(uint8_t field)
{
  switch (field) {
  case SLM_CFG_PORT: comm_endpoint(0); return comm_port[0];
  case SLM_CFG_PORT2: comm_endpoint(1); return comm_port[1];
  case SLM_CFG_NETWORK: {
    /* Команда 12: авто, GSM, авто 3G/4G, UMTS, LTE; значения QCFG другие. */
    static const uint8_t rat[] = {0, 1, 0, 5, 3};
    return config.netType < sizeof(rat) ? rat[config.netType] : 0;
  }
  case SLM_CFG_RESERVE: comm_endpoint(1); return config.useServer2 && comm_host[1][0] && comm_port[1];
  /* Ручной тип (команда 105) имеет приоритет; 0/255 - определить по AT+GMM. */
  case SLM_CFG_TYPE: return device.moduleType;
  default: return 0;
  }
}
#endif
#ifndef HYBRID_CARD
const slm_settings_t comm_settings = {comm_text, comm_number};
#endif
/* v5: PC13 - питание модема, PC14 - ключ включения; high - физический уровень. */
#ifndef HYBRID_CARD
static void comm_gpio(uint8_t pin, bool high)
{
  uint16_t bit = (uint16_t)(1u << pin);
  if (high) GPIOC->BSRR = bit; else GPIOC->BRR = bit;
}
#endif
#ifndef HYBRID_CARD
static void comm_type(uint8_t type) { device.moduleType = type; }
#endif
#ifndef HYBRID_CARD
static void comm_imei(const char *imei) { memcpy(device.IMEIGSM, imei, 16); }
#endif
/* Старт и повторный старт (заводские настройки) новой связи. */
#ifndef HYBRID_CARD
void user_comm_boot(void)
{
  static const slm_handlers_t handlers = {comm_gpio, comm_type, comm_imei};
  comm_split(0, config.useServer1 || !config.useServer2 ? config.server1 : config.server2);
  comm_split(1, config.server2);
  if (!user_comm_start(&comm_settings, &handlers, timers_getMillis()))
    (void)comm_modem_restart(true, timers_getMillis());
}
#endif

/* >>> Cooperative storage implementation */
#define STORE_CLEAR_MARK 0x31434C00u
#ifndef HYBRID_CARD
static void store_usb_session(void)
{
  if (store.usb_epoch == usb_bus_resets) return;
  store.usb_epoch = usb_bus_resets;
  idclear_runtime_quiet(CH_USB); fw_service_quiet(CH_USB); upload_runtime_quiet(CH_USB);
  fw_async_quiet(CH_USB);
  table_runtime_quiet(CH_USB); service_crc_quiet(CH_USB);
  log_unacked[0] = temp_unacked[0] = 0; store.seen &= 10u;
  if (store.job && store.channel == CH_USB) store.quiet = 1;
}
#endif
/* device.reserv[0..7] is the existing upload checkpoint. Bytes 8..15 hold
 * a cleanup intent and its exclusive end. The CRC of device covers both. */
#ifndef HYBRID_CARD
static void store_marker(uint8_t cmd, uint32_t end)
{
  uint32_t mark = cmd ? STORE_CLEAR_MARK | cmd : 0;
  memcpy(device.reserv + 8, &mark, 4); memcpy(device.reserv + 12, &end, 4);
}
#endif
#ifndef HYBRID_CARD
bool store_snapshot(void)
{
  config.crc = crc16_calc((uint8_t*)&config, sizeof(config) - 2);
  device.crc = crc16_calc((uint8_t*)&device, sizeof(device) - 2);
  return mem_config_begin((uint8_t*)&config, (uint8_t*)&device);
}
#endif
#ifndef HYBRID_CARD
static void store_restore(const device_s *saved)
{
  uint32_t mark, end, start, limit;
  memcpy(&mark, saved->reserv + 8, 4); memcpy(&end, saved->reserv + 12, 4);
  if (mark != (STORE_CLEAR_MARK | 10u) && mark != (STORE_CLEAR_MARK | 9u)) return;
  uint8_t cmd = (uint8_t)mark;
  start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;
  limit = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;
  if (end < start || end > limit) return;
  store_marker(cmd, end); store.clear = store.cmd = cmd;
  store.erase = start & ~4095u; store.end = end;
  store.job = US_RECOVER; store.waiting = 0; store.quiet = 1;
  store.erase = start;
}
#endif
#ifndef HYBRID_CARD
bool store_upload_request(uint8_t cmd, dataChanel_e cch, uint8_t uniq)
{
  uint8_t slot = (cmd == 10 ? 0 : 2) + (cch == CH_USB ? 0 : 1);
  if (store.job || mem_config_result()) return false;
  store.cmd = cmd; store.channel = cch; store.uniq = uniq; store.quiet = 0;
  store.repeat = (store.seen & (1u << slot)) && store.last_id[slot] == uniq;
  store.last_id[slot] = uniq; store.seen |= 1u << slot;
  return true;
}
#endif
#ifndef HYBRID_CARD
static bool store_ack(uint8_t cmd, dataChanel_e cch)
{
  uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;
  uint8_t channel = cch == CH_USB ? 0 : 1;
  if (!pending[channel] || store.repeat) return true;
  uint16_t old = upload_index(cmd, cch);
  store.cursor = pending[channel];
  upload_setIndex(cmd, cch, store.cursor); upload_store();
  bool accepted = store_snapshot();
  upload_setIndex(cmd, cch, old); upload_store();
  if (accepted) store.job = US_ACK;
  else store.seen &= ~(1u << ((cmd == 10 ? 0 : 2) + channel));
  return false;
}
#endif
#ifndef HYBRID_CARD
static bool store_finish(uint8_t cmd)
{
  bool active = cmd == 10 ? device.status.readyLogData || device.logsCount : device.status.readyID || device.tempIDsCount;
  if (!active && !upload_index(cmd, CH_SERVER) && !upload_index(cmd, CH_USB)) return true;
  uint32_t start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;
  uint32_t end = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;
  uint32_t used = cmd == 10 ? device.newAddressLog : device.newAddressTempID;
  if (used > start && used < end) end = used;
  store.clear = cmd; store.erase = start & ~4095u; store.end = end;
  store_marker(cmd, end);
  /* Keep the already durable cursors until the cleanup intent itself is durable. */
  store.job = US_MARK;
  if (!store_snapshot()) store.retry_at = timers_getMillis() + 1000u;
  return false;
}
#endif
#ifndef HYBRID_CARD
static void store_command_start(uint8_t cmd, dataChanel_e cch, uint8_t uniq)
{
  store.cmd = cmd; store.channel = cch; store.uniq = uniq; store.quiet = 0;
  if (store_snapshot()) { store.job = US_COMMAND; store_save_requested = 0; }
  else user_send_answer_comand(cmd, CMD_RUNERROR, cch, uniq);
}
#endif
#ifndef HYBRID_CARD
static void store_config_complete(void)
{
  rs485_set_config(mem_config_snapshot());
  device.datetimeConfig = config.datetime; device.status.defaultConfig = 0;
  sync_packet(SYNC_CONFIG);
  if (comm_modem_state() == SLM_IDLE) user_comm_boot();
}
#endif
#ifndef HYBRID_CARD
static bool store_config_start(void)
{
  if (store.job || !store_snapshot()) return false;
  store.job = US_CONFIG; return true;
}
#endif
#ifndef HYBRID_CARD
static void store_reply(void)
{
uint8_t cmd=store.cmd,id=store.uniq,quiet=store.quiet;
  dataChanel_e cch=(dataChanel_e)store.channel;
  if(!quiet && cch==CH_SERVER && comm_tx_busy()) return;
  if(!quiet && cmd==12) { if(service_crc_begin_config(cch,id)) store.job=0; return; }
  store.job=0; if(quiet) return;
  if(cmd==10) user_send_logRecords(cch,id);
  else if(cmd==9) user_send_tempIDs(cch,id);
  else user_send_answer_comand(cmd,CMD_SUCCESS,cch,id);
}
#endif
#ifndef HYBRID_CARD
void user_storage_poll(void)
{
  store_usb_session();
  if(idindex_runtime_active()) { idindex_poll(mem[MB_ID].start); return; }
  if(service_crc_active()) { service_crc_poll(); return; }
  if(expiry_runtime_active()) { expiry_runtime_poll(timers_getMillis()); return; }
  if(upload_runtime_active()) { upload_runtime_poll(); return; }
  if(idclear_runtime_active()) { idclear_runtime_poll(timers_getMillis()); return; }
  if(fw_service_active()) { fw_service_poll(); return; }

  uint32_t now = timers_getMillis();
  uint8_t result = mem_config_result();
  fw_service_config_result(result);
  if (table_runtime_active()) { table_runtime_poll(now); return; }
  if (photo_storage_busy()) { photo_storage_poll(now); return; }
  if (fw_async_active()) { fw_storage_poll(now); return; }
  if (log_async_active()) { log_storage_poll(now); return; }
  if (result == 1) { mem_config_poll(now); return; }
  if (store.job == US_REPLY) { store_reply(); return; }
  if (store.job == US_RECOVER) {
    /* An older application ignores the reserved intent and may have appended
     * after a rollback. Preserve a nonempty first sector; resend its records
     * from cursor zero instead of erasing data that might not be acknowledged. */
    uint8_t b[64];
    uint32_t start = mem[store.clear == 10 ? MB_LOG : MB_TEMPID].start;
    if ((int32_t)(now - store.retry_at) < 0) return;
    if (!w25_read_data(store.erase, b, sizeof(b))) { store.retry_at = now + 1000u; return; }
    for (uint16_t i = 0; i < sizeof(b); ++i) if (b[i] != 255u) {
      upload_reset(store.clear); store_marker(0, 0); store.job = US_RECOVER_CLEAN;
      if (!store_snapshot()) store.retry_at = now + 1000u;
      return;
    }
    store.erase += sizeof(b);
    if (!(store.erase & 4095u) || store.erase >= store.end) {
      store.erase = start & ~4095u; store.job = US_ERASE;
    }
    return;
  }
  if (store.job == US_ERASE) {
    if ((int32_t)(now - store.retry_at) < 0) return;
    if (store.waiting) {
      uint8_t r = w25_async_poll(now);
      if (r == 1) return;
      store.waiting = 0;
      if (r != 2) { store.retry_at = now + 1000u; return; }
      store.erase += 4096u;
      return;
    }
    if (store.erase < store.end) {
      store.waiting = w25_async_start(store.erase, NULL, 0, now);
      if (!store.waiting) store.retry_at = now + 1000u;
      return;
    }
    upload_reset(store.clear);
    if (store.clear == 10) {
      device.logsCount = 0; device.status.readyLogData = 0; device.newAddressLog = mem[MB_LOG].start;
    } else {
      device.tempIDsCount = 0; device.status.readyID = 0; device.newAddressTempID = mem[MB_TEMPID].start;
    }
    store_marker(0, 0); store.job = US_CLEAN;
    if (!store_snapshot()) store.retry_at = now + 1000u;
    return;
  }
  if (result == 3) {
    mem_config_reset();
    if (store.job == US_MARK || store.job == US_CLEAN || store.job == US_RECOVER_CLEAN) {
      /* Never append over a possibly published cleanup intent, even on an error. */
      store.retry_at = now + 1000u; return;
    }
    uint8_t job = store.job;
    store.job = 0;
    if (job == US_SAVE) store_save_requested = 1;
    if (job == US_ACK) store.seen &= ~(1u << ((store.cmd == 10 ? 0 : 2) + (store.channel == CH_USB ? 0 : 1)));
    if ((job == US_CONFIG || job == US_COMMAND) && !store.quiet)
      user_send_answer_comand(store.cmd, CMD_RUNERROR, (dataChanel_e)store.channel, store.uniq);
    return;
  }
  if (result == 2) {
    uint8_t job = store.job;
    if (job == US_CONFIG) {
      store_config_complete();
    }
    mem_config_reset();
    if (job == US_ACK) {
      uint16_t *pending = store.cmd == 10 ? log_unacked : temp_unacked;
      upload_setIndex(store.cmd, (dataChanel_e)store.channel, store.cursor); upload_store();
      pending[store.channel == CH_USB ? 0 : 1] = 0;
      store.job = US_REPLY;
    } else if (job == US_MARK) {
      upload_reset(store.clear); store.job = US_ERASE; store.retry_at = now;
    } else if (job == US_CLEAN || job == US_RECOVER_CLEAN) {
      store.clear = 0; store.job = US_REPLY;
    } else if (job == US_CONFIG || job == US_COMMAND) store.job = US_REPLY;
    else if (job == US_RESET) {
      if (log_pendingCount || store_save_requested) {
        store.job = 0; store_save_requested = 0; return;
      }
      store_reset_requested = 0; store.job = 0;
      USB_PULLUP_OFF; comm_modem_off(); NVIC_SystemReset();
    } else store.job = 0;
    return;
  }
  if (store.job == US_MARK || store.job == US_CLEAN || store.job == US_RECOVER_CLEAN) {
    if ((int32_t)(now - store.retry_at) >= 0 && !store_snapshot()) store.retry_at = now + 1000u;
    return;
  }
  if (store.job || !w25_read_available()) return;
  if (log_pendingCount && store.clear != 10 &&
      (store_reset_requested || (!store_save_requested && (log_timeKnown || device.worktime >= LOG_TIME_WAIT)))) {
    log_storage_poll(now); return;
  }
  if (store_reset_requested) {
    log_flushPending();
    if (log_pendingCount) return;
    if (store_snapshot()) store.job = US_RESET;
  } else if (store_save_requested) {
    if (store_snapshot()) { store_save_requested = 0; store.job = US_SAVE; }
  }
}
#endif
/* <<< Cooperative storage implementation */
