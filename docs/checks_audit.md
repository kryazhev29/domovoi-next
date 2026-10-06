# Опись проверок — этап 3

База `46be7f9`, `compare/c-asm`. Инвентаризация относится к текущим строкам исходников; воспроизводится `python Tests/audit_checks.py --check`.

Опись намеренно шире проверок отказа: включены все `if`, `for/while`, `switch/default`, условные выражения и возвраты, ABI-assert, чтения NOR, сравнения памяти/строк и расчёты CRC. Повторная строка означает отдельную операцию внутри условия, а не вторую проверку в прошивке. Декларации CRC тоже видны в этой описи исходника; это не подсчёт вызовов или байтов. Сохранены исходное условие и функция: это позволяет проверить классификацию без пересказа большого файла.

Метки консервативные: совпадение текста или CRC не доказывает дубль. Все строки «защита» и «нужна» сохранены. Ввод проходит проверки на границе своего слоя: UART/кадр/CRC, затем аргументы команды; это разные факты. CRC NOR не заменяет проверку результата записи, valid-header не заменяет чтение после обрыва питания. Граница после await/poll, USB epoch и Flash ownership проверяются заново, поскольку состояние может измениться.

Мёртвые ветки ASM_RUNTIME удалены отдельно, без удаления исполняемых проверок: 50 препроцессированных IAR translation units дали одинаковые токены до/после во всех шести Release/ReleaseCustomer проектах. Для ASM использованы фактические карты и перечни команд сборки; C-зеркала не являются его входами.

Кольцо `comm_modem_diag()->failure_step/failure_reason/failure_next` сохранено: `Modules/comm_recover.c:rc_early` использует его для восстановления. `fault_capture`, `fault_record`, stack paint/canary и остановка до IWDG сохранены в обеих сборках.

Кандидаты, оставленные: `service_crc_poll` предварительно проверяет занятый TX, а `comm_tx_acquire` проверяет его повторно. Предварительная проверка экономит вызовы в занятом канале; проверка acquire защищает владение. Повторные `mem_tables_ready` защищают Flash/NOR, поэтому не удалены. `cfg_equal_flash` сверяет именно зеркальную копию, CRC — валидность выбранного слота: это разные гарантии.

Исполняемые проверки не удалены. Единственный доказанный кандидат «дубль» — часть `!bytes` в `table_runtime_begin`; он сохранён после двух отменённых шагов по правилу красной проверки. Остальные части того же условия защищают владение/границы Flash и остались. Ни «недостижимо», ни «повтор» не присвоены без достаточного доказательства. Отключение диагностического кода описано отдельно в разделе «Этап 3» отчёта.

Всего записей: 2103; дубль: 1, защита: 987, нужна: 1115.

## User/c/user_fun_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/user_fun_runtime.c:163` / `storage_idle` | `return !flash_owner && !w25_async_active() && w25_read_available();` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:167` / `user_storage_command_ready` | `return !store.job && !store_reset_requested && !mem_config_result() && storage_idle();` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:171` / `store_command_allowed` | `return cmd == 24 \|\| cmd == 27 \|\| user_storage_command_ready();` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:175` / `user_storage_flash_ready` | `return store.clear!=9 && storage_idle();` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:194` / `upload_index` | `if (cmd == 10) return cch == CH_USB ? log_usbReadIndex : device.readLogIDIndex;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:194` / `upload_index` | `return cch == CH_USB ? log_usbReadIndex : device.readLogIDIndex;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:194` / `upload_index` | `if (cmd == 10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:195` / `upload_index` | `return cch == CH_USB ? temp_usbReadIndex : device.readTempIDIndex;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:195` / `upload_index` | `return cch == CH_USB ? temp_usbReadIndex : device.readTempIDIndex;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:200` / `upload_setIndex` | `if (cmd == 10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:201` / `upload_setIndex` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:203` / `upload_setIndex` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:220` / `upload_restore` | `if (mark != 0x314C5055u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:232` / `upload_reset` | `uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:239` / `upload_ack` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:240` / `upload_ack` | `uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:241` / `upload_ack` | `uint8_t channel = cch == CH_USB ? 0 : 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:242` / `upload_ack` | `if (!pending[channel])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:246` / `upload_ack` | `if (!user_writeConfigAndDevice())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:257` / `upload_finish` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:259` / `upload_finish` | `bool active = cmd == 10 ? device.status.readyLogData \|\| device.logsCount != 0 : device.status.readyID \|\| device.tempIDsCount != 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:261` / `upload_finish` | `if (server \|\| usb \|\| active)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:265` / `upload_finish` | `if (!user_writeConfigAndDevice())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:271` / `upload_finish` | `if (active && !(cmd == 10 ? log_clearLogBuffer() : user_clearTempIdBuffer())) return false;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:271` / `upload_finish` | `if (active && !(cmd == 10 ? log_clearLogBuffer() : user_clearTempIdBuffer()))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:273` / `upload_finish` | `if (cmd == 10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:373` / `gsm_user_debug` | `if (device.logBits & 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:382` / `user_debug` | `if (device.logBits & 4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:403` / `usb_data_reading` | `if (len < 11 \|\| memcmp(ptr, "SLDN", 4))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:403` / `usb_data_reading` | `memcmp(ptr, "SLDN", 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:405` / `usb_data_reading` | `if (body < 3 \|\| body != len - 8)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:416` / `reader_id` | `if (device.sourceGetID == source && (source != 3 \|\| device.addressGetID == address))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:469` / `user_imeivalid` | `if (chkdigitstr(device.IMEIGSM, 15))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:483` / `user_unipassword` | `if (user_imeivalid() == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:487` / `user_unipassword` | `for (i=0; i<3; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:492` / `user_unipassword` | `for (i=0; i<3; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:497` / `user_unipassword` | `for (i=0; i<3; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:528` / `user_timeToInt` | `switch (mon)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:544` / `user_timeToInt` | `if (div((temp+2),4).rem == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:546` / `user_timeToInt` | `if (mon < 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:620` / `server_rx_reset` | `if (server_rx_busy)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:630` / `server_rx_begin_pass` | `if (server_rx_busy)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:634` / `server_rx_begin_pass` | `if (server_rx_count && (uint32_t)(timers_getMillis() - server_rx_last) >= SERVER_RX_TIMEOUT_MS)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:652` / `server_rx_scan` | `if (server_rx_ready \|\| server_rx_busy \|\| !server_rx_work)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:653` / `server_rx_scan` | `while (server_rx_offset < server_rx_count && server_rx_work >= 8u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:656` / `server_rx_scan` | `if (!server_rx_total)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:658` / `server_rx_scan` | `n = left > 64u ? 64u : left;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:659` / `server_rx_scan` | `if (n > server_rx_work - 8u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:660` / `server_rx_scan` | `if (!n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:662` / `server_rx_scan` | `server_rx_work -= found + 4u < n ? found + 4u : n;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:664` / `server_rx_scan` | `if (found == n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:665` / `server_rx_scan` | `if (n - found < 4u && left > n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:668` / `server_rx_scan` | `if (left < 6u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:670` / `server_rx_scan` | `if (body < 3u \|\| body > SERVER_RX_CAPACITY - 8u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:673` / `server_rx_scan` | `if (left < body + 8u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:679` / `server_rx_scan` | `if (n > server_rx_work)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:680` / `server_rx_scan` | `if (n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:681` / `server_rx_scan` | `crc16_calcInit(p + server_rx_pos, n, server_rx_crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:684` / `server_rx_scan` | `if (server_rx_pos != server_rx_total - 2u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:685` / `server_rx_scan` | `if (server_rx_crc == ((uint16_t)p[server_rx_total - 2u] \| ((uint16_t)p[server_rx_total - 1u] << 8)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:688` / `server_rx_scan` | `if (server_rx_offset && server_rx_compacted)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:696` / `server_rx_scan` | `if (server_rx_offset && !server_rx_compacted)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:709` / `user_dataconnection` | `if (handle != device.connectionhandle \|\| server_rx_cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:710` / `user_dataconnection` | `if (!server_rx_busy && server_rx_count && (uint32_t)(now - server_rx_last) >= SERVER_RX_TIMEOUT_MS)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:712` / `user_dataconnection` | `if (siz)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:713` / `user_dataconnection` | `while (siz)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:714` / `user_dataconnection` | `if (server_rx_count == SERVER_RX_CAPACITY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:719` / `user_dataconnection` | `if (!n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:720` / `user_dataconnection` | `if (n > siz)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:724` / `user_dataconnection` | `if (!server_rx_defer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:733` / `server_rx_process` | `if (server_rx_provisional \|\| server_rx_busy)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:736` / `server_rx_process` | `if (!server_rx_ready \|\| !server_rx_commands \|\| comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:737` / `server_rx_process` | `if (!store_command_allowed(server_rx_buffer[8]))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:743` / `server_rx_process` | `if (server_rx_cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:754` / `server_rx_verified` | `return server_rx_busy && p == server_rx_buffer + 4;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:759` / `user_comm_rx_capacity` | `{ return server_rx_cancelled ? 0 : (uint16_t)(sizeof(server_rx_buffer) - server_rx_count);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:759` / `user_comm_rx_capacity` | `return server_rx_cancelled ? 0 : (uint16_t)(sizeof(server_rx_buffer) - server_rx_count);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:765` / `user_comm_rx` | `if (count > capacity)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:775` / `user_comm_rx_done` | `if (reason)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:788` / `user_comm_session` | `if (store.job && store.channel == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:790` / `user_comm_session` | `if (!open)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:800` / `user_smsreceived` | `if (device.triggers.smsToAdmin && *data != '#' && !user_send_smsToServer(data))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:823` / `user_smsreceived` | `if (dtEnd != dt)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:827` / `user_smsreceived` | `if (*dt == '#')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:834` / `user_smsreceived` | `if (v != dt)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:836` / `user_smsreceived` | `if (config.pinCode == *(uint32_t*)tempStrBuffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:845` / `user_smsreceived` | `if (arg == dt)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:856` / `user_smsreceived` | `for (char *c = v; *c; ++c)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:856` / `user_smsreceived` | `if (*c >= 'a' && *c <= 'z')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:858` / `user_smsreceived` | `if (validPin == true)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:860` / `user_smsreceived` | `if (fw_update_quiet() && (strcmp(cmd, "add") == 0 \|\| strcmp(cmd, "reset") == 0 \|\| strcmp(cmd, "setout") == 0 \|\| strcmp(cmd, "default") == 0))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:860` / `user_smsreceived` | `strcmp(cmd, "add")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:860` / `user_smsreceived` | `strcmp(cmd, "reset")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:861` / `user_smsreceived` | `strcmp(cmd, "setout")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:861` / `user_smsreceived` | `strcmp(cmd, "default")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:866` / `user_smsreceived` | `if (strcmp(cmd, "add") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:866` / `user_smsreceived` | `strcmp(cmd, "add")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:867` / `user_smsreceived` | `if (validFormat)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:883` / `user_smsreceived` | `if (strcmp(cmd, "imei") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:883` / `user_smsreceived` | `strcmp(cmd, "imei")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:890` / `user_smsreceived` | `if (strcmp(cmd, "status") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:890` / `user_smsreceived` | `strcmp(cmd, "status")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:895` / `user_smsreceived` | `device.csq, net[device.netType < 6u ? device.netType : 0], device.connected, user_comm_server(comm_server_address()) + 1, device.firmwareVersion, device.triggers.handUnBlock, device.triggers.sheduleUnBlock, device.triggers.autoregistration, device.triggers.openAll);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:903` / `user_smsreceived` | `if (strcmp(cmd, "reset") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:903` / `user_smsreceived` | `strcmp(cmd, "reset")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:910` / `user_smsreceived` | `if (strcmp(cmd, "setout") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:910` / `user_smsreceived` | `strcmp(cmd, "setout")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:911` / `user_smsreceived` | `if (validFormat)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:914` / `user_smsreceived` | `if (out == 1 \|\| out == 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:915` / `user_smsreceived` | `if (config.rele[out - 1u].onCmdServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:916` / `user_smsreceived` | `out_load (out == 1 ? OUT1 : OUT2, OUT_ON, 0, 0, 0);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:917` / `user_smsreceived` | `strcat(v, out == 1 ? ": Out 1 enabled" : ": Out 2 enabled");` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:932` / `user_smsreceived` | `if (strcmp(cmd, "update") == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:932` / `user_smsreceived` | `strcmp(cmd, "update")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:937` / `user_smsreceived` | `if (strcmp(cmd, "default") == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:937` / `user_smsreceived` | `strcmp(cmd, "default")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:959` / `user_sendsms` | `if (user_comm_sms(num, mes))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:983` / `user_incomingcall` | `if (typ == CT_PHONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1052` / `user_init` | `if (device.memID == 0x17C2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1070` / `user_init` | `if (BKP->DR1 == PVD_RESET_MARK && log_addSystemEvent(LOG_SYS_RESETONPOWER))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1075` / `user_init` | `if (config.wiegandType != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1089` / `user_clearMemBlock` | `if (mem[mb].size == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1099` / `user_clearMemBlock` | `for (adr = mem[mb].start; adr < endAdr; adr += 4096)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1100` / `user_clearMemBlock` | `if (!mem_erase_sector(adr))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1103` / `user_clearMemBlock` | `if (mb == MB_ID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1126` / `sync_end_tick` | `if (!sync_quiet \|\| --sync_quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1127` / `sync_end_tick` | `for (uint8_t i = 0; i < 5u; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1128` / `sync_end_tick` | `if ((sync_pending & (1u << i)) && log_addSystemEvent((logSystem_e)(LOG_SYS_ENDIDUPDATE + i)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1130` / `sync_end_tick` | `if (sync_pending)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1140` / `user_loop` | `if (config.wiegandType != 0 && user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1142` / `user_loop` | `if (user_storage_command_ready() && user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1143` / `user_loop` | `if (user_storage_command_ready() && user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1149` / `user_loop` | `if (timers_get(1) >= 50)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1153` / `user_loop` | `if (device.registredOnServer && device.FWinProgress)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1154` / `user_loop` | `if (device.startWaitFWtime == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1157` / `user_loop` | `if (user_getFWblockFromServer((dataChanel_e)device.cchFWupdate, device.FWblockIdx))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1161` / `user_loop` | `if ((timers_getMillis() - device.startWaitFWtime) >= FW_BLOCK_RETRY_MS)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1170` / `user_loop` | `if (timers_get(2) >= 100)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1181` / `user_loop` | `if (device.registredOnServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1184` / `user_loop` | `if (status_event_due(st, timers_getMillis()))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1188` / `user_loop` | `if (device.status.stateOut1 \|\| device.status.stateOut2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1197` / `user_loop` | `while (outTicks != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1202` / `user_loop` | `while (elapsedSeconds != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1209` / `user_loop` | `if (USB_DETECT)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1211` / `user_loop` | `if (usb_get_state() != ON)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1222` / `user_loop` | `if (user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1236` / `user_loop` | `if (device.timeToReset)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1237` / `user_loop` | `if (--device.timeToReset == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1247` / `user_loop` | `if (device.timeToDefault)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1248` / `user_loop` | `if (--device.timeToDefault == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1257` / `user_loop` | `if (device.timeToGetID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1259` / `user_loop` | `if (--device.timeToGetID == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1268` / `user_loop` | `if (device.answerTimeout)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1269` / `user_loop` | `if (--device.answerTimeout == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1270` / `user_loop` | `switch (device.answerID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1280` / `user_loop` | `if (++device.lmCount >= MAXSTATUSANSWERERROR)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1293` / `user_loop` | `if (device.registredOnServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1295` / `user_loop` | `if (device.lmTimer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1296` / `user_loop` | `if (--device.lmTimer == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1304` / `user_loop` | `if (device.rebootToUpdate)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1306` / `user_loop` | `if (app_boot_request())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1315` / `user_loop` | `if (device.updateTimeout)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1316` / `user_loop` | `if (--device.updateTimeout == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1318` / `user_loop` | `if (device.FWinProgress && device.updateTimeout % 60u == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1324` / `user_loop` | `if (app_trial_expired())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1334` / `user_loop` | `if (timers_get(4) >= 10000)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1340` / `user_loop` | `if (timers_get(5) >= 60000)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1347` / `user_loop` | `if (config.restartTime != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1348` / `user_loop` | `if (device.worktime >= ((uint32_t)config.restartTime*3600))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1381` / `user_initvariables` | `if (device.logsCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1384` / `user_initvariables` | `if (device.tempIDsCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1387` / `user_initvariables` | `if (device.countPhoto)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1397` / `config_exchange` | `for (uint16_t i = 0; i < sizeof(config); i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1411` / `config_terminate_strings` | `if (memchr(config.field, 0, sizeof(config.field)) == NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1422` / `config_terminate_strings` | `if (changed)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1422` / `config_terminate_strings` | `crc16_calc((uint8_t*)&config, sizeof(config) - 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1436` / `user_readconfig` | `while (read_count)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1438` / `user_readconfig` | `mem_read(mem[MB_CONFIG].start, (pInt8U)&config, sizeof(config))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1439` / `user_readconfig` | `if (config.crc == crc16_calc((pInt8U)&config, sizeof(config)-2))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1439` / `user_readconfig` | `crc16_calc((pInt8U)&config, sizeof(config)-2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1446` / `user_readconfig` | `if (read_count == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1458` / `user_writeconfig` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1460` / `user_writeconfig` | `crc16_calc((pInt8U)&config, sizeof(config)-2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1515` / `user_factoryReset` | `if (!user_writeConfigAndDevice())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1555` / `user_readsavedinformation` | `while (read_count--)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1558` / `user_readsavedinformation` | `mem_read (mem[MB_CONFIG].start + sizeof(config), (pInt8U)&DV, sizeof(DV))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1559` / `user_readsavedinformation` | `if (DV.crc == crc16_calc((pInt8U)&DV, sizeof(DV)-2))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1559` / `user_readsavedinformation` | `crc16_calc((pInt8U)&DV, sizeof(DV)-2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1606` / `user_savereset` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1629` / `user_saveindex` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1639` / `user_writeConfigAndDevice` | `crc16_calc((pInt8U)&config, sizeof(config)-2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1640` / `user_writeConfigAndDevice` | `crc16_calc((pInt8U)&device, sizeof(device)-2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1651` / `port_check_data` | `if (!user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1653` / `port_check_data` | `if (!device.getPhoto && uart_getstate(PORT1_PORT) && !uart_isReceiving(PORT1_PORT))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1674` / `user_calcCRC16InFlash` | `while (siz)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1675` / `user_calcCRC16InFlash` | `if (siz > 1024)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1680` / `user_calcCRC16InFlash` | `mem_read(address, bfr, block)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1681` / `user_calcCRC16InFlash` | `crc16_calcInit(bfr, block, crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1704` / `id_crc16` | `if (off < id_used_bytes) real = id_used_bytes - off < n ? id_used_bytes - off : n;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1704` / `id_crc16` | `if (off < id_used_bytes)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1705` / `id_crc16` | `uint16_t crc = real ? user_calcCRC16InFlash(mem[MB_ID].start + off, real) : 0xFFFF;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1706` / `id_crc16` | `crc16_ffInit(n - real, crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1716` / `user_comm_acquire` | `if (p && needed > capacity)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1720` / `file scope` | `if ((cch) == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1720` / `file scope` | `while (0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1725` / `user_send_anyData` | `if (size > 65524u \|\| (size && data == NULL))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1728` / `user_send_anyData` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1730` / `user_send_anyData` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1749` / `user_send_anyData` | `if (size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1752` / `user_send_anyData` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1757` / `user_send_anyData` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1783` / `user_send_registrationData` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1784` / `user_send_registrationData` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1820` / `user_send_registrationData` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1825` / `user_send_registrationData` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1850` / `user_send_statusPack` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1851` / `user_send_statusPack` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1879` / `user_send_statusPack` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1884` / `user_send_statusPack` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1892` / `user_send_statusPack` | `if (device.answerID != 2 \|\| device.answerTimeout == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1915` / `user_send_statusPackSecond` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1916` / `user_send_statusPackSecond` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1947` / `user_send_statusPackSecond` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1952` / `user_send_statusPackSecond` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1980` / `fw_status_poll` | `if (!fw_status_pending.due \|\| !comm_server_online())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:1982` / `fw_status_poll` | `if (!b)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1985` / `fw_status_poll` | `crc16_calc(b+4,7)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:1986` / `fw_status_poll` | `if (comm_tx_commit(13))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1996` / `user_send_statusUpdateFW` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1997` / `user_send_statusUpdateFW` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:1998` / `user_send_statusUpdateFW` | `if (cch == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2025` / `user_send_statusUpdateFW` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2030` / `user_send_statusUpdateFW` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2049` / `user_send_crcPack` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2054` / `user_send_crcPack` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2055` / `user_send_crcPack` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2088` / `user_send_crcPack` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2093` / `user_send_crcPack` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2113` / `user_send_crcMemoryID` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2122` / `user_send_crcMemoryID` | `if (idx == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2128` / `user_send_crcMemoryID` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2130` / `user_send_crcMemoryID` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2144` / `user_send_crcMemoryID` | `if (idx == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2151` / `user_send_crcMemoryID` | `for (uint16_t i = 0; i < 100; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2169` / `user_send_crcMemoryID` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2174` / `user_send_crcMemoryID` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2195` / `user_send_smsToServer` | `if (!comm_server_online())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2199` / `user_send_smsToServer` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2200` / `user_send_smsToServer` | `if (!sms)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2202` / `user_send_smsToServer` | `while (size + 11u <= capacity && sms[size])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2203` / `user_send_smsToServer` | `if (size + 11u > capacity)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2206` / `user_send_smsToServer` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2226` / `user_send_smsToServer` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2231` / `user_send_smsToServer` | `if (comm_tx_commit(len + 8))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2254` / `unreg_sms_poll` | `if (!unreg_sms_pending.due)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2257` / `unreg_sms_poll` | `if (user_send_smsToServer(text))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2275` / `user_send_readedIDtoServer` | `if (id == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2282` / `reader_reply_poll` | `if (!reader_out.state)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2284` / `reader_reply_poll` | `if (!b)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2288` / `reader_reply_poll` | `crc16_calc(&b[4], 16)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2290` / `reader_reply_poll` | `if (comm_tx_commit(22))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2302` / `user_send_answer_comand` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2303` / `user_send_answer_comand` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2316` / `user_send_answer_comand` | `send_buffer[6] = (cmd == 7 \|\| cmd == 21) ? 5 : 255;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2326` / `user_send_answer_comand` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2331` / `user_send_answer_comand` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2354` / `user_send_timeshtamp` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2355` / `user_send_timeshtamp` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2374` / `user_send_timeshtamp` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2379` / `user_send_timeshtamp` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2403` / `user_send_identPackToConfigurator` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2404` / `user_send_identPackToConfigurator` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2418` / `user_send_identPackToConfigurator` | `if (config.pinCode == 0x31323334)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2424` / `user_send_identPackToConfigurator` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2429` / `user_send_identPackToConfigurator` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2452` / `user_send_statusPackToConfigurator` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2453` / `user_send_statusPackToConfigurator` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2490` / `user_send_statusPackToConfigurator` | `if (fault_record[0] == FAULT_RECORD_MAGIC)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2492` / `user_send_statusPackToConfigurator` | `for (i = 0; i < sizeof(fault_record); i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2499` / `user_send_statusPackToConfigurator` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2504` / `user_send_statusPackToConfigurator` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2526` / `user_send_configToConfigurator` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(len + 8) : work_acquire(len + 8, WORK_REPLY);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2528` / `user_send_configToConfigurator` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2548` / `user_send_configToConfigurator` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2553` / `user_send_configToConfigurator` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2576` / `user_send_logRecords` | `if (upload_runtime_begin(10,cch,uniqId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2577` / `user_send_logRecords` | `if (cch != CH_USB && cch != CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2578` / `user_send_logRecords` | `if (store_runtime && !store_upload_request(10, cch, uniqId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2579` / `user_send_logRecords` | `if (!upload_ack(10, cch))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2585` / `user_send_logRecords` | `if (device.logsCount == 0xFFFFu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2592` / `user_send_logRecords` | `if (device.logsCount == 0 \|\| readIdx >= device.logsCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2593` / `user_send_logRecords` | `if (!upload_finish(10))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2601` / `user_send_logRecords` | `if (cntId > 32)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2603` / `user_send_logRecords` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire((cntId * sizeof(cellLog_s)) + 16) : work_acquire((cntId * sizeof(cellLog_s)) + 16, WORK_REPLY);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2607` / `user_send_logRecords` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2626` / `user_send_logRecords` | `if (!log_readRecords(readIdx, &send_buffer[12], cntId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2629` / `user_send_logRecords` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2635` / `user_send_logRecords` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2649` / `user_send_logRecords` | `log_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2649` / `user_send_logRecords` | `log_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2650` / `user_send_logRecords` | `if (cch != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2660` / `user_send_tempIDs` | `if (upload_runtime_begin(9,cch,uniqId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2661` / `user_send_tempIDs` | `if (cch != CH_USB && cch != CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2662` / `user_send_tempIDs` | `if (store_runtime && !store_upload_request(9, cch, uniqId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2663` / `user_send_tempIDs` | `if (!upload_ack(9, cch))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2668` / `user_send_tempIDs` | `if (device.tempIDsCount == 0xFFFFu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2670` / `user_send_tempIDs` | `if (device.tempIDsCount == 0 \|\| readIdx >= device.tempIDsCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2671` / `user_send_tempIDs` | `if (!upload_finish(9))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2681` / `user_send_tempIDs` | `if (cntId > 100)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2683` / `user_send_tempIDs` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire((cntId * 10) + 16) : work_acquire((cntId * 10) + 16, WORK_REPLY);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2687` / `user_send_tempIDs` | `if (send_buffer != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2706` / `user_send_tempIDs` | `for (uint16_t i = 0; i < cntId; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2707` / `user_send_tempIDs` | `if (!mem_read(readAdr + 2, &send_buffer[12 + (i*10)], 10))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2707` / `user_send_tempIDs` | `mem_read(readAdr + 2, &send_buffer[12 + (i*10)], 10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2711` / `user_send_tempIDs` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2717` / `user_send_tempIDs` | `switch (cch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2731` / `user_send_tempIDs` | `temp_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2731` / `user_send_tempIDs` | `temp_unacked[cch == CH_USB ? 0 : 1] = sent ? readIdx + cntId : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2742` / `user_getFWblockFromServer` | `if (fw_service_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2743` / `user_getFWblockFromServer` | `if (fw_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2745` / `user_getFWblockFromServer` | `uint8_t *send_buffer = cch == CH_SERVER ? user_comm_acquire(sizeof(send_local)) : send_local;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2746` / `user_getFWblockFromServer` | `if (!send_buffer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2751` / `user_getFWblockFromServer` | `if (cch != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2769` / `user_getFWblockFromServer` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:2774` / `user_getFWblockFromServer` | `if (cch == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2775` / `user_getFWblockFromServer` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2812` / `id_first_pass` | `return (t == 0xffffffff \|\| t < ID_TIME_VALID) ? 0 : t;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2812` / `id_first_pass` | `return (t == 0xffffffff \|\| t < ID_TIME_VALID) ? 0 : t;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2819` / `id_expired` | `return c->useTime != 0 && first != 0 && device.datetime >= first && device.datetime - first >= (uint32_t)c->useTime * 60u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2826` / `id_time_end` | `if (c->useTime == 0 \|\| first == 0 \|\| c->reserve[6] == ID_TIME_DONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2828` / `id_time_end` | `if (first > 0xffffffffu - (uint32_t)c->useTime * 60u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2830` / `id_time_end` | `return (end / ONE_DAY == first / ONE_DAY) ? end : 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2830` / `id_time_end` | `return (end / ONE_DAY == first / ONE_DAY) ? end : 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2835` / `id_watch_end` | `if (end != 0 && (id_nextEnd == 0 \|\| end < id_nextEnd))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2847` / `id_today` | `return device.datetime >= ID_TIME_VALID ? (uint16_t)(device.datetime / ONE_DAY) : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2847` / `id_today` | `return device.datetime >= ID_TIME_VALID ? (uint16_t)(device.datetime / ONE_DAY) : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2852` / `id_pass_day` | `return d == 0xffff ? 0 : d;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2852` / `id_pass_day` | `return d == 0xffff ? 0 : d;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2860` / `id_new_day` | `if (today == 0 \|\| last == today)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2861` / `id_new_day` | `if (last != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2872` / `usb_cmd_without_pin` | `return cmd == 100 \|\| cmd == 101 \|\| cmd == 102 \|\| cmd == USB_DIAG_COMMAND \|\| cmd == USB_COMM_DIAG_COMMAND;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2874` / `usb_cmd_without_pin` | `return cmd == 100 \|\| cmd == 101 \|\| cmd == 102;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2899` / `status_event_due` | `return ((status ^ status_reported) & STATUS_EVENT_MASK) != 0 && (uint32_t)(now_ms - status_reported_ms) >= STATUS_EVENT_GAP_MS;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2930` / `fw_reason_code` | `if (text == NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2931` / `fw_reason_code` | `for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2932` / `fw_reason_code` | `if (strcmp(text, names[i]) == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2932` / `fw_reason_code` | `strcmp(text, names[i])` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:2933` / `fw_reason_code` | `if (strncmp(text, "BOOT ", 5) == 0 && text[5] >= '0' && text[5] <= '9')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2933` / `fw_reason_code` | `strncmp(text, "BOOT ", 5)` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:2968` / `fw_abort` | `if (reason != NULL) fw_log(strcmp(reason, "TIMEOUT") == 0 ? LOG_FW_TIMEOUT : LOG_FW_FAIL, reason);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:2968` / `fw_abort` | `if (reason != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2968` / `fw_abort` | `strcmp(reason, "TIMEOUT")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:2969` / `fw_abort` | `if (device.cchFWupdate != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2979` / `fw_end` | `if (device.cchFWupdate == CH_USB \|\| device.registredOnServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:2994` / `fw_image_foreign` | `return hw != 0 && (w & 0xFFFFFF00u) == FW_HW_MARKER_TAG && (uint8_t)w != hw;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3005` / `fw_start_rejected` | `if (number == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3006` / `fw_start_rejected` | `if (number == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3007` / `fw_start_rejected` | `return (size > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) \|\| ((size & 3u) != 0) \|\| (blocks < (size + 1023u) / 1024u) \|\| ((FWBLOCK_OFFSET + (uint32_t)blocks * 1024u) > mem[MB_FIRMWARE].size) \|\| (build == 0) \|\| (size == 0) \|\| (crc == 0) \|\| (blocks == 0);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3019` / `fw_start_refused` | `if (BOOT_VER < 0x30)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3020` / `fw_start_refused` | `if (boot_manual_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3021` / `fw_start_refused` | `if (app_trial_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3022` / `fw_start_refused` | `if (device.rebootToUpdate \|\| device.FWinProgress \|\| fw_rollback_end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3032` / `fw_same_download` | `if (!device.FWinProgress \|\| device.cchFWupdate != cch \|\| blocks != device.FWblockCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3033` / `fw_same_download` | `if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3033` / `fw_same_download` | `mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3034` / `fw_same_download` | `return h.firmwareVersion == number && h.buildDate == build && h.firmwareSize == size && h.firmwareCRC == crc;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3045` / `fw_rollback` | `if (!mem_read(mem[MB_BACKUP].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3045` / `fw_rollback` | `mem_read(mem[MB_BACKUP].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3046` / `fw_rollback` | `if ((fwParam.buildDate == 0xffffffff) \|\| (fwParam.firmwareSize == 0xffffffff) \|\| (fwParam.firmwareCRC == 0xffffffff) \|\| (fwParam.firmwareVersion == 0xffff))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3051` / `fw_rollback` | `if ((fwParam.firmwareSize > (CPU_ENDMEMORYADDRESS + 1u - APLICATION_ADDRESS)) \|\| !user_checkFirmware(mem[MB_BACKUP].start + FWBLOCK_OFFSET, fwParam.firmwareSize, fwParam.firmwareCRC))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3068` / `fw_rollback_poll` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3071` / `fw_rollback_poll` | `if (!fw_rollback_end)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3072` / `fw_rollback_poll` | `n = fw_rollback_end - fw_rollback_pos > 128u ? 128u : (uint16_t)(fw_rollback_end - fw_rollback_pos);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3073` / `fw_rollback_poll` | `if ((fw_rollback_pos & 4095u) == 0 && !mem_erase_sector(mem[MB_FIRMWARE].start + fw_rollback_pos))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3076` / `fw_rollback_poll` | `if (!mem_read(mem[MB_BACKUP].start + fw_rollback_pos, bf, n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3076` / `fw_rollback_poll` | `mem_read(mem[MB_BACKUP].start + fw_rollback_pos, bf, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3077` / `fw_rollback_poll` | `if (fw_rollback_pos == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3078` / `fw_rollback_poll` | `if (!mem_write(mem[MB_FIRMWARE].start + fw_rollback_pos, bf, n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3081` / `fw_rollback_poll` | `if (fw_rollback_pos < fw_rollback_end)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3083` / `fw_rollback_poll` | `if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) \|\| !user_checkFirmware(mem[MB_FIRMWARE].start + FWBLOCK_OFFSET, h.firmwareSize, h.firmwareCRC))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3083` / `fw_rollback_poll` | `mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3109` / `fw_start` | `if (fw_start_rejected(number, build, size, crc, blocks))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3113` / `fw_start` | `if (number != 1 && fw_same_download(number, build, size, crc, blocks, cch))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3118` / `fw_start` | `if (busy != NULL)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3122` / `fw_start` | `if (number == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3125` / `fw_start` | `if (!mem_erase_sector(mem[MB_FIRMWARE].start))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3132` / `fw_start` | `if (!mem_write(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3144` / `fw_start` | `if (cch != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3160` / `fw_block_written` | `for (off = 0; off < 1024u; off += sizeof(b))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3161` / `fw_block_written` | `if (!mem_read(adr + off, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3161` / `fw_block_written` | `mem_read(adr + off, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3162` / `fw_block_written` | `if (memcmp(b, data + off, sizeof(b)) != 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3162` / `fw_block_written` | `memcmp(b, data + off, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3176` / `fw_blocks_done` | `if (crc_ok)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3178` / `fw_blocks_done` | `if (device.cchFWupdate != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3179` / `fw_blocks_done` | `return crc_ok ? FW_BLOCK_OK : FW_BLOCK_END;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3179` / `fw_blocks_done` | `return crc_ok ? FW_BLOCK_OK : FW_BLOCK_END;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3195` / `fw_take_block` | `if (boot_manual_busy() \|\| cch != (dataChanel_e)device.cchFWupdate)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3196` / `fw_take_block` | `if (n != 1026u) return device.FWblockIdx >= device.FWblockCount ? FW_BLOCK_END : FW_BLOCK_BADFRAME;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3196` / `fw_take_block` | `return device.FWblockIdx >= device.FWblockCount ? FW_BLOCK_END : FW_BLOCK_BADFRAME;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3196` / `fw_take_block` | `if (n != 1026u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3198` / `fw_take_block` | `if (block < device.FWblockIdx)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3199` / `fw_take_block` | `if (device.FWblockIdx >= device.FWblockCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3200` / `fw_take_block` | `if (block != device.FWblockIdx)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3201` / `fw_take_block` | `if (block == 0 && fw_image_foreign(&p[2], device.hardwareVersion))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3210` / `fw_take_block` | `if (store_runtime) return fw_async_begin(&p[2], adr, cch) ? FW_BLOCK_PENDING : FW_BLOCK_BADWRITE;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3210` / `fw_take_block` | `return fw_async_begin(&p[2], adr, cch) ? FW_BLOCK_PENDING : FW_BLOCK_BADWRITE;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3210` / `fw_take_block` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3214` / `fw_take_block` | `if (!written \|\| !fw_block_written(adr, &p[2]))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3215` / `fw_take_block` | `if (++fw_write_errors < FW_WRITE_TRIES)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3223` / `fw_take_block` | `if (device.FWblockIdx < device.FWblockCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3224` / `fw_take_block` | `if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3224` / `fw_take_block` | `mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3237` / `fw_resume_block` | `return device.FWblockIdx < device.FWblockCount ? device.FWblockIdx : UINT16_MAX;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3237` / `fw_resume_block` | `return device.FWblockIdx < device.FWblockCount ? device.FWblockIdx : UINT16_MAX;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3249` / `fw_resume_after_boot` | `if (!device.FWinProgress)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3254` / `fw_resume_after_boot` | `if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) \|\| BOOT_VER < 0x30 \|\| h.appIdent != 0xFFFFFFFFu \|\| h.firmwareVersion < 2 \|\| fw_start_rejected(h.firmwareVersion, h.buildDate, h.firmwareSize, h.firmwareCRC, device.FWblockCount))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3254` / `fw_resume_after_boot` | `mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3265` / `fw_resume_after_boot` | `if (device.FWblockIdx == UINT16_MAX)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3267` / `fw_resume_after_boot` | `if (device.updateTimeout == 0 \|\| device.updateTimeout > FW_TIMEOUT_S)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3278` / `cmd8_event` | `switch (source)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3282` / `cmd8_event` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3299` / `sync_cells_valid` | `if (len < 8)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3301` / `sync_cells_valid` | `if (count == 0 \|\| (uint32_t)(len - 8) != (uint32_t)count * cell_size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3302` / `sync_cells_valid` | `for (uint16_t i = 0; i < count; i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3305` / `sync_cells_valid` | `if (mb == MB_ID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3306` / `sync_cells_valid` | `if (index == 0 \|\| index > mem[mb].size / cell_size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3307` / `sync_cells_valid` | `if (mb == MB_ID && cell[12] == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3317` / `command_args_valid` | `switch (ptr[4])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3319` / `command_args_valid` | `if (len < 4)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3320` / `command_args_valid` | `return ptr[5] != CMD_SUCCESS \|\| len >= 8;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3322` / `command_args_valid` | `return len >= 4;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3323` / `command_args_valid` | `return len >= 6;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3324` / `command_args_valid` | `return len >= 7;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3325` / `command_args_valid` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3369` / `user_parse_comand` | `if (len < 3)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3372` / `user_parse_comand` | `i = (cch == CH_SERVER && server_rx_verified(ptr)) ? crc : crc16_calc(ptr, len + 2);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3372` / `user_parse_comand` | `crc16_calc(ptr, len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:3378` / `user_parse_comand` | `if (crc != i)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3384` / `user_parse_comand` | `if (!command_args_valid(ptr, len))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3392` / `user_parse_comand` | `if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4]))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3397` / `user_parse_comand` | `if (store_runtime && !store_command_allowed(ptr[4]))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3400` / `user_parse_comand` | `switch (ptr[4])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3406` / `user_parse_comand` | `if (cch != CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3409` / `user_parse_comand` | `switch ((cmdCode_e)ptr[5])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3414` / `user_parse_comand` | `if (app_trial_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3416` / `user_parse_comand` | `if (!app_trial_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3420` / `user_parse_comand` | `if (app_boot_error_unreported())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3423` / `user_parse_comand` | `fw_log(strcmp(reason, "TRIAL FAIL") == 0 ? LOG_FW_ROLLBACK : LOG_FW_FAIL, reason);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3423` / `user_parse_comand` | `strcmp(reason, "TRIAL FAIL")` | **нужна** | результат операции используется последующим кодом |
| `User/c/user_fun_runtime.c:3446` / `user_parse_comand` | `if (!app_trial_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3452` / `user_parse_comand` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3463` / `user_parse_comand` | `if (device.rebootToUpdate)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3467` / `user_parse_comand` | `if (device.FWinProgress)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3468` / `user_parse_comand` | `if (ptr[5] == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3482` / `user_parse_comand` | `if (ptr[5] == 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3495` / `user_parse_comand` | `if ((len - 3) == 16)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3498` / `user_parse_comand` | `if (device.status.unblockHand != device.triggers.handUnBlock)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3499` / `user_parse_comand` | `if (device.triggers.handUnBlock)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3505` / `user_parse_comand` | `if (device.status.unblockOnScene != device.triggers.sheduleUnBlock)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3506` / `user_parse_comand` | `if (device.triggers.sheduleUnBlock)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3512` / `user_parse_comand` | `if (device.status.autoRegistration != device.triggers.autoregistration)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3513` / `user_parse_comand` | `if (device.triggers.autoregistration)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3519` / `user_parse_comand` | `if (device.status.openAnyUser != device.triggers.openAll)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3520` / `user_parse_comand` | `if (device.triggers.openAll)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3543` / `user_parse_comand` | `if ((ptr[5] == 0) \|\| (ptr[6] == 0) \|\| ((ptr[6] == 3) && (ptr[7] == 0)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3548` / `user_parse_comand` | `if (device.timeToGetID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3557` / `user_parse_comand` | `user_send_answer_comand(7, len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR, cch, uniqId);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3564` / `user_parse_comand` | `if (fw_update_quiet())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3568` / `user_parse_comand` | `if (ptr[5] != 0 && (len - 3) >= 3)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3575` / `user_parse_comand` | `if ((len - 3) >= 13)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3576` / `user_parse_comand` | `if (ptr[6] > 1 \|\| ptr[7] > 1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3584` / `user_parse_comand` | `if (ptr[6] == 0 && config.rele[0].onCmdServer)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3585` / `user_parse_comand` | `if (ptr[7] == 0 && config.rele[1].onCmdServer)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3588` / `user_parse_comand` | `if ((ptr[6] == 1 \|\| ptr[7] == 1) && !user_activateOut((ptr[6] == 1) && config.rele[0].onCmdServer, (ptr[7] == 1) && config.rele[1].onCmdServer, CT_SERVER, tempStrBuffer))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3620` / `user_parse_comand` | `if (len >= 14 && ptr[5] != 0 && ptr[15] != 0 && memchr(config.countryCode, 0, sizeof(config.countryCode)) != NULL)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3625` / `user_parse_comand` | `if (text == NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3646` / `user_parse_comand` | `if ((len - 3) == sizeof(config))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3651` / `user_parse_comand` | `if ((p[8] == 0) \|\| (p[137] == 0))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3668` / `user_parse_comand` | `if (!saved)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3679` / `user_parse_comand` | `if (!sync_cells_valid(ptr, len, MB_ID, sizeof(cellID_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3683` / `user_parse_comand` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3687` / `user_parse_comand` | `if (!table_runtime_begin(ptr,cch,uniqId))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3695` / `user_parse_comand` | `if (!sync_cells_valid(ptr, len, MB_SHEDULE, sizeof(cellShedule_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3699` / `user_parse_comand` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3703` / `user_parse_comand` | `if (!table_runtime_begin(ptr,cch,uniqId))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3711` / `user_parse_comand` | `if (!sync_cells_valid(ptr, len, MB_GROUP, sizeof(cellGroup_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3715` / `user_parse_comand` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3719` / `user_parse_comand` | `if (!table_runtime_begin(ptr,cch,uniqId))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3729` / `user_parse_comand` | `if (len < 8 \|\| ptr[9] == 0 \|\| (uint32_t)(len - 8) != (uint32_t)ptr[9] * sizeof(cellHoliday_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3733` / `user_parse_comand` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3737` / `user_parse_comand` | `if (!table_runtime_begin(ptr,cch,uniqId))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3754` / `user_parse_comand` | `if (ptr[5] > 100)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3763` / `user_parse_comand` | `if (cch != CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3766` / `user_parse_comand` | `if ((len - 3) == 16)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3767` / `user_parse_comand` | `if (fw_service_begin(ptr+5,cch,uniqId))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3769` / `user_parse_comand` | `if (started == CMD_SUCCESS && store_save_requested)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3779` / `user_parse_comand` | `if (cch != CH_USB && len >= 5)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3786` / `user_parse_comand` | `if (code == FW_BLOCK_PENDING)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3795` / `user_parse_comand` | `if (len == 3 \|\| len == 9)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3824` / `user_parse_comand` | `if (cch == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3825` / `user_parse_comand` | `if (device.answerID == 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3839` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3844` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3849` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3850` / `user_parse_comand` | `if (*(uint32_t*)&ptr[5] == config.pinCode)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3860` / `user_parse_comand` | `if (cch==CH_USB && !idclear_runtime_begin(cch,uniqId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3872` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3880` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3890` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3894` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3900` / `user_parse_comand` | `if (cch != CH_USB \|\| len < 4)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3904` / `user_parse_comand` | `if (device.FWinProgress \|\| device.rebootToUpdate \|\| app_trial_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3910` / `user_parse_comand` | `if (response[0] == 2 && response[1] == MANUAL_OK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3921` / `user_parse_comand` | `if (cch == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3925` / `user_parse_comand` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:3937` / `user_indication` | `if (comm_server_online())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3938` / `user_indication` | `if (modem_slm336_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3947` / `user_readinputs` | `if (!adc_startInjectedConvertion())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3950` / `user_readinputs` | `device.status.stateIn1 = (adc_getinjectedvalue_mV(IN1_CHANEL) > 3000)?1:0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3951` / `user_readinputs` | `device.status.stateIn2 = (adc_getinjectedvalue_mV(IN2_CHANEL) > 3000)?1:0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3955` / `user_readinputs` | `if (device.laststatus.stateIn1 != device.status.stateIn1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3956` / `user_readinputs` | `if (config.inputMode[0])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3957` / `user_readinputs` | `switch (config.inputMode[0])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3966` / `user_readinputs` | `if (device.status.stateIn1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3975` / `user_readinputs` | `if (device.laststatus.stateIn2 != device.status.stateIn2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3976` / `user_readinputs` | `if (config.inputMode[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3977` / `user_readinputs` | `switch (config.inputMode[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:3986` / `user_readinputs` | `if (device.status.stateIn2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4001` / `user_last10digitsnumber` | `return (l <= 10)?num:(num + (l-10));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4001` / `user_last10digitsnumber` | `return (l <= 10)?num:(num + (l-10));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4016` / `user_getweakday` | `if (wd > 6)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4034` / `user_findID` | `for (uint8_t i = 0; i < 10; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4035` / `user_findID` | `if (idsum == 2550 \|\| idsum == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4039` / `user_findID` | `if (r >= 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4040` / `user_findID` | `if (r)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4041` / `user_findID` | `return r == 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4050` / `user_findID` | `for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4051` / `user_findID` | `if (!mem_read(adr, tbuffer, 1024))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4051` / `user_findID` | `mem_read(adr, tbuffer, 1024)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4053` / `user_findID` | `for (uint16_t a = 0; a < 1024 && adr + a < endAdr; a += sizeof(cellID_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4056` / `user_findID` | `if (c->index == 0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4058` / `user_findID` | `if (memcmp(c->ID, id, 10) == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4058` / `user_findID` | `memcmp(c->ID, id, 10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4077` / `user_findTempID` | `if (startAddress < mem[MB_TEMPID].start)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4080` / `user_findTempID` | `for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4081` / `user_findTempID` | `if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4081` / `user_findTempID` | `mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4083` / `user_findTempID` | `if (memcmp(cellId.ID, id, 10) == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4083` / `user_findTempID` | `memcmp(cellId.ID, id, 10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4101` / `user_addIdToTempBuffer` | `if (startAddress < mem[MB_TEMPID].start)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4104` / `user_addIdToTempBuffer` | `for (adr = mem[MB_TEMPID].start; adr < startAddress; adr += sizeof(cellID_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4105` / `user_addIdToTempBuffer` | `if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4105` / `user_addIdToTempBuffer` | `mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4107` / `user_addIdToTempBuffer` | `if (memcmp(cellId.ID, id, 10) == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4107` / `user_addIdToTempBuffer` | `memcmp(cellId.ID, id, 10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4113` / `user_addIdToTempBuffer` | `for (adr = startAddress; adr < endAdr; adr += sizeof(cellID_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4114` / `user_addIdToTempBuffer` | `if (!mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4114` / `user_addIdToTempBuffer` | `mem_read(adr, (uint8_t*)&cellId, sizeof(cellID_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4116` / `user_addIdToTempBuffer` | `if (cellId.index == 0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4121` / `user_addIdToTempBuffer` | `switch (tp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4127` / `user_addIdToTempBuffer` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4129` / `user_addIdToTempBuffer` | `if (!mem_write(adr, (uint8_t*)&cellId, sizeof(cellID_s)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4145` / `shedule_time_ok` | `return start <= stop ? (now >= start && now <= stop) : (now >= start \|\| now <= stop);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4145` / `shedule_time_ok` | `return start <= stop ? (now >= start && now <= stop) : (now >= start \|\| now <= stop);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4151` / `user_checkIdAndControl` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4153` / `user_checkIdAndControl` | `switch (tp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4165` / `user_checkIdAndControl` | `if (device.status.unblockHand)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4172` / `user_checkIdAndControl` | `if (device.status.unblockOnScene)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4183` / `user_checkIdAndControl` | `for (uint8_t i = 0; i < 2; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4184` / `user_checkIdAndControl` | `if (config.rele[i].useInBlock == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4185` / `user_checkIdAndControl` | `switch (tp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4190` / `user_checkIdAndControl` | `if (config.rele[i].onWiegand)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4193` / `user_checkIdAndControl` | `if (config.rele[i].onRF433)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4196` / `user_checkIdAndControl` | `if (config.rele[i].onRFID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4199` / `user_checkIdAndControl` | `if (config.rele[i].onCamera)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4202` / `user_checkIdAndControl` | `if (config.rele[i].onCmdServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4204` / `user_checkIdAndControl` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4205` / `user_checkIdAndControl` | `if (config.rele[i].onOther)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4212` / `user_checkIdAndControl` | `if (user_findTempID(id))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4221` / `user_checkIdAndControl` | `if (device.status.autoRegistration)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4227` / `user_checkIdAndControl` | `if (device.status.openAnyUser)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4236` / `user_checkIdAndControl` | `if (device.triggers.openAllByShedule)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4242` / `user_checkIdAndControl` | `if (user_checkShedule(device.triggers.openAllByShedule) == true)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4249` / `user_checkIdAndControl` | `if (onOut[0] \|\| onOut[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4254` / `user_checkIdAndControl` | `if(found>1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4255` / `user_checkIdAndControl` | `if (found)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4257` / `user_checkIdAndControl` | `if ((cellId.blockTime != 0) && (cellId.blockTime < device.datetime))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4265` / `user_checkIdAndControl` | `if (id_expired(&cellId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4275` / `user_checkIdAndControl` | `if (user_getGroup(cellId.group, &group) == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4284` / `user_checkIdAndControl` | `if (group.blocked)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4293` / `user_checkIdAndControl` | `switch (tp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4295` / `user_checkIdAndControl` | `if (group.onPhone == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4298` / `user_checkIdAndControl` | `if (group.onWiegand == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4301` / `user_checkIdAndControl` | `if (group.onRF433 == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4304` / `user_checkIdAndControl` | `if (group.onRFID == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4307` / `user_checkIdAndControl` | `if (group.onAuto == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4309` / `user_checkIdAndControl` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4314` / `user_checkIdAndControl` | `if (group.sheduleNumber == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4325` / `user_checkIdAndControl` | `if (user_getShedule(group.sheduleNumber, &shedule) == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4334` / `user_checkIdAndControl` | `if (shedule.blocked)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4343` / `user_checkIdAndControl` | `if (shedule.day[device.day] == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4357` / `user_checkIdAndControl` | `if (flag == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4367` / `user_checkIdAndControl` | `switch (shedule.onHoliday)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4383` / `user_checkIdAndControl` | `if (flag == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4398` / `user_checkIdAndControl` | `if (device.triggers.smsUnreg)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4423` / `user_activateOut` | `if (!run[0] && !run[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4438` / `user_activateOut` | `uint8_t found=(id!=NULL && id[0]!=0)?user_findID(id,&cellId):0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4439` / `user_activateOut` | `if(found>1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4440` / `user_activateOut` | `if (found)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4443` / `user_activateOut` | `if (cellId.cntPassOnDay != 255)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4444` / `user_activateOut` | `if (id_new_day(&cellId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4446` / `user_activateOut` | `if (cellId.cntPass != 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4452` / `user_activateOut` | `if (save)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4457` / `user_activateOut` | `if (cellId.useTime != 0 && id_first_pass(&cellId) == 0 && device.datetime >= ID_TIME_VALID)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4462` / `user_activateOut` | `if (save && !user_updateID(&cellId))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4467` / `user_activateOut` | `for (out = 0; out < 2; out++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4468` / `user_activateOut` | `if (!run[out])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4470` / `user_activateOut` | `user_debug(out == 0 ? ">>U Act Out: Out 1\r\n" : ">>U Act Out: Out 2\r\n");` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4473` / `user_activateOut` | `if (config.rele[out].mode == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4477` / `user_activateOut` | `out_load(out, (tp == CT_SERVER) ? OUT_ON : OUT_INV, 0, 0, 0);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4480` / `user_activateOut` | `switch (tp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4499` / `user_activateOut` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4503` / `user_activateOut` | `if (cnt != 255)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4504` / `user_activateOut` | `if (cnt == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4505` / `user_activateOut` | `if (started)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4513` / `user_getGroup` | `if(!num)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4515` / `user_getGroup` | `if(offset+sizeof(cellGroup_s)>mem[MB_GROUP].size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4516` / `user_getGroup` | `return mem_read(mem[MB_GROUP].start+offset,(uint8_t *)group,sizeof(cellGroup_s)) && *(uint8_t *)group==num;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4516` / `user_getGroup` | `mem_read(mem[MB_GROUP].start+offset,(uint8_t *)group,sizeof(cellGroup_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4524` / `user_getShedule` | `if(!num)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4526` / `user_getShedule` | `if(offset+sizeof(cellShedule_s)>mem[MB_SHEDULE].size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4527` / `user_getShedule` | `return mem_read(mem[MB_SHEDULE].start+offset,(uint8_t *)shedule,sizeof(cellShedule_s)) && *(uint8_t *)shedule==num;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4527` / `user_getShedule` | `mem_read(mem[MB_SHEDULE].start+offset,(uint8_t *)shedule,sizeof(cellShedule_s))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4536` / `user_dayIsHoliday` | `for(uint32_t offset=0;offset<1024;offset+=64)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4537` / `user_dayIsHoliday` | `if(!mem_read(mem[MB_HOLIDAY].start+offset,data,64))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4537` / `user_dayIsHoliday` | `mem_read(mem[MB_HOLIDAY].start+offset,data,64)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4538` / `user_dayIsHoliday` | `for(unsigned i=0;i<64;i+=sizeof(cellHoliday_s))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4539` / `user_dayIsHoliday` | `if(value/86400u==day)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4554` / `user_clearTempIdBuffer` | `for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 4096)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4555` / `user_clearTempIdBuffer` | `if (!mem_erase_sector(adr))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4581` / `user_getNumbersCount` | `for (adr = mem[MB_ID].start; adr < endAdr; adr += 1024)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4582` / `user_getNumbersCount` | `mem_read(adr, tbuffer, 1024)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4584` / `user_getNumbersCount` | `for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4588` / `user_getNumbersCount` | `for (uint8_t b = 0; b < sizeof(cellID_s); b++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4589` / `user_getNumbersCount` | `if (tbuffer[a + b] != 0xff)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4591` / `user_getNumbersCount` | `if (c->index == 0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4626` / `user_getTempIDRecordsCount` | `for (adr = mem[MB_TEMPID].start; adr < endAdr; adr += 1024)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4627` / `user_getTempIDRecordsCount` | `if (!mem_read(adr, tbuffer, 1024))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4627` / `user_getTempIDRecordsCount` | `mem_read(adr, tbuffer, 1024)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4629` / `user_getTempIDRecordsCount` | `for (uint16_t a = 0; a < 1024; a += sizeof(cellID_s))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4632` / `user_getTempIDRecordsCount` | `if (c->index == 0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4649` / `user_checkShedule` | `if (!mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4653` / `user_checkShedule` | `if (user_getShedule(shd, &shedule) == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4661` / `user_checkShedule` | `if (shedule.blocked)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4669` / `user_checkShedule` | `if (shedule.day[device.day] == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4683` / `user_checkShedule` | `if (flag == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4692` / `user_checkShedule` | `switch (shedule.onHoliday)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4708` / `user_checkShedule` | `if (flag == false)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4726` / `user_updateID` | `if (c->index != 0 && c->index != 0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4728` / `user_updateID` | `if (adr + sizeof(cellID_s) <= endAdr)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4729` / `user_updateID` | `if (!mem_read(adr, (uint8_t*)&index, 2))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4729` / `user_updateID` | `mem_read(adr, (uint8_t*)&index, 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4730` / `user_updateID` | `if (index == c->index)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4733` / `user_updateID` | `for (uint32_t adr = mem[MB_ID].start; adr + sizeof(cellID_s) <= endAdr; adr += sizeof(cellID_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4734` / `user_updateID` | `if (!mem_read(adr, (uint8_t*)&index, 2))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4734` / `user_updateID` | `mem_read(adr, (uint8_t*)&index, 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4735` / `user_updateID` | `if (index == c->index)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4755` / `log_clearLogBuffer` | `if (device.newAddressLog > mem[MB_LOG].start && device.newAddressLog < endAdr)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4758` / `log_clearLogBuffer` | `for (adr = mem[MB_LOG].start & ~(uint32_t)4095; adr < endAdr; adr += 4096)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4759` / `log_clearLogBuffer` | `if (!mem_erase_sector(adr))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4776` / `log_blank` | `for (uint16_t i = 0; i < sizeof(*cell); i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4776` / `log_blank` | `if (p[i] != 0xFF)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4790` / `log_valid` | `if (cell->index == 0xFFFFFFFFu)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4792` / `log_valid` | `if (mark == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4793` / `log_valid` | `for (uint8_t i = 0; i < sizeof(cell->reserve); i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4793` / `log_valid` | `if (cell->reserve[i] != 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4796` / `log_valid` | `if (mark != 0x32474F4Cu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4799` / `log_valid` | `return inverse == (uint16_t)~crc && crc == crc16_calc((uint8_t*)cell, sizeof(*cell) - sizeof(cell->reserve));` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4800` / `log_valid` | `crc16_calc((uint8_t*)cell, sizeof(*cell) - sizeof(cell->reserve))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4813` / `log_uniqueIndex` | `if (adr >= mem[MB_LOG].start + sizeof(cellLog_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4814` / `log_uniqueIndex` | `if (mem_read(adr - sizeof(cellLog_s), (uint8_t*)&prev, sizeof(prev)) && log_valid(&prev) && prev.index > last)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4814` / `log_uniqueIndex` | `mem_read(adr - sizeof(cellLog_s), (uint8_t*)&prev, sizeof(prev))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4817` / `log_uniqueIndex` | `if (last != 0 && cl->index <= last)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4825` / `log_addEvent` | `if (store.clear == 10 \|\| !user_storage_flash_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4829` / `log_addEvent` | `if (startAddress < mem[MB_LOG].start)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4830` / `log_addEvent` | `for (uint32_t adr = startAddress; adr + sizeof(cell) <= endAdr; adr += sizeof(cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4832` / `log_addEvent` | `if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4832` / `log_addEvent` | `mem_read(adr, (uint8_t*)&cell, sizeof(cell))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4833` / `log_addEvent` | `if (log_blank(&cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4836` / `log_addEvent` | `crc16_calc((uint8_t*)&cell, sizeof(cell) - sizeof(cell.reserve))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4843` / `log_addEvent` | `if (!mem_write(adr, (uint8_t*)&cell, sizeof(cell) - 4) \|\| !mem_write(adr + sizeof(cell) - 4, (uint8_t*)&mark, 4))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4845` / `log_addEvent` | `if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell)) \|\| !log_valid(&cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4845` / `log_addEvent` | `mem_read(adr, (uint8_t*)&cell, sizeof(cell))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4862` / `log_getRecordsCount` | `for (uint32_t adr = mem[MB_LOG].start; adr < endAdr; adr += sizeof(tbuffer))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4863` / `log_getRecordsCount` | `uint16_t n = endAdr - adr > sizeof(tbuffer) ? sizeof(tbuffer) : endAdr - adr;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4865` / `log_getRecordsCount` | `if (!mem_read(adr, tbuffer, n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4865` / `log_getRecordsCount` | `mem_read(adr, tbuffer, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4866` / `log_getRecordsCount` | `for (uint16_t a = 0; a + sizeof(cellLog_s) <= n; a += sizeof(cellLog_s))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4868` / `log_getRecordsCount` | `if (log_blank(cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4872` / `log_getRecordsCount` | `if (log_valid(cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4874` / `log_getRecordsCount` | `if (cell->index > log_lastIndex)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4889` / `log_readRecords` | `for (uint32_t adr = mem[MB_LOG].start; adr + sizeof(cell) <= endAdr && copied < count; adr += sizeof(cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4891` / `log_readRecords` | `if (!mem_read(adr, (uint8_t*)&cell, sizeof(cell)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4891` / `log_readRecords` | `mem_read(adr, (uint8_t*)&cell, sizeof(cell))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:4892` / `log_readRecords` | `if (log_blank(&cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4893` / `log_readRecords` | `if (!log_valid(&cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4894` / `log_readRecords` | `if (index++ < first)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4899` / `log_readRecords` | `return copied == count;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4930` / `user_storage_background_ready` | `return user_storage_command_ready() && !store_save_requested && !(log_pendingCount && (log_timeKnown \|\| device.worktime >= LOG_TIME_WAIT));` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4954` / `fw_async_active` | `return fw_job.phase != 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:4959` / `fw_async_cancel` | `if (fw_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4965` / `fw_async_quiet` | `if (fw_async_active() && fw_job.channel == channel)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4974` / `fw_async_begin` | `if (flash_owner \|\| store.job \|\| mem_config_result() \|\| w25_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4975` / `fw_async_begin` | `if (address < mem[MB_FIRMWARE].start + FWBLOCK_OFFSET \|\| address > mem[MB_FIRMWARE].start + mem[MB_FIRMWARE].size - 1024u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4978` / `fw_async_begin` | `if (!fw_job.data)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:4984` / `fw_async_begin` | `fw_job.phase = ((address + 4095u) & ~4095u) < address + 1024u ? FA_ERASE : FA_WRITE;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5001` / `fw_storage_poll` | `if (fw_job.channel == CH_USB && fw_job.usb_epoch != usb_bus_resets)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5002` / `fw_storage_poll` | `if (fw_job.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5003` / `fw_storage_poll` | `if (fw_job.waiting && w25_async_poll(now) == 1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5007` / `fw_storage_poll` | `if (fw_job.phase == FA_REPLY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5008` / `fw_storage_poll` | `if (!fw_job.quiet && fw_job.channel == CH_SERVER && comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5010` / `fw_storage_poll` | `if (!fw_job.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5013` / `fw_storage_poll` | `if (fw_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5015` / `fw_storage_poll` | `if (r == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5017` / `fw_storage_poll` | `if (r != 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5018` / `fw_storage_poll` | `if (fw_job.phase == FA_ERASE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5022` / `fw_storage_poll` | `if (!w25_read_available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5023` / `fw_storage_poll` | `if (fw_job.phase == FA_ERASE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5025` / `fw_storage_poll` | `if (!fw_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5028` / `fw_storage_poll` | `if (fw_job.phase == FA_WRITE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5029` / `fw_storage_poll` | `if (fw_job.offset < 1024u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5031` / `fw_storage_poll` | `if (!fw_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5038` / `fw_storage_poll` | `if (device.FWblockIdx < device.FWblockCount)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5041` / `fw_storage_poll` | `if (fw_job.phase == FA_HEADER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5042` / `fw_storage_poll` | `if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&header, sizeof(header)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5042` / `fw_storage_poll` | `mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&header, sizeof(header))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5043` / `fw_storage_poll` | `if (!header.firmwareSize \|\| (header.firmwareSize & 3u) \|\| header.firmwareSize > (uint32_t)device.FWblockCount * 1024u \|\| header.firmwareSize > mem[MB_FIRMWARE].size - FWBLOCK_OFFSET)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5052` / `fw_storage_poll` | `if (fw_job.phase == FA_CRC)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5053` / `fw_storage_poll` | `n = fw_job.remaining > 64u ? 64u : (uint16_t)fw_job.remaining;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5055` / `fw_storage_poll` | `if (!mem_read(fw_job.position, fw_job.data, n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5055` / `fw_storage_poll` | `mem_read(fw_job.position, fw_job.data, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5056` / `fw_storage_poll` | `for (uint16_t i = 0; i < n; i += 4u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5058` / `fw_storage_poll` | `CRC32(fw_job.crc, word)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5061` / `fw_storage_poll` | `if (!fw_job.remaining)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5066` / `fw_storage_poll` | `r = ++fw_write_errors < FW_WRITE_TRIES ? FW_BLOCK_BADWRITE : FW_BLOCK_END;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5068` / `fw_storage_poll` | `if (r == FW_BLOCK_END)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5096` / `log_async_active` | `return log_job.phase != 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5104` / `log_async_begin` | `if (log_job.address < mem[MB_LOG].start)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5117` / `log_async_step` | `if (!log_job.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5118` / `log_async_step` | `if (log_job.result == 3 && (int32_t)(now - log_job.retry_at) < 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5119` / `log_async_step` | `if (log_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5121` / `log_async_step` | `if (r == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5123` / `log_async_step` | `if (r != 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5127` / `log_async_step` | `if (!w25_read_available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5128` / `log_async_step` | `if (log_job.address < mem[MB_LOG].start \|\| log_job.address > mem[MB_LOG].start + mem[MB_LOG].size - sizeof(actual))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5132` / `log_async_step` | `switch (log_job.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5134` / `log_async_step` | `if (log_job.address >= mem[MB_LOG].start + sizeof(actual))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5135` / `log_async_step` | `if (!mem_read(log_job.address - sizeof(actual), (uint8_t*)&actual, sizeof(actual)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5135` / `log_async_step` | `mem_read(log_job.address - sizeof(actual), (uint8_t*)&actual, sizeof(actual))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5136` / `log_async_step` | `if (log_valid(&actual) && actual.index > log_job.previous)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5140` / `log_async_step` | `if (!mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5140` / `log_async_step` | `mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5141` / `log_async_step` | `if (!log_blank(&actual))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5142` / `log_async_step` | `if (log_valid(&actual) && actual.index > log_job.previous)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5145` / `log_async_step` | `if (log_job.previous && log_job.cell.index <= log_job.previous)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5146` / `log_async_step` | `crc16_calc((uint8_t*)&log_job.cell, sizeof(actual) - sizeof(actual.reserve))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5154` / `log_async_step` | `log_job.waiting = w25_async_start(log_job.address + (r ? 0u : sizeof(actual) - 4u), (uint8_t*)&log_job.cell + (r ? 0u : sizeof(actual) - 4u), r ? sizeof(actual) - 4u : 4u, now);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5155` / `log_async_step` | `(uint8_t*)&log_job.cell + (r ? 0u : sizeof(actual) - 4u), r ? sizeof(actual) - 4u : 4u, now);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5155` / `log_async_step` | `(uint8_t*)&log_job.cell + (r ? 0u : sizeof(actual) - 4u), r ? sizeof(actual) - 4u : 4u, now);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5156` / `log_async_step` | `if (!log_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5160` / `log_async_step` | `if (!mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5160` / `log_async_step` | `mem_read(log_job.address, (uint8_t*)&actual, sizeof(actual))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5161` / `log_async_step` | `if (!memcmp(&actual, &log_job.cell, sizeof(actual)) && log_valid(&actual))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5161` / `log_async_step` | `memcmp(&actual, &log_job.cell, sizeof(actual))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5168` / `log_async_step` | `if (log_job.phase == LA_VERIFY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5172` / `log_async_step` | `for (uint16_t i = 0; i < sizeof(actual); ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5173` / `log_async_step` | `if ((((uint8_t*)&actual)[i] & ((uint8_t*)&log_job.cell)[i]) != ((uint8_t*)&log_job.cell)[i])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5174` / `log_async_step` | `if (log_valid(&actual) && actual.index > log_job.previous)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5183` / `log_async_step` | `if (log_job.phase == LA_PREVIOUS \|\| log_job.phase == LA_FIND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5192` / `log_storage_poll` | `if (!log_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5193` / `log_storage_poll` | `if (log_pendingCount && (log_job.result != 3 \|\| (int32_t)(now - log_job.retry_at) >= 0))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5197` / `log_storage_poll` | `if (log_async_step(now) != 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5207` / `log_flushPending` | `if (store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5208` / `log_flushPending` | `while (log_pendingCount)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5209` / `log_flushPending` | `if (!log_addEvent(&log_pending[0]))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5220` / `log_setTime` | `if (!log_timeKnown)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5221` / `log_setTime` | `for (uint8_t i = log_async_active() ? 1u : 0u;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5221` / `log_setTime` | `for (uint8_t i = log_async_active() ? 1u : 0u; i < log_pendingCount; i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5233` / `log_checkPending` | `if (log_pendingCount && (log_timeKnown \|\| device.worktime >= LOG_TIME_WAIT))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5239` / `log_put` | `if (!store_runtime && (log_timeKnown \|\| device.worktime >= LOG_TIME_WAIT))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5241` / `log_put` | `if (!log_pendingCount && log_addEvent(cell))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5243` / `log_put` | `if (!store_runtime && log_pendingCount == LOG_PENDING_MAX)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5244` / `log_put` | `if (log_pendingCount == LOG_PENDING_MAX)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5247` / `log_put` | `if (!log_timeKnown)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5290` / `log_addSystemEventID` | `if (id != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5309` / `log_addControlEvent` | `if (id != NULL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5366` / `user_getPhotoCount` | `if (cleanup)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5367` / `user_getPhotoCount` | `device.newAddressPhoto = cleanup == 1 ? adr : UINT32_MAX;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5370` / `user_getPhotoCount` | `if (!photo_recover())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5371` / `user_getPhotoCount` | `while (adr + sizeof(h) <= end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5372` / `user_getPhotoCount` | `if (!mem_read(adr, (uint8_t*)&h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5372` / `user_getPhotoCount` | `mem_read(adr, (uint8_t*)&h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5373` / `user_getPhotoCount` | `if (h.datetime == 0xFFFFFFFFu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5374` / `user_getPhotoCount` | `if (!h.size \|\| h.size > 65535u \|\| end - adr < PHOTO_RECORD_HEAD \|\| h.size > end - adr - PHOTO_RECORD_HEAD)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5378` / `user_getPhotoCount` | `if (photo_confirmed_offset(adr, (uint16_t)h.size) < h.size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5378` / `user_getPhotoCount` | `photo_confirmed_offset(adr, (uint16_t)h.size)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5392` / `user_checkFirmware` | `if (!size \|\| (size & 3u))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5413` / `user_checkFirmware` | `while (size > 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5414` / `user_checkFirmware` | `readed = (size > 128) ? 128 : size;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5415` / `user_checkFirmware` | `if (!mem_read (adr, bf, readed))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5415` / `user_checkFirmware` | `mem_read (adr, bf, readed)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5417` / `user_checkFirmware` | `for (i = 0; i < (readed/4); i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5418` / `user_checkFirmware` | `CRC32(crcvalue, bf32[i])` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5425` / `user_checkFirmware` | `if (crcvalue != crc)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5434` / `CRC32` | `CRC32(uint32_t crc, uint32_t data)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5436` / `CRC32` | `for (uint8_t i = 0; i < 32; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5437` / `CRC32` | `if (crc&0x80000000)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5496` / `user_send_usbDiag` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5530` / `user_send_commDiag` | `for (uint8_t i = 0; i < 10u; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5532` / `user_send_commDiag` | `for (uint8_t i = 0; i < SLM_STEP_COUNT; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5537` / `user_send_commDiag` | `crc16_calc(&send_buffer[4], len + 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5547` / `user_comm_local_time` | `{ return device.datetime >= ID_TIME_VALID ? device.datetime : UINT32_MAX;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5547` / `user_comm_local_time` | `return device.datetime >= ID_TIME_VALID ? device.datetime : UINT32_MAX;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5560` / `user_comm_balance` | `device.balance = rubles < 1 ? 1u : rubles > 65000 ? 65000u : (uint16_t)rubles;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5560` / `user_comm_balance` | `device.balance = rubles < 1 ? 1u : rubles > 65000 ? 65000u : (uint16_t)rubles;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5562` / `user_comm_balance` | `if (rubles < 100)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5574` / `user_comm_link` | `if (kind >= sizeof(first))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5576` / `user_comm_link` | `if (kind <= COMM_LINK_LOST)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5582` / `user_comm_busy` | `return device.FWinProgress != 0 \|\| comm_call_active();` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5605` / `comm_split` | `size_t len = colon ? (size_t)(colon - server) : strlen(server);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5606` / `comm_split` | `if (len >= sizeof(comm_host[n]))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5608` / `comm_split` | `comm_port[n] = colon ? (uint16_t)atoi(colon + 1) : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5613` / `user_comm_server` | `return address \|\| (!config.useServer1 && config.useServer2);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5620` / `comm_endpoint` | `comm_split(n, n \|\| (!config.useServer1 && config.useServer2) ? config.server2 : config.server1);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5627` / `comm_text` | `switch (field)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5633` / `comm_text` | `if (*p++ == ',')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5633` / `comm_text` | `while (part && p < end && *p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5634` / `comm_text` | `if (part)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5635` / `comm_text` | `while (p < end && *p && *p != ',' && n + 1u < sizeof(comm_credential))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5638` / `comm_text` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5645` / `comm_number` | `switch (field)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5651` / `comm_number` | `return config.netType < sizeof(rat) ? rat[config.netType] : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5651` / `comm_number` | `return config.netType < sizeof(rat) ? rat[config.netType] : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5653` / `comm_number` | `return config.useServer2 && comm_host[1][0] && comm_port[1];` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5656` / `comm_number` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5668` / `comm_gpio` | `if (high)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5682` / `user_comm_boot` | `comm_split(0, config.useServer1 \|\| !config.useServer2 ? config.server1 : config.server2);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5684` / `user_comm_boot` | `if (!user_comm_start(&comm_settings, &handlers, timers_getMillis()))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5694` / `store_usb_session` | `if (store.usb_epoch == usb_bus_resets)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5700` / `store_usb_session` | `if (store.job && store.channel == CH_USB)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5708` / `store_marker` | `uint32_t mark = cmd ? STORE_CLEAR_MARK \| cmd : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5715` / `store_snapshot` | `crc16_calc((uint8_t*)&config, sizeof(config) - 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5716` / `store_snapshot` | `crc16_calc((uint8_t*)&device, sizeof(device) - 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5725` / `store_restore` | `if (mark != (STORE_CLEAR_MARK \| 10u) && mark != (STORE_CLEAR_MARK \| 9u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5727` / `store_restore` | `start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5728` / `store_restore` | `limit = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5729` / `store_restore` | `if (end < start \|\| end > limit)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5739` / `store_upload_request` | `uint8_t slot = (cmd == 10 ? 0 : 2) + (cch == CH_USB ? 0 : 1);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5739` / `store_upload_request` | `uint8_t slot = (cmd == 10 ? 0 : 2) + (cch == CH_USB ? 0 : 1);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5740` / `store_upload_request` | `if (store.job \|\| mem_config_result())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5750` / `store_ack` | `uint16_t *pending = cmd == 10 ? log_unacked : temp_unacked;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5751` / `store_ack` | `uint8_t channel = cch == CH_USB ? 0 : 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5752` / `store_ack` | `if (!pending[channel] \|\| store.repeat)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5758` / `store_ack` | `if (accepted)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5759` / `store_ack` | `else store.seen &= ~(1u << ((cmd == 10 ? 0 : 2) + channel));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5766` / `store_finish` | `bool active = cmd == 10 ? device.status.readyLogData \|\| device.logsCount : device.status.readyID \|\| device.tempIDsCount;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5767` / `store_finish` | `if (!active && !upload_index(cmd, CH_SERVER) && !upload_index(cmd, CH_USB))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5768` / `store_finish` | `uint32_t start = mem[cmd == 10 ? MB_LOG : MB_TEMPID].start;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5769` / `store_finish` | `uint32_t end = start + mem[cmd == 10 ? MB_LOG : MB_TEMPID].size;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5770` / `store_finish` | `uint32_t used = cmd == 10 ? device.newAddressLog : device.newAddressTempID;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5771` / `store_finish` | `if (used > start && used < end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5776` / `store_finish` | `if (!store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5784` / `store_command_start` | `if (store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5794` / `store_config_complete` | `if (comm_modem_state() == SLM_IDLE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5800` / `store_config_start` | `if (store.job \|\| !store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5809` / `store_reply` | `if(!quiet && cch==CH_SERVER && comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5810` / `store_reply` | `if(!quiet && cmd==12)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5810` / `store_reply` | `if(service_crc_begin_config(cch,id))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5811` / `store_reply` | `if(quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5812` / `store_reply` | `if(cmd==10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5813` / `store_reply` | `if(cmd==9)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5821` / `user_storage_poll` | `if(idindex_runtime_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5822` / `user_storage_poll` | `if(service_crc_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5823` / `user_storage_poll` | `if(expiry_runtime_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5824` / `user_storage_poll` | `if(upload_runtime_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5825` / `user_storage_poll` | `if(idclear_runtime_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5826` / `user_storage_poll` | `if(fw_service_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5831` / `user_storage_poll` | `if (table_runtime_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5832` / `user_storage_poll` | `if (photo_storage_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5833` / `user_storage_poll` | `if (fw_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5834` / `user_storage_poll` | `if (log_async_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5835` / `user_storage_poll` | `if (result == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5836` / `user_storage_poll` | `if (store.job == US_REPLY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5837` / `user_storage_poll` | `if (store.job == US_RECOVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5842` / `user_storage_poll` | `uint32_t start = mem[store.clear == 10 ? MB_LOG : MB_TEMPID].start;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5843` / `user_storage_poll` | `if ((int32_t)(now - store.retry_at) < 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5844` / `user_storage_poll` | `if (!w25_read_data(store.erase, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5844` / `user_storage_poll` | `w25_read_data(store.erase, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/user_fun_runtime.c:5845` / `user_storage_poll` | `for (uint16_t i = 0; i < sizeof(b); ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5845` / `user_storage_poll` | `if (b[i] != 255u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5847` / `user_storage_poll` | `if (!store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5851` / `user_storage_poll` | `if (!(store.erase & 4095u) \|\| store.erase >= store.end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5856` / `user_storage_poll` | `if (store.job == US_ERASE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5857` / `user_storage_poll` | `if ((int32_t)(now - store.retry_at) < 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5858` / `user_storage_poll` | `if (store.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5860` / `user_storage_poll` | `if (r == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5862` / `user_storage_poll` | `if (r != 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5866` / `user_storage_poll` | `if (store.erase < store.end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5868` / `user_storage_poll` | `if (!store.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5872` / `user_storage_poll` | `if (store.clear == 10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5878` / `user_storage_poll` | `if (!store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5881` / `user_storage_poll` | `if (result == 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5883` / `user_storage_poll` | `if (store.job == US_MARK \|\| store.job == US_CLEAN \|\| store.job == US_RECOVER_CLEAN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5889` / `user_storage_poll` | `if (job == US_SAVE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5890` / `user_storage_poll` | `if (job == US_ACK) store.seen &= ~(1u << ((store.cmd == 10 ? 0 : 2) + (store.channel == CH_USB ? 0 : 1)));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5890` / `user_storage_poll` | `if (job == US_ACK) store.seen &= ~(1u << ((store.cmd == 10 ? 0 : 2) + (store.channel == CH_USB ? 0 : 1)));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5890` / `user_storage_poll` | `if (job == US_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5891` / `user_storage_poll` | `if ((job == US_CONFIG \|\| job == US_COMMAND) && !store.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5895` / `user_storage_poll` | `if (result == 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5897` / `user_storage_poll` | `if (job == US_CONFIG)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5901` / `user_storage_poll` | `if (job == US_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5902` / `user_storage_poll` | `uint16_t *pending = store.cmd == 10 ? log_unacked : temp_unacked;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5904` / `user_storage_poll` | `pending[store.channel == CH_USB ? 0 : 1] = 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5906` / `user_storage_poll` | `if (job == US_MARK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5908` / `user_storage_poll` | `if (job == US_CLEAN \|\| job == US_RECOVER_CLEAN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5910` / `user_storage_poll` | `if (job == US_CONFIG \|\| job == US_COMMAND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5911` / `user_storage_poll` | `if (job == US_RESET)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5912` / `user_storage_poll` | `if (log_pendingCount \|\| store_save_requested)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5920` / `user_storage_poll` | `if (store.job == US_MARK \|\| store.job == US_CLEAN \|\| store.job == US_RECOVER_CLEAN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5921` / `user_storage_poll` | `if ((int32_t)(now - store.retry_at) >= 0 && !store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5924` / `user_storage_poll` | `if (store.job \|\| !w25_read_available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5925` / `user_storage_poll` | `if (log_pendingCount && store.clear != 10 && (store_reset_requested \|\| (!store_save_requested && (log_timeKnown \|\| device.worktime >= LOG_TIME_WAIT))))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5929` / `user_storage_poll` | `if (store_reset_requested)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5931` / `user_storage_poll` | `if (log_pendingCount)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/user_fun_runtime.c:5932` / `user_storage_poll` | `if (store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5933` / `user_storage_poll` | `if (store_save_requested)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/user_fun_runtime.c:5934` / `user_storage_poll` | `if (store_snapshot())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |

## User/c/table_runtime.inc

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/table_runtime.inc:13` / `table_runtime_active` | `return table_job.phase!=0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:16` / `table_runtime_quiet` | `if (table_job.phase && table_job.channel==channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:20` / `table_runtime_cancel` | `if (!table_job.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:26` / `table_chunk` | `if (!w25_read_data(table_job.sector+off,page,64))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:26` / `table_chunk` | `w25_read_data(table_job.sector+off,page,64)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/table_runtime.inc:27` / `table_chunk` | `if (table_job.cmd==16)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:28` / `table_chunk` | `for (uint16_t i=0;i<64 && off+i<mem[MB_HOLIDAY].size;i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:29` / `table_chunk` | `page[i]=off+i<table_job.bytes ? table_job.records[off+i] : 255;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:32` / `table_chunk` | `for (uint16_t i=0;i<table_job.batch;i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:35` / `table_chunk` | `if (table_job.cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:37` / `table_chunk` | `if (address<table_job.sector+off \|\| address>=table_job.sector+off+64)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:39` / `table_chunk` | `if (table_job.cell_size>64-local)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:40` / `table_chunk` | `if (table_job.cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:45` / `table_chunk` | `if (sjr.phase==16)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:46` / `table_chunk` | `if (old->index==0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:49` / `table_chunk` | `if (index>table_job.last)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:50` / `table_chunk` | `if ((uint32_t)index*32>table_job.used)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:52` / `table_chunk` | `memcmp(old->ID,record+2,10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/table_runtime.inc:54` / `table_chunk` | `if (keep)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:56` / `table_chunk` | `if (sjr.phase==16)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:63` / `table_runtime_begin` | `uint8_t cmd=ptr[4],mb=cmd==13 ? MB_ID : cmd==14 ? MB_SHEDULE : cmd==15 ? MB_GROUP : MB_HOLIDAY;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:63` / `table_runtime_begin` | `uint8_t cmd=ptr[4],mb=cmd==13 ? MB_ID : cmd==14 ? MB_SHEDULE : cmd==15 ? MB_GROUP : MB_HOLIDAY;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:63` / `table_runtime_begin` | `uint8_t cmd=ptr[4],mb=cmd==13 ? MB_ID : cmd==14 ? MB_SHEDULE : cmd==15 ? MB_GROUP : MB_HOLIDAY;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:64` / `table_runtime_begin` | `uint16_t size=cmd==13 ? sizeof(cellID_s) : cmd==14 ? sizeof(cellShedule_s) : cmd==15 ? sizeof(cellGroup_s) : sizeof(cellHoliday_s);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:64` / `table_runtime_begin` | `uint16_t size=cmd==13 ? sizeof(cellID_s) : cmd==14 ? sizeof(cellShedule_s) : cmd==15 ? sizeof(cellGroup_s) : sizeof(cellHoliday_s);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:65` / `table_runtime_begin` | `cmd==15 ? sizeof(cellGroup_s) : sizeof(cellHoliday_s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:67` / `table_runtime_begin` | `if (!user_storage_command_ready() \|\| !bytes \|\| bytes>2048 \|\| !mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:67` / `table_runtime_begin` | `!bytes` | **дубль** | Только !bytes: все четыре вызова из user_sync требуют ptr[9]!=0; User/c/user_fun_runtime.c:3679,3695,3711 — sync_cells_valid для 13/14/15; :3729 — явная проверка для 16. Размеры 32/32/16/4, uint8_t count: произведение ненулевое и не переполняет uint16_t. 16 CRC-valid zero-count входов и три bypass-мутации sync_cells_valid: Tests/asm/test_stage3_entry_guards.py. Проверка сохранена: оба шага −8 Б отменены после красных host-проверок; см. отчёт. |
| `User/c/table_runtime.inc:69` / `table_runtime_begin` | `if (!snapshot)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:76` / `table_runtime_begin` | `if (cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:88` / `table_scan_begin` | `if (table_job.cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:89` / `table_scan_begin` | `if (!table_job.failed && !table_job.cancelled && id_used_bytes!=UINT32_MAX)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:97` / `table_scan_begin` | `if (table_job.cmd==16 && !table_job.failed && !table_job.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:103` / `table_runtime_finish` | `if (!table_job.failed && !table_job.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:105` / `table_runtime_finish` | `if (cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:106` / `table_runtime_finish` | `if (cmd==14)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:107` / `table_runtime_finish` | `if (cmd==15)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:110` / `table_runtime_finish` | `if (!table_job.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:111` / `table_runtime_finish` | `if (!table_job.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:117` / `table_runtime_poll` | `if (table_job.phase==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:118` / `table_runtime_poll` | `if (table_job.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:120` / `table_runtime_poll` | `if (table_job.cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:121` / `table_runtime_poll` | `table_job.sector=table_job.cmd==16 ? table_job.start : (table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:124` / `table_runtime_poll` | `if (table_job.cmd==16)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:125` / `table_runtime_poll` | `while (table_job.position+table_job.batch<table_job.count)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:127` / `table_runtime_poll` | `if (table_job.cmd==13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:128` / `table_runtime_poll` | `if (((table_job.start+(uint32_t)(index-1)*table_job.cell_size)&~4095u)!=table_job.sector)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:131` / `table_runtime_poll` | `if (!sj_runtime_begin(table_job.sector,table_chunk,0))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:135` / `table_runtime_poll` | `if (table_job.phase==2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:137` / `table_runtime_poll` | `if (result==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:139` / `table_runtime_poll` | `if (result!=2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:141` / `table_runtime_poll` | `if (sj_runtime_blocked())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:146` / `table_runtime_poll` | `if (table_job.position==table_job.count \|\| table_job.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:150` / `table_runtime_poll` | `if (table_job.phase==3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:155` / `table_runtime_poll` | `if (table_job.offset==table_job.sector)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:156` / `table_runtime_poll` | `crc16_ffInit(mem[table_job.mb].size-table_job.offset,table_job.crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/table_runtime.inc:157` / `table_runtime_poll` | `if (table_job.batch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:163` / `table_runtime_poll` | `if (table_job.sector-table_job.offset<n)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:164` / `table_runtime_poll` | `if (!w25_read_data(table_job.start+table_job.offset,b,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:164` / `table_runtime_poll` | `w25_read_data(table_job.start+table_job.offset,b,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/table_runtime.inc:167` / `table_runtime_poll` | `crc16_calcInit(b,n,table_job.crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/table_runtime.inc:168` / `table_runtime_poll` | `for (uint16_t i=0;i<n;i+=32)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:168` / `table_runtime_poll` | `if (table_job.batch)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:170` / `table_runtime_poll` | `if (!fast_all_ff(b+i,32))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:171` / `table_runtime_poll` | `if (cell->index==0xffff)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:178` / `table_runtime_poll` | `while (--chunks && timers_getMillis()-started<1u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:181` / `table_runtime_poll` | `if (table_job.phase==4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:182` / `table_runtime_poll` | `if (!table_job.quiet && table_job.channel==CH_SERVER && comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/table_runtime.inc:183` / `table_runtime_poll` | `if (!table_job.reply_wait)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/table_runtime.inc:184` / `table_runtime_poll` | `if ((int32_t)(now-table_job.reply_until)<0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |

## User/c/photo_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/photo_runtime.c:41` / `photo_alloc` | `if (!photo)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:42` / `photo_alloc` | `return photo != NULL;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:54` / `rs485_baud` | `switch (config.reserv[0])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:59` / `rs485_baud` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:65` / `rs485_init` | `if (config.rs485Mode > 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:66` / `rs485_init` | `if (config.reserv[0] > 4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:70` / `rs485_init` | `for (i = 0; i < 8; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:71` / `rs485_init` | `if (config.rs485Periphery[i].type == 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:72` / `rs485_init` | `if (config.rs485Periphery[i].type)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:74` / `rs485_init` | `if (camera && other)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:83` / `rs485_set_config` | `memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:85` / `rs485_set_config` | `if (changed && photo && photo->stage)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:90` / `rs485_set_config` | `if (!photo \|\| !photo->stage)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:100` / `photo_fail` | `while (*why && n < 40)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:108` / `photo_fail` | `for (i = 0; i < sizeof(photo->rxHead) && i < bytes; i++)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:124` / `photo_issue` | `switch (photo->stage)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:132` / `photo_issue` | `if (photo->stage == P_SYNC)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:142` / `photo_storage_end` | `return mem[MB_PHOTO].start + (mem[MB_PHOTO].size >= 8192u ? mem[MB_PHOTO].size - 8192u : 0);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:142` / `photo_storage_end` | `return mem[MB_PHOTO].start + (mem[MB_PHOTO].size >= 8192u ? mem[MB_PHOTO].size - 8192u : 0);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:149` / `photo_backup_crc` | `while (n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:150` / `photo_backup_crc` | `step = n > sizeof(b) ? sizeof(b) : n;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:151` / `photo_backup_crc` | `if (!mem_read(address, b, step))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:151` / `photo_backup_crc` | `mem_read(address, b, step)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:152` / `photo_backup_crc` | `crc32_update(crc, b, step)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:161` / `photo_cleanup_active` | `if (mem[MB_PHOTO].size < 8192u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:162` / `photo_cleanup_active` | `if (!mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:162` / `photo_cleanup_active` | `mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:163` / `photo_cleanup_active` | `return marker == 0u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:168` / `photo_recover` | `if (mem[MB_PHOTO].size < 8192u \|\| photo_cleanup_active())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:169` / `photo_recover` | `if (!mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:169` / `photo_recover` | `mem_read(scratch + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:170` / `photo_recover` | `if (h[3] != 0x50484F54u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:171` / `photo_recover` | `if (h[0] < mem[MB_PHOTO].start \|\| h[0] >= scratch \|\| (h[0] & 4095u) \|\| !h[1] \|\| h[1] >= MEMSECTORSIZE \|\| (!photo_restore_phase && (!photo_backup_crc(scratch, (uint16_t)h[1], &crc) \|\| crc != h[2])))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:182` / `photo_frame` | `if (photo->stage == P_DATA)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:185` / `photo_frame` | `if (p[0] != 0xAA)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:186` / `photo_frame` | `if (p[1] == 0x0F)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:187` / `photo_frame` | `if (photo->stage == P_SYNC)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:188` / `photo_frame` | `if (p[1] == 0x0E && p[2] == 0x0D)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:189` / `photo_frame` | `if (p[1] == 0x0D)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:190` / `photo_frame` | `if (!photo->syncAck \|\| !photo->syncSeen)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:194` / `photo_frame` | `if (photo->stage == P_GET && p[1] == 0x0A && p[2] == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:196` / `photo_frame` | `if (!photo_prepare(size))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:197` / `photo_frame` | `if (p[1] == 0x0E && ((photo->stage == P_INIT && p[2] == 1) \|\| (photo->stage == P_SIZE && p[2] == 6) \|\| (photo->stage == P_SNAP && p[2] == 5)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:209` / `photo_confirmed_offset` | `photo_confirmed_offset(uint32_t address, uint16_t size)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:213` / `photo_confirmed_offset` | `if (!mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:213` / `photo_confirmed_offset` | `mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:214` / `photo_confirmed_offset` | `if (full == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:215` / `photo_confirmed_offset` | `for (i = 0; i < PHOTO_ACK_SLOTS; i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:216` / `photo_confirmed_offset` | `if (!mem_read(address + sizeof(photoParam_s) + 8u * i, (uint8_t*)entry, sizeof(entry)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:216` / `photo_confirmed_offset` | `mem_read(address + sizeof(photoParam_s) + 8u * i, (uint8_t*)entry, sizeof(entry))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.c:217` / `photo_confirmed_offset` | `if (entry[0] == 0xFFFFu && entry[1] == 0xFFFFu && entry[2] == 0xFFFFu && entry[3] == 0xFFFFu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:219` / `photo_confirmed_offset` | `if (entry[2] == 0 && entry[3] == 0 && (uint16_t)(entry[0] ^ entry[1]) == 0xFFFFu && entry[0] <= size && entry[0] > done)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.c:235` / `photo_connection_lost` | `if (photo && photo->channel == CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.c:236` / `photo_connection_lost` | `if (photo && photo->channel==CH_SERVER)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |

## User/c/photo_runtime.inc

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/photo_runtime.inc:29` / `photo_storage_busy` | `return pr.phase!=0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:33` / `photo_storage_cancel` | `if (pr.task==PT_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:34` / `photo_storage_cancel` | `if (photo)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:34` / `photo_storage_cancel` | `if (photo->stage)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:43` / `pr_fail` | `if (photo)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:44` / `pr_fail` | `if (photo->stage)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:46` / `pr_fail` | `if ((pr.task==PT_REQUEST \|\| pr.task==PT_ACK) && !pr.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:53` / `pr_write` | `if (!n \|\| n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:54` / `pr_write` | `if (p!=sjr.data)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:68` / `pr_scan_finish` | `if (pr.task==PT_SEND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:71` / `pr_scan_finish` | `if (pr.task==PT_START)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:72` / `pr_scan_finish` | `if (pr.task==PT_REQUEST && pr.requested_number!=0xffffffffu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:73` / `pr_scan_finish` | `if (photo && !pr.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:76` / `pr_scan_finish` | `if (pr.task==PT_REQUEST \|\| pr.task==PT_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:82` / `pr_start` | `for (uint8_t i=0;i<8;i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:82` / `pr_start` | `if (config.rs485Periphery[i].type==3 && config.rs485Periphery[i].address==address)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:83` / `pr_start` | `if (!found \|\| config.rs485Mode!=1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:84` / `pr_start` | `if (!user_storage_command_ready() \|\| !pr.initialized \|\| pr.phase \|\| pr.fault \|\| pr.cleanup \|\| device.status.readyToUpdate \|\| !photo_alloc() \|\| photo->stage)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:85` / `pr_start` | `if ((!device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start) \|\| device.newAddressPhoto>photo_storage_end() \|\| photo_storage_end()-device.newAddressPhoto<=PHOTO_RECORD_HEAD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:86` / `pr_start` | `if (pr.last_number==0xfffffffeu \|\| device.datetime==0xffffffffu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:93` / `pr_prepare` | `if (!size \|\| size>65535 \|\| photo->address>end \|\| end-photo->address<PHOTO_RECORD_HEAD \|\| size>end-photo->address-PHOTO_RECORD_HEAD)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:97` / `pr_prepare` | `if (!pr.prefix)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:103` / `pr_data_ready` | `if (used>photo->size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:105` / `pr_data_ready` | `if (used==photo->size && photo_storage_end()-end>=sizeof(photoParam_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:106` / `pr_data_ready` | `if (end<=photo_erased_to)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:107` / `pr_data_ready` | `if (photo_erased_to>=photo_storage_end())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:112` / `pr_frame` | `if (n<6 \|\| n>64 \|\| pr.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:114` / `pr_frame` | `for (uint16_t i=0;i<n-2;i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:115` / `pr_frame` | `if (id!=photo->packet \|\| !len \|\| len>58 \|\| n!=len+6 \|\| p[n-2]!=sum \|\| p[n-1] \|\| len>photo->size-photo->written)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:121` / `pr_request` | `if (size && size!=6)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:122` / `pr_request` | `if (!user_storage_command_ready() \|\| !pr.initialized \|\| pr.phase \|\| pr.fault \|\| pr.cleanup \|\| device.status.readyToUpdate \|\| channel==CH_SMS \|\| !photo_alloc() \|\| photo->stage)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:124` / `pr_request` | `if (size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:132` / `photo_request_answer` | `if ((uint8_t)r!=255)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:142` / `pr_cleanup_begin` | `if (last<=first \|\| last>end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:143` / `pr_cleanup_begin` | `if (end-last>=sizeof(photoParam_s))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:145` / `pr_cleanup_begin` | `if (last> end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:152` / `photo_storage_poll` | `if (!pr.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:153` / `photo_storage_poll` | `if (photo && photo->channel==CH_USB && pr.usb_epoch!=usb_bus_resets && (pr.task==PT_REQUEST \|\| pr.task==PT_ACK \|\| pr.task==PT_SEND))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:154` / `photo_storage_poll` | `if (device.status.readyToUpdate)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:155` / `photo_storage_poll` | `if (pr.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:157` / `photo_storage_poll` | `if (r==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:159` / `photo_storage_poll` | `if (r!=2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:160` / `photo_storage_poll` | `if (!pr.erase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:161` / `photo_storage_poll` | `if (pr.erase \|\| pr.io_done==pr.io_len)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:164` / `photo_storage_poll` | `if (pr.cancelled && !pr.committed)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:165` / `photo_storage_poll` | `switch (pr.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:168` / `photo_storage_poll` | `if (!pr.erase && n>256u-((pr.io_address+pr.io_done)&255u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:170` / `photo_storage_poll` | `if (!w25_async_start(pr.io_address+pr.io_done,pr.erase ? 0 : sjr.data+pr.io_done,n,now)) { pr_fail();` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:170` / `photo_storage_poll` | `if (!w25_async_start(pr.io_address+pr.io_done,pr.erase ? 0 : sjr.data+pr.io_done,n,now))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:173` / `photo_storage_poll` | `if (!w25_read_data(scratch+4096+32,(uint8_t*)pr.h,12))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:173` / `photo_storage_poll` | `w25_read_data(scratch+4096+32,(uint8_t*)pr.h,12)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:174` / `photo_storage_poll` | `if (pr.h[0]==0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:178` / `photo_storage_poll` | `if (pr.h[1]!=~pr.h[2] \|\| pr.h[1]>scratch \|\| pr.h[1]<=mem[MB_PHOTO].start \|\| (pr.h[1]&4095u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:181` / `photo_storage_poll` | `if (!w25_read_data(scratch+4096,(uint8_t*)pr.h,16))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:181` / `photo_storage_poll` | `w25_read_data(scratch+4096,(uint8_t*)pr.h,16)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:182` / `photo_storage_poll` | `if (pr.h[3]==0x50484f54u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:183` / `photo_storage_poll` | `if (pr.h[0]<mem[MB_PHOTO].start \|\| pr.h[0]>=scratch \|\| (pr.h[0]&4095u) \|\| !pr.h[1] \|\| pr.h[1]>=4096)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:188` / `photo_storage_poll` | `if (!w25_read_data(scratch+4096+pr.offset,sjr.data,64))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:188` / `photo_storage_poll` | `w25_read_data(scratch+4096+pr.offset,sjr.data,64)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:189` / `photo_storage_poll` | `if (!fast_all_ff(sjr.data,64))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:190` / `photo_storage_poll` | `if (pr.offset==4096)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:194` / `photo_storage_poll` | `if (pr.scan+sizeof(photoParam_s)>scratch \|\| (pr.task==PT_SEND && pr.scan>=device.newAddressPhoto))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:195` / `photo_storage_poll` | `if (!w25_read_data(pr.scan,(uint8_t*)pr.h,8))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:195` / `photo_storage_poll` | `w25_read_data(pr.scan,(uint8_t*)pr.h,8)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:196` / `photo_storage_poll` | `if (pr.h[0]==0xffffffffu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:197` / `photo_storage_poll` | `if (!pr.h[1] \|\| pr.h[1]>65535 \|\| scratch-pr.scan<PHOTO_RECORD_HEAD \|\| pr.h[1]>scratch-pr.scan-PHOTO_RECORD_HEAD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:198` / `photo_storage_poll` | `if (pr.h[0]>pr.max_number)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:201` / `photo_storage_poll` | `if (!w25_read_data(pr.scan+PHOTO_RECORD_HEAD-1,sjr.data,1))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:201` / `photo_storage_poll` | `w25_read_data(pr.scan+PHOTO_RECORD_HEAD-1,sjr.data,1)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:202` / `photo_storage_poll` | `if (!sjr.data[0])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:206` / `photo_storage_poll` | `if (!w25_read_data(pr.scan+8+pr.slot,sjr.data,64))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:206` / `photo_storage_poll` | `w25_read_data(pr.scan+8+pr.slot,sjr.data,64)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:207` / `photo_storage_poll` | `for (uint8_t i=0;i<64;i+=8)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:210` / `photo_storage_poll` | `if (fast_all_ff(b,8))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:211` / `photo_storage_poll` | `if (!b[4] && !b[5] && !b[6] && !b[7] && (uint16_t)(value^inverse)==65535 && value<=pr.h[1] && value>pr.confirmed)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:213` / `photo_storage_poll` | `if (pr.slot==1024)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:215` / `photo_storage_poll` | `if (pr.confirmed<pr.h[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:215` / `photo_storage_poll` | `if (!pr.first)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:216` / `photo_storage_poll` | `if (pr.task==PT_REQUEST && pr.requested_number!=0xffffffffu && ((pr.requested_number && pr.requested_number==pr.h[0]) \|\| (!pr.requested_number && pr.confirmed<pr.h[1])))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:218` / `photo_storage_poll` | `if (pr.requested_offset>pr.h[1] \|\| (pr.requested_offset>pr.confirmed && pr.requested_offset!=pr.h[1] && pr.free_slot==65535))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:219` / `photo_storage_poll` | `if (!pr.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:225` / `photo_storage_poll` | `if (pr.task==PT_SEND && pr.confirmed<pr.h[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:226` / `photo_storage_poll` | `uint16_t off=pr.scan==photo->readAddress ? photo->offset : 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:227` / `photo_storage_poll` | `if (off<pr.confirmed)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:228` / `photo_storage_poll` | `if (off>=pr.h[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:229` / `photo_storage_poll` | `if (pr.send_size>512)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:234` / `photo_storage_poll` | `if (pr.cleanup \|\| !pr.initialized \|\| (!device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start) \|\| device.newAddressPhoto>scratch \|\| scratch-device.newAddressPhoto<=PHOTO_RECORD_HEAD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:236` / `photo_storage_poll` | `photo->stamp=device.datetime ? device.datetime : 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:237` / `photo_storage_poll` | `if (pr.last_number>=photo->stamp)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:238` / `photo_storage_poll` | `if (photo->stamp==0xffffffffu \|\| !photo->stamp)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:244` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:245` / `photo_storage_poll` | `if (!w25_read_data(pr.scan,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:245` / `photo_storage_poll` | `w25_read_data(pr.scan,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:246` / `photo_storage_poll` | `if (!fast_all_ff(sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:248` / `photo_storage_poll` | `if (pr.scan==pr.limit)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:253` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:254` / `photo_storage_poll` | `if (!w25_read_data((photo->address&~4095u)+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:254` / `photo_storage_poll` | `w25_read_data((photo->address&~4095u)+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:255` / `photo_storage_poll` | `crc32_update(pr.crc,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:259` / `photo_storage_poll` | `if (pr.offset==pr.prefix)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:263` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:264` / `photo_storage_poll` | `if (!w25_read_data(scratch+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:264` / `photo_storage_poll` | `w25_read_data(scratch+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:265` / `photo_storage_poll` | `crc32_update(pr.crc,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:266` / `photo_storage_poll` | `if (pr.offset==pr.prefix)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:267` / `photo_storage_poll` | `if (pr.crc!=pr.expected)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:275` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:276` / `photo_storage_poll` | `if (!w25_read_data(scratch+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:276` / `photo_storage_poll` | `w25_read_data(scratch+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:277` / `photo_storage_poll` | `crc32_update(pr.crc,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:278` / `photo_storage_poll` | `if (pr.offset==pr.h[1])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:278` / `photo_storage_poll` | `if (pr.crc!=pr.h[2])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:282` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:283` / `photo_storage_poll` | `if (!w25_read_data(scratch+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:283` / `photo_storage_poll` | `w25_read_data(scratch+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:287` / `photo_storage_poll` | `if (pr.offset==pr.h[1])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:291` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:292` / `photo_storage_poll` | `if (!w25_read_data(pr.h[0]+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:292` / `photo_storage_poll` | `w25_read_data(pr.h[0]+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:293` / `photo_storage_poll` | `crc32_update(pr.crc,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:294` / `photo_storage_poll` | `if (pr.offset==pr.h[1])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:294` / `photo_storage_poll` | `if (pr.crc!=pr.h[2])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:298` / `photo_storage_poll` | `if (pr.committed)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:300` / `photo_storage_poll` | `if (pr.task==PT_PREP && !pr.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:301` / `photo_storage_poll` | `if (pr.cancelled)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:306` / `photo_storage_poll` | `if (photo->written!=photo->size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:321` / `photo_storage_poll` | `if (photo)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:324` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:325` / `photo_storage_poll` | `if (!n \|\| pr.offset+n>256)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:326` / `photo_storage_poll` | `if (!w25_read_data(scratch+4096+44+pr.offset,sjr.data,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:326` / `photo_storage_poll` | `w25_read_data(scratch+4096+44+pr.offset,sjr.data,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:327` / `photo_storage_poll` | `for (uint16_t i=0;i<n;i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:327` / `photo_storage_poll` | `if (sjr.data[i]!=0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:329` / `photo_storage_poll` | `if (mem[MB_PHOTO].start+(uint32_t)pr.offset*4096==pr.limit)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:337` / `photo_storage_poll` | `if (!w25_read_data(pr.scan,(uint8_t*)pr.h,8))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:337` / `photo_storage_poll` | `w25_read_data(pr.scan,(uint8_t*)pr.h,8)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:338` / `photo_storage_poll` | `if (!pr.h[1] \|\| pr.h[1]>65535 \|\| pr.scan>pr.target \|\| pr.scan>scratch \|\| scratch-pr.scan<PHOTO_RECORD_HEAD \|\| pr.h[1]>scratch-pr.scan-PHOTO_RECORD_HEAD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:339` / `photo_storage_poll` | `if (pr.scan==pr.target && pr.requested_offset<=pr.target_done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:342` / `photo_storage_poll` | `if (pr.scan<pr.target \|\| pr.requested_offset==pr.target_size)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:351` / `photo_storage_poll` | `if (pr.scan==pr.target)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:355` / `photo_storage_poll` | `if (!pr.quiet && photo->channel==CH_SERVER && comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:356` / `photo_storage_poll` | `if (!pr.reply_wait)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:357` / `photo_storage_poll` | `if ((int32_t)(now-pr.reply_until)<0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:360` / `photo_storage_poll` | `if (!pr.quiet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:362` / `photo_storage_poll` | `photo->readAddress=pr.first ? pr.first : device.newAddressPhoto;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:362` / `photo_storage_poll` | `photo->offset=pr.first ? pr.first_offset : 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:367` / `photo_storage_poll` | `if (!pr.tx)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:368` / `photo_storage_poll` | `if (!pr.tx)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:369` / `photo_storage_poll` | `uint16_t len=pr.send_size ? pr.send_size+14 : 4;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:371` / `photo_storage_poll` | `pr.tx[9]=pr.send_size ? (device.countPhoto>255 ? 255 : (uint8_t)device.countPhoto) : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:371` / `photo_storage_poll` | `pr.tx[9]=pr.send_size ? (device.countPhoto>255 ? 255 : (uint8_t)device.countPhoto) : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:372` / `photo_storage_poll` | `if (pr.send_size)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:375` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:376` / `photo_storage_poll` | `if (n && !w25_read_data(pr.scan+PHOTO_RECORD_HEAD+pr.send_offset+pr.offset,pr.tx+20+pr.offset,n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:376` / `photo_storage_poll` | `w25_read_data(pr.scan+PHOTO_RECORD_HEAD+pr.send_offset+pr.offset,pr.tx+20+pr.offset,n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:378` / `photo_storage_poll` | `if (pr.offset==pr.send_size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:381` / `photo_storage_poll` | `n=(pr.send_size ? pr.send_size+16 : 6)-pr.offset;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:381` / `photo_storage_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:382` / `photo_storage_poll` | `crc16_calcInit(pr.tx+4+pr.offset,n,pr.send_crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/photo_runtime.inc:383` / `photo_storage_poll` | `if (pr.offset==(pr.send_size ? pr.send_size+16 : 6)) { memcpy(pr.tx+4+pr.offset,&pr.send_crc,2);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:383` / `photo_storage_poll` | `if (pr.offset==(pr.send_size ? pr.send_size+16 : 6))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:386` / `photo_storage_poll` | `if (pr.quiet \|\| !photo->pending)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:387` / `photo_storage_poll` | `n=pr.send_size ? pr.send_size+22 : 12;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:389` / `photo_storage_poll` | `if (!ok)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:390` / `photo_storage_poll` | `if (!pr.reply_wait)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:391` / `photo_storage_poll` | `if ((int32_t)(now-pr.reply_until)<0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:394` / `photo_storage_poll` | `if (pr.send_size)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:396` / `photo_storage_poll` | `if (photo->offset==pr.h[1])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:401` / `photo_storage_poll` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:406` / `pr_poll` | `if (pr.phase \|\| !user_storage_background_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:408` / `pr_poll` | `if (!pr.initialized && !pr.fault)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:409` / `pr_poll` | `if (photo)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:410` / `pr_poll` | `if (photo->pending && photo->channel==CH_USB && pr.usb_epoch!=usb_bus_resets)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:411` / `pr_poll` | `if (device.status.readyToUpdate)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:413` / `pr_poll` | `if (photo->stage && uart_getstate(PHOTO_PORT) && !uart_isReceiving(PHOTO_PORT))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:414` / `pr_poll` | `if (n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:415` / `pr_poll` | `for (uint16_t i=0;i<n && photo->stage && !pr.phase;i++)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:416` / `pr_poll` | `if (photo->used>=64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:417` / `pr_poll` | `if (photo->rxBytes<sizeof(photo->rxHead))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:418` / `pr_poll` | `if (photo->rxBytes!=65535)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:420` / `pr_poll` | `if (photo->stage==P_DATA && photo->used>=4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:420` / `pr_poll` | `if (want>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:421` / `pr_poll` | `if (photo->used==want)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:425` / `pr_poll` | `if (pr.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:426` / `pr_poll` | `if (photo->stage && (uint32_t)(now-photo->started)>=60000u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:427` / `pr_poll` | `if (photo->stage && photo->stage!=P_PREP && (int32_t)(now-photo->deadline)>=0 && (photo->stage==P_DATA \|\| (uint32_t)(now-photo->last)>=50u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:429` / `pr_poll` | `if (photo->stage!=P_DATA \|\| pr_data_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:430` / `pr_poll` | `if (photo->retry++>=(photo->stage==P_SYNC ? 30 : 3)) photo_fail("no answer");` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:430` / `pr_poll` | `if (photo->retry++>=(photo->stage==P_SYNC ? 30 : 3))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:434` / `pr_poll` | `if (pr.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/photo_runtime.inc:435` / `pr_poll` | `if (photo->pending)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:436` / `pr_poll` | `if ((uint32_t)(now-photo_pending_since)>=60000 \|\| (photo->channel==CH_SERVER && !comm_channel_ready()))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:440` / `pr_poll` | `if (!photo->stage && !photo->pending && !pr.phase)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/photo_runtime.inc:442` / `pr_poll` | `if (!pr.phase && !pr.fault && !device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start && (!photo \|\| !photo->stage))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |

## User/c/memory_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/memory_runtime.c:12` / `sj_io_read` | `return sj_span(adr, n) && w25_read_data(adr, p, n);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:12` / `sj_io_read` | `w25_read_data(adr, p, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:13` / `sj_io_write` | `return sj_span(adr, n) && w25_write_data(adr, (uint8_t*)p, n);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:14` / `sj_io_erase` | `return !(adr & 4095u) && sj_span(adr, 4096) && w25_erase_sector(adr);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:20` / `mem_tables_ready` | `return flash_read_allowed() && !table_fault && !sj_runtime_blocked() && (mem_storage_ready \|\| (!store_runtime && mem_recover()));` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:24` / `mem_guard` | `if (n > 0xFFFFFFFFu - adr)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:25` / `mem_guard` | `if (n == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:27` / `mem_guard` | `for (sector = adr & ~4095u; ; sector += MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:28` / `mem_guard` | `if (sj_destination(sector))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:29` / `mem_guard` | `if (sector == last)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:34` / `mem_reserved` | `return n && (n > 0xFFFFFFFFu - adr \|\| (adr < SJ_META + SJ_SECTOR && adr + n > SJ_DATA));` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:38` / `mem_replace_sector` | `if (!sj_destination(adr) \|\| image == NULL)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:47` / `mem_replace_pages` | `if (!sj_destination(adr) \|\| reader == NULL)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:62` / `mem_overlay` | `uint32_t from = off > start ? off : start;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:64` / `mem_overlay` | `if (end > start + size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:65` / `mem_overlay` | `if (from < end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:71` / `mem_image_page` | `if (!w25_read_data(image->source + off, page, SJ_PAGE))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:71` / `mem_image_page` | `w25_read_data(image->source + off, page, SJ_PAGE)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:91` / `cfg_image_crc` | `crc32_update(0xFFFFFFFFu, (uint8_t*)&generation, 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:92` / `cfg_image_crc` | `for (off = 0; off < MEMSECTORSIZE; off += sizeof(b))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:93` / `cfg_image_crc` | `if (!w25_read_data(address + off, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:93` / `cfg_image_crc` | `w25_read_data(address + off, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:94` / `cfg_image_crc` | `crc32_update(c, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:102` / `cfg_valid` | `if (!w25_read_data(address + MEMSECTORSIZE, (uint8_t*)h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:102` / `cfg_valid` | `w25_read_data(address + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:103` / `cfg_valid` | `if (h[0] != CFG_MAGIC \|\| h[3] != ~h[2])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:104` / `cfg_valid` | `if (!cfg_image_crc(address, h[1], &crc) \|\| crc != h[2])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:113` / `cfg_select` | `if (vb && (!va \|\| (int32_t)(b - a) > 0))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:116` / `cfg_select` | `if (va)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:122` / `cfg_pages_equal` | `for (uint16_t off = 0; off < MEMSECTORSIZE; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:124` / `cfg_pages_equal` | `if (!mem_image_page(image, off, page))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:125` / `cfg_pages_equal` | `for (uint16_t i = 0; i < SJ_PAGE; i += sizeof(check))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:126` / `cfg_pages_equal` | `if (!w25_read_data(address + off + i, check, sizeof(check)) \|\| memcmp(page + i, check, sizeof(check)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:126` / `cfg_pages_equal` | `w25_read_data(address + off + i, check, sizeof(check))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:127` / `cfg_pages_equal` | `memcmp(page + i, check, sizeof(check))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:137` / `cfg_equal_flash` | `for (off = 0; off < MEMSECTORSIZE; off += sizeof(left))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:138` / `cfg_equal_flash` | `if (!w25_read_data(a + off, left, sizeof(left)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:138` / `cfg_equal_flash` | `w25_read_data(a + off, left, sizeof(left))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:139` / `cfg_equal_flash` | `if (!w25_read_data(b + off, right, sizeof(right)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:139` / `cfg_equal_flash` | `w25_read_data(b + off, right, sizeof(right))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:140` / `cfg_equal_flash` | `if (memcmp(left, right, sizeof(left)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:140` / `cfg_equal_flash` | `memcmp(left, right, sizeof(left))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:148` / `cfg_legacy_crc` | `for (off = 0; off < 510; off += count)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:150` / `cfg_legacy_crc` | `if (count > sizeof(b))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:151` / `cfg_legacy_crc` | `if (!w25_read_data(MEM_CONFIG_LEGACY + offset + off, b, count))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:151` / `cfg_legacy_crc` | `w25_read_data(MEM_CONFIG_LEGACY + offset + off, b, count)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:152` / `cfg_legacy_crc` | `crc16_calcInit(b, count, crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:154` / `cfg_legacy_crc` | `if (!w25_read_data(MEM_CONFIG_LEGACY + offset + 510, (uint8_t*)&expected, 2))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:154` / `cfg_legacy_crc` | `w25_read_data(MEM_CONFIG_LEGACY + offset + 510, (uint8_t*)&expected, 2)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:155` / `cfg_legacy_crc` | `return crc == expected;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:161` / `cfg_publish_pages` | `uint32_t target = active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:163` / `cfg_publish_pages` | `if (!w25_erase_sector(target + MEMSECTORSIZE))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:164` / `cfg_publish_pages` | `if (!w25_read_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:164` / `cfg_publish_pages` | `w25_read_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:165` / `cfg_publish_pages` | `if (h[0] != 0xFFFFFFFFu \|\| h[1] != 0xFFFFFFFFu \|\| h[2] != 0xFFFFFFFFu \|\| h[3] != 0xFFFFFFFFu)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:166` / `cfg_publish_pages` | `if (!w25_erase_sector(target))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:168` / `cfg_publish_pages` | `crc32_update(0xFFFFFFFFu, (uint8_t*)&h[1], 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:169` / `cfg_publish_pages` | `for (uint16_t off = 0; off < MEMSECTORSIZE; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:171` / `cfg_publish_pages` | `if (!mem_image_page(image, off, page))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:172` / `cfg_publish_pages` | `crc32_update(crc, page, sizeof(page))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:173` / `cfg_publish_pages` | `if (!w25_write_data(target + off, page, sizeof(page)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:175` / `cfg_publish_pages` | `if (!cfg_pages_equal(target, image))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:177` / `cfg_publish_pages` | `if (!w25_write_data(target + MEMSECTORSIZE, (uint8_t*)h, sizeof(h)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:178` / `cfg_publish_pages` | `if (!cfg_valid(target, &check) \|\| check != h[1])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:185` / `cfg_publish` | `mem_image_s image = {source, 0, p, NULL, p == NULL ? 0 : MEMSECTORSIZE, 0};` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:196` / `cfg_prepare` | `if (cfg_legacy_checked)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:198` / `cfg_prepare` | `if (cfg_legacy_crc(0) && (active == MEM_CONFIG_LEGACY \|\| cfg_legacy_crc(512)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:199` / `cfg_prepare` | `if (active == MEM_CONFIG_LEGACY \|\| !cfg_equal_flash(active, MEM_CONFIG_LEGACY))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:200` / `cfg_prepare` | `if (!cfg_publish(NULL, MEM_CONFIG_LEGACY, active, generation))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:213` / `cfg_blank_sector` | `for (off = 0; off < MEMSECTORSIZE; off += sizeof(b))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:214` / `cfg_blank_sector` | `if (!w25_read_data(address + off, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:214` / `cfg_blank_sector` | `w25_read_data(address + off, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:215` / `cfg_blank_sector` | `if (!fast_all_ff(b, sizeof(b)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:222` / `cfg_mirror` | `if (cfg_pages_equal(MEM_CONFIG_LEGACY, image))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:223` / `cfg_mirror` | `if (!w25_erase_sector(0x1CD000u) \|\| !cfg_blank_sector(0x1CD000u) \|\| !w25_erase_sector(0x1CE000u) \|\| !cfg_blank_sector(0x1CE000u))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:226` / `cfg_mirror` | `for (uint16_t off = 1024; off < MEMSECTORSIZE; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:228` / `cfg_mirror` | `if (!mem_image_page(image, off, page) \|\| !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:230` / `cfg_mirror` | `for (uint16_t off = 512; off < 1024; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:231` / `cfg_mirror` | `if (!mem_image_page(image, off, page) \|\| !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:233` / `cfg_mirror` | `for (uint16_t off = 512; off < MEMSECTORSIZE; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:234` / `cfg_mirror` | `if (!mem_image_page(image, off, page))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:235` / `cfg_mirror` | `for (uint16_t i = 0; i < SJ_PAGE; i += sizeof(check))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:236` / `cfg_mirror` | `if (!w25_read_data(MEM_CONFIG_LEGACY + off + i, check, sizeof(check)) \|\| memcmp(page + i, check, sizeof(check)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:236` / `cfg_mirror` | `w25_read_data(MEM_CONFIG_LEGACY + off + i, check, sizeof(check))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:236` / `cfg_mirror` | `memcmp(page + i, check, sizeof(check))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:239` / `cfg_mirror` | `for (uint16_t off = 0; off < 512; off += SJ_PAGE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:240` / `cfg_mirror` | `if (!mem_image_page(image, off, page) \|\| !w25_write_data(MEM_CONFIG_LEGACY + off, page, sizeof(page)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:247` / `cfg_commit_parts` | `if (!cfg_prepare())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:250` / `cfg_commit_parts` | `if (active == MEM_CONFIG_LEGACY)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:252` / `cfg_commit_parts` | `if (!cfg_publish_pages(&image, active, generation))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:254` / `cfg_commit_parts` | `if (!cfg_mirror(&image))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:257` / `cfg_commit_parts` | `if (!cfg_mirror(&image))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:258` / `cfg_commit_parts` | `if (cfg_pages_equal(active, &image))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:259` / `cfg_commit_parts` | `if (!cfg_publish_pages(&image, active, generation))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:264` / `cfg_range` | `return adr >= MEM_CONFIG_LEGACY && adr < MEM_CONFIG_LEGACY + MEMSECTORSIZE && size <= MEM_CONFIG_LEGACY + MEMSECTORSIZE - adr;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:271` / `cfg_read` | `if (store_runtime \|\| mem_config_result() == 1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:272` / `cfg_read` | `if (cfg_prepare())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:274` / `cfg_read` | `w25_read_data(active + adr - MEM_CONFIG_LEGACY, buff, size)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:278` / `mem_read` | `mem_read (Int32U adr, pInt8U buff, Int16U size)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:281` / `mem_read` | `if (cfg_range(adr, size))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:282` / `mem_read` | `return mem_guard(adr, size) && w25_read_data(adr, buff, size);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:282` / `mem_read` | `w25_read_data(adr, buff, size)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:290` / `mem_write` | `if (cfg_range(adr, size))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:291` / `mem_write` | `return !mem_reserved(adr, size) && mem_guard(adr, size) && w25_write_data(adr, buff, size);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:297` / `mem_patch2` | `if ((nfirst && first == NULL) \|\| (nsecond && second == NULL) \|\| size > MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:299` / `mem_patch2` | `if (cfg_range(adr, (uint16_t)size))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:301` / `mem_patch2` | `if (size > MEMSECTORSIZE - (adr - sector) \|\| !sj_destination(sector) \|\| !mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:315` / `mem_erase_sector` | `return !mem_reserved(adr, MEMSECTORSIZE) && mem_guard(adr, MEMSECTORSIZE) && w25_erase_sector(adr);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:331` / `mem_erase_mem` | `for (;fadr <= eadr; fadr += MEMSECTORSIZE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:334` / `mem_erase_mem` | `if (!mem_erase_sector(fadr))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:354` / `mem_config_reset` | `if (cfg_job.result == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:359` / `mem_config_begin` | `if (flash_owner \|\| table_fault \|\| sj_runtime_blocked() \|\| cfg_job.result \|\| w25_async_active() \|\| !cfg_legacy_checked \|\| !first \|\| !second)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:361` / `mem_config_begin` | `if (!cfg_job.snapshot)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:364` / `mem_config_begin` | `cfg_job.target = cfg_cached_active == MEM_CONFIG_SLOT0 ? MEM_CONFIG_SLOT1 : MEM_CONFIG_SLOT0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:373` / `cfg_async_page` | `if (cfg_job.offset < 1024u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:376` / `cfg_async_page` | `w25_read_data(cfg_job.source + cfg_job.offset, sjr.data, sizeof(sjr.data))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:382` / `mem_config_poll` | `if (cfg_job.result != 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:383` / `mem_config_poll` | `if (cfg_job.waiting)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:385` / `mem_config_poll` | `if (r == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:387` / `mem_config_poll` | `if (r != 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:388` / `mem_config_poll` | `if (cfg_job.first && cfg_job.state == CA_MIRROR0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:394` / `mem_config_poll` | `switch (cfg_job.state)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:396` / `mem_config_poll` | `if (!cfg_async_page() \|\| !w25_read_data(cfg_job.source + cfg_job.offset, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:396` / `mem_config_poll` | `w25_read_data(cfg_job.source + cfg_job.offset, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:397` / `mem_config_poll` | `if (memcmp(b, sjr.data, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:397` / `mem_config_poll` | `memcmp(b, sjr.data, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:398` / `mem_config_poll` | `if (!w25_read_data(MEM_CONFIG_LEGACY + cfg_job.offset, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:398` / `mem_config_poll` | `w25_read_data(MEM_CONFIG_LEGACY + cfg_job.offset, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:399` / `mem_config_poll` | `if (memcmp(b, sjr.data, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:399` / `mem_config_poll` | `memcmp(b, sjr.data, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:401` / `mem_config_poll` | `if (cfg_job.offset == MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:403` / `mem_config_poll` | `cfg_job.state = cfg_job.first ? CA_META : cfg_job.mirror_changed ? CA_MIRROR0 : cfg_job.changed ? CA_META : CA_DONE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:403` / `mem_config_poll` | `cfg_job.state = cfg_job.first ? CA_META : cfg_job.mirror_changed ? CA_MIRROR0 : cfg_job.changed ? CA_META : CA_DONE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:403` / `mem_config_poll` | `cfg_job.state = cfg_job.first ? CA_META : cfg_job.mirror_changed ? CA_MIRROR0 : cfg_job.changed ? CA_META : CA_DONE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:411` / `mem_config_poll` | `crc32_update(0xFFFFFFFFu, (uint8_t*)&cfg_job.header[1], 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:413` / `mem_config_poll` | `if (!cfg_async_page())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:414` / `mem_config_poll` | `address = (cfg_job.state == CA_COPY ? cfg_job.target : MEM_CONFIG_LEGACY) + cfg_job.offset;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:415` / `mem_config_poll` | `if (!w25_async_start(address, sjr.data, sizeof(sjr.data), now))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:416` / `mem_config_poll` | `if (cfg_job.state == CA_COPY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:416` / `mem_config_poll` | `crc32_update(cfg_job.header[2], sjr.data, sizeof(sjr.data))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:418` / `mem_config_poll` | `if (cfg_job.state == CA_TAIL && cfg_job.offset == MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:419` / `mem_config_poll` | `if (cfg_job.state == CA_DEVICE && cfg_job.offset == 1024)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:420` / `mem_config_poll` | `else if (cfg_job.state == CA_CONFIG && cfg_job.offset == 512) cfg_job.state = cfg_job.first \|\| !cfg_job.changed ? CA_DONE : CA_META;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:420` / `mem_config_poll` | `if (cfg_job.state == CA_CONFIG && cfg_job.offset == 512)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:421` / `mem_config_poll` | `if (cfg_job.state == CA_COPY && cfg_job.offset == MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:422` / `mem_config_poll` | `crc32_update(0xFFFFFFFFu, (uint8_t*)&cfg_job.header[1], 4)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:426` / `mem_config_poll` | `if (!w25_read_data(cfg_job.target + cfg_job.offset, b, sizeof(b)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:426` / `mem_config_poll` | `w25_read_data(cfg_job.target + cfg_job.offset, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:427` / `mem_config_poll` | `crc32_update(cfg_job.crc, b, sizeof(b))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/memory_runtime.c:428` / `mem_config_poll` | `if (cfg_job.offset == MEMSECTORSIZE)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:429` / `mem_config_poll` | `if (cfg_job.crc != cfg_job.header[2])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:434` / `mem_config_poll` | `if (!w25_async_start(cfg_job.target + MEMSECTORSIZE, (uint8_t*)cfg_job.header, sizeof(cfg_job.header), now))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:435` / `mem_config_poll` | `cfg_job.state = cfg_job.first ? CA_MIRROR0 : CA_DONE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:437` / `mem_config_poll` | `if (cfg_job.changed \|\| cfg_job.first)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/memory_runtime.c:439` / `mem_config_poll` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:441` / `mem_config_poll` | `if (!w25_async_start(address, NULL, 0, now))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:449` / `flash_claim` | `if (flash_owner && ((flash_owner ^ owner) & 0x7fu))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/memory_runtime.c:453` / `flash_release` | `if (!((flash_owner ^ owner) & 0x7fu))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |

## User/c/fw_service_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/fw_service_runtime.c:7` / `file scope` | `CRC32(uint32_t,uint32_t)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/fw_service_runtime.c:21` / `file scope` | `RT_ASSERT(fw_service_size,sizeof(fw_service_s)==128)` | **защита** | ABI, размер объекта или смещение поля C/ASM |
| `User/c/fw_service_runtime.c:25` / `fw_service_active` | `return fw_service_context && (fw_service_context->phase!=8 \|\| !store[18]);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:28` / `fw_service_config_result` | `if(fw_service_context && fw_service_context->phase==8 && (result==2 \|\| result==3))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:33` / `fw_service_quiet` | `if(fw_service_context && fw_service_context->channel==channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:36` / `fw_service_cancel` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:37` / `fw_service_cancel` | `if(p->pending)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:43` / `fw_service_begin` | `if(!store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:44` / `fw_service_begin` | `if(fw_service_context \|\| !user_storage_command_ready() \|\| !mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:45` / `fw_service_begin` | `if(fw_start_rejected(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:47` / `fw_service_begin` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:51` / `fw_service_begin` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:52` / `fw_service_begin` | `if(!p->reserve)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:54` / `fw_service_begin` | `if(rt_get16(data)!=1 && fw_same_download(rt_get16(data),rt_get32(data+2),rt_get32(data+6),rt_get32(data+10),rt_get16(data+14),channel))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:58` / `fw_service_begin` | `if(reason)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:62` / `fw_service_begin` | `if(rt_get16(data)==1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:75` / `fw_service_poll` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:76` / `fw_service_poll` | `if(p->pending)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:77` / `fw_service_poll` | `if(result==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:78` / `fw_service_poll` | `if(p->cancel)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:79` / `fw_service_poll` | `if(result!=2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:80` / `fw_service_poll` | `if(phase==4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:81` / `fw_service_poll` | `if(phase==5)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:82` / `fw_service_poll` | `if(phase==10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:85` / `fw_service_poll` | `p->phase=p->pos>=p->end?13:(p->pos&4095u)?11:10;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:85` / `fw_service_poll` | `p->phase=p->pos>=p->end?13:(p->pos&4095u)?11:10;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:89` / `fw_service_poll` | `if(p->cancel)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:90` / `fw_service_poll` | `switch(phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:92` / `fw_service_poll` | `address=mem[0].start+(phase==10?p->pos:0);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:96` / `fw_service_poll` | `if(!w25_async_start(address,source,(uint16_t)n,timers_getMillis()))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:99` / `fw_service_poll` | `if(n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:100` / `fw_service_poll` | `if(!w25_read_data(mem[1].start+p->pos,p->data,p->len))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:100` / `fw_service_poll` | `w25_read_data(mem[1].start+p->pos,p->data,p->len)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/fw_service_runtime.c:101` / `fw_service_poll` | `if(!p->pos)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:104` / `fw_service_poll` | `if(!w25_read_data(mem[phase==1?1:0].start,p->header,32)) goto read_error;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:104` / `fw_service_poll` | `if(!w25_read_data(mem[phase==1?1:0].start,p->header,32))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:104` / `fw_service_poll` | `w25_read_data(mem[phase==1?1:0].start,p->header,32)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/fw_service_runtime.c:105` / `fw_service_poll` | `if(rt_get32(p->header+20)==0xffffffffu \|\| rt_get32(p->header+12)==0xffffffffu \|\| rt_get16(p->header+5)==65535)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:106` / `fw_service_poll` | `reason=phase>=13?"CRC":"NO BACKUP";` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:109` / `fw_service_poll` | `if(!n \|\| (n&3u) \|\| n>0x17e00u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:110` / `fw_service_poll` | `p->phase=phase==13?14:2;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:112` / `fw_service_poll` | `if(n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:113` / `fw_service_poll` | `if(!w25_read_data(mem[phase==2?1:0].start+p->pos,p->data,p->len)) goto read_error;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:113` / `fw_service_poll` | `if(!w25_read_data(mem[phase==2?1:0].start+p->pos,p->data,p->len))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:113` / `fw_service_poll` | `w25_read_data(mem[phase==2?1:0].start+p->pos,p->data,p->len)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/fw_service_runtime.c:114` / `fw_service_poll` | `for(n=0;n<p->len;n+=4)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:114` / `fw_service_poll` | `CRC32(p->crc,rt_get32(p->data+n))` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/fw_service_runtime.c:115` / `fw_service_poll` | `if(p->pos<p->end)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:116` / `fw_service_poll` | `if(p->crc!=p->expected)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:117` / `fw_service_poll` | `if(phase==14)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:121` / `fw_service_poll` | `if(p->mode)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:124` / `fw_service_poll` | `if(p->channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:129` / `fw_service_poll` | `if(!saved)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:133` / `fw_service_poll` | `if(store[18])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:134` / `fw_service_poll` | `if(p->config_result!=2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:138` / `fw_service_poll` | `if(!p->quiet && !reply_send(p->data,p->len,p->channel))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:139` / `fw_service_poll` | `if(p->data[8]==11 && !p->data[10] && p->mode==1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:141` / `fw_service_poll` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:144` / `fw_service_poll` | `if(phase>=13)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:145` / `fw_service_poll` | `if(phase==2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:146` / `fw_service_poll` | `if(phase==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:148` / `fw_service_poll` | `bad_crc: reason=phase==14?"CRC":"BAD BACKUP";` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/fw_service_runtime.c:150` / `fw_service_poll` | `if(p->phase<10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:153` / `fw_service_poll` | `if(p->phase>=10 && p->phase<16)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:156` / `fw_service_poll` | `if(p->channel && !device.registredOnServer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/fw_service_runtime.c:159` / `fw_service_poll` | `crc16_calc(p->data+4,7)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |

## User/c/service_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/service_runtime.c:14` / `file scope` | `RT_ASSERT(service_crc_size,sizeof(service_crc_s)==312)` | **защита** | ABI, размер объекта или смещение поля C/ASM |
| `User/c/service_runtime.c:15` / `file scope` | `RT_ASSERT(service_crc_packet,offsetof(service_crc_s,packet)==96)` | **защита** | ABI, размер объекта или смещение поля C/ASM |
| `User/c/service_runtime.c:24` / `service_crc_quiet` | `if(service_crc_job && service_crc_job->channel==channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:28` / `sc_begin` | `if(channel>1 \|\| service_crc_job \|\| !user_storage_flash_ready() \|\| mem_config_result()==1)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:29` / `sc_begin` | `if(!mem_tables_ready() \|\| index>100 \|\| !(p=work_acquire(sizeof *p, WORK_SERVICE)))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:30` / `sc_begin` | `if(kind!=4) user_send_answer_comand(kind==5?17:18,(cmdCode_e)2,(dataChanel_e)channel,uniq);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:30` / `sc_begin` | `if(kind!=4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:35` / `sc_begin` | `p->length=kind==6?211:kind==15?14:21;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:35` / `sc_begin` | `p->length=kind==6?211:kind==15?14:21;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:38` / `sc_begin` | `if(kind==15)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:42` / `service_crc_begin_id` | `uint8_t service_crc_begin_id(uint8_t index,uint8_t channel,uint8_t uniq) { return sc_begin(index?15:6,channel,uniq,index);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:42` / `service_crc_begin_id` | `return sc_begin(index?15:6,channel,uniq,index);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:48` / `sc_setup` | `if(p->kind==5)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:49` / `sc_setup` | `if(p->kind==4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:50` / `sc_setup` | `offset=16000u*(p->kind==15?p->index-1:p->segment);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:52` / `sc_setup` | `if(part==2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:53` / `sc_setup` | `if(p->kind!=5)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:54` / `sc_setup` | `real=p->used<offset?0:p->used-offset;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:54` / `sc_setup` | `if(real>size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:58` / `sc_setup` | `if(part==6)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:65` / `service_crc_poll` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:66` / `service_crc_poll` | `if(p->phase==1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:67` / `service_crc_poll` | `if(p->phase==2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:69` / `service_crc_poll` | `if(p->real)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:70` / `service_crc_poll` | `if(!w25_read_available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:72` / `service_crc_poll` | `if(!p->waiting)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:73` / `service_crc_poll` | `if(now-p->wait_start>=6000u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:76` / `service_crc_poll` | `n=p->real>64?64:p->real;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:77` / `service_crc_poll` | `if(!w25_read_data(p->address,p->scratch,(uint16_t)n))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:77` / `service_crc_poll` | `w25_read_data(p->address,p->scratch,(uint16_t)n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/service_runtime.c:79` / `service_crc_poll` | `crc16_calcInit(p->scratch,(uint16_t)n,p->crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/service_runtime.c:80` / `service_crc_poll` | `if (--chunks && timers_getMillis()-started<1u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:83` / `service_crc_poll` | `if(p->erased)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:84` / `service_crc_poll` | `crc16_ffInit(p->erased,p->crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/service_runtime.c:86` / `service_crc_poll` | `if(p->kind==4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:90` / `service_crc_poll` | `out=p->packet+(p->kind==15?10:9+2*p->segment);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:91` / `service_crc_poll` | `if(p->kind!=15 && ++p->segment!=(p->kind==6?100:5)) { p->phase=1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:91` / `service_crc_poll` | `if(p->kind!=15 && ++p->segment!=(p->kind==6?100:5))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:95` / `service_crc_poll` | `if(p->phase==3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:96` / `service_crc_poll` | `crc16_calc(p->packet+4,p->length-6)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/service_runtime.c:98` / `service_crc_poll` | `if(p->channel==1 && comm_tx_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:99` / `service_crc_poll` | `if(reply_send(p->packet,p->length,p->channel))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:102` / `service_crc_poll` | `answer11(p->packet,p->packet[7],p->kind==5?17:p->kind==4?12:18,2);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:102` / `service_crc_poll` | `answer11(p->packet,p->packet[7],p->kind==5?17:p->kind==4?12:18,2);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:107` / `reply_send` | `if(!channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:108` / `reply_send` | `if(!out)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/service_runtime.c:110` / `reply_send` | `if(comm_tx_commit(size))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/service_runtime.c:118` / `answer11` | `crc16_calc(packet+4,13)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |

## User/c/service_ff.inc

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|

## User/c/upload_runtime.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `User/c/upload_runtime.c:15` / `file scope` | `RT_ASSERT(upload_size,sizeof(upload_s)==36)` | **защита** | ABI, размер объекта или смещение поля C/ASM |
| `User/c/upload_runtime.c:17` / `upload_runtime_active` | `return upload_runtime_context!=NULL;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:20` / `upload_runtime_cancel` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:22` / `upload_runtime_cancel` | `if(p->packet)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:25` / `upload_runtime_quiet` | `if(upload_runtime_context && upload_runtime_context->channel==channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:28` / `upload_runtime_begin` | `unsigned part=command==10?7:8;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:29` / `upload_runtime_begin` | `if(!store_runtime)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:30` / `upload_runtime_begin` | `if(upload_runtime_context \|\| !mem_tables_ready())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:31` / `upload_runtime_begin` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:32` / `upload_runtime_begin` | `if(!p->packet)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:33` / `upload_runtime_begin` | `if(!store_upload_request(command,channel,uniq))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:36` / `upload_runtime_begin` | `if(!work_upload_ack(command,(uint8_t)channel,p))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:47` / `upload_runtime_poll` | `if(!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:49` / `upload_runtime_poll` | `if(!p->channel && p->epoch!=usb_bus_resets)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:50` / `upload_runtime_poll` | `switch(p->phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:53` / `upload_runtime_poll` | `if(!done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:54` / `upload_runtime_poll` | `if(!w25_read_data(p->pos,b,32))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:54` / `upload_runtime_poll` | `w25_read_data(p->pos,b,32)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/upload_runtime.c:55` / `upload_runtime_poll` | `done=command==10?log_blank(b):((record[0]&65535u)==65535u);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:56` / `upload_runtime_poll` | `if(!done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:57` / `upload_runtime_poll` | `if(command!=10 \|\| log_valid(b))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:58` / `upload_runtime_poll` | `if(command==10 && log_lastIndex<record[0])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:64` / `upload_runtime_poll` | `if(command==10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:66` / `upload_runtime_poll` | `if(p->total<=p->first)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:68` / `upload_runtime_poll` | `if(!done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:74` / `upload_runtime_poll` | `if(n>(command==10?32:100)) n=command==10?32:100;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:74` / `upload_runtime_poll` | `if(n>(command==10?32:100)) n=command==10?32:100;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:74` / `upload_runtime_poll` | `if(n>(command==10?32:100))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:75` / `upload_runtime_poll` | `p->length=n*(command==10?32:10)+14;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:80` / `upload_runtime_poll` | `p->pos=command==10?mem[7].start:mem[8].start+32u*p->first;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:82` / `upload_runtime_poll` | `if(p->pos+32>p->limit)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:83` / `upload_runtime_poll` | `if(command!=10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:84` / `upload_runtime_poll` | `if(!w25_read_data(p->pos+2,p->packet+12+10*p->copied,10))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:84` / `upload_runtime_poll` | `w25_read_data(p->pos+2,p->packet+12+10*p->copied,10)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/upload_runtime.c:86` / `upload_runtime_poll` | `if(!w25_read_data(p->pos,b,32) \|\| log_blank(b))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:86` / `upload_runtime_poll` | `w25_read_data(p->pos,b,32)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/upload_runtime.c:87` / `upload_runtime_poll` | `if(!log_valid(b))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:88` / `upload_runtime_poll` | `if(p->seen++<p->first)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:91` / `upload_runtime_poll` | `if(++p->copied>=p->wanted)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:95` / `upload_runtime_poll` | `if(n>64)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:96` / `upload_runtime_poll` | `crc16_calcInit(p->packet+p->offset,n,p->crc)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `User/c/upload_runtime.c:97` / `upload_runtime_poll` | `if(p->offset>=p->length-2)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:100` / `upload_runtime_poll` | `if(!reply_send(p->packet,p->length,p->channel))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:101` / `upload_runtime_poll` | `if(!p->channel && p->epoch!=usb_bus_resets)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:102` / `upload_runtime_poll` | `if(!p->wanted)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `User/c/upload_runtime.c:103` / `upload_runtime_poll` | `(command==10?log_unacked:temp_unacked)[p->channel]=p->first+p->wanted;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:104` / `upload_runtime_poll` | `if(command==10)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:104` / `upload_runtime_poll` | `if(p->channel)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `User/c/upload_runtime.c:107` / `upload_runtime_poll` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |

## Modules/comm_at.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `Modules/comm_at.c:20` / `event` | `if (handlers.event)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:26` / `comm_at_equal` | `return count == n && !memcmp(data, text, n);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:26` / `comm_at_equal` | `memcmp(data, text, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_at.c:32` / `comm_at_prefix` | `return count >= n && !memcmp(data, text, n);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:32` / `comm_at_prefix` | `memcmp(data, text, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_at.c:41` / `comm_at_number` | `while (*pos < count && data[*pos] >= '0' && data[*pos] <= '9')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:43` / `comm_at_number` | `if (n > UINT32_MAX / 10u \|\| (n == UINT32_MAX / 10u && digit > UINT32_MAX % 10u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:45` / `comm_at_number` | `if (n > limit)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:48` / `comm_at_number` | `return *pos != begin;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:53` / `take` | `if (*pos >= count \|\| data[*pos] != c)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:60` / `comm_at_spaces` | `while (pos < count && data[pos] == ' ')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:73` / `fault` | `uint8_t kind = (state >= RUN && state <= COMPLETE) ? COMM_AT_DONE : state == IDLE ? COMM_AT_STREAM_ERROR : COMM_AT_NO_ANSWER;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:74` / `fault` | `: state == IDLE ? COMM_AT_STREAM_ERROR : COMM_AT_NO_ANSWER;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:82` / `expired` | `return (uint32_t)(now - started) >= duration;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:89` / `qird_request` | `const char *pattern = format == COMM_AT_SLM_QIRD ? "AT+QIRD=\002,\002" : "AT+QIRD=\002,\002,\002,\002";` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:91` / `qird_request` | `if (!comm_at_parse(command,(uint16_t)(command_count - 1u),pattern,values))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:93` / `qird_request` | `if (format == COMM_AT_M66_QIRD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:94` / `qird_request` | `if (values[0] > 1 \|\| !values[1] \|\| values[1] > 2 \|\| values[2] > 5)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:96` / `qird_request` | `if (values[0] > 7)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:97` / `qird_request` | `if (!values[index] \|\| values[index] > 1500)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:102` / `comm_at_init` | `if (in_poll)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:104` / `comm_at_init` | `if (callbacks)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:115` / `comm_at_field` | `for (i = 0; i < size; ++i)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:117` / `comm_at_field` | `return !host \|\| i != 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:117` / `comm_at_field` | `if (!c)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:118` / `comm_at_field` | `if (host ? !((c >= 'a' && c <= 'z') \|\| (c >= 'A' && c <= 'Z') \|\| (c >= '0' && c <= '9') \|\| c == '.' \|\| c == '-') : c < 32 \|\| c > 126 \|\| c == '"' \|\| c == '\\') return false;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:118` / `comm_at_field` | `if (host ? !((c >= 'a' && c <= 'z') \|\| (c >= 'A' && c <= 'Z') \|\| (c >= '0' && c <= '9') \|\| c == '.' \|\| c == '-') : c < 32 \|\| c > 126 \|\| c == '"' \|\| c == '\\')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:125` / `comm_at_sink` | `if (!in_poll)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:129` / `comm_at_busy` | `return state != IDLE \|\| in_poll;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:131` / `comm_at_prompt` | `return state == IDLE && result == COMM_AT_GOT_PROMPT;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:136` / `available` | `return !comm_at_busy() && !line_count && !cr && !comm_uart_peek(&view);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:147` / `comm_at_start` | `if (!available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:148` / `comm_at_start` | `if (!request \|\| !request->command \|\| request->count < 2u \|\| request->count >= sizeof(command) \|\| request->mode == COMM_AT_SEND \|\| request->mode > COMM_AT_PDU \|\| !request->timeout_ms \|\| request->timeout_ms > INT32_MAX \|\| !recovery \|\| recovery > INT32_MAX)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:152` / `comm_at_start` | `if (request->command[0] != 'A' \|\| request->command[1] != 'T')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:153` / `comm_at_start` | `for (i = 0; i < request->count; ++i)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:155` / `comm_at_start` | `if (value < ' ' \|\| value > '~')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:162` / `comm_at_start` | `if ((mode == COMM_AT_SLM_QIRD \|\| mode == COMM_AT_M66_QIRD) && (!handlers.binary \|\| !qird_request(mode)))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:164` / `comm_at_start` | `if (!comm_uart_tx(command, command_count))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:173` / `solicited` | `if ((state != RUN && state != DRAIN) \|\| command_count < 4u \|\| command[2] != '+')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:174` / `solicited` | `while (n < command_count && command[n] != '=' && command[n] != '?' && command[n] != '\r')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:177` / `solicited` | `return line_count > n && line[n] == ':' && !memcmp(line, command + 2, n);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:177` / `solicited` | `memcmp(line, command + 2, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_at.c:183` / `comm_at_choice` | `for (uint8_t i = 1; *names; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:185` / `comm_at_choice` | `if (count == n && !memcmp(data, names, n))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:185` / `comm_at_choice` | `memcmp(data, names, n)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_at.c:192` / `urc` | `return (line_count && line[0] == '+' && !solicited()) \|\| (state != RUN && state != DRAIN) \|\| comm_at_choice(line, line_count, "RING\0NO CARRIER\0Call Ready\0SMS Ready\0" "AT READY\0RDY\0SEND OK\0SEND FAIL\0");` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:202` / `m66_length` | `if (!comm_at_parse(p,n,"\003.\003.\003.\003:\004,TCP,\002",values) && !comm_at_parse(p,n,"\003.\003.\003.\003:\004,UDP,\002",values))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:205` / `m66_length` | `return values[4] && values[5] <= read_limit;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:209` / `binary_header` | `uint16_t pos = grammar == M66_HEADER ? 0u : comm_at_spaces(line, line_count, 6);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:211` / `binary_header` | `if (header)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:212` / `binary_header` | `if (mode == COMM_AT_M66_QIRD && pos == line_count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:216` / `binary_header` | `if (mode == COMM_AT_SLM_QIRD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:217` / `binary_header` | `if (!comm_at_number(line, line_count, &pos, read_limit, &n) \|\| pos != line_count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:220` / `binary_header` | `if (!m66_length(pos, &n))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:223` / `binary_header` | `grammar = n ? BODY : LINES;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:228` / `comm_at_payload` | `if (!available())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:229` / `comm_at_payload` | `if (!data \|\| !count \|\| !timeout_ms \|\| timeout_ms > INT32_MAX \|\| result != COMM_AT_GOT_PROMPT \|\| (finals != COMM_AT_SEND && finals != COMM_AT_TEXT))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:232` / `comm_at_payload` | `if (!comm_uart_tx(data, count))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:241` / `final` | `? "SEND OK\0SEND FAIL\0ERROR\0" : "OK\0ERROR\0");` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:242` / `final` | `if (matched) { *code = matched == 1 ? COMM_AT_OK : COMM_AT_ERROR;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:242` / `final` | `if (matched)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:244` / `final` | `if (!comm_at_prefix(line, line_count, "+CME ERROR:") && !comm_at_prefix(line, line_count, "+CMS ERROR:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:246` / `final` | `uint8_t letter = pos < line_count ? line[pos] \| 32u : 0;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:247` / `final` | `if (letter >= 'a' && letter <= 'z')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:252` / `final` | `if (!comm_at_number(line, line_count, &pos, UINT16_MAX, &value) \|\| pos != line_count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:266` / `complete_line` | `if (!line_count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:266` / `complete_line` | `if (pdu_body == 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:267` / `complete_line` | `if ((state == RUN \|\| state == SYNC_WAIT) && line_count + 1u == command_count && !memcmp(line, command, line_count))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:268` / `complete_line` | `memcmp(line, command, line_count)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_at.c:269` / `complete_line` | `if (pdu_body)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:270` / `complete_line` | `if (comm_at_prefix(line, line_count, "+QIRD:") && ((state != RUN && state != DRAIN && state != SYNC_SEND && state != SYNC_WAIT) \|\| (mode != COMM_AT_SLM_QIRD && mode != COMM_AT_M66_QIRD)))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:275` / `complete_line` | `if (final(&code))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:276` / `complete_line` | `if (state == RUN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:277` / `complete_line` | `if (code == COMM_AT_OK && ((mode == COMM_AT_SLM_QIRD && !header) \|\| grammar == M66_HEADER))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:279` / `complete_line` | `if (code == COMM_AT_CME && modem_code == UINT16_MAX)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:283` / `complete_line` | `if (state == SYNC_WAIT && sync_seen && code == COMM_AT_OK)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:285` / `complete_line` | `if (state == DRAIN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:288` / `complete_line` | `if ((state == RUN \|\| (state >= DRAIN && state <= SYNC_WAIT)) && (mode == COMM_AT_SLM_QIRD \|\| mode == COMM_AT_M66_QIRD))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:290` / `complete_line` | `if (comm_at_prefix(line, line_count, "+QIRD:") \|\| grammar == M66_HEADER)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:291` / `complete_line` | `if (grammar != M66_HEADER \|\| !urc())` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:294` / `complete_line` | `if (state == SYNC_WAIT && comm_at_prefix(line, line_count, "+CMEE:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:298` / `complete_line` | `if (urc())` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:299` / `complete_line` | `if (state == RUN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:301` / `complete_line` | `if (mode == COMM_AT_PDU && solicited())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:307` / `byte` | `if (grammar == BODY_CR \|\| grammar == BODY_LF)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:308` / `byte` | `if (value != (grammar == BODY_CR ? '\r' : '\n')) { fault(COMM_AT_BAD_LENGTH);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:308` / `byte` | `if (value != (grammar == BODY_CR ? '\r' : '\n'))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:309` / `byte` | `grammar = grammar == BODY_CR ? BODY_LF : LINES;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:312` / `byte` | `if (prompt_space)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:314` / `byte` | `if (value == ' ')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:316` / `byte` | `if (state == RUN && mode == COMM_AT_PROMPT && !line_count && !cr && value == '>')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:322` / `byte` | `if (value == '\r')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:323` / `byte` | `if (value == '\n')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:328` / `byte` | `if (!value \|\| cr)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:329` / `byte` | `if (line_count >= sizeof(line))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:331` / `byte` | `if (comm_at_prefix(line, line_count, "+CUSD:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:332` / `byte` | `if (!pdu_body \|\| state != RUN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:342` / `comm_at_poll` | `if (in_poll \|\| state == FAULT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:344` / `comm_at_poll` | `if (comm_uart_errors())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:345` / `comm_at_poll` | `if ((state >= RUN && state <= COMPLETE) && expired(now_ms, timeout))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:347` / `comm_at_poll` | `state = grammar != LINES \|\| line_count \|\| cr ? DRAIN : SYNC_SEND;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:352` / `comm_at_poll` | `if ((state >= DRAIN && state <= SYNC_COMPLETE) && expired(now_ms, recovery))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:358` / `comm_at_poll` | `while (budget && state != FAULT && (count = comm_uart_peek(&view)) != 0u)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:359` / `comm_at_poll` | `if (count > budget)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:360` / `comm_at_poll` | `if (grammar == BODY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:361` / `comm_at_poll` | `if (count > remaining)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:363` / `comm_at_poll` | `used = state != RUN ? count : handlers.binary(view, count);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:364` / `comm_at_poll` | `if (comm_uart_errors())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:365` / `comm_at_poll` | `if (used > count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:366` / `comm_at_poll` | `if (!used)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:368` / `comm_at_poll` | `if (!remaining)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:371` / `comm_at_poll` | `if (state == IDLE && !line_count && !cr)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:374` / `comm_at_poll` | `if (state == FAULT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_at.c:375` / `comm_at_poll` | `if (!comm_uart_consume(used) \|\| comm_uart_errors())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:379` / `comm_at_poll` | `if (state == IDLE && (line_count \|\| cr) && expired(now_ms, recovery))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:380` / `comm_at_poll` | `if (state == DRAIN && grammar == LINES && !line_count && !cr)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:381` / `comm_at_poll` | `if ((state == COMPLETE \|\| state == SYNC_COMPLETE \|\| state == SYNC_SEND) && comm_uart_tx_done() && !line_count && !cr && !comm_uart_peek(&view))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:383` / `comm_at_poll` | `if (state != SYNC_SEND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:386` / `comm_at_poll` | `if (done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:391` / `comm_at_poll` | `if (comm_uart_tx(command, command_count))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:401` / `comm_at_copy` | `while (pos < capacity && *text)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:402` / `comm_at_copy` | `if (pos < capacity)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:408` / `comm_at_decimal` | `while (divisor > value && divisor > 1u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:410` / `comm_at_decimal` | `if (pos >= capacity)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:413` / `comm_at_decimal` | `while (divisor)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:414` / `comm_at_decimal` | `if (pos < capacity)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:423` / `comm_at_parse` | `while (*pattern)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_at.c:425` / `comm_at_parse` | `if (token == '*')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:426` / `comm_at_parse` | `return pos == count \|\| data[pos] == ',';` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:426` / `comm_at_parse` | `if (token == '\|')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:427` / `comm_at_parse` | `if (token <= 6)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:428` / `comm_at_parse` | `if (token == 1 \|\| token >= 5)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:429` / `comm_at_parse` | `uint32_t limit = (token == 3 \|\| token == 6) ? 255u : token >= 4 ? UINT16_MAX : UINT32_MAX;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:429` / `comm_at_parse` | `uint32_t limit = (token == 3 \|\| token == 6) ? 255u : token >= 4 ? UINT16_MAX : UINT32_MAX;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:430` / `comm_at_parse` | `if (!comm_at_number(data, count, &pos, limit, out++))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:431` / `comm_at_parse` | `if (!take(data, count, &pos, token))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_at.c:433` / `comm_at_parse` | `return pos == count;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |

## Modules/comm_modem.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `Modules/comm_modem.c:32` / `file scope` | `typedef char slm_diag_budget[(sizeof(slm_diag_t) <= 128u) ? 1 : -1];` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:39` / `slm_elapsed` | `return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:39` / `slm_elapsed` | `return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:47` / `comm_modem_record` | `if (slm.attempt.recording)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:49` / `comm_modem_record` | `if (!(slm.attempt.status.completed_phases & (1u << slm.attempt.status.phase)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:53` / `comm_modem_record` | `if (next > SLM_STEP_COUNT)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:59` / `comm_modem_record` | `uint8_t phase = next == SLM_STEP_COUNT ? SLM_PHASE_COUNT : next < SLM_AT_READY ? SLM_PHASE_POWER : next >= SLM_TCP ? SLM_PHASE_TCP : (uint8_t)(slm.profile->steps[next - SLM_AT_READY].action >> 4);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:60` / `comm_modem_record` | `: next < SLM_AT_READY ? SLM_PHASE_POWER : next >= SLM_TCP ? SLM_PHASE_TCP : (uint8_t)(slm.profile->steps[next - SLM_AT_READY].action >> 4);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:60` / `comm_modem_record` | `: next < SLM_AT_READY ? SLM_PHASE_POWER : next >= SLM_TCP ? SLM_PHASE_TCP : (uint8_t)(slm.profile->steps[next - SLM_AT_READY].action >> 4);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:62` / `comm_modem_record` | `if (phase != slm.attempt.status.phase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:66` / `comm_modem_record` | `if (phase < SLM_PHASE_COUNT)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:72` / `comm_modem_record` | `if (!(slm.attempt.status.completed_phases & (1u << phase)))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:76` / `comm_modem_record` | `if (reason)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:78` / `comm_modem_record` | `if (slot >= sizeof(slm.diag.failure_step))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:81` / `comm_modem_record` | `slm.diag.failure_seconds[slot] = seconds > UINT16_MAX ? UINT16_MAX : (uint16_t)seconds;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:88` / `comm_modem_record` | `slm.diag.failure_next = slot + 1u == sizeof(slm.diag.failure_step) ? 0 : (uint8_t)(slot + 1u);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:90` / `comm_modem_record` | `if (slm.diag.failure_count < sizeof(slm.diag.failure_step))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:93` / `comm_modem_record` | `if (next == SLM_STEP_COUNT && slm.attempt.status.step == SLM_READ && !reason)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:96` / `comm_modem_record` | `if (slm.attempt.recording)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:106` / `slm_fail` | `if (slm.attempt.status.state == SLM_FAILED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:114` / `slm_event` | `if (kind == COMM_AT_URC)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:115` / `slm_event` | `for (uint8_t i = 0; i < COMM_URC_COUNT; ++i)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:115` / `slm_event` | `if (slm.urc[i])` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:116` / `slm_event` | `if (slm.side.running)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:117` / `slm_event` | `if (kind == COMM_AT_LINE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:117` / `slm_event` | `if (slm.side.req->line)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:118` / `slm_event` | `if (kind == COMM_AT_DONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:120` / `slm_event` | `if (detail == COMM_AT_GOT_PROMPT && slm.side.req->payload)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:122` / `slm_event` | `if (detail <= COMM_AT_TIMEOUT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:126` / `slm_event` | `if (kind == COMM_AT_DONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:130` / `slm_event` | `if (detail > COMM_AT_TIMEOUT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:132` / `slm_event` | `if (slm.transport_event && (slm.attempt.status.state == SLM_READY \|\| kind == COMM_AT_URC \|\| kind == COMM_AT_NO_ANSWER \|\| kind == COMM_AT_STREAM_ERROR))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:135` / `slm_event` | `if (slm.attempt.status.state == SLM_READY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:136` / `slm_event` | `if (kind == COMM_AT_NO_ANSWER \|\| kind == COMM_AT_STREAM_ERROR)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:138` / `slm_event` | `uint8_t reason = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:139` / `slm_event` | `if (!slm.transport_event)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:144` / `slm_event` | `if (p && comm_at_parse(p,n,slm.profile->pdp,&value) && value == 1)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:147` / `slm_event` | `if (kind == COMM_AT_NO_ANSWER \|\| kind == COMM_AT_STREAM_ERROR)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:149` / `slm_event` | `slm_fail(kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:151` / `slm_event` | `if (kind == COMM_AT_DONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:153` / `slm_event` | `if (slm.attempt.status.state != SLM_STARTING)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:156` / `slm_event` | `uint8_t action = step >= SLM_AT_READY ? slm.profile->steps[step - SLM_AT_READY].action & 15u : MODEM_OK;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:157` / `slm_event` | `if (comm_at_parse(p,n,slm.profile->pdp,values) && values[0] == 1)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:158` / `slm_event` | `if (step >= SLM_ACTIVATE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:159` / `slm_event` | `if (slm.attempt.status.state == SLM_STARTING)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:160` / `slm_event` | `switch (action)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:162` / `slm_event` | `if (comm_at_parse(p,n,slm.profile->network_reply[slm.attempt.query],values) && values[0] <= 2 && values[1] <= 5)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:164` / `slm_event` | `if (values[1] == 1 \|\| values[1] == 5)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:168` / `slm_event` | `if (comm_at_parse(p,n,slm.profile->rat,values) && values[0] <= 8)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:169` / `slm_event` | `slm.attempt.answer = values[0] == slm.settings.number(SLM_CFG_NETWORK) ? 1 : 2;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:172` / `slm_event` | `if (comm_at_prefix(p,n,slm.profile->port)) slm.attempt.answer = comm_at_equal(p + strlen(slm.profile->port),(uint16_t)(n - strlen(slm.profile->port)),slm.profile->port_ready) ? 1 : 2;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:172` / `slm_event` | `if (comm_at_prefix(p,n,slm.profile->port))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:175` / `slm_event` | `if (comm_at_prefix(p,n,slm.profile->model) \|\| (comm_at_prefix(p,n,"MEIG_") && comm_at_prefix(p + 5,(uint16_t)(n - 5),slm.profile->model)))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:181` / `slm_event` | `if (answer) slm.attempt.answer = answer < 4 ? answer : 4;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:181` / `slm_event` | `if (answer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:187` / `slm_event` | `if (n != 15)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:188` / `slm_event` | `for (uint16_t i = 0; i < n; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:188` / `slm_event` | `if (p[i] < '0' \|\| p[i] > '9')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:191` / `slm_event` | `if (slm.imei_found)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:195` / `slm_event` | `if (n != 15)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:196` / `slm_event` | `for (uint16_t i = 0; i < n; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:196` / `slm_event` | `if (p[i] < '0' \|\| p[i] > '9')` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:198` / `slm_event` | `slm.attempt.mnc = !memcmp(p,"250",3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:198` / `slm_event` | `memcmp(p,"250",3)` | **защита** | данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт |
| `Modules/comm_modem.c:215` / `slm_valid_settings` | `if (!slm.settings.number(SLM_CFG_PORT) \|\| (type != slm.profile->type && type != 0 && type != 255) \|\| slm.settings.number(SLM_CFG_NETWORK) > 5)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:217` / `slm_valid_settings` | `for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:220` / `slm_valid_settings` | `if (!p \|\| !capacity \|\| !comm_at_field(p, capacity, field == SLM_CFG_HOST))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:227` / `slm_side_cancel` | `if (slm.side.queued \|\| slm.side.running \|\| slm.side.prompted)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:250` / `slm_begin` | `if (power)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:262` / `comm_modem_start` | `if (slm.attempt.in_poll \|\| !selected \|\| !settings \|\| !settings->text \|\| !settings->number \|\| !handlers \|\| !handlers->gpio \|\| slm.attempt.status.state == SLM_STARTING \|\| slm.attempt.status.state == SLM_READY)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:267` / `comm_modem_start` | `if (!slm_valid_settings())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:279` / `comm_modem_restart` | `if (slm.attempt.in_poll \|\| !slm.profile \|\| !slm.gpio)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:280` / `comm_modem_restart` | `if (comm_power_v5_state() != COMM_PWR_READY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:281` / `comm_modem_restart` | `if (power)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:287` / `comm_modem_off` | `if (slm.attempt.in_poll)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:293` / `comm_modem_listen` | `if (slot < COMM_URC_COUNT)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:298` / `slm_side_allowed` | `return slm.attempt.status.state == SLM_READY \|\| (slm.attempt.status.state == SLM_STARTING && slm.attempt.status.step >= SLM_SIM);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:303` / `comm_modem_side` | `if (!req \|\| !req->cmd \|\| req->timeout >= COMM_T_COUNT \|\| slm.side.queued \|\| slm.side.running \|\| slm.side.prompted \|\| (slm.side.done && slm.side.req != req) \|\| !slm_side_allowed())` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:310` / `comm_modem_side_result` | `if (!slm.side.done \|\| slm.side.req != req)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:318` / `slm_side_issue` | `if (slm.side.prompted)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:320` / `slm_side_issue` | `if (r == COMM_AT_BUSY)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:323` / `slm_side_issue` | `if (!slm.side.queued \|\| comm_at_busy() \|\| comm_at_prompt())` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:324` / `slm_side_issue` | `if (!slm_side_allowed())` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:328` / `slm_side_issue` | `if (r == COMM_AT_BUSY)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:331` / `slm_side_issue` | `if (r == COMM_AT_ACCEPTED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:339` / `comm_modem_request` | `if (slm.side.prompted)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:340` / `comm_modem_request` | `if ((slm.attempt.status.state == SLM_STARTING && !slm.attempt.in_poll) \|\| (slm.attempt.status.state != SLM_STARTING && slm.attempt.status.state != SLM_READY))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:343` / `comm_modem_request` | `if (r == COMM_AT_ACCEPTED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:348` / `comm_modem_payload` | `if (slm.side.prompted)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:350` / `comm_modem_payload` | `if (r == COMM_AT_ACCEPTED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:355` / `comm_modem_result` | `if (!slm.attempt.done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:361` / `comm_modem_reserve` | `return slm.settings.number && slm.settings.number(SLM_CFG_RESERVE);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:368` / `comm_modem_build` | `uint16_t n = comm_at_copy(cmd, size, 0, *p && *p != 'E' ? "AT+" : "AT");` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:369` / `comm_modem_build` | `while (*p && n < size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:373` / `comm_modem_build` | `switch (token)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:382` / `comm_modem_build` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:384` / `comm_modem_build` | `n = text ? comm_at_copy(cmd, size, n, text) : comm_at_decimal(cmd, size, n, number);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:386` / `comm_modem_build` | `if (n < size)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:394` / `slm_issue` | `if (n >= sizeof(cmd))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:400` / `comm_modem_ready` | `return slm.attempt.status.state == SLM_READY;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:410` / `slm_apn` | `if (slm_text(SLM_CFG_APN)[0])` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:411` / `slm_apn` | `for (uint8_t i = 0; i < sizeof(apn_map) / sizeof(apn_map[0]); ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:412` / `slm_apn` | `if (slm.attempt.mnc == apn_map[i].mnc)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:417` / `slm_send` | `if (!slm_valid_settings())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:420` / `slm_send` | `if (result == COMM_AT_BUSY)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:421` / `slm_send` | `if (slm.attempt.now - slm.attempt.next >= comm_timeouts[COMM_T_AT_SIMPLE])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:422` / `slm_send` | `if (result != COMM_AT_ACCEPTED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:431` / `slm_completed` | `if (slm.attempt.result != COMM_AT_OK && !(action == MODEM_ANY && slm.attempt.result < COMM_AT_GOT_PROMPT))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:432` / `slm_completed` | `uint8_t reason = slm.attempt.result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : slm.attempt.result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:433` / `slm_completed` | `: slm.attempt.result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:434` / `slm_completed` | `if (action == MODEM_SIM && (slm.attempt.status.modem_code == 10 \|\| slm.attempt.answer == 4))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:435` / `slm_completed` | `if (action == MODEM_PDP && slm.attempt.result != COMM_AT_TIMEOUT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:436` / `slm_completed` | `if (action == MODEM_READY && slm.attempt.result <= COMM_AT_TIMEOUT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:441` / `slm_completed` | `switch (action)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:443` / `slm_completed` | `if (!slm.attempt.answer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:444` / `slm_completed` | `if (slm.attempt.query == 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:445` / `slm_completed` | `if (!slm.attempt.net_registered)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:450` / `slm_completed` | `if (!slm.attempt.answer) { slm_fail(action == MODEM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:450` / `slm_completed` | `if (!slm.attempt.answer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:453` / `slm_completed` | `if (slm.attempt.answer != 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:454` / `slm_completed` | `slm_fail(slm.attempt.answer == 2 ? SLM_ERR_PIN : slm.attempt.answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:454` / `slm_completed` | `slm_fail(slm.attempt.answer == 2 ? SLM_ERR_PIN : slm.attempt.answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:458` / `slm_completed` | `uint8_t alternate = action == MODEM_READY ? slm.settings.number(SLM_CFG_TYPE) == slm.profile->type : action == MODEM_NETWORK ? slm_text(SLM_CFG_APN)[0] != 0 : slm.attempt.answer == 1;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:459` / `slm_completed` | `: action == MODEM_NETWORK ? slm_text(SLM_CFG_APN)[0] != 0 : slm.attempt.answer == 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:460` / `slm_completed` | `if (action == MODEM_MODEL && slm.type_found)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:461` / `slm_completed` | `uint8_t next = alternate ? entry->alternate : entry->next;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:463` / `slm_completed` | `if (next == SLM_STEP_COUNT)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:467` / `comm_modem_poll` | `if (slm.attempt.in_poll \|\| slm.attempt.status.state == SLM_IDLE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:471` / `comm_modem_poll` | `if (slm.attempt.status.state == SLM_STARTING && slm.attempt.status.step <= SLM_POWER_BOOT)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:475` / `comm_modem_poll` | `if (p != slm.attempt.status.step + 1u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:476` / `comm_modem_poll` | `if (p == COMM_PWR_READY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:477` / `comm_modem_poll` | `comm_modem_record(p == COMM_PWR_READY ? SLM_AT_READY : (uint8_t)(p - 1u), 0, slm.attempt.status.modem_code);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_modem.c:483` / `comm_modem_poll` | `if (slm.attempt.status.state != SLM_STARTING)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:485` / `comm_modem_poll` | `if (slm.attempt.done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:486` / `comm_modem_poll` | `if (slm.attempt.status.state != SLM_STARTING)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_modem.c:487` / `comm_modem_poll` | `if (slm.attempt.status.step == SLM_AT_READY && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_AT_READY])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:490` / `comm_modem_poll` | `if (slm.attempt.status.step == SLM_NETWORK && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_NETWORK])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_modem.c:493` / `comm_modem_poll` | `if (!comm_at_busy() && (int32_t)(now_ms - slm.attempt.next) >= 0)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |

## Modules/comm_channel.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `Modules/comm_channel.c:26` / `channel_emit` | `if (ch.event)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:33` / `channel_ip` | `if (!comm_at_parse(p,n,quoted,values) && !comm_at_parse(p,n,plain,values))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:38` / `channel_read_done` | `if (ch.flags.read_active)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:40` / `channel_read_done` | `if (ch.read_done)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:45` / `channel_binary` | `if (!ch.flags.read_active \|\| !ch.binary)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:47` / `channel_binary` | `if (used <= n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:52` / `channel_at_event` | `if (kind == COMM_AT_NO_ANSWER \|\| kind == COMM_AT_STREAM_ERROR)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:53` / `channel_at_event` | `uint8_t reason = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:55` / `channel_at_event` | `if (comm_modem_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:58` / `channel_at_event` | `if (kind == COMM_AT_DONE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:59` / `channel_at_event` | `if (!p)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:61` / `channel_at_event` | `if (comm_at_parse(p,n,comm_modem_profile()->recv,v) && v[0] == 0)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:62` / `channel_at_event` | `if (comm_at_parse(p,n,comm_modem_profile()->closed,v) && v[0] == 0)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:64` / `channel_at_event` | `if (comm_modem_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:67` / `channel_at_event` | `if (comm_at_parse(p,n,comm_modem_profile()->pdp,v) && v[0] == 1)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:71` / `channel_at_event` | `if (ch.operation == OP_QUERY && kind == COMM_AT_LINE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:71` / `channel_at_event` | `if (query.line)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:72` / `channel_at_event` | `if (ch.operation == OP_VERIFY && comm_at_prefix(p,n,"+QISTATE:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:73` / `channel_at_event` | `if (ch.operation == OP_OPEN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:74` / `channel_at_event` | `if (comm_at_parse(p,n,comm_modem_profile()->open,v) && v[0] == 0)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:77` / `channel_at_event` | `if (ch.operation == OP_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:78` / `channel_at_event` | `if (comm_at_parse(p,n,"+QISEND:\001,\001,\001",v) && v[1] <= v[0] && v[2] == v[0] - v[1])` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:79` / `channel_at_event` | `ch.answer = v[2] == 0 ? 1 : 2;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:83` / `channel_at_event` | `if (ch.probe_active == COMM_CH_PING)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:84` / `channel_at_event` | `if (!comm_at_prefix(p,n,"+QPING:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:88` / `channel_at_event` | `if (!comm_at_scan(p,n,&pos,UINT32_MAX,&code) \|\| code \|\| pos >= n \|\| p[pos++] != ',')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:90` / `channel_at_event` | `if (channel_ip(p,n,"\"\006.\006.\006.\006\",\001,\001,\001", "\006.\006.\006.\006,\001,\001,\001",v))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:92` / `channel_at_event` | `if (comm_at_parse(p,n,"\001,\001,\001,\001,\001,\001",v) && v[1] > 0 && v[1] <= v[0])` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:93` / `channel_at_event` | `if (ch.probe_active == COMM_CH_DNS && comm_at_prefix(p,n,"+QIURC: \"dnsgip\","))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:96` / `channel_at_event` | `if (channel_ip(p,n,"\"\006.\006.\006.\006\"","\006.\006.\006.\006",v) && ch.probe_answer == 2)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:98` / `channel_at_event` | `else if (comm_at_parse(p,n,"\001,\001,\001",v)) ch.probe_answer = v[0] == 0 && v[1] > 0 ? 2 : 3;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:98` / `channel_at_event` | `if (comm_at_parse(p,n,"\001,\001,\001",v))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:112` / `comm_channel_rx_capacity` | `{ ch.capacity = count > 256u ? 256u : count;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:113` / `comm_channel_ready` | `return ch.state == CH_OPEN;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:114` / `comm_channel_fault` | `return ch.state == CH_FAULT;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:118` / `comm_channel_diag` | `return ch.state \| (uint32_t)ch.tx_state << 8 \| (uint32_t)ch.operation << 16 \| (uint32_t)ch.connections << 24;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:120` / `comm_tx_busy` | `return ch.tx_state != TX_FREE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:124` / `comm_tx_acquire` | `if (capacity)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:125` / `comm_tx_acquire` | `if (!comm_channel_ready() \|\| ch.tx_state != TX_FREE \|\| ch.flags.urgent)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:130` / `comm_tx_commit` | `if (ch.tx_state != TX_BUILD \|\| !count \|\| count > sizeof(frame))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:133` / `comm_tx_release` | `if (ch.tx_state == TX_BUILD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:138` / `comm_channel_reopen` | `if (ch.state == CH_CLOSE \|\| ch.state == CH_CLOSING \|\| ch.state == CH_VERIFY \|\| ch.state == CH_PAUSE \|\| ch.state == CH_FAULT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:139` / `comm_channel_reopen` | `if (ch.state == CH_INITIAL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:142` / `comm_channel_reopen` | `if (!ch.flags.registered_session && ++ch.address_failures >= 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:144` / `comm_channel_reopen` | `if (modem_slm336_reserve())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:147` / `comm_channel_reopen` | `if (ch.connections >= 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:148` / `comm_channel_reopen` | `uint8_t t = ch.retry_step < 3 ? (uint8_t)(COMM_T_SERVER_RETRY_1 + ch.retry_step) : COMM_T_SERVER_RETRY_MAX;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:149` / `comm_channel_reopen` | `if (ch.reopen_pause < comm_timeouts[t])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:150` / `comm_channel_reopen` | `if (ch.retry_step < 3)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:153` / `comm_channel_reopen` | `if (had_socket)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:158` / `comm_channel_query` | `if (!comm_channel_ready() \|\| query.cmd \|\| !cmd)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:163` / `comm_channel_probe` | `if (!comm_channel_ready() \|\| ch.probe \|\| ch.probe_active \|\| (kind != COMM_CH_PING && kind != COMM_CH_DNS))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:169` / `channel_command` | `if (op == OP_OPEN \|\| op == OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:171` / `channel_command` | `const char *p = profile->commands + (op == OP_OPEN ? profile->tcp : profile->read);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:174` / `channel_command` | `if (op == OP_OPEN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:177` / `channel_command` | `host = settings->text(ch.address ? SLM_CFG_HOST2 : SLM_CFG_HOST, &capacity);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:178` / `channel_command` | `value = settings->number(ch.address ? SLM_CFG_PORT2 : SLM_CFG_PORT);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:179` / `channel_command` | `if (!host \|\| !capacity \|\| !value \|\| !comm_at_field(host, capacity, true))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:181` / `channel_command` | `if (comm_modem_build(cmd, sizeof(cmd), p, host, value) >= sizeof(cmd))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:184` / `channel_command` | `comm_at_request_t req = {(const uint8_t *)s, s ? (uint16_t)strlen(s) : 0, mode, comm_timeouts[timeout_id]};` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:185` / `channel_command` | `uint8_t r = op == OP_SEND ? comm_modem_payload(frame, ch.frame_count, ch.now) : comm_modem_request(&req, ch.now);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:187` / `channel_command` | `if (r != COMM_AT_ACCEPTED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:189` / `channel_command` | `if (op == OP_OPEN \|\| op == OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:190` / `channel_command` | `comm_modem_record(op == OP_OPEN ? SLM_TCP : SLM_READ, 0, 0);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:191` / `channel_command` | `if (op == OP_OPEN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:199` / `channel_completed` | `if (op == OP_OPEN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:200` / `channel_completed` | `if (result == COMM_AT_OK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:201` / `channel_completed` | `if (result == UINT8_MAX \|\| result == COMM_AT_OK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:202` / `channel_completed` | `if (!ch.flags.open_ack)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:203` / `channel_completed` | `if (ch.now - ch.operation_since >= comm_timeouts[COMM_T_QIOPEN])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:204` / `channel_completed` | `if (!ch.flags.open_seen)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:205` / `channel_completed` | `else result = ch.open_code ? COMM_AT_ERROR : COMM_AT_OK;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:208` / `channel_completed` | `uint8_t reason = result == COMM_AT_OK ? SLM_ERR_NONE : result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_TCP;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:209` / `channel_completed` | `: result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_TCP;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:210` / `channel_completed` | `comm_modem_record(SLM_STEP_COUNT, reason, ch.flags.open_seen ? ch.open_code : comm_modem_status()->modem_code);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:211` / `channel_completed` | `if (ch.state == CH_OPENING)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:212` / `channel_completed` | `if (reason)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:217` / `channel_completed` | `if (result == UINT8_MAX)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:219` / `channel_completed` | `if (op == OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:220` / `channel_completed` | `uint8_t reason = result == COMM_AT_OK ? SLM_ERR_NONE : result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:220` / `channel_completed` | `uint8_t reason = result == COMM_AT_OK ? SLM_ERR_NONE : result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:221` / `channel_completed` | `: result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:223` / `channel_completed` | `if (ch.flags.read_active)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:227` / `channel_completed` | `if (reason)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:230` / `channel_completed` | `if (op == OP_QUERY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:231` / `channel_completed` | `if (op == OP_PING \|\| op == OP_DNS)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:232` / `channel_completed` | `if (result != COMM_AT_OK && ch.probe_active == (op == OP_PING ? COMM_CH_PING : COMM_CH_DNS)) { ch.probe = ch.probe_active = 0;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:232` / `channel_completed` | `if (result != COMM_AT_OK && ch.probe_active == (op == OP_PING ? COMM_CH_PING : COMM_CH_DNS))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:237` / `channel_completed` | `if (ch.state == CH_CLOSE && op != OP_CLOSE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:238` / `channel_completed` | `if (op == OP_CLOSE \|\| op == OP_VERIFY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:239` / `channel_completed` | `if (op == OP_CLOSE && result == COMM_AT_ERROR && !ch.flags.peer_closed)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:243` / `channel_completed` | `if ((result != COMM_AT_OK && !(result == COMM_AT_ERROR && ch.flags.peer_closed)) \|\| (op == OP_VERIFY && ch.answer))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:251` / `channel_completed` | `if (result != (op == OP_PROMPT ? COMM_AT_GOT_PROMPT : COMM_AT_OK)) { comm_modem_record(SLM_STEP_COUNT, result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_AT, comm_modem_status()->modem_code);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:251` / `channel_completed` | `if (result != (op == OP_PROMPT ? COMM_AT_GOT_PROMPT : COMM_AT_OK))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:252` / `channel_completed` | `comm_modem_record(SLM_STEP_COUNT, result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_AT, comm_modem_status()->modem_code);` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:256` / `channel_completed` | `if (op == OP_PROMPT)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:257` / `channel_completed` | `if (op == OP_SEND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:258` / `channel_completed` | `if (op == OP_ACK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:259` / `channel_completed` | `if (!ch.answer)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:260` / `channel_completed` | `if (ch.answer == 1)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:268` / `comm_channel_poll` | `if (ch.state == CH_FAULT)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:269` / `comm_channel_poll` | `if (ch.operation == OP_OPEN \|\| ch.operation == OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:271` / `comm_channel_poll` | `if (ch.operation)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:272` / `comm_channel_poll` | `if (ch.state == CH_INITIAL)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:273` / `comm_channel_poll` | `if (!comm_modem_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:276` / `comm_channel_poll` | `if (ch.state == CH_OPEN && !comm_modem_ready())` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:277` / `comm_channel_poll` | `if (ch.state == CH_OPEN && ch.tx_state != TX_FREE && now_ms - ch.tx_since > comm_timeouts[COMM_T_UNACKED])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:279` / `comm_channel_poll` | `if (ch.probe_active)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:280` / `comm_channel_poll` | `uint8_t t = ch.probe_active == COMM_CH_PING ? COMM_T_PING : COMM_T_DNS;` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:281` / `comm_channel_poll` | `if (ch.probe_answer == 1 \|\| now_ms - ch.probe_since >= comm_timeouts[t] \|\| (ch.probe_active == COMM_CH_DNS && ch.probe_answer == 3))` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:285` / `comm_channel_poll` | `if (modem_slm336_busy())` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:286` / `comm_channel_poll` | `if (ch.state == CH_CLOSE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:287` / `comm_channel_poll` | `if (ch.operation == OP_OPEN)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:289` / `comm_channel_poll` | `if (channel_command("AT+QICLOSE=0", COMM_AT_TEXT, COMM_T_TCP_CLOSE, OP_CLOSE))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:292` / `comm_channel_poll` | `if (ch.state == CH_VERIFY)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:295` / `comm_channel_poll` | `if (!ch.operation)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:298` / `comm_channel_poll` | `if (ch.state == CH_PAUSE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:299` / `comm_channel_poll` | `if (now_ms - ch.operation_since < ch.reopen_pause)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:300` / `comm_channel_poll` | `if (!channel_command(NULL, COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_OPEN))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:304` / `comm_channel_poll` | `if (ch.state != CH_OPEN \|\| ch.operation)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:305` / `comm_channel_poll` | `if (ch.tx_state == TX_QUEUED)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:308` / `comm_channel_poll` | `if (n < sizeof(cmd) && channel_command(cmd, COMM_AT_PROMPT, COMM_T_SEND_PROMPT, OP_PROMPT))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:309` / `comm_channel_poll` | `if (ch.tx_state == TX_PAYLOAD)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:310` / `comm_channel_poll` | `if (channel_command(NULL, COMM_AT_SEND, COMM_T_SEND_RESULT, OP_SEND))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:311` / `comm_channel_poll` | `if (ch.tx_state == TX_ACK && (int32_t)(now_ms - ch.ack_next) >= 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:313` / `comm_channel_poll` | `if (ch.tx_state == TX_FREE && ch.probe && !ch.probe_active)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:316` / `comm_channel_poll` | `if (dns && ch.flags.dns_fence && now_ms - ch.dns_since < comm_timeouts[COMM_T_DNS_FENCE])` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_channel.c:318` / `comm_channel_poll` | `} else if (channel_command(dns ? "AT+QIDNSGIP=1,\"ya.ru\"" : "AT+QPING=1,\"ya.ru\",3,2", COMM_AT_TEXT, COMM_T_AT_SIMPLE, dns ? OP_DNS : OP_PING)) { if (dns) { ch.flags.dns_fence = 1;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:318` / `comm_channel_poll` | `if (channel_command(dns ? "AT+QIDNSGIP=1,\"ya.ru\"" : "AT+QPING=1,\"ya.ru\",3,2", COMM_AT_TEXT, COMM_T_AT_SIMPLE, dns ? OP_DNS : OP_PING))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:319` / `comm_channel_poll` | `COMM_AT_TEXT, COMM_T_AT_SIMPLE, dns ? OP_DNS : OP_PING)) { if (dns) { ch.flags.dns_fence = 1;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:320` / `comm_channel_poll` | `if (dns)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_channel.c:325` / `comm_channel_poll` | `if (ch.tx_state == TX_FREE && !ch.flags.urgent && ch.flags.recv && ch.capacity && ch.binary)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_channel.c:327` / `comm_channel_poll` | `if (ch.tx_state == TX_FREE && !ch.flags.urgent && query.cmd)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |

## Modules/comm_sms.c

| Место / функция | Что проверяет | Метка | Основание |
|---|---|---|---|
| `Modules/comm_sms.c:29` / `sms_char` | `if (msg.alpha)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:29` / `sms_char` | `if (msg.nlen + 1u < sizeof(msg.number))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:30` / `sms_char` | `if (msg.tlen < SMS_TEXT)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:35` / `sms_septet` | `if (msg.skip)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:36` / `sms_septet` | `if (s == 0x1B)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:37` / `sms_septet` | `sms_char(s == 0 ? '@' : s == 2 ? '$' : s == 0x11 ? '_' : s == '\r' \|\| s == '\n' ? (char)s : s < 0x20 \|\| s == 0x24 \|\| s == 0x40 \|\| (s >= 0x5B && s <= 0x60) \|\| s >= 0x7B ? '?' : (char)s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:37` / `sms_septet` | `sms_char(s == 0 ? '@' : s == 2 ? '$' : s == 0x11 ? '_' : s == '\r' \|\| s == '\n' ? (char)s : s < 0x20 \|\| s == 0x24 \|\| s == 0x40 \|\| (s >= 0x5B && s <= 0x60) \|\| s >= 0x7B ? '?' : (char)s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:37` / `sms_septet` | `sms_char(s == 0 ? '@' : s == 2 ? '$' : s == 0x11 ? '_' : s == '\r' \|\| s == '\n' ? (char)s : s < 0x20 \|\| s == 0x24 \|\| s == 0x40 \|\| (s >= 0x5B && s <= 0x60) \|\| s >= 0x7B ? '?' : (char)s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:37` / `sms_septet` | `sms_char(s == 0 ? '@' : s == 2 ? '$' : s == 0x11 ? '_' : s == '\r' \|\| s == '\n' ? (char)s : s < 0x20 \|\| s == 0x24 \|\| s == 0x40 \|\| (s >= 0x5B && s <= 0x60) \|\| s >= 0x7B ? '?' : (char)s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:38` / `sms_septet` | `: s < 0x20 \|\| s == 0x24 \|\| s == 0x40 \|\| (s >= 0x5B && s <= 0x60) \|\| s >= 0x7B ? '?' : (char)s);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:43` / `sms_cp1251` | `return u < 0x80 ? (char)u : u >= 0x410 && u <= 0x44F ? (char)(u - 0x410 + 0xC0) : u == 0x401 ? (char)0xA8 : u == 0x451 ? (char)0xB8 : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:43` / `sms_cp1251` | `return u < 0x80 ? (char)u : u >= 0x410 && u <= 0x44F ? (char)(u - 0x410 + 0xC0) : u == 0x401 ? (char)0xA8 : u == 0x451 ? (char)0xB8 : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:43` / `sms_cp1251` | `return u < 0x80 ? (char)u : u >= 0x410 && u <= 0x44F ? (char)(u - 0x410 + 0xC0) : u == 0x401 ? (char)0xA8 : u == 0x451 ? (char)0xB8 : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:44` / `sms_cp1251` | `: u == 0x401 ? (char)0xA8 : u == 0x451 ? (char)0xB8 : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:44` / `sms_cp1251` | `: u == 0x401 ? (char)0xA8 : u == 0x451 ? (char)0xB8 : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:49` / `sms_unpack` | `while (msg.bits >= 7u && msg.septets)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:55` / `sms_octet` | `switch (msg.field)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:56` / `sms_octet` | `msg.field = o ? P_SMSC : P_FO;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:57` / `sms_octet` | `if (!--msg.left)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:58` / `sms_octet` | `msg.field = (o & 3u) ? P_BAD : P_OA_LEN;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:59` / `sms_octet` | `msg.field = o > 20u ? P_BAD : P_OA_TYPE;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:62` / `sms_octet` | `if (o == 0x91u && !msg.alpha)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:63` / `sms_octet` | `if (msg.alpha)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:65` / `sms_octet` | `msg.field = msg.left ? P_OA : P_PID;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:67` / `sms_octet` | `if (msg.alpha)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:68` / `sms_octet` | `for (uint8_t i = 0; i < 2u; ++i, o >>= 4)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:70` / `sms_octet` | `if (d < 10u && msg.nlen + 1u < sizeof(msg.number))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:72` / `sms_octet` | `if (!--msg.left)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:75` / `sms_octet` | `uint8_t a = (o & 0x80u) == 0 ? (o >> 2) & 3u : (o & 0xF0u) == 0xE0u ? 2u : (o & 0xF0u) == 0xF0u ? (o >> 2) & 1u : 0u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:75` / `sms_octet` | `uint8_t a = (o & 0x80u) == 0 ? (o >> 2) & 3u : (o & 0xF0u) == 0xE0u ? 2u : (o & 0xF0u) == 0xF0u ? (o >> 2) & 1u : 0u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:76` / `sms_octet` | `: (o & 0xF0u) == 0xF0u ? (o >> 2) & 1u : 0u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:77` / `sms_octet` | `msg.coding = a == 2u ? CODE_UCS2 : a == 1u ? CODE_8BIT : CODE_GSM;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:77` / `sms_octet` | `msg.coding = a == 2u ? CODE_UCS2 : a == 1u ? CODE_8BIT : CODE_GSM;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:80` / `sms_octet` | `if (!--msg.left)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:83` / `sms_octet` | `if (msg.coding == CODE_GSM)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:84` / `sms_octet` | `msg.field = !msg.left ? P_END : msg.udh ? P_UDHL : P_UD;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:84` / `sms_octet` | `msg.field = !msg.left ? P_END : msg.udh ? P_UDHL : P_UD;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:86` / `sms_octet` | `if (msg.coding == CODE_GSM)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:88` / `sms_octet` | `msg.field = --msg.left ? P_UD : P_END;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:90` / `sms_octet` | `if (msg.coding == CODE_GSM)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:91` / `sms_octet` | `if (msg.coding == CODE_8BIT && msg.skip)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:92` / `sms_octet` | `if (msg.coding == CODE_8BIT)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:93` / `sms_octet` | `if (msg.skip)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:94` / `sms_octet` | `if (!msg.half)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:96` / `sms_octet` | `if (!--msg.left)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:97` / `sms_octet` | `default:` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:101` / `sms_hex` | `{ return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10u : c >= 'a' && c <= 'f' ? c - 'a' + 10u : 16u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:101` / `sms_hex` | `{ return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10u : c >= 'a' && c <= 'f' ? c - 'a' + 10u : 16u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:101` / `sms_hex` | `{ return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10u : c >= 'a' && c <= 'f' ? c - 'a' + 10u : 16u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:101` / `sms_hex` | `return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10u : c >= 'a' && c <= 'f' ? c - 'a' + 10u : 16u;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:105` / `sms_index` | `return comm_at_scan(p, n, &pos, 255u, &v) && (pos == n \|\| p[pos] == ',') ? (uint8_t)v : 0u;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:105` / `sms_index` | `return comm_at_scan(p, n, &pos, 255u, &v) && (pos == n \|\| p[pos] == ',') ? (uint8_t)v : 0u;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:109` / `sms_queue` | `for (uint8_t i = 0; i < msg.count; ++i)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:109` / `sms_queue` | `if (msg.queue[i] == index)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:110` / `sms_queue` | `if (msg.count < sizeof(msg.queue))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:116` / `sms_line` | `if (msg.op == SMS_OP_LIST)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:118` / `sms_line` | `if (comm_at_prefix(p, n, "+CMGL:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:118` / `sms_line` | `if (i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:121` / `sms_line` | `if (msg.op != SMS_OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:122` / `sms_line` | `if (comm_at_prefix(p, n, "+CMGR:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:125` / `sms_line` | `for (uint16_t i = 0; i < n && msg.field < P_END; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:127` / `sms_line` | `if (v > 15u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:128` / `sms_line` | `if (!msg.nib_half)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:134` / `sms_urc` | `if (!comm_at_prefix(p, n, "+CMTI:"))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:136` / `sms_urc` | `while (pos && p[pos - 1u] != ',')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:137` / `sms_urc` | `uint8_t i = pos ? sms_index(p, n, pos) : 0;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:138` / `sms_urc` | `if (i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:149` / `sms_issue` | `msg.req.payload = op == SMS_OP_SEND ? msg.out : NULL;` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:150` / `sms_issue` | `if (!comm_modem_side(&msg.req))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:156` / `sms_command` | `if (index)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:163` / `sms_done` | `if (op == SMS_OP_SETUP)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:164` / `sms_done` | `if (r != COMM_AT_OK)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:165` / `sms_done` | `if (++msg.step == sizeof(sms_setup) / sizeof(sms_setup[0]))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:166` / `sms_done` | `if (op == SMS_OP_READ)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:170` / `sms_done` | `if (r == COMM_AT_OK && msg.field == P_END && msg.nlen && msg.received)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:173` / `sms_done` | `if (op == SMS_OP_DELETE)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:174` / `sms_done` | `if (op == SMS_OP_SEND)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:180` / `comm_sms_poll` | `if (session != msg.session)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:183` / `comm_sms_poll` | `if (msg.op)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:185` / `comm_sms_poll` | `if (r == UINT8_MAX)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:188` / `comm_sms_poll` | `if (msg.deliver)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:189` / `comm_sms_poll` | `if (!msg.received(msg.number, msg.text))` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:193` / `comm_sms_poll` | `if (st->state != SLM_READY && !(st->state == SLM_STARTING && st->step > SLM_SIM))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:194` / `comm_sms_poll` | `if (!msg.ready)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:195` / `comm_sms_poll` | `if ((int32_t)(now_ms - msg.retry) >= 0)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:196` / `comm_sms_poll` | `if (msg.erase)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:197` / `comm_sms_poll` | `if (msg.tx)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:198` / `comm_sms_poll` | `if (msg.count)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:199` / `comm_sms_poll` | `if (msg.scan)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:209` / `sms_gsm` | `return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' \|\| c == '\n' ? c : c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:209` / `sms_gsm` | `return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' \|\| c == '\n' ? c : c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:209` / `sms_gsm` | `return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' \|\| c == '\n' ? c : c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:209` / `sms_gsm` | `return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' \|\| c == '\n' ? c : c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:209` / `sms_gsm` | `return c == '@' ? 0 : c == '$' ? 2 : c == '_' ? 0x11 : c == '\r' \|\| c == '\n' ? c : c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:210` / `sms_gsm` | `: c < 0x20 \|\| (c >= 0x5B && c <= 0x5E) \|\| c == 0x60 \|\| c >= 0x7B ? '?' : c;` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:214` / `comm_sms_send` | `if (msg.tx \|\| !number \|\| !text)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:216` / `comm_sms_send` | `if (*number == '+')` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:217` / `comm_sms_send` | `if (number[n] < '0' \|\| number[n] > '9' \|\| ++n > 20u)` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:217` / `comm_sms_send` | `while (number[n])` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:218` / `comm_sms_send` | `if (!n)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:219` / `comm_sms_send` | `if (n >= 11u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:221` / `comm_sms_send` | `if ((uint8_t)text[len] >= 0x80u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:221` / `comm_sms_send` | `while (text[len] && len < 255u)` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:222` / `comm_sms_send` | `uint8_t chars = gsm ? (len > SMS_UD * 8u / 7u ? (uint8_t)(SMS_UD * 8u / 7u) : len) : (len > SMS_UD / 2u ? (uint8_t)(SMS_UD / 2u) : len);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:222` / `comm_sms_send` | `uint8_t chars = gsm ? (len > SMS_UD * 8u / 7u ? (uint8_t)(SMS_UD * 8u / 7u) : len) : (len > SMS_UD / 2u ? (uint8_t)(SMS_UD / 2u) : len);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:223` / `comm_sms_send` | `: (len > SMS_UD / 2u ? (uint8_t)(SMS_UD / 2u) : len);` | **защита** | граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ |
| `Modules/comm_sms.c:227` / `comm_sms_send` | `for (uint8_t i = 0; i < n; i = (uint8_t)(i + 2u))` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:228` / `comm_sms_send` | `sms_put((uint8_t)((i + 1u < n ? (number[i + 1u] - '0') << 4 : 0xF0) \| (number[i] - '0')));` | **защита** | границы входа модема/SMS, целостность потока или владение транспортом |
| `Modules/comm_sms.c:229` / `comm_sms_send` | `sms_put(gsm ? 0x00 : 0x08);` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:230` / `comm_sms_send` | `sms_put(gsm ? chars : (uint8_t)(chars * 2u));` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:232` / `comm_sms_send` | `for (uint8_t i = 0; i < chars; ++i)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:234` / `comm_sms_send` | `if (!gsm)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:235` / `comm_sms_send` | `uint16_t u = c < 0x80u ? c : c >= 0xC0u ? (uint16_t)(c - 0xC0u + 0x410u) : c == 0xA8u ? 0x401u : c == 0xB8u ? 0x451u : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:235` / `comm_sms_send` | `uint16_t u = c < 0x80u ? c : c >= 0xC0u ? (uint16_t)(c - 0xC0u + 0x410u) : c == 0xA8u ? 0x401u : c == 0xB8u ? 0x451u : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:235` / `comm_sms_send` | `uint16_t u = c < 0x80u ? c : c >= 0xC0u ? (uint16_t)(c - 0xC0u + 0x410u) : c == 0xA8u ? 0x401u : c == 0xB8u ? 0x451u : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:235` / `comm_sms_send` | `uint16_t u = c < 0x80u ? c : c >= 0xC0u ? (uint16_t)(c - 0xC0u + 0x410u) : c == 0xA8u ? 0x401u : c == 0xB8u ? 0x451u : '?';` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:239` / `comm_sms_send` | `if (bits >= 8u)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
| `Modules/comm_sms.c:241` / `comm_sms_send` | `if (bits)` | **нужна** | выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния |
