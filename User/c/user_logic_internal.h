// Доработка: Кряжев А., 2026
// Внутренний интерфейс тематических модулей пользовательской логики.
// Входной кадр проверяет граница commands/RX, запись выполняет владелец store.
// Общие курсоры и намерения находятся в device; после сброса их восстанавливает config.
#ifndef USER_LOGIC_INTERNAL_H
#define USER_LOGIC_INTERNAL_H
#include "runtime_internal.h"
#include "../domovoi_diag.h"
// Времена и значения полей определены протоколом, единицы — секунды или миллисекунды.
enum {
  TIMETOREGRESPONSE = 5, TIMETOSTATUSRESPONSE = 15,
  PROTOCOL_VERSION = 1, REG_UNKNOWN_IMEI = 4, REG_NOT_BOUND = 5,
  USB_DIAG_COMMAND = 111, USB_COMM_DIAG_COMMAND = 112,
  FW_TIMEOUT_S = 600, FW_BLOCK_RETRY_MS = 20000, FW_WRITE_TRIES = 3,
  FW_BLOCK_PENDING = 252, FW_BLOCK_OK = 0, FW_BLOCK_END = 253,
  FW_BLOCK_BADWRITE = 254, FW_BLOCK_BADFRAME = 255,
  LOG_PENDING_MAX = 16, LOG_TIME_WAIT = 600, SYNC_QUIET_S = 30,
  SYNC_CONFIG = 2
};
#define ID_TIME_VALID 1600000000u
#define ID_TIME_DONE 0x38u
// Размещение долговечных намерений в резерве device задано прежним образом флеша.
#define STORE_CLEAR_MARK 0x31434C00u
extern uint8_t uniqIDCnt;
extern char tempStrBuffer[128];
extern uint8_t store_save_requested;
extern store_state_s store;
extern uint16_t temp_usbReadIndex, temp_unacked[2];
extern uint8_t sync_pending, sync_quiet, log_pendingCount, log_timeKnown;
extern uint32_t id_nextEnd, log_lastIndex;
extern bool id_endScan;
extern logControl_e server_out_event;
extern uint8_t unblock_active;
enum { INPUT_GATE_CLOSED = 8 }; // Временный номер функции входа; менять только здесь.
void user_unblock_input_poll(void);
void user_unblock_poll(void);
void user_unblock_recheck(bool restore);
extern unsigned int loop_max_ms, loop_stalls;
// Разрешить команду с учётом её владельца и состояния хранения.
bool store_command_allowed(uint8_t cmd);
// Вернуть подтверждённый курсор заданной выгрузки и канала.
uint16_t temp_index(dataChanel_e cch);
// Установить рабочий курсор заданной выгрузки и канала.
void temp_set_index(dataChanel_e cch, uint16_t value);
// Поместить подтверждённые курсоры USB в резерв device.
void upload_store(void);
// Восстановить подтверждённые курсоры из сохранённого device.
void upload_restore(const device_s *saved);
// Обнулить курсоры и неподтверждённые позиции выбранной таблицы.
void upload_reset(uint8_t cmd);
// Вызвать обработчики секундного тика драйверов.
void timers_user_handle(void);
// Передать целый проверенный USB-кадр разбору команд.
// Превратить код Wiegand в десятибайтовый ID и проверить доступ.
void WG_readedID(uint32_t id);
// Пассивный режим 485: посылка n байт — ID; false — доступ не готов, кадр остаётся в буфере.
bool rs485_readedID(const uint8_t *p, uint16_t n);
// Удалить незавершённый кадр по пределу ожидания 30 секунд.
void server_rx_begin_pass(void);
// Выполнить один готовый кадр и сохранить хвост потока.
void server_rx_process(void);
// Распознать кадр, CRC которого уже проверена сборщиком.
// Вернуть свободное место для QIRD-backpressure.
uint16_t user_comm_rx_capacity(void);
// Принять очередную часть ещё не завершённого QIRD.
uint16_t user_comm_rx(const uint8_t *data, uint16_t count);
// Разрешить разбор законченного QIRD либо отбросить его при ошибке.
void user_comm_rx_done(uint16_t count, uint8_t reason);
// Очистить буфер и неподтверждённые серверные ответы при смене TCP.
void user_comm_session(bool open);
// Отметить таблицу для уведомления об окончании синхронизации.
void sync_packet(uint8_t bit);
// Номер текущих суток для счётчиков ID и запись суток прохода в ID (user_ids.c).
uint16_t id_today(void);
void id_set_pass_day(cellID_s *c, uint16_t day);
// Записать окончание синхронизации после периода тишины.
void sync_end_tick(void);
// Обменять рабочую конфигурацию с телом принимаемой команды.
void config_exchange(uint8_t *data);
// Завершить строковые поля на границе нового config.
void config_terminate_strings(void);
// Привести ПИН настроек к числу 0–99999; false — ПИН недопустим.
bool config_pin_number(uint32_t *pin);
// Отправить ожидающий результат обновления при освобождении серверного TX.
void fw_status_poll(void);
// Запомнить последнее неизвестное ID для уведомления администратора.
void unreg_sms_note(const char *id);
// Передать ожидающее уведомление, когда слот SMS свободен.
void unreg_sms_poll(void);
// Передать ожидающий ответ считывателя при свободном TX.
void reader_reply_poll(void);
// Передать идентификацию устройства конфигуратору.
void user_send_identPackToConfigurator(dataChanel_e cch);
// Передать состояние устройства конфигуратору.
void user_send_statusPackToConfigurator(dataChanel_e cch);
// Передать конфигурацию устройства конфигуратору.
void user_send_configToConfigurator(dataChanel_e cch);
// Прочитать долговечную дату первого прохода ID.
uint32_t id_first_pass(cellID_s *c);
// Сохранить дату первого прохода ID в резерв записи.
void id_set_first_pass(cellID_s *c, uint32_t t);
// Определить, закончился ли срок действия ID.
bool id_expired(cellID_s *c);
// Вычислить окончание срока действия ID по первой дате прохода.
uint32_t id_time_end(cellID_s *c);
// Учесть ближайшее окончание срока при очередном обходе.
void id_watch_end(uint32_t end);
// Определить, нужно ли восстановить суточный лимит проходов.
bool id_new_day(cellID_s *c);
// Определить необходимость внеочередного статуса с ограничением частоты.
uint8_t status_event_due(uint16_t status, uint32_t now_ms);
// Запомнить состояние и время переданного статуса.
void status_event_sent(uint16_t status, uint32_t now_ms);
// Вернуть запрет управляющих действий во время обновления.
uint8_t fw_update_quiet(void);
// Проверить внешние параметры образа прошивки.
uint8_t fw_start_rejected(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks);
// Распознать повтор команды начала уже идущей загрузки.
uint8_t fw_same_download(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks, dataChanel_e cch);
// Проверить номер блока и передать его асинхронной записи.
uint8_t fw_take_block(const uint8_t *p, uint16_t n, dataChanel_e cch);
// Восстановить допустимую загрузку из заголовка и сохранённого device.
void fw_resume_after_boot(void);
// Проверить текущую минуту по интервалу расписания.
bool shedule_time_ok(const cellShedule_s *s);
// Окно прочитанного расписания: 0 — закрыто, 1 — открыто, 2 — праздники не прочитаны.
uint8_t shedule_window(const cellShedule_s *s);
// Определить стёртую запись журнала.
bool log_blank(const cellLog_s *cell);
// Пометить текущую запись отменённой без раннего освобождения флеша.
void fw_async_cancel(void);
// Подавить ответ записи при закрытии её канала.
void fw_async_quiet(dataChanel_e channel);
// Зафиксировать уникальный номер ответа текущему блоку.
void fw_async_reply(uint8_t id);
// Продвинуть проверенную запись блока и CRC полного образа.
void fw_storage_poll(uint32_t now);
// Передать первое ожидающее событие владельцу записи журнала.
void log_storage_poll(uint32_t now);
// Преобразовать числовое значение системного события в поле ID.
bool log_addSystemValue(logSystem_e num, int32_t value);
// Снять диагностические поля USB для ответа 111.
void user_send_usbDiag(uint8_t uniqId);
// Снять диагностические поля модема для ответа 112.
void user_send_commDiag(uint8_t uniqId);
// Вернуть достоверное местное время для автомата восстановления.
uint32_t user_comm_local_time(void);
// Вернуть разрешение VoLTE для текущего профиля.
uint8_t user_comm_volte(void);
// Записать системное событие слабого сигнала.
void user_comm_low_signal(uint8_t csq);
// Обновить поколение связи и записать событие её изменения.
bool user_comm_link(uint8_t kind, uint8_t address);
// Сообщить занятость обновлением или телефонным вызовом.
bool user_comm_busy(void);
// Сопоставить адрес канала с сервером настройки.
uint8_t user_comm_server(uint8_t address);
// Выбрать строку сервера и один раз разобрать её адрес.
void comm_endpoints(void);
// Подготовить адреса и запустить модем с обработчиками приложения.
void user_comm_boot(void);
// Перед перезагрузкой: модем выходит из сети, затем снимается питание.
void user_comm_off(void);
// Передать снимок config/device атомарному сохранению.
bool store_snapshot(void), store_cursor_snapshot(void);
// Восстановить незавершённую долговечную очистку после старта.
// Принять новый запрос выгрузки без повторного подтверждения.
bool store_upload_request(uint8_t cmd, dataChanel_e cch, uint8_t uniq);
// Начать долговечное сохранение подтверждённого курсора.
bool store_temp_ack(dataChanel_e cch);
// Сохранить намерение очистки до её первого стирания.
void store_finish(uint8_t cmd);
// Зафиксировать поля отложенного ответа владельца хранения.
void store_command_start(uint8_t cmd, dataChanel_e cch, uint8_t uniq);
// Начать сохранение временно подставленной новой конфигурации.
bool store_config_start(void);
// Освободить буфер ответа у его владельца.
void user_comm_free(uint8_t *buffer, dataChanel_e channel);
// Проверить формат журнала, которым владеет кольцо хранения.
extern bool log_valid(const cellLog_s *);
#endif
