# Построчный разбор лишнего (архитектор, 06–07.10.2026), база 3b30322 HYBRID

Формат: файл:строки (C-эталон; в гибриде горячие функции — ASM-копии в User/hybrid/user_fun_hot.s, правка в обоих) — что лишнее — почему безопасно убрать — оценка.

## user_fun_runtime.c

- 572–797 приём кадров сервера: кредиты работы, compacted/commands/provisional/defer/cancelled. Упростить (п.3 чистки). ~400 Б.
- 3126–3148 CRC USB-кадра пересчитывается, хотя usb_data_reading получает «ровно один целый кадр с проверенной CRC». ~20 Б, время.
- 3442–3507 команды 13–16: mem_tables_ready()+store_command_allowed в разборе И в table_runtime_begin. ~60 Б.
- 5117–5130 user_getPhotoCount: синхронный скан фото при старте дублирует асинхронный (pr_poll PR_META/HEAD). ~750 Б вместе с photo_recover/confirmed_offset/cleanup_active.
- 1984–2077 user_send_crcPack / user_send_crcMemoryID: ветка без store_runtime (синхронный CRC с буфером 1 КБ на стеке, id_crc16) — команды 17/18 приходят только после user_init (store_runtime=1) → синхронная ветка недостижима. Проверить: есть ли вызов до store_runtime=1. ~300 Б.
- 243–281 upload_ack / upload_finish: то же — синхронная ветка при store_runtime=0. Проверить достижимость.
- 1762–1889 user_send_registrationData / user_send_statusPack: SLDN-заголовок и CRC собираются вручную, рядом есть packet_send. Плюс пустые case CH_SMS. ~120 Б.
- 2082–2094 user_send_smsToServer: `if (send_buffer != NULL)` после уже сделанного return на NULL; `!sms` для внутреннего вызова.
- 1731–1733 user_send_anyData: size>65524 и data==NULL для внутренних вызовов.
- 380–393, 1007–1024, 515–519, 971–977 пустые заглушки/отладка: user_smssended, user_comanderror, user_parseUSSD, user_wait_stable_power, gsm_user_debug.
- 1340–1344 пустой 10-секундный таймер в user_loop.
- 937–939 SMS «update» — пустая ветка (strcmp + ничего). Либо реализовать, либо убрать строку.
- 865–866 strcmp четырёх команд для проверки режима обновления повторяет последующие strcmp — разбирать команду один раз (номер), потом switch. ~100 Б.
- photo_runtime.c 94–114 photo_fail строит строку 200 Б для user_debug, который в Release — пустая функция (2 Б в карте). ~250 Б.
- photo_runtime.c 62–77, 86 user_debug-строки RS485/Photo в Release — в пустую функцию.

## comm_at.c

- 86–99, 198–224 M66 (QIRD M66, m66_length, M66_HEADER) — на v5 не используется. ~300 Б.
- 143–167, 226–235 проверки аргументов внутренних вызовов (NULL, >INT32_MAX, «AT», печатность каждого символа). ~80 Б.

## comm_modem.c

- 415–417 slm_send: slm_valid_settings (цикл comm_at_field по 4 строкам) перед КАЖДОЙ AT-командой; уже проверено в comm_modem_start. ~40 Б, время.
- 260–264, 301–304 проверки указателей профиля/обработчиков — профиль один (SLM336), константный.
- in_poll — проверить реальную реентерабельность.

## Мёртвые синхронные ветки (store_runtime=1 в конце user_init и больше не сбрасывается)

Доступно до store_runtime=1 (оставить): user_writeconfig, user_saveindex, user_writeConfigAndDevice, user_setdefault, sched16_boot, fw_resume_after_boot→user_saveindex, upload_restore.
Только после старта (синхронная ветка недостижима — убрать вместе с функциями, которые больше никто не зовёт):
- 2834 fw_rollback_poll: `if (store_runtime) return;` первой строкой — тело (стирание/копирование/проверка) мёртвое; вызов из user_loop тоже. ~240 Б.
- 2976–2994 fw_take_block после `if (store_runtime) return fw_async_begin…` — синхронная запись, fw_block_written, повторы. ~250 Б.
- 1985–2019 user_send_crcPack, 2025–2077 user_send_crcMemoryID — синхронный CRC (id_crc16, user_calcCRC16InFlash с буфером 1 КБ на стеке). ~350 Б, если user_calcCRC16InFlash/id_crc16 больше нигде.
- 243–258 upload_ack, 261–280 upload_finish — ветка без store_runtime. ~200 Б.
- 2344/2428 `store_runtime &&` в условии — всегда истина в работе.
- 3163 `store_runtime &&` в разборе команд — всегда истина.
- memory_runtime.c:20 `(!store_runtime && mem_recover())` — проверить достижимость; upload_runtime.c:36 `!store_runtime && command!=10`; fw_service_runtime.c:44 `if(!store_runtime) return false;`; flash_w25_runtime.c:72 тернарный по store_runtime — разобрать каждую: до user_init или нет.
Перед удалением: вызвать ли что-то из этого в user_init/user_readsavedinformation/user_setdefault/factoryReset ДО строки 1087 — проверить граф вызовов, не грепом по имени, а по фактическим вызовам из user_init (и тест старта).

## Дубли кадрирования SLDN

Вручную собирают SLDN+длину+CRC (рядом есть packet_send): user_send_registrationData, user_send_statusPack, user_send_smsToServer, reader_reply_poll, user_send_logRecords, user_send_tempIDs, user_getFWblockFromServer, fw_status_poll. ~8 копий × ~30 Б. Плюс пустые `case CH_SMS:` и `if (cch != CH_USB && cch != CH_SERVER) return;` ~250 Б всего.

## Причины обновления строками

fw_reason_code (2690–2701): причины передаются строками ("PARAM", "BUSY", … 15 штук) и переводятся в код через strcmp по таблице; fw_end("…"), fw_abort("…"), fw_log(..., "…"), app_boot_error_text→strcmp("TRIAL FAIL"). Передавать сразу код (enum). ~250 Б (строки + strcmp-цикл).

## user_fun_runtime.c 3697–4400

- 3836–3876 user_findTempID и user_addIdToTempBuffer: один и тот же цикл поиска по буферу временных ID — addIdToTempBuffer должна звать findTempID. ~60 Б.
- 3711–3757 user_readinputs: пустые switch по config.inputMode («//777») — код без действия. Убрать.
- 4246–4267 user_activateOut: switch tp→событие — таблица из 6 байт. ~40 Б.
- 4332–4365 user_getNumbersCount: синхронный проход всей базы 1,6 МБ при старте (буфер 1 КБ на стеке) — проверить, не дублирует ли фоновую пересборку индекса (idindex_poll/idindex_scanned) и пересчёт в table_scan. Если дублирует — оставить один (фоновый), счётчик/конец базы брать оттуда.
- 3796–3832 user_findID: запасной линейный перебор с буфером 1 КБ на стеке, пока индекс не готов — оставить (нужен), но буфер можно 256 Б (тот же код, меньше стек).
- 3938–3944 unblockOnScene → return (сцены не исполняются) — известная задача, ждёт Алексея, не трогать в чистке.

## user_fun_runtime.c 4400–5663

- 76 мест `stage4_unused_*` (log_clearLogBuffer, log_uniqueIndex, log_addEvent, log_getRecordsCount, log_readRecords, log_async_*…) — в BIN не попадают (линковщик выбрасывает), но это мёртвый исходник: удалить (0 Б флеша, −сотни строк).
- 5008–5103 log_addSystemEvent / log_addSystemEventID / log_addControlEvent / log_addUpdateEvent / log_addPhotoEvent: пять копий «memset, index=datetime, datetime, logType, …, log_put» — одна общая функция заполнения + тонкие обёртки. ~120 Б.
- 4737–4741 fw_async_begin: проверка диапазона адреса, который вычислен внутри fw_take_block из проверенного номера блока. Убрать.
- 4806–4812 FA_HEADER: повторная проверка полей заголовка, уже проверенных fw_start_rejected при команде 19 и при fw_resume_after_boot. Оставить только сравнение CRC.
- 5325–5345 comm_split/comm_endpoint: строка «домен:порт» разбирается (strrchr+atoi) заново при КАЖДОМ запросе поля настройки модема (comm_text/comm_number на каждую AT-команду). Разбирать один раз при старте и при сохранении команды 12 (store_config_complete). Убирает и atoi отсюда. ~60 Б, время.
- 5187–5263 user_send_usbDiag/commDiag — ручная сборка SLDN (только стенд, DOMOVOI_DIAG) — через packet_send.
- 5541–5661 user_storage_poll: цепочка из 15 «if active → poll; return». Проверить, нужна ли store US_RECOVER (совместимость со «старым приложением после отката» — после перехода на кольцевой журнал этапа 5 и отказа от переноса старого журнала это, возможно, мёртвая ветка для cmd 10). Проверить: store_finish(10)/US_ERASE стирает весь MB_LOG — при кольцевом журнале выгрузка журнала подтверждается через log_ring_ack_poll, а не стиранием области; если так, ветки cmd==10 в store_finish/US_ERASE/US_RECOVER/store_restore мёртвые. ~300 Б.

## memory_runtime.c (хранилище настроек)

- Каждое сохранение настроек пишет ТРИ копии: слот A/B (8 КиБ, CRC32, поколение) + зеркало на старое место 0x1CD400 (2 сектора, для отката на старую прошивку и старого загрузчика 0x32). Новый загрузчик (Bootloader/) старое место не читает. Синхронно: cfg_prepare/cfg_legacy_crc/cfg_equal_flash/cfg_mirror/cfg_blank_sector; асинхронно: состояния CA_MIRROR0/1, CA_TAIL/DEVICE/CONFIG. **ВОПРОС ВЛАДЕЛЬЦУ:** нужен ли откат на прошивки до A/B и плата со старым загрузчиком 0x32? Если нет — зеркало убрать: ~1,2 КБ и вдвое меньше стираний/записей на каждое сохранение.
- 21–35 mem_guard / mem_reserved: проверка переполнения адреса `n > 0xFFFFFFFF - adr` при каждом чтении/записи — адреса внутренние. Небольшое.
- 297 mem_patch2: проверки NULL для внутренних вызовов.
- 318–337 mem_rewrite (совместимый вход «для старых тестов»), mem_erase_mem (div()) — проверить, вызываются ли в прошивке; если только тесты — убрать.

## comm_channel.c

- 179 OP_OPEN: comm_at_field(host) — третья проверка того же адреса (после сохранения настроек и slm_valid_settings). Убрать.

## uart_stm32f10x.c (1 КБ)

- Драйвер на три USART, в работе только PORT1_PORT=0 (USART1, RS-485); модем — отдельный comm_uart (USART3, DMA). uart_init с ветками USART2/USART3, параметр num во всех функциях, USART2_IRQHandler. Свести к одному порту 485 на регистрах. ~500 Б.
- uart_waitresp / uart_sendStrAndWaitResp / uart_matchresponse / uart_sendstr / uart_on / uart_off / uart_takerx / uart_rxview / uart_getptr — в BIN нет (не вызываются), исходник удалить.

## comm_sms.c — оставить (PDU нужен для кириллицы/UCS2), мелочь: проверки NULL в comm_sms_send.

## flash_w25_runtime.c — двойная проверка записи

- Драйвер САМ читает назад и сравнивает каждую запись и каждое стирание (w25_page_program→w25_verify, w25_erase_sector→w25_verify, w25_async_poll сравнивает с источником/FF). Верхние уровни читают и сравнивают то же ещё раз: cfg_publish_pages→cfg_pages_equal + cfg_valid (полный CRC слота заново), cfg_mirror→cfg_pages_equal ×2, журнал LA_VERIFY (mem_read+memcmp+log_valid после проверенной записи), fw_block_written (синхронный путь, и так мёртвый), фото CRC копий. Оставить одну проверку — в драйвере; повторные чтения-сравнения после уже проверенной записи убрать (CRC-заголовки, нужные для выбора слота после обрыва питания, — оставить). ~300 Б и заметное время сохранения.
- 21–24 w25_async_start: проверки адреса/длины/выравнивания для внутренних вызовов.
- 17 w25_read_available: spi_ready() дважды.
- 188–218 w25_clear_status (команда 0x30 — не команда W25Q), w25_erase_chip — проверить вызовы; если нет — исходник удалить.

## usb_stm32f10x_runtime.c

- 326 usb_check_data проверяет CRC кадра → user_parse_comand 3137 считает её ещё раз для CH_USB (подтверждено). Убрать второй расчёт.
- 207–216 usb_sendstr — нужен только для user_debug (в Release пустой): под DOMOVOI_DIAG.

## sector_journal_runtime.c / sector_runtime_runtime.c — перепроверки

Асинхронный (sjr): шаг 3–4 пишет SJ_DATA по 64 Б (каждый кусок уже проверен драйвером в w25_async_poll), шаг 5 читает весь сектор ещё раз ради CRC (CRC можно считать по исходным данным в шаге 3 — уже считается в expected, повторное чтение лишнее), шаг 7/9 читает заголовок назад (драйвер уже проверил), шаг 14 читает И назначение, И SJ_DATA и memcmp + CRC — третье чтение. Синхронный (sj_replace_pages, sj_recover): sj_blank после стирания (драйвер проверил стирание), sj_pages_equal(SJ_DATA) после записи, sj_flash_crc повторно, чтение заголовка назад ×2, sj_recover в начале и в конце.
Оставить: CRC SJ_DATA в заголовке (нужен для восстановления после обрыва), проверку «сектор не изменился — не писать». Убрать повторные чтения/сравнения после проверенных драйвером записи/стирания. ~400 Б, сохранение таблицы примерно вдвое быстрее.

## fw_service_runtime.c — команда 19 обрабатывается целиком здесь

fw_service_begin возвращает false только при !store_runtime (не бывает после старта) → в user_parse_comand case 19 всё после `if (fw_service_begin(...)) break;` мёртвое: fw_start (312 Б), fw_rollback (148 Б), а fw_rollback_poll (240 Б) — уже отмечен. Оставить fw_start_rejected/fw_start_refused/fw_same_download (их зовёт fw_service). ~700 Б вместе.
- fw_service использует магические смещения store[18], store[21..24] — хрупко; при чистке дать структуре store имя полей в общем заголовке.

## Дубли сборки кадра (дополнение)

answer11 (service_runtime.c) и user_send_answer_comand делают один и тот же ответ 11; idclear (готовый success), upload_runtime (case 1/2), sc_begin, fw_service (команда 4 с 253), user_send_usbDiag/commDiag — ещё ручные SLDN+CRC. Одна функция кадра на всё. Итого дублей ~14.

## Прочие модули

- idindex_runtime.c ix_add: mem_write (драйвер уже проверил) + mem_read назад + сравнение — третья проверка. ~30 Б.
- expiry_runtime.c 28, idclear_runtime.c 19: проверки выравнивания/диапазона КОНСТАНТ mem[] во время работы — убрать (или static_assert).
- expiry_runtime.c: снимок 1 КБ в общей рабочей памяти ради чтения базы — читать по 64 Б как остальные (меньше занятость work memory).
- wiegand.c: закомментированный старый код в прерываниях; два прохода WG_process (флаг → следующий вызов) — один.
- outs.c: `out >= OUTSCOUNT` во внутренних функциях.
- adc_stm32f10x.c: регулярный канал и датчик температуры настраиваются, но adc_gettemp не вызывается (нет в BIN) — убрать настройку канала 16 и TempSensor.
- board.c: 11 вызовов GPIO_Init → запись CRL/CRH по портам (часть п.6).
- main.c:90 delay_ms(500) при старте — полсекунды; оставить, если нужно для питания модема/флеш (проверить причину, не убирать вслепую).

## Важно для задачи ПИН

user_unipassword (универсальный пароль из IMEI) НЕ ВЫЗЫВАЕТСЯ — в BIN её нет. Универсальный ПИН, о котором пишет Алексей (IMEI + 20140), сейчас не работает вовсе. Учесть в задании ПИН 4–5 цифр.

## Вопрос владельцу

- Зеркало настроек на старом месте (0x1CD400) — нужно ли ещё (откат на прошивки до A/B, старый загрузчик 0x32)? Если нет — ~1,2 КБ и вдвое меньше записей.

## РЕШЕНИЕ ВЛАДЕЛЬЦА 07.10: зеркало настроек 0x1CD400 — УБРАТЬ

Откат на прошивки до A/B и старый загрузчик 0x32 не поддерживаются. Убрать cfg_mirror, cfg_prepare/cfg_legacy_crc/cfg_equal_flash/cfg_blank_sector, состояния CA_MIRROR0/1/CA_TAIL/CA_DEVICE/CA_CONFIG, cfg_legacy_checked. Перенос со старого места при первом старте (если слотов A/B ещё нет, а на 0x1CD400 целые настройки) — оставить ОДИН раз: прочитать legacy → опубликовать слот → больше legacy не трогать. Тесты обрыва питания при сохранении настроек — зелёные.

## Структурное 1: холодный код в ASM (User/hybrid/user_fun_hot.s, 18,5 КБ)

В «горячем» ASM-файле гибрида лежат НЕ горячие функции: user_parse_comand (ASM 2226 / C 2046), command_args_valid (208/76), user_send_* (сборка ответов), user_smsreceived, user_loop, store_*, fw_*, log_add*, user_readsavedinformation, user_setdefault, user_init… Суммарно ASM 18 496 Б против 17 820 Б той же логики на C (+676), а весь user_fun в гибриде 22 880 против 21 428 в C (+1452). Скорость этих функций не нужна (разбор команды раз в секунды). Действительно горячее — CRC16/32, FNV-хеш, fast_kernels, поиск ID — уже отдельные ASM-ядра (crc_asm.s, id_hash_asm.s, fast_kernels).
Решение: гибрид = C для всей логики + ASM-ядра там, где замер показал выигрыш. user_fun_hot.s → удалить, функции из C-эталона. Ожидание: −1,4 КБ и одна копия логики вместо двух. Перед этим — замер циклов (эмулятор) user_checkIdAndControl/user_findID/ответ 12/17 до и после: не хуже.

## Структурное 2: 12 отдельных автоматов работы с флеш (~20 КБ хранилища)

Свой «старт записи → ожидание → сверка → повтор/ошибка» у: table_runtime, sjr (sector_runtime), cfg_job (memory_runtime), fw_job и log_job и store (user_fun), photo pr, fw_service, idclear, idindex, id_crc_cache, log_ring; плюс expiry/upload/service читают по-своему. Арбитраж flash_owner/flash_claim и цепочка из 15 «if active → poll» в user_storage_poll. Это следующий этап ПОСЛЕ чистки: один исполнитель флеш-операций (очередь из одной операции: read/program/erase с итогом), автоматы описывают только «что писать». Оценка −3…5 КБ и меньше мест для ошибок обрыва питания. Отдельное задание с планом архитектора.

## РЕШЕНИЯ ВЛАДЕЛЬЦА 07.10 по железу v5

- **Пульты 433 — не нужны.** Убрать RF433_readedID и источник 2 «RF433» в reader_id (команда 6 с источником 2 → ответ «ошибка параметра», как для неизвестного источника — сверить с протоколом), вход PA1 не настраивать. Поля протокола (onRF433 в настройках реле, тип ID RF433, onRF433 группы) — на проводе остаются как есть (формат пакетов не меняем), просто ничего не включают. RFID_readedID (485 RFID) — тоже никем не вызывается: убрать вместе с RF433; когда появится драйвер считывателя 485 — вернётся с ним.
- **Измерение внешнего питания VCC — убрать.** ADC1 канал 4 (PA4) из инжектированной последовательности, VCC_CHANEL/VCC_PIN/VCC_PORT из board.h, ветка VCC в adc_getinjectedvalue_mV, настройка PA4 в board_init и adc_init. Последовательность АЦП — 2 канала (IN1, IN2).
