/*
 * Доработка: Кряжев А., 2026
 * Внешний интерфейс логики: доступ, команды, журнал и периодическое обслуживание.
 * Запись флеш проходит через сервис хранилища; его журнал сохраняет восстановление после обрыва питания.
 */

#ifndef USERFUN_H
#define USERFUN_H
#include <stdint.h>
#include "arm_comm.h"
#include "structures.h"

extern cellMem_s mem[10];

// Принять кадр канала после проверки его границы и контрольной суммы.
// Принять SMS от модема и выполнить разрешённую текстовую команду.
bool user_smsreceived (char* num, char* data, bool *first);
// Проверить числовой ПИН настроек либо универсальный ПИН действительного IMEI.
bool user_pin_valid(uint32_t pin);
// Поставить исходящую SMS в очередь модема.
bool user_sendsms(char *mes, char *num);
// Проверить номер звонящего по правилам доступа.
bool user_incomingcall (const char* num, uint8_t typ);

// инициализация
void user_init (void);
// циклический обработчик
void user_loop(void);
// инициализация переменных
void user_initvariables (void);
// Применить проверенный снимок заводской очистки без чтения и записи NOR.
void user_factory_variables(device_s *saved);
// Чтение конфигурации с памяти
void user_readconfig(void);
// запись конфигурации в память
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
uint8_t user_findTempID(char *id);
// добавляет ID во временный буфер
bool user_addIdToTempBuffer(char *id, codeType_e tp);
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
void user_ids_boot(void);
// получаем кол-во ID во временном буфере и тут же обновляем адрес записи
uint16_t user_getTempIDRecordsCount(void);
// проверить расписание: 0 — закрыто, 1 — открыто, 2 — отказ чтения
uint8_t user_checkShedule(uint8_t shd);
// обновить данные ID
bool user_updateID(cellID_s *c);
// Удалить истёкшие ID в фоновом проходе.
void user_checkIdTimeEnd(void);

// добавить событие в буфер
bool log_addEvent(cellLog_s *cl);
// Перенести ожидающие события из ОЗУ в кольцевой журнал.
void log_flushPending(void);
// Подставить полученное время событиям, записанным до синхронизации.
void log_setTime(uint32_t now);
// Продвинуть накопленные события при освобождении флеша.
void log_checkPending(void);
// получаем кол-во записей в журнале
uint16_t log_getRecordsCount(void);
// добавить системное событие
bool log_addSystemEvent(logSystem_e num);
// добавить системное событие
bool log_addSystemEventID(logSystem_e num, char *id);
// добавить событие доступа
bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt);
// Записать номер сохранённой фотографии.
bool log_addPhotoEvent(uint32_t number);
// Записать результат и причину обновления прошивки.
bool log_addUpdateEvent(logUpdate_e num, uint32_t reason);

bool user_checkFirmware (uint32_t adr, uint32_t size, uint32_t crc);

// Продвинуть единственного активного владельца флеша.
void user_storage_poll(void);
// Разрешить новую команду записи при свободном хранилище.
bool user_command_ready(const uint8_t *ptr, bool usb_input);
// Проверить, доступен ли флеш обработчикам вызова и SMS.
#include "flash_exec.h"
bool user_access_ready(const char *id);

#endif
