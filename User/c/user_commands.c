// Доработка: Кряжев А., 2026
// Граница команд сервера и USB: длина, аргументы, ПИН и запуск нужного владельца.
// USB и сервер передают целый кадр с уже проверенной CRC.
// Долговечность изменений и ответы после записи обеспечивают вызываемые автоматы.
#include "user_logic_internal.h"
#include "user_fw_reason.h"
#include "../sched16.h"
#include "../table_runtime.h"
#include "photo.h"
#include "photo_runtime.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_modem.h"
#include "../../Bootloader/app_update.h"
#include "../../Bootloader/boot_manual.h"
#include "prof.h"

// Определить команды USB, доступные до авторизации.
static bool usb_cmd_without_pin(uint8_t cmd)
{
#if DOMOVOI_DIAG
  return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_DIAG_COMMAND || cmd == USB_COMM_DIAG_COMMAND;
#else
  return cmd == 100 || cmd == 101 || cmd == 102;
#endif
}

// Выбрать событие журнала по источнику управления выходом.
static logControl_e cmd8_event(uint8_t source)
{
  switch (source){
  case 1: return LOG_CON_OUTONBYAPP;
  case 3: return LOG_CON_OUTONBYTEST;
  case 4: return LOG_CON_OUTONBYVOICE;
  default: return LOG_CON_OUTONBYAPI;
  }
}

// Проверить размер, количество и номера принимаемых ячеек.
static bool sync_cells_valid(const uint8_t *ptr, uint16_t len, memBlock_e mb, uint16_t cell_size)
{
  uint16_t count, index;
  if (len < 8) return false;
  count = ptr[9];
  if (count == 0 || (uint32_t)(len - 8) != (uint32_t)count * cell_size) return false;
  for (uint16_t i = 0; i < count; i++){
    const uint8_t *cell = ptr + 10 + (uint32_t)i * cell_size;
    index = cell[0];
    if (mb == MB_ID) index |= (uint16_t)cell[1] << 8;
    if (index == 0 || index > mem[mb].size / cell_size) return false;
    if (mb == MB_ID && cell[12] == 0) return false;
  }
  return true;
}

// Проверить обязательные аргументы до обращения к полям кадра.
static bool command_args_valid(const uint8_t *ptr, uint16_t len)
{
  // Проверяем обязательные аргументы до любых действий команды.
  switch (ptr[4]){
    case 1:
      if (len < 4) return false;
      return ptr[5] != CMD_SUCCESS || len >= 8;
    case 2: case 3: case 18: case 24: case 105: case 108:
      return len >= 4;
    case 6: return len >= 6 && ptr[5] && (ptr[6] == 1 || (ptr[6] == 3 && ptr[7]));
    case 23: case 102: return len >= 7;
    default: return true;
  }
}

// Выполнить одну команду с проверенными длиной и контрольной суммой.
void user_parse_comand(uint8_t* ptr, dataChanel_e cch){

  uint16_t len;
  uint8_t uniqId;
  uint32_t tmp32;

  len = ptr[1]; // длина
  len = (len<<8) | ptr[0];
  if (len < 3) return; // В неполном заголовке нет команды и uniqId.
  bool server_verified = cch == CH_SERVER && server_rx_verified(ptr);
  uniqId = ptr[3];
  wdt_reload();

  if (cch != CH_USB && !server_verified && crc16_calc(ptr, len + 2) != rt_get16(ptr + len + 2))
  {
    user_send_answer_comand(ptr[4], CMD_CRCERROR, cch, uniqId);
    return;
  }

  if (!command_args_valid(ptr, len)){
    user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
    return;
  }

  /* ПИН (команда 102) проверялся, но нигде не требовался: без него по USB стирались база ID
     и настройки. Теперь остальные команды по USB - только после верного ПИНа, иначе ответ 4
     «нужен пароль» (18.09.2026). */
  if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4])){
    user_send_answer_comand(ptr[4], CMD_GETPASSWORD, cch, uniqId);
    return;
  }

  if (!server_verified && !store_command_allowed(ptr[4])) {
    user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId); return;
  }
  switch (ptr[4]) // команда
  {
    case 1: // Ответ контроллеру на регистрацию
      if (cch != CH_SERVER) break; // ответ на регистрацию бывает только от сервера
      comm_server_registered(ptr[5] == CMD_SUCCESS);
      switch ((cmdCode_e)ptr[5]){
      case CMD_SUCCESS:
        memcpy((uint8_t*)&tmp32, &ptr[6], 4); // время сервера
        log_setTime(tmp32);
        device.registredOnServer = 1;
        user_unblock_recheck(false);
        if (app_trial_active()){
          app_trial_confirm();
          if (!app_trial_active()) fw_event(LOG_FW_DONE, FW_REASON_NONE);
        } // прошивка дошла до сервера - обновление подтверждено
        /* Неудачная установка или проба до этого старта: только журнал, версию сервер видит в
           статусе 2 (E122). */
        if (app_boot_error_unreported()){
          fw_reason_e code = (fw_reason_e)app_boot_error_code();
          fw_event(code == FW_REASON_TRIAL_FAIL ? LOG_FW_ROLLBACK : LOG_FW_FAIL, code);
          app_boot_error_reported();
        }
        {
          uint16_t st;
          memcpy((uint8_t*)&st, (uint8_t*)&device.status, 2);
          status_event_sent(st, timers_getMillis());
        }
        break;

      case CMD_PARAMERROR:
      case CMD_RUNERROR:
      case CMD_UNDEFRESULT:
        device.registredOnServer = 0;
        comm_channel_reopen(0);
        break;

      case REG_UNKNOWN_IMEI:
      case REG_NOT_BOUND:
        // пробная прошивка могла сама прочитать IMEI неверно: не подтверждаем и базу не стираем,
        // при повторении откат по таймауту вернёт прежнюю прошивку
        if (!app_trial_active())
          user_factoryReset();
        device.registredOnServer = 0;
        comm_channel_reopen(0);
        break;

      default:
        break;
      }
      break;

    case 2: // перезагрузить
      /* Бит 11 (E440): перезагрузка отменяет загрузку; откат или установка, уже идущие в
         загрузчик, не трогаются. */
      if (device.rebootToUpdate){
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
      if (device.FWinProgress) fw_cancel(FW_REASON_CANCEL);
      if (ptr[5] == 0){

        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        device.timeToReset = 2;
      } else{
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); // кода 5 в ответе 11 нет (строка 537)
        device.timeToReset = ptr[5];
      }
      break;

    case 3: // команда установить заводские настройки
      if (ptr[5] == 0){
        user_factoryReset();
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      } else{
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId); // кода 5 в ответе 11 нет (строка 537)
        device.timeToDefault = ptr[5];
      }
      break;

    case 5: // Установка состояния триггерных функций
      if ((len - 3) == 16){
        memcpy((uint8_t*)&device.triggers, &ptr[5], 16);

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
        device.status.autoRegistration = device.triggers.autoregistration;
        device.status.openAnyUser = device.triggers.openAll;
        user_unblock_recheck(false);
        // сохранить состояние в памяти
        store_command_start(5, cch, uniqId);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 6:
      device.timeToGetID = ptr[5];
      device.sourceGetID = ptr[6];
      device.addressGetID = ptr[7];
      user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      break;

    case 7: // запрос фото с камеры
      user_send_answer_comand(7, len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR, cch, uniqId);
      break;

    case 8: // управление выходами
      if (fw_update_quiet() || unblock_active){
        user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        break;
      }
      if (ptr[5] != 0 && (len - 3) >= 3){
        cmdCode_e res8 = CMD_SUCCESS;
        //ptr[5] - источник события, от него зависит номер события в журнале (E184)
        //ptr[8] - ID абонента для передачи контроллеру информации для какого ID нужно сделать декримент счетчика проходов в сутки.
        /* ID абонента есть только в полном пакете (13 байт); в коротком ID пустой,
           за концом пакета ничего не читается. */
        memset(tempStrBuffer, 0, 11);
        if ((len - 3) >= 13) strncpy(tempStrBuffer, (char*)&ptr[8], 10);
        if (ptr[6] > 1 || ptr[7] > 1){
          res8 = CMD_PARAMERROR;
        } else{
          /* Протокол2, строки 185-188: 1 - включить, 0 - выключить; источник задаёт только номер
             события в журнале (тест из админки - тоже). Управляются только выходы, которым
             настройки реле разрешают команды сервера (onCmdServer); включение - по режиму реле
             и счётчику проходов ID. */
          server_out_event = cmd8_event(ptr[5]);
          if (ptr[6] == 0 && config.rele[0].onCmdServer && !config.rele[0].useInBlock) out_load(OUT1, OUT_OFF, 0, 0, 0);
          if (ptr[7] == 0 && config.rele[1].onCmdServer && !config.rele[1].useInBlock) out_load(OUT2, OUT_OFF, 0, 0, 0);
          /* Запрошенный выход не включился (не разрешён, занят, кончились проходы) - ответ 2,
             а не «выполнено»: сервер должен знать, что дверь не открылась (18.09.2026). */
          if ((ptr[6] == 1 || ptr[7] == 1) &&
              !user_activateOut((ptr[6] == 1) && config.rele[0].onCmdServer,
                           (ptr[7] == 1) && config.rele[1].onCmdServer, CT_SERVER, tempStrBuffer))
            res8 = CMD_RUNERROR;
        }
        user_send_answer_comand(ptr[4], res8, cch, uniqId);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 9: // запрос журнала событий
      user_send_logRecords(cch, uniqId);
      break;

    case 10: // запрос ID добавленных через SMS или Авторегистрацию
      user_send_tempIDs(cch, uniqId);
      break;

    case 11: // отправить смс на заданный номер с заданным текстом
      /* Номер занимает 10 байт. Текст ограничен длиной кадра, NUL на проводе необязателен.
       * Собственная копия не позволяет захватить CRC или соседний пакет в текст SMS. */
      if (len >= 14 && ptr[5] != 0 && ptr[15] != 0 &&
          memchr(config.countryCode, 0, sizeof(config.countryCode)) != NULL){
        uint16_t text_size = len - 13;
        char number[sizeof(config.countryCode) + 11];
        char *text = work_acquire(text_size + 1u, WORK_SMS);
        if (text == NULL){
          user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
          break;
        }
        strcpy(number, config.countryCode);
        strncat(number, (char*)&ptr[5], 10);
        memcpy(text, &ptr[15], text_size);
        text[text_size] = 0;
        user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
        user_sendsms(text, number);
        work_release(text, WORK_SMS);
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 12: // установка настроек контроллера
      if ((len - 3) == sizeof(config)){
        const config_s *received = (const config_s *)&ptr[5];
        uint32_t pin;
        memcpy(&pin, &ptr[5 + offsetof(config_s, pinCode)], sizeof(pin));

        /* Сервер 1 и код страны обязательны; сервер 2 может быть пустым, его использование
           задаёт useServer2. */
        if ((received->server1[0] == 0) || (received->countryCode[0] == 0) ||
            !config_pin_number(&pin)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        }
        else{
          uint32_t oldtime = device.datetimeConfig;
          uint16_t oldcrc = device.crc;
          uint8_t olddefault = device.status.defaultConfig;
          config_exchange(&ptr[5]);
          config.pinCode = pin;
          config_terminate_strings();
          /* Новый набор проверяется до записи; отказ оставляет прежние настройки. */
          comm_endpoints();
          if (!comm_modem_valid_settings()) {
            config_exchange(&ptr[5]); comm_endpoints();
            user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
            break;
          }
          device.datetimeConfig = config.datetime;
          device.status.defaultConfig = 0;
          store.channel = cch; store.uniq = uniqId; store.cmd = 12; store.quiet = 0;
          bool saved = store_config_start();
          config_exchange(&ptr[5]); comm_endpoints();
          device.datetimeConfig = oldtime;
          device.crc = oldcrc;
          device.status.defaultConfig = olddefault;
          if (!saved) user_send_answer_comand(ptr[4], CMD_RUNERROR, cch, uniqId);
        }
      }
      else{
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 13: // отправить блок ID абонентов в контроллер
      {
        // определяем сектор, в который будем писать, читаем, стираем и пишем.
        if (!sync_cells_valid(ptr, len, MB_ID, sizeof(cellID_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);

      }
      break;

    case 14: // синхро. расписаний
      {
        if (!sched16_valid(ptr, len)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);

      }
      break;

    case 15: // синхро групп
      {
        if (!sync_cells_valid(ptr, len, MB_GROUP, sizeof(cellGroup_s))){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);

      }
      break;

    case 16: // синхро. праздников
      {
        /* Праздники - список дат без номера ячейки (Протокол2, строки 359-362), синхронизируются
           целиком. Раньше номером ячейки считался младший байт даты: полночь отвергалась. */
        if (len < 8 || ptr[9] == 0 || (uint32_t)(len - 8) != (uint32_t)ptr[9] * sizeof(cellHoliday_s)){
          user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
          break;
        }
        if (!table_runtime_begin(ptr,cch,uniqId))
          user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);

      }
      break;

    case 17: // CRC всех 5-ти блоков данных
      user_send_crcPack(cch, uniqId);
      break;

    case 18: // запрос CRC базы абонентов поблочно
      if (ptr[5] > 100) { // блоков 100: номер больше - чтение мимо базы
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
        break;
      }
      user_send_crcMemoryID(ptr[5], cch, uniqId);
      break;

    case 19: // Обновление прошивки
      if ((len - 3) == 16){
        (void)fw_service_begin(ptr+5,cch,uniqId);
      } else{
        fw_event(LOG_FW_FAIL, FW_REASON_PARAM);
        user_send_answer_comand(ptr[4], CMD_PARAMERROR, cch, uniqId);
      }
      break;

    case 20: // Блок прошивки, ответ командой 4
      {
        uint8_t code = fw_take_block(&ptr[5], (uint16_t)(len - 3), cch);
        if (code == FW_BLOCK_PENDING) fw_async_reply(uniqId);
        else user_send_statusUpdateFW(ptr[4], code, cch, uniqId);
      }
      break;

    case 21: // запрос фотографий
      if (len == 3 || len == 9)

        photo_request_answer(cch,uniqId,ptr+5,(uint8_t)(len-3));
      else user_send_answer_comand(21, CMD_PARAMERROR, cch, uniqId);
      break;

    case 22: // запрос статуса 2
      user_send_statusPackSecond(cch, uniqId);
      break;

    case 23: // установить время контроллера
      memcpy((uint8_t*)&tmp32, &ptr[5], 4);
      log_setTime(tmp32);
      user_send_answer_comand(ptr[4], CMD_SUCCESS, cch, uniqId);
      break;

    case 24: // команда подтверждения приема команды от контроллера
      /* Ответ на статус 1 заканчивает ожидание и счёт пропущенных ответов. ptr[5] - уникальный
         номер подтверждаемого сообщения, а не команда: он не должен заканчивать ожидание ответа
         на регистрацию. */
      if (cch == CH_SERVER) comm_server_status_ack(ptr[5]);
      break;

    case 27: // произвольная команда
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
        device.userAutorized = user_parse_pin((const char *)&ptr[5], len - 3u, &tmp32) &&
                               user_pin_valid(tmp32);
        user_send_anyData(102, &device.userAutorized, 1, cch, uniqId);
      }
      break;

    case 103: // очистить память идентификаторов
      if (cch==CH_USB && !idclear_runtime_begin(cch,uniqId))
        user_send_answer_comand(ptr[4],CMD_RUNERROR,cch,uniqId);
      break;

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

    #if DOMOVOI_DIAG
    case USB_DIAG_COMMAND:
      if (cch == CH_USB)
        user_send_usbDiag(uniqId);
      break;
    case USB_COMM_DIAG_COMMAND:
      if (cch == CH_USB) { user_send_commDiag(uniqId); }
      break;

    #endif
    case MANUAL_BOOT_COMMAND: { /* Файл загрузчика вручную, только по USB. */
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
        /* Дать USB отправить подтверждение. Успех проверяется после перезапуска. */
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
