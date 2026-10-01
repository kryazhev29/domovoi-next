// Доработка: Кряжев А., 2026
// Модуль работы с gsm модулем
// Автор: Корякин Сергей

#include "gsm.h"

// ************** Включить режим отладки **********
#define DEBUG_GSM

#define GSMRESPTIMEOUT                  (uint16_t)500 //время ожидания ответа на команду
#define GSMRESPTIMEOUTLONG              (uint16_t)3000 //время ожидания ответа на команду

#define gsm_send(a,b)                   uart_send(GSM_PORT,a,b)
#define gsm_sendStrAndWaitResp(a,b,c,d) uart_sendStrAndWaitResp(GSM_PORT, a, b, sizeof(gsm_strbuf), d, c)

#ifdef DEBUG_GSM
  #define gsm_sendstr(a)                  uart_sendstr(GSM_PORT,a); gsm_user_debug(">>> "); gsm_user_debug(a)
  #define gsm_debug(a)                    gsm_user_debug(a)
#else
  #define gsm_sendstr(a)                  uart_sendstr(GSM_PORT,a)
#endif

#pragma data_alignment=4
char gsm_strbuf[128]; // временный строковый буфер

#pragma data_alignment=4
GsmStr GSM;


// Инициализация переменных ----------------------------------------------------
static void gsm_rxreset(void);
static void gsm_rxfailed(void);
static bool gsm_rxpending(void);

void gsm_initvar (uint8_t multiConnection, uint8_t textMode)
{
  moduleType_e mt = GSM.moduleType;
  
  gsm_rxreset();
  memset((uint8_t*)&GSM, 0, sizeof(GSM));
  
  if (multiConnection != 0){
    GSM.muxmode = 1;
  }
  
  GSM.textMode = textMode;
  GSM.moduleType = mt;
#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Init variable\r\n"); 
#endif
}

// Процесс ---------------------------------------------------------------------
void gsm_pwrProc (Int8U ms)
{
  switch (GSM.operation){
  case PO_NONE:
    break;
  case PO_ON:
    switch (GSM.timer){
    case 100: // включить питание
      GSM_PWR_ON;
      GSM_KEY_OFF;
      uart_on(GSM_PORT);
      break;
  
    case 1000: // через 1 сек
      GSM_KEY_ON;
      break;
      
    case 3000: // через 2 сек
      GSM_KEY_OFF;
      break;
      
    case 5000: // через 2 сек
      gsm_runcmd("AT+GMM\r", "OK"); // модель
      gsm_dataread(PREINIT, NULL, NULL, 0, 0);
      break;
    
    case 6000: // модуль готов
      GSM.operation = PO_NONE;
      GSM.pwr = 1;
#ifdef DEBUG_GSM
      gsm_debug("<<< GSM: is ON\r\n"); 
#endif
      break;
    }
    break;
    
  case PO_OFF:
    switch (GSM.timer){
    case 100: // выключить питание
      if (GSM.moduleType == M_UG95){
        gsm_runcmd("AT+CPOF\r", "OK");
        GSM.timer = 9000;
      } else{
        GSM_KEY_ON;
      }
      break;
     
    case 3500:
      GSM_KEY_OFF;
      break; 
     
    case 4000:
    case 10000: 
      uart_off(GSM_PORT);
      GSM_PWR_OFF;
      GSM.operation = PO_NONE;
      GSM.pwr = 0;
#ifdef DEBUG_GSM
      gsm_debug("<<< GSM: is OFF\r\n"); 
#endif
      break;
    }
    break;
    
  case PO_ONDELAY:
    if (GSM.timer > 5000){
      GSM.timer = 0;
      GSM.operation = PO_ON;
    }
    break;
  }
  GSM.timer += ms;
}

// Управление питанием модуля --------------------------------------------------
// 0 - выключить, 1 - включить
bool gsm_power (Int8U pwr)
{
  if (GSM.operation != PO_NONE) 
    return false; 
  
  gsm_initvar(GSM.muxmode, GSM.textMode);

  if (pwr != 0){
    GSM.pwr = 0;
    GSM.operation = PO_ON;
#ifdef DEBUG_GSM
    gsm_debug(">>> GSM: Power ON\r\n"); 
#endif
  } else{
    GSM.pwr = 1;
    GSM.operation = PO_OFF;
#ifdef DEBUG_GSM
    gsm_debug(">>> GSM: Power OFF\r\n");
#endif    
  }
  GSM.timer = 0;
  return true;
}

// URC, пришедшие в ответ на команду -------------------------------------------
/* >>> URC из ответа на команду */
/* Модем шлёт URC (данные от сервера, SMS, разрыв связи) в любой момент, в том числе пока идёт
 * обмен AT-командой. uart_sendStrAndWaitResp отдаёт такой URC как ответ на команду, а uart_clear
 * в начале следующей команды стирает его из буфера: цикл main до него не доходит и кадр пропадает.
 * Поэтому перед отправкой команды URC откладывается сюда, а цикл main забирает его через
 * gsm_takeurc. Разбирать URC на месте нельзя: gsm_read зовёт gsm_dataread, а тот из обработчика
 * приложения может снова вызвать gsm_runcmd и затереть gsm_strbuf. */
#define GSM_URC_MAX                     (uint16_t)2048 // предел отложенного, дальше кадр теряется

static char *gsm_urcbuf = NULL; // отложенные URC, ждут цикл main
static uint16_t gsm_urclen = 0;
static bool gsm_urclost = false;

// Поиск подстроки в двоичных данных: нули внутри кадра не останавливают поиск
static char* gsm_memfind (char *ptr, uint16_t size, const char *pattern)
{
  uint16_t l = (uint16_t)strlen(pattern);
  uint16_t i;

  if ((ptr == NULL) || (l == 0) || (size < l)) return NULL;

  for (i = 0; i <= (uint16_t)(size - l); i++){
    if (memcmp(&ptr[i], pattern, l) == 0)
      return &ptr[i];
  }
  return NULL;
}

// Первое в кадре сообщение модема, которое не может быть ответом на команду
static char* gsm_findurc (char *ptr, uint16_t size)
{
  static const char *marks[] = {"+QIURC:", "+CMT:", "+CIEV:", "+CLIP:", "+RECEIVE:", "+IPD", "PDP: DEACT"};
  char *first = NULL;
  char *p;
  uint8_t i;

  for (i = 0; i < (uint8_t)(sizeof(marks) / sizeof(marks[0])); i++){
    p = gsm_memfind(ptr, size, marks[i]);
    if ((p != NULL) && ((first == NULL) || (p < first)))
      first = p;
  }
  if (first == NULL){
    // UART мог закончиться посередине первого заголовка.
    char *q = ptr;
    while (q < ptr + size && (*q == '\r' || *q == '\n')) q++;
    uint16_t left = (uint16_t)(ptr + size - q);
    for (i = 0; i < (uint8_t)(sizeof(marks) / sizeof(marks[0])); i++){
      uint16_t n = (uint16_t)strlen(marks[i]);
      if (left && left < n && memcmp(q, marks[i], left) == 0) return q;
    }
  }
  return first;
}

// Отложить URC из буфера приёма, пока его не стёрла очередная команда
static void gsm_stashurc (void)
{
  uint16_t count = uart_received(GSM_PORT);
  if (count == 0) return;
  char *raw = (char*)uart_getptr(GSM_PORT);
  char *urc = (gsm_urclen != 0 || gsm_rxpending()) ? raw : gsm_findurc(raw, count);
  if (urc == NULL){
    uart_takerx(GSM_PORT, NULL, count); // только прежний ответ, не новый хвост
    return;
  }
  uint16_t offset = (uint16_t)(urc - raw);
  if (count > (uint16_t)(GSM_URC_MAX - gsm_urclen)){
    gsm_urclost = true;
    uart_takerx(GSM_PORT, NULL, count);
    return;
  }
  if (gsm_urcbuf == NULL) gsm_urcbuf = calloc(GSM_URC_MAX + 8u, 1);
  if (gsm_urcbuf == NULL){
    gsm_urclost = true;
    uart_takerx(GSM_PORT, NULL, count);
    return;
  }
  count = uart_takerx(GSM_PORT, (uint8_t*)gsm_urcbuf + gsm_urclen, count);
  if (offset > count) offset = count;
  memmove(gsm_urcbuf + gsm_urclen, gsm_urcbuf + gsm_urclen + offset, count - offset);
  gsm_urclen += (uint16_t)(count - offset);
}

// Разобрать отложенные URC, вызывается из цикла main
void gsm_takeurc (void)
{
  char *buf = gsm_urcbuf;
  uint16_t len = gsm_urclen;
  char *compact;

  if (gsm_urclost){
    gsm_urclost = false;
    gsm_urcbuf = NULL;
    gsm_urclen = 0;
    free(buf);
    gsm_rxfailed(); // отказ явно закрывает соединение, не смешивает неполные кадры
    return;
  }
  if (buf == NULL) return;
  // Перед разбором отдаём неиспользованный хвост heap другим командам.
  compact = realloc(buf, (uint32_t)len + 8u);
  if (compact != NULL) buf = compact;

  gsm_urcbuf = NULL; // снимаем заранее: gsm_read может отложить следующий
  gsm_urclen = 0;

  gsm_read(buf, len);
  free(buf);
}
/* <<< URC из ответа на команду */

// Выполнить команду -----------------------------------------------------------
char* gsm_runcmd(char *str, char *resp)
{
  /* То, что уже лежит в буфере: uart_sendStrAndWaitResp начинает с uart_clear, и любой URC
   * пропал бы до того, как его увидит цикл main. */
  gsm_stashurc();

#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Cmd "); 
  gsm_debug(str);
  gsm_sendStrAndWaitResp(str, gsm_strbuf, resp, GSMRESPTIMEOUT);
  gsm_debug("<<< GSM: Answer ");
  gsm_debug(gsm_strbuf);
  gsm_debug("-----\r\n");
  
  return gsm_strbuf;  
#else
  gsm_sendStrAndWaitResp(str, NULL, resp, GSMRESPTIMEOUT);
  
  return NULL;
#endif
}

// Инициализация модуля --------------------------------------------------------
void gsm_init(void)
{
  // выходим, если инициализация была
  if (GSM.init){
    return;
  }
  
  //gsm_runcmd("AT+GMM\r", "OK"); // model type identification  
  gsm_runcmd("ATE0\r", "OK"); // Командное эхо
  gsm_runcmd("ATV1\r", "OK"); // текстовые ответы
  gsm_runcmd("AT+IPR=115200\r", "OK"); // Скорость передачи 115200
  gsm_runcmd("AT+CFUN=1\r", "OK"); // полная функциональность
  gsm_runcmd("AT+IFC=0,0\r", "OK"); // Контроль передачи данных отключен
  gsm_runcmd("AT+CLIP=1\r", "OK"); // Показывать номер звонящего
  
  if (GSM.textMode != 0){
    gsm_runcmd("AT+CMGF=1\r", "OK"); // TEXT режим (0 - PDU режим, 1 - TEXT)  
  } else{
    gsm_runcmd("AT+CMGF=0\r", "OK"); // TEXT режим (0 - PDU режим, 1 - TEXT)
  }
    
  gsm_runcmd("AT+CSCS=\"GSM\"\r", "OK"); // Алфавит: GSM, HEX, IRA, PCCP437, UCS2, 8859-1 
  
  if (GSM.moduleType == M_SLM336){
    gsm_runcmd("AT+URCMODE=1\r", "OK"); // СМС монитор включен
    gsm_runcmd("AT+CNMI=2,2\r", "OK"); // СМС монитор включен
  } else{
    gsm_runcmd("AT+CNMI=2,2,2,1,0\r", "OK"); // СМС монитор включен
  }
  
  if (GSM.moduleType != M_UG95){
    if (GSM.moduleType != M_SLM336){
      gsm_runcmd("AT+QMOSTAT=1\r", "OK");
      gsm_runcmd("AT+QIHEAD=1\r", "OK"); // Добавлять к входным данным +IPD
      gsm_runcmd("AT+QEXTUNSOL=\"SQ\",1\r", "OK"); // Уровень сигнала
      
      if (GSM.muxmode){
        gsm_runcmd("AT+QIMUX=1\r", "OK"); // Включить множественные подключения
      } else{
        gsm_runcmd("AT+QIMUX=0\r", "OK"); // ВЫключить множественные подключения 
      }
    }
  gsm_runcmd("AT+QSPN?\r", "OK"); // Оператор
  }

  if (GSM.moduleType == M_UG95){
    gsm_runcmd("AT+QINDCFG=\"act\",1\r", "OK"); // Включить вывод сообщений об типе сети +QIND: "act",...
  }
    
  if (GSM.moduleType != M_M66){
    gsm_runcmd("AT+QURCCFG=\"urcport\",\"uart1\"\r", "OK"); // Вывод URC сообщений в порт main
  }
  
  gsm_runcmd("AT+GMR\r", "OK"); // Версия прошивки
  gsm_runcmd("AT+CREG=1\r", "OK"); // Отображать информацию о регистрации
  gsm_runcmd("AT+CGREG=1\r", "OK"); // Отображать информацию о регистрации
  
  /* Модуль готов (+CPIN: READY / RDY / +SIM1 Ready или регистрация в сети, user_gsm.c).
     Quectel M65/M95 при автоподборе скорости первые строки не выдают - без флага не
     принимались 0, CONNECT OK и +IPD. Доработка: Кряжев А., 2026 */
  GSM.ready = 1;
  GSM.init = 1;
}

// Обработка данных c модуля gsm -----------------------------------------------
// Оба варианта регистрации: <stat> и <n>,<stat>; короткие строки не являются ответом.
static bool gsm_regstate(char *p, char *end, uint8_t *state)
{
  while (p < end && *p == ' ') p++;
  if (p >= end || *p < '0' || *p > '9') return false;
  uint8_t n = (uint8_t)(*p++ - '0');
  if (p < end && *p == ','){
    p++;
    while (p < end && *p == ' ') p++;
    if (p >= end || *p < '0' || *p > '9') return false;
    n = (uint8_t)(*p - '0');
  }
  *state = n;
  return true;
}

// Длина тела допустима только вместе с разделителем и всеми принятыми байтами.
static bool gsm_datalength(char *first, char *end, char separator, uint16_t *len, char **body)
{
  uint32_t n = 0;
  char *p = first;
  while (p < end && *p == ' ') p++;
  if (p >= end || *p < '0' || *p > '9') return false;
  while (p < end && *p >= '0' && *p <= '9'){
    n = n * 10u + (uint8_t)(*p++ - '0');
    if (n > 65535u) return false;
  }
  if (separator == '\n' && p < end && *p == '\r') p++;
  if (p >= end || *p++ != separator) return false;
  *len = (uint16_t)n;
  *body = p;
  return true;
}

static void gsm_readtext(char* ptr, Int16U size)
{
  if (ptr == NULL) return;
  if (size == 0) return;
  if (GSM.operation == PO_OFF) return;
  
  char *ps;
  char *var;
  uint16_t l;
  
#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Recv ");
  gsm_debug(ptr);
  gsm_debug("-----\r\n");  
#endif

  // <CR><LF>IMEI<CR><LF>
  if (GSM.getimei){
    var = ptr;
    while (var < ptr + size && (*var == '\r' || *var == '\n' || *var == ' ')) var++;
    if ((ptr + size - var >= 15) && chkdigitstr(var, 15)){
      strncpy(gsm_strbuf, var, 15);
      gsm_strbuf[15] = 0;
      GSM.getimei = 0;
      gsm_dataread(IMEI, gsm_strbuf, NULL, 0, 0);
    }
  }
  
  // +CLIP: "+7
  if ((ps = strstr(ptr, "+CLIP:")) != NULL){
    var = skipchar('"', ps, 1);
    user_strcpy(gsm_strbuf, var, '"', 16);
    l = strlen(gsm_strbuf);
    if ((l >= 10) && (l < 16)) // если скопировали номер, а не пустоту
      gsm_dataread (CLIP, gsm_strbuf, NULL, 0, 0);                    
  }
  
  // +CREG: 1,1
  if ((ps = strstr(ptr, "+CREG:")) != NULL){
    if (gsm_regstate(ps + 6, ptr + size, &GSM.creg))
      gsm_dataread (CREG, ps + 6, NULL, GSM.creg, 0);
  }

  // +CGREG: 1,0
  if ((ps = strstr(ptr, "+CGREG:")) != NULL){
    if (gsm_regstate(ps + 7, ptr + size, &GSM.cgreg))
      gsm_dataread (CGREG, ps + 7, NULL, GSM.cgreg, 0);
  } 
  
  // +CCLK: "18/04/06,12:39:29+24"
  if ((ps = strstr(ptr, "+CCLK:")) != NULL){  
    user_strcpy (gsm_strbuf, ps+8, '"', 32);
    if (strlen(gsm_strbuf) < 32)
      gsm_dataread (RTCCLOCK, gsm_strbuf, ps+8, 0, 0);         
  }
  
  // +CM
  if ((ps = strstr(ptr, "+CM")) != NULL){              
    // +CMT: <oa>,[<alpha>],<scts>[,<tooa>,<fo>,<pid>,<dcs>,<sca>,<tosca>,<length>]<CR><LF><data>
    // +CMT: [<alpha>],<length><CR><LF><pdu>
    // +CMT: "+7xxxxxxxxxx",,"2021/11/24 16:46:45+24"<CR><LF>0421043E043E043104490435043D0438043500200434043E0441044204300432043B0435043D043E002E<CR><LF>                                        
    if (strncmp("+CMT:", ps, 5) == 0){              
      var = skipchar('"', ps, 1);  
      user_strcpy (gsm_strbuf, var, '"', 16);
      //777 определять pdu или text
      do{
        var = skipchar(0x0a, var, 1);
      } while(*var == 0x0d);
      gsm_dataread (SMS, gsm_strbuf, var, 0, 0);
      return; /*текст смс после перевода каретки и до следующего перевода каретки*/      
    }
    
    // +CMGS
    if (strncmp("+CMGS:", ps, 6) == 0){              
      gsm_dataread (SMSSENDED, NULL, NULL, 0, 0);         
    }
    
    // +CME ERROR: 10
    if (strncmp("+CME ERROR:", ps, 11) == 0){               
      switch (atoi(ps+12)){
      case 10: // нет сим карты
      case 3517: // ошибка сим карты
        if (!GSM.nosim)
          gsm_dataread(NOSIM, NULL, NULL, 0, 0);
        GSM.nosim = 1;
        break;
      }        
    }

    // +CMGR: "REC READ","+7xxxxx",,"2025/01/09,19:45:20+12",145,4,0,0,"+7xxxxx",145,11<CR><LF><data><CR><LF>OK
    // +CMGR: 1,,29<CR><LF>07919761980644F2040B919780012948F70000521090915402210BA3F3BC3DA7836833590C<CR><LF>OK
    if (strncmp("+CMGR:", ps, 6) == 0){
      var = skipchar(',', ps, 1);
      var = skipchar('"', var, 1);
      user_strcpy (gsm_strbuf, var, '"', 16);
      var = skipchar(0x0a, var, 1);
      //777 определять pdu или text
      gsm_dataread (SMS, gsm_strbuf, var, 0, 0);
      return; /*текст смс после перевода каретки и до следующего перевода каретки*/          
    }
  }

  // +CS
  if ((ps = strstr(ptr, "+CS")) != NULL){
    // +CSQ: 29,55
    if (strncmp("+CSQ:", ps, 5) == 0){         
      uint8_t ber = 99;
      
      var = skipchar(',', ps, 1);
      if (var != ps)
        ber = atoi(var);
      gsm_dataread (CSQ, ps+6, NULL, atoi(ps+6), ber);           
    }
    
    // +CSQN: 24
    if (strncmp("+CSQN:", ps, 6) == 0){     
      uint8_t ber = 99;
      
      var = skipchar(',', ps, 1);
      if (var != ps)
        ber = atoi(var);  
      gsm_dataread (CSQ, ps+7, NULL, atoi(ps+7), ber);       
    }
  }

  // +CUSD: 2,"04110430043B0430043D0441003A003200320032002C0035003804400020",72
  /*777if ((ps = strstr(ptr, "+CUSD:")) != NULL){               
    char *v1, *v2;
    uint16_t count = 0, limit = (sizeof(gsm_strbuf)-8)*4;

    // выбираем строку шетнадцатеричного вида
    v1 = skipchar('"', ps, 1);
    v2 = v1;
    for (;limit != 0;){
      if (*v2 == '"') break;
      if (*v2 == 0) break;
      count++;
      limit--;
      v2++;
    }
    *v2 = 0;

    if (count != 0){
      if ((v1[0] == '0') && ((v1[1] == '0') || (v1[1] == '4'))){ // ucs2 code
        decodeUCS2toText(gsm_strbuf, v1, count);
        gsm_dataread(CUSD, gsm_strbuf, NULL, 0, 0); // UCS2
      }
      else 
        gsm_dataread(CUSD, v1, NULL, 0, 0); // Orig  
    }       
  }*/
  
  // +CIEV: "MESSAGE", 1
  if ((ps = strstr(ptr, "+CIEV:")) != NULL){ 
    var = skipchar('"', ps, 1);
    // "MESSAGE", 1
    if (strncmp("MESSAGE", var, 7) == 0){
      var = skipchar(',', ps, 1);
      if (var != ps)
        gsm_readsms(atoi(var));
    }
  }
  
  // +CPIN:
  if ((ps = strstr(ptr, "+CPIN:")) != NULL){                
    ps += 7;
    // READY
    if (strncmp("READY", ps, 5) == 0){
      if (GSM.ready == 0){
        GSM.ready = 1;
        gsm_dataread (READY, NULL, NULL, 0, 0);
      }
    }
    
    // SIM PIN
    if (strncmp("SIM PIN", ps, 7) == 0){
      gsm_dataread (GETPIN, NULL, NULL, 0, 0);
    }
    
    // NOT INS
    /*if (strncmp("NOT INS", ps, 7) == 0){
      if (!GSM.nosim) 
        gsm_dataread (NOSIM, NULL, NULL, 0, 0);
      GSM.nosim = 1;
    }*/        
  }
  
  if (GSM.moduleType != M_SLM336){
    // +RECEIVE: connectionindex, sizedata
    // команда появляется в M95 если QIMUX=1, вслед за ней идет IPD
    if ((ps = strstr(ptr, "+RECEIVE:")) != NULL){             
      if (GSM.ready && chkdigitchar(*(ps+10)))
        GSM.dataconnectionindex = atoi(ps+10);        
    }     

    // +IPD,5:abcde // SIM900
    // +IPD125:abcde // M95

  
    // PDP: DEACT
    if ((ps = strstr(ptr, "PDP: DEACT")) != NULL){             
      if (GSM.ready)
        gsm_dataread (PDPDEACT, NULL, NULL, 0, 0);   
    }
  }  
    
  // Ready
  if ((ps = strstr(ptr, "+SIM1 Ready")) != NULL){             
    if (GSM.ready == 0){
      GSM.ready = 1;  
      gsm_dataread (READY, NULL, NULL, 0, 0);
    }      
  }
  
  // +QSPN: "Tele2",0
  if ((ps = strstr(ptr, "+QSPN:")) != NULL){             
    user_strcpy (gsm_strbuf, ps+5, '"', 16);
    gsm_dataread (OPNAME, gsm_strbuf, NULL, 0, 0);         
  }
  
  // +QPING: 0[,ipAaddr>,bytes,time,ttl]
  if ((ps = strstr(ptr, "+QPING:")) != NULL){             
    gsm_dataread(QPING, ps+8, NULL, 0, 0);           
  }
  
  // +QI
  if ((ps = strstr(ptr, "+QI")) != NULL){
    // +QIOPEN: <connectID>,<err>
    if (strncmp("+QIOPEN:", ps, 8) == 0){
      if (GSM.muxmode && chkdigitchar(*(ps+9))) 
        GSM.dataconnectionindex = atoi(ps+9);
      
      var = skipchar(',', ps, 1);
      if (var != ps){
        if (atoi(var) == 0) 
          gsm_dataread (CONNECTED, NULL, NULL, 0, GSM.dataconnectionindex);
        else 
          gsm_dataread (FAILCONNECT, NULL, NULL, 0, GSM.dataconnectionindex);        
      }
    }
    
    // +QISACK: 100, 90, 10
    if (strncmp("+QISACK:", ps, 8) == 0){
      uint32_t len;
      
      var = skipchar(',', ps, 2);
      if (var != ps){
        if (*var == ' ')
          len = atoi(var+1);
        else
          len = atoi(var);
        
        gsm_dataread(QISACK, ps+9, NULL, len, 0);        
      }
    }
    
    // +QISEND: 100, 90, 10
    if (strncmp("+QISEND:", ps, 8) == 0){
      uint32_t len;
      
      var = skipchar(',', ps, 2);
      if (var != ps){
        if (*var == ' ')
          len = atoi(var+1);
        else
          len = atoi(var);
        
        gsm_dataread(QISACK, ps+9, NULL, len, 0);        
      }
    }

    // +QIURC:
    if (strncmp("+QIURC:", ps, 7) == 0){
      uint8_t cid;
      
      var = skipchar('"', ps, 1);
      user_strcpy(gsm_strbuf, var, '"', 16);

      var = skipchar(',', ps, 1);
      cid = atoi(var);
      
      if (GSM.muxmode) // множество подключений, 0, CLOSED
        GSM.dataconnectionindex = cid;

      // +QIURC: "closed",<connectID>
      if (strncmp("closed", gsm_strbuf, 6) == 0){
        gsm_disconnect(GSM.dataconnectionindex);  
        gsm_dataread (CLOSECONNECT, NULL, NULL, 0, GSM.dataconnectionindex);
      }
      
      // +QIURC: "recv",<connectID>,<currentrecvlength><CR><LF><data>


      // +QIURC: "pdpdeact",<contextID>
      if (strncmp("pdpdeact", gsm_strbuf, 8) == 0){
        if (GSM.moduleType == M_SLM336) 
          gsm_runcmd("AT+QIDEACT=1\r", "OK"); // Деактивация PDP контекста
        else
          gsm_runcmd("AT+QIDEACT\r", "OK"); // Деактивация PDP контекста
        gsm_dataread (PDPDEACT, NULL, NULL, 0, 0);  
      }
    }

    // +QIND: "act","GSM"
    if (strncmp("+QIND:", ps, 6) == 0){
      var = skipchar('"', ps, 1);
      if (strncmp("act", var, 3) == 0){
        var = skipchar('"', ps, 2);
        if (var != ps){
          user_strcpy(gsm_strbuf, var, '"', 15);
          gsm_dataread(URC_NETTYPE, gsm_strbuf, NULL, 0, 0);          
        }
      }
    }
    
    // +QACT: 1,<context_state>,<context_type>[,<IP_address>]]
    if (strncmp("+QACT:", ps, 6) == 0){
      var = skipchar(',', ps, 1);
      GSM.pdpAct = atoi(var);
    }    
  }
    
  // +QNTP: 0
  if ((ps = strstr(ptr, "+QNTP:")) != NULL){             
    gsm_dataread (NTPRESP, NULL, NULL, atoi(ps+7), 0);       
  }

  // Call
  /*if ((ps = strstr(ptr, "Call")) != NULL){             
    if (GSM.ready == 0){
      GSM.ready = 1;  
      gsm_dataread (READY, NULL, NULL, 0, 0);   
    }       
  }*/
  
  // CLOSED
  if (GSM.moduleType != M_SLM336){  
    if ((ps = strstr(ptr, "CLOSED")) != NULL){             
      if (GSM.ready){
        if (GSM.muxmode && (ps - ptr >= 3) && chkdigitchar(*(ps-3))){ // множество подключений, 0, CLOSED
          GSM.dataconnectionindex = atoi(ps-3);
        }
        gsm_dataread (CLOSECONNECT, NULL, NULL, 0, GSM.dataconnectionindex);
      }    
    }
  }
  
  // NO CARR
  if ((ps = strstr(ptr, "NO CARR")) != NULL){
    gsm_dataread (NOCARY, NULL, NULL, 0, 0);
  }
  
  // NO ANSWER
  if ((ps = strstr(ptr, "NO ANSWER")) != NULL){
    gsm_dataread (NOANSWER, NULL, NULL, 0, 0);
  }
  
  // RING
  if ((ps = strstr(ptr, "RING")) != NULL){             
    gsm_dataread (RING, NULL, NULL, 0, 0);
  }
  
  // Ready
  if ((ps = strstr(ptr, "RDY")) != NULL){             
    if (GSM.ready == 0){
      GSM.ready = 1;  
      gsm_dataread (READY, NULL, NULL, 0, 0);
    }      
  }
    
  // POWER DOWN
  if ((ps = strstr(ptr, "POWER DOWN")) != NULL){ 
    gsm_dataread (PWRDOWN, NULL, NULL, 0, 0);      
  }
  
  // SIM PIN
  if ((ps = strstr(ptr, "SIM PIN")) != NULL){             
    gsm_dataread (GETPIN, NULL, NULL, 0, 0);              
  }
  
    // ERROR
  if ((ps = strstr(ptr, "ERROR")) != NULL){             
    gsm_dataread (ATERROR, NULL, NULL, 0, 0);               
  }
  
  // OK
  if ((ps = strstr(ptr, "OK")) != NULL){             
    gsm_dataread (ATOK, NULL, NULL, 0, 0);              
  }  
  
  // 0, CONNECT 
  if (GSM.moduleType != M_SLM336){
    if ((ps = strstr(ptr, "CONNECT")) != NULL){
      // множество подключений, тогда до CONNECT был индекс, типа 0, CONNECT OK
      if (GSM.muxmode && GSM.ready){
        if ((ps - ptr >= 3) && chkdigitstr(ps-3, 1) && (*(ps-2) == ',')){
          GSM.dataconnectionindex = atoi(ps-3);
          if ((ptr + size - ps >= 10) && (strncmp("OK", ps+8, 2) == 0))
            gsm_dataread (CONNECTED, NULL, NULL, 0, GSM.dataconnectionindex);
          if ((ptr + size - ps >= 12) && (strncmp("FAIL", ps+8, 4) == 0))
            gsm_dataread (FAILCONNECT, NULL, NULL, 0, GSM.dataconnectionindex);
        }
      }            
    }
  }

    // +COPS: 0,0,"CHN-UNICOM",2
  if (GSM.moduleType == M_UG95){
    if ((ps = strstr(ptr, "+COPS:")) != NULL){   
      char oper[16];
      
      var = skipchar('"', ps, 1);
      if (var != ps){
        user_strcpy(oper, var, '"', 15);
        sprintf(gsm_strbuf,"AT+COPS=4,0,\"%s\",0\r",oper);
        gsm_runcmd(gsm_strbuf, "OK");        
      }
    }    
  }
  
  // +QNWINFO: “FDD LTE”,46011,“LTE BAND 3”,1825
  if ((ps = strstr(ptr, "+QNWINFO:")) != NULL){   
    var = skipchar('"', ps, 1);
    if (var != ps){
      user_strcpy(gsm_strbuf, var, '"', 15);
      var = skipchar(',', var, 1);
      if (var != ps)
        gsm_dataread(URC_NETTYPE, gsm_strbuf, NULL, atoi(var), 0);      
    }
  }
  
  // модуль не определен
  if ((GSM.moduleType == M_OTHER) || (GSM.moduleType == M_UNDEF)){
    // SLM3
    if ((ps = strstr(ptr, "SLM3")) != NULL){             
      GSM.moduleType = M_SLM336;
      gsm_dataread (SETMODULETYPE, NULL, NULL, GSM.moduleType, 0); 
  #ifdef DEBUG_GSM
      gsm_debug(">>> Module SLM336\r\n"); 
  #endif    
    }
    
    // EG915
    if ((ps = strstr(ptr, "EG915")) != NULL){             
      GSM.moduleType = M_UG95;
      gsm_dataread (SETMODULETYPE, NULL, NULL, GSM.moduleType, 0);
  #ifdef DEBUG_GSM
      gsm_debug(">>> Module EG915\r\n"); 
  #endif
    }
    
    // UG95
    if ((ps = strstr(ptr, "UG95")) != NULL){             
      GSM.moduleType = M_UG95;
      gsm_dataread (SETMODULETYPE, NULL, NULL, GSM.moduleType, 0);
  #ifdef DEBUG_GSM
      gsm_debug(">>> Module UG95\r\n"); 
  #endif
    }
    
    // M66
    if ((ps = strstr(ptr, "M66")) != NULL){             
      GSM.moduleType = M_M66;
      gsm_dataread (SETMODULETYPE, NULL, NULL, GSM.moduleType, 0);
  #ifdef DEBUG_GSM
      gsm_debug(">>> Module M66\r\n"); 
  #endif
    }
  }
}

// Инициализация GPRS ----------------------------------------------------------
/* >>> Поток RX: заголовки и строки отделены от двоичного тела. */
static char *gsm_linebuf = NULL;
static uint16_t gsm_linelen = 0;
static uint16_t gsm_linecap = 0;
static uint16_t gsm_remaining = 0;
static uint8_t gsm_smsline = 0;
static bool gsm_rxpending(void) {return gsm_linelen != 0 || gsm_remaining != 0;}

static void gsm_rxreset(void)
{
  free(gsm_linebuf);
  gsm_linebuf = NULL;
  gsm_linelen = gsm_linecap = gsm_remaining = 0;
  gsm_smsline = 0;
}

// Потерянное тело нельзя склеивать со следующим ответом: связь восстанавливается штатно.
static void gsm_rxfailed(void)
{
  gsm_rxreset();
  gsm_dataread(CLOSECONNECT, NULL, NULL, 0, GSM.dataconnectionindex);
}

static bool gsm_linebyte(char ch)
{
  if (gsm_linelen == gsm_linecap){
    uint16_t cap = gsm_linecap ? (uint16_t)(gsm_linecap * 2u) : 64u;
    if (cap > GSM_URC_MAX) return false;
    char *p = realloc(gsm_linebuf, (uint32_t)cap + 16u);
    if (p == NULL) return false;
    gsm_linebuf = p;
    gsm_linecap = cap;
  }
  gsm_linebuf[gsm_linelen++] = ch;
  memset(gsm_linebuf + gsm_linelen, 0, 16u);
  return true;
}

// Здесь завершается только заголовок; тело учитывается отдельно по фактически принятым байтам.
static bool gsm_frameheader(void)
{
  char *p;
  char *body;
  uint16_t n;
  if (strncmp(gsm_linebuf, "+IPD", 4) == 0){
    p = gsm_linebuf + 4;
    if (*p == ',') p++;
    if (!gsm_datalength(p, gsm_linebuf + gsm_linelen, ':', &n, &body)) return false;
  } else{
    p = strchr(gsm_linebuf, ',');
    if (p == NULL) return false;
    p++;
    while (*p == ' ') p++;
    if (*p < '0' || *p > '9') return false;
    uint16_t cid = 0;
    while (*p >= '0' && *p <= '9'){
      cid = (uint16_t)(cid * 10u + (uint8_t)(*p++ - '0'));
      if (cid > 255u) return false;
    }
    while (*p == ' ') p++;
    if (*p++ != ',') return false;
    if (!gsm_datalength(p, gsm_linebuf + gsm_linelen, '\n', &n, &body)) return false;
    if (GSM.muxmode) GSM.dataconnectionindex = (uint8_t)cid;
  }
  gsm_remaining = n;
  free(gsm_linebuf);
  gsm_linebuf = NULL;
  gsm_linelen = gsm_linecap = 0;
  return true;
}

void gsm_read(char* ptr, Int16U size)
{
  if (ptr == NULL || size == 0 || GSM.operation == PO_OFF) return;
  while (size){
    if (gsm_remaining){
      uint16_t n = size < gsm_remaining ? size : gsm_remaining;
      // Состояние передаётся до вызова: обработчик может отправить новую AT-команду.
      gsm_remaining -= n;
      gsm_dataread(DATA, ptr, NULL, n, GSM.dataconnectionindex);
      ptr += n;
      size -= n;
      continue;
    }
    char ch = *ptr++;
    size--;
    if (gsm_linelen == 0 && (ch == '\r' || ch == '\n')) continue;
    if (!gsm_linebyte(ch)){
      gsm_rxfailed();
      return;
    }
    if (ch == ':' && strncmp(gsm_linebuf, "+IPD", 4) == 0){
      if (!gsm_frameheader()) {gsm_rxfailed(); return;}
      continue;
    }
    if (ch != '\n') continue;
    char *urc = strncmp(gsm_linebuf, "+QIURC:", 7) == 0 ? strchr(gsm_linebuf, '"') : NULL;
    if (urc != NULL && strncmp(urc, "\"recv\"", 6) == 0){
      if (!gsm_frameheader()) {gsm_rxfailed(); return;}
      continue;
    }
    if (!gsm_smsline && (strncmp(gsm_linebuf, "+CMT:", 5) == 0 || strncmp(gsm_linebuf, "+CMGR:", 6) == 0)){
      gsm_smsline = 1; // СМС: строка заголовка и следующая строка текста идут вместе.
      continue;
    }
    char *line = gsm_linebuf;
    uint16_t len = gsm_linelen;
    gsm_linebuf = NULL;
    gsm_linelen = gsm_linecap = 0;
    gsm_smsline = 0;
    gsm_readtext(line, len);
    free(line);
  }
}
/* <<< Поток RX */

void gsm_initgprs(char *apn, char *login, char *pass)
{
  if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)) 
    sprintf(gsm_strbuf, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",0\r", apn, login, pass);
  else 
    sprintf(gsm_strbuf, "AT+QICSGP=1,\"%s\",\"%s\",\"%s\"\r", apn, login, pass);
  
  gsm_runcmd(gsm_strbuf, "OK");
}

// Запрос IMEI модуля ----------------------------------------------------------
void gsm_getimei(void)
{
  GSM.getimei = 1;
  gsm_runcmd("AT+GSN\r", "OK"); // Получить IMEI модуля
}

// Запрос уровня сигнала -------------------------------------------------------
void gsm_getcsq(void)
{
  gsm_runcmd("AT+CSQ\r", "OK"); // Уровень сигнала
}

// Отправка данных по GPRS -----------------------------------------------------
// buf - буфер, len - размер, c - соединение
bool gsm_senddata (pInt8U buf, Int16U len, Int8U c)
{
  if (len > 1900)
    return false;
  
  if (GSM.muxmode)
    sprintf(gsm_strbuf, "AT+QISEND=%d,%d\r", c, len);
  else 
    sprintf(gsm_strbuf, "AT+QISEND=%d\r", len);
  
  if (gsm_runcmd(gsm_strbuf, ">") == NULL) return false;
  gsm_send(buf, len);
  if (uart_tx_failed(GSM_PORT)) return false;
  
#ifdef DEBUG_GSM
  gsm_debug(">>>");
  ///usb_send(buf, len);
  gsm_debug("<<<\r\n");  
#endif
  
  return gsm_runcmd("\r", "OK") != NULL; // ответ SEND OK
}

// Подключить/отключить GPRS ---------------------------------------------------
// 0 - отключить 1 - включить
void gsm_setgprs (Int8U onoff)
{
  if (onoff){
    gsm_runcmd("AT+CGATT=1\r", "OK"); // Подключение к сети GPRS
    if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336))
      gsm_runcmd("AT+QIACT=1\r", "OK"); // Активация PDP контента (таймаут до 150 сек)
  }
  else{
    if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)) 
      gsm_runcmd("AT+QIDEACT=1\r", "OK"); // Деактивация PDP контекста
    else 
      gsm_runcmd("AT+QIDEACT\r", "OK"); // Деактивация PDP контекста
    gsm_runcmd("AT+CGATT=0\r", "OK");
  }
}

// Установить подключение ------------------------------------------------------
void gsm_connect (char *ip, Int16U port, Int8U c)
{
  if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)){
    // до 150 сек на подключение
    sprintf (gsm_strbuf, "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1\r",c,ip,port); // local port 0 and access mode 1 - direct push mode
    gsm_runcmd(gsm_strbuf, "OK"); // Подключиться к серверу
  } else{
    if (chkdigitchar(*ip))
      gsm_runcmd("AT+QIDNSIP=0\r", "OK"); // если первый симвод цифра, то подключение по ip
    else 
      gsm_runcmd("AT+QIDNSIP=1\r", "OK"); // иначе - через доменное имя 
    
    gsm_runcmd("AT+QIHEAD=1\r", "OK");
    if (GSM.muxmode) 
      sprintf (gsm_strbuf, "AT+QIOPEN=%d,\"TCP\",\"%s\",\"%d\"\r",c,ip,port);
    else 
      sprintf (gsm_strbuf, "AT+QIOPEN=\"TCP\",\"%s\",\"%d\"\r",ip,port);
    
    gsm_runcmd(gsm_strbuf, "OK"); // Подключиться к серверу    
  }
}

// Разорвать подключение -------------------------------------------------------
void gsm_disconnect (Int8U c)
{
  if (GSM.muxmode){
    sprintf(gsm_strbuf, "AT+QICLOSE=%d\r",c); 
    gsm_runcmd(gsm_strbuf, "OK");
  }
  else{
    gsm_runcmd("AT+QICLOSE\r", "OK");
  }
}

// Набрать номер ---------------------------------------------------------------
void gsm_call(char *num)
{
  if (num == NULL) return;
  
  sprintf(gsm_strbuf,"ATD%s;\r",num);
  gsm_runcmd(gsm_strbuf, "OK");
}

// Отправить USSD запрос -------------------------------------------------------
void gsm_sendussd (char* str)
{
  sprintf(gsm_strbuf,"AT+CUSD=1,\"%s\"\r",str);
  gsm_runcmd(gsm_strbuf, "OK");
}

// Отправка СМС сообщения ------------------------------------------------------
// num - номер, ptr - строка 
void gsm_sendsms (char *num, char *str)
{
  if (str == NULL) return;
  if (num == NULL) return;
  
  //sprintf (gsm_strbuf, "AT+CMGS=\"%s\",129\r",num);
  sprintf (gsm_strbuf, "AT+CMGS=\"%s\"\r",num);
  gsm_runcmd(gsm_strbuf, ">");
  gsm_sendstr(str);
  gsm_runcmd("\032\r", "OK"); // конец сообщения
}

// Запрос о состоянии регистрации в сети ---------------------------------------
/* Каждая команда ждёт ответа до GSMRESPTIMEOUT. Молчащий модуль стоил двух таймаутов (около 1
 * с) на каждом опросе раз в секунду и тормозил главный цикл, пока модуль молчал. Без ответа на
 * +CREG запрос +CGREG не отправляется, а после двух опросов без ответа до модуля доходит только
 * каждый третий. Любой ответ возвращает опрос на каждом вызове. */
#define GSM_REG_SILENT_POLLS  2u
#define GSM_REG_SILENT_EVERY  3u
void gsm_getregstate (void)
{
  char *p, *ps;
  uint8_t v;
  
  if (GSM.regSilent >= GSM_REG_SILENT_POLLS) {
    if (++GSM.regSkip < GSM_REG_SILENT_EVERY) return;
    GSM.regSkip = 0;
  }
  
#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Get reg state\r\n");
#endif
  
  p = gsm_runcmd("AT+CREG?\r", "OK"); // Отображать информацию о регистрации
  
  if (p == NULL || *p == 0) { // модуль молчит: не ждать ещё и ответа на +CGREG
    if (GSM.regSilent < 255) GSM.regSilent++;
    return;
  }
  GSM.regSilent = 0;
  GSM.regSkip = 0;
  
  if ((ps = strstr(p, "+CREG:")) != NULL){           
    if (*(ps+8) == ',')
      v = atoi(ps+9);
    else
      v = atoi(ps+7);
    
    if (v != GSM.creg){
      GSM.creg = v;
      gsm_dataread (CREG, ps+7, NULL, GSM.creg, 0);      
    }     
  }
  
  p = gsm_runcmd("AT+CGREG?\r", "OK"); // Отображать информацию о регистрации
  
  // +CGREG: 1,0
  if ((ps = strstr(p, "+CGREG:")) != NULL){        
    if (*(ps+9) == ',')
      v = atoi(ps+10);
    else 
      v = atoi(ps+8);
    
    if (v != GSM.cgreg){
      GSM.cgreg = v;
      gsm_dataread (CGREG, ps+8, NULL, GSM.cgreg, 0);      
    }        
  }
}

// Обработчик принятых данных --------------------------------------------------
__weak void gsm_dataread (gsmresp_e gsmop, char* ptr1, char* ptr2, uint32_t v1, int32_t v2)
{

}

// Завершить звонок ------------------------------------------------------------
void gsm_endcall (void)
{
  gsm_runcmd("ATH\r", "OK");
}

// Принять вызов ---------------------------------------------------------------
void gsm_answercall (void)
{
  gsm_runcmd("ATA\r", "OK");
}

// Отправить PIN code на СИМ карту ---------------------------------------------
void gsm_simpin (char* pin, char* newpin)
{
  if (newpin == NULL) 
    sprintf (gsm_strbuf, "AT+CPIN=\"%s\"\r",pin);
  else 
    sprintf (gsm_strbuf, "AT+CPIN=\"%s\",\"%s\"\r",pin,newpin);
  
  gsm_runcmd (gsm_strbuf, "OK");
}

// Запрос состояния питания ----------------------------------------------------
uint8_t gsm_getPowerState (void)
{
  return (uint8_t)GSM.pwr;
}

// узнать номер операции -------------------------------------------------------
/*uint8_t gsm_getoperation(void)
{
  return (uint8_t)GSM.operation;
}*/

// программное переключение СИМ карты ------------------------------------------
void gsm_selectSIM (uint8_t i)
{
  if (GSM.moduleType != M_SLM336){
    sprintf (gsm_strbuf, "AT+QDSIM=%d\r",i);
    gsm_runcmd (gsm_strbuf, "OK");
  }
}

// режим функциональности ------------------------------------------------------
void gsm_setFUN (uint8_t f)
{
  sprintf (gsm_strbuf, "AT+CFUN=%d\r",f);
  gsm_runcmd (gsm_strbuf, "OK");
}

// Ping remote server ----------------------------------------------------------
void gsm_ping (char* host)
{
  if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)){
    sprintf(gsm_strbuf,"AT+QPING=1,\"%s\"\r",host);
  } else{
    sprintf(gsm_strbuf,"AT+QPING=\"%s\"\r",host);
  }
  gsm_runcmd(gsm_strbuf, "OK");
}

// get RTC clock ---------------------------------------------------------------
void gsm_getclock (void)
{
  if (GSM.creg) 
    gsm_runcmd("AT+CCLK?\r", "OK");
}

// set NTP server --------------------------------------------------------------
void gsm_setNTPserver (char* ip, uint16_t p)
{
  switch (GSM.moduleType){
  case M_UG95:
    if (p == 0)
      sprintf(gsm_strbuf,"AT+QNTP=1,\"%s\"\r",ip);
    else
      sprintf(gsm_strbuf,"AT+QNTP=1,\"%s\",%d\r",ip, p);
    break;
    
  case M_SLM336:
    if (p == 0)
      sprintf(gsm_strbuf,"AT+QNTP=1,\"%s\"\r",ip);
    else
      sprintf(gsm_strbuf,"AT+QNTP=1,\"%s\",%d,1\r",ip, p);
    break;
    
  default:
    if (p == 0)
      sprintf(gsm_strbuf,"AT+QNTP=\"%s\"\r",ip);
    else
      sprintf(gsm_strbuf,"AT+QNTP=\"%s\",%d\r",ip, p);
    break;
  }

   gsm_runcmd(gsm_strbuf, "OK");
}

// инфо о переданных/подтвержденных/не подтвержденных данных -------------------
void gsm_getinformationsending (uint8_t c)
{
  if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)) 
    sprintf (gsm_strbuf, "AT+QISEND=%d,0\r",c);
  else 
    sprintf (gsm_strbuf, "AT+QISACK=%d\r",c);
  
  gsm_runcmd (gsm_strbuf, "OK");
}

// включить синхронизацию времени с временем сети ------------------------------
void gsm_enablenetworktime (void)
{
  if (GSM.moduleType != M_SLM336){
    gsm_runcmd("AT+QNITZ=1\r", "OK"); // Enable to synchronize time from GSM network
  }
  
  if ((GSM.moduleType == M_UG95) || (GSM.moduleType == M_SLM336)) 
    gsm_runcmd("AT+CTZU=1\r", "OK"); // Update GMT time with time zone to RTC, save time zone into NVRAM
  else 
    gsm_runcmd("AT+CTZU=2\r", "OK"); // Update GMT time with time zone to RTC, save time zone into NVRAM
  
  gsm_runcmd("AT+CTZR=1\r", "OK"); // Enable time zone change event reporting by unsolicited result code
}

// Чтение СМС из памяти
void gsm_readsms(uint8_t idx)
{
  strcpy (gsm_strbuf, "AT+CMGL=\"REC UNREAD\"\r");
#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Cmd ");
  gsm_debug(gsm_strbuf);
  gsm_sendStrAndWaitResp(gsm_strbuf, gsm_strbuf, "OK", GSMRESPTIMEOUTLONG);
  gsm_debug(">>> GSM: Answer ");
  gsm_debug(gsm_strbuf);
  gsm_debug("-----\r\n");  
#else
  gsm_sendStrAndWaitResp(gsm_strbuf, NULL, "OK", GSMRESPTIMEOUTLONG); 
#endif
}

// Удаление СМС из памяти
void gsm_deletesms(uint8_t idx)
{
  if (idx == 0)
    sprintf (gsm_strbuf, "AT+CMGD=%d,%d\r", 1, 4);
  else
    sprintf (gsm_strbuf, "AT+CMGD=%d\r", idx);
  
  gsm_runcmd (gsm_strbuf, "OK");
}

// установить тип используемой сети
void gsm_setNetworkType(netType_e nt)
{
  if (GSM.moduleType == M_UG95)
    gsm_runcmd("AT+COPS?\r", "OK");
  else if (GSM.moduleType == M_SLM336){
    sprintf(gsm_strbuf, "AT+QCFG=\"nwscanmode\",%d,1\r", (uint8_t)nt);
    gsm_runcmd(gsm_strbuf, "OK");
  }
}

// запрос типа сети
void gsm_getNetworkType(void)
{
  // +QNWINFO: <Act>,<oper>,<band>,<channel>
  if (GSM.moduleType == M_SLM336)
    gsm_runcmd("AT+QNWINFO\r", "OK");
}

// состояние активации gprs CGATT
uint8_t gsm_getCGATT(void){
  bool res = gsm_sendStrAndWaitResp("AT+CGATT?", gsm_strbuf, "OK", GSMRESPTIMEOUT);
  
#ifdef DEBUG_GSM
  gsm_debug(">>> GSM: Cmd AT+CGATT?\r\n <<< GSM: Answer ");
  gsm_debug(gsm_strbuf);
  gsm_debug("-----\r\n");
#endif
  if (res == true){
    char* ps;
    
    if ((ps = strstr(gsm_strbuf, "+CGATT:")) != NULL){
      return atoi(ps+8);
    }
  }
  return 0;
}

// запрос состояния PDP контекста
void gsm_getPDPState(void){
  gsm_runcmd("AT+QIACT?", "OK");
}

// тип модуля - чтение
moduleType_e gsm_getModuleType(void){
  return GSM.moduleType;
}

// тип модуля - установка
void gsm_setModuleType(moduleType_e mt){
  GSM.moduleType = mt;
}

__weak void gsm_user_debug (char* ptr)
{
  usb_sendstr(ptr);
}