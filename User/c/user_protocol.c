// Доработка: Кряжев А., 2026
// Тела ответов SLDN, состояние ожидания и уведомления о синхронизации.
// Заголовок и CRC формирует packet.c; ответ 11 полностью формирует answer11.
// Канал владеет TX после принятия; неподтверждённые данные остаются у своих владельцев.
// Поля device копируются в кадр по таблицам; packet_send молчит на чужом канале.
#include "user_logic_internal.h"
#include "../../Modules/comm_server.h"
#include "prof.h"
#include <stddef.h>
enum { STATUS_EVENT_MASK = 0x28FFu, STATUS_EVENT_GAP_MS = 1000u };
uint8_t sync_pending, sync_quiet;
static struct { uint8_t due, percent, code, id; } fw_status_pending;
// Ответы 11 серверу, не попавшие в занятый слот: очередь на три, лишний отбрасывается.
static struct { uint8_t cmd, code, id; } answer_queue[3];
static uint8_t answer_count;
static struct { uint8_t state; char id[10]; } reader_out;
static uint16_t status_reported;
static uint32_t status_reported_ms;
// Поле кадра из device: позиция в кадре, смещение в device, длина.
typedef struct { uint8_t dst, src, len; } field_s;
#define F(dst, name, len) { dst, offsetof(device_s, name), len }
#define FIELDS(t) t, sizeof(t) / sizeof *(t)
static const field_s reg_fields[] = { // регистрация 1
  F(9, IMEIGSM, 15), F(24, protocolVersion, 2), F(26, datetimeID, 4), F(30, datetimeGroup, 4),
  F(34, datetimeShedule, 4), F(38, datetimeConfig, 4), F(42, datetimeHoliday, 4)
};
static const field_s status_fields[] = { F(9, datetime, 4), F(13, status, 2) }; // статус 1
static const field_s status2_fields[] = { // статус 2
  F(9, datetime, 4), F(13, csq, 1), F(14, balance, 2), F(16, firmwareVersion, 1),
  F(17, buildDate, 4), F(21, bootVersion, 1), F(22, peripheryState, 8)
};
static const field_s ident_fields[] = { F(9, IMEIGSM, 15) }; // идентификация 100
// Слитые поля таблиц лежат в device подряд.
typedef char fields_adjacent[(offsetof(device_s, hardwareVersion) == offsetof(device_s, protocolVersion) + 1u &&
                              offsetof(device_s, cgreg) == offsetof(device_s, creg) + 1u) ? 1 : -1];
static const field_s cfg_status_fields[] = { // статус конфигуратора 101
  F(9, datetime, 4), F(13, status, 2), F(15, firmwareVersion, 1), F(16, buildDate, 4),
  F(20, bootVersion, 1), F(21, hardwareVersion, 1), F(22, csq, 1), F(23, balance, 2),
  F(25, creg, 2), F(27, connected, 1), F(28, netType, 1), F(29, peripheryState, 8),
  F(37, numbersCount, 2), F(39, moduleType, 1)
};

// Отметить таблицу для уведомления об окончании синхронизации.
void sync_packet(uint8_t bit) { sync_pending |= bit; sync_quiet = SYNC_QUIET_S; }

// Записать окончание синхронизации после периода тишины.
void sync_end_tick(void)
{
  if (!sync_quiet || --sync_quiet) return;
  for (uint8_t i = 0; i < 5u; ++i)
    if ((sync_pending & (1u << i)) && log_addSystemEvent((logSystem_e)(LOG_SYS_ENDIDUPDATE + i)))
      sync_pending &= (uint8_t)~(1u << i);
  if (sync_pending) sync_quiet = 1;
}

// Получить серверный TX достаточной ёмкости.
uint8_t *user_comm_acquire(uint16_t needed)
{
  uint16_t capacity; uint8_t *p = comm_tx_acquire(&capacity);
  if (p && needed > capacity) { comm_tx_release(); return NULL; }
  return p;
}

// Освободить буфер ответа у его владельца.
void user_comm_free(uint8_t *buffer, dataChanel_e channel)
{
  if (channel == CH_SERVER) comm_tx_release();
  else work_release(buffer, WORK_REPLY);
}

// Выдать буфер кадра (сервер — общий слот, иначе local) и записать заголовок команды SLDN.
static uint8_t *frame_begin(uint8_t *local, uint16_t size, dataChanel_e cch, uint8_t ttl, uint8_t uniq, uint8_t cmd)
{
  uint8_t *b = cch == CH_SERVER ? user_comm_acquire(size) : local;
  if (b) { b[6] = ttl; b[7] = uniq; b[8] = cmd; }
  return b;
}

// Скопировать поля device в полученный кадр по таблице.
static uint8_t *frame_fields(uint8_t *b, const field_s *f, uint8_t n)
{
  if (b) for (; n; --n, ++f) memcpy(b + f->dst, (const uint8_t *)&device + f->src, f->len);
  return b;
}

// Собрать произвольное тело ответа через общий кадр SLDN.
void user_send_anyData(uint8_t id, uint8_t* data, uint16_t size, dataChanel_e cch, uint8_t uniqId){
  // весь кадр должен помещаться в 16-битную длину транспорта
  uint16_t len = size + 3;
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);
  if (!send_buffer && cch == CH_USB) send_buffer = work_usb_reply(len + 8);

  if (send_buffer != NULL){

    // ------- меняемый блок ---------
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = uniqId;
    send_buffer[8] = id;

    if (size) memcpy(&send_buffer[9], data, size);
    // ------- меняемый блок ---------

    packet_send(send_buffer, len, (uint8_t)cch);

    user_comm_free(send_buffer, cch);
  }
}

// Передать регистрацию с IMEI и таймштампами таблиц.
void user_send_registrationData(dataChanel_e cch){
  uint8_t send_local[48];
  uint8_t *b = frame_fields(frame_begin(send_local, sizeof send_local, cch, TIMETOREGRESPONSE, uniqIDCnt, 1), FIELDS(reg_fields));
  if (!b) return;
  uniqIDCnt++;
  packet_send(b, 40, (uint8_t)cch);
}

// Передать статус 1 и установить срок его подтверждения.
void user_send_statusPack(dataChanel_e cch){
  uint8_t send_local[26];
  uint8_t *b = frame_fields(frame_begin(send_local, sizeof send_local, cch, TIMETOSTATUSRESPONSE, uniqIDCnt, 2), FIELDS(status_fields));
  if (!b) return;
  uniqIDCnt++;
  uint16_t capacity = user_comm_rx_capacity();
  memcpy(&b[15], &capacity, 2); // Размер буфера входящих данных
  memset(&b[17], 0, 7); // резерв
  if (cch == CH_SERVER){
    comm_server_status_sent(b[7]);
    status_event_sent((uint16_t)(b[13] | (b[14] << 8)), timers_getMillis());
  }
  packet_send(b, 18, (uint8_t)cch);
}

// Передать статус 2 с уровнем сигнала и версиями.
void user_send_statusPackSecond(dataChanel_e cch, uint8_t uniqId){
  uint8_t send_local[32];
  uint8_t *b = frame_fields(frame_begin(send_local, sizeof send_local, cch, 255, uniqId, 3), FIELDS(status2_fields));
  if (b) packet_send(b, 24, (uint8_t)cch);
}

// Отправить ожидающий результат обновления при освобождении серверного TX.
void fw_status_poll(void)
{
  uint8_t *b;
  if (!fw_status_pending.due || !comm_server_online()) return;
  b = user_comm_acquire(13);
  if (!b) return;
  b[6] = 255; b[7] = fw_status_pending.id; b[8] = 4;
  b[9] = fw_status_pending.percent; b[10] = fw_status_pending.code;
  if (packet_send(b, 5, CH_SERVER)) fw_status_pending.due = 0;
}

// Передать код результата блока по каналу обновления.
void user_send_statusUpdateFW(uint8_t cmd, uint8_t code, dataChanel_e cch, uint8_t uniqId){
  (void)cmd;
  uint16_t len = 5;
  uint8_t send_local[13];
  uint8_t *send_buffer = frame_begin(send_local, sizeof(send_local), cch, 255, uniqId, 4);
  if (!send_buffer) {
    if (cch == CH_SERVER) {
      fw_status_pending.due=1; fw_status_pending.percent=device.firmwarePercent;
      fw_status_pending.code=code; fw_status_pending.id=uniqId;
    }
    return;
  }

  // ------- меняемый блок ---------
  send_buffer[9] = device.firmwarePercent; // Процент хода обновления или код ошибки
  send_buffer[10] = code; // 0 блок принят, 253 загрузка прекращена, 254 и 255 блок запрошен снова
  // ------- меняемый блок ---------

  packet_send(send_buffer, len, (uint8_t)cch);
}

// Запустить фоновое получение CRC таблиц.
void user_send_crcPack(dataChanel_e cch, uint8_t uniqId){
  service_crc_begin_pack(cch,uniqId);
}

// Запустить фоновое получение CRC блока ID.
void user_send_crcMemoryID(uint8_t idx, dataChanel_e cch, uint8_t uniqId){
  service_crc_begin_id(idx,cch,uniqId);
}

// Передать текст SMS в ограниченном серверном TX.
bool user_send_smsToServer(char *sms)
{
  uint16_t capacity, size = 0;
  uint8_t *b;
  if (!sms || !comm_server_online()) return false;
  b = comm_tx_acquire(&capacity);
  if (!b) return false;
  while (size + 11u <= capacity && sms[size]) ++size;
  if (size + 11u > capacity) { comm_tx_release(); return false; }
  b[6] = 255; b[7] = uniqIDCnt++; b[8] = 7;
  memcpy(b + 9, sms, size);
  if (packet_send(b, size + 3u, CH_SERVER)) return true;
  comm_tx_release();
  return false;
}

// Зафиксировать ответ считывателя до отправки серверу.
void user_send_readedIDtoServer(uint8_t* id, uint8_t cmdState){
  reader_out.state = cmdState; // 1 - ID считан, 255 - время вышло
  if (id == 0) memset(reader_out.id, 0, 10);
  else memcpy(reader_out.id, id, 10);
}

// Передать ожидающий ответ считывателя при свободном TX.
void reader_reply_poll(void)
{
  uint8_t *b;
  if (!reader_out.state) return;
  b = user_comm_acquire(22);
  if (!b) return;
  b[6] = 255; b[7] = uniqIDCnt++; b[8] = 8; b[9] = reader_out.state;
  memcpy(b + 10, reader_out.id, 10);
  if (packet_send(b, 14, CH_SERVER)) reader_out.state = 0;
}

// Передать единый ответ 11 без повторного формирования CRC.
/* Слот сервера бывает занят статусом или регистрацией в том же проходе (плата №1, 23.10.2026:
   ответ на команду 2 терялся); такой ответ ждёт в answer_queue. */
void user_send_answer_comand(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId)
{
  uint8_t local[18];
  uint8_t *buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(local)) : local;
  if (!buffer) {
    if (answer_count < sizeof answer_queue / sizeof *answer_queue) {
      answer_queue[answer_count].cmd = cmd; answer_queue[answer_count].code = (uint8_t)cc;
      answer_queue[answer_count].id = uniqId; answer_count++;
    }
    return;
  }
  answer11(buffer, uniqId, cmd, (uint8_t)cc);
  if (cch == CH_USB) usb_send(buffer, sizeof(local));
  else if (cch == CH_SERVER) comm_tx_commit(sizeof(local));
}

// Передать первый отложенный ответ 11 при свободном слоте сервера.
void answer_reply_poll(void)
{
  uint8_t *b;
  if (!answer_count || !comm_server_online()) return;
  b = user_comm_acquire(18);
  if (!b) return;
  answer11(b, answer_queue[0].id, answer_queue[0].cmd, answer_queue[0].code);
  if (!comm_tx_commit(18)) return;
  --answer_count;
  memmove(answer_queue, answer_queue + 1, answer_count * sizeof *answer_queue);
}

// Новый сеанс TCP: ответы прошлого сеанса с его номерами не отправляются.
void answer_reply_drop(void) { answer_count = 0; }

// Передать таймштамп и CRC изменённой таблицы.
void user_send_timeshtamp(shtamp_e sht, uint32_t time, uint16_t crcData, dataChanel_e cch, uint8_t uniqId){

  uint16_t len = 10;
  uint8_t send_local[18];
  uint8_t *send_buffer = frame_begin(send_local, sizeof(send_local), cch, 255, uniqId, 12);
  if (!send_buffer) return;

  send_buffer[9] = sht; // к каким данным относится таймштамп
  memcpy(&send_buffer[10], (uint8_t*)&time, 4); // таймштамп
  memcpy(&send_buffer[14], (uint8_t*)&crcData, 2); // Контрольная сумма структуры

  packet_send(send_buffer, len, (uint8_t)cch);
}

// Передать идентификацию устройства конфигуратору.
void user_send_identPackToConfigurator(dataChanel_e cch){
  uint8_t send_local[27];
  uint8_t *b = frame_fields(frame_begin(send_local, sizeof send_local, cch, 0, 0, 100), FIELDS(ident_fields));
  if (!b) return;
  b[24] = config.pinCode == 4321u;
  packet_send(b, 19, (uint8_t)cch);
}

// Передать состояние устройства конфигуратору.
void user_send_statusPackToConfigurator(dataChanel_e cch){
  if (id_scan_cell) return; // кадр 101 ждёт полный счёт, приёмник удерживает запрос
  uint16_t len = 34;
  uint8_t send_local[42 + sizeof(fault_record)];
  uint8_t *b = frame_fields(frame_begin(send_local, sizeof send_local, cch, 0, 0, 101), FIELDS(cfg_status_fields));
  if (!b) return;
  /* Необязательное расширение FLT1 после исходных данных статуса. */
  if (fault_record[0] == FAULT_RECORD_MAGIC) {
    uint16_t i;
    for (i = 0; i < sizeof(fault_record); i++)
      b[40 + i] = ((volatile uint8_t*)fault_record)[i];
    len += sizeof(fault_record);
  }
  packet_send(b, len, (uint8_t)cch);
}

// Передать конфигурацию устройства конфигуратору.
void user_send_configToConfigurator(dataChanel_e cch){
  user_send_anyData(106, (uint8_t *)&config, sizeof(config), cch, 0);
}

// Запросить следующий блок после освобождения владельцев обновления.
bool user_getFWblockFromServer(dataChanel_e cch, uint16_t block){
  if (fw_service_busy()) return false;
  if (flash_job.kind == FX_FWBLOCK && flash_job.result == FX_PENDING) return false;
  uint8_t send_local[16];
  uint8_t *send_buffer = frame_begin(send_local, sizeof(send_local), cch, 20, uniqIDCnt, 13);
  if (!send_buffer) return false;
  uniqIDCnt++;
  uint16_t len = 5;

  send_buffer[9] = _1B(block);  // номер блока
  send_buffer[10] = _2B(block);

  bool sent = packet_send(send_buffer, len, (uint8_t)cch);
  return cch != CH_SERVER || sent;
}

// Определить необходимость внеочередного статуса с ограничением частоты.
uint8_t status_event_due(uint16_t status, uint32_t now_ms)
{
  return ((status ^ status_reported) & STATUS_EVENT_MASK) != 0 &&
         (uint32_t)(now_ms - status_reported_ms) >= STATUS_EVENT_GAP_MS;
}

// Запомнить состояние и время переданного статуса.
void status_event_sent(uint16_t status, uint32_t now_ms)
{
  status_reported = status;
  status_reported_ms = now_ms;
}
