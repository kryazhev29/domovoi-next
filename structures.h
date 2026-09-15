// Файл описания структур данных

#ifndef _STRUCTURES_H
#define _STRUCTURES_H

#define ON      1
#define OFF     0

#define _1B(a) (Int8U)(a)      /* берем младший байт*/
#define _2B(a) (Int8U)(a>>8)   /* берем второй байт*/
#define _3B(a) (Int8U)(a>>16)  /* берем третий байт*/
#define _4B(a) (Int8U)(a>>24)  /* берем четвертый байт*/

#define _LT(a) (Int8U)(a&0x0f)
#define _HT(a) (Int8U)((a>>4)&0x0f)

#define ONE_DAY         (3600*24)
#define ONE_WEEKDAYS    7
#define ONE_WEEK        (ONE_DAY * ONE_WEEKDAYS)


// Тип события журнала
typedef enum{
  LT_SYSTEM = 0,
  LT_COMAND,
  LT_TRANSFER,
  LT_CONTROL
}logEventType_e;

// Системное событи журнала
typedef enum{
  LOG_SYS_DEFCONF = 1,
  LOG_SYS_GOTOUPDATE,
  LOG_SYS_RUNUPDATE,
  LOG_SYS_ENDUPDATE,
  LOG_SYS_ERRUPDATE,
  LOG_SYS_HWERRUPDATE,
  LOG_SYS_ENDIDUPDATE,
  LOG_SYS_ENDCONFUPDATE,
  LOG_SYS_ENDGROUPUPDATE,
  LOG_SYS_ENDSHEDULEUPDATE,
  LOG_SYS_ENDHOLLIDAYUPDATE,
  LOG_SYS_NOTGSMREG,
  LOG_SYS_NOTGPRSREG,
  LOG_SYS_ERRCONNECTION,
  LOG_SYS_CONNECT1,
  LOG_SYS_CONNECT2,
  LOG_SYS_ERRCONNECT1,
  LOG_SYS_ERRCONNECT2,
  LOG_SYS_FAILCONNECT1,
  LOG_SYS_FAILCONNECT2,
  LOG_SYS_IN1ON,
  LOG_SYS_IN1OFF,
  LOG_SYS_IN2ON,
  LOG_SYS_IN2OFF,
  LOG_SYS_AUTOREGEN,
  LOG_SYS_AUTOREGDIS,
  LOG_SYS_OPENALLEN,
  LOG_SYS_OPENALLDIS,
  LOG_SYS_UNBLOCKSHEDEN,
  LOG_SYS_UNBLOCKSHEDDIS,
  LOG_SYS_UNDLOCKHANDEN,
  LOG_SYS_UNDLOCKHANDDIS,
  LOG_SYS_RESETONTIMER,
  LOG_SYS_RESETONCMD,
  LOG_SYS_RESETONSHEDULE,
  LOG_SYS_RECVSMS,
  LOG_SYS_SENDSMS,
  LOG_SYS_LOWBALANCE,
  LOG_SYS_LOWSIGNAL  
}logSystem_e;

// событиe доступа журнала
typedef enum{
  LOG_CON_IDNOTINBASE = 40,
  LOG_CON_IDBLOCKED,
  LOG_CON_IDADDBYSMS,
  LOG_CON_IDADDBYAUTOREG,
  LOG_CON_OUTONBYPHONE,
  LOG_CON_OUTONBYRFID,
  LOG_CON_OUTONBYWIEGAND,
  LOG_CON_OUTONBYRF433,
  LOG_CON_OUTONBYAVTO,
  LOG_CON_OUTONBYCMD,
  LOG_CON_RECVIDONREADER,
  LOG_CON_PHOTOFIX
}logControl_e;

// событиe обмена журнала
typedef enum{
  LOG_TR_CONNECTED = 0,
  LOG_TR_NOBINDING = 1,
  LOG_TR_NOINSYSTEM = 2,
  LOG_TR_STATUS1 = 3,
  LOG_TR_STATUS2 = 4,
  LOG_TR_ANSWERONBINDING = 5
}logTransfer_e;

// событиe команды журнала
typedef enum{
  LOG_CMD_OUTEN = 7,
  LOG_CMD_RESET,
  LOG_CMD_SENDSMS,
  LOG_CMD_FACTORY,
  LOG_CMD_SETTRIGGER,
  LOG_CMD_GETIDHARD,
  LOG_CMD_GETPHOTO,
  LOG_CMD_OUTSEN,
  LOG_CMD_GETLOG,
  LOG_CMD_GETIDAUTOREGSMS,
  LOG_CMD_SENDINGSMS,
  LOG_CMD_SYNCCONFIG,
  LOG_CMD_SYNCID,
  LOG_CMD_SYNCSHEDULE,
  LOG_CMD_SYNCGROUP,
  LOG_CMD_SYNCHOLLIDAY,
  LOG_CMD_GETCRCSTRUCT,
  LOG_CMD_GETCRCIDBLOCK,
  LOG_CMD_RUNUPDATE,
  LOG_CMD_SENDUPDBLOCK,
  LOG_CMD_GETSTATUS2,
  LOG_CMD_SETTIME,
  LOG_CMD_ANSWER
}logComand_e;

// 
typedef enum{
  SHTAMP_NONE = 0, 
  SHTAMP_ID = 1, 
  SHTAMP_GROUP = 2,
  SHTAMP_SHEDULE = 3,
  SHTAMP_CONFIG = 4,
  SHTAMP_HOLIDAY = 5,
  SHTAMP_ERROR = 255
}shtamp_e;

// 
typedef enum{
  CMD_SUCCESS = 0, 
  CMD_PARAMERROR = 1, 
  CMD_RUNERROR = 2,
  CMD_UNDEFRESULT = 3,
  CMD_GETPASSWORD = 4,
  CMD_RUNNING = 5,
  CMD_CANCEL = 6,
  CMD_GOTONEXT = 7,
  CMD_UNDEFIMEI = 8,
  CMD_UNDEFOBJECT = 9,
  CMD_ROLLBACKRUN = 253, /* protocol D532: firmware rollback started */
  CMD_CRCERROR = 254,
  CMD_UNDEFINE = 255
}cmdCode_e;

//блок памяти
typedef enum{
  MB_FIRMWARE = 0,
  MB_BACKUP = 1,
  MB_ID = 2,
  MB_GROUP = 3,
  MB_SHEDULE = 4,
  MB_HOLIDAY = 5,
  MB_CONFIG = 6,
  MB_LOG = 7,
  MB_TEMPID = 8,
  MB_PHOTO = 9
}memBlock_e;

// Ячейка Праздничные дни
#pragma pack(push, 1)
typedef struct {
  uint32_t start; // начало блока
  uint32_t size; // размер
} cellMem_s; // 8 байт
#pragma pack(pop)

// Канал передачи
typedef enum{
  CH_USB = 0, CH_SERVER, CH_SMS
}dataChanel_e;

// тип записи
typedef enum{
  CT_PHONE = 1, CT_WIEGAND = 2, CT_RF433 = 3, CT_RFID = 4, CT_AUTO = 5,
  CT_SERVER = 6, CT_OTHER = 7
}codeType_e;

// Признак синхронизации времени
typedef enum{
  TS_SERVER1 = 0,
  TS_SERVER2 = 1,
  TS_NTP = 2,
  TS_USER = 3
}timeSyncType_e;

// Битовая
#pragma pack(push, 1)
typedef struct {
  Int8U b0:1;
  Int8U b1:1;
  Int8U b2:1;
  Int8U b3:1;
  Int8U b4:1;
  Int8U b5:1;
  Int8U b6:1;
  Int8U b7:1;
}BITS_; // 1 байт
#pragma pack(pop)

// Триггерные функции
#pragma pack(push, 1)
typedef struct {
  uint8_t handUnBlock; // Ручная разблокировка: 0 - выключить, 1 включить
  uint8_t sheduleUnBlock; // Разблокировка по сценарию: 0 - выключить, 1 включить
  uint8_t autoregistration; // Авторегастрация: 0 - выключить, 1 включить
  uint8_t openAll; // Открывать всем: 0 - выключить, 1 включить
  uint8_t smsToAdmin; // Пересылать входящие SMS на телефон администратора: 0 - выключить, 1 включить
  uint8_t smsUnreg; // Информировать админа по SMS, если ID не зарегистрирован в базе абонентов: 0 - выключить, 1 включить
  uint8_t smsLowBalance; // Информировать админа по SMS при балансе менее 100: 0 - выключить, 1 включить
  uint8_t openAllByShedule; // Реагировать на все вызовы по расписанию: 0 - выключить, 1 включить
  uint8_t reserve[8];
} triggers_s; // 16 байт
#pragma pack(pop)

// Статус
#pragma pack(push, 1)
typedef struct {
  Int16U stateOut2:1; // состояние выхода 2
  Int16U stateOut1:1; // состояние выхода 1
  Int16U stateIn2:1; // состояние входа 2
  Int16U stateIn1:1; // состояние входа 1
  Int16U openAnyUser:1; // открывать всем
  Int16U autoRegistration:1; // авторегистрация
  Int16U unblockOnScene:1; // разблокировка по сценарию
  Int16U unblockHand:1; // ручная разблокировка
  Int16U res1:1;
  Int16U res2:1;
  Int16U res3:1;
  Int16U readyToUpdate:1; // Готов к обновлению прошивки
  Int16U readyLogData:1; // Есть данные в журнале событий
  Int16U readyPhoto:1; // Есть фотографии на выгрузку
  Int16U readyID:1; // Есть ID, добавленные через авторегистрацию или SMS
  Int16U defaultConfig:1; // Я обнулен, имею заводские настройки. требуется полная синхронизация
}status_s; // 2 байта
#pragma pack(pop)

// Структура оперативных переменных
#pragma pack(push, 1)
typedef struct {
  char IMEIGSM[16];  // IMEIGSM
  uint16_t memID; // идентификатор памяти
  
  uint8_t protocolVersion;
  uint8_t hardwareVersion;
  uint16_t firmwareVersion; // версия прошивки
  uint8_t bootVersion; // версия загрузчика
  uint32_t buildDate; // дата создания прошивки

  uint32_t datetimeConfig; // Дата последнего обновления Настр.
  uint32_t datetimeID;
  uint32_t datetimeShedule;
  uint32_t datetimeGroup;
  uint32_t datetimeHoliday;
 
  uint32_t datetime;
  uint32_t worktime;
    
  uint8_t day;
  uint8_t hour;
  uint8_t minute;  
  
  char callingnumber[16]; // номер звонящего
  uint8_t csq;   // уровень сигнала  
  uint8_t creg; // состояние регистрации GSM сети
  uint8_t cgreg; // регистрация GPRS
  uint16_t balance;
  uint8_t netType;
  uint8_t connected;
//80
//  uint8_t errorCode; //0 - резерв 1 - контроллер отсутствует в базе
/*2 - контроллер не привязан не к одному из объектов
3 - контроллер привязан к объекту*/
//  uint64_t objNumber; // Уникальный номер объекта. При коде ошибки 1 и 2 данные равны нулю
//  char objName[128]; // Наименование объекта. При коде ошибки 1 и 2 данные равны пустой строке
//  char controllerName[128]; // Наименование контроллера на объекте. При коде ошибки 1 и 2 данные равны пустой строке
//  char adminNumber[16]; // Телефон администратора объект. При коде ошибки 1 и 2 данные равны пустой строке
//80
  uint16_t numbersCount;
  uint16_t tempIDsCount;
  uint32_t newAddressTempID;  
  uint16_t logsCount;
  uint32_t newAddressLog;  

  triggers_s triggers;
  
  uint8_t timeToReset; // время на перезагрузку
  uint8_t timeToDefault; // время на сброс настроек
  uint8_t timeToGetID; //0 - Резерв 1-255 - таймаут захвата, время, через в течении которого контроллер должен вернуть ID или ошибку ----------------------------------------
  uint8_t sourceGetID; //0 - Резерв 1 - Wiegand 2 - RF433 3 - 485    
  uint8_t addressGetID; // Указывается только если выбран интерфейс 485. В противном случае поле игнорируется
  uint8_t getPhoto; // 0 - резерв 1 - 255 - номер сетевого адреса камеры
 
  uint8_t peripheryState[8];
  uint8_t updateState; 
  uint8_t loadDefaultConfig; // загружена конфигурация по умолчанию
//126
  int8_t connectionhandle;
  int8_t updateConnectionHandle;  
  
  status_s status;
  status_s laststatus; 
  
  uint8_t registredOnServer;
  uint8_t lmTimer;
  uint8_t lmCount;
  uint8_t answerID;
  uint8_t answerTimeout;  
  uint8_t userAutorized;
  uint8_t configuratorConnected;  
//139  
  uint32_t firmwareSize;
  uint32_t firmwareCRC;
  uint16_t FWblockCount;
  uint8_t firmwarePercent;
//150
  uint16_t readLogIDIndex;
  uint16_t readTempIDIndex;  
  uint16_t countPhoto;
  uint32_t newAddressPhoto;   
//160
  uint8_t last_hour;
  bool useServer2; // флаг указывает что работаем с резервным сервером
  uint8_t reconnectCount;
  uint8_t FWinProgress;
  uint16_t freeBufferSize;  
  uint16_t readPhotoIndex;
  uint8_t moduleType; // тип модуля gsm
  uint8_t logBits; // тип включенного лога
  uint8_t rebootToUpdate; // флаг перезагрузки в режим обновления FW
  uint16_t FWblockIdx;
  uint32_t startWaitFWtime; // время начала ожидания
  uint8_t cchFWupdate;
  uint16_t updateTimeout;
  
  uint8_t reserv[329];
  uint16_t crc; // контрольная сумма
} device_s; // 512 байт
#pragma pack(pop)

// Ячейка ID
#pragma pack(push, 1)
typedef struct {
  uint16_t index; // 0 - резерв 1-50000 - номер ячейки
  char ID[10]; // Значение ID Абонента
  uint8_t group; // 0 - Резерв 1-64 - номер группы
  uint32_t blockTime; // Дата тайм запрета, учитывается только дата, полле которой ID будет помечен на удаление
  uint16_t useTime; // Время действия  0 - бессрочно - по умолчанию 1-65535 - Время в минутах, после которого ID будет помечен на удаление. Отсчет начинается с первого прохода.
  struct{
    uint8_t isPhone:1; // принадлежность ID
    uint8_t isRFID:1;
    uint8_t isAuto:1;
    uint8_t isRF433:1;
    uint8_t isWiegand:1;
    uint8_t isCode:1;
    uint8_t isRes1:1;
    uint8_t isRes2:1;
  };
  uint8_t cntPassOnDay; // кол-во проходов в сутки
  uint8_t cntPass; // Счетчик количества проходов
  uint8_t reserve[10];
} cellID_s; // 32 байта
#pragma pack(pop)

// Ячейка Расписание
#pragma pack(push, 1)
typedef struct {
  uint8_t index; // 0 - резерв 1-64 - номер ячейки
  uint8_t startHour; // Время начала часы
  uint8_t startMinute; // Время начала минуты
  uint8_t stopHour;
  uint8_t stopMinute;
  uint8_t blocked; // Блокировка расписания: 0 - разблокировано, 1 - заблокировано
  uint8_t day[7]; // Разрешить в ПН: 0 - запрещено, 1 - разрешено
  uint8_t scene[8]; // 0 - Резерв 1-64 - номер расписания для сцены 1
  uint8_t onHoliday; // 0 - Резерв
/*1 - Игнорировать праздничные дни
2 - Выполнять сценарии только в праздничные дни
3 - Выполнять сценарии во все дни, кроме праздничных*/
  uint8_t reserve[10]; 
} cellShedule_s; // 32 байта
#pragma pack(pop)

// Ячейка Группы
#pragma pack(push, 1)
typedef struct {
  uint8_t index; // 0 - резерв 1-64 - номер ячейки
  uint8_t sheduleNumber; // 0 - Круглосуточно 1-64 - номер расписания
  uint8_t blocked; // Блокировка группы: 0 - выключена, 1 - включена
  uint8_t onPhone; // Разрешить управления голосовым вызовом: 0 - выключено, 1 - включено
  uint8_t onRF433;
  uint8_t onRFID;
  uint8_t onWiegand;
  uint8_t onAuto;
  uint8_t onLink;
  uint8_t onCode;
  uint8_t reserve[6]; 
} cellGroup_s; // 16 байт
#pragma pack(pop) 

// Ячейка Праздничные дни
#pragma pack(push, 1)
typedef struct {
  uint32_t datetime; // Дататайм по линуксу
} cellHoliday_s; // 4 байта
#pragma pack(pop)

// Тип периферии
#pragma pack(push, 1)
typedef struct {
  uint8_t address;
  uint8_t type;
} rs485Periphery_s; // 2 байта
#pragma pack(pop)

// Режим реле
#pragma pack(push, 1)
typedef struct {
  uint8_t useInBlock; // Применяется для функции разблокировки: 0 - выключено, 1 - включено
  uint8_t mode; // Режим работы: 0 - импульс, 1 - уровень
  uint8_t onCmdServer; // Разрешить активировать по любой команде от сервера: 0 - выключено, 1 - включено
  uint8_t onWiegand; // Разрешить активировать от ID, полученных по интерфейсу Wiegand: 0 - выключено, 1 - включено
  uint8_t onCamera; // Разрешить активировать от ID, полученных по интерфейсу 485 от камеры: 0 - выключено, 1 - включено
  uint8_t onRFID; // Разрешить активировать от ID, полученных по интерфейсу 485 RFID устройства: 0 - выключено, 1 - включено
  uint8_t onOther; // Разрешить активировать Разрешить аот ID, полученных по интерфейсу 485 прочие: 0 - выключено, 1 - включено
  uint8_t onRF433; // Разрешить активировать от ID, полученных по интерфейсу RF433: 0 - выключено, 1 - включено
  uint8_t tImpulse; // 0 - резерв 1-255 - время импульса в секундах
  uint8_t tPause; // -/-
  uint8_t cnt; // -/-
} releMode_s; // 11 байт
#pragma pack(pop)

// Ячейка журнала
#pragma pack(push, 1)
typedef struct {
  uint32_t index; // уникальный номер
  uint32_t datetime; // время
  uint8_t logType;
  char ID[10]; // Значение ID Абонента
  uint32_t photoNum;
  uint8_t passCnt;
  uint8_t reserve[8];
} cellLog_s; // 32 байта
#pragma pack(pop)  

// Параметры фото
#pragma pack(push, 1)
typedef struct {
  uint32_t datetime;
  uint32_t size;
} photoParam_s; // 8 байт
#pragma pack(pop)

// Параметры прошивки
#pragma pack(push, 1)
typedef struct {
  uint8_t hardwareVersion;
  uint8_t bootVersion; // версия загрузчика
  uint16_t firmwareVersion; // версия прошивки
  uint32_t buildDate; // дата создания прошивки
  uint32_t firmwareSize;
  uint32_t firmwareCRC;
  uint32_t res[4];
} firmwareParam_s; // 32 байта
#pragma pack(pop)

// Параметры прошивки для загрузчика и во флеш
#pragma pack(push, 1)
typedef struct {
  uint32_t appIdent;
  uint8_t hardwareVersion;
  uint16_t firmwareVersion; // версия прошивки
  uint8_t res;
  uint32_t firmwareSize;
  uint32_t firmwareCRC;
  uint32_t res2;
  uint32_t buildDate; // дата создания прошивки
  uint8_t res3[8];
} bootFWparam_s; // 32 байта
#pragma pack(pop)

// Параметры конфигурации
#pragma pack(push, 1)
typedef struct {
  uint32_t datetime; // Таймштамп настроек
  uint32_t pinCode; // ПИН-код доступа к настройкам 4 цифры пин кода, прочие символы не допускаются
  char server1[32]; // сервер 1 (IP адрес или домен + порт)
  char server2[32];
  char NTPserver[32]; // 0 - синхронизация по основному серверу, любой текст является IP или доменом NTP сервера. Порт не пишется.
  char netOperator[32]; // 0 - все нули - автоматический выбор, считывание данных об операторе с сим карты
  uint8_t netType; // 0 - Автоматически 1 - Только GPRS/EGDE 2 - Автоматически 3G/4G 3 - Только 3G 4 - Только 4G
  char countryCode[8]; // Код страны
  char simPhone[16]; // Телефон симкарты
  char ussdBalance[8]; // USSD команда запроса баланса
  uint8_t restartTime; // 0 - функция отключена 1-23 - интервал времени, через который контроллер будет автоматически перезагружаться
  uint8_t fullLog; // Расширенный лог для отладки: 0 - отключено, 1 - включено
  uint8_t useVoLTE; // Иипользовать VoLTE: 0 - отключено, 1 - включено
  uint8_t rs485Mode; // 0 - Пассисный. Все, что валится в порт считается ID, 
  // 1 - активный адресный. Для каждого адреса свой тип. RFID считыватели Chafon, JPEG камеры OV, ...
  uint8_t useNTP; // Включить синхронизацию времени по NTP: 0 - выключено, 1 - включенно
  uint8_t reserv[4];
  uint8_t wiegandType; // 0 - вход отключен
/*1 - кодонаборная панель
2 - RFID считыватель
3 - ....
4 - ....*/
  rs485Periphery_s rs485Periphery[8]; // Интерфейс 485
  releMode_s rele[2]; // Настройки Реле 
  uint8_t inputMode[2]; // Функция входа 0 - вход отключен 1-255 - номер функции
  uint8_t useServer1;
  uint8_t useServer2;
  
  uint8_t reserv2[289];
  // контрольная сумма
  uint16_t crc;
} config_s; // 512 байт
#pragma pack(pop)

#endif