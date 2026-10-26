// Доработка: Кряжев А., 2026
// Граница команд сервера и USB: длина, аргументы, ПИН и запуск нужного владельца.
// USB и сервер передают целый кадр с уже проверенной CRC.
// Долговечность изменений и ответы после записи обеспечивают вызываемые автоматы.
#include "usb_log.h"
#include "user_logic_internal.h"
#include "user_fw_reason.h"
#include "../sched16.h"
#include "../table_runtime.h"
#include "photo.h"
#include "photo_runtime.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_modem.h"
#include "../../Modules/comm_channel.h"
#include "../../Bootloader/app_update.h"
#include "../../Bootloader/boot_manual.h"
#include "prof.h"

// Ответ 11 отдаёт сам владелец или он не нужен; коды протокола 0–9, 254 и 255 заняты.
#define CMD_OWNER_REPLY ((cmdCode_e)253)

// Определить команды USB, доступные до авторизации.
static bool usb_cmd_without_pin(uint8_t cmd)
{
#if DOMOVOI_DIAG
  return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_DIAG_COMMAND || cmd == USB_COMM_DIAG_COMMAND;
#else
  return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_COMM_DIAG_COMMAND;
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

// Локальный USB-контракт Wi-Fi не расходует стек обычных команд сервера.
static void usb_wifi_command(uint8_t *ptr, uint16_t len, uint8_t uniqId)
{
  uint8_t reply[242]; /* 113: 6 Б состояния + точка доступа, уровень и SSID моста; 115: страница списка сетей (22.10.2026) */
  if (ptr[4] == 115) { /* список сетей Wi-Fi: тело [первая строка], ответ страницей */
    user_send_anyData(115, reply, comm_channel_wifi_scan(len > 3u ? ptr[5] : 0u, reply, sizeof(reply)), CH_USB, uniqId);
    return;
  }
  if (ptr[4] == 113) {
    uint8_t count = 2;
    if (len > 3u) { reply[0] = comm_channel_wifi_setup(&ptr[5], (uint16_t)(len - 3u)); reply[1]=0; }
    else { reply[2]=1; reply[3]=device.wifiModem; reply[4]=device.wifiMode; reply[0] = comm_channel_bridge(); reply[1] = comm_channel_wifi_link();
      reply[5] = reply[0] ? 1u : device.netType == 4 ? 4u : device.netType == 1 ? 2u : 0u;
      count = (uint8_t)(6u + comm_channel_wifi_info(&reply[6])); }
    user_send_anyData(113, reply, count, CH_USB, uniqId);
    return;
  }
  if (len != 5u || ptr[5] > 1u || ptr[6] > 2u || (!ptr[5] && ptr[6])) {
    user_send_answer_comand(114, CMD_PARAMERROR, CH_USB, uniqId); return;
  }
  {
    uint8_t old[4]; memcpy(old, &device.wifiModem, 4);
    /* Долг выключения — только при уходе с Wi-Fi: в «Только GSM» TCP моста уже выключен при каждом запуске. */
    device.wifiStop |= device.wifiModem && device.wifiMode && (!ptr[5] || !ptr[6]);
    device.wifiModem = ptr[5]; device.wifiMode = ptr[6]; device.wifiMark = 0x57;
    store_command_start(114, CH_USB, uniqId);
    memcpy(&device.wifiModem, old, 4); // Применение только после verify.
  }
}

// Резервная выгрузка базы ID в NextConfig (USB 116, Алексей 26.10.2026). Тело: первая ячейка (2 Б).
// Ответ: код (0 прочитано, 1 флеш занята — повторить), следующая ячейка (2 Б, 0 — конец),
// число записей и записи по 32 Б как в памяти; за вызов не больше 256 ячеек и 7 записей.
static void usb_ids_page(const uint8_t *ptr, uint16_t len, uint8_t uniqId)
{
  uint8_t reply[4u + 7u * sizeof(cellID_s)];
  uint32_t cells = mem[MB_ID].size / sizeof(cellID_s), from = len > 4u ? (uint32_t)(ptr[5] | ptr[6] << 8) : 1u;
  uint32_t cell = from ? from : 1u, stop = cell + 256u > cells + 1u ? cells + 1u : cell + 256u;
  uint8_t n = 0;
  reply[0] = !flash_exec_read_allowed();
  for (; !reply[0] && cell < stop && n < 7u; ++cell) {
    const cellID_s *c = (const cellID_s *)&reply[4u + n * sizeof(cellID_s)];
    if (!mem_read(mem[MB_ID].start + (cell - 1u) * sizeof(cellID_s), (uint8_t *)c, sizeof(cellID_s))) reply[0] = 1;
    else if (c->index != 0xffff && c->reserve[9] != 255) ++n; // пустые ячейки — как при стартовом скане
  }
  if (reply[0]) { n = 0; cell = from; }
  else if (cell > cells) cell = 0;
  reply[1] = (uint8_t)cell; reply[2] = (uint8_t)(cell >> 8); reply[3] = n;
  user_send_anyData(116, reply, (uint16_t)(4u + n * sizeof(cellID_s)), CH_USB, uniqId);
}

// Выполнить одну команду с проверенными длиной и контрольной суммой.
void user_parse_comand(uint8_t* ptr, dataChanel_e cch){

  uint16_t len;
  uint8_t uniqId;
  uint32_t tmp32;
  cmdCode_e code = CMD_OWNER_REPLY;

  len = ptr[1]; // длина
  len = (len<<8) | ptr[0];
  uniqId = ptr[3];
  wdt_reload();

  if (!command_args_valid(ptr, len)){
    code = CMD_PARAMERROR;
    goto reply;
  }

  /* ПИН (команда 102) проверялся, но нигде не требовался: без него по USB стирались база ID
     и настройки. Теперь остальные команды по USB - только после верного ПИНа, иначе ответ 4
     «нужен пароль» (18.09.2026). */
  if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4])){
    code = CMD_GETPASSWORD;
    goto reply;
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
        /* Неудачная установка или проба до этого старта: только журнал, версию сервер видит в
           статусе 2 (E122). */
        fw_boot_event_poll();
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
          store_factory_request(3, 0, 1, NULL);
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
        code = CMD_RUNERROR;
        break;
      }
      if (device.FWinProgress) fw_cancel(FW_REASON_CANCEL);
      device.timeToReset = ptr[5] ? ptr[5] : 2; // кода 5 в ответе 11 нет (строка 537)
      code = CMD_SUCCESS;
      break;

    case 3: // команда установить заводские настройки
      if (!store_factory_request((uint8_t)cch, uniqId, ptr[5], NULL)) code = CMD_RUNERROR;
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
      else code = CMD_PARAMERROR;
      break;

    case 6:
      device.timeToGetID = ptr[5];
      device.sourceGetID = ptr[6];
      device.addressGetID = ptr[7];
      code = CMD_SUCCESS;
      break;

    case 7: // запрос фото с камеры
      code = len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR;
      break;

    case 8: // управление выходами
      if (device.status.readyToUpdate || fw_update_quiet() || unblock_active){
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
        if ((len - 3) >= 13) user_strcpy(tempStrBuffer, (char*)&ptr[8], 0, 10);
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
              !user_activateOut(ptr[6] == 1, ptr[7] == 1, CT_SERVER, tempStrBuffer))
            res8 = CMD_RUNERROR;
        }
        if (flash_job.kind == FX_STORE && flash_job.step >= FS_ID) store_access_reply((uint8_t)cch, uniqId);
        else code = res8;
      }
      else code = CMD_PARAMERROR;
      break;

    case 9: // запрос журнала событий
    case 10: // запрос ID добавленных через SMS или Авторегистрацию
      upload_runtime_request(ptr[4] == 9 ? 10 : 9, cch, uniqId, ptr[2]); // 9 — журнал, 10 — временные ID
      break;

    case 11: { // отправить смс на заданный номер с заданным текстом
      /* Номер занимает 10 байт. Текст ограничен длиной кадра, NUL на проводе необязателен.
       * Собственная копия не позволяет захватить CRC или соседний пакет в текст SMS. */
      uint8_t digits = 0;
      if (len >= 14)
        while (digits < 10 && ptr[5 + digits] >= '0' && ptr[5 + digits] <= '9') ++digits;
      if (digits == 10 && ptr[15] != 0 &&
          text_terminated(config.countryCode, sizeof(config.countryCode))){
        uint16_t text_size = len - 13;
        char number[sizeof(config.countryCode) + 11];
        char *text = work_acquire(text_size + 1u, WORK_SMS);
        if (text == NULL){
          code = CMD_RUNERROR;
          break;
        }
        strcpy(number, config.countryCode);
        user_strcpy(number + strlen(number), (char*)&ptr[5], 0, 10);
        memcpy(text, &ptr[15], text_size);
        text[text_size] = 0;
        code = user_sendsms(text, number) ? CMD_SUCCESS : CMD_RUNERROR;
        work_release(text, WORK_SMS);
      }
      else code = CMD_PARAMERROR;
      break;
    }

    case 12: // установка настроек контроллера
      if ((len - 3) == sizeof(config)){
        const config_s *received = (const config_s *)&ptr[5];
        uint32_t pin;
        memcpy(&pin, &ptr[5 + offsetof(config_s, pinCode)], sizeof(pin));

        /* Сервер 1 и код страны обязательны; сервер 2 может быть пустым, его использование
           задаёт useServer2. */
        if ((received->server1[0] == 0) || (received->countryCode[0] == 0) ||
            !config_pin_number(&pin)){
          code = CMD_PARAMERROR;
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
            code = CMD_PARAMERROR;
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
          if (!saved) code = CMD_RUNERROR;
        }
      }
      else code = CMD_PARAMERROR;
      break;

    case 13: // отправить блок ID абонентов в контроллер
    case 14: // синхро. расписаний
    case 15: // синхро групп
    case 16: { // синхро. праздников
      bool valid;
      if (ptr[4] == 13) valid = sync_cells_valid(ptr, len, MB_ID, sizeof(cellID_s));
      else if (ptr[4] == 14) valid = sched16_valid(ptr, len);
      else if (ptr[4] == 15) valid = sync_cells_valid(ptr, len, MB_GROUP, sizeof(cellGroup_s));
      /* Праздники - список дат без номера ячейки (Протокол2, строки 359-362), синхронизируются
         целиком. Раньше номером ячейки считался младший байт даты: полночь отвергалась. */
      else valid = len >= 8 && ptr[9] != 0 && (uint32_t)(len - 8) == (uint32_t)ptr[9] * sizeof(cellHoliday_s);
      // Сектор определяется, читается, стирается и пишется автоматом таблиц.
      if (!valid) code = CMD_PARAMERROR;
      else if (!table_runtime_begin(ptr,cch,uniqId)) code = CMD_RUNERROR;
      break;
    }

    case 17: // CRC всех 5-ти блоков данных
      user_send_crcPack(cch, uniqId);
      break;

    case 18: // запрос CRC базы абонентов поблочно
      if (ptr[5] > 100) code = CMD_PARAMERROR; // блоков 100: номер больше - чтение мимо базы
      else user_send_crcMemoryID(ptr[5], cch, uniqId);
      break;

    case 19: // Обновление прошивки
      if ((len - 3) == 16){
        (void)fw_service_begin(ptr+5,cch,uniqId);
      } else{
        fw_event_request(LOG_FW_FAIL, FW_REASON_PARAM);
        code = CMD_PARAMERROR;
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
      else code = CMD_PARAMERROR;
      break;

    case 22: // запрос статуса 2
      user_send_statusPackSecond(cch, uniqId);
      break;

    case 23: // установить время контроллера
      memcpy((uint8_t*)&tmp32, &ptr[5], 4);
      log_setTime(tmp32);
      code = CMD_SUCCESS;
      break;

    case 24: // команда подтверждения приема команды от контроллера
      /* Ответ на статус 1 заканчивает ожидание и счёт пропущенных ответов. ptr[5] - уникальный
         номер подтверждаемого сообщения, а не команда: он не должен заканчивать ожидание ответа
         на регистрацию. */
      if (cch == CH_SERVER) comm_server_status_ack(ptr[5]);
      break;

    case 27: // произвольная команда
      code = CMD_SUCCESS;
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
      if (cch==CH_USB && !idclear_runtime_begin(cch,uniqId)) code = CMD_RUNERROR;
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
      if (cch == CH_USB)
        user_send_anyData(107, NULL, 0, cch, uniqId);
      break;

    #if DOMOVOI_DIAG
    case USB_DIAG_COMMAND:
      if (cch == CH_USB)
        user_send_usbDiag(uniqId);
      break;
    #endif
    case USB_COMM_DIAG_COMMAND:
      if (cch == CH_USB) { user_send_commDiag(uniqId); }
      break;

    case MANUAL_BOOT_COMMAND: { /* Файл загрузчика вручную, только по USB. */
      uint8_t response[8];
      if (cch != CH_USB || len < 4) {
        code = CMD_PARAMERROR;
        break;
      }
      flash_exec_args_t args = { NULL, FS_SYNC };
      if (device.FWinProgress || device.rebootToUpdate || app_trial_active() || !flash_exec_begin(FX_STORE, &args)) {
        code = CMD_RUNNING;
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
      flash_exec_finish(response[1] == MANUAL_OK ? FX_OK : FX_ERROR);
      flash_exec_take_result();
      break;
    }

    case 108: // включить лог
      if (cch == CH_USB) {
        user_send_anyData(108, NULL, 0, cch, uniqId);
        usb_log_enable(ptr[5]);
      }
      break;

    case 113: case 114: case 115: if (cch == CH_USB) usb_wifi_command(ptr,len,uniqId); break;
    case 116: if (cch == CH_USB) usb_ids_page(ptr,len,uniqId); break;

    default: // команда не поддерживается
      code = CMD_UNDEFINE;
      break;
  }
reply:
  if (code != CMD_OWNER_REPLY) user_send_answer_comand(ptr[4], code, cch, uniqId);
}
