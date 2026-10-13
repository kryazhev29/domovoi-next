// Доработка: Кряжев А., 2026
// Тела ответов SLDN, состояние ожидания и уведомления о синхронизации.
// Заголовок и CRC формирует packet.c; ответ 11 полностью формирует answer11.
// Канал владеет TX после принятия; неподтверждённые данные остаются у своих владельцев.
#include "user_logic_internal.h"
#include "../../Modules/comm_server.h"
#include "prof.h"
enum { STATUS_EVENT_MASK = 0x08FFu, STATUS_EVENT_GAP_MS = 1000u };
uint8_t sync_pending, sync_quiet;
static struct { uint8_t due, percent, code, id; } fw_status_pending;
static struct { uint8_t state; char id[10]; } reader_out;
static uint16_t status_reported;
static uint32_t status_reported_ms;

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

// Собрать произвольное тело ответа через общий кадр SLDN.
void user_send_anyData(uint8_t id, uint8_t* data, uint16_t size, dataChanel_e cch, uint8_t uniqId){
  // весь кадр должен помещаться в 16-битную длину транспорта
  uint16_t len = size + 3;
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);

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

  uint16_t len = 40;
  uint8_t send_local[48];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;

  // ------- меняемый блок ---------
  send_buffer[6] = TIMETOREGRESPONSE; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++;
  send_buffer[8] = 1;

  memcpy(&send_buffer[9], (uint8_t*)&device.IMEIGSM, 15);

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

  switch (cch){
  case CH_USB:
    packet_send(send_buffer, len, CH_USB);
    break;

  case CH_SERVER:
    packet_send(send_buffer, len, CH_SERVER);
    break;

  }
}

// Передать статус 1 и установить срок его подтверждения.
void user_send_statusPack(dataChanel_e cch){

  uint16_t len = 18;
  uint8_t send_local[26];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;

  // ------- меняемый блок ---------
  send_buffer[6] = TIMETOSTATUSRESPONSE; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++;
  send_buffer[8] = 2;

  tmp32 = device.datetime;
  memcpy(&send_buffer[9], (uint8_t*)&tmp32, 4); // время
  memcpy(&send_buffer[13], (uint8_t*)&device.status, 2); // статус
  tmp32 = user_comm_rx_capacity();
  memcpy(&send_buffer[15], (uint8_t*)&tmp32, 2); // Размер буфера входящих данных

  memset(&send_buffer[17], 0, 7); // резерв
  // ------- меняемый блок ---------

  switch (cch){
  case CH_USB:
    packet_send(send_buffer, len, CH_USB);
    break;

  case CH_SERVER:
    comm_server_status_sent(send_buffer[7]);
    status_event_sent((uint16_t)(send_buffer[13] | (send_buffer[14] << 8)), timers_getMillis());
    packet_send(send_buffer, len, CH_SERVER);
    break;

  }
}

// Передать статус 2 с уровнем сигнала и версиями.
void user_send_statusPackSecond(dataChanel_e cch, uint8_t uniqId){

  uint16_t len = 24;
  uint8_t send_local[32];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;

  // ------- меняемый блок ---------
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId;
  send_buffer[8] = 3;

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

  packet_send(send_buffer, len, (uint8_t)cch);
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
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) {
    if (cch == CH_SERVER) {
      fw_status_pending.due=1; fw_status_pending.percent=device.firmwarePercent;
      fw_status_pending.code=code; fw_status_pending.id=uniqId;
    }
    return;
  }

  // ------- меняемый блок ---------
  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId;
  send_buffer[8] = 4;
  //send_buffer[9] = 20; // Отправка блока прошивки
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
  if (!comm_server_online()) return false;
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
void user_send_answer_comand(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId)
{
  uint8_t local[19];
  uint8_t *buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(local)) : local;
  if (!buffer) return;
  answer11(buffer, uniqId, cmd, (uint8_t)cc);
  if (cch == CH_USB) usb_send(buffer, sizeof(local));
  else if (cch == CH_SERVER) comm_tx_commit(sizeof(local));
}

// Передать таймштамп и CRC изменённой таблицы.
void user_send_timeshtamp(shtamp_e sht, uint32_t time, uint16_t crcData, dataChanel_e cch, uint8_t uniqId){

  uint16_t len = 10;
  uint8_t send_local[18];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;

  send_buffer[6] = 255; // таймаут актуальности команды
  send_buffer[7] = uniqId;
  send_buffer[8] = 12;
  send_buffer[9] = sht; // к каким данным относится таймштамп
  memcpy(&send_buffer[10], (uint8_t*)&time, 4); // таймштамп
  memcpy(&send_buffer[14], (uint8_t*)&crcData, 2); // Контрольная сумма структуры

  packet_send(send_buffer, len, (uint8_t)cch);
}

// Передать идентификацию устройства конфигуратору.
void user_send_identPackToConfigurator(dataChanel_e cch){

  uint16_t len = 19;
  uint8_t send_local[27];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;

  // ------- меняемый блок ---------
  send_buffer[6] = 0; // таймаут актуальности команды
  send_buffer[7] = 0;
  send_buffer[8] = 100;
  memcpy(&send_buffer[9], (uint8_t*)&device.IMEIGSM, 15);
  if (config.pinCode == 4321u)
    send_buffer[24] = 1;
  else
    send_buffer[24] = 0;
  // ------- меняемый блок ---------

  packet_send(send_buffer, len, (uint8_t)cch);
}

// Передать состояние устройства конфигуратору.
void user_send_statusPackToConfigurator(dataChanel_e cch){
  if (id_scan_cell) return; // кадр 101 ждёт полный счёт, приёмник удерживает запрос

  uint16_t len = 34;
  uint8_t send_local[42 + sizeof(fault_record)];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return;
  uint32_t tmp32;

  // ------- меняемый блок ---------
  send_buffer[6] = 0; // таймаут актуальности команды
  send_buffer[7] = 0;
  send_buffer[8] = 101;

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
  }

  packet_send(send_buffer, len, (uint8_t)cch);
}

// Передать конфигурацию устройства конфигуратору.
void user_send_configToConfigurator(dataChanel_e cch){

  uint16_t len = sizeof(config_s) + 3;
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);

  if (send_buffer != NULL){

    // ------- меняемый блок ---------
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = 0;
    send_buffer[8] = 106;
    memcpy(&send_buffer[9], (uint8_t*)&config, sizeof(config_s));
    // ------- меняемый блок ---------

    packet_send(send_buffer, len, (uint8_t)cch);

    user_comm_free(send_buffer, cch);
  }
}

// Запросить следующий блок после освобождения владельцев обновления.
bool user_getFWblockFromServer(dataChanel_e cch, uint16_t block){
  if (fw_service_busy()) return false;
  if (flash_job.kind == FX_FWBLOCK && flash_job.result == FX_PENDING) return false;
  uint8_t send_local[16];
  uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;
  if (!send_buffer) return false;
  uint16_t len = 5;

  send_buffer[6] = 20; // таймаут актуальности команды
  send_buffer[7] = uniqIDCnt++;
  send_buffer[8] = 13;
  send_buffer[9] = _1B(block);  // номер блока
  send_buffer[10] = _2B(block);

  if (cch == CH_SERVER) return packet_send(send_buffer, len, CH_SERVER);
  if (cch == CH_USB) packet_send(send_buffer, len, CH_USB);
  return true;
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
