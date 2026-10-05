# План C + ASM и памяти — 6 октября 2026

База: `d9e3dd5a0f656b1a5b9905531b66fde5e18ec9d9`, ветка `compare/c-asm`. Этот документ составлен до изменения исходников прошивки. Сборки выполняются только в %TEMP%, плата и сервер не используются, коммитов нет.

## 1. Проверенная исходная точка

IAR 9.50.1.380, Release High/Size. IarBuild умеет выгрузить базу команд (-jsondb), но -build завершается ошибкой weakly_canonical / Access denied для временного объекта. Выполнены напрямую все 76 команд неизменённой базы сборки каждого приложения, включая linker и ielftool; обе сборки успешны. Это не выдаётся за успешный запуск оболочки IarBuild -build.

| Байт | ASM | C |
|---|---:|---:|
| BIN | 92717 | 91729 |
| RO code | 85391 | 84235 |
| RO data | 7326 | 7494 |
| RW code | 516 | 516 |
| RW data (с heap/stack) | 19881 | 19881 |
| CSTACK / HEAP | 4096 / 4096 | 4096 / 4096 |
| Промежуток HEAP → fault_record | 64 | 64 |

ROM, NOR, протокол, Bootloader и DWT не меняются. Значение RW data не включает RW code, а размер BIN нельзя вычислять по сумме RAM. Свежий MODULE SUMMARY даёт user_fun 29391 / 29027 Б Flash: это отличается от приведённой в задании разбивки; общие BIN при этом совпадают.

Новый run_all и парное сравнение **пока не выполнены**: установленный Python311/Python314 недоступен песочнице, python отсутствует в доступном PATH, окружения Release/test-venv здесь нет. Загрузка переносного Python также не прошла. Архивные 67/67 и 198 сценариев не засчитываются как проверка этой работы. До снятия этой зависимости нельзя выполнить обязательную проверку этапа A и переходить к изменениям прошивки.

## 2. Выбор гибрида

Полный список содержит 231 связанных PUBLIC-функций заменяемых модулей. Суммы Flash этих модулей: ASM 54435, C 54041; независимый минимум по модулям 53851. Нижняя оценка модульного выбора относительно C: 190 Б; это не прогноз BIN и не выбор по горячим путям.

В таблице C — размер символа ENTRY LIST свежей карты. ASM-карта не публикует длину символа: значение с † — диапазон его входного FDE из .debug_frame свежего ELF, сверенный по адресу с картой. Общие ?Subroutine, литералы и выравнивание в него не входят. Таким образом, точный вклад каждой ASM-функции во Flash из одной карты **недоступен**; числа † нельзя суммировать как стоимость модуля. Для равных скоростей решение по размеру с † предварительное.

I/E — накопленные инструкции / оценка fetch из существующего Tests/asm/c_vs_asm_speed.json, а не новые замеры. В архиве некоторые функции вызывались разное число раз или с разными аргументами. Кандидат по скорости выбран по I/число вызовов; окончательное решение требует одинаковых входов и полного пути. E не является тактами платы.

Горячими считаются CRC, журнал, поиск/проверка карты, реле, серверный RX, ответы и store_snapshot. Функция, от которой прямо зависит этот путь, также требует профиля. Для прочих берётся меньший код; при неизвестном размере выбор остаётся открытым.

У user_fun есть общие хвосты ?Subroutine и условные CFI нескольких функций. Нельзя просто вырезать текст между PUBLIC-метками или включить оба объекта с одинаковыми определениями. План реализации: разнести замкнутые группы тела/хвостов на секции в существующих исходниках; оставить одну копию RAM-данных; в User/hybrid хранить только селектор и новый общий код. Если группа имеет вход в середину с нестандартным контрактом регистров, выбирать всю группу/модуль и явно показать потерю скорости. Пока безопасное разнесение не доказано. Выбор user_fun целиком ASM замедляет путь карты против C; целиком C замедляет ответы/журнал против ASM. Такой компромисс нельзя объявлять выполнением ограничения 3%.

| Функция | Модуль | C, Б | ASM, Б | I/E C | I/E ASM | Горячая | Кандидат | Причина |
|---|---|---:|---:|---:|---:|:---:|:---:|---|
| CRC32 | crc_asm | 86 | 88† | 43686/114878 | 43686/114878 | да | C | разница I <5%; предварительно по байтам |
| WG_readedID | user_fun_runtime | 36 | 36† | 3197473/7444227 | 3400523/8654079 | да | C | архивный I/вызов; перепроверить одинаковый вход |
| answer11 | service_runtime | 84 | 58† | 27692/72240 | 13760/39388 | нет | ASM | меньший входной код; проверить общие хвосты |
| crc16_calc | crc_asm | 6 | 6† | 2294213/6189473 | 1149714/3440814 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| crc16_calcInit | crc_asm | 34 | 48† | 4766836/12854536 | 2387924/7143678 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| crc16_ffInit | crc_runtime | 56 | 54† | 21149/49799 | 8094/19520 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| crc32_update | crc_asm | 48 | 182† | 12778819/34328269 | 8371788/24541312 | нет | C | меньший входной код; проверить общие хвосты |
| expiry_runtime_active | expiry_runtime | 12 | 12† | 540441548/1414650588 | 427218941/1173155449 | нет | ASM | код равен; оставить ASM |
| expiry_runtime_begin | expiry_runtime | 162 | 158† | 546702/1427046 | 436710/1193518 | нет | ASM | меньший входной код; проверить общие хвосты |
| expiry_runtime_cancel | expiry_runtime | 14 | 12† | 129/285 | 114/240 | нет | ASM | меньший входной код; проверить общие хвосты |
| expiry_runtime_poll | expiry_runtime | 524 | 494† | 543722430/1423313342 | 429729420/1180273072 | нет | ASM | меньший входной код; проверить общие хвосты |
| fast_all_ff | fast_kernels | 74 | 62† | 943836/2189018 | 642430/1500608 | нет | ASM | меньший входной код; проверить общие хвосты |
| fast_fnv10 | fast_kernels | 68 | 68† | 2346/4692 | 2346/4692 | нет | ASM | код равен; оставить ASM |
| fast_gf2_apply | fast_kernels | 30 | 28† | 167342/381008 | 94948/228104 | нет | ASM | меньший входной код; проверить общие хвосты |
| fast_sldn | fast_kernels | 64 | 52† | 39330/84240 | 33662/78494 | нет | ASM | меньший входной код; проверить общие хвосты |
| flash_claim | memory_runtime | 26 | 22† | 101594/224536 | 53466/116302 | нет | ASM | меньший входной код; проверить общие хвосты |
| flash_read_allowed | memory_runtime | 12 | 12† | 50715/111573 | 60858/142002 | нет | ASM | код равен; оставить ASM |
| flash_release | memory_runtime | 20 | 16† | 23173/48923 | 20606/46376 | нет | ASM | меньший входной код; проверить общие хвосты |
| fw_abort | user_fun_runtime | 84 | 84† | 5683/14439 | 5686/14612 | нет | ASM | код равен; оставить ASM |
| fw_end | user_fun_runtime | 42 | 42† | 800/2038 | 593/1535 | нет | ASM | код равен; оставить ASM |
| fw_log | user_fun_runtime | 22 | 22† | 4361/10905 | 4224/10826 | нет | ASM | код равен; оставить ASM |
| fw_same_download | user_fun_runtime | 84 | 84† | 395/931 | 395/883 | нет | ASM | код равен; оставить ASM |
| fw_service_active | fw_service_runtime | 34 | 38† | 153469062/405676384 | 130320788/356215492 | нет | C | меньший входной код; проверить общие хвосты |
| fw_service_begin | fw_service_runtime | 390 | 366† | 800188/2115224 | 642630/1751710 | нет | ASM | меньший входной код; проверить общие хвосты |
| fw_service_busy | fw_service_runtime | 8 | 8† | 16116/48348 | 16116/37604 | нет | ASM | код равен; оставить ASM |
| fw_service_cancel | fw_service_runtime | 52 | 58† | 7082/16654 | 7506/17690 | нет | C | меньший входной код; проверить общие хвосты |
| fw_service_config_result | fw_service_runtime | 38 | 36† | 449416/1048926 | 449228/1048554 | нет | ASM | меньший входной код; проверить общие хвосты |
| fw_service_poll | fw_service_runtime | 1110 | 1174† | 1578904/4181912 | 771348/2193488 | нет | C | меньший входной код; проверить общие хвосты |
| fw_service_quiet | fw_service_runtime | 26 | 24† | 240/656 | 232/564 | нет | ASM | меньший входной код; проверить общие хвосты |
| fw_start_refused | user_fun_runtime | 74 | 74† | 2280/5776 | 2280/5624 | нет | ASM | код равен; оставить ASM |
| fw_start_rejected | user_fun_runtime | 78 | 78† | 1157/2625 | 1157/2625 | нет | ASM | код равен; оставить ASM |
| gsm_check_data | user_fun_runtime | 14 | 14† | 1/3 | 1/3 | нет | ASM | код равен; оставить ASM |
| id_time_end | user_fun_runtime | 72 | 72† | 1341/3379 | 1341/3283 | нет | ASM | код равен; оставить ASM |
| id_watch_end | user_fun_runtime | 22 | 22† | 16/46 | 16/46 | нет | ASM | код равен; оставить ASM |
| idclear_runtime_active | idclear_runtime | 10 | 12† | 250271/624917 | 312302/687058 | нет | C | меньший входной код; проверить общие хвосты |
| idclear_runtime_begin | idclear_runtime | 182 | 160† | 10077/24997 | 8294/20878 | нет | ASM | меньший входной код; проверить общие хвосты |
| idclear_runtime_cancel | idclear_runtime | 20 | 14† | 124/280 | 94/210 | нет | ASM | меньший входной код; проверить общие хвосты |
| idclear_runtime_poll | idclear_runtime | 190 | 190† | 10883/25205 | 9891/23073 | нет | ASM | код равен; оставить ASM |
| idclear_runtime_quiet | idclear_runtime | 16 | 14† | 90/232 | 89/197 | нет | ASM | меньший входной код; проверить общие хвосты |
| idindex_find | idindex_runtime | 118 | 118† | 208/496 | 208/496 | нет | ASM | код равен; оставить ASM |
| idindex_hash | id_hash_asm | 92 | 84† | 1025/2075 | 1000/2050 | нет | ASM | меньший входной код; проверить общие хвосты |
| idindex_poll | idindex_runtime | 424 | 490† | 11298222/25981618 | 10855220/25921098 | нет | C | меньший входной код; проверить общие хвосты |
| idindex_runtime_active | idindex_runtime | 10 | 14† | 293630/727592 | 365957/944573 | нет | C | меньший входной код; проверить общие хвосты |
| idindex_runtime_suspend | idindex_runtime | 8 | 10† | 132/330 | 132/330 | нет | C | меньший входной код; проверить общие хвосты |
| idindex_scanned | idindex_runtime | 54 | 54† | 1666/4188 | 1667/4205 | нет | ASM | код равен; оставить ASM |
| log_addControlEvent | user_fun_runtime | 40 | 40† | 588/1504 | 287/719 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_addEvent | user_fun_runtime | 232 | 228† | 1911/4903 | 1282/3382 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_addPhotoEvent | user_fun_runtime | 36 | 36† | 237/601 | 100/246 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_addSystemEvent | user_fun_runtime | 26 | 26† | 1398/3594 | 576/1464 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_addSystemEventID | user_fun_runtime | 34 | 34† | 1168/2992 | 566/1422 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_addUpdateEvent | user_fun_runtime | 82 | 82† | 496/1268 | 222/558 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| log_blank | user_fun_runtime | 28 | 28† | 9943/21903 | 9943/21903 | нет | ASM | код равен; оставить ASM |
| log_checkPending | user_fun_runtime | 32 | 32† | 4/12 | 4/10 | нет | ASM | код равен; оставить ASM |
| log_clearLogBuffer | user_fun_runtime | 104 | 106† | 2074/5078 | 2074/5122 | нет | C | меньший входной код; проверить общие хвосты |
| log_flushPending | user_fun_runtime | 58 | 58† | 20/52 | 20/52 | нет | ASM | код равен; оставить ASM |
| log_getRecordsCount | user_fun_runtime | 148 | 150† | 1476/3412 | 1480/3448 | нет | C | меньший входной код; проверить общие хвосты |
| log_setTime | user_fun_runtime | 90 | 90† | 36/94 | 36/94 | нет | ASM | код равен; оставить ASM |
| log_valid | user_fun_runtime | 118 | 118† | 279870/747718 | 151566/438838 | нет | ASM | код равен; оставить ASM |
| mem_config_begin | memory_runtime | 190 | 176† | 347481/901209 | 28363/77871 | нет | ASM | меньший входной код; проверить общие хвосты |
| mem_config_poll | memory_runtime | 632 | 626† | 10352913/27237989 | 8059343/22065869 | нет | ASM | меньший входной код; проверить общие хвосты |
| mem_config_reset | memory_runtime | 28 | 28† | 3421/8023 | 3421/8023 | нет | ASM | код равен; оставить ASM |
| mem_config_result | memory_runtime | 10 | 10† | 211893/635679 | 211893/635679 | нет | ASM | код равен; оставить ASM |
| mem_config_snapshot | memory_runtime | 8 | 8† | 6/16 | 6/16 | нет | ASM | код равен; оставить ASM |
| mem_erase_sector | memory_runtime | 50 | 50† | 25635/63261 | 25725/63531 | нет | ASM | код равен; оставить ASM |
| mem_patch | memory_runtime | 12 | 12† | 521595/1318629 | 394121/1016115 | нет | ASM | код равен; оставить ASM |
| mem_patch2 | memory_runtime | 148 | 148† | 2613871/6742507 | 1612482/4260650 | нет | ASM | код равен; оставить ASM |
| mem_read | memory_runtime | 32 | 32† | 806524/1986746 | 826124/2084702 | нет | ASM | код равен; оставить ASM |
| mem_recover | memory_runtime | 16 | 16† | 134/330 | 134/330 | нет | ASM | код равен; оставить ASM |
| mem_replace_pages | memory_runtime | 42 | 42† | 1043180/2637220 | 788273/2032277 | нет | ASM | код равен; оставить ASM |
| mem_tables_ready | memory_runtime | 52 | 44† | 253329/638399 | 273579/699149 | нет | ASM | меньший входной код; проверить общие хвосты |
| mem_write | memory_runtime | 58 | 58† | 397/979 | 401/1007 | нет | ASM | код равен; оставить ASM |
| photo_cleanup_active | photo_runtime | 48 | 48† | 920/2300 | 920/2320 | нет | ASM | код равен; оставить ASM |
| photo_confirmed_offset | photo_runtime | 142 | 142† | 172/420 | 172/428 | нет | ASM | код равен; оставить ASM |
| photo_connection_lost | photo_runtime | 46 | 42† | 87/213 | 85/209 | нет | ASM | меньший входной код; проверить общие хвосты |
| photo_poll | photo_runtime | 4 | 4† | 635826/1493468 | 617702/1464188 | нет | ASM | код равен; оставить ASM |
| photo_recover | photo_runtime | 96 | 96† | 955/2385 | 955/2385 | нет | ASM | код равен; оставить ASM |
| photo_request_answer | photo_runtime | 34 | 34† | 1783/4455 | 1705/4299 | нет | ASM | код равен; оставить ASM |
| photo_start | photo_runtime | 4 | 4† | 63891/159305 | 63527/158699 | нет | ASM | код равен; оставить ASM |
| photo_storage_busy | photo_runtime | 14 | 14† | 165558365/440582709 | 138552906/381380166 | нет | ASM | код равен; оставить ASM |
| photo_storage_cancel | photo_runtime | 58 | 58† | 668/1766 | 668/1794 | нет | ASM | код равен; оставить ASM |
| photo_storage_end | photo_runtime | 22 | 22† | 170352/425880 | 170352/425880 | нет | ASM | код равен; оставить ASM |
| photo_storage_poll | photo_runtime | 3286 | 3294† | 4133650/10800422 | 3130957/8512567 | нет | C | меньший входной код; проверить общие хвосты |
| port_check_data | user_fun_runtime | 44 | 44† | 4272528/11191660 | 3670912/9873740 | нет | ASM | код равен; оставить ASM |
| reply_send | service_runtime | 52 | 44† | 49090/120230 | 37960/96780 | нет | ASM | меньший входной код; проверить общие хвосты |
| rs485_baud | photo_runtime | 52 | 52† | 847/2299 | 847/2299 | нет | ASM | код равен; оставить ASM |
| rs485_init | photo_runtime | 110 | 110† | 2001/4803 | 2001/4803 | нет | ASM | код равен; оставить ASM |
| rs485_set_config | photo_runtime | 116 | 116† | 3361/8609 | 901/2251 | нет | ASM | код равен; оставить ASM |
| server_rx_begin_pass | user_fun_runtime | 76 | 76† | 308946/827158 | 164130/481180 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| server_rx_process | user_fun_runtime | 126 | 126† | 308523/826123 | 163804/480382 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| service_crc_active | service_runtime | 8 | 6† | 257959400/634814668 | 190130766/489835384 | нет | ASM | меньший входной код; проверить общие хвосты |
| service_crc_begin_config | service_runtime | 10 | 12† | 108920/271092 | 23218/58234 | нет | C | меньший входной код; проверить общие хвосты |
| service_crc_begin_id | service_runtime | 10 | 12† | 6092/15168 | 1402/3526 | нет | C | меньший входной код; проверить общие хвосты |
| service_crc_begin_pack | service_runtime | 10 | 12† | 27151/67551 | 6021/15125 | нет | C | меньший входной код; проверить общие хвосты |
| service_crc_cancel | service_runtime | 26 | 24† | 5731/13131 | 5636/13250 | нет | ASM | меньший входной код; проверить общие хвосты |
| service_crc_poll | service_runtime | 392 | 394† | 2063428/5423480 | 1150562/3253204 | нет | C | меньший входной код; проверить общие хвосты |
| service_crc_quiet | service_runtime | 20 | 14† | 333/857 | 327/779 | нет | ASM | меньший входной код; проверить общие хвосты |
| sj_destination | sector_journal_runtime | 56 | 56† | 90558/204966 | 90558/204966 | нет | ASM | код равен; оставить ASM |
| sj_io_erase | memory_runtime | 36 | 36† | 292/684 | 292/684 | нет | ASM | код равен; оставить ASM |
| sj_io_read | memory_runtime | 8 | 8† | 48491/115515 | 48491/115515 | нет | ASM | код равен; оставить ASM |
| sj_io_service | memory_runtime | 4 | 4† | 643/3215 | 643/3215 | нет | ASM | код равен; оставить ASM |
| sj_io_write | memory_runtime | 18 | 18† | 2539/6183 | 2539/6183 | нет | ASM | код равен; оставить ASM |
| sj_recover | sector_journal_runtime | 214 | 214† | 553615/1394475 | 442611/1147843 | нет | ASM | код равен; оставить ASM |
| sj_replace_pages | sector_journal_runtime | 350 | 350† | 1043206/2637278 | 788299/2032335 | нет | ASM | код равен; оставить ASM |
| sj_runtime_begin | sector_runtime_runtime | 92 | 94† | 32483/82259 | 30155/70391 | нет | C | меньший входной код; проверить общие хвосты |
| sj_runtime_blocked | sector_runtime_runtime | 28 | 28† | 71497/173639 | 71497/173639 | нет | ASM | код равен; оставить ASM |
| sj_runtime_cancel | sector_runtime_runtime | 12 | 12† | 5040/12600 | 5040/12600 | нет | ASM | код равен; оставить ASM |
| sj_runtime_poll | sector_runtime_runtime | 656 | 660† | 5603196/14838294 | 4411702/12205148 | нет | C | меньший входной код; проверить общие хвосты |
| sj_runtime_reset | sector_runtime_runtime | 16 | 16† | 190/494 | 190/494 | нет | ASM | код равен; оставить ASM |
| sj_span | sector_journal_runtime | 50 | 50† | 28912/66226 | 28912/66226 | нет | ASM | код равен; оставить ASM |
| spi_init | spi_stm32f10x_runtime | 154 | 154† | 544/1284 | 544/1284 | нет | ASM | код равен; оставить ASM |
| spi_initCS | spi_stm32f10x_runtime | 54 | 54† | 302/722 | 302/722 | нет | ASM | код равен; оставить ASM |
| spi_read | spi_stm32f10x_runtime | 40 | 40† | 310/716 | 310/716 | нет | ASM | код равен; оставить ASM |
| spi_read_byte | spi_stm32f10x_runtime | 40 | 40† | 94/222 | 94/222 | нет | ASM | код равен; оставить ASM |
| spi_ready | spi_stm32f10x_runtime | 14 | 14† | 132612/309428 | 132612/309428 | нет | ASM | код равен; оставить ASM |
| spi_send | spi_stm32f10x_runtime | 36 | 36† | 305/709 | 305/709 | нет | ASM | код равен; оставить ASM |
| spi_send_byte | spi_stm32f10x_runtime | 40 | 40† | 94/222 | 94/222 | нет | ASM | код равен; оставить ASM |
| store_snapshot | user_fun_runtime | 38 | 38† | 1361733/3646783 | 564560/1687424 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| store_upload_request | user_fun_runtime | 94 | 94† | 1301/3125 | 1301/3125 | нет | ASM | код равен; оставить ASM |
| timers_user_handle | user_fun_runtime | 14 | 14† | 95/249 | 95/251 | нет | ASM | код равен; оставить ASM |
| upload_ack | user_fun_runtime | 102 | 102† | 921/2441 | 921/2441 | нет | ASM | код равен; оставить ASM |
| upload_finish | user_fun_runtime | 172 | 172† | 282/768 | 282/768 | нет | ASM | код равен; оставить ASM |
| upload_index | user_fun_runtime | 42 | 42† | 320/870 | 320/870 | нет | ASM | код равен; оставить ASM |
| upload_runtime_active | upload_runtime | 12 | 14† | 252510/629688 | 315373/818277 | нет | C | меньший входной код; проверить общие хвосты |
| upload_runtime_begin | upload_runtime | 182 | 200† | 12437/29751 | 12665/30663 | нет | C | меньший входной код; проверить общие хвосты |
| upload_runtime_cancel | upload_runtime | 40 | 36† | 4020/9160 | 3982/9122 | нет | ASM | меньший входной код; проверить общие хвосты |
| upload_runtime_poll | upload_runtime | 726 | 736† | 450062/1181702 | 256830/716782 | нет | C | меньший входной код; проверить общие хвосты |
| upload_runtime_quiet | upload_runtime | 22 | 22† | 1216/2800 | 1206/2858 | нет | ASM | код равен; оставить ASM |
| usb_bus_error_seen | usb_stm32f10x_runtime | 4 | 4† | 5/11 | 5/11 | нет | ASM | код равен; оставить ASM |
| usb_check_data | usb_stm32f10x_runtime | 432 | 406† | 55/149 | 54/142 | нет | ASM | меньший входной код; проверить общие хвосты |
| usb_clear_buffer | usb_stm32f10x_runtime | 40 | 40† | 34/82 | 34/74 | нет | ASM | код равен; оставить ASM |
| usb_data_reading | user_fun_runtime | 54 | 54† | 6/14 | 6/14 | нет | ASM | код равен; оставить ASM |
| usb_datareceived | usb_stm32f10x_runtime | 232 | 232† | 86/218 | 81/203 | нет | ASM | код равен; оставить ASM |
| usb_diag_fill | usb_stm32f10x_runtime | 124 | 124† | 130/298 | 130/298 | нет | ASM | код равен; оставить ASM |
| usb_diag_fill_addr | usb_stm32f10x_runtime | 34 | 34† | 18/40 | 18/40 | нет | ASM | код равен; оставить ASM |
| usb_diag_fill_errors | usb_stm32f10x_runtime | 18 | 18† | 10/22 | 10/24 | нет | ASM | код равен; оставить ASM |
| usb_diag_fill_regs | usb_stm32f10x_runtime | 36 | 36† | 102/242 | 102/258 | нет | ASM | код равен; оставить ASM |
| usb_get_state | usb_stm32f10x_runtime | 6 | 6† | 3/7 | 3/7 | нет | ASM | код равен; оставить ASM |
| usb_in_done | usb_stm32f10x_runtime | 20 | 20† | 11/25 | 11/25 | нет | ASM | код равен; оставить ASM |
| usb_init | usb_stm32f10x_runtime | 44 | 44† | 14/38 | 14/38 | нет | ASM | код равен; оставить ASM |
| usb_request_recovery | usb_stm32f10x_runtime | 208 | 208† | 171/409 | 171/407 | нет | ASM | код равен; оставить ASM |
| usb_send | usb_stm32f10x_runtime | 44 | 44† | 111/279 | 111/279 | нет | ASM | код равен; оставить ASM |
| usb_sendstr | usb_stm32f10x_runtime | 56 | 56† | 229/559 | 217/537 | нет | ASM | код равен; оставить ASM |
| usb_sof_seen | usb_stm32f10x_runtime | 10 | 10† | 4/10 | 4/10 | нет | ASM | код равен; оставить ASM |
| usb_timer1ms | usb_stm32f10x_runtime | 180 | 180† | 136/356 | 136/360 | нет | ASM | код равен; оставить ASM |
| usb_try_send | usb_stm32f10x_runtime | 162 | 164† | 520794/1183384 | 336427/812863 | нет | C | меньший входной код; проверить общие хвосты |
| user_activateOut | user_fun_runtime | 468 | 464† | 4704919/11463865 | 5309141/13677261 | да | C | архивный I/вызов; перепроверить одинаковый вход |
| user_addIdToTempBuffer | user_fun_runtime | 242 | 240† | 339/851 | 328/826 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_calcCRC16InFlash | user_fun_runtime | 68 | 64† | 1401/3751 | 761/2221 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_checkFirmware | user_fun_runtime | 90 | 90† | 144/362 | 146/368 | нет | ASM | код равен; оставить ASM |
| user_checkIdAndControl | user_fun_runtime | 656 | 660† | 11013622/26662378 | 12323896/31695534 | да | C | архивный I/вызов; перепроверить одинаковый вход |
| user_checkIdTimeEnd | user_fun_runtime | 4 | 212† | 242/584 | 236/572 | нет | C | меньший входной код; проверить общие хвосты |
| user_checkShedule | user_fun_runtime | 126 | 128† | 48/120 | 55/135 | нет | C | меньший входной код; проверить общие хвосты |
| user_clearMemBlock | user_fun_runtime | 86 | 86† | 25264/63102 | 25326/63292 | нет | ASM | код равен; оставить ASM |
| user_clearTempIdBuffer | user_fun_runtime | 80 | 80† | 1452/3592 | 1478/3670 | нет | ASM | код равен; оставить ASM |
| user_comm_acquire | user_fun_runtime | 28 | 28† | 13905/32445 | 13905/32445 | нет | ASM | код равен; оставить ASM |
| user_comm_balance | user_fun_runtime | 58 | 58† | 11/29 | 11/29 | нет | ASM | код равен; оставить ASM |
| user_comm_boot | user_fun_runtime | 82 | 82† | 13074/32108 | 13094/31932 | нет | ASM | код равен; оставить ASM |
| user_comm_busy | user_fun_runtime | 22 | 22† | 11/33 | 11/33 | нет | ASM | код равен; оставить ASM |
| user_comm_link | user_fun_runtime | 36 | 36† | 253/649 | 116/294 | нет | ASM | код равен; оставить ASM |
| user_comm_local_time | user_fun_runtime | 22 | 22† | 6/16 | 6/16 | нет | ASM | код равен; оставить ASM |
| user_comm_low_signal | user_fun_runtime | 6 | 6† | 569/1395 | 418/1004 | нет | ASM | код равен; оставить ASM |
| user_comm_poll | user_comm_runtime | 68 | 68† | 321/823 | 309/799 | нет | ASM | код равен; оставить ASM |
| user_comm_rx | user_fun_runtime | 50 | 50† | 308962/827194 | 164146/481216 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_comm_rx_capacity | user_fun_runtime | 26 | 26† | 27/69 | 27/69 | да | ASM | разница I <5%; предварительно по байтам |
| user_comm_rx_done | user_fun_runtime | 18 | 18† | 308501/826061 | 163782/480320 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_comm_server | user_fun_runtime | 26 | 26† | 14/38 | 14/38 | нет | ASM | код равен; оставить ASM |
| user_comm_session | user_fun_runtime | 98 | 98† | 2053/5247 | 2040/5196 | нет | ASM | код равен; оставить ASM |
| user_comm_sms | user_comm_runtime | 4 | 4† | 2964/6896 | 2964/6896 | нет | ASM | код равен; оставить ASM |
| user_comm_start | user_comm_runtime | 66 | 66† | 8482/20572 | 8371/20187 | нет | ASM | код равен; оставить ASM |
| user_comm_ussd | user_fun_runtime | 6 | 6† | 2/6 | 2/6 | нет | ASM | код равен; оставить ASM |
| user_comm_volte | user_fun_runtime | 10 | 10† | 3/9 | 3/9 | нет | ASM | код равен; оставить ASM |
| user_dataconnection | user_fun_runtime | 172 | 172† | 432/1068 | 335/831 | нет | ASM | код равен; оставить ASM |
| user_dayIsHoliday | user_fun_runtime | 92 | 86† | 13976/33064 | 11576/28934 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_debug | user_fun_runtime | 18 | 18† | 5/15 | 5/15 | нет | ASM | код равен; оставить ASM |
| user_factoryReset | user_fun_runtime | 120 | 120† | 716090/1836684 | 431548/1143136 | нет | ASM | код равен; оставить ASM |
| user_findID | user_fun_runtime | 180 | 178† | 11024173/26688535 | 12335822/31726478 | да | C | архивный I/вызов; перепроверить одинаковый вход |
| user_findTempID | user_fun_runtime | 68 | 68† | 160/380 | 160/380 | нет | ASM | код равен; оставить ASM |
| user_getFWblockFromServer | user_fun_runtime | 124 | 128† | 35178/94710 | 37884/97416 | нет | C | меньший входной код; проверить общие хвосты |
| user_getGroup | user_fun_runtime | 56 | 12† | 994/2382 | 1028/2530 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_getNumbersCount | user_fun_runtime | 166 | 166† | 15340585/32733087 | 15343761/35899587 | нет | ASM | код равен; оставить ASM |
| user_getPhotoCount | user_fun_runtime | 202 | 202† | 1596/3996 | 1596/4012 | нет | ASM | код равен; оставить ASM |
| user_getShedule | user_fun_runtime | 56 | 12† | 347/831 | 363/891 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_getTempIDRecordsCount | user_fun_runtime | 96 | 96† | 488/1208 | 496/1232 | нет | ASM | код равен; оставить ASM |
| user_getweakday | user_fun_runtime | 32 | 32† | 12/28 | 12/28 | нет | ASM | код равен; оставить ASM |
| user_incomingcall | user_fun_runtime | 68 | 68† | 1405/3373 | 1244/2964 | нет | ASM | код равен; оставить ASM |
| user_indication | user_fun_runtime | 36 | 36† | 38/90 | 38/90 | нет | ASM | код равен; оставить ASM |
| user_init | user_fun_runtime | 218 | 218† | 15606975/33559005 | 15370310/36087146 | нет | ASM | код равен; оставить ASM |
| user_initvariables | user_fun_runtime | 120 | 120† | 15400802/32935800 | 15342618/35938912 | нет | ASM | код равен; оставить ASM |
| user_last10digitsnumber | user_fun_runtime | 24 | 24† | 192/448 | 144/360 | нет | ASM | код равен; оставить ASM |
| user_loop | user_fun_runtime | 960 | 966† | 196/516 | 194/508 | нет | C | меньший входной код; проверить общие хвосты |
| user_parse_comand | user_fun_runtime | 2208 | 2238† | 3199610/8401164 | 2442627/6696761 | нет | C | меньший входной код; проверить общие хвосты |
| user_readconfig | user_fun_runtime | 74 | 74† | 961900/2551364 | 492668/1395468 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_readinputs | user_fun_runtime | 190 | 190† | 14/36 | 14/36 | нет | ASM | код равен; оставить ASM |
| user_readsavedinformation | user_fun_runtime | 250 | 250† | 226526/611164 | 114176/341474 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_saveindex | user_fun_runtime | 24 | 24† | 88/220 | 88/220 | нет | ASM | код равен; оставить ASM |
| user_savereset | user_fun_runtime | 70 | 70† | 739/1871 | 757/1943 | нет | ASM | код равен; оставить ASM |
| user_send_answer_comand | user_fun_runtime | 140 | 140† | 339/893 | 206/576 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_anyData | user_fun_runtime | 178 | 178† | 1156/2780 | 1064/2558 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_configToConfigurator | user_fun_runtime | 138 | 138† | 13109/35079 | 6413/18855 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_crcMemoryID | user_fun_runtime | 224 | 234† | 6157/15353 | 1422/3576 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_crcPack | user_fun_runtime | 204 | 208† | 27431/68191 | 6101/15325 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_identPackToConfigurator | user_fun_runtime | 106 | 106† | 561/1489 | 322/906 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_logRecords | user_fun_runtime | 350 | 362† | 81262/212134 | 49760/136614 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_readedIDtoServer | user_fun_runtime | 26 | 26† | 59/153 | 45/115 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_registrationData | user_fun_runtime | 196 | 196† | 1015/2705 | 566/1622 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_smsToServer | user_fun_runtime | 122 | 116† | 12/28 | 12/32 | да | ASM | разница I <5%; предварительно по байтам |
| user_send_statusPack | user_fun_runtime | 190 | 190† | 526/1394 | 310/870 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_statusPackSecond | user_fun_runtime | 158 | 158† | 609/1617 | 346/984 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_statusPackToConfigurator | user_fun_runtime | 266 | 262† | 832/2218 | 469/1343 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_statusUpdateFW | user_fun_runtime | 124 | 124† | 402/1054 | 256/700 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_tempIDs | user_fun_runtime | 408 | 412† | 19820/49942 | 16271/42431 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_send_timeshtamp | user_fun_runtime | 110 | 110† | 310/816 | 187/519 | да | ASM | архивный I/вызов; перепроверить одинаковый вход |
| user_sendsms | user_fun_runtime | 44 | 44† | 1837/4349 | 1675/3933 | нет | ASM | код равен; оставить ASM |
| user_setdefault | user_fun_runtime | 162 | 162† | 1147230/3006374 | 602242/1642178 | нет | ASM | код равен; оставить ASM |
| user_smsreceived | user_fun_runtime | 640 | 640† | 1216/2890 | 1048/2454 | нет | ASM | код равен; оставить ASM |
| user_storage_background_ready | user_fun_runtime | 50 | 50† | 42547/109143 | 38847/103593 | нет | ASM | код равен; оставить ASM |
| user_storage_command_ready | user_fun_runtime | 42 | 40† | 99130/253322 | 94045/250405 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_storage_flash_ready | user_fun_runtime | 26 | 18† | 20065/47781 | 13858/35094 | нет | ASM | меньший входной код; проверить общие хвосты |
| user_storage_poll | user_fun_runtime | 816 | 838† | 2120061511/5566183633 | 1619693928/4452198314 | нет | C | меньший входной код; проверить общие хвосты |
| user_updateID | user_fun_runtime | 122 | 124† | 24602/59964 | 25114/63550 | нет | C | меньший входной код; проверить общие хвосты |
| user_wait_stable_power | user_fun_runtime | 2 | 2† | 1/3 | 1/3 | нет | ASM | код равен; оставить ASM |
| user_writeConfigAndDevice | user_fun_runtime | 52 | 52† | 1693360/4436468 | 885638/2412546 | нет | ASM | код равен; оставить ASM |
| w25_async_active | flash_w25_runtime | 12 | 12† | 288/720 | 288/720 | нет | ASM | код равен; оставить ASM |
| w25_async_poll | flash_w25_runtime | 158 | 158† | 4036036/9291126 | 4036036/9233850 | нет | ASM | код равен; оставить ASM |
| w25_async_start | flash_w25_runtime | 196 | 196† | 37932/96318 | 37932/93336 | нет | ASM | код равен; оставить ASM |
| w25_erase_sector | flash_w25_runtime | 98 | 98† | 38224/89230 | 38224/89206 | нет | ASM | код равен; оставить ASM |
| w25_read_available | flash_w25_runtime | 30 | 30† | 259946/644214 | 259946/621610 | нет | ASM | код равен; оставить ASM |
| w25_read_data | flash_w25_runtime | 88 | 88† | 498871/1260035 | 498871/1218891 | нет | ASM | код равен; оставить ASM |
| w25_read_devID | flash_w25_runtime | 62 | 62† | 365/935 | 365/895 | нет | ASM | код равен; оставить ASM |
| w25_read_status | flash_w25_runtime | 26 | 26† | 265536/663840 | 265536/619584 | нет | ASM | код равен; оставить ASM |
| w25_write_data | flash_w25_runtime | 102 | 102† | 16/36 | 16/36 | нет | ASM | код равен; оставить ASM |

Общие драйверы Modules, USB/ST и main используются прежние: у них нет отдельного C/ASM-выбора. Финальный селектор обязан сохранить все имена, runtime_layout и поведение; run_all HYBRID и HYBRID/ASM должны проверять также полный путь карта → GPIO и сервер → comm_tx_commit, а не только ядра.

## 3. Владельцы рабочей памяти

В источниках 17 мест malloc и **ещё calloc фото-контекста 180 Б**. Все они должны быть учтены, иначе basic heap останется связанным. Размеры структур ниже проверены RT_ASSERT или константой ASM photo_alloc. Выравнивание области — 8. IRQ не вызывает эти выделения; это необходимо ещё подтвердить полным графом.

| Владелец | Полезный размер, Б | Получение / освобождение | Одновременная жизнь и доказательство |
|---|---:|---|---|
| expiry | 1092 | expiry_runtime_begin / expiry_release после drain/cancel | photo-context может оставаться. Команды требуют user_storage_command_ready; FO_EXPIRY запрещает таблицы |
| FW service context + reserve | 128 + 1024 | fw_service_begin / fw_service_cancel | photo-context разрешён. В фазе handoff reserve отдаётся **до** store_snapshot; context ещё жив |
| FW service context + config | 128 + 1024 | handoff, phase 8 / завершение store и ответ | Это допустимая пара, даже когда fw_service_context временно NULL и FO_FWSERVICE снят |
| ID clear | 56 | idclear_runtime_begin / idclear_release | Требует command_ready; ID clear release отдельно от Flash release в автомате |
| config snapshot | 1024 | mem_config_begin / mem_config_reset | result==1 удерживает память, result 2/3 ещё не освобождает её. FW context может ждать ответа |
| CRC service | 312 | sc_begin / service_crc_cancel | Требует flash_ready и не допускает cfg result==1; собственный packet[212] до успешного reply_send |
| upload context + packet | 36 + 1040 | upload_runtime_begin / upload_runtime_cancel | Резерв до ACK. Packet остаётся до передачи/отмены, ACK может требовать config snapshot после освобождения upload |
| photo-context | 180, calloc | photo_alloc / idle !stage && !pending | Отдельная долговременная область: pending upload живёт при задачах таблиц/FW. Она не может быть частью union Flash-owner |
| photo USB packet | 534 | photo_send / сразу после usb_send или ошибки чтения | context жив; ответ должен быть скопирован транспортом до release. Может потребоваться при другой фоновой задаче |
| user anyData | size + 11 | user_send_anyData / user_comm_free | Сервер использует comm_tx lease, USB — общий кратковременный ответ. Реальные callers передают короткие структуры |
| user CRC IDs | 211 или 14 | user_send_crcMemoryID / user_comm_free | Legacy fallback, cch != server. Не считать освобождённым до usb_send |
| user config | 523 (config 512 + 11) | user_send_configToConfigurator / user_comm_free | Не 1024. Reply нужен до копирования в USB |
| user legacy log | до 1040 | user_send_logRecords / user_comm_free | 32 × 32 + 16; провод 1038. При runtime=1 работает upload owner |
| user legacy temp IDs | до 1016 | user_send_tempIDs / user_comm_free | 100 × 10 + 16; провод 1014. При runtime=1 работает upload owner |
| SMS command text | до 2028 при полном 2048-Б кадре | user_parse_comand(11) / free после user_sendsms | len <=2040, text_size=len−13, плюс NUL. Протокольную копию нельзя незаметно ограничить 160 Б |
| FW block | 1024 | fw_async_begin / после durable write, error/cancel | FO_FWBLOCK; текущие данные удерживаются до завершения NOR, а ответ может остаться в phase FA_REPLY |

Ссылки на доказательства: User/c/user_fun_runtime.c:101,3077,3304,4615,4646,5276; memory_runtime.c:357,362; fw_service_runtime.c:38,127; photo_runtime.c:50,633,732; upload_runtime.c:22,31. ASM использует те же фазы и смещения; проверка совпадения — тесты модели, пока не запущены.

Предлагаемая область: **2224 Б**, включая 184 Б под выровненный photo-context, 2032 Б union и 8 Б учёта владельцев. В union крупнейший кратковременный SMS — 2028 Б; фоновые наборы с буфером photo 534 Б: FW 1152+534=1686, expiry 1092+534=1626, upload 1076+534=1610. Округлённый максимум 2032. Это условный проектный размер, а не доказанный максимум модели.

Условие union: SMS/legacy USB-команда не начинается при живой Flash-задаче в runtime=1; это следует из gate store_command_allowed в парсере. Нельзя доказывать его одним flash_claim: некоторые callers игнорируют результат claim, а cfg snapshot имеет отдельный result. Необходимо проверить lifetime result 2/3, все фоновые callers и legacy runtime=0. Если допустимая команда пересекается с удерживаемым snapshot, размер должен вырасти до суммы, а прежний успешный ответ не должен превращаться в отказ.

Тесты области: таблица всех владельцев; для каждой запрещённой пары A/B обе очередности acquire, проверка отказа и canary первой области; для каждой разрешённой пары разные canary, независимый release; отдельно FW handoff/context+config, USB reset в pending, cancellation с активной NOR, reply retry и ACK. Старые тесты, вызывающие __basic_malloc непосредственно и проверяющие заголовок heap, требуют переноса на явное владение: оставить allocator ради этих фикстур нельзя. Их проверки OOM/NOR/wire сохраняются.

## 4. Оценка этапа C до реализации

| Приём | Предварительная экономия RAM, Б | Основание / условие |
|---|---:|---|
| HEAP 4096 → область 2224 | 1872 | 4096 − 2224; только после доказательства lifetime |
| Удалить allocator RW | 8 | heap0.o RW data; Flash-код heap0 ещё 368 Б, замена потребует собственного кода владения |
| Стек 4096 → 3296 | 800 | ceil8((2376+256)×1.25); **не принимается до полного .suc** |
| Кольцо 1280 → 1040 | 240 | Ответ log 1038 Б, один слот кольца пустой, 1039 минимум. 1040 сохраняет возможность одного полного пакета, но уменьшает очередь: нужна проверка backpressure |
| Кольцо 1280 → 256 | 0 принимаемых | Не работает с атомарной публикацией 1038 Б; требуется writer с владением до последнего PMA-пакета |
| Const таблицы/строки | 0 подтверждённых | net, handlers, таблицы CRC/USB уже const; конфигурация, pending и RAM writer изменяемы |

Прогноз свободного промежутка только C: 64+1872+8+800+240 = **2984 Б**, на 88 Б ниже 3072. Выключение debug из D потенциально убирает ещё tempStrBuffer[128], тогда **3112 Б**, запас всего 40 Б над целью. Выравнивание, полный граф стека и дополнительные разрешённые пары могут съесть этот запас. Цель C не обещается. USB/PMA вариант рассматривается после минимального размера 1040, с сохранением packet ownership/reset/PRIMASK и замером реального общего RAM, включая новые буферы.

Стек переносить отдельным place at address mem:0x20000000 { block CSTACK }; обычные RW должны начинаться за ним. ROM, .intvec и fault_record 0x20004fc0 не меняются. При MSP ниже 0x20000000 может произойти ошибка stacking ещё до HardFault-handler и lockup; одна перестановка стека **не гарантирует** запись fault_record. Нужна отдельная проверка entry wrapper/резервного стека и сценария stacking fault, без заявления о проверке платы.

Для .suc карта перечисляет косвенные calls Modules/comm_* (баланс/AT/канал/модем/info/SMS/recover/server/power/call), USB core callbacks/endpoint dispatch и sj_replace_pages/sj_pages_equal. Обязательны все реально устанавливаемые hooks из user_comm, таблиц USB property/user requests и page readers config/ID/expiry. Максимум IRQ надо считать по текущей группировке и приоритетам: hw_config включает Group_2 (USB preempt 2), comm_uart preempt 1, wakeup/board preempt 0; учитывать SysTick/PVD, exception frame и переходы конфигурации. Простая сумма interrupt Total Use 256 не доказывает худшее вложение.

После IAR анализа — раскраска всей CSTACK на каждом Machine и суммарный максимум по run_all/paired; сохранённый minimum SP дополняется canary и IRQ frame. Модель не заменяет стек на плате.

## 5. Оценка этапа D до реализации

| Приём | Оценка Flash, Б | Что измерить и когда оставить |
|---|---:|---|
| Release без DEBUG_USER | 2444 Б уникальных исходных строк + код вызовов; чистый выигрыш пока неизвестен | В C DEBUG_USER включён безусловно. Строки могут совпадать/удерживаться; не засчитывать их сумму как сокращение BIN. ASM debug участки общие, нужна аккуратная общая правка |
| Весь sprintf → короткие formatter | Верхняя удаляемая библиотека 1325; ожидаемый чистый 700–1100 | 1261 xprintfsmall +64 sprintf, минус новый formatter. Кроме 12 debug мест есть 3 hex ID, SMS STATUS, u32 decimal helper; убрать все Release-ссылки |
| RAM debug scratch | 128 RAM, Flash отдельный замер | tempStrBuffer используется debug-путями; в ASM нельзя оставить общий DS8 хвост или нечаянно сдвинуть ABI layout |
| Multi-file compilation | 0–512, не обещано | Эксперимент только Release; контролировать C/ASM/HYBRID, runtime_layout, inlining и полный тест |
| merge_duplicate_sections | 0–512, не суммировать с MFC заранее | Отдельный замер. Карта уже содержит общие хвосты; эффект может быть 0 |
| SLDN/length/answer11 factoring | 0–300, только места ≥100 | reply_send и answer11 уже выделены в базе; короткий 8-Б заголовок сам по себе не обещает 100 Б на место |

Цель BIN <=87633 (=91729−4096). Суммы оценок перекрываются: formatter/debug/MFC могут убирать один и тот же код. Ни одно слагаемое не является доказательством достижения 4096 Б. После каждого шага сохранить BIN/RAM/modular totals до/после в %TEMP%, полный прогон трёх проектов и обе пары; шаг без выигрыша откатить. Пять последовательных шагов с выигрышем <30 Б — закончить подбор и зафиксировать результат.

SMS formatter обязан сохранить CRLF, знак/десятичные значения, fallback netType>=6 → Not network, NET 0..5, cloud index+1 и все триггеры. Отдельно hex reader ID 00%08X и decimal u32 helper: сверить все граничные значения, а не только STATUS. Общее форматирование не менять протокол.

## 6. Последовательность продолжения

1. Доступный Python, затем новый baseline run_all C/ASM и paired на свежих ELF; сохранить исходные образы в %TEMP% для дальнейшего сравнения с d9e3dd5.
2. Разнести только замкнутые группы гибрида, создать HYBRID ewp без копий C/ASM-файлов; подключить проект в build/run_all/paired. Проверить ≥198 сценариев и горячие пути <=1.03 от лучшей базы.
3. Общая область, затем полный .suc/IRQ и перенос/выбор стека, USB и const — каждое изменение отдельно с измерением и полным прогоном.
4. Formatter/debug, два флага IAR раздельно и только оправданные дубли. Общие правки проверять на всех трёх проектах, сравнивать не только новые C/ASM, но и с сохранённой базой.
5. Финальная проверка после последней правки; отчёт Tests/HYBRID_MEMORY_20261006.md, не более трёх новых итоговых JSON. Коммитов нет по явному поручению владельца.

Этап E выполняется независимо и только чтением/расчётом; его результаты находятся в отчёте.
