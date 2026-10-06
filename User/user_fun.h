/*
 * Доработка: Кряжев А., 2026
 * Внешний интерфейс логики: доступ, команды, журнал и периодическое обслуживание.
 * Запись флеш проходит через сервис хранилища; его журнал сохраняет восстановление после обрыва питания.
 */
#include "includes.h"

#ifndef USERFUN_H
#define USERFUN_H

/* Обычный RX 485; на время съёмки photo.c подключает буфер из heap. */
#define PORTBUFSIZE             (uint16_t)32

extern cellMem_s mem[10];
void user_debug(char *text);

// валиден ли IMEI
Int8U user_imeivalid(void);
// универсальный пароль на конфигурацию
Int16U user_unipassword (void);
// ожидание пока напряжение питания не станет выше порога

// >>> переопределение функций --------------------------
void user_errorconnection (int8_t handle);
void user_dataconnection (int8_t handle, uint8_t* data, uint16_t siz);
bool user_smsreceived (char* num, char* data);
void user_sendsms(char *mes, char *num);
void user_smssended (void);
bool user_incomingcall (char* num, uint8_t typ);
void user_comanderror (int16_t er);

// инициализация
void user_init (void);
// очистить блок памяти
bool user_clearMemBlock(memBlock_e mb);
// циклический обработчик
void user_loop(void);
// инициализация переменных
void user_initvariables (void);
// Чтение конфигурации с памяти
void user_readconfig(void);
// запись конфигурации в память
void user_writeconfig (void);
// Загрузить настройки по умолчанию
void user_setdefault (void);
// Сброс всего
void user_factoryReset (void);
// чтение сохраненной информации
void user_readsavedinformation (void);
// Безопастное завершение работы прибора
void user_savereset (void);
// Сохранение указателей чтения/записи в памяти
void user_saveindex (void);
// запись конфигурации и параметров device в память
bool user_writeConfigAndDevice(void);
// проверка порта 485
void port_check_data(void);
// проверка входящих данных от модуля
void gsm_check_data(void);
// Отправка данных в формате протокола
void user_send_anyData(uint8_t id, uint8_t* data, uint16_t size, dataChanel_e cch, uint8_t uniqId);
// Пакет 1 - пакет регистрации контроллера на сервере
void user_send_registrationData(dataChanel_e cch);
// Пакет 2 - пакет статуса 1
void user_send_statusPack(dataChanel_e cch);
// Пакет 3 - пакет статуса 2
void user_send_statusPackSecond(dataChanel_e cch, uint8_t uniqId);
// Пакет 4 - пакет статуса обновления FW
void user_send_statusUpdateFW(uint8_t cmd, uint8_t code, dataChanel_e cch, uint8_t uniqId);
// Пакет 5 - контрольные суммы данных структур
void user_send_crcPack(dataChanel_e cch, uint8_t uniqId);
// Пакет 6,16 -  пакет контрольные суммы стуктуры ID абонентов
void user_send_crcMemoryID(uint8_t idx, dataChanel_e cch, uint8_t uniqId);
// Пакет 7 -  трансляция SMS
bool user_send_smsToServer(char* sms);
// Пакет 8 - трансляция ID со считывателей
void user_send_readedIDtoServer(uint8_t* id, uint8_t cmdState);
// Пакет 11 - подтверждение приема команды от сервера
void user_send_answer_comand(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId);
// Пакет 12 - таймштамп структур
void user_send_timeshtamp(shtamp_e sht, uint32_t time, uint16_t crcData, dataChanel_e cch, uint8_t uniqId);
// Пакет 10 - выгрузка журнала событий
void user_send_logRecords(dataChanel_e cch, uint8_t uniqId);
// Пакет 9 - выгрузка ID из временного буфера
void user_send_tempIDs(dataChanel_e cch, uint8_t uniqId);
// Пакет 13 - Запрос конкретного блока прошивки
bool user_getFWblockFromServer(dataChanel_e cch, uint16_t block);
// Разбор данных
void user_parse_comand(uint8_t* ptr, dataChanel_e cch);
// индикация
void user_indication (void);
// опрос входов
void user_readinputs(void);
// вернуть указатель на 10ть последних цифр номера при номере более 10
char* user_last10digitsnumber (char* num);
// узнать текущий день недели 
uint8_t user_getweakday (uint32_t t);

// поиск ID в памяти, если есть то возвращаются параметры ячейки
uint8_t user_findID(char *id, cellID_s *cell);
// поиск ID во временном буфере
bool user_findTempID(char *id);
// добавляет ID во временный буфер
void user_addIdToTempBuffer(char *id, codeType_e tp);
// проверка ID по параметрам и управление выходом
void user_checkIdAndControl(char *id, codeType_e tp);
// управление выходом в соответствии с настройками
bool user_activateOut(bool out1, bool out2, codeType_e tp, char *id);
// считать параметры группы
bool user_getGroup(uint16_t num, cellGroup_s *group);
// считать параметры расписания
bool user_getShedule(uint16_t num, cellShedule_s *shedule);
// проверить праздничный день
uint8_t user_dayIsHoliday(void);
// сколько номеров в памяти
uint16_t user_getNumbersCount(void);
// получаем кол-во ID во временном буфере и тут же обновляем адрес записи
uint16_t user_getTempIDRecordsCount(void);
// проверить расписание
bool user_checkShedule(uint8_t shd);
// обновить данные ID
bool user_updateID(cellID_s *c);
void user_checkIdTimeEnd(void);

// добавить событие в буфер
bool log_addEvent(cellLog_s *cl);
void log_flushPending(void);
void log_setTime(uint32_t now);
void log_checkPending(void);
// получаем кол-во записей в журнале
uint16_t log_getRecordsCount(void);
// добавить системное событие
bool log_addSystemEvent(logSystem_e num);
// добавить системное событие
bool log_addSystemEventID(logSystem_e num, char *id);
// добавить событие доступа
bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt);
bool log_addPhotoEvent(uint32_t number);
bool log_addUpdateEvent(logUpdate_e num, uint32_t reason);


bool user_checkFirmware (uint32_t adr, uint32_t size, uint32_t crc);

void user_storage_poll(void);
bool user_storage_command_ready(void);
bool user_storage_background_ready(void);
bool user_storage_flash_ready(void);

#endif
