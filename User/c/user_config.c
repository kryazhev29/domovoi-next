// Доработка: Кряжев А., 2026
// Стартовое чтение конфигурации, восстановление device и настройки по умолчанию.
// До конца user_init допускается только необходимый синхронный путь восстановления.
// Атомарной записью и выбором долговечного образа владеет memory_runtime.
#include "user_logic_internal.h"
#include "../table_runtime.h"
#include "../sched16.h"
#include "../log_ring.h"
#include "photo_runtime.h"
#include "../../Modules/comm_modem.h"
#pragma data_alignment=4
cellMem_s mem[10];
#pragma data_alignment=4
config_s config;
#pragma data_alignment=4
device_s device;

enum { UNIVERSAL_PIN_BASE = 32768 };

// Формула заказчика; неверный IMEI не создаёт универсального ПИНа.
bool user_pin_valid(uint32_t pin)
{
  int32_t a = 0, b = 0, c = 0, d = 0;
  if (pin == config.pinCode) return true;
  for (uint8_t i = 0; i < 15u; ++i) {
    char ch = device.IMEIGSM[i];
    if (ch < '0' || ch > '9') return false;
    int32_t digit = ch - '0';
    if (i < 3u) c = c * 10 + digit;
    else if (i < 6u) d = d * 10 + digit;
    else if (i >= 11u) a = a * 10 + digit;
    else if (i >= 8u) b = b * 10 + digit;
  }
  if (device.IMEIGSM[15] != 0) return false;
  return pin == (uint16_t)(UNIVERSAL_PIN_BASE + 2 * a + c - 3 * b - d);
}

// ПИН настроек числом; 4 ASCII-цифры (старые платы, сервер СофтЭксперт) переводятся в число.
bool config_pin_number(uint32_t *pin)
{
  return *pin <= 99999u || user_parse_pin((const char *)pin, sizeof(*pin), pin);
}

// Восстановить device, курсоры и стартовые счётчики.
void user_initvariables (void)
{
  // чистим
  memset((pInt8U)&device, 0, sizeof(device));

  device.connectionhandle = -1;
  device.memID = w25_read_devID();

  // Сначала локальный device: тип модема и Wi-Fi не принадлежат конфигурации сервера.
  user_readsavedinformation();
  user_comm_boot();
  device.protocolVersion = PROTOCOL_VERSION;

  user_ids_boot();
  device.logsCount = log_getRecordsCount();
  device.tempIDsCount = user_getTempIDRecordsCount();
  // Очередь фото восстановит единственный асинхронный скан.
  device.countPhoto = 0;
  device.status.readyPhoto = 0;
}

// Обменять рабочую конфигурацию с телом принимаемой команды.
void config_exchange(uint8_t *data)
{
  uint8_t *p = (uint8_t*)&config;
  for (uint16_t i = 0; i < sizeof(config); i++) {
    uint8_t b = p[i];
    p[i] = data[i];
    data[i] = b;
  }
}

// Завершить строковые поля на границе нового config.
// Завершить только поле без нуля: исправленная конфигурация получает новую CRC.
// Есть ли конец строки в первых capacity байтах.
bool text_terminated(const char *text, size_t capacity)
{
  while (capacity--) if (!*text++) return true;
  return false;
}

#pragma inline=forced
static bool config_terminate(char *text, size_t capacity)
{
  if (text_terminated(text, capacity)) return false;
  text[capacity - 1u] = 0;
  return true;
}
// Завершить строки внешних настроек и обновить CRC, если пришлось исправить границу.
void config_terminate_strings(void)
{
  bool changed = false;
  // Все строковые поля внешней конфигурации проходят одну проверку границы.
  static const struct { uint8_t offset, size; } strings[]={
#define CONFIG_STRING(name) { offsetof(config_s,name), sizeof(config.name) }
    CONFIG_STRING(server1), CONFIG_STRING(server2), CONFIG_STRING(NTPserver),
    CONFIG_STRING(netOperator), CONFIG_STRING(countryCode), CONFIG_STRING(simPhone), CONFIG_STRING(ussdBalance)
#undef CONFIG_STRING
  };
  for (uint8_t i=0; i<sizeof(strings)/sizeof(strings[0]); ++i)
    changed |= config_terminate((char *)&config+strings[i].offset,strings[i].size);
  if (changed) config.crc = crc16_calc((uint8_t *)&config, sizeof(config) - 2);
}

// Выбрать допустимый сохранённый config либо начальные настройки.
void user_readconfig(void)
{
  wdt_reload();
  if (mem_read(mem[MB_CONFIG].start, (uint8_t *)&config, sizeof(config)) &&
      config.crc == crc16_calc((uint8_t *)&config, sizeof(config) - 2u)) {
    config_terminate_strings();
    if (config.pinCode > 99999u) {
      if (!config_pin_number(&config.pinCode)) config.pinCode = 4321u;
      // Сначала восстановить device вместе с ACK, затем сохранить общий снимок.
      config.crc = crc16_calc((uint8_t *)&config, sizeof(config) - 2u);
      store_save_requested = 1;
    }
  } else {
    user_setdefault();
    user_saveindex();
  }
}

// Сформировать заводские настройки текущего профиля.
void user_default_values(uint8_t source)
{
  wdt_reload();

  // чистим
  memset((pInt8U)&config, 0, sizeof(config));

  config.pinCode = 4321u;
  strcpy(config.server1, "gate.gsmgate.ru:15010");
  strcpy(config.server2, "gate.gsmgate.ru:15020");
  strcpy(config.countryCode, "+7");
  config.useServer1 = 1;

  config.rele[0].onCmdServer = 1;
  config.rele[0].onOther = 1;
  config.rele[0].tImpulse = 1;
  config.rele[0].tPause = 3;
  config.rele[0].cnt = 1;

  device.loadDefaultConfig = 1;
  device.moduleType = MODEM_TYPE_AUTO;
  device.wifiStop |= device.wifiModem && device.wifiMode; /* как USB 114: долг только при уходе с Wi-Fi */
  device.wifiModem = device.wifiMode = 0;
  device.wifiMark = 0x57;
  device.status.defaultConfig = 1;
  log_addSystemValue(LOG_SYS_DEFCONF, source);

}

void user_setdefault(void)
{
  user_default_values(5); // восстановление повреждённой/отсутствующей конфигурации
  user_writeConfigAndDevice();
}

// Передать согласованный протокольный сброс владельцу хранения.
void user_factoryReset (void){
  if (store_runtime && !flash_exec_idle()) {
    device.timeToDefault = 1;
    return;
  }
  if (!store_factory_begin()) {
    device.timeToDefault = 1;
    return;
  }
  device.timeToDefault = 0;
}

// Применить уже проверенное устойчивое состояние без I/O.
static void user_restore_device(device_s *saved)
{
  saved->IMEIGSM[sizeof(saved->IMEIGSM) - 1] = 0; // Завершаем после проверки CRC.
  strcpy(device.IMEIGSM, saved->IMEIGSM); // восстанавливаем IMEI GSM

  // Непрерывные устойчивые поля; рабочие сроки/состояния при старте не восстанавливаем.
  static const uint8_t keep[][2]={
    {offsetof(device_s,hardwareVersion), offsetof(device_s,datetime)-offsetof(device_s,hardwareVersion)},
    {offsetof(device_s,firmwareSize), offsetof(device_s,readLogIDIndex)-offsetof(device_s,firmwareSize)},
#define DEVICE_KEEP(name) {offsetof(device_s,name),sizeof(device.name)}
    DEVICE_KEEP(triggers), DEVICE_KEEP(FWblockIdx), DEVICE_KEEP(FWinProgress),
    DEVICE_KEEP(updateTimeout), DEVICE_KEEP(moduleType)
#undef DEVICE_KEEP
  };
  for (uint8_t i=0; i<sizeof(keep)/sizeof(keep[0]); ++i)
    memcpy((uint8_t *)&device+keep[i][0],(uint8_t *)saved+keep[i][0],keep[i][1]);
  // Бит 15 статуса: после заводского сброса сервер видит запрос полной синхронизации.
  device.status.defaultConfig = saved->status.defaultConfig;

  if (saved->wifiMark == 0x57 && (saved->wifiModem | saved->wifiStop) <= 1 && saved->wifiMode <= 2)
    memcpy(&device.wifiModem, &saved->wifiModem, 4);
  upload_restore(saved);
  sched16_restore(saved);
}
// Подготовить чистый device из сохранённых метаданных, без стартовых сканов.
void user_factory_variables(device_s *saved)
{
  uint32_t memory = device.memID;
  memset(&device, 0, sizeof(device));
  device.connectionhandle = -1;
  device.memID = memory;
  device.freeBufferSize = 2048;
  user_restore_device(saved);
  // Нулевые таймштампы в регистрации снимают у сервера флаг проверки на завод (Алексей 26.10).
  memset(&device.datetimeConfig, 0, offsetof(device_s, datetime) - offsetof(device_s, datetimeConfig));
  device.protocolVersion = PROTOCOL_VERSION;
  device.FWblockCount = 0;
  device.FWblockIdx = 0;
  device.FWinProgress = 0;
  device.updateTimeout = 0;
  log_ring_cleared();
  device.newAddressTempID = mem[MB_TEMPID].start;
  temp_next = 0;
}

// Восстановить устойчивые поля device и незавершённые действия.
void user_readsavedinformation (void)
{
  device_s DV;
  wdt_reload();
  if (mem_read(mem[MB_CONFIG].start + sizeof(config), (uint8_t *)&DV, sizeof(DV)) &&
    DV.crc == crc16_calc((uint8_t *)&DV, sizeof(DV) - 2u)) {
    if (!mem_cursor_restore(&DV)) { mem_storage_ready = false; return; }
    user_restore_device(&DV);
  }
  fw_resume_after_boot();
}

// Сохранить device перед программным перезапуском.
void user_savereset (void)
{
  if (store_runtime) {
    flash_exec_cancel();
    fw_service_cancel();
    upload_runtime_cancel();
    photo_storage_cancel();
    store_reset_requested = 1;
    return;
  }
  log_flushPending(); // события, не дождавшиеся времени, - в журнал до перезагрузки
  user_saveindex(); // сохраняем индексы и т.п.
  wdt_reload();

  // Отключаем USB
  USB_PULLUP_OFF; // выключаем подтягивающий резистор

  // выключаем gsm
  user_comm_off();

  // перезагружаем
  NVIC_SystemReset();
}

// Запросить сохранение device, учитывая стартовую синхронную фазу.
void user_saveindex (void)
{
  if (store_runtime) { store_save_requested = 1; return; }
  user_writeConfigAndDevice();
}

// Сохранить config/device при старте тем же автоматом A/B, что в работе, дожидаясь проверки W25.
bool user_writeConfigAndDevice(void)
{
  wdt_reload();
  config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);
  device.crc = crc16_calc((pInt8U)&device, sizeof(device)-2);
  if (!mem_config_begin((const uint8_t*)&config, (const uint8_t*)&device)) return false;
  while (mem_config_result() == 1) { wdt_reload(); mem_config_poll(timers_getMillis()); }
  bool ok = mem_config_result() == 2;
  mem_config_reset();
  return ok;
}
