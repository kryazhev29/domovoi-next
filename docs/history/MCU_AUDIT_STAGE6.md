# Исправления аудита ДОМОВОЙ NEXT — этап 6

01.10.2026. Ветка codex/mcu-audit; исходная прошивка 29e8edd, итог 2c49bd3. Владелец поручил «добивай все оставшиеся», поэтому после этого отчёта разрешён этап 7 без повторного запроса. AGENTS.md и PROJECT_STATUS.md прочитаны. Агенты не запускались; платы, сервер, COM, master и исходный worktree не трогались.

| Ошибка | Коммит | Тест: падал → проходит | Поведение |
|---|---|---|---|
| B22 | `24489ff` | test_iar_uart_clock.py: GPIO clock absent → PASS | USART2/3 включают GPIO через APB2, USART через APB1. |
| B29 | `286a4a3` | test_iar_elapsed_time.py: потеря секунд/счётчик вызовов trial → PASS | Накопленные секунды обрабатываются полностью, остаток сохраняется атомарно. Проба использует ранее свободный timer[6], срок 1800 с прежний. Начало срока FW/Q4 не менялось. |
| B30 | `4798a39` | idexpiry: FAIL 64-bit reference → PASS | Срок ID не складывается с переполнением; вне диапазона Unix uint32 нет ложного раннего окончания. |
| B31 | `98aa861` | test_iar_pvd.py: EXTI16 disabled → PASS | По явному согласованию владельца включён сброс ниже 2,5 В. ISR подтверждает pending/PVDO и очищает EXTI16; восстановление питания/ложный IRQ не сбрасывают плату. |
| B32 | `dacc99f` | usbsetupbounds: MMIO до проверки endpoint → PASS | Все 65536 wIndex при количестве endpoint 1..8; GET_STATUS/CLEAR_FEATURE/SET_FEATURE отвергают неверный номер до MMIO. |
| B33 | `3439dd2` | test_iar_pma_bounds.py: unmapped read/write на нечётном хвосте → PASS | PMA TX/RX читают/пишут ровно n байт, 0..127, включая нечётные и невыровненные буферы. |
| B34 | `2c49bd3` | test_iar_peripheral_waits.py: пять FAIL stuck/CE_OK → PASS | SPI/ADC/UART имеют бюджет ожидания; SPI/W25 и модем передают отказ. АЦП сохраняет входы; HSE/PLL/SYSCLK при отказе запрашивает reset до приложения. SPI/ADC отключаются до повторной инициализации; UART ошибка снимается новой передачей. |
| B36 | `fca1fd2` | boot USB_Istr: бюджет инструкций исчерпан; Suspend на 286a4a3: polls>50000 → PASS | RESET ожидается ограниченно; восстановление USB в main, detach 20 мс без ожидания в IRQ. Условный Suspend проверен host, в текущем IAR образе он удалён компоновщиком. |
| B37 | `3e83dc9` | test_iar_boot_tx.py: stalled loop/IRQ получил 1 байт → PASS | Весь кадр публикуется атомарно, переполнение/отказ не оставляют префикс. TX очередь имеет volatile и снимки под PRIMASK; 450 IRQ границ, stalled active/idle, reset. Внутренний send возвращает uint8 результат, формат ответа прежний. |
| B39 | `edda8ca` | tempidmeta: неверные metadata → PASS | Новый tempID обнуляется целиком: ровно один нужный флаг, резерв/default нулевые, повтор не добавляет запись. |
| B40 | `866513b` | test_iar_callback_init.py: Error=0xA5A5A5A5 → PASS | ConnectParam_t/GsmParam_t полностью инициализированы; необязательные callbacks NULL. |
| B41 | `0fe2b27` | test_iar_send_bounds.py: allocation 65536 → PASS | Проверяется вся 16-битная длина кадра до malloc; NULL с ненулевым payload отвергается. Обычный формат прежний, OOM без отправки. |
| B42 | `46c31be` | utcday: holiday false на границе INT32_MAX → PASS | Праздники, день недели/час/минута используют unsigned UTC; границы 2038 и UINT32_MAX проверены. |

Каждая ошибка получила исполняемый падающий тест до исправления и отдельный коммит. UART/PVD/таймеры/PMA/boot/peripheral проверки исполняют настоящий IAR код в Unicorn; периферия и прерывания моделируются. Это не испытания платы. Инициализация PVD дополнительно согласована владельцем: «Да, включить сброс ниже 2,5 В».

## Обязательные проверки

После каждого исправления выполнены все тогда существовавшие host-группы, tx idle/reset/esof/busy/large, Bootloader/tests/run_tests.py и четыре сборки IAR. На окончательном коде: 44 группы PASS; загрузчик PASS; DomovoiNext Release/Release_IPv3 и DomovoiBoot Release/Modem_Release — 0 ошибок. Полные журналы: Release/phase6-<Bxx>-host.log, -boot.log, -iar.log; окончательные B32/B34/B37 — с суффиксом -final. Промежуточные несовпадения тестовой сигнатуры B37 устранены, полный прогон повторён.

Все новые IAR тесты PASS. Новых предупреждений нет. B34 убрала Pe550 о неиспользуемой dt в SPI: у приложения осталось 2 предупреждения (Ls016 USB_Init, Ls017 gsm_dataread), у загрузчика 2 прежних (Pa082 boot_main volatile read order, Ls016 USB_Init). Адреса 0x08008000/0x08008200, boot 32 КБ/footer 0x32, кадры/ответы и ICF сохранены. Изменённая прошивка проверена как cp1251/CRLF; git diff --check PASS.

## Изменения загрузчика 0x32 отдельно

- B36: общий USB_WaitReset, ограниченный ESOF/Suspend; boot_platform содержит request/poll recovery, main вызывает poll; USB переподключается из main через 20 мс. Watchdog/main больше не заперты в USB IRQ.
- B37: собственные TX indices/state volatile; producer публикует целую посылку под PRIMASK, consumer/EP1 используют снимки; timeout не уходит в UINT32_MAX. USB reset очищает очередь. Обновлены внутренняя сигнатура send и stub загрузочного теста; 18-байтный ответ не изменён.
- B29 app_update.c относится к приложению: timer[6] для trial. Состояния boot journal, установка/откат и команда 110 не изменены.
- B34 ограничивает ожидания clock в CMSIS; boot_core и замена загрузчика по 110 не изменялись. Оба boot BIN 32768 байт/footer 0x32. Реальная установка не выполнялась.

## ОЗУ из карты Release

| Величина | До | После |
|---|---:|---:|
| Readwrite data | 19771 Б | 19769 Б |
| Readwrite code | 636 Б | 636 Б |
| CSTACK | 4096 Б | 4096 Б |
| HEAP | 5120 Б | 5120 Б |
| Щель до fault_record | **56 Б** | **56 Б** |

Адреса/SHA256/буферы: [MCU_AUDIT_STAGE6_MEMORY.json](MCU_AUDIT_STAGE6_MEMORY.json). Удалён trial_seconds; txfailed размещён в прежнем padding uart_t, uart в карте остаётся 48 байт. SPI/ADC fault используют SPE/ADON периферии без нового статического буфера. ICF не уменьшались.

## Что осталось

B01–B42 закрыты в коде с указанными ограничениями аппаратной проверки. Этап 7 ещё не выполнен: USB const, mem[], log_pendingAt, копии UART/stash и большие секторные heap buffers. Q1–Q4/Q6 остаются вопросами протокола; их не меняли по догадке, Q5 уже согласован. Изменения происхождения срока FW не входили в B29.

Без платы не проверены порог/гистерезис PVD и циклы reset при плохом питании, реальные времена ожиданий/USB re-enumeration/ADC/SPI/PLL/UART, watchdog, вложенные IRQ/стек и power-cut гарантии NOR. При физическом отказе HSE/PLL reset будет повторяться до устранения причины. Для старого приложения при откате по-прежнему нужен обновлённый загрузчик с recovery этапа 5; ничего на платы не писалось.
