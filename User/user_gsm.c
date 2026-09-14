// Пользовательские функции, работа с gsm
// Алгоритм управления модулем gsm

#include "user_gsm.h"

// ************** Включить режим отладки **********
#define DEBUG_USERGSM

// таймауты в мс
#define RINGTIMEOUT             (uint16_t)5000
#define TIMETOGETREADY          (uint16_t)30000 // время на получение ответа о готовности модуля
#define TIMETOANSWER		(int16_t)3000 // время на получение ответа на команду
#define MAXRESETCOUNT           (int8_t)2 // кол-во перезагрузок модуля, после чего идет полная перезагрузка устройства
#define TIMETORESTARTGSM        (uint16_t)10000 // время между выключением и включением
#define TIMETOREGGSM            (uint16_t)30000 // время на регистрацию в сети
#define TIMETOREGGPRS           (uint16_t)30000 // время на регистрацию в gprs
#define TIMETOREREGGPRS         (uint16_t)10000 // время на перерегистрацию
#define MAXERRORS               (uint8_t)5 // макс кол-во ошибок
#define MAXNOTANSWERCOUNT       (uint8_t)3 // макс ков-во потери ответов

#pragma data_alignment=4
UserGsm_t gsm;

#define TIMETOSEND              (uint16_t)3000 // время на отправку данных
#define TIMETORECONNECT         (uint16_t)5000 // время на переподключение

#pragma data_alignment=4
Connection_t ucon[MAX_CONNECTIONS];
uint8_t qisackindex = 0;


////////////////////////////////////////////////////////////////////////////////
// Организация работы с gsm -------------------------------

// переопределено: обработка данных с модуля gsm -----------------------------------------------
void gsm_dataread (gsmresp_e gsmop, char* ptr1, char* ptr2, uint32_t v1, int32_t v2)
{
  // сбрасываем счетчики
  gsm.answer_timeout = 0;
  gsm.noanswercount = 0;

  switch (gsmop){
  case READY: // готов
     gsm.state_mashine = GSM_READY;
     gsm.timer_ms = 0;
     break;
   
  case IMEI: // имей
     strncpy(gsm.imei, ptr1, 15);
     gsm.imei[15] = '\0';
     gsm.imei_readed = 1;
     break;
   
  case CLIP: // номер звонящего
     gsm.clip = 1;
     user_strcpy(gsm.incomingNumber, ptr1, '\0', 19);
     if (gsm.param.IncomingCall != NULL){
       #pragma calls = user_incomingcall
       if (gsm.param.IncomingCall(gsm.incomingNumber, CT_PHONE)){
         gsm.clip = 0;
         gsm.incomingNumber[0] = 0;
       }
     }
#ifdef RINGTIMEOUT
     gsm.busySend = 1;
     gsm.ringTimeout = RINGTIMEOUT;     
#endif
     break;   
   
  case RING: // кто-то звонит
    gsm.ring = 1;
    if (gsm.clip){ // есть номер    
      if (gsm.param.IncomingCall != NULL)
      {
        #pragma calls = user_incomingcall
        if (gsm.param.IncomingCall(gsm.incomingNumber, CT_PHONE)){
          gsm.clip = 0;
          gsm.incomingNumber[0] = 0;
        }
      }
      gsm.ring = 0; 
    } else{
      gsm_runcmd("AT+CLIP=1\r", "OK");
    }
    
#ifdef RINGTIMEOUT
     gsm.busySend = 1;
     gsm.ringTimeout = RINGTIMEOUT;     
#endif
    break;   

  case SMS: // пришло СМС
    if (gsm.param.SMSReceived != NULL)
    {
      #pragma calls = user_smsreceived
      gsm.param.SMSReceived(ptr1, ptr2);
    }
    break; 

  case NOCARY: // нет ответа
  case BUSY:
  case NOANSWER:
    gsm.ring = 0;
    gsm.clip = 0;
    break;   

  case CREG: // рег gsm
    gsm.rouming = (v1 == 5)?1:0;

    if ((v1 == 1) || (v1 == 5))
    {
      if (!gsm.ready) // нет готовности
        gsm.state_mashine = GSM_READY;	
      
      if (gsm.gsmreg != 1)
        gsm_runcmd("AT+CLIP=1\r", "OK"); // надо повторять команду, т.к. на 3G не срабатывает при инициализации
      
      gsm.gsmreg = 1;
    }
    else
    {
      gsm.gsmreg = 0;
      gsm.gprsreg = 0;	
      
      if (gsm.state_mashine == GSM_WORK) // пропала в рабочем режиме
      {
        usergsm_event(E_NOGSM);
        
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Fail reg gsm\r\n");
#endif
        
        if (gsm.param.autoswitch_sim) // авто переключение СИМ
          usergsm_selectsim(0xff);
        else if (gsm.param.autorestart) // авто перезагрузка
          usergsm_restart();
      }
    }	   
    break;   

  case CGREG: // рег gprs
  case PDPDEACT:   
    if ((v1 == 1) || (v1 == 5)){
      gsm.gprsreg = 1;
    }
    else
    {
      gsm.gprsreg = 0;
      
      if (gsm.state_mashine == GSM_WORK)
      {
        usergsm_event(E_NOGPRS);  
        
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Fail reg gprs\r\n");
#endif        
        
        if (gsm.gsmreg){ // если есть регистрация в сети, то переподключим gprs 
          usergsm_rereggprs();
        } else{
          if (gsm.param.autoswitch_sim) // авто переключение СИМ 
            usergsm_selectsim(0xff);
          else if (gsm.param.autorestart) // авто перезагрузка 
            usergsm_restart();  
        }
      }
    }
    break;   

  case CONNECTED: // подключен к серверу
    connection_connected(v2);
    break;

  case FAILCONNECT: // не удалось подключиться
    connection_disconnected(v2);
    break;

  case CLOSECONNECT: // соединение закрыто
    connection_disconnected(v2);
    break;

  case DATA: // данные с сервера
    connection_received(v2, (pInt8U)ptr1, v1);
    break;

  case SMSSENDED: // смс отправлено
    if (gsm.param.SMSSended != NULL)
    {
      #pragma calls = user_smssended
      gsm.param.SMSSended();
    }
    break;

  case CSQ: // уровень сигнала
    gsm.csq = v1;
    gsm.ber = v2;
    break;

  case NOSIM: // нет сим карты
    usergsm_event(E_NOSIM);
    
#ifdef DEBUG_USERGSM
  usergsm_debug("*** SIM not inserted\r\n");
#endif
  
    if (gsm.param.autoswitch_sim) // авто переключение СИМ  
      usergsm_selectsim(0xff);
    else if (gsm.param.autorestart) // авто перезагрузка
      usergsm_restart();
    break;

  case GETPIN: // запроc пин кода
    break;

  case CUSD: // данные cusd
    user_parseUSSD(ptr1);
    break;

  case PREINIT: // после 5 секунд после включения
    /*gsm.timer_ms = 0;
    gsm.local_timer = 0;
    gsm.state_mashine = GSM_READIMEI;*/
    /*uart_sendstr(GSM_PORT, "AT\r\nAT+CPIN?\r\nAT+GMM\r\n");
    gsm_getregstate();*/
    break;

  case ATERROR: // ошибка при приеме AT команды
    /*if (++gsm.errorcount >= MAXERRORS)
    {
    gsm.errorcount = 0;
    usergsm_rereggprs();
    }*/ 
    break;

  case ATOK: // ответ модуля ОК
    break;
     
  case PWRDOWN: // модуль отключился
#ifdef DEBUG_USERGSM
  usergsm_debug("*** GSM is power down\r\n");
#endif
    /*gsm.timer_ms = 0;
    gsm.restart = 1;
    gsm.state_mashine = GSM_RESTART; // включаем повторно
    usergsm_clearconnections();*/
    usergsm_restart();
    break;
   
  case OPNAME: // наименование оператора СИМ карты
    user_strcpy(gsm.opname, ptr1, '\0', 15);
    gsm.opname_readed = 1;
    break;

  case QPING:
    if (atoi(ptr1) != 0)
      usergsm_event(E_PINGTIMEOUT);
    break;
     
  case RTCCLOCK: // 18/04/06,12:39:29+24
    {       
    uint16_t y;
    uint8_t mm, d, h, m, s;
    int32_t z;

    y = atoi(ptr1)+2000;
    if (y > 2100) y = atoi(ptr1);
    ptr1 = skipchar('/',ptr1,1);
    mm = atoi(ptr1);
    ptr1 = skipchar('/',ptr1,1);
    d = atoi(ptr1);
    ptr1 = skipchar(',',ptr1,1);
    h = atoi(ptr1);
    ptr1 = skipchar(':',ptr1,1);
    m = atoi(ptr1);
    ptr1 = skipchar(':',ptr1,1);
    s = atoi(ptr1);
    z = atoi(ptr1+2)*900; // поправка на часовой пояс в секундах

    gsm.datetime = user_timeToInt(y, mm, d, h, m, s) + z;
    }
    break;

  case QISACK: // инфо о том сколько данных передано
    connection_dataAcked(v1);
    break;
     
  case NTPRESP: // ответ на команду настройки NTP сервера
    if (v1 == 0)
      gsm.ntpconnected = 1;
    else 
      gsm.ntpconnected = 0;
    break;

  case URC_NETTYPE: // тип сети (строка)

    if (strcmp(ptr1, "UNKNOWN") == 0){
      gsm.net_type = 0;
      break;
    }
    if (strcmp(ptr1, "GSM") == 0){
      gsm.net_type = 1;
      break;
    }
    if (strcmp(ptr1, "EGPRS") == 0){
      gsm.net_type = 1;
      break;
    }
    if (strcmp(ptr1, "WCDMA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "HSDPA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "HSUPA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "HSDPA&HSUPA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "LTE") == 0){
      gsm.net_type = 4;
      break;
    }
    if (strcmp(ptr1, "TD-SCDMA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "CDMA") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "HDR") == 0){
      gsm.net_type = 3;
      break;
    }
    if (strcmp(ptr1, "EVDO") == 0){
      gsm.net_type = 3;
      break;
    }
    break;
     
  case SETMODULETYPE:
    gsm.param.moduleType = (moduleType_e)v1;
    usergsm_event(E_MODULETYPE);
    break;    
    
  default:
    break;
  }
}

// переключить/установить СИМ карту ------------
__weak void usergsm_setsim (uint8_t sim){}

// обработчик событий
__weak void usergsm_event(gsmevent_e ev){}

// сброс оперативных переменных
static void usergsm_clearStructBits(void){
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Clear struct bits\r\n");
#endif  
  gsm.restart = 0;
  gsm.ready = 0;  
  gsm.simindex = 0;
  gsm.imei_readed = 0;
  gsm.timer_ms = 0;
  gsm.local_timer = 0;
  gsm.answer_timeout = 0;
  gsm.incomingNumber[0] = 0;
  gsm.opname_readed = 0;
  gsm.errorcount = 0;  
  gsm.noanswercount = 0;
  gsm.ringTimeout = 0;
  gsm.busySend = 0;
  gsm.connectCnt = 0;
  
  usergsm_clearregvars();  
}

// инициализация параметров ----------------------------------------------------
void usergsm_init (GsmParam_t *param)
{  
  memset((uint8_t*)&gsm, 0, sizeof(UserGsm_t));
  gsm.param = *param;
  gsm_initvar(MAX_CONNECTIONS, param->textMode); // текст режим
  
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Init user gsm\r\n");
#endif
}

// алгоритм работы с модулем gsm -----------------------------------------------
// передается интервал
void usergsm_work (uint16_t ms)
{
  if (!gsm.param.enable) return;  

  gsm_pwrProc(ms); // обработчик включения/выключения модуля
  connection_timer(ms); // обработчик подключений

  switch (gsm.state_mashine){
  // питание подано  
  case GSM_POWERON: 
    if (gsm.timer_ms > TIMETOGETREADY) // прошло время, не получили ответ о готовности
    {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Timeout power ON\r\n");
#endif      
      if (gsm.param.autoswitch_sim != 0) // переключение СИМ разрешено
        usergsm_selectsim(0xff);
      else if (gsm.param.autorestart) // авто перезагрузка 
        usergsm_restart();
    } 
    else if (gsm.timer_ms > 5000) // через 5 сек после включения запрашиваем состояние регистрации, вдруг Call Ready не придет
    {
      if (gsm_getPowerState())
      {
        if (gsm.local_timer > 1000) // раз в 1 сек
        {
          gsm.local_timer = 0;
          gsm.answer_timeout = TIMETOANSWER; // время на получение ответа
          
          gsm_getregstate(); // состояние сети
        }      
      }
    }
    break;
  
  // выключение
  case GSM_POWEROFF: 
    if (gsm_getPowerState()) break; // ждем завершения выполнения процесса
    
    // задана перезагрузка
    if (gsm.restart != 0){
      // исчерпали попытки
      if (++gsm.resetCnt == MAXRESETCOUNT){
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Reset cpu\r\n");
#endif
        user_savereset();
        break;
      }
      
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Restart gsm\r\n");
#endif
  
      gsm.state_mashine = GSM_RESTART;
      gsm.timer_ms = 0;
    } else{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Goto stop cicle\r\n");
#endif
      gsm.state_mashine = GSM_STOPED;
    }
    break;
    
  // перезапуск модуля
  case GSM_RESTART: 
    if (gsm.timer_ms > TIMETORESTARTGSM){
      gsm.timer_ms = 0;
      usergsm_power(ON);
    }
    break; 
   
  case GSM_READY: // готов
#ifdef DEBUG_USERGSM
  usergsm_debug("*** is Ready\r\n");
#endif
    gsm.resetCnt = 0;
    gsm.ready = 1;
    gsm.state_mashine = GSM_READIMEI;
    gsm.timer_ms = 0;
    gsm.local_timer = 0;
    
    gsm_init();
    gsm_getimei();
    usergsm_event(E_READY);     
    break;

  case GSM_READIMEI: // запрос imei модуля
    if (gsm.imei_readed || (gsm.timer_ms > 5000))
    {
      gsm.state_mashine = GSM_REGTOGSM;
      gsm.timer_ms = 0;
      gsm.local_timer = 0;
      usergsm_event(E_IMEI);
    }
    else if (gsm.local_timer > 1000)
    {
      gsm.local_timer = 0;
      gsm_getimei();
    }
    break;    
    
  case GSM_REGTOGSM: // регистрируемся в gsm
    // есть регистрация
    if (gsm.gsmreg)
    {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Registred in GSM\r\n");
#endif
      usergsm_event(E_GSMREG);
      gsm_getcsq(); // уровень сигнала
      
      if (gsm.param.gprs_enable) // gprs используется
      {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** GPSR enabled. Switch ON\r\n");
#endif
        gsm_setgprs(ON);	// подключаем gprs 
        gsm.state_mashine = GSM_REGTOGPRS;
        gsm.timer_ms = 0;
      }
      else{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Goto work cicle\r\n");
#endif
        gsm.state_mashine = GSM_WORK;
        gsm.timer_ms = 0;
      }
    }
    else
    {
      // нет gsm
      if (gsm.timer_ms > TIMETOREGGSM) 
      {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Timeout reg in GSM\r\n");
#endif        
        usergsm_event(E_NOGSM);
                
        if (gsm.param.autoswitch_sim) // авто переключение СИМ карт
          usergsm_selectsim(0xff);
        else if (gsm.param.autorestart) // авто перезагрузка
          usergsm_restart();
      }
      else if (gsm.local_timer > 1000)
      {      
        gsm.local_timer = 0;
        gsm_getregstate(); // запрос статуса сети
      }
    }
    break;

  case GSM_REGTOGPRS: // регистрируемся в gprs
    // если есть gprs
    if (gsm.gprsreg) 
    {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Registred in GPRS\r\n");
  usergsm_debug("*** Goto work cicle\r\n");
#endif            
      gsm.internet = 1;
      gsm.state_mashine = GSM_WORK;
      gsm.timer_ms = 0;
      usergsm_event(E_GPRSREG);
    }
    else
    {
      // нет gprs
      if (gsm.timer_ms > TIMETOREGGPRS)
      {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Timeout reg in GPRS\r\n");
#endif        
        usergsm_event(E_NOGPRS);
        
        if (gsm.param.autoswitch_sim) // авто переключение СИМ карт
          usergsm_selectsim(0xff);
        else if (gsm.param.autorestart) // авто перезагрузка
          usergsm_restart();
      }
      else if (gsm.local_timer > 1000)
      {       
        gsm.local_timer = 0;
        gsm_getregstate();       
      }
    }
    break;

  case GSM_WORK: // рабочий режим
    if (gsm.local_timer == 1000)
    {     
      gsm.answer_timeout = TIMETOANSWER; // время на получение ответа
      gsm_getregstate(); // состояние сети
      gsm_getcsq(); // запрос уровня сигнала
    }
    
    if (gsm.local_timer > 10000){ // 10 сек
      gsm.local_timer = 0;
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Work cicle\r\n");
#endif 
    }
    break;
   
  case GSM_SOFTSIMSWITCH: // программное переключение СИМ
    switch (gsm.timer_ms)
    {
    case 1000: // через 1 сек после CFUN
      gsm_selectSIM(gsm.simindex);
      break;
      
    case 1500: // восстанавливаем режим CFUN
      gsm_setFUN(1);
      gsm.state_mashine = GSM_POWERON;
      gsm.timer_ms = 0;
      break;
    }
    break;

  case GSM_REREGGPRS: // перерегистрация gprs
    if (gsm.timer_ms > TIMETOREREGGPRS)
    {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Rereg GPRS\r\n");
#endif
      gsm.state_mashine = GSM_REGTOGSM;
      gsm.timer_ms = 0;
      if (++gsm.errorcount >= MAXERRORS){
        gsm.errorcount = 0;
        usergsm_restart();
      }
    }
    break;
  }

  // счет времени
  gsm.timer_ms += ms;
  gsm.local_timer += ms;

  // обработка таймаута ответа
  if (gsm.answer_timeout > 0)
  {
    gsm.answer_timeout -= ms;
    if (gsm.answer_timeout <= 0) // нет ответа от модуля gsm
    {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Not answer from module\r\n");
#endif
      // максимальное кол-во не ответов
      if (++gsm.noanswercount == MAXNOTANSWERCOUNT)
      {
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Max answer error\r\n");
#endif
        gsm.noanswercount = 0;
        usergsm_event(E_NOTANSWER);
        usergsm_restart();
      }
    }
  }
  
  // обработчик таймаута ring
  if (gsm.ringTimeout > 0)
  {
    gsm.ringTimeout -= ms;
    if (gsm.ringTimeout <= 0){
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Ring timeout\r\n");
#endif      
      gsm.busySend = 0;}
  }
}

// переключить сим карту -------------------
// sim = 255 - следующая
// true - СИМ карта переключена
bool usergsm_selectsim (uint8_t sim)
{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Select SIM\r\n");
#endif  
  if (gsm.param.soft_simswitch) // переключение через АТ команду
  {
    if (sim == 0xff) 
      gsm.simindex = !gsm.simindex; // следующую
    else 
      gsm.simindex = sim & 0x01; // выбранную
    
    gsm_setFUN(0); // режим ограниченной функциональности
    usergsm_clearregvars();
    gsm.state_mashine = GSM_SOFTSIMSWITCH;
    gsm.timer_ms = 0;
  } else{
    connection_deleteAll();
    usergsm_setsim(sim); 
    usergsm_restart();
  }
  
  return true;
}

// перезапустить модуль gsm ---------------------
void usergsm_restart (void)
{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Restart function\r\n");
#endif
  usergsm_power(OFF);
  gsm.restart = 1;
  gsm.timer_ms = 0;
}

// управление питанием gsm модуля ------------------
bool usergsm_power (Int8U p)
{
  if (p != 0){
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Power ON\r\n");
#endif
    gsm_initvar(MAX_CONNECTIONS, gsm.param.textMode);
    gsm_setModuleType(gsm.param.moduleType);
    usergsm_clearStructBits();
    
    usergsm_event(E_PWRON);
    gsm.state_mashine = GSM_POWERON;
  } else{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Power OFF\r\n");
#endif
    connection_deleteAll();
    gsm_setgprs(OFF);
    usergsm_clearStructBits();
    
    usergsm_event(E_PWROFF);
    gsm.state_mashine = GSM_POWEROFF;
  }
  gsm_power(p);
  
  return true;
}

// перерегистрация в gprs
void usergsm_rereggprs (void)
{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Rereg function\r\n");
#endif
  connection_deleteAll();
  usergsm_clearregvars();
  gsm_setgprs(OFF);
  gsm.state_mashine = GSM_REREGGPRS;
  gsm.timer_ms = 0;
}

// запрос уровня сигнала
uint8_t usergsm_getcsq (void)
{
  return gsm.csq;
}

// запрос уровня ошибок
uint8_t usergsm_getber (void)
{
  return gsm.ber;
}

// запрос imei
char* usergsm_getimei (void)
{
  return gsm.imei;
}

// текущая СИМ карта
uint8_t usergsm_currentsim (void)
{
  return (uint8_t)gsm.simindex;
}

// есть интернет или нет
uint8_t usergsm_getinternetstate (void)
{
  return (uint8_t)gsm.internet;
}

// состояние подключения NTP
uint8_t usergsm_getntpstate (void)
{
  return (uint8_t)gsm.ntpconnected;
}

// текущее состояние
uint8_t usergsm_getStateMachine(void){
  return (uint8_t)gsm.state_mashine;
}

// сброс переменных о состоянии регистрации
void usergsm_clearregvars (void)
{
#ifdef DEBUG_USERGSM
  usergsm_debug("*** Clear reg vars\r\n");
#endif
  gsm.gsmreg = 0;
  gsm.gprsreg = 0;
  gsm.rouming = 0;
  gsm.internet = 0;
  gsm.csq = 0;
  gsm.ber = 0;
  gsm.ring = 0;  
  gsm.clip = 0;
  gsm.ntpconnected = 0;
  gsm.net_type = 0;
}

// тип сети
uint8_t usergsm_getNetType(void){
  return gsm.net_type;
}

// запросить время сети
void usergsm_getClock(void){
  if (gsm.ring) return;
  gsm_getclock();
}

// есть входящий вызов или нет
bool usergsm_ring(void){
  return (bool)gsm.ring;
}

// есть входящий вызов или нет
void usergsm_endCall(void){
  gsm_endcall();
  gsm.ring = 0;
  gsm.clip = 0;
}

// передача занята
bool usergsm_busySend(void){
  return (bool)gsm.busySend;
}

__weak void usergsm_debug(char* ptr)
{
  usb_sendstr(ptr);
}

////////////////////////////////////////////////////////////////////////////////
// Организация подключений -------------------------------

// проверка индекса
static bool connection_validparam (int8_t h){
  if ((h < 0) || (h >= MAX_CONNECTIONS)) return false;  
  return true;
}

// сброс переменных
static void connection_clear (Connection_t* p){
  p->constate = 0;
  p->busy = 0;
  p->sended = 1;
  p->sending = 0;
  p->timer = 0;  
  p->sendtimeout = 0;
  p->isOpen = 0;
  p->isClose = 0;
}

// подключение установлено
static void connection_connected (int8_t h)
{
  if (!connection_validparam(h)) return;
  
  connection_clear(&ucon[h]); // чистим переменные
  ucon[h].constate = 1;
  ucon[h].count = 0;
  
  if (ucon[h].param.Connected != NULL)
  {
    #pragma calls = user_connected
    ucon[h].param.Connected(h); // если указан обработчик
  }
}

// подключение разорвано
static void connection_disconnected (int8_t h)
{
  if (!connection_validparam(h)) return;

  if (ucon[h].isClose == 0){ // если соединение закрыто не контроллером

    if (ucon[h].param.Disconnected != NULL)
    {
      #pragma calls = user_disconnected 
      ucon[h].param.Disconnected(h, ucon[h].param.auto_reconnect); // если указан обработчик
    }
    
    // автоматическое переподключение
    if (ucon[h].param.auto_reconnect)
    {
      connection_close(h);
      ucon[h].timer = TIMETORECONNECT; // через x сек переоткроем
      ucon[h].busy = 1;
    } else{
      connection_clear(&ucon[h]); // чистим переменные
    }    
  }
  
  ucon[h].isClose = 0;
}

// данные приняты
static void connection_received (int8_t h, uint8_t* data, uint16_t siz)
{
  if (!connection_validparam(h)) return;
  
  if (ucon[h].param.DataReceived != NULL)
  {
    #pragma calls = user_dataconnection
    ucon[h].param.DataReceived(h, data, siz); // если указан обработчик
  }
}

// отправленные данные подтверждены
static void connection_dataAcked (uint32_t cnt)
{
  ucon[qisackindex].qisackWait = 0;  
  
  if (cnt == 0){
    ucon[qisackindex].sendtimeout = 0;
    ucon[qisackindex].sended = 1;
    ucon[qisackindex].sending = 0;
  }
}

// инициализация параметров подключения
void connection_init (Connection_t* p)
{
  connection_clear(p);
  
  p->handle = -1;
  p->count = 0;
  p->param.ip = NULL;
  p->param.port = 0;
  p->param.timeout = 0;
  p->param.auto_reconnect = 0;
  p->param.Connected = NULL;
  p->param.Disconnected = NULL;
  p->param.Error = NULL;
  p->param.DataReceived = NULL;
}

// создать подключение
int8_t connection_create (ConnectParam_t* p)
{
  for (uint8_t i=0; i<MAX_CONNECTIONS; i++){
    if (ucon[i].param.ip == NULL) // ячейка свободна
    {
      ucon[i].param = *p; // копируем параметры
      connection_clear(&ucon[i]);
      ucon[i].count = 0;
      return i;
    }    
  }
  return -1;
}

// удалить подключение
int8_t connection_delete (int8_t h)
{
  if (!connection_validparam(h)) return CE_PARAM;  

  gsm_disconnect(h); // закрываем подключение
  connection_init(&ucon[h]); // сбрасываем соединение
  
  return CE_OK;
}

// открыть подключение
int8_t connection_open (int8_t h)
{
  if (!connection_validparam(h)) return CE_PARAM;
  if (ucon[h].constate) return CE_CONNECTED;
  if (ucon[h].busy) return CE_BUSY;
  if (ucon[h].param.ip == NULL) return CE_IP;
  if (!gsm.internet) return CE_NOINTERNET;
  
  ucon[h].timer = ucon[h].param.timeout; // время на подключение
  ucon[h].busy = 1;
  ucon[h].isOpen = 1;
  // открываем подключение
  if (ucon[h].param.port != 0){
    gsm_connect(ucon[h].param.ip, ucon[h].param.port, h);
  } else{
    char ip[32];
    uint16_t port;
    
    user_strcpy(ip, ucon[h].param.ip, ':', 31);
    port = atoi(skipchar(':', ucon[h].param.ip, 1));
    
    gsm_connect(ip, port, h);
  }
  
  return CE_OK;
}

// закрыть подключение
int8_t connection_close (int8_t h)
{
  if (!connection_validparam(h)) return CE_PARAM;
  
  connection_clear(&ucon[h]);
  ucon[h].isClose = 1;
  // закрываем подключение
  gsm_disconnect(h);
  
  return CE_OK;
}

// переподключить соединение
int8_t connection_reopen (int8_t h)
{
  if (!connection_validparam(h)) return CE_PARAM;
  
  connection_close(h); // закрываем
  ucon[h].timer = TIMETORECONNECT; // через x сек переоткроем
  ucon[h].busy = 1;

  return CE_OK;
}

// состояние подключения
int8_t connection_state (int8_t h)
{
  if (!connection_validparam(h)) return CE_PARAM;
  if (ucon[h].constate) return CE_CONNECTED;
  
  return CE_NOTCONNECTED;
}

// отправить данные
int8_t connection_send (int8_t h, uint8_t* data, uint16_t siz)
{
  if (!connection_validparam(h)) return CE_PARAM;
  if (!ucon[h].constate) return CE_NOTCONNECTED;
  if (ucon[h].param.wait_sendData){
    if (ucon[h].sended == 0) return CE_BUSY;
  }

  ucon[h].sended = 0;
  ucon[h].sendtimeout = TIMETOSEND;
  ucon[h].sending = 1;
  
  // отправляем
  gsm_senddata (data, siz, h);
  
  return CE_OK;
}

// отправлены данные или нет
bool connection_datasended (int8_t h)
{
  if (!connection_validparam(h)) return false;
  
  return (bool)ucon[h].sended;
}

// обаботчик процесса подключений
void connection_timer (uint8_t ms)
{
  for (uint8_t i=0; i<MAX_CONNECTIONS; i++)
  {
    if (usergsm_getinternetstate()) // при наличии интернета
    {
      if (ucon[i].timer > 0) // таймер запущен
      {
        ucon[i].timer -= ms;
        if (ucon[i].timer > 0) continue;
        
        ucon[i].busy = 0;
        
        if (ucon[i].param.auto_reconnect)
        {
          if (ucon[i].isOpen){ // новое открываемое
            if (ucon[i].param.maxConnectCount != 0){
              if (++ucon[i].count == ucon[i].param.maxConnectCount){
                if (ucon[i].param.Error != NULL)
                {
                  #pragma calls = user_errorconnection
                  ucon[i].param.Error(i); // если указан обработчик
                }
              }              
            }
            connection_close(i);
            ucon[i].isOpen = 0;
            ucon[i].timer = TIMETORECONNECT;
            ucon[i].busy = 1;              
          } else{
            connection_open(i);
          }
        }
      }      
    }
    
    // время на отправку данных
    if (ucon[i].sendtimeout > 0)
    {
      ucon[i].sendtimeout -= ms;
      if (ucon[i].sendtimeout <= 0)
      { 
        ucon[i].sended = 1;
        ucon[i].sendtimeout = 0;
        ucon[i].sending = 0;
        ucon[i].qisackWait = 0;
      }
    }   
  }

  if (ucon[qisackindex].qisackWait == 0){ // нет ожидания ответа QISACK
    // следующее подключение
    for (uint8_t i = 0; i < MAX_CONNECTIONS; i++){
      if (++qisackindex >= MAX_CONNECTIONS) 
        qisackindex = 0;
      if (ucon[qisackindex].sending){
        ucon[qisackindex].qisackWait = 1;
        gsm_getinformationsending(qisackindex);      
      }
    }
  }
}

// нет передаваемых данных по соединениям
bool connection_allDataSended (void)
{
  for (uint8_t i=0; i<MAX_CONNECTIONS; i++)
    if (ucon[i].sending) return false;
  
  return true;
}

// удалить все подключения
void connection_deleteAll(void){
  for (uint8_t i=0; i<MAX_CONNECTIONS; i++)
    connection_delete(i);
}
////////////////////////////////////////////////////////////////////////////////
