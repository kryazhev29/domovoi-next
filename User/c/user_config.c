// Доработка: Кряжев А., 2026
// Стартовое чтение конфигурации, восстановление device и настройки по умолчанию.
// До конца user_init допускается только необходимый синхронный путь восстановления.
// Атомарной записью и выбором долговечного образа владеет memory_runtime.
#include "user_logic_internal.h"
#pragma data_alignment=4
cellMem_s mem[10];
#pragma data_alignment=4
config_s config;
#pragma data_alignment=4
device_s device;

// Проверить наличие всех пятнадцати цифр IMEI.
Int8U user_imeivalid(void)
{
  if (chkdigitstr(device.IMEIGSM, 15))
    return 1;
  return 0;
}

// Вычислить прежний универсальный ПИН без изменения его алгоритма.
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

// Стереть указанную область синхронно при стартовом восстановлении.
bool user_clearMemBlock(memBlock_e mb){
  if (mb==MB_SHEDULE) return sched16_clear();

  if (mem[mb].size == 0) return true;

#ifdef DEBUG_USER
  user_debug(">>U Clear mem block ...\r\n");
#endif

  uint32_t adr;
  uint32_t endAdr = mem[mb].start + mem[mb].size;

  // по секторно стираем
  for (adr = mem[mb].start; adr < endAdr; adr += 4096){
    if (!mem_erase_sector(adr)) return false;
    wdt_reload();
  }
  if (mb == MB_ID) idindex_scanned(0, 0); /* база пуста - индекс пересобирается в фоне */
  return true;
}

// Восстановить device, курсоры и стартовые счётчики.
void user_initvariables (void)
{
  // чистим
  memset((pInt8U)&device, 0, sizeof(device));

  device.connectionhandle = -1;
  device.memID = w25_read_devID();
  device.freeBufferSize = 2048;

  // чтение сохранённой информации
  user_readsavedinformation();
  device.protocolVersion = PROTOCOL_VERSION;

  user_comm_boot();

  // смотрим сколько в памяти номеров
  device.numbersCount = user_getNumbersCount();
  device.logsCount = log_getRecordsCount();
  if (device.logsCount)
    device.status.readyLogData = 1;
  device.tempIDsCount = user_getTempIDRecordsCount();
  if (device.tempIDsCount)
    device.status.readyID = 1;
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
static bool config_terminate(char *text, size_t capacity)
{
  if (memchr(text, 0, capacity)) return false;
  text[capacity - 1u] = 0;
  return true;
}
// Завершить строки внешних настроек и обновить CRC, если пришлось исправить границу.
void config_terminate_strings(void)
{
  bool changed = false;
  changed |= config_terminate(config.server1, sizeof(config.server1));
  changed |= config_terminate(config.server2, sizeof(config.server2));
  changed |= config_terminate(config.NTPserver, sizeof(config.NTPserver));
  changed |= config_terminate(config.netOperator, sizeof(config.netOperator));
  changed |= config_terminate(config.countryCode, sizeof(config.countryCode));
  changed |= config_terminate(config.simPhone, sizeof(config.simPhone));
  changed |= config_terminate(config.ussdBalance, sizeof(config.ussdBalance));
  if (changed) config.crc = crc16_calc((uint8_t *)&config, sizeof(config) - 2);
}

// Выбрать допустимый сохранённый config либо начальные настройки.
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
  else config_terminate_strings();
}

// Сформировать заводские настройки текущего профиля.
void user_setdefault (void)
{
  wdt_reload();

#ifdef DEBUG_USER
  user_debug(">>U Set default params ...\r\n");
#endif

  // чистим
  memset((pInt8U)&config, 0, sizeof(config));

  config.pinCode = 0x31323334;
  strcpy(config.server1, "gate.gsmgate.ru:15010");
  strcpy(config.server2, "gate.gsmgate.ru:15020");
  strcpy(config.countryCode, "+7");
  strcpy(config.netOperator, "internet,gdata,gdata");
  config.netType = 0; // автоматически: плата с модемом 4G
  config.useServer1 = 1;

  config.rele[0].mode = 0;
  config.rele[0].onCmdServer = 1;
  config.rele[0].onOther = 1;
  config.rele[0].tImpulse = 1;
  config.rele[0].tPause = 3;
  config.rele[0].cnt = 1;

  device.loadDefaultConfig = 1;
  device.moduleType = MODEM_TYPE_AUTO;
  device.status.defaultConfig = 1;
  log_addSystemEvent(LOG_SYS_DEFCONF);

  user_writeConfigAndDevice();
}

// Передать согласованный протокольный сброс владельцу хранения.
void user_factoryReset (void){
  fw_async_cancel();
#ifdef DEBUG_USER
  user_debug(">>U Factory reset\r\n");
#endif

  // закрываем соединение с сервером
  upload_reset(10);
  upload_reset(9);
  if (!user_writeConfigAndDevice()) return; // не стираем с сохранённым старым курсором

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

  /* Заводской сброс отменяет загрузку: user_setdefault сохраняет это состояние, и после
     перезапуска ничего не продолжается. */
  device.FWinProgress = 0;
  device.status.readyToUpdate = 0;
  device.updateTimeout = 0;

  // чистим конфиг
  user_setdefault();
  // реинициализируем переменные
  user_initvariables();

  // перезагружаем gsm
  user_comm_boot();
}

// Восстановить устойчивые поля device и незавершённые действия.
void user_readsavedinformation (void)
{
  device_s DV;
  uint8_t read_count = 10;

  while (read_count--){
    wdt_reload();
    // Блок DEVICE
    mem_read (mem[MB_CONFIG].start + sizeof(config), (pInt8U)&DV, sizeof(DV));
    if (DV.crc == crc16_calc((pInt8U)&DV, sizeof(DV)-2)){ // контрольная сумма совпала
      DV.IMEIGSM[sizeof(DV.IMEIGSM) - 1] = 0; // Завершаем после проверки CRC.
      strcpy(device.IMEIGSM, DV.IMEIGSM); // восстанавливаем IMEI GSM

      device.protocolVersion = PROTOCOL_VERSION; // всегда версия из таблицы (строка 426), не сохранённая
      device.hardwareVersion = DV.hardwareVersion;
      device.firmwareVersion = DV.firmwareVersion;
      device.bootVersion = DV.bootVersion;
      device.buildDate = DV.buildDate;

      device.datetimeConfig = DV.datetimeConfig;
      device.datetimeID = DV.datetimeID;
      device.datetimeShedule = DV.datetimeShedule;
      device.datetimeGroup = DV.datetimeGroup;
      device.datetimeHoliday = DV.datetimeHoliday;

      device.triggers = DV.triggers;

      device.firmwareSize = DV.firmwareSize;
      device.firmwareCRC = DV.firmwareCRC;
      device.FWblockCount = DV.FWblockCount;
      device.FWblockIdx = DV.FWblockIdx;
      device.firmwarePercent = DV.firmwarePercent;
      device.FWinProgress = DV.FWinProgress;
      device.updateTimeout = DV.updateTimeout;

      device.moduleType = DV.moduleType;
      upload_restore(&DV);
      store_restore(&DV);
      sched16_restore(&DV);

      break;
    }
    delay_ms(10);
  }
  fw_resume_after_boot();
}

// Сохранить device перед программным перезапуском.
void user_savereset (void)
{
  if (store_runtime) { idclear_runtime_cancel(); expiry_runtime_cancel(); fw_service_cancel(); upload_runtime_cancel(); service_crc_cancel(); table_runtime_cancel(); photo_storage_cancel(); store_reset_requested = 1; return; }
  log_flushPending(); // события, не дождавшиеся времени, - в журнал до перезагрузки
  user_saveindex(); // сохраняем индексы и т.п.
  wdt_reload();

  // Отключаем USB
  USB_PULLUP_OFF; // выключаем подтягивающий резистор

  // выключаем gsm
  comm_modem_off();

  // перезагружаем
  NVIC_SystemReset();
}

// Запросить сохранение device, учитывая стартовую синхронную фазу.
void user_saveindex (void)
{
  if (store_runtime) { store_save_requested = 1; return; }
  user_writeConfigAndDevice();
}

// Сохранить config/device через их единственный атомарный владелец.
bool user_writeConfigAndDevice(void)
{
  wdt_reload();
  config.crc = crc16_calc((pInt8U)&config, sizeof(config)-2);
  device.crc = crc16_calc((pInt8U)&device, sizeof(device)-2);
  bool ok = mem_patch2(mem[MB_CONFIG].start, (const uint8_t*)&config, sizeof(config),
                      (const uint8_t*)&device, sizeof(device));
  wdt_reload();
  return ok;
}
