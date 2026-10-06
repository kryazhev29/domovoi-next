// Доработка: Кряжев А., 2026
// Адаптер настроек, времени и событий модема к пользовательской логике.
// Домен и порт разбираются при старте и после сохранения команды 12.
// Настройки хранятся атомарно в хранилище; рабочие строки адаптера живут только в RAM.
#include "user_logic_internal.h"
static char comm_host[2][32];
static uint16_t comm_port[2];
static char comm_credential[sizeof(config.netOperator)];
// Обработчики используются только таблицей настроек этого модуля.
static const char *comm_text(uint8_t field, uint16_t *capacity);
static uint16_t comm_number(uint8_t field);
static const slm_settings_t comm_settings = {comm_text, comm_number};

// Вернуть достоверное местное время для автомата восстановления.
uint32_t user_comm_local_time(void)
{ return device.datetime >= ID_TIME_VALID ? device.datetime : UINT32_MAX; }

// Вернуть настроенный запрос баланса.
const char *user_comm_ussd(void) { return config.ussdBalance; }

// Вернуть разрешение VoLTE для текущего профиля.
uint8_t user_comm_volte(void) { return config.useVoLTE; }

// Обновить баланс и записать его изменение в журнал.
void user_comm_balance(int32_t rubles)
{
  device.balance = rubles < 1 ? 1u : rubles > 65000 ? 65000u : (uint16_t)rubles; /* статус 2: 0 - не определён */
  /* 37 «Баланс ниже нормы»: после каждого запроса, порог 100 р (триггер «баланс менее 100»). */
  if (rubles < 100) log_addSystemValue(LOG_SYS_LOWBALANCE, rubles);
}

// Записать системное событие слабого сигнала.
void user_comm_low_signal(uint8_t csq) { log_addSystemValue(LOG_SYS_LOWSIGNAL, csq); }

// Обновить поколение связи и записать событие её изменения.
bool user_comm_link(uint8_t kind, uint8_t address)
{
  static const uint8_t first[] = {LOG_SYS_CONNECT1, LOG_SYS_ERRCONNECT1, LOG_SYS_FAILCONNECT1, LOG_SYS_NOTGSMREG, LOG_SYS_NOTGPRSREG};
  if (kind >= sizeof(first)) return true;
  uint8_t event = first[kind];
  if (kind <= COMM_LINK_LOST) event = (uint8_t)(event + user_comm_server(address));
  return log_addSystemEvent((logSystem_e)event);
}

// Сообщить занятость обновлением или телефонным вызовом.
bool user_comm_busy(void) { return device.FWinProgress != 0 || comm_call_active(); }

// Сохранить разобранные домен и порт одного сервера.
static void comm_split(uint8_t n, const char *server)
{
  const char *colon = NULL, *p = server;
  size_t len;
  while (*p) { if (*p == ':') colon = p; ++p; }
  len = colon ? (size_t)(colon - server) : (size_t)(p - server);
  if (len >= sizeof(comm_host[n])) len = sizeof(comm_host[n]) - 1u;
  memcpy(comm_host[n], server, len); comm_host[n][len] = 0;
  comm_port[n] = colon ? (uint16_t)user_decimal(colon + 1) : 0;
}

// Сопоставить адрес канала с сервером настройки.
uint8_t user_comm_server(uint8_t address) { return address || (!config.useServer1 && config.useServer2); }

// Выбрать строку сервера и один раз разобрать её адрес.
void comm_endpoint(uint8_t n)
{
  comm_split(n, n || (!config.useServer1 && config.useServer2) ? config.server2 : config.server1);
}

// Вернуть ограниченное строковое поле настройки модема.
static const char *comm_text(uint8_t field, uint16_t *capacity)
{
  static const char empty[1] = "";
  switch (field) {
  case SLM_CFG_HOST: *capacity = sizeof(comm_host[0]); return comm_host[0];
  case SLM_CFG_HOST2: *capacity = sizeof(comm_host[1]); return comm_host[1];
  case SLM_CFG_APN: case SLM_CFG_LOGIN: case SLM_CFG_PASSWORD: {
    const char *p = config.netOperator, *end = p + sizeof(config.netOperator);
    uint8_t part = (uint8_t)(field - SLM_CFG_APN), n = 0;
    while (part && p < end && *p) { if (*p++ == ',') --part; }
    if (part) { *capacity = sizeof(empty); return empty; }
    while (p < end && *p && *p != ',' && n + 1u < sizeof(comm_credential)) comm_credential[n++] = *p++;
    comm_credential[n] = 0; *capacity = sizeof(comm_credential); return comm_credential;
  }
  default: *capacity = 0; return NULL;
  }
}

// Вернуть числовое поле настройки модема из готового состояния.
static uint16_t comm_number(uint8_t field)
{
  switch (field) {
  case SLM_CFG_PORT: return comm_port[0];
  case SLM_CFG_PORT2: return comm_port[1];
  case SLM_CFG_NETWORK: {
    /* Команда 12: авто, GSM, авто 3G/4G, UMTS, LTE; значения QCFG другие. */
    static const uint8_t rat[] = {0, 1, 0, 5, 3};
    return config.netType < sizeof(rat) ? rat[config.netType] : 0;
  }
  case SLM_CFG_RESERVE: comm_endpoint(1); return config.useServer2 && comm_host[1][0] && comm_port[1];
  /* Ручной тип (команда 105) имеет приоритет; 0/255 - определить по AT+GMM. */
  case SLM_CFG_TYPE: return device.moduleType;
  default: return 0;
  }
}

// Установить физический уровень вывода питания или ключа модема v5.
static void comm_gpio(uint8_t pin, bool high)
{
  uint16_t bit = (uint16_t)(1u << pin);
  if (high) GPIOC->BSRR = bit; else GPIOC->BRR = bit;
}

// Сохранить полученный тип модема.
static void comm_type(uint8_t type) { device.moduleType = type; }

// Сохранить принятый IMEI модема.
static void comm_imei(const char *imei) { memcpy(device.IMEIGSM, imei, 16); }

// Подготовить адреса и запустить модем с обработчиками приложения.
void user_comm_boot(void)
{
  comm_endpoint(0); comm_endpoint(1);
  static const slm_handlers_t handlers = {comm_gpio, comm_type, comm_imei};
  if (!user_comm_start(&comm_settings, &handlers, timers_getMillis()))
    (void)comm_modem_restart(true, timers_getMillis());
}
