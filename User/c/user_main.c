// Доработка: Кряжев А., 2026
// Главный цикл: входы, индикация, секундные сроки и запуск фоновых владельцев.
// Инициализация восстанавливает настройки до разрешения рабочих команд.
// Запись флеша выполняют владельцы хранилища; после сброса сроки восстанавливаются из device.
#include "user_logic_internal.h"
#include "user_comm.h"
#include "user_fw_reason.h"
#include "../sched16.h"
#include "photo.h"
#include "photo_runtime.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_info.h"
#include "../../Modules/comm_modem.h"
#include "../../Bootloader/app_update.h"
#pragma data_alignment=4
uint8_t port_buffer[PORTBUFSIZE];
#pragma data_alignment=4
char tempStrBuffer[128];
uint8_t uniqIDCnt;

// Вызвать обработчики миллисекундного тика драйверов.
void timers_user_handle(void)
{
  usb_timer1ms();
  uart_timer1ms();
}

// Передать целый проверенный USB-кадр разбору команд.
void usb_data_reading(uint8_t* ptr, uint16_t len)
{
  uint16_t body;
  if (len < 11 || memcmp(ptr, "SLDN", 4)) return;
  body = (uint16_t)ptr[4] | ((uint16_t)ptr[5] << 8);
  if (body < 3 || body != len - 8) return;
  /* Сборщик USB передаёт ровно один целый кадр с проверенной CRC. */
  user_parse_comand(ptr + 4, CH_USB);
}

// Восстановить память и настройки, затем разрешить рабочие команды.
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
  /* Логический адрес старого места. memory.c хранит новые образы в резерве 0x1F5000..0x1F8FFF
     (два сектора данных и два сектора фиксации). */
  mem[MB_CONFIG].start = 1889280;
  mem[MB_CONFIG].size = 4096;
  mem[MB_LOG].start = 1900544;
  mem[MB_LOG].size = 98304;
  mem[MB_TEMPID].start = 1999872;
  mem[MB_TEMPID].size = 49152;

  if (device.memID == 0x17C2){ // память 16 МиБ
    mem[MB_PHOTO].start = 2097152;
    mem[MB_PHOTO].size = 1048576;
  } else{
    mem[MB_PHOTO].start = 0;
    mem[MB_PHOTO].size = 0;
  }

  // Считываем конфигурацию
  // Ошибка recovery блокирует таблицы; конфигурация и связь используют отдельные области.
  (void)mem_recover();
  user_readconfig();

  // инициализация переменных
  user_initvariables();
  (void)sched16_boot();
  user_unblock_recheck(true);
  // Сброс по питанию: одна запись; при отказе журнала сохраняем метку.
  if (BKP->DR1 == PVD_RESET_MARK && log_addSystemEvent(LOG_SYS_RESETONPOWER))
    BKP->DR1 = 0;
  app_boot_metadata();

  // Виганд
  if (config.wiegandType != 0)
    WG_init();

  // внешний порт
  rs485_init(); // порт 485
  uart_setRecTimeout(10); // время заполнения/ожидания приемного буфера 10 мс
  store_runtime = 1;
}

// Выполнить очередной шаг входов, выходов и календарных сроков.
void user_loop(void)
{
  user_unblock_poll();
  /* Карта - первой и на каждом проходе: конец кадра ловится сразу после паузы,
     а не на следующем 10-мс такте. */
  if (config.wiegandType != 0 && user_storage_flash_ready())
    WG_process();
  if (user_storage_command_ready() && user_storage_flash_ready()) idindex_poll(mem[MB_ID].start); /* фоновая пересборка индекса ID, короткий шаг */
  fw_status_poll();
  unreg_sms_poll();
  reader_reply_poll(); /* пакет 8 ждёт свободного слота канала */

  // каждые 50 мс
  if (timers_get(1) >= 50)
  {
    timers_set(1,0);
    // запущен процесс обновления
    if (device.registredOnServer && device.FWinProgress){
      if (device.startWaitFWtime == 0){
        /* Срок ожидания - только для ушедшего запроса: канал занят (ушёл ответ 4) - повтор
           через 50 мс, а не через 20 с (было: блок в 20 с, 74 блока не успевали за 10 минут). */
        if (user_getFWblockFromServer((dataChanel_e)device.cchFWupdate, device.FWblockIdx))
          device.startWaitFWtime = timers_getMillis();
      } else{
        // не дождались блока прошивки
        if ((timers_getMillis() - device.startWaitFWtime) >= FW_BLOCK_RETRY_MS){
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

    led_fun100ms(); // обработка индикации
    user_readinputs(); // входы
    // состояние выходов
    device.status.stateOut1 = out_getstate(OUT1);
    device.status.stateOut2 = out_getstate(OUT2);
    /* Статус 1 вне очереди после изменения входа, выхода, триггера или бита 11. */
    if (device.registredOnServer){
      uint16_t st;
      memcpy((uint8_t*)&st, (uint8_t*)&device.status, 2);
      if (status_event_due(st, timers_getMillis()))
        user_send_statusPack(CH_SERVER);
    }
    // состояние выхода отображаем светодиодом
    if (device.status.stateOut1 || device.status.stateOut2){
      LED3_ON;
    } else{
      LED3_OFF;
    }
  }

  /* Выходы - раз в 100 мс (outs.h): импульс и пауза с точностью 0,1 с. */
  uint32_t outTicks = timers_take(0, 100);
  while (outTicks != 0) { --outTicks; out_process(); }

  // каждую 1 сек
  // воспроизводим накопленные секунды; новые тики остаются на следующий проход
  uint32_t elapsedSeconds = timers_take(3, 1000);
  while (elapsedSeconds != 0)
  {
    --elapsedSeconds;
    wdt_reload();

    // есть подключение по USB но не настроено
    if (USB_DETECT)
    {
      if (usb_get_state() != ON)
        usb_init();
    }
    else
      device.userAutorized = 0; // кабель вынут - ПИН вводится заново

    user_indication(); // индикация

    device.worktime++; // время работы от момента включения, в секундах
    device.datetime++; // внутренние часы
    log_checkPending(); // события, ждущие времени от сервера
    if (user_storage_flash_ready()) user_checkIdTimeEnd(); // окончание времени действия ID - событие 138
    // вычислим день недели, час и минуту
    device.day = user_getweakday(device.datetime);
    device.hour = device.datetime % 86400u / 3600u;
    device.minute = device.datetime % 3600u / 60u;
    // уровень сигнала
    device.connected = comm_channel_ready();
    /* Статусы 1/2 и NextConfig: сигнал, регистрация CS/PS, тип сети (как у прежней связи). */
    const comm_info_t *info = comm_info();
    device.csq = info->csq;
    device.creg = info->creg;
    device.cgreg = info->cgreg;
    device.netType = info->net;

    // перезагрузка через заданный таймаут
    if (device.timeToReset){
      if (--device.timeToReset == 0){
        log_addSystemEvent(LOG_SYS_RESETONCMD);
        user_savereset();
      }
    }

    // сброс на заводские
    if (device.timeToDefault){
      if (--device.timeToDefault == 0){
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
    sync_end_tick(); // события 6-10

    /* Перезагрузиться в режим обновления FW. Занятая флеш (журнал, база) отказала бы
       в чтении образа: ждём следующей секунды, а не теряем загрузку (плата 07.10). */
    if (device.rebootToUpdate && user_storage_command_ready()){
      device.rebootToUpdate = 0;
      if (app_boot_request()) {
        fw_event(LOG_FW_INSTALL, FW_REASON_NONE);
        user_savereset();
      } else {
        fw_stop(FW_REASON_INSTALL, uniqIDCnt++);
      }
    }

    // таймаут обновления
    if (device.updateTimeout){
      if (--device.updateTimeout == 0){
        fw_stop(FW_REASON_TIMEOUT, uniqIDCnt++);
      } else if (device.FWinProgress && device.updateTimeout % 60u == 0){
        user_saveindex(); /* после сброса докачка продолжает с оставшимся временем */
      }
    }

    /* Прошивка дошла до сервера - проба подтверждается записью во флеш. Занятая флеш -
       повтор через секунду: иначе через 30 мин загрузчик откатил бы годную прошивку (07.10). */
    if (app_trial_active() && device.registredOnServer && user_storage_command_ready()){
      app_trial_confirm();
      if (!app_trial_active()) fw_event(LOG_FW_DONE, FW_REASON_NONE);
    }

    // пробная прошивка после обновления так и не зарегистрировалась на сервере - откат
    if (app_trial_expired() && user_storage_command_ready()){
      app_trial_rollback();
      user_savereset();
    }
  }

  // каждую 1 минуту
  if (timers_get(5) >= 60000)
  {
    timers_set(5,0);

    // использовать время NTP - синхронизировать

    // задано время на перезагрузку
    if (config.restartTime != 0){
      if (device.worktime >= ((uint32_t)config.restartTime*3600)){
        log_addSystemEvent(LOG_SYS_RESETONTIMER);
        user_savereset();
      }
    }

  } // 1 минута
}

// Обработать RS485 после окончания кадра и освободить приёмный буфер.
void port_check_data(void)
{
  if (!user_storage_flash_ready()) return;
  photo_poll();
  if (!device.getPhoto && uart_getstate() && !uart_isReceiving())
    uart_clear();
}

// Продвинуть связь и владельцев хранения в главном цикле.
void gsm_check_data (void)
{
  /* Каждый проход главного цикла: порт, AT, модем, канал, сервер, восстановление. */
  user_comm_poll(timers_getMillis());
}

// Выбрать индикацию по состоянию сети и выходов.
void user_indication (void)
{
  // NET LED: на сервере - горит, модем в сети - мигает, выходит на связь - мигает часто
  if (comm_server_online()) led_load(NET_LED, LED_ON);
  else if (comm_modem_ready()) led_load(NET_LED, LED_FL1);
  else led_load(NET_LED, LED_FL2);
}

// Прочитать входы АЦП и записать события изменения.
void user_readinputs (void){
  // при отказе АЦП сохраняем последние состояния входов
  if (!adc_startInjectedConvertion()) return;

  // дискретное состояние входов
  device.status.stateIn1 = (adc_getinjectedvalue_mV(IN1_CHANEL) > 3000)?1:0;
  device.status.stateIn2 = (adc_getinjectedvalue_mV(IN2_CHANEL) > 3000)?1:0;

  // вход 1

  if (device.laststatus.stateIn1 != device.status.stateIn1){

    if (device.status.stateIn1){
      log_addSystemEvent(LOG_SYS_IN1ON);
    }
    else
      log_addSystemEvent(LOG_SYS_IN1OFF);
    device.laststatus.stateIn1 = device.status.stateIn1;
  }

  // вход 2
  if (device.laststatus.stateIn2 != device.status.stateIn2){

    if (device.status.stateIn2)
      log_addSystemEvent(LOG_SYS_IN2ON);
    else
      log_addSystemEvent(LOG_SYS_IN2OFF);
    device.laststatus.stateIn2 = device.status.stateIn2;
  }
}
