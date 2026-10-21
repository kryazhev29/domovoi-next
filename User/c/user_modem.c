// Доработка: Кряжев А., 2026
// Адаптер настроек, времени и событий модема к пользовательской логике.
// Домен и порт разбираются при старте и после сохранения команды 12.
// Настройки хранятся атомарно в хранилище; рабочие строки адаптера живут только в RAM.
#include "user_logic_internal.h"
#include "../../Modules/comm_call.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_modem.h"
#include "user_comm.h"
#include "../../Modules/comm_at.h"
#include "../comm_timeouts.h"
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

// Вернуть настроенный запрос баланса. SLM336 - пусто: USSD рвёт передачу данных и в 2G, и в 4G
// (плата №1 21.10.2026: после меню *105# статус без TCP-подтверждения, модем завис, перезапуск питанием);
// старая v5 (gsm.c, gsm_sendussd) USSD на SLM336 тоже не отправляла.
const char *user_comm_ussd(void) { return device.moduleType == MODEM_TYPE_SLM336 ? "" : config.ussdBalance; }

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

// Сохранить разобранные домен и порт одного сервера. Адрес без порта (так его записал
// сервер СофтЭксперт 20.10) получает заводской порт своего поля: 15010 или 15020.
static void comm_split(uint8_t n, const char *server)
{
  const char *colon = NULL, *p = server;
  size_t len;
  while (*p) { if (*p == ':') colon = p; ++p; }
  len = colon ? (size_t)(colon - server) : (size_t)(p - server);
  if (len >= sizeof(comm_host[n])) len = sizeof(comm_host[n]) - 1u;
  memcpy(comm_host[n], server, len); comm_host[n][len] = 0;
  comm_port[n] = server == config.server2 ? 15020u : 15010u;
  if (colon) {
    comm_port[n] = 0;
    uint16_t pos = 0, count = (uint16_t)(p - colon - 1);
    uint32_t port;
    if (comm_at_number((const uint8_t *)colon + 1, count, &pos, UINT16_MAX, &port) && pos == count)
      comm_port[n] = (uint16_t)port;
  }
}

// Сопоставить адрес канала с сервером настройки.
uint8_t user_comm_server(uint8_t address) { return address || (!config.useServer1 && config.useServer2); }

// Один раз разобрать адреса обоих серверов: основной (с учётом выбора) и резервный.
void comm_endpoints(void)
{
  comm_split(0, config.useServer1 || !config.useServer2 ? config.server1 : config.server2);
  comm_split(1, config.server2);
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
  /* Пустой резерв разрешён протоколом; непустой проверяется даже при порте 0.
     Когда выбран только сервер 2, второго адреса для повтора нет. */
  case SLM_CFG_RESERVE: return config.useServer1 && config.useServer2 && comm_host[1][0];
  /* Wi-Fi-мост DR164 задан в таблице 485 типом 1 при активном режиме линии. Адрес 0 -
     резерв по Протоколу2: такая запись не включает мост (сервер 20.10 писал адрес на место типа). */
  case SLM_CFG_WIFI: {
    if (config.rs485Mode != 1) return 0;
    for (uint8_t i = 0; i < 8u; ++i)
      if (config.rs485Periphery[i].type == 1 && config.rs485Periphery[i].address) return 1;
    return 0;
  }
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
  static const slm_handlers_t handlers = {comm_gpio, comm_type, comm_imei};
  comm_endpoints();
  /* Неверные настройки не запускают модем: перезапуск с ними бессмыслен. */
  if (comm_modem_state() != SLM_IDLE) comm_modem_off();
  (void)user_comm_start(&comm_settings, &handlers, timers_getMillis());
}

// Проверено на плате №1 13.10.2026 (a83833c): 9 стартов подряд, QPOWD перед каждым, до TCP 14,8-15,9 с, сбоев настройки нет; повтор шага проверен на модели, на плате не срабатывал
// Перед перезагрузкой: модем выходит из сети, затем снимается питание.
void user_comm_off(void)
{
  uint32_t start = timers_getMillis();
  while (!comm_modem_powered_down() && timers_getMillis() - start < comm_timeouts[COMM_T_POWER_DOWN])
    wdt_reload();
  comm_modem_off();
}
