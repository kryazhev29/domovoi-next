// Доработка: Кряжев А., 2026. Пассивный режим 485: посылка -> ID, захват команды 6, маршруты RFID/прочие.
// Зависимости моделируются, код проверяемого модуля вставляет стенд (run_host.py work rs485id).
// Проверки не используют физическую плату и сеть.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* ТИПЫ ПРОТОКОЛА */
#define ID_TIME_VALID 1600000000u
enum { OUT_OFF = 0, OUT_ON, OUT_INV, OUT_IMP };
typedef struct { uint16_t event; uint8_t type, count, outputs, started; } store_io_s;
static struct { union { store_io_s storeio; } data; } flash_job;
static logControl_e server_out_event = LOG_CON_OUTONBYAPI;
static uint8_t unblock_active;
static uint8_t wf_line[96]; /* строка моста прошивки: вне моста — буфер пассивного приёма */
static struct {
  uint8_t rs485Mode;
  struct { uint8_t address, type; } rs485Periphery[8];
  struct { uint8_t useInBlock, mode, onCmdServer, onWiegand, onCamera, onRFID, onOther, onRF433, tImpulse, tPause, cnt; } rele[2];
} config;
static struct {
  uint32_t datetime;
  uint8_t timeToGetID, sourceGetID, addressGetID, getPhoto;
  struct { uint8_t openAnyUser, autoRegistration; } status;
  struct { uint8_t openAllByShedule, smsUnreg; } triggers;
} device;
/* Наблюдаемое: запрос базы, реле, журнал, ответ 8. */
static bool read_allowed = true, access_ready = true;
static uint8_t find_result = 1, temp_result, group_onRFID = 1, group_onWiegand = 1, group_onAuto = 1, group_onPhone = 1;
static unsigned finds, loads, events, replies, notes, photos;
static char find_id[11], event_id[11], reply_id[11];
static uint8_t load_out[4], load_mode[4], event_type[4], reply_state;
static bool flash_exec_read_allowed(void) { return read_allowed; }
static bool __attribute__((unused)) user_access_ready(const char *id) { CHECK(strlen(id) == 10); return access_ready; }
static uint8_t user_findTempID(char *id) { (void)id; return temp_result; }
static uint8_t user_findID(char *id, cellID_s *cell)
{
  memcpy(find_id, id, 10); find_id[10] = 0; ++finds;
  memset(cell, 0, sizeof(*cell)); cell->group = 1; cell->cntPassOnDay = 255;
  return find_result;
}
static bool user_addIdToTempBuffer(char *id, codeType_e tp) { (void)id; (void)tp; return false; }
static bool user_getGroup(uint16_t num, cellGroup_s *g)
{
  CHECK(num == 1); memset(g, 0, sizeof(*g)); g->index = 1;
  g->onRFID = group_onRFID; g->onWiegand = group_onWiegand; g->onAuto = group_onAuto; g->onPhone = group_onPhone;
  return true;
}
static bool user_getShedule(uint16_t num, cellShedule_s *s) { (void)num; (void)s; CHECK(0); return false; }
static uint8_t user_checkShedule(uint8_t shd) { (void)shd; return 1; }
static uint8_t shedule_window(const cellShedule_s *s) { (void)s; return 1; }
static bool id_expired(cellID_s *c) { (void)c; return false; }
static void unreg_sms_note(const char *id) { (void)id; ++notes; }
static bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt)
{
  (void)cnt; CHECK(events < 4); event_type[events++] = (uint8_t)num;
  memcpy(event_id, id, 10); event_id[10] = 0; return true;
}
static uint8_t out_getmodestate(uint8_t out) { (void)out; return 0; }
static void out_load(uint8_t out, uint8_t mode, uint16_t activ, uint16_t pause, uint8_t period)
{ (void)activ; (void)pause; (void)period; CHECK(loads < 4); load_out[loads] = out; load_mode[loads++] = mode; }
static bool user_updateID(cellID_s *c) { (void)c; CHECK(0); return false; }
static bool id_new_day(cellID_s *c) { (void)c; return false; }
static uint32_t id_first_pass(cellID_s *c) { (void)c; return 1; }
static void id_set_first_pass(cellID_s *c, uint32_t t) { (void)c; (void)t; CHECK(0); }
static void id_watch_end(uint32_t end) { (void)end; }
static uint32_t id_time_end(cellID_s *c) { (void)c; return 0; }
static uint8_t photo_start(uint8_t address) { (void)address; ++photos; return 0; }
static void user_send_readedIDtoServer(uint8_t *id, uint8_t state)
{ ++replies; reply_state = state; memcpy(reply_id, id, 10); reply_id[10] = 0; }
/* Порт главного цикла для port_check_data: кадр в wf_line, пауза приёма уже прошла. */
static unsigned polls, clears; static uint8_t port_state = 1, port_receiving, port_wifi; static uint16_t port_count;
static void photo_poll(void) { ++polls; }
static bool __attribute__((unused)) comm_channel_wifi(void) { return port_wifi != 0; }
static uint8_t uart_getstate(void) { return port_state; }
static bool uart_isReceiving(void) { return port_receiving != 0; }
static uint16_t uart_received(void) { return port_count; }
static void uart_clear(void) { ++clears; port_count = 0; port_state = 0; }
void user_checkIdAndControl(char *id, codeType_e tp);
bool user_activateOut(bool out1, bool out2, codeType_e tp, char *id);
void user_access_complete(uint8_t outputs, codeType_e tp, char *id, uint8_t cnt, bool started);
bool rs485_readedID(const uint8_t *p, uint16_t n);
void WG_readedID(uint32_t id);
void port_check_data(void);
/* КОД МОДУЛЯ */
static void reset(void)
{
  memset(&config, 0, sizeof(config)); memset(&device, 0, sizeof(device)); memset(wf_line, 0, sizeof(wf_line));
  device.datetime = ID_TIME_VALID + 1;
  config.rele[0].tImpulse = config.rele[0].cnt = config.rele[1].tImpulse = config.rele[1].cnt = 1;
  read_allowed = access_ready = true; find_result = 1; temp_result = 0;
  group_onRFID = group_onWiegand = group_onAuto = group_onPhone = 1; unblock_active = 0;
  finds = loads = events = replies = notes = photos = polls = clears = 0;
  find_id[0] = event_id[0] = reply_id[0] = 0; reply_state = 0;
  port_state = 1; port_receiving = port_wifi = 0; port_count = 0;
}
/* Посылка с линии в пассивном режиме, реле 1 разрешено «от 485 прочие». */
static bool frame(const char *data, size_t n)
{
  reset(); config.rele[0].onOther = 1;
  return rs485_readedID((const uint8_t *)data, (uint16_t)n);
}
/* Посылка дала проход с этим ID: запрос базы, импульс реле 1, событие 70, без ответа 8. */
static void passes(const char *data, size_t n, const char *id)
{
  CHECK(frame(data, n));
  CHECK(finds && !strcmp(find_id, id));
  CHECK(loads == 1 && load_out[0] == 0 && load_mode[0] == OUT_IMP);
  CHECK(events == 1 && event_type[0] == LOG_CON_OUTONBYRFID && !strcmp(event_id, id));
  CHECK(!replies && !notes);
}
#define PASSES(s, id) passes(s, sizeof(s) - 1, id)
/* Посылка отброшена без записи: ни запроса базы, ни реле, ни событий; кадр считается принятым. */
static void dropped(const char *data, size_t n)
{
  CHECK(frame(data, n));
  CHECK(!finds && !loads && !events && !replies && !notes);
}
#define DROPPED(s) dropped(s, sizeof(s) - 1)
/* Поднести ID к линии через главный цикл: кадр из n байт уже в wf_line. */
static void line(const char *data, size_t n)
{
  memcpy(wf_line, data, n); port_count = (uint16_t)n; port_state = 1;
}
int main(void)
{
  char big[96];
  char id[11] = "0012345678";
  /* Разбор: последние 10 печатных символов, нули слева, управляющие и пробелы — разделители. */
  PASSES("0012345678\r\n", "0012345678");
  PASSES("0012345678", "0012345678");
  PASSES("ID:0012345678\r\n", "0012345678");
  PASSES("12345678\r\n", "0012345678");
  PASSES("5\n", "0000000005");
  PASSES(" 1234 5678 90 ", "1234567890");
  PASSES("\x02 12345 \x03\r\n", "0000012345");
  PASSES("ABCDEFGHIJK", "BCDEFGHIJK");
  PASSES("12\x7f" "34", "0000001234");
  PASSES("a-b_c~!", "000a-b_c~!");
  PASSES("0012345678\r\n0012345679\r\n", "0012345679"); /* две посылки в одном кадре: последняя */
  /* Мусор: только разделители или байт вне ASCII — без записи. */
  DROPPED("\r\n"); DROPPED(""); DROPPED("\x01\x02\x7f \t"); DROPPED("\xff\x00");
  DROPPED("12\x80" "34"); DROPPED("0012345678\xc0"); DROPPED("\x80" "0012345678");
  /* Слишком длинная: полный буфер 96 Б отброшен, 95 Б принято (последние 10). */
  memset(big, '1', sizeof(big)); dropped(big, sizeof(big)); dropped(big, 200);
  big[94] = '2'; passes(big, 95, "1111111112");
  /* Доступ не готов: кадр не принят (остаётся в буфере), ничего не записано. */
  reset(); config.rele[0].onOther = 1; access_ready = false;
  CHECK(!rs485_readedID((const uint8_t *)"0012345678\r\n", 12) && !finds && !loads && !events && !replies);
  /* Захват команды 6 с источником 3: ответ 8 и событие 78, доступа нет, захват снят; адрес не важен. */
  reset(); config.rele[0].onOther = 1; device.timeToGetID = 5; device.sourceGetID = 3; device.addressGetID = 7;
  CHECK(rs485_readedID((const uint8_t *)"ID:0012345678\r\n", 15));
  CHECK(replies == 1 && reply_state == 1 && !strcmp(reply_id, "0012345678"));
  CHECK(events == 1 && event_type[0] == LOG_CON_RECVIDONREADER && !strcmp(event_id, "0012345678"));
  CHECK(!loads && !finds && !device.timeToGetID && !device.sourceGetID);
  /* Мусор при захвате захват не снимает. */
  reset(); device.timeToGetID = 5; device.sourceGetID = 3;
  CHECK(rs485_readedID((const uint8_t *)"\r\n", 2) && !replies && !events && device.timeToGetID == 5);
  /* Чужой захват (Wiegand) не даёт доступа по 485 и не снимается. */
  reset(); config.rele[0].onOther = 1; device.timeToGetID = 5; device.sourceGetID = 1;
  CHECK(rs485_readedID((const uint8_t *)"0012345678\r\n", 12));
  CHECK(!replies && !events && !loads && !finds && device.timeToGetID == 5 && device.sourceGetID == 1);
  /* Wiegand при захвате с 485 — так же; при захвате с Wiegand — ответ 8; без захвата — проход, событие 71. */
  reset(); config.rele[0].onWiegand = 1; device.timeToGetID = 5; device.sourceGetID = 3;
  WG_readedID(0x42); CHECK(!replies && !events && !loads && device.timeToGetID == 5);
  reset(); config.rele[0].onWiegand = 1; device.timeToGetID = 5; device.sourceGetID = 1;
  WG_readedID(0x42);
  CHECK(replies == 1 && !strcmp(reply_id, "0000000042") && events == 1 && event_type[0] == LOG_CON_RECVIDONREADER);
  CHECK(!loads && !device.timeToGetID && !device.sourceGetID);
  reset(); config.rele[0].onWiegand = 1; WG_readedID(0x42);
  CHECK(loads == 1 && events == 1 && event_type[0] == LOG_CON_OUTONBYWIEGAND && !strcmp(event_id, "0000000042"));
  /* Маршрут RFID: реле по onRFID, группа по onRFID, событие 70. */
  reset(); config.rele[0].onRFID = 1; user_checkIdAndControl(id, CT_RFID);
  CHECK(loads == 1 && load_out[0] == 0 && events == 1 && event_type[0] == LOG_CON_OUTONBYRFID && !strcmp(event_id, id));
  reset(); config.rele[0].onOther = config.rele[0].onWiegand = config.rele[0].onCamera = config.rele[0].onCmdServer = 1;
  user_checkIdAndControl(id, CT_RFID);
  CHECK(!loads && !events && !finds); /* прочие, Wiegand, камера и сервер RFID не разрешают */
  reset(); config.rele[1].onRFID = 1; user_checkIdAndControl(id, CT_RFID);
  CHECK(loads == 1 && load_out[0] == 1);
  reset(); config.rele[0].onRFID = 1; group_onRFID = 0; user_checkIdAndControl(id, CT_RFID);
  CHECK(!loads && !events); /* группа запрещает RFID */
  reset(); config.rele[0].onRFID = 1; group_onWiegand = group_onAuto = group_onPhone = 0; user_checkIdAndControl(id, CT_RFID);
  CHECK(loads == 1); /* другие разрешения группы RFID не трогают */
  /* Прочие (пассивный 485): реле по onOther, группа не ограничивает, событие 70. */
  reset(); config.rele[0].onOther = 1; group_onRFID = group_onWiegand = group_onAuto = group_onPhone = 0;
  user_checkIdAndControl(id, CT_OTHER);
  CHECK(loads == 1 && events == 1 && event_type[0] == LOG_CON_OUTONBYRFID && !strcmp(event_id, id));
  reset(); config.rele[0].onRFID = config.rele[0].onWiegand = config.rele[0].onCamera = 1; user_checkIdAndControl(id, CT_OTHER);
  CHECK(!loads && !events && !finds);
  /* Прежние пути не изменились. */
  reset(); config.rele[0].onWiegand = 1; user_checkIdAndControl(id, CT_WIEGAND);
  CHECK(loads == 1 && events == 1 && event_type[0] == LOG_CON_OUTONBYWIEGAND);
  reset(); config.rele[0].onWiegand = 1; group_onWiegand = 0; user_checkIdAndControl(id, CT_WIEGAND); CHECK(!loads);
  reset(); config.rele[0].onCamera = 1; user_checkIdAndControl(id, CT_AUTO);
  CHECK(loads == 1 && events == 1 && event_type[0] == LOG_CON_OUTONBYAVTO);
  reset(); config.rele[0].onCamera = 1; group_onAuto = 0; user_checkIdAndControl(id, CT_AUTO); CHECK(!loads);
  reset(); user_checkIdAndControl(id, CT_PHONE);
  CHECK(loads == 2 && events == 1 && event_type[0] == LOG_CON_OUTONBYPHONE);
  /* ID не в базе по 485: событие 65 и заметка для SMS, реле молчат. */
  reset(); config.rele[0].onOther = 1; find_result = 0; device.triggers.smsUnreg = 1;
  CHECK(rs485_readedID((const uint8_t *)"0099999999", 10));
  CHECK(!loads && events == 1 && event_type[0] == LOG_CON_IDNOTINBASE && !strcmp(event_id, "0099999999") && notes == 1);
  /* useInBlock закрывает реле для 485 так же, как для остальных. */
  reset(); config.rele[0].onOther = 1; config.rele[0].useInBlock = 1; user_checkIdAndControl(id, CT_OTHER);
  CHECK(!loads && !events);
  /* Главный цикл: кадр после паузы разбирается и освобождается; пустой порт не трогается. */
  reset(); config.rele[0].onOther = 1; line("0012345678\r\n", 12);
  port_check_data(); CHECK(polls == 1 && clears == 1 && loads == 1 && !strcmp(find_id, "0012345678"));
  port_check_data(); CHECK(polls == 2 && clears == 1 && loads == 1);
  /* Неготовый доступ держит кадр в буфере до следующего прохода. */
  reset(); config.rele[0].onOther = 1; line("0012345678\r\n", 12); access_ready = false;
  port_check_data(); CHECK(!clears && !loads && port_count == 12);
  access_ready = true; port_check_data(); CHECK(clears == 1 && loads == 1);
  /* Приём продолжается, мост, съёмка, занятая флеш — кадр не трогается. */
  reset(); config.rele[0].onOther = 1; line("0012345678\r\n", 12);
  port_receiving = 1; port_check_data(); CHECK(polls == 1 && !clears && !loads);
  port_receiving = 0; port_wifi = 1; port_check_data(); CHECK(polls == 2 && !clears && !loads);
  port_wifi = 0; device.getPhoto = 5; port_check_data(); CHECK(polls == 3 && !clears && !loads);
  device.getPhoto = 0; read_allowed = false; port_check_data(); CHECK(polls == 3 && !clears && !loads);
  read_allowed = true; port_check_data(); CHECK(polls == 4 && clears == 1 && loads == 1);
  /* Активный режим: кадр освобождается без разбора (сюда встанет опрос периферии). */
  reset(); config.rele[0].onOther = 1; config.rs485Mode = 1; line("0012345678\r\n", 12);
  port_check_data(); CHECK(clears == 1 && !loads && !finds);
  /* Полный буфер: отброшен и освобождён. */
  reset(); config.rele[0].onOther = 1; memset(big, '1', sizeof(big)); line(big, sizeof(big));
  port_check_data(); CHECK(clears == 1 && !loads && !finds);
  CHECK(!photos);
  puts("rs485id: last 10 printable chars, zero padding, garbage/long frames dropped, capture 6/source 3, RFID/other routes, port poll PASS");
  return 0;
}
