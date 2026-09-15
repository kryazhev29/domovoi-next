// Пользовательские функции

#include "user_fun.h"
#include "usb_istr.h"
#include "../Bootloader/app_update.h"
#include "../Bootloader/boot_manual.h"

// ************** Включить режим отладки **********
#define DEBUG_USER

#define UNIPASS_INITVALUE       32768
#define CRYPT_START             (uint8_t)0x5A
#define TIMETOREGRESPONSE    (uint8_t)5 // время на получение ответа от сервера
#define TIMETOSTATUSRESPONSE    (uint8_t)15 
#define STATUSTIMERINTERVAL     (uint8_t)15 // интервал отправки статуса
#define MAXSTATUSANSWERERROR    (uint8_t)1 // кол-во не получения ответа на статус

// команда 7 запрос фото
// если getPhoto не равно нулю то сделать фото
// сервер обновления и передачи фото
// алгоритм переключения на второй сервер

// таблица адресов областей памяти контроллера
#pragma data_alignment=4
cellMem_s mem[16];

// конфигурация
#pragma data_alignment=4
config_s config;

// оперативные переменные
#pragma data_alignment=4
device_s device;

// буфер под прием данных от gsm
#pragma data_alignment=4
uint8_t gsm_buffer[GSMBUFSIZE];

static void fw_resume_after_boot(void);

// буфер под прием данных от внешнего порта
#pragma data_alignment=4
uint8_t port_buffer[PORTBUFSIZE];

union{
  char tempStrBuffer[128]; // временный строковый буфер
  uint32_t bf32[32];
  uint8_t bf[128];
};

uint8_t uniqIDCnt = 0;

// >>> переопределение функций --------------------------
void gsm_user_debug (char* ptr)
{
  if (device.logBits & 1)
    usb_sendstr(ptr);
}

void usergsm_debug(char* ptr)
{
  if (device.logBits & 2)
    usb_sendstr(ptr);
}

void user_debug(char* ptr)
{
  if (device.logBits & 4)
    usb_sendstr(ptr);
}

void timers_user_handle(void)
{
  usb_timer1ms();
  uart_timer1ms();
}

// обработка данных от usb
void usb_data_reading(uint8_t* ptr, uint16_t len)
{
  uint16_t body;
  if (len < 11 || memcmp(ptr, "SLDN", 4)) return;
  body = (uint16_t)ptr[4] | ((uint16_t)ptr[5] << 8);
  if (body < 3 || body != len - 8) return;
  /* The USB assembler hands over exactly one complete, CRC-checked frame. */
  user_parse_comand(ptr + 4, CH_USB);
}

// обработчик событий
void usergsm_event(gsmevent_e ev)
{
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U Event %d\r\n", ev);
  user_debug(tempStrBuffer); 
#endif  
  switch (ev)
  {
  case E_READY:
    // задать тип используемой сети
    switch(config.netType){
    case 0: // Автоматически
      gsm_setNetworkType(NT_AUTO);
      break;
      
    case 1: // Только GPRS/EGDE
      gsm_setNetworkType(NT_GSM);
      break;
      
    case 2: // Автоматически 3G/4G
      gsm_setNetworkType(NT_AUTO);
      break;
      
    case 3: // Только 3G
      gsm_setNetworkType(NT_UMTS);
      break;
      
    case 4: // Только 4G
       gsm_setNetworkType(NT_LTE);
      break;      
    }
    // ntp
    if (config.useNTP){
      if (config.NTPserver[0] != 0)
        gsm_setNTPserver(config.NTPserver, 123);
    }
    break;

  case E_GSMREG:
    {
      char *tptr;
      
      device.creg = 1;
      // при регистрации в сети, отправляем параметры точки доступа
      //internet,gdata,gdata
      if (config.netOperator[0] != 0){
        user_strcpy(tempStrBuffer,config.netOperator,',',30);
        tptr = skipchar(',',config.netOperator,1);
        if (tptr != config.netOperator)
          user_strcpy(&tempStrBuffer[32],tptr,',',30);
        else
          tempStrBuffer[32] = 0;
        tptr = skipchar(',',config.netOperator,2);
        if (tptr != config.netOperator)
          user_strcpy(&tempStrBuffer[64],tptr,0,30);
        else
          tempStrBuffer[64] = 0;
        gsm_initgprs(&tempStrBuffer[0], &tempStrBuffer[32], &tempStrBuffer[64]);  
      }
    }
    break;

  case E_NOGSM:
    device.creg = 0;
    device.cgreg = 0;
    log_addSystemEvent(LOG_SYS_NOTGSMREG);
    break;

  case E_GPRSREG:
    device.cgreg = 1;
    user_setconnections(); // создаем подключение к серверу
    break;

  case E_NOGPRS:
    device.cgreg = 0;
    log_addSystemEvent(LOG_SYS_NOTGPRSREG);
    break;
    
  case E_NOSIM:
    break;

  case E_NOTANSWER: // по какой-то причине модуль не ответил на команду
    break;

  case E_PWRON:
  case E_PWROFF:
    device.creg = 0;
    device.cgreg = 0;
    break;

  case E_PINGTIMEOUT: // не получили ответ на ping
    break;

  case E_IMEI:
    strcpy(device.IMEIGSM, usergsm_getimei());
    break;
    
  case E_MODULETYPE:
    device.moduleType = gsm_getModuleType();
    user_saveindex();
    break;
    
  default:
    break;
  }
}

// получен идентификатор по wiegand
void WG_readedID(uint32_t id){
  char tb[10];
  
  sprintf(tb, "00%08X", id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U Wiegand ID: %s\r\n", tb);
  user_debug(tempStrBuffer); 
#endif
  
  if (device.sourceGetID == 1) // запрос ID c сервера
  {
    device.timeToGetID = 0;
    device.sourceGetID = 0;
    user_send_readedIDtoServer((uint8_t*)tb, 1);
  }
  
  // проверить номер
  user_checkIdAndControl(tb, CT_WIEGAND); 
}

// получен идентификатор по rf433
void RF433_readedID(uint32_t id){
  char tb[10];
  //777
  sprintf(tb, "00%08X", id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U RF433 ID: %s\r\n", tb);
  user_debug(tempStrBuffer); 
#endif
  
  if (device.sourceGetID == 2) // запрос ID c сервера
  {
    device.timeToGetID = 0;
    device.sourceGetID = 0;
    user_send_readedIDtoServer((uint8_t*)tb, 1);
  }
  
  // проверить номер
  user_checkIdAndControl(tb, CT_RF433); 
}

// получен идентификатор по RFID
void RFID_readedID(uint32_t id, uint8_t address){
  char tb[10];
  //777
  sprintf(tb, "00%08X", id);
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U RFID ID: %s\r\n", tb);
  user_debug(tempStrBuffer); 
#endif
  
  if ((device.sourceGetID == 3) && (device.addressGetID == address)) // запрос ID c сервера
  {
    device.timeToGetID = 0;
    device.sourceGetID = 0;
    user_send_readedIDtoServer((uint8_t*)tb, 1);
  }
  // проверить номер
  user_checkIdAndControl(tb, CT_RFID); 
}
// <<< переопределение функций --------------------------

// >>> вспомогательные ---------------------------------------------------------
// валиден ли IMEI
Int8U user_imeivalid(void)
{
  if (chkdigitstr(device.IMEIGSM, 15)) 
    return 1;
  return 0;
}

// универсальный пароль на конфигурацию
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

// ожидание пока напряжение питания не станет выше порога
void user_wait_stable_power(void)
{

}

// Преобразует время в целочисленное
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
// <<< вспомогательные ---------------------------------------------------------

// >>> переопределение функций --------------------------
// подключились к серверу
void user_connected (int8_t handle)
{
#ifdef DEBUG_USER
  user_debug(">>U Connected ...\r\n"); 
#endif 
  log_addSystemEvent(LOG_SYS_CONNECT1);
  
  // отправляем данные регистрации
  user_send_registrationData(CH_SERVER);
}

// отключились от сервера
void user_disconnected (int8_t handle, uint8_t arec)
{
  if (handle != device.connectionhandle) return; 
  
#ifdef DEBUG_USER
  user_debug(">>U Disconnected ...\r\n"); 
#endif
  
  if (arec == 0)
    device.connectionhandle = -1;
  
  device.registredOnServer = 0;
  device.answerTimeout = 0;
  
  // считаем попытки и если что переключаемся на резервный
  if (++device.reconnectCount == 10){
    device.reconnectCount = 0;
    device.useServer2 = !device.useServer2;
  }
}

// ошибка подключения
void user_errorconnection (int8_t handle){
#ifdef DEBUG_USER
  user_debug(">>U Error connection ...\r\n"); 
#endif
  device.registredOnServer = 0;
  device.answerTimeout = 0;
  
  log_addSystemEvent(LOG_SYS_ERRCONNECTION);
}

// данные с сервера
void user_dataconnection (int8_t handle, uint8_t* data, uint16_t siz)
{
  if (handle != device.connectionhandle) return;
  
  uint16_t i;

#ifdef DEBUG_USER
  user_debug(">>U Data from server ...\r\n"); 
#endif  
  
  for (i=0; i<(siz - 4); i++)
  {
    if (data[i] != 'S') continue;
    i++;
    if (data[i] != 'L') continue;
    i++;
    if (data[i] != 'D') continue;
    i++;
    if (data[i] != 'N') continue;
    i++;
    user_parse_comand(&data[i], CH_SERVER);
  }
}

// принято СМС
// формат команды [Символ – #][Команда][Пробел][ПИН код контроллера][Запятая][Аргументы команды]
void user_smsreceived (char* num, char* data)
{
  log_addSystemEventID(LOG_SYS_RECVSMS, user_last10digitsnumber(num));
  
  //777 в какой момент utc конвертить в текст?
  /*if ((dt[0] == '0') && ((dt[1] == '0') || (dt[1] == '4'))){ // ucs2 code
    if ((dtEnd != dt) && ((dtEnd - dt) <= 512)){
      decodeUCS2toText(tempStrBuffer, dt, (uint16_t)(dtEnd - dt - 1));
      dt = tempStrBuffer;
    }
  }*/
  
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U SMS recv: %s\r\n", data);
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

    if (validPin == true){
      // добавить ID
      if (strcmp(v, "add") == 0){
        if (validFormat){
          strncpy(&tempStrBuffer[96], arg, 10);
          user_addIdToTempBuffer(arg, CT_OTHER);
          log_addControlEvent(LOG_CON_IDADDBYSMS, arg, 0, 0);
          strcat(v, ": ");
          strcat(v, &tempStrBuffer[96]);
          strcat(v, " - Ok");
          user_sendsms(num, v);
        } else{
          strcat(v, ": Error \"Format not correct\""); 
          user_sendsms(num, v);
        }
      } else
      
      // запросить IMEI
      if (strcmp(v, "imei") == 0){
        strcat(v, ": ");
        strcat(v, device.IMEIGSM);
        user_sendsms(num, v);
      } else
        
      // запросить статус
      if (strcmp(v, "status") == 0){
        sprintf(v, "status:\r\n-signal: %d\r\n-net: %s\r\n-cloud: %d\r\n-fw: %d\r\n-trig: %s", 
                device.csq, "", device.connected, device.firmwareVersion, "");
        user_sendsms(num, v);
      } else
        
      // перезагрузить
      if (strcmp(v, "reset") == 0){
        strcat(v, ": Go to reset");
        user_sendsms(num, v);
        device.timeToReset = 5;
      } else
        
      // включить выход N
      if (strcmp(v, "setout") == 0){
        if (validFormat){
          uint8_t out = atoi(arg);
          
          if (out == 1){
            if (config.rele[0].onCmdServer){
              out_load (OUT1, OUT_ON, 0, 0, 0);
              strcat(v, ": Out 1 enabled");
            } else{
              strcat(v, ": Error, Out 1 is blocked");
            }
          } else if (out == 2){
            if (config.rele[1].onCmdServer){
              out_load (OUT2, OUT_ON, 0, 0, 0);
              strcat(v, ": Out 2 enabled");          
            } else{
              strcat(v, ": Error, Out 2 is blocked");             
            }
          }
          user_sendsms(num, v);  
        } else{
          strcat(v, ": Error \"Format not correct\""); 
          user_sendsms(num, v);
        }  
      } else
        
      // обновить прошивку N
      if (strcmp(v, "update") == 0){
        //777
      } else
        
      // заводские настройки
      if (strcmp(v, "default") == 0){
        user_factoryReset();
        strcat(v, ": Complete");
        user_sendsms(num, v);
      }
      
    } else{
      strcat(v, ": Error \"Pin not valid\""); 
      user_sendsms(num, v);
    }
  }

  // пересылать смс администратору
  if (device.triggers.smsToAdmin){
    user_send_smsToServer(data);
  }  
}

// отправить СМС
void user_sendsms(char *mes, char *num){  
#ifdef DEBUG_USER
  user_debug(">>U Sending SMS ...\r\n"); 
#endif
  gsm_sendsms(num, mes);
  log_addSystemEventID(LOG_SYS_SENDSMS, user_last10digitsnumber(num));
}

// СМС отправлено
void user_smssended (void)
{
#ifdef DEBUG_USER
  user_debug(">>U SMS sended ...\r\n"); 
#endif  
}

// входящий вызов
bool user_incomingcall (char* num, uint8_t typ)
{
#ifdef DEBUG_USER
  sprintf(tempStrBuffer, ">>U InCalling: %s\r\n", num);
  user_debug(tempStrBuffer); 
#endif
  
  if (typ == CT_PHONE){
    gsm_answercall();
    delay_ms(1000);
    usergsm_endCall();
    
    // номер звонящего
    strcpy(device.callingnumber, num);    
  }  

  // 10 цифр номера
  char *ldn = user_last10digitsnumber(num);  
  
  // проверить номер
  user_checkIdAndControl(ldn, (codeType_e)typ);  

  return true;
}

// ошибка команды
void user_comanderror (int16_t er)
{
#ifdef DEBUG_USER
  user_debug(">>U Comand error ...\r\n"); 
#endif
}

// организовать подключения
void user_setconnections(void)
{
  if ((config.server1[0] == 0) && (config.server2[0] == 0)) return; // подключение к серверу не задействовано
  if ((config.useServer1 == 0) && (config.useServer2 == 0)) return;
  
  
  ConnectParam_t cp;

  if (device.useServer2){
    // резервный сервер
    cp.ip = config.server2;
    cp.port = 0;
#ifdef DEBUG_USER
  user_debug(">>U Set connection to server 2 ...\r\n"); 
#endif
  } else{
    if (config.useServer1){
      // основной сервер
      cp.ip = config.server1;
      cp.port = 0;
  #ifdef DEBUG_USER
    user_debug(">>U Set connection to server 1 ...\r\n"); 
  #endif      
    } else{
      // резервный сервер
      cp.ip = config.server2;
      cp.port = 0;
  #ifdef DEBUG_USER
    user_debug(">>U Set connection to server 2 ...\r\n"); 
  #endif
    } 
  }

  cp.timeout = 5000; // время на подключение  
  cp.maxConnectCount = 0; // попыток подключиться
  cp.auto_reconnect = 1; // время активности ноль - соедиенение постоянное
  cp.wait_sendData = 0;
  cp.Connected = user_connected;
  cp.Disconnected = user_disconnected;  
  cp.DataReceived = user_dataconnection;
  
  device.connectionhandle = connection_create(&cp);
  
  // открываем подключение сразу
  if (cp.auto_reconnect)
    connection_open(device.connectionhandle);
}

// парсер USSD
void user_parseUSSD(char* ptr)
{
#ifdef DEBUG_USER
  user_debug(">>U Parse USSD ...\r\n"); 
#endif  
}
// <<< переопределение функций --------------------------


// инициализация
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
  /* Logical legacy address. memory.c stores new images in reserved
     0x1F5000..0x1F8FFF (two data + two commit sectors). */
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
  user_readconfig();

  // инициализация переменных
  user_initvariables();
  app_boot_metadata();

  // Виганд
  if (config.wiegandType != 0)
    WG_init(); 
  
  // порт gsm
  uart_init(GSM_PORT, 115200, 0, gsm_buffer, GSMBUFSIZE); // порт gsm
  uart_setRecTimeout(GSM_PORT, 10); // время заполнения/ожидания приемного буфера 10 мс
  
  // внешний порт
  uart_init(PORT1_PORT, 115200, 0, port_buffer, PORTBUFSIZE); // порт 485
  uart_setRecTimeout(PORT1_PORT, 10); // время заполнения/ожидания приемного буфера 10 мс  
}

// очистить блок памяти
void user_clearMemBlock(memBlock_e mb){
  
  if (mem[mb].size == 0) return;

#ifdef DEBUG_USER
  user_debug(">>U Clear mem block ...\r\n"); 
#endif
  
  uint32_t adr;
  uint32_t endAdr = mem[mb].start + mem[mb].size;

  // по секторно стираем
  for (adr = mem[mb].start; adr < endAdr; adr += 4096){
    mem_erase_sector(adr);
    wdt_reload();
  }
}

// циклический обработчик
void user_loop(void)
{
  // каждые 10 мс
  if (timers_get(0) >= 10)
  {
    timers_set(0,0);
    if (config.wiegandType != 0)
      WG_process();
  }

  // каждые 50 мс
  if (timers_get(1) >= 50)
  {
    timers_set(1,0);
    // запущен процесс обновления
    if (device.registredOnServer && device.FWinProgress){
      if (device.startWaitFWtime == 0){
        user_getFWblockFromServer((dataChanel_e)device.cchFWupdate, device.FWblockIdx);
        device.startWaitFWtime = timers_getMillis();
      } else{
        // не дождались блока прошивки
        if ((timers_getMillis() - device.startWaitFWtime) > 15000){
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

    usergsm_work(100);
    led_fun100ms(); // обработка индикации
    user_readinputs(); // входы
    // состояние выходов
    device.status.stateOut1 = out_getstate(OUT1);
    device.status.stateOut2 = out_getstate(OUT2);
    // состояние выхода отображаем светодиодом
    if (device.status.stateOut1 || device.status.stateOut2){
      LED3_ON;
    } else{
      LED3_OFF;
    }
  }

  // каждую 1 сек
  if (timers_get(3) >= 1000)
  {
    timers_set(3,0);
    wdt_reload();
    
    out_process(); // обработка алгоритма работы выхода
    
    // есть подключение по USB но не настроено
    if (USB_DETECT)
    {
      if (usb_get_state() != ON)
        usb_init();
    }
    
    user_indication(); // индикация
        
    device.worktime++; // время работы от момента включения, в секундах
    device.datetime++; // внутренние часы
    // вычислим день недели, час и минуту
    device.day = user_getweakday(device.datetime);
    device.hour = div(div(device.datetime, 86400).rem, 3600).quot;
    device.minute = div(div(device.datetime, 3600).rem, 60).quot;
    // уровень сигнала
    device.csq = usergsm_getcsq();
    device.connected = (connection_state(device.connectionhandle)==CE_CONNECTED)?1:0;
    device.netType = usergsm_getNetType();
    
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
    
    // время получения ответа от сервера при подключении
    if (device.answerTimeout){
      if (--device.answerTimeout == 0){
        switch (device.answerID){
        case 1:
#ifdef DEBUG_USER
  user_debug(">>U Not answer to reg pack\r\n"); 
#endif
          device.registredOnServer = 0;
          connection_reopen(device.connectionhandle);
          break;
          
        case 2:
          if (++device.lmCount == MAXSTATUSANSWERERROR){
#ifdef DEBUG_USER
  user_debug(">>U Not answer to status\r\n"); 
#endif            
            device.registredOnServer = 0;
            connection_reopen(device.connectionhandle);
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
          /*if (config.fullLog != 0)
          {
            log_addTransferEvent(LOG_TR_STATUS1);
          }*/
        }
      }
    }
    
    // перезагрузиться в режим обновления FW
    if (device.rebootToUpdate){
      device.rebootToUpdate = 0;
      if (app_boot_request()) user_savereset();
      else {
        device.firmwarePercent = 252;
        device.FWinProgress = 0;
        device.status.readyToUpdate = 0;
        user_saveindex();
        user_send_statusUpdateFW(20, CMD_RUNERROR, (dataChanel_e)device.cchFWupdate, uniqIDCnt++);
      }
    }
    
    // таймаут обновления
    if (device.updateTimeout){
      if (--device.updateTimeout == 0){
        device.FWinProgress = 0;
        device.status.readyToUpdate = 0;
        device.firmwarePercent = 249;
        user_saveindex();
        if (device.registredOnServer)
          user_send_statusUpdateFW(20, CMD_RUNERROR, (dataChanel_e)device.cchFWupdate, uniqIDCnt++);
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
    if (config.useNTP){
      usergsm_getClock();
    }
    
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
    
    // раз в сутки
    if ((device.last_hour == 23) && (device.hour == 0)){
      
      // проверка баланса 
      if (config.ussdBalance[0] != 0){
        // информируем
        if (device.triggers.smsLowBalance){
          if (device.balance < 100){
            // информировать админа
            log_addSystemEvent(LOG_SYS_LOWBALANCE);
          }
        }
        // проверяем
        gsm_sendussd(config.ussdBalance);
      }
      
      // перезагрузить счетчик проходов раз в сутки
      user_reloadPassCounter();
      
    }
    device.last_hour = device.hour;
  } // 1 минута
}

// инициализация переменных
void user_initvariables (void)
{
  // чистим
  memset((pInt8U)&device, 0, sizeof(device));

  device.connectionhandle = -1;
  device.memID = w25_read_devID();
  device.freeBufferSize = 2048;
 
  // чтение сохранённой информации
  user_readsavedinformation();
  
  // загружаем параметры gsm
  GsmParam_t param;

  param.enable = 1;
  param.autoswitch_sim = 0;
  param.autorestart = 1;
  param.gprs_enable = 1;
  param.soft_simswitch = 0;
  param.textMode = 1;
  param.SMSReceived = user_smsreceived;
  param.SMSSended = user_smssended;
  param.IncomingCall = user_incomingcall;
  param.moduleType = (moduleType_e)device.moduleType;
  
  usergsm_init (&param);
  
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

// Чтение конфигурации с памяти
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
}

// запись конфигурации в память
void user_writeconfig(void)
{
  uint8_t *ptr = malloc(MEMSECTORSIZE);

  if (ptr != NULL){
#ifdef DEBUG_USER
  user_debug(">>U Write config ...\r\n"); 
#endif    
    
    wdt_reload();
    config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);
    mem_rewrite(mem[MB_CONFIG].start, (pInt8U)&config, sizeof(config), ptr); 
    wdt_reload();
    
    free(ptr);    
  }
}

// Загрузить настройки по умолчанию
void user_setdefault (void)
{
  wdt_reload();
  
#ifdef DEBUG_USER
  user_debug(">>U Set default params ...\r\n"); 
#endif
  
  // чистим
  memset((pInt8U)&config, 0, sizeof(config));

  config.pinCode = 0x31323334;
  strcpy(config.server1, "93.189.229.197:15010");
  strcpy(config.server2, "93.189.229.197:15020");
  strcpy(config.countryCode, "+7");
  strcpy(config.netOperator, "internet,gdata,gdata");
  config.netType = NT_GSM;
  config.useServer1 = 1;
  
  config.rele[0].mode = 0;
  config.rele[0].onCmdServer = 1;
  config.rele[0].onOther = 1;
  config.rele[0].tImpulse = 1;
  config.rele[0].tPause = 3;
  config.rele[0].cnt = 1;
  
  device.loadDefaultConfig = 1;
  device.moduleType = M_OTHER;
  device.status.defaultConfig = 1;
  log_addSystemEvent(LOG_SYS_DEFCONF);
  
  user_writeConfigAndDevice();
  //user_writeconfig();
}

// Сброс всего
void user_factoryReset (void){
#ifdef DEBUG_USER
  user_debug(">>U Factory reset\r\n"); 
#endif  
  
  // закрываем соединение с сервером
  connection_deleteAll();
  
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
  
  // чистим конфиг
  user_setdefault();
  // реинициализируем переменные
  user_initvariables();
  
  // перезагружаем gsm
  usergsm_restart();
}

// чтение сохраненной информации
void user_readsavedinformation (void)
{
  device_s DV;
  uint8_t read_count = 10;
  
  while (read_count--){
    wdt_reload();
    // Блок DEVICE
    mem_read (mem[MB_CONFIG].start + sizeof(config), (pInt8U)&DV, sizeof(DV));
    if (DV.crc == crc16_calc((pInt8U)&DV, sizeof(DV)-2)){ // контрольная сумма совпала
      strcpy(device.IMEIGSM, DV.IMEIGSM); // восстанавливаем IMEI GSM

      device.protocolVersion = DV.protocolVersion;
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
      device.firmwarePercent = DV.firmwarePercent;
      device.FWinProgress = DV.FWinProgress;
      
      device.moduleType = DV.moduleType;
       
      break;
    }
    delay_ms(10);
  }
  fw_resume_after_boot();
}

// Безопастное завершение работы прибора
void user_savereset (void)
{
  user_saveindex(); // сохраняем индексы и т.п.
  wdt_reload();
  
  // Отключаем USB
  USB_PULLUP_OFF; // выключаем подтягивающий резистор

  // выключаем gsm
  usergsm_power(OFF);
  while (gsm_getPowerState()){
    wdt_reload();
    delay_ms(10);    
    gsm_pwrProc(10); // обработчик включения/выключения модуля
  }
  
  // перезагружаем
  NVIC_SystemReset();
}

// Сохранение указателей чтения/записи в памяти
void user_saveindex (void)
{
  uint8_t *ptr = malloc(MEMSECTORSIZE);
  
  if (ptr != NULL){
#ifdef DEBUG_USER
  user_debug(">>U Save index ...\r\n"); 
#endif    
    wdt_reload();
    // считаем контрольную сумму
    device.crc = crc16_calc((pInt8U)&device, sizeof(device)-2);
    mem_rewrite(mem[MB_CONFIG].start + sizeof(config), (pInt8U)&device, sizeof(device), ptr);
    wdt_reload();
    
    free(ptr);    
  }
} 

// запись конфигурации и параметров device в память
void user_writeConfigAndDevice(void)
{
  uint8_t *ptr = malloc(MEMSECTORSIZE);

  if (ptr != NULL){
#ifdef DEBUG_USER
  user_debug(">>U Write config and device ...\r\n"); 
#endif    
    
    wdt_reload();
    
    mem_read (mem[MB_CONFIG].start, ptr, MEMSECTORSIZE);
    
    config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);    
    memcpy(ptr, (pInt8U)&config, sizeof(config));
  
    device.crc = crc16_calc((pInt8U)&device, sizeof(device)-2);
    memcpy(&ptr[sizeof(config)], (pInt8U)&device, sizeof(device));

    mem_write (mem[MB_CONFIG].start, ptr, MEMSECTORSIZE);

    wdt_reload();

    free(ptr);    
  }
}

// проверка порта 485
void port_check_data(void)
{
  // есть данные
  if (uart_getstate(PORT1_PORT)){
    // ждем завершения приема
    while (uart_isReceiving(PORT1_PORT)){
      wdt_reload();
    }
    
    uint16_t count = uart_received(PORT1_PORT);
    char *buf = calloc(count + 8, 1);
    
    if (buf != NULL){
      // копируем во временный
      memcpy(buf, uart_getptr(PORT1_PORT), count);
      uart_clear(PORT1_PORT);
      
      //777 парсим
      
      free(buf);
    } else{
      uart_clear(PORT1_PORT);
    }
  }
}

// проверка входящих данных от модуля
void gsm_check_data (void)
{
  // есть данные
  if (uart_getstate(GSM_PORT)){
    // ждем завершения приема
    while (uart_isReceiving(GSM_PORT)){
      wdt_reload();
    }
    
    uint16_t count = uart_received(GSM_PORT);
    char *buf = calloc(count + 8, 1);
    
    if (buf != NULL){
      // копируем во временный
      memcpy(buf, uart_getptr(GSM_PORT), count);
      uart_clear(GSM_PORT);
      buf[count] = 0;
      
      // парсим
      gsm_read(buf, count);
      
      free(buf);
    } else{
      uart_clear(GSM_PORT);
    }
  }
}

// расчет контрольной суммы структуры данных во flash
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

// Отправка данных в формате протокола
void user_send_anyData(uint8_t id, uint8_t* data, uint16_t size, dataChanel_e cch, uint8_t uniqId){
  uint16_t crc;
  uint16_t len = size + 3; // timeout comand + uniqId + id + payload  
  uint8_t *send_buffer = malloc(len + 8);
  
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

    memcpy(&send_buffer[9], data, size); // imei
    // ------- меняемый блок ---------
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      connection_send(device.connectionhandle, send_buffer, len + 8);
      break;
      
    case CH_SMS:
      
      break;
    } 
    
    free(send_buffer);
  }
}

// Пакет 1 - пакет регистрации контроллера на сервере
void user_send_registrationData(dataChanel_e cch){
  
  uint16_t crc;
  uint16_t len = 40; // timeout comand + uniqIDCnt + id + payload
  uint8_t send_buffer[48];
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }    
}

// Пакет 2 - пакет статуса 1
void user_send_statusPack(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 18; // timeout comand + uniqIDCnt + id + payload
  uint8_t send_buffer[26];
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
    device.answerID = 2;
    device.answerTimeout = TIMETOSTATUSRESPONSE;
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 3 - пакет статуса 2
void user_send_statusPackSecond(dataChanel_e cch, uint8_t uniqId){
    
  uint16_t crc;
  uint16_t len = 24; // timeout comand + uniqId + id + payload
  uint8_t send_buffer[32];
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 4 - пакет статуса обновления FW
void user_send_statusUpdateFW(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId){
  uint16_t crc;
  uint16_t len = 5; // timeout comand + uniqId + id + payload
  uint8_t send_buffer[13];
  
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
  send_buffer[10] = cc;
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 5 - контрольные суммы данных структур
void user_send_crcPack(dataChanel_e cch, uint8_t uniqId){
  uint16_t crc;
  uint16_t len = 13; // timeout comand + uniqId + id + payload
  uint16_t crcData;
  uint8_t send_buffer[21];
  
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
  
  crcData = user_calcCRC16InFlash(mem[MB_ID].start, mem[MB_ID].size);
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }

}

// Пакет 6,15 -  пакет контрольные суммы стуктуры ID абонентов
void user_send_crcMemoryID(uint8_t idx, dataChanel_e cch, uint8_t uniqId){
  uint16_t crc;
  uint16_t len; // timeout comand + , uint8_t uniqId + id + payload
  uint16_t crcData;
  uint8_t memAddress = mem[MB_ID].start;
  uint32_t sizeBlock;
  
  if (idx == 0){
    len = 203;
  } else{
    len = 5;
  }
  
  uint8_t *send_buffer = malloc(len + 8);
  
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
        crcData = user_calcCRC16InFlash(memAddress, sizeBlock);
        memcpy(&send_buffer[9 + (i*2)], (uint8_t*)&crcData, 2); // crc id
        memAddress += sizeBlock;
      }
    } else{ // конкретный id
      send_buffer[8] = 15; // id
      
#ifdef DEBUG_USER
  user_debug(">>U Send crc data (cmd 16)\r\n"); 
#endif  
      
      crcData = user_calcCRC16InFlash(memAddress + (sizeBlock*idx), sizeBlock);
      memcpy(&send_buffer[9], (uint8_t*)&crcData, 2); // crc id
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
      connection_send(device.connectionhandle, send_buffer, len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }
    free(send_buffer);
  }
}

// Пакет 7 -  трансляция SMS
void user_send_smsToServer(char* sms){
  uint16_t crc;
  uint16_t len = strlen(sms) + 3; // timeout comand + uniqIdCnt + id + payload
  uint8_t *send_buffer = malloc(len + 8);
  
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
    
    connection_send(device.connectionhandle, send_buffer, len + 8);
    
    free(send_buffer);
  }
}

// Пакет 8 - трансляция ID со считывателей
// cmdState: 1 - норм, 255 - таймаут
void user_send_readedIDtoServer(uint8_t* id, uint8_t cmdState){
  uint16_t crc;
  uint16_t len = 14; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[22];
  
#ifdef DEBUG_USER
  user_debug(">>U IDs data (cmd 8)\r\n"); 
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
  send_buffer[8] = 8; // id
  send_buffer[9] = cmdState; // Код выполнения
  if (id == 0)
    memset(&send_buffer[10], 0, 10);
  else
    memcpy(&send_buffer[10], id, 10);
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  connection_send(device.connectionhandle, send_buffer, len + 8);
}

// Пакет 11 - подтверждение приема команды от сервера
void user_send_answer_comand(uint8_t cmd, cmdCode_e cc, dataChanel_e cch, uint8_t uniqId){
  
  uint16_t crc;
  uint16_t len = 11; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[19];
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
  send_buffer[6] = 255; // таймаут актуальности команды
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 12 - таймштамп структур
void user_send_timeshtamp(shtamp_e sht, uint32_t time, uint16_t crcData, dataChanel_e cch, uint8_t uniqId){
  
  uint16_t crc;
  uint16_t len = 10; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[18];
  
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 17 - запрос информации о привязки контроллера к объекту
/*void user_send_getInfoObject(void){
  uint16_t crc;
  uint16_t len = 18; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[26];
  
#ifdef DEBUG_USER
  user_debug(">>U Get info (cmd 17)\r\n"); 
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
  send_buffer[8] = 17; // id
  memcpy(&send_buffer[9], (uint8_t*)&device.IMEIGSM, 15);
  // ------- меняемый блок ---------
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  connection_send(device.connectionhandle, send_buffer, len + 8);
}*/

// Пакет 100 - идентификация
void user_send_identPackToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 19; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[27];
  
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
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 101 - пакет статуса 3
void user_send_statusPackToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = 34; // timeout comand + uniqIdCnt + id + payload
  uint8_t send_buffer[42];
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
  
  crc = crc16_calc(&send_buffer[4], len + 2);
  
  send_buffer[len + 6] = crc&0xff; // crc
  send_buffer[len + 7] = (crc>>8)&0xff;
  
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }
}

// Пакет 106 - параметры конфигурации
void user_send_configToConfigurator(dataChanel_e cch){
    
  uint16_t crc;
  uint16_t len = sizeof(config_s) + 3; // timeout comand + uniqIdCnt + id + payload
  uint8_t *send_buffer = malloc(len + 8);
  
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
      connection_send(device.connectionhandle, send_buffer, len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    free(send_buffer);
  }
}

// Пакет 10 - выгрузка журнала событий
void user_send_logRecords(dataChanel_e cch, uint8_t uniqId){
 
  // читаем кол-во записей в памяти
  device.logsCount = log_getRecordsCount();

  // записей нет
  if (device.logsCount == 0){
    uint16_t tmp16 = device.logsCount;
    user_send_anyData(10, (uint8_t*)&tmp16, 2, cch, uniqId);
    device.status.readyLogData = 0;
    device.readLogIDIndex = 0;
    return;
  }
  
  uint32_t readAdr = mem[MB_LOG].start + ((uint32_t)device.readLogIDIndex * sizeof(cellLog_s));
  
  // подтверждение приема - выгрузили все
  if (readAdr >= device.newAddressLog){
    uint16_t tmp16 = device.logsCount;
    log_clearLogBuffer();
    user_send_anyData(10, (uint8_t*)&tmp16, 2, cch, uniqId);
    return;
  }

  uint16_t crc;  
  uint16_t cntId = device.logsCount - device.readLogIDIndex;

  if (cntId > 32) cntId = 32;
  // выделим память 1 кБ под записи журнала (32 * 32)
  uint8_t *send_buffer = malloc((cntId * sizeof(cellLog_s)) + 16);
  uint16_t len = (cntId * sizeof(cellLog_s)) + 6;
  uint16_t cntId2 = device.logsCount - device.readLogIDIndex - cntId;
  
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
    mem_read (readAdr, &send_buffer[12], cntId * sizeof(cellLog_s));
    
    memcpy(bf, send_buffer, 128);
    
    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      connection_send(device.connectionhandle, send_buffer, len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    device.readLogIDIndex += cntId;
    device.status.readyLogData = 1;
    
    free(send_buffer);
  }
}

// Пакет 9 - выгрузка ID из временного буфера
void user_send_tempIDs(dataChanel_e cch, uint8_t uniqId){
 
  // читаем кол-во записей в памяти
  device.tempIDsCount = user_getTempIDRecordsCount();

  // записей нет
  if (device.tempIDsCount == 0){
    uint16_t tmp16 = device.tempIDsCount;
    user_send_anyData(9, (uint8_t*)&tmp16, 2, cch, uniqId);
    device.status.readyID = 0;
    device.readTempIDIndex = 0;
    return;
  }
  
  uint32_t readAdr = mem[MB_TEMPID].start + ((uint32_t)device.readTempIDIndex * sizeof(cellID_s));
  
  // подтверждение приема - выгрузили все
  if (readAdr >= device.newAddressTempID){
    uint16_t tmp16 = device.tempIDsCount;
    user_clearTempIdBuffer();
    user_send_anyData(9, (uint8_t*)&tmp16, 2, cch, uniqId);
    return;
  }

  uint16_t crc;  
  uint16_t cntId = device.tempIDsCount - device.readTempIDIndex;

  if (cntId > 100) cntId = 100;
  // выделим память 1 кБ под ID (100 * 10)
  uint8_t *send_buffer = malloc((cntId * 10) + 16);
  uint16_t len = (cntId * 10) + 6;
  uint16_t cntId2 = device.tempIDsCount - device.readTempIDIndex - cntId;
  
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
      mem_read (readAdr + 2, &send_buffer[12 + (i*10)], 10);
      readAdr += sizeof(cellID_s);
    }

    crc = crc16_calc(&send_buffer[4], len + 2);
    
    send_buffer[len + 6] = crc&0xff; // crc
    send_buffer[len + 7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len + 8);
      break;
      
    case CH_SERVER:
      connection_send(device.connectionhandle, send_buffer, len + 8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    device.readTempIDIndex += cntId;
    device.status.readyID = 1;
    
    free(send_buffer);
  }
}

// Пакет 13 - Запрос конкретного блока прошивки
void user_getFWblockFromServer(dataChanel_e cch, uint16_t block){
  uint8_t send_buffer[16];
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
    
  switch (cch){
  case CH_USB:
    usb_send(send_buffer, len + 8);
    break;
    
  case CH_SERVER:
    connection_send(device.connectionhandle, send_buffer, len + 8);
    break;
    
  case CH_SMS:
    
    break;
  }    
}

// Пакет 14 - передача фотографии
void user_send_Photos(dataChanel_e cch, uint8_t uniqId){
//777
/*Фото в очереди	1	Кол-во фото в очереди		0 - нет фото на передачу (следующие поля не включены в пакет) 
Номер фото	4	Дататайм формирования фото		
Размер фото	4	Размер фото в байтах		
Номер блока	2	Блок размером не более 1024 байт		Номер блока можно заменить адресом смещения от начала файла. Как удобнее серверу?
Блок данных	N	Данные фото		*/ 
  
  // читаем кол-во записей в памяти
  device.countPhoto = user_getPhotoCount();

  // записей нет
  if (device.countPhoto == 0){
    uint16_t tmp16 = device.countPhoto;
    user_send_anyData(14, (uint8_t*)&tmp16, 2, cch, uniqId);
    device.status.readyPhoto = 0;
    device.readPhotoIndex = 0;
    return;
  }
  //777
/*  uint32_t readAdr = mem[MB_TEMPID].start + ((uint32_t)device.readTempIDIndex * sizeof(cellID_s));
  
  // подтверждение приема - выгрузили все
  if (readAdr >= device.newAddressTempID){
    user_clearTempIdBuffer();
    user_send_anyData(9, (uint8_t*)&device.tempIDsCount, 2, cch);
    return;
  }

  uint16_t crc;  
  uint16_t cntId = device.tempIDsCount - device.readTempIDIndex;

  if (cntId > 100) cntId = 100;
  // выделим память 1 кБ под ID (100 * 10)
  uint8_t *send_buffer = malloc((cntId * 10) + 16);
  uint16_t len = (cntId * 10) + 5;
  uint16_t cntId2 = device.tempIDsCount - device.readTempIDIndex - cntId;
  
  if (send_buffer != NULL){
    
#ifdef DEBUG_USER
  usb_sendstr(">>U Send temp IDs (cmd 9)\r\n"); 
#endif      
    
    send_buffer[0] = 'S'; // идент
    send_buffer[1] = 'L';
    send_buffer[2] = 'D';
    send_buffer[3] = 'N';
    send_buffer[4] = _1B(len);  // длина
    send_buffer[5] = _2B(len);
    send_buffer[6] = 255; // таймаут актуальности команды
    send_buffer[7] = 9; // id        
    send_buffer[8] = _1B(cntId2);  // остаток записей
    send_buffer[9] = _2B(cntId2);
    send_buffer[10] = cntId; // записей в текущем пакете
    // читаем из памяти
    for (uint16_t i = 0; i < cntId; i++){
      mem_read (readAdr + 2, &send_buffer[11 + (i*10)], 10);
      readAdr += sizeof(cellID_s);
    }

    crc = crc16_calc(&send_buffer[4], len+2);
    
    send_buffer[len+6] = crc&0xff; // crc
    send_buffer[len+7] = (crc>>8)&0xff;
    
    switch (cch){
    case CH_USB:
      usb_send(send_buffer, len+8);
      break;
      
    case CH_SERVER:
      connection_send(device.connectionhandle, send_buffer, len+8);
      break;
      
    case CH_SMS:
      
      break;
    }    
    
    device.readTempIDIndex += cntId;
    device.status.readyID = 1;
    
    free(send_buffer);
  }*/
}

/* Command 19 fields. Number 0 is reserved; number 1 rolls back to the backup
 * copy, so build, size, CRC and block count are not used and may be zero. */
static uint8_t fw_start_rejected(uint16_t number, uint32_t build, uint32_t size, uint32_t crc, uint16_t blocks)
{
  if (BOOT_VER < 0x30 || number == 0) return 1;
  if (number == 1) return 0;
  return (size > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) ||
         ((size & 3u) != 0) ||
         (blocks < (size + 1023u) / 1024u) ||
         ((FWBLOCK_OFFSET + (uint32_t)blocks * 1024u) > mem[MB_FIRMWARE].size) ||
         (build == 0) || (size == 0) || (crc == 0) || (blocks == 0);
}

/* Command 19 accepted (protocol E119, changes row 14): the server gets 5,
 * "ready to receive". NextConfig over USB still expects 0 until it is updated. */
static cmdCode_e fw_start_accepted(dataChanel_e cch)
{
  return cch == CH_USB ? CMD_SUCCESS : CMD_RUNNING;
}

/* All blocks received. A bad image CRC also ends the update: stop requesting
 * blocks, clear status bit 11 (table F437) and report 252 (table D473). */
static void fw_blocks_done(uint8_t crc_ok)
{
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;
  if (crc_ok) {
    device.rebootToUpdate = 1;
    device.firmwarePercent = 248;
  } else {
    device.firmwarePercent = 252;
  }
  if (device.cchFWupdate != CH_USB) user_saveindex();
}

/* Blocks are programmed into the area erased by command 19 and never erased
 * again: programming a block twice leaves the same bytes, programming a torn
 * block completes it. Resume from the last block holding any programmed byte,
 * the one that may be torn. */
static uint16_t fw_resume_block(void)
{
  uint8_t b[64];
  uint16_t block = device.FWblockCount, off, i;
  while (block != 0) {
    --block;
    for (off = 0; off < 1024u; off += sizeof(b)) {
      mem_read(mem[MB_FIRMWARE].start + FWBLOCK_OFFSET + (uint32_t)block * 1024u + off, b, sizeof(b));
      for (i = 0; i < sizeof(b); i++)
        if (b[i] != 0xFF) return block;
    }
    wdt_reload();
  }
  return 0;
}

/* After a reset, continue a server download that was in progress (customer
 * 15.09.2026). The staged header must still describe an update: erased magic,
 * a firmware number of 2 or more and fields command 19 would accept. */
static void fw_resume_after_boot(void)
{
  bootFWparam_s h;
  if (!device.FWinProgress) return;
  mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h));
  if (h.appIdent != 0xFFFFFFFFu || h.firmwareVersion < 2 ||
      fw_start_rejected(h.firmwareVersion, h.buildDate, h.firmwareSize, h.firmwareCRC, device.FWblockCount)) {
    device.FWinProgress = 0;
    user_saveindex();
    return;
  }
  device.FWblockIdx = fw_resume_block();
  device.startWaitFWtime = 0;
  device.cchFWupdate = CH_SERVER;
  device.updateTimeout = 3600;
  device.status.readyToUpdate = 1;
  device.firmwarePercent = device.FWblockIdx * 100 / device.FWblockCount;
}

// Разбор данных
void user_parse_comand(uint8_t* ptr, dataChanel_e cch){
  
  uint16_t len, crc, i;
  //uint8_t cmd_timeout;
  uint8_t uniqId;
  uint32_t tmp32;

  len = ptr[1]; // длина
  len = (len<<8) | ptr[0];
  crc = ptr[len + 3];  // crc
  crc = (crc<<8) | ptr[len + 2];
  i = crc16_calc(&ptr[0], len + 2);

  //cmd_timeout = ptr[2];
  uniqId = ptr[3];
  wdt_reload();  
  
  if (crc != i) // ошибка контрольной суммы
  {
    user_send_answer_comand(ptr[4], CMD_CRCERROR, cch, uniqId);
    return;
  }

  switch (ptr[4]) // команда
  {
    case 1: // Ответ контроллеру на регистрацию
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd reg (cmd 1)\r\n"); 
#endif        
      if (cch != CH_SERVER) break; // ответ на регистрацию бывает только от сервера
      device.answerTimeout = 0;
      switch ((cmdCode_e)ptr[5]){
      case CMD_SUCCESS:
        memcpy((uint8_t*)&tmp32, &ptr[6], 4); // время сервера
        device.datetime = tmp32;
        device.registredOnServer = 1;
        app_trial_confirm(); // прошивка дошла до сервера - обновление подтверждено
        if (device.firmwarePercent == 252 && app_boot_error_unreported()){
          user_send_statusUpdateFW(20, CMD_RUNERROR, cch, uniqIDCnt++);
          app_boot_error_reported();
        }
        device.lmTimer = 5;
        device.lmCount = 0;
        break;
        
      case CMD_PARAMERROR:
      case CMD_RUNERROR:
      case CMD_UNDEFRESULT:
        device.registredOnServer = 0;
        connection_reopen(device.connectionhandle);
        break;
        
      case CMD_UNDEFIMEI:
      case CMD_UNDEFOBJECT:
        // пробная прошивка могла сама прочитать IMEI неверно: не подтверждаем и базу не стираем,
        // при повторении откат по таймауту вернёт прежнюю прошивку
        if (!app_trial_active())
          user_factoryReset();
        device.registredOnServer = 0;
        connection_reopen(device.connectionhandle);
        break;
       
      default:
        break;
      }
      break;

    case 2: // перезагрузить
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd reset (cmd 2)\r\n"); 
#endif 
      log_addComandEvent(LOG_CMD_RESET, 0);
      if (ptr[5] == 0){
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        log_addSystemEvent(LOG_SYS_RESETONCMD);
        delay_ms(1000);
        user_savereset();
      } else{
        user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        device.timeToReset = ptr[5];
      }
      break;
    
    case 3: // команда установить заводские настройки
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd factory (cmd 3)\r\n"); 
#endif 
      log_addComandEvent(LOG_CMD_FACTORY, 0);
      if (ptr[5] == 0){
        user_factoryReset();
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      } else{
        user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        device.timeToDefault = ptr[5];
      }
      break;      

    case 5: // Установка состояния триггерных функций
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd triggers (cmd 5)\r\n"); 
#endif 
      if ((len - 3) == 16){
        log_addComandEvent(LOG_CMD_SETTRIGGER, 0);
        memcpy((uint8_t*)&device.triggers, &ptr[5], 16);
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        
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
        user_saveindex();   
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
        log_addComandEvent(LOG_CMD_GETIDHARD, 0);
        device.timeToGetID = ptr[5];
        if (device.timeToGetID){
          device.sourceGetID = ptr[6];
          device.addressGetID = ptr[7];
        }
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);        
      }
      break;      

    case 7: // запрос фото с камеры
      //777
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get photo (cmd 7)\r\n"); 
#endif 
      if (ptr[5] != 0){
        log_addComandEvent(LOG_CMD_GETPHOTO, 0);
        if (device.status.readyPhoto)
        {
          user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
          device.getPhoto = ptr[5];      
        }
        else
        {
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          device.getPhoto = 0; 
        }        
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      

    case 8: // управление выходами
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd set out (cmd 8)\r\n"); 
#endif 
      if (ptr[5] != 0){
        //ptr[5] - источник события (3 - тест из админки)
        //ptr[8] - ID абонента для передачи контроллеру информации для какого ID нужно сделать декримент счетчика проходов в сутки.
        if (ptr[5] != 3){
          strncpy(tempStrBuffer, (char*)&ptr[8], 10);
          user_activateOut(0, CT_SERVER, tempStrBuffer);
          user_activateOut(1, CT_SERVER, tempStrBuffer);        
        } else{
          out_load (OUT1, (ptr[6] != 0)?OUT_ON:OUT_OFF, 0, 0, 0);
          out_load (OUT2, (ptr[7] != 0)?OUT_ON:OUT_OFF, 0, 0, 0);          
        }
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);        
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      

    case 9: // запрос журнала событий
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get log (cmd 9)\r\n"); 
#endif 
      //log_addComandEvent(LOG_CMD_GETLOG);
      user_send_logRecords(cch, uniqId);
      break;      

    case 10: // запрос ID добавленных через SMS или Авторегистрацию
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get IDs (cmd 10)\r\n"); 
#endif       
      log_addComandEvent(LOG_CMD_GETIDAUTOREGSMS, 0);
      user_send_tempIDs(cch, uniqId);
      break;      
      
    case 11: // отправить смс на заданный номер с заданным текстом
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd send sms (cmd 11)\r\n"); 
#endif 
      if ((ptr[5] != 0) && (ptr[15] != 0)){
        log_addComandEvent(LOG_CMD_SENDINGSMS, 0);
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        strcpy(tempStrBuffer, config.countryCode);
        strncat(tempStrBuffer, (char*)&ptr[5], 10);
        user_sendsms((char*)&ptr[15], tempStrBuffer);        
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
        
        if ((p[8] == 0) || (p[40] == 0) || (p[137] == 0)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        }
        else{
          log_addComandEvent(LOG_CMD_SYNCCONFIG, 0);
          memcpy((uint8_t*)&config, &ptr[5], sizeof(config));
          user_send_timeshtamp(SHTAMP_CONFIG, config.datetime, 
                               user_calcCRC16InFlash(mem[MB_CONFIG].start, mem[MB_CONFIG].size), cch, uniqId);
          // дататайм
          device.datetimeConfig = config.datetime;
          device.status.defaultConfig = 0;
          user_writeConfigAndDevice();           
        }
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 13: // отправить блок ID абонентов в контроллер
      {
        // определяем сектор, в который будем писать, читаем, стираем и пишем.
        uint16_t idCnt = ptr[9];
        uint16_t cellNum;
        uint32_t sector = -1;
        uint32_t newSector = -1;
        uint8_t cnt = 0;
        uint8_t group;
        
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd sync IDs (cmd 13)\r\n"); 
#endif 
  
         if(idCnt == 0){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }  
  
        if ((len - 8) != (idCnt * sizeof(cellID_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;         
        }
          
        log_addComandEvent(LOG_CMD_SYNCID, 0);

        uint8_t *membfr = malloc(MEMSECTORSIZE);
        
        if (membfr == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }        
        
        // дататайм
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        device.datetimeID = tmp32;
        
        // пишем в память
        for (uint16_t i = 0; i<idCnt; i++){
          // номер ячейки
          cellNum = *(uint16_t*)&ptr[(i*sizeof(cellID_s)) + 10];
          group = *(uint16_t*)&ptr[(i*sizeof(cellID_s)) + 22];
          
          if ((cellNum == 0) || (group == 0)){
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
            break;
          }
          
          cellNum -= 1;
          // адрес сектора
          newSector = div(mem[MB_ID].start + (cellNum*sizeof(cellID_s)), 4096).quot * 4096;
          // сектор поменялся, читаем
          if (newSector != sector){
            // ранее сектор был считан, сохраняем
            if (cnt != 0){
              wdt_reload();
              mem_erase_sector(sector);
              mem_write(sector, membfr, MEMSECTORSIZE);
            }
            cnt = 0;
            mem_read(newSector, membfr, MEMSECTORSIZE);
            sector = newSector;
          }
          // копируем ячейку
          cellNum = mem[MB_ID].start + (cellNum*sizeof(cellID_s)) - newSector;
          memcpy(membfr + cellNum, &ptr[(i*sizeof(cellID_s)) + 10], sizeof(cellID_s));
          cnt++;
          device.numbersCount++;
        }
        
        // ранее сектор был считан, сохраняем
        if (cnt != 0){
          wdt_reload();
          mem_erase_sector(sector);
          mem_write(sector, membfr, MEMSECTORSIZE);
        }        
        
        user_send_timeshtamp(SHTAMP_ID, device.datetimeID, 
                           user_calcCRC16InFlash(mem[MB_ID].start, mem[MB_ID].size), cch, uniqId);
        
        free(membfr);
      }
      break;
      
    case 14: // синхро. расписаний
      {
        uint8_t idCnt = ptr[9];
        uint16_t cellNum;
        uint32_t sector = -1;
        uint32_t newSector = -1;
        uint8_t cnt = 0;
        
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd sync shedule (cmd 14)\r\n"); 
#endif     
    
        if(idCnt == 0){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }  
  
        if ((len - 8) != (idCnt * sizeof(cellShedule_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;         
        }
        
        log_addComandEvent(LOG_CMD_SYNCSHEDULE, 0);

        uint8_t *membfr = malloc(MEMSECTORSIZE);
        
        if (membfr == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        
        // дататайм
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        device.datetimeShedule = tmp32;
        
        // пишем в память
        for (uint16_t i = 0; i<idCnt; i++){
          // номер ячейки
          cellNum = ptr[(i*sizeof(cellShedule_s)) + 10];
          
          if (cellNum == 0){
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
            break;
          }
          
          cellNum -= 1;
          // адрес сектора
          newSector = div(mem[MB_SHEDULE].start + (cellNum*sizeof(cellShedule_s)), 4096).quot * 4096;
          // сектор поменялся, читаем
          if (newSector != sector){
            // ранее сектор был считан, сохраняем
            if (cnt != 0){
              wdt_reload();
              mem_erase_sector(sector);
              mem_write(sector, membfr, MEMSECTORSIZE);
            }
            cnt = 0;
            mem_read(newSector, membfr, MEMSECTORSIZE);
            sector = newSector;
          }
          // копируем ячейку
          cellNum = mem[MB_SHEDULE].start + (cellNum*sizeof(cellShedule_s)) - newSector;
          memcpy(membfr + cellNum, &ptr[(i*sizeof(cellShedule_s)) + 10], sizeof(cellShedule_s));
          cnt++;
        }
        
        // ранее сектор был считан, сохраняем
        if (cnt != 0){
          wdt_reload();
          mem_erase_sector(sector);
          mem_write(sector, membfr, MEMSECTORSIZE);
        }        

        user_send_timeshtamp(SHTAMP_SHEDULE, device.datetimeShedule, 
                           user_calcCRC16InFlash(mem[MB_SHEDULE].start, mem[MB_SHEDULE].size), cch, uniqId); 
      
        free(membfr);
      }      
      break;
      
    case 15: // синхро групп 
      {
        uint8_t idCnt = ptr[9];
        uint16_t cellNum;
        uint32_t sector = -1;
        uint32_t newSector = -1;
        uint8_t cnt = 0;
        
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd sync group (cmd 15)\r\n"); 
#endif         
        
        if(idCnt == 0){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
  
        if ((len - 8) != (idCnt * sizeof(cellGroup_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;         
        }
                
        log_addComandEvent(LOG_CMD_SYNCGROUP, 0);

        uint8_t *membfr = malloc(MEMSECTORSIZE);
        
        if (membfr == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        
        // дататайм
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        device.datetimeGroup = tmp32;
 
        // пишем в память
        for (uint16_t i = 0; i<idCnt; i++){
          // номер ячейки
          cellNum = ptr[(i*sizeof(cellGroup_s)) + 10];
          
          if (cellNum == 0){
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
            break;
          }
          
          cellNum -= 1;
          // адрес сектора
          newSector = div(mem[MB_GROUP].start + (cellNum*sizeof(cellGroup_s)), 4096).quot * 4096;
          // сектор поменялся, читаем
          if (newSector != sector){
            // ранее сектор был считан, сохраняем
            if (cnt != 0){
              wdt_reload();
              mem_erase_sector(sector);
              mem_write(sector, membfr, MEMSECTORSIZE);
            }
            cnt = 0;
            mem_read(newSector, membfr, MEMSECTORSIZE);
            sector = newSector;
          }
          // копируем ячейку
          cellNum = mem[MB_GROUP].start + (cellNum*sizeof(cellGroup_s)) - newSector;
          memcpy(membfr + cellNum, &ptr[(i*sizeof(cellGroup_s)) + 10], sizeof(cellGroup_s));
          cnt++;
        }
        
        // ранее сектор был считан, сохраняем
        if (cnt != 0){
          wdt_reload();
          mem_erase_sector(sector);
          mem_write(sector, membfr, MEMSECTORSIZE);
        } 
        
        user_send_timeshtamp(SHTAMP_GROUP, device.datetimeGroup, 
                           user_calcCRC16InFlash(mem[MB_GROUP].start, mem[MB_GROUP].size), cch, uniqId); 
      
        free(membfr);
      }
      break;
      
    case 16: // синхро. праздников
      {
        uint8_t idCnt = ptr[9];
        uint16_t cellNum;
        uint32_t sector = -1;
        uint32_t newSector = -1;
        uint8_t cnt = 0;
        
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd sync holiday (cmd 16)\r\n"); 
#endif         
        
        if(idCnt == 0){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }  
        
        if ((len - 8) != (idCnt * sizeof(cellHoliday_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;         
        }
  
        log_addComandEvent(LOG_CMD_SYNCHOLLIDAY, 0);

        uint8_t *membfr = malloc(MEMSECTORSIZE);
        
        if (membfr == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        
        // дататайм
        memcpy((uint8_t*)&tmp32, &ptr[5], 4);
        device.datetimeHoliday = tmp32;
        
        // пишем в память
        for (uint16_t i = 0; i<idCnt; i++){
          // номер ячейки
          cellNum = ptr[(i*sizeof(cellHoliday_s)) + 10];
          
          if (cellNum == 0){
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
            break;
          }
          
          cellNum -= 1;
          // адрес сектора
          newSector = div(mem[MB_HOLIDAY].start + (cellNum*sizeof(cellHoliday_s)), 4096).quot * 4096;
          // сектор поменялся, читаем
          if (newSector != sector){
            // ранее сектор был считан, сохраняем
            if (cnt != 0){
              wdt_reload();
              mem_erase_sector(sector);
              mem_write(sector, membfr, MEMSECTORSIZE);
            }
            cnt = 0;
            mem_read(newSector, membfr, MEMSECTORSIZE);
            sector = newSector;
          }
          // копируем ячейку
          cellNum = mem[MB_HOLIDAY].start + (cellNum*sizeof(cellHoliday_s)) - newSector;
          memcpy(membfr + cellNum, &ptr[(i*sizeof(cellHoliday_s)) + 10], sizeof(cellHoliday_s));
          cnt++;
        }
        
        // ранее сектор был считан, сохраняем
        if (cnt != 0){
          wdt_reload();
          mem_erase_sector(sector);
          mem_write(sector, membfr, MEMSECTORSIZE);
        }
        
        user_send_timeshtamp(SHTAMP_HOLIDAY, device.datetimeHoliday, 
                           user_calcCRC16InFlash(mem[MB_HOLIDAY].start, mem[MB_HOLIDAY].size), cch, uniqId); 
      
        free(membfr);
      }
      break;
          
    case 17: // CRC всех 5-ти блоков данных
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get crc (cmd 17)\r\n"); 
#endif 
      log_addComandEvent(LOG_CMD_GETCRCSTRUCT, 0);
      user_send_crcPack(cch, uniqId);
      break;      
      
    case 18: // запрос CRC базы абонентов поблочно
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd crc ID (cmd 18)\r\n"); 
#endif       
      log_addComandEvent(LOG_CMD_GETCRCIDBLOCK, 0);
      user_send_crcMemoryID(ptr[5], cch, uniqId);
      break;      
      
    case 19: // Обновление прошивки
      if (boot_manual_busy() || app_trial_active()) {
        user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        break;
      }
#ifdef DEBUG_USER
  if (cch != CH_USB)    
    user_debug(">>U Recv cmd fw update (cmd 19)\r\n"); 
#endif   
      if ((len - 3) == 16){
        if (device.FWinProgress){
          user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        } else{
          uint16_t firmwareVersion;
          uint32_t buildDate;
          uint32_t firmwareSize;
          uint32_t firmwareCRC;
          uint16_t FWblockCount;

          memcpy((uint8_t*)&firmwareVersion, &ptr[5], 2);
          memcpy((uint8_t*)&buildDate, &ptr[7], 4);
          memcpy((uint8_t*)&firmwareSize, &ptr[11], 4);
          memcpy((uint8_t*)&firmwareCRC, &ptr[15], 4);
          memcpy((uint8_t*)&FWblockCount, &ptr[19], 2);
          
          if (fw_start_rejected(firmwareVersion, buildDate, firmwareSize, firmwareCRC, FWblockCount)){
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          } else{
            
            log_addComandEvent(LOG_CMD_RUNUPDATE, 0);
            
            // обновиться на предыдущую
            if (firmwareVersion == 1){
              bootFWparam_s fwParam;
              // читаем параметры
              mem_read(mem[MB_BACKUP].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s));
              // пусто
              if ((fwParam.buildDate == 0xffffffff) ||
                  (fwParam.firmwareSize == 0xffffffff) ||
                    (fwParam.firmwareCRC == 0xffffffff) ||
                      (fwParam.firmwareVersion == 0xffff)){
                user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
                device.firmwarePercent = 253;
                user_send_statusUpdateFW(ptr[4], CMD_RUNERROR, cch, uniqId);
              } else{
                // проверим контрольную сумму
                if (user_checkFirmware(mem[MB_BACKUP].start + FWBLOCK_OFFSET, fwParam.firmwareSize, fwParam.firmwareCRC)){
                  device.hardwareVersion = fwParam.hardwareVersion;
                  device.firmwareVersion = fwParam.firmwareVersion;
                  device.buildDate = fwParam.buildDate;
                  device.firmwareSize = fwParam.firmwareSize;
                  device.firmwareCRC = fwParam.firmwareCRC;
                  // копируем из резервной в основную
                  user_clearMemBlock(MB_FIRMWARE);
                  for (uint32_t block = 0; block < (fwParam.firmwareSize + FWBLOCK_OFFSET); block += 128){
                    mem_read(mem[MB_BACKUP].start + block, bf, 128);
                  if (block == 0) memset(bf, 0xff, 4);
                    mem_write(mem[MB_FIRMWARE].start + block, bf, 128);
                  }
                  
                  user_send_answer_comand(ptr[4], CMD_ROLLBACKRUN, cch, uniqId);
                  //device.FWinProgress = 1;
                  device.rebootToUpdate = 1;
                } else{
                  user_send_answer_comand(ptr[4], CMD_CRCERROR, cch, uniqId); 
                  device.firmwarePercent = 252;
                  user_send_statusUpdateFW(ptr[4], CMD_RUNERROR, cch, uniqId);
                }
              }
            } else{
              
              //777 по какому параметру проверить версию железа?
              
              uint8_t *data = malloc(MEMSECTORSIZE);

              if (data != NULL){
                
                #ifdef DEBUG_USER
                if (cch != CH_USB)   
                  user_debug(">>U Erase FW memory ...\r\n"); 
                #endif
                
                user_clearMemBlock(MB_FIRMWARE);
                bootFWparam_s fwParam;
                memset(&fwParam, 0xff, sizeof(fwParam));
                fwParam.hardwareVersion = device.hardwareVersion;
                fwParam.firmwareVersion = firmwareVersion;
                fwParam.firmwareSize = firmwareSize;
                fwParam.firmwareCRC = firmwareCRC;
                fwParam.buildDate = buildDate;
                
                // блок информации о прошивке
                mem_write(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s));
                wdt_reload();

                device.FWblockCount = FWblockCount;
                device.FWblockIdx = 0;
                device.firmwarePercent = 0;
                device.startWaitFWtime = 0;
                device.cchFWupdate = cch;
                
                user_send_answer_comand(ptr[4], fw_start_accepted(cch), cch, uniqId);
                
                if (cch != CH_USB){
                  device.FWinProgress = 1;
                  device.updateTimeout = 3600; // запускаем таймаут на обновление
                  
                  device.status.readyToUpdate = 1;
                }
                
                free(data);
                /* Resume after a reboot: the download state must survive a reset. */
                if (cch != CH_USB) user_saveindex();
              } else{
                user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId); 
              }
            }
          }        
        }        
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      
      
    case 20: // Обновление прошивки
      if (boot_manual_busy()) {
        user_send_answer_comand(ptr[4], CMD_RUNNING, cch, uniqId);
        break;
      }
#ifdef DEBUG_USER
if (cch != CH_USB)   
  user_debug(">>U Recv cmd fw update (cmd 20)\r\n"); 
#endif 
      if (((len - 5) == 1024) && (*(uint16_t*)&ptr[5] == device.FWblockIdx)){

        log_addComandEvent(LOG_CMD_SENDUPDBLOCK, 0);
        
        // пишем в память
        {
          #ifdef DEBUG_USER
          if (cch != CH_USB){
            sprintf(tempStrBuffer, ">>U Write FW block %d ...\r\n", device.FWblockIdx);
            user_debug(tempStrBuffer);
          }
          #endif    

          uint32_t adr = mem[MB_FIRMWARE].start + FWBLOCK_OFFSET + ((uint32_t)device.FWblockIdx * 1024);
          
          /* Command 19 erased the image area: program in place. mem_rewrite skipped
           * every block crossing a 4 KiB sector (blocks 3, 7, 11, ...). */
          mem_write(adr, &ptr[7], 1024);
          wdt_reload();
          
          device.FWblockIdx += 1;
          device.startWaitFWtime = 0;

          // получены все блоки
          if (device.FWblockIdx == device.FWblockCount){
            bootFWparam_s fwParam;
            
            // блок информации о прошивке
            mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s)); 
            // проверяем контрольную сумму
            if (user_checkFirmware(mem[MB_FIRMWARE].start + FWBLOCK_OFFSET, fwParam.firmwareSize, fwParam.firmwareCRC)){
              device.firmwareVersion = fwParam.firmwareVersion;
              device.buildDate = fwParam.buildDate;
              device.firmwareSize = fwParam.firmwareSize;
              device.firmwareCRC = fwParam.firmwareCRC;

              fw_blocks_done(1);
            } else{
              fw_blocks_done(0);
            }
          } else{
            device.firmwarePercent = device.FWblockIdx * 100 / device.FWblockCount;
          }
          user_send_statusUpdateFW(ptr[4], CMD_SUCCESS, cch, uniqId);
        }
      } else{
        device.firmwarePercent = 255;
        user_send_statusUpdateFW(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;      
      
    case 21: // запрос фотографий 
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get photo (cmd 21)\r\n"); 
#endif 
      user_send_Photos(cch, uniqId);
      break;      
      
    case 22: // запрос статуса 2
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd get status (cmd 22)\r\n"); 
#endif 
      log_addComandEvent(LOG_CMD_GETSTATUS2, 0);
      user_send_statusPackSecond(cch, uniqId);
      /*if (config.fullLog != 0)
      {
        log_addTransferEvent(LOG_TR_STATUS2);
      }*/
      break;      
      
    case 23: // установить время контроллера
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd set time (cmd 23)\r\n"); 
#endif 
      log_addComandEvent(LOG_CMD_SETTIME, 0);
      memcpy((uint8_t*)&tmp32, &ptr[5], 4);
      device.datetime = tmp32;
      user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); 
      break;      
       
    case 24: // команда подтверждения приема команды от контроллера
#ifdef DEBUG_USER
  user_debug(">>U Recv cmd answer (cmd 24)\r\n"); 
#endif 
      if (device.answerID == 2)
        device.answerTimeout = 0;
  
      log_addComandEvent(LOG_CMD_ANSWER, 0);
      //ptr[5] - На какую именно команду выдается подтверждение
      //ptr[6] - код выполнения
      switch (ptr[5]){
        case 1:
          
          break;
          
        case 2:
          device.answerTimeout = 0;
          break;
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
      if (cch == CH_USB){
        user_clearMemBlock(MB_ID);
        user_send_anyData(103, NULL, 0, cch, uniqId);
      } 
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
      
    case MANUAL_BOOT_COMMAND: { /* Manual bootloader file, USB only. */
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
        /* Let USB send its acceptance. Success is checked after restart. */
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

// индикация
void user_indication (void)
{
  // NET LED
  if (gsm_getPowerState()) // питание подано
  {
    if (device.creg)
    {
      if (connection_state(device.connectionhandle) == CE_CONNECTED)
        led_load(NET_LED, LED_ON);
      else 
        led_load(NET_LED, LED_FL1);
    }
    else led_load(NET_LED, LED_FL2);
  }
  else led_load(NET_LED, LED_OFF);
}

// опрос входов
void user_readinputs (void){
  adc_startInjectedConvertion();
  
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
    if (device.status.stateIn1)
      log_addSystemEvent(LOG_SYS_IN1ON);
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

// вернуть указатель на 10ть последних цифр номера при номере более 10
char* user_last10digitsnumber (char* num)
{
  uint16_t l = strlen(num);

  return (l <= 10)?num:(num + (l-10));
}

// узнать текущий день недели 
// 0 - Пн, 1 - Вт ...
uint8_t user_getweakday (uint32_t t)
{
  // какой сейчас день по счету
  uint32_t d = div(t,86400).quot;
  uint8_t wd = div(d,7).rem; // день недели, 0 - Чт

  // преобразуем
  wd += 3;
  if (wd > 6) wd -= 7;

  return wd;
}


// ********************** Работа с ID **************************

// поиск ID в памяти, если есть то возвращаются параметры ячейки
bool user_findID(char *id, cellID_s *cell){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff) return false;
      // пустое поле ID
      uint16_t sum = 0;
      for (uint8_t i = 0; i<10; i++)
        sum += c->ID[i];
      if (sum == 2550) return false;
      // проверяем на совпадение
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
bool user_findTempID(char *id){
  
  uint32_t adr;
  //uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;
  
  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;
  
  // проверим, что такого ID нет в буфере
  for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s)){
    mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s));
    // такой ID есть
    if (memcmp(cellId.ID, id, 10) == 0){
      return true;
    }
  }
  // не нашли  
  return false;
}

// добавляет ID во временный буфер
void user_addIdToTempBuffer(char *id, codeType_e tp){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  cellID_s cellId;
  uint32_t startAddress = device.newAddressTempID;
  
  if (startAddress < mem[MB_TEMPID].start) startAddress = mem[MB_TEMPID].start;
  
  // проверим, что такого ID нет в буфере
  for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s)){
    mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s));
    // такой ID есть
    if (memcmp(cellId.ID, id, 10) == 0){
      return;
    }
  }
  
  // по странично 
  for (adr = startAddress; adr < endAdr; adr += sizeof(cellID_s)){
    mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s));
    // ячейка пустая, она должна быть стерта
    if (cellId.index == 0xffff){
      cellId.index = 0;
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
      mem_write(adr, (uint8_t*)&cellId, sizeof(cellID_s));
      device.newAddressTempID = adr + sizeof(cellID_s);
      device.status.readyID = 1;
      return;
    }
  }
}

// проверка ID по параметрам и управление выходом
// через нее проходят все ID
void user_checkIdAndControl(char *id, codeType_e tp){
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
  
  // проверяем ID из временного буфера
  if (user_findTempID(id)){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Open by temp ID\r\n"); 
#endif
    user_activateOut(0, tp, id);
    user_activateOut(1, tp, id);
    return;    
  }
  
  // авторегистрация
  if (device.status.autoRegistration){
    user_addIdToTempBuffer(id, tp);
    log_addControlEvent(LOG_CON_IDADDBYAUTOREG, id, 0, 0);
  }
  
  // открывать всем без разбора
  if (device.status.openAnyUser){
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Open any user\r\n"); 
#endif
    user_activateOut(0, tp, id);
    user_activateOut(1, tp, id);
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
      user_activateOut(0, tp, id);
      user_activateOut(1, tp, id);      
    }
    return;
  }
  
  bool onOut[2] = {false, false};
  
  // проверим можно ли управлять этим способом
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
  
  // можно, ищем ID
  if (onOut[0] || onOut[1]){
    cellID_s cellId;
    
    // ID найден
    if (user_findID(id, &cellId)){
      // проверим разрешение по дате запрета
      if ((cellId.blockTime != 0) && (cellId.blockTime < device.datetime))
      {
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Blocked on date\r\n"); 
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
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0, 0);
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
        user_activateOut(0, tp, id);
        user_activateOut(1, tp, id);
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
        log_addControlEvent(LOG_CON_IDBLOCKED, id, 0, 0);
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
      if (shedule.startHour <= shedule.stopHour){
        if ((device.hour >= shedule.startHour) && (device.minute >= shedule.startMinute) &&
            (device.hour <= shedule.stopHour) && (device.minute <= shedule.stopMinute)){
          flag = true;
        } 
      } else{
        if (((device.hour >= shedule.stopHour) && (device.minute >= shedule.stopMinute)) ||
            ((device.hour <= shedule.startHour) && (device.minute <= shedule.startMinute))){
          flag = true;
        }
      }
      
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
        flag = user_dayIsHoliday();
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
      
      user_activateOut(0, tp, id);
      user_activateOut(1, tp, id);
    } else{
#ifdef DEBUG_USER
    user_debug(">>U Check ID: Not founded\r\n"); 
#endif
      // ID не найден - включено информирование админа
      log_addControlEvent(LOG_CON_IDNOTINBASE, id, 0, 0);
      if (device.triggers.smsUnreg){
        sprintf(tempStrBuffer, "%s: ID is unregistred!", id);
        user_send_smsToServer(tempStrBuffer);
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

// управление выходом в соответствии с настройками
void user_activateOut(uint8_t out, codeType_e tp, char *id){
  
  if (out >= 2) return;  
#ifdef DEBUG_USER
  if (out == 0)
    user_debug(">>U Act Out: Out 1\r\n");
  else
    user_debug(">>U Act Out: Out 2\r\n");
#endif   
  // используется в режиме блокировки
  if (config.rele[out].useInBlock)
  {
#ifdef DEBUG_USER
    user_debug(">>U Act Out: Use in blocking\r\n"); 
#endif     
    return;
  }
  // узнаем выполняется алгоритм выхода или нет
  if (out_getmodestate(out))
  {
#ifdef DEBUG_USER
    user_debug(">>U Act Out: Is running\r\n"); 
#endif     
    return;
  }

  // проверим счетчик проходов
  uint8_t cnt = 255;
  cellID_s cellId;
  // ID найден
  if (user_findID(id, &cellId)){
    // задано кол-во проходов
    if (cellId.cntPassOnDay != 255){
      // есть проходы
      if (cellId.cntPass != 0){
        cellId.cntPass -= 1;
        cnt = cellId.cntPass;
        user_updateID(&cellId);      
      }
      else{
        return;
      }
    }
  }
    
  switch (tp){
  case CT_PHONE:
    log_addControlEvent(LOG_CON_OUTONBYPHONE, id, 0, cnt);
    break;
  case CT_WIEGAND:
    log_addControlEvent(LOG_CON_OUTONBYWIEGAND, id, 0, cnt);
    break;
  case CT_RF433:
    log_addControlEvent(LOG_CON_OUTONBYRF433, id, 0, cnt);
    break;
  case CT_RFID:
    log_addControlEvent(LOG_CON_OUTONBYRFID, id, 0, cnt);
    break;
  case CT_AUTO:
    log_addControlEvent(LOG_CON_OUTONBYAVTO, id, 0, cnt);
    break;
  case CT_SERVER:
    log_addControlEvent(LOG_CON_OUTONBYCMD, id, 0, cnt);
    break;
  default:
    break;
  }

#ifdef DEBUG_USER
    user_debug(">>U Act Out: Activate\r\n"); 
#endif   
  // импульс
  if (config.rele[out].mode == 0){
    out_load(out, OUT_IMP, config.rele[out].tImpulse, config.rele[out].tPause, config.rele[out].cnt);
  } else{
    // уровень
    out_load(out, OUT_INV, 0, 0, 0);
  }
}

// считать параметры группы
bool user_getGroup(uint16_t num, cellGroup_s *group){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_GROUP].start + mem[MB_GROUP].size;
  uint8_t tbuffer[1024];
  cellGroup_s *c;
  
  // по странично читаем
  for (adr = mem[MB_GROUP].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellGroup_s)){
      c = (cellGroup_s*)&tbuffer[a];
      if (c->index == num){
        memcpy(group, c, sizeof(cellGroup_s));
        return true;
      }
    }
  }
  // не нашли  
  return false;
}

// считать параметры расписания
bool user_getShedule(uint16_t num, cellShedule_s *shedule){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_SHEDULE].start + mem[MB_SHEDULE].size;
  uint8_t tbuffer[1024];
  cellShedule_s *c;
  
  // по странично читаем
  for (adr = mem[MB_SHEDULE].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellShedule_s)){
      c = (cellShedule_s*)&tbuffer[a];
      if (c->index == num){
        memcpy(shedule, c, sizeof(cellShedule_s));
        return true;
      }
    }
  }
  // не нашли  
  return false;
}

// проверить праздничный день
bool user_dayIsHoliday(void){
  
  uint8_t tbuffer[1024];
  cellHoliday_s *c;
  
  mem_read(mem[MB_HOLIDAY].start, tbuffer, 1024);
  
  // перебираем ячейки
  for (uint16_t a = 0; a < 1024; a += sizeof(cellHoliday_s)){
    c = (cellHoliday_s*)&tbuffer[a];
    if (div(c->datetime, 86400).quot == div(device.datetime, 86400).quot)
      return true;
  }
  
  return false;
}

// очистить буфер временных ID
void user_clearTempIdBuffer(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  
  // по секторно стираем
  for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 4096){
    mem_erase_sector(adr);
    wdt_reload();
  }
  
  device.status.readyID = 0;
  device.tempIDsCount = 0;
  device.readTempIDIndex = 0;
  device.newAddressTempID = mem[MB_TEMPID].start;
}

// сколько номеров в памяти
uint16_t user_getNumbersCount(void){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  uint16_t cnt = 0;
  
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff) return cnt;
      
      cnt++;
    }
  }
  
  return cnt;
}

// получаем кол-во ID во временном буфере и тут же обновляем адрес записи
uint16_t user_getTempIDRecordsCount(void){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_TEMPID].start + mem[MB_TEMPID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  uint16_t cnt = 0;
  
  // по странично читаем
  for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
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
  
  return cnt;
}

// проверить расписание
bool user_checkShedule(uint8_t shd){
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
  if (shedule.startHour <= shedule.stopHour){
    if ((device.hour >= shedule.startHour) && (device.minute >= shedule.startMinute) &&
        (device.hour <= shedule.stopHour) && (device.minute <= shedule.stopMinute)){
      flag = true;
    } 
  } else{
    if (((device.hour >= shedule.stopHour) && (device.minute >= shedule.stopMinute)) ||
        ((device.hour <= shedule.startHour) && (device.minute <= shedule.startMinute))){
      flag = true;
    }
  }
      
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
    flag = user_dayIsHoliday();
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

// обновить значение счетчика проходов на начало суток
void user_reloadPassCounter(void){
  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint8_t tbuffer[1024];
  cellID_s *c;
  bool change = false;
  
  // по странично читаем
  for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s)){
      c = (cellID_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffff) return;
      // пустое поле ID
      uint16_t sum = 0;
      for (uint8_t i = 0; i<10; i++)
        sum += c->ID[i];
      if (sum == 2550) return;
      // кол-во проходов задано
      if (c->cntPassOnDay != 255){
        c->cntPass = c->cntPassOnDay;
        change = true;
      }
    }
    // обновляем информацию, если были изменения
    if (change == true){
      mem_write(adr, tbuffer, 1024);
    }
  }
}

// обновить данные ID
void user_updateID(cellID_s *c){
  uint32_t adr;
  uint32_t endAdr = mem[MB_ID].start + mem[MB_ID].size;
  uint16_t index;
  
  // читаем index ячеек
  for (adr = mem[MB_ID].start; adr < endAdr; adr += sizeof(cellID_s)){
    mem_read(adr, (uint8_t*)&index, 2);
    if (index == c->index){
      uint8_t *ptr = malloc(MEMSECTORSIZE);
      
      if (ptr != NULL){
        // перезаписываем ячейку новыми данными
        mem_rewrite (adr, (uint8_t*)c, sizeof(cellID_s), ptr);
        free(ptr);
      }
      break;
    }
  }
}

// ********************** Работа с log **************************

// очистить буфер журнала
void log_clearLogBuffer(void){

  uint32_t adr;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  
  // по секторно стираем
  for (adr = mem[MB_LOG].start; adr < endAdr; adr += 4096){
    mem_erase_sector(adr);
    wdt_reload();
  }
  
  device.readLogIDIndex = 0;
  device.logsCount = 0;
  device.status.readyLogData = 0;
  device.newAddressLog = mem[MB_LOG].start;
}

// добавить событие в буфер
bool log_addEvent(cellLog_s *cl){
  uint32_t adr;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  cellLog_s cell;
  uint32_t startAddress = device.newAddressLog;
  
  if (startAddress < mem[MB_LOG].start) startAddress = mem[MB_LOG].start;
  // по странично 
  for (adr = startAddress; adr < endAdr; adr += sizeof(cellLog_s)){
    mem_read(adr, (uint8_t*)&cell, sizeof(cellLog_s));
    // ячейка пустая, она должна быть стерта
    if (cell.index == 0xffffffff){
      mem_write(adr, (uint8_t*)cl, sizeof(cellLog_s));
      device.newAddressLog = adr + sizeof(cellLog_s);
      device.status.readyLogData = 1;
      return true;
    }
  }
  
  return false;
}

// получаем кол-во записей в журнале
uint16_t log_getRecordsCount(void){
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_LOG].start + mem[MB_LOG].size;
  uint8_t tbuffer[1024];
  cellLog_s *c;
  uint16_t cnt = 0;
  
  // по странично читаем
  for (adr = mem[MB_LOG].start; adr < endAdr; adr += 1024){
    mem_read(adr, tbuffer, 1024);
    // перебираем ячейки
    for (uint16_t a = 0; a < 1024; a += sizeof(cellLog_s)){
      c = (cellLog_s*)&tbuffer[a];
      // пустая ячейка
      if (c->index == 0xffffffff){
        device.newAddressLog = adr + a;
        return cnt;
      }
      
      cnt++;
    }
  }
  return cnt;
}

// добавить системное событие
bool log_addSystemEvent(logSystem_e num){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LT_SYSTEM;
  //cell.logNum = (uint16_t)num;
  
  return log_addEvent(&cell);
}

// добавить системное событие
bool log_addSystemEventID(logSystem_e num, char *id){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LT_SYSTEM;
  //cell.logNum = (uint16_t)num;
  
  if (id != NULL)
    memcpy(cell.ID, id, 10);
    
  return log_addEvent(&cell);
}

// добавить событие команда
bool log_addComandEvent(logComand_e num, uint8_t cnt){
  cellLog_s cell;

  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LT_COMAND;
  //cell.logNum = (uint16_t)num;
  cell.passCnt = cnt;
  
  return log_addEvent(&cell);  
}

// добавить событие обмена
bool log_addTransferEvent(logTransfer_e num){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LT_TRANSFER;
  //cell.logNum = (uint16_t)num;  
  
  return log_addEvent(&cell);  
}

// добавить событие доступа
bool log_addControlEvent(logControl_e num, char *id, uint32_t photo, uint8_t cnt){
  cellLog_s cell;
  
  memset((uint8_t*)&cell, 0, sizeof(cellLog_s));
  
  cell.index = device.datetime;
  cell.datetime = device.datetime;
  cell.logType = (uint8_t)LT_CONTROL;
  //cell.logNum = (uint16_t)num;
  cell.passCnt = cnt;
  
  if (id != NULL)
    memcpy(cell.ID, id, 10);
  
  cell.photoNum = photo;
  
  return log_addEvent(&cell);  
}


// ********************** Память фото **************

// получаем кол-во фото в памяти
uint16_t user_getPhotoCount(void){
  // 4 байта ID, 4 байта размер фото, N данные фото
  
  uint32_t adr;
  uint32_t endAdr = mem[MB_PHOTO].start + mem[MB_PHOTO].size;
  uint16_t cnt = 0;
  photoParam_s param;
  
  for (adr = mem[MB_PHOTO].start; adr < endAdr;){
    mem_read(adr, (uint8_t*)&param, sizeof(photoParam_s));
    // пустая ячейка
    if (param.datetime == 0xffffffff){
      device.newAddressPhoto = adr;
      return cnt;
    }
    adr += sizeof(photoParam_s) + param.size;
    cnt++;   
  }
  return cnt;
}


// ************************ Прошивка ***********************
bool user_checkFirmware (uint32_t adr, uint32_t size, uint32_t crc){

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
    mem_read (adr, bf, readed);
    
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