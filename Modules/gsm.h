// Модуль работы с gsm модулем
// Автор: Корякин Сергей

#include "main.h"

#ifndef GSM_H
#define GSM_H

typedef enum{
    READY = 0, 
    IMEI, 
    CLIP, 
    RING, 
    MORING, 
    SMS, 
    MOCONNECT, 
    NOCARY, 
    BUSY, 
    NOANSWER, 
    CREG, 
    CGREG,
    PDPDEACT,
    CONNECTED, 
    FAILCONNECT, 
    CLOSECONNECT,
    DATA, 
    SMSSENDED, 
    CSQ, 
    NOSIM, 
    GETPIN,
    CUSD, 
    DTMF, 
    TEMP, 
    CENG, 
    PREINIT, 
    ATERROR, 
    ATOK, 
    PWRDOWN, 
    OPNAME, 
    QPING, 
    RTCCLOCK, 
    QISACK,
    NTPRESP, 
    URC_NETTYPE,
    SETMODULETYPE
}gsmresp_e;

typedef enum{
  M_OTHER = 0, 
  M_UG95 = 1, 
  M_SLM336 = 2, 
  M_M66 = 3,
  
  M_UNDEF = 255
}moduleType_e;

typedef enum{ 
  PO_NONE = 0, 
  PO_OFF = 1, 
  PO_ON = 2, 
  PO_ONDELAY = 3
}pwrOp_e;

typedef enum{
  NT_AUTO = 0, 
  NT_GSM = 1, 
  NT_WCDMA = 2, 
  NT_LTE = 3,
  NT_TDSCDMA = 4,
  NT_UMTS = 5,
  NT_CDMA = 6,
  NT_HDR = 7,
  NT_CDMAEVDO = 8
}netType_e;

typedef struct {
  struct{
  uint16_t pwr:1; // питание
  uint16_t ready:1; // готовность модуля
  uint16_t getpin:1; // запрос пин кода
  uint16_t getimei:1; // запрос имей
  uint16_t nosim:1; // флаг того что нет сим карты
  uint16_t muxmode:1; // режим множественных подключений
  uint16_t init:1; // инициализация пройдена
  uint16_t textMode:1; // режим 0 - PDU, 1 - Text
  uint16_t pdpAct:1;
  uint16_t res:7;
  };
  Int8U creg;
  Int8U cgreg;
  pwrOp_e operation;
  Int16U timer; // таймер управления процессом вкл/выкл питания
  Int8U dataconnectionindex; // индекс подключения при приеме данных в режиме множественных подключений
  moduleType_e moduleType;
} GsmStr; // 9 байт


// инициализация переменных
void gsm_initvar (uint8_t multiConnection, uint8_t textMode);

// Процесс 
void gsm_pwrProc(Int8U ms);

// Управление питанием модуля
bool gsm_power (Int8U pwr);

// выполнить команду
char* gsm_runcmd(char *str, char *resp);

// Инициализация модуля
void gsm_init(void);

// Обработка данных c модуля gsm
void gsm_read(char* ptr, Int16U size);

// Инициализация GPRS
void gsm_initgprs(char *apn, char *login, char *pass);

// Запрос IMEI модуля
void gsm_getimei(void);

// Запрос уровня сигнала
void gsm_getcsq(void);

// Отправка данных по GPRS
void gsm_senddata (pInt8U buf, Int16U len, Int8U c);

// Подключить/отключить GPRS
// 0 - отключить 1 - включить
void gsm_setgprs (Int8U onoff);

// Установить подключение
void gsm_connect (char *ip, Int16U port, Int8U c);

// Разорвать подключение
void gsm_disconnect (Int8U c);

// Набрать номер
void gsm_call(char *num);

// Отправить USSD запрос
void gsm_sendussd (char* str);

// Отправка СМС сообщения
// num - номер, ptr - строка 
void gsm_sendsms (char *num, char *str);

// Запрос о состоянии регистрации в сети
void gsm_getregstate (void);

// Обработчик принятых данных
void gsm_dataread (gsmresp_e gsmop, char* ptr1, char* ptr2, uint32_t v1, int32_t v2);

// Завершить звонок
void gsm_endcall (void);

// Принять вызов
void gsm_answercall (void);

// Отправить PIN code на СИМ карту
void gsm_simpin (char* pin, char* newpin);

// Запрос состояния питания
uint8_t gsm_getPowerState (void);

// узнать номер операции
//uint8_t gsm_getoperation(void);

// программное переключение СИМ карты
void gsm_selectSIM (uint8_t i);

// режим функциональности
void gsm_setFUN (uint8_t f);

// Ping remote server
void gsm_ping (char* host);

// get RTC clock 
void gsm_getclock (void);

// set NTP server 
void gsm_setNTPserver (char* ip, uint16_t p);

// инфо о переданных/подтвержденных/не подтвержденных данных
void gsm_getinformationsending (uint8_t c);

// включить синхронизацию времени с временем сети
void gsm_enablenetworktime (void);

// Чтение СМС из памяти
void gsm_readsms(uint8_t idx);

// Удаление СМС из памяти
void gsm_deletesms(uint8_t idx);

// установить тип используемой сети
void gsm_setNetworkType(netType_e nt);

// запрос типа сети
void gsm_getNetworkType(void);

// состояние активации gprs CGATT
uint8_t gsm_getCGATT(void);

// запрос состояния PDP контекста
void gsm_getPDPState(void);

// тип модуля
moduleType_e gsm_getModuleType(void);

// тип модуля - установка
void gsm_setModuleType(moduleType_e mt);

void gsm_user_debug (char* ptr);

#endif