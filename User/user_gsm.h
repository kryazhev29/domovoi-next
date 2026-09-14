// Пользовательские функции

#include "includes.h"

#ifndef USERGSM_H
#define USERGSM_H

#define MAX_CONNECTIONS (uint8_t)4 //ограничение по max кол-ву соединений

typedef enum{
  GSM_STOPED = 0,
  GSM_POWERON,
  GSM_POWEROFF,
  GSM_RESTART,
  GSM_READY,
  GSM_READIMEI, 
  
  GSM_REGTOGSM,
  GSM_REGTOGPRS,
  GSM_WORK,
  GSM_SOFTSIMSWITCH,

  GSM_REREGGPRS
}gsmstate_e;

typedef enum{
  E_NONE = 0,
  E_READY,
  E_GSMREG,
  E_NOGSM,
  E_GPRSREG,
  E_NOGPRS,
  E_NOSIM,
  E_NOTANSWER,
  E_PWRON,
  E_PWROFF,
  E_SELECTSIM,
  E_PINGTIMEOUT,
  E_IMEI,
  E_MODULETYPE
}gsmevent_e;

typedef enum{
  CE_OK = 0,
  CE_CONNECTED,
  CE_PARAM,
  CE_IP,
  CE_NOTCONNECTED,
  CE_BUSY,
  CE_NOINTERNET
}CONERROR_e;

// структура параметров подключения
typedef struct{
  char* ip; // ip/домен
  uint16_t port; // порт
  uint16_t timeout; // таймаут подключения
  uint8_t maxConnectCount; // кол-во попыток подключиться  
  struct{
    uint8_t auto_reconnect:1; // авто переподключение
    uint8_t wait_sendData:1; // перед отправкой ждать пока предыдущие не отправятся
    uint8_t res:6;
  };   
  void (*Connected) (int8_t handle);
  void (*Disconnected) (int8_t handle, uint8_t arec);
  void (*Error) (int8_t handle);
  void (*DataReceived) (int8_t handle, uint8_t* data, uint16_t siz);
}ConnectParam_t;

// структура подключения
typedef struct{
  ConnectParam_t param;
  int8_t handle; // хэндл    
  struct{
    uint8_t constate:1; // состояние подключения
    uint8_t busy:1;
    uint8_t sended:1; // данные переданы
    uint8_t sending:1; // идет передача
    uint8_t isOpen:1; // ручное открытие
    uint8_t isClose:1; // ручное/закрытие
    uint8_t qisackWait:1; // ждем ответ QISACK
    uint8_t res:1;
  };
  uint8_t count; // счетчик подключений
  int16_t timer; // таймер времени
  int16_t sendtimeout;
}Connection_t;

// парамтры работы
typedef struct{
  struct{
    uint8_t enable:1; // включен
    uint8_t autoswitch_sim:1; // авто переключение СИМ
    uint8_t autorestart:1; // авто перезагрузка
    uint8_t gprs_enable:1; // gprs включен
    uint8_t soft_simswitch:1; // программное переключение СИМ
    uint8_t textMode:1; // текстовый режим СМС
    uint8_t reserved:2;
  };  
  void (*SMSReceived) (char* num, char* data);
  void (*SMSSended) (void);
  bool (*IncomingCall) (char* num, uint8_t typ);
  void (*ComandError) (int16_t er);
  moduleType_e moduleType;
}GsmParam_t;

// параметры работы
typedef struct{
  GsmParam_t param;
  struct{
    uint8_t restart:1; // перезагрузить
    uint8_t ready:1; // модуль готов 
    uint8_t gsmreg:1; // есть рег в gsm
    uint8_t gprsreg:1; // есть рез в gprs
    uint8_t rouming:1; // роуминг
    uint8_t internet:1; // есть интернет
    uint8_t simindex:1; // СИМ 0 или 1
    uint8_t imei_readed:1; // imei считан
  };
  uint8_t state_mashine; // state mashine
  uint32_t timer_ms; // счет времени
  uint16_t local_timer; // интервальный таймер
  int16_t answer_timeout; // время на ответ
  uint8_t resetCnt; // кол-во перезапусков до полного reset
  
  char imei[16]; // imei
  char incomingNumber[20]; // номер звонящего
  uint8_t csq; // уровень сигнала
  uint8_t ber; // качество

  struct{  
    uint8_t ring:1;
    uint8_t clip:1;    
    uint8_t ntpconnected:1;
    uint8_t busySend:1;
    uint8_t opname_readed:1;
    uint8_t res:3;
  };

  char opname[16];
  uint32_t datetime;
  uint8_t net_type;  
  uint8_t errorcount;
  uint8_t noanswercount;
  int16_t ringTimeout;
  //uint16_t dataToSend;
  uint8_t connectCnt;
}UserGsm_t;


// переключить/установить СИМ карту ------------
void usergsm_setsim (uint8_t sim);

// обработчик событий
void usergsm_event(gsmevent_e ev);

// сброс оперативных переменных
static void usergsm_clearStructBits(void);

// инициализация переменных
void usergsm_init (GsmParam_t *param);

// алгоритм работы с модулем gsm
void usergsm_work (uint16_t ms);

// переключить сим карту -------------------
// sim = 255 - следующая
// true - СИМ карта переключена
bool usergsm_selectsim (uint8_t sim);

// перезапустить модуль gsm ---------------------
void usergsm_restart (void);

// управление питанием gsm модуля ------------------
bool usergsm_power (Int8U p);

// перерегистрация в gprs
void usergsm_rereggprs (void);

// запрос уровня сигнала
uint8_t usergsm_getcsq (void);

// запрос уровня ошибок
uint8_t usergsm_getber (void);

// запрос imei
char* usergsm_getimei (void);

// текущая СИМ карта
uint8_t usergsm_currentsim (void);

// есть интернет или нет
uint8_t usergsm_getinternetstate (void);

// состояние подключения NTP
uint8_t usergsm_getntpstate (void);

// текущее состояние
uint8_t usergsm_getStateMachine(void);

// сброс переменных о состоянии регистрации
void usergsm_clearregvars (void);

// тип сети
uint8_t usergsm_getNetType(void);

// запросить время сети
void usergsm_getClock(void);

// есть входящий вызов или нет
bool usergsm_ring(void);

// есть входящий вызов или нет
void usergsm_endCall(void);

// передача занята
bool usergsm_busySend(void);

void usergsm_debug(char* ptr);

////////////////////////////////////////////////////////////////////////////////
// проверка индекса
static bool connection_validparam (int8_t h);

// сброс переменных
static void connection_clear (Connection_t* p);

// подключение установлено
static void connection_connected (int8_t h);

// подключение разорвано
static void connection_disconnected (int8_t h);

// данные приняты
static void connection_received (int8_t h, uint8_t* data, uint16_t siz);

// отправленные данные подтверждены
static void connection_dataAcked (uint32_t cnt);

// инициализация параметров подключения
void connection_init (Connection_t* p);

// создать подключение
int8_t connection_create (ConnectParam_t* p);

// удалить подключение
int8_t connection_delete (int8_t h);

// открыть подключение
int8_t connection_open (int8_t h);

// закрыть подключение
int8_t connection_close (int8_t h);

// переподключить соединение
int8_t connection_reopen (int8_t h);

// состояние подключения
int8_t connection_state (int8_t h);

// отправить данные
int8_t connection_send (int8_t h, uint8_t* data, uint16_t siz);

// отправлены данные или нет
bool connection_datasended (int8_t h);

// обаботчик процесса подключений
void connection_timer (uint8_t ms);

// нет передаваемых данных по соединениям
bool connection_allDataSended (void);

// удалить все подключения
void connection_deleteAll(void);
////////////////////////////////////////////////////////////////////////////////

#endif