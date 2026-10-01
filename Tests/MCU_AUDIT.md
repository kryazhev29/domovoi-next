# Аудит ДОМОВОЙ NEXT, STM32F103

Дата: 01.10.2026. Основа: `fix/ram-modem-rx`, `c7046d1`. Ветка: `codex/mcu-audit`. Worktree: `C:\Users\kryaz\Desktop\мк\_worktrees\domovoi-mcu-audit`.

Проверены прикладные исходники, драйверы, загрузчик, USB-библиотека, активные startup/обработчики/CMSIS-интерфейсы и все ICF. Неиспользуемые варианты CMSIS перечислены с ограничением проверки; DSP и код для других ядер не объявлены проверенной прошивкой F103. StdPeriph проверена по вызовам нашего кода, как требовалось. `AGENTS.md` и `PROJECT_STATUS.md` прочитаны до изменений. Дополнительные агенты не запускались.

На момент исходного аудита семь ошибок исправлены отдельными коммитами с тестами, падавшими до правки. Отдельная оптимизация USB-таблиц освободила **56 байт статической ОЗУ**, подтверждено картами IAR до/после. Остальные находки оставлены явно открытыми. Это аудит с локальными исправлениями, а не утверждение, что вся прошивка стала безошибочной. Master, исходный worktree, платы, сервер и COM-порты не изменялись. Адреса приложения `0x08008000/0x08008200`, загрузчик 32 КБ и формат обмена сохранены. Изменённые исходники прошивки сохранены в cp1251/CRLF.

Результат этапа 1: **B09, B18, B08 исправлены** отдельными коммитами; все обязательные проверки пройдены, щель перед fault_record **80 → 80 байт**. Подробности: [MCU_AUDIT_STAGE1.md](MCU_AUDIT_STAGE1.md). Дальнейшие этапы ожидают «да» владельца.

Результат этапа 2: **B10–B14 исправлены**, щель **80 → 80 байт**, все обязательные проверки пройдены. [MCU_AUDIT_STAGE2.md](MCU_AUDIT_STAGE2.md). Всего закрыто 15 из 42 находок; 27 остаются открытыми. Этап 3 ожидает «да».

## Ведомость файлов

«Да» означает чтение прикладного файла/проверку его контрактов, а не испытание на плате. Для библиотек указана фактическая граница анализа. Номера B относятся к таблице ошибок, Q — к вопросам протокола ниже. Пустое поле находок не означает доказанное отсутствие ошибок при всех аппаратных состояниях.

Всего в ведомости **157 файлов**, включая vendor-файлы с явно ограниченной областью.

| Файл | Проверен | Результат / граница |
|---|---|---|
| `Bootloader/DomovoiBoot.ewp` | Да | Release/Modem_Release: startup MD, ICF, аппаратные define и outputs проверены, обе сборки PASS. |
| `Bootloader/DomovoiBoot.icf` | Да | ROM до 0x08007FFB, marker по 0x08007FFC, CSTACK 2048, RAM20К; map/образ 32768 подтверждены. |
| `Bootloader/app_update.c` | Да | Полностью: boot request/metadata/trial confirmation/rollback, __ramfunc replace; B29; stage/write результат ограничен void mem API (B23). |
| `Bootloader/app_update.h` | Да | Все application/boot-metadata/trial интерфейсы проверены. |
| `Bootloader/board_profile.h` | Да | Профили платы и pin/macros проверены по board.h. |
| `Bootloader/boot_core.c` | Да | Полностью: validate/header/CRC/HW, backup/install/journal/trial/rollback/recovery; host power-cut и IAR emulator PASS. |
| `Bootloader/boot_core.h` | Да | Все magic/адреса/размеры/header/trial-контракты; 0x8000/0x8200/32К сохранены. |
| `Bootloader/boot_main.c` | Да | Полностью: USB ISR ring/parser/CRC/dispatch/session, main/recovery; host/IAR PASS; Pa082 на двух volatile чтениях, функциональное нарушение не доказано. |
| `Bootloader/boot_manual.c` | Да | Полностью: cmd110 validate/stage/retry/apply/CRC/HW/USB-only; host manual и RAM writer PASS; отключение питания при self-replace не безопасно без SWD. |
| `Bootloader/boot_manual.h` | Да | 32К, явный apply, интерфейсы stage/program и запреты проверены. |
| `Bootloader/boot_platform.c` | Да | Полностью: SPI/Flash timeout/readback/WDT/USB/app jump/NVIC teardown; boot emulator PASS. |
| `Bootloader/boot_platform.h` | Да | Все platform-контракты проверены. |
| `Bootloader/hw_config.c` | Да | Полностью: собственная legacy TX очередь, USB clock/serial/low power; B37; без volatile/единого atomic enqueue. |
| `Bootloader/includes.h` | Да | Порядок USB/ST/profile включений проверен. |
| `Bootloader/startup_gcc.s` | Да | Просмотрен целиком: vectors/data/bss/entry/default loops; IAR использует другой startup; нужен только GCC-профилю. |
| `Bootloader/stm32_it.h` | Да | Объявления активных USB IRQ проверены. |
| `Bootloader/stm32f10x_conf.h` | Да | Модули/отключённый assert; наши ST-вызовы проверены. |
| `CMSIS/Include/arm_comm.h` | Да | Все типы/макросы/условные ARM critical helpers просмотрены; старые helpers не вызываются прошивкой F103. |
| `CMSIS/Include/arm_common_tables.h` | Применимость | Не используется Cortex-M3/F103 кодом и активными .ewp: DSP/другое ядро. Полный аудит vendor-реализации не выполнялся. |
| `CMSIS/Include/arm_math.h` | Применимость | Не используется Cortex-M3/F103 кодом и активными .ewp: DSP/другое ядро. Полный аудит vendor-реализации не выполнялся. |
| `CMSIS/Include/core_cm0.h` | Применимость | Не используется Cortex-M3/F103 кодом и активными .ewp: DSP/другое ядро. Полный аудит vendor-реализации не выполнялся. |
| `CMSIS/Include/core_cm3.h` | Да | Активные NVIC/SCB/SysTick/Reset контракты, register volatile/masks проверены; не аудит всех неиспользуемых vendor функций. |
| `CMSIS/Include/core_cm4.h` | Применимость | Не используется Cortex-M3/F103 кодом и активными .ewp: DSP/другое ядро. Полный аудит vendor-реализации не выполнялся. |
| `CMSIS/Include/core_cm4_simd.h` | Применимость | Не используется Cortex-M3/F103 кодом и активными .ewp: DSP/другое ядро. Полный аудит vendor-реализации не выполнялся. |
| `CMSIS/Include/core_cmFunc.h` | Да | Проверены активный __ICCARM__ путь cmsis_iar и используемые interrupt/barrier контракты; прочие компиляторы вне активной сборки. |
| `CMSIS/Include/core_cmInstr.h` | Да | Проверен активный IAR путь intrinsic/barriers; прочие compiler branches вне активной сборки. |
| `CMSIS/STM32F10x/Include/stm32f10x.h` | Да | Активные MD IRQ/types/registers/HSE12MHz и константы используемых блоков; периферия иных STM32 моделей вне активного аудита. |
| `CMSIS/STM32F10x/Include/system_stm32f10x.h` | Да | Контракты SystemInit/SystemCoreClockUpdate и частоты проверены. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_cl.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_hd.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_hd_vl.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_ld.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_ld_vl.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_md.s` | Да, startup | Активный IAR MD: векторы/TRIL/HW-marker/reset/SystemInit/default handlers; приложение/boot собираются с ним. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_md_vl.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/Source/startup_stm32f10x_xl.s` | Да, startup | Другой density: проверены vector/reset/default-handler структура и неприменимость к активным .ewp; не собирался. |
| `CMSIS/STM32F10x/system_stm32f10x.c` | Да | SystemInit/clock update/активные MD+72MHz, HSE timeout, PLL waits и VTOR; B06/B34. Другие clock/density branches не используются четырьмя сборками. |
| `DomovoiNext.ewp` | Да | Release/Release_IPv3, startup MD, активный ICF, определения профилей и outputs проверены; обе сборки PASS. |
| `DomovoiNext.icf` | Да | Просмотрен; не выбран .ewp. Старое размещение от 0x08000000 и CSTACK=0x800; не использовать для требуемой Release. |
| `Modules/adc_stm32f10x.c` | Да | Все calibrate/regular/injected/temperature/flags; B34; времена/флаги hardware проверить отдельно. |
| `Modules/adc_stm32f10x.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `Modules/gsm.c` | Да | Весь код и все модели модемов; B15–B17/B35, URC ownership/heap; реальный parser не покрыт старым gsmurc. |
| `Modules/gsm.h` | Да | Все enum/состояния/контракты размеров/команд проверены; комментарий размера GsmStr не используется вместо map. |
| `Modules/spi_stm32f10x.c` | Да | Все SPI init/byte/bulk/flags; B34. spi_read действительно делает *dst++=DR; ложная находка о потере данных исключена. |
| `Modules/spi_stm32f10x.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `Modules/timers.c` | Да | Все тики/set/get/delay/millis; B20/B29; unsigned subtraction корректна для wrap интервалов. |
| `Modules/timers.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `Modules/uart_stm32f10x.c` | Да | Все init/IRQ/RX/TX/wait/clear/buffer handoff; B18–B22/B34/B35. |
| `Modules/uart_stm32f10x.h` | Да | Все UART-контракты, recsize и таймауты проверены; отсутствие универсальных диапазонов caller. |
| `Modules/usb_stm32f10x.c` | Да | Полностью: RX framing/CRC/overflow/timeout/recovery, TX/debug/диагностика; rx/recovery/tx PASS; B12 относится к caller. |
| `Modules/usb_stm32f10x.h` | Да | Все API и volatile diagnostic counters проверены. |
| `Modules/wdt_stm32f10x.c` | Да | Настройка/перезаряд IWDG; код проверен, реальный LSI/timeout не измерен. |
| `Modules/wdt_stm32f10x.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `STM32F10x_StdPeriph_Driver/inc/misc.h` | Вызовы ST | NVIC priority/group/vector контракты caller проверены; VTOR/USB B06. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_adc.h` | Вызовы ST | Регулярные/injected/calibration/flags: бессрочные caller waits B34. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_bkp.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_can.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_cec.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_crc.h` | Вызовы ST | Используемый hardware CRC32 clock/reset/data проверен; software/host FW CRC модели PASS. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_dac.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_dbgmcu.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_dma.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_exti.h` | Вызовы ST | B31/B38: отсутствующий PVD setup и WG handoff относятся к caller. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_flash.h` | Вызовы ST | Использованные erase/program/status/latency; Bootloader также обращается к регистрам напрямую, emulator PASS. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_fsmc.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_gpio.h` | Вызовы ST | Пины/remap/clock по обоим профилям и Bootloader проверены. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_i2c.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_iwdg.h` | Вызовы ST | Параметры prescaler/reload/enable просмотрены, аппаратное время не доказано. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_pwr.h` | Вызовы ST | B31: PVD enable не настраивает EXTI16; low-power вызовы просмотрены. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_rcc.h` | Вызовы ST | B22: неверная шина GPIO; остальные найденные clock вызовы просмотрены. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_rtc.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_sdio.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_spi.h` | Вызовы ST | Init/send/receive/flags: бессрочные caller waits B34. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_tim.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_usart.h` | Вызовы ST | Init/TXE/RXNE/IT clearing и SR→DR: B18–B22/B34/B35 в caller. |
| `STM32F10x_StdPeriph_Driver/inc/stm32f10x_wwdg.h` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/misc.c` | Вызовы ST | NVIC priority/group/vector контракты caller проверены; VTOR/USB B06. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_adc.c` | Вызовы ST | Регулярные/injected/calibration/flags: бессрочные caller waits B34. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_bkp.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_can.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_cec.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_crc.c` | Вызовы ST | Используемый hardware CRC32 clock/reset/data проверен; software/host FW CRC модели PASS. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_dac.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_dbgmcu.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c` | Вызовы ST | B31/B38: отсутствующий PVD setup и WG handoff относятся к caller. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c` | Вызовы ST | Использованные erase/program/status/latency; Bootloader также обращается к регистрам напрямую, emulator PASS. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_fsmc.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c` | Вызовы ST | Пины/remap/clock по обоим профилям и Bootloader проверены. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_iwdg.c` | Вызовы ST | Параметры prescaler/reload/enable просмотрены, аппаратное время не доказано. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_pwr.c` | Вызовы ST | B31: PVD enable не настраивает EXTI16; low-power вызовы просмотрены. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c` | Вызовы ST | B22: неверная шина GPIO; остальные найденные clock вызовы просмотрены. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_sdio.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c` | Вызовы ST | Init/send/receive/flags: бессрочные caller waits B34. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c` | Вызовы ST | Init/TXE/RXNE/IT clearing и SR→DR: B18–B22/B34/B35 в caller. |
| `STM32F10x_StdPeriph_Driver/src/stm32f10x_wwdg.c` | Вне границы ST | Наших вызовов не найдено; библиотечная реализация целиком исключена по правилу задания. |
| `STM32_USB-FS-Device_Driver/inc/usb_core.h` | Да | Control state machine/SETUP/standard features/endpoints/data transfer; B32. |
| `STM32_USB-FS-Device_Driver/inc/usb_def.h` | Да | Все USB request/descriptor/recipient/feature enum проверены. |
| `STM32_USB-FS-Device_Driver/inc/usb_init.h` | Да | USB pointers/device setup/interrupt mask; indirect-call stack warning остаётся. |
| `STM32_USB-FS-Device_Driver/inc/usb_int.h` | Да | CTR LP/HP, callback tables/EP indexing, EP0 state transitions просмотрены; extern callback-таблиц согласованы как const. |
| `STM32_USB-FS-Device_Driver/inc/usb_lib.h` | Да | Все зависимости/порядок заголовков проверены. |
| `STM32_USB-FS-Device_Driver/inc/usb_mem.h` | Да | Все копии RAM↔PMA, чётность/указатели; B33. |
| `STM32_USB-FS-Device_Driver/inc/usb_regs.h` | Да | Все register wrappers/macros/count/double-buffer; caller обязан проверять endpoint до MMIO, B32. |
| `STM32_USB-FS-Device_Driver/inc/usb_sil.h` | Да | Read/write/PMA/endpoint count/init контракты; B33. |
| `STM32_USB-FS-Device_Driver/inc/usb_type.h` | Да | Типы/volatile/NULL/bool и inclusion проверены. |
| `STM32_USB-FS-Device_Driver/src/usb_core.c` | Да | Control state machine/SETUP/standard features/endpoints/data transfer; B32. |
| `STM32_USB-FS-Device_Driver/src/usb_init.c` | Да | USB pointers/device setup/interrupt mask; indirect-call stack warning остаётся. |
| `STM32_USB-FS-Device_Driver/src/usb_int.c` | Да | CTR LP/HP, callback tables/EP indexing, EP0 state transitions просмотрены; extern callback-таблиц согласованы как const. |
| `STM32_USB-FS-Device_Driver/src/usb_mem.c` | Да | Все копии RAM↔PMA, чётность/указатели; B33. |
| `STM32_USB-FS-Device_Driver/src/usb_regs.c` | Да | Все register wrappers/macros/count/double-buffer; caller обязан проверять endpoint до MMIO, B32. |
| `STM32_USB-FS-Device_Driver/src/usb_sil.c` | Да | Read/write/PMA/endpoint count/init контракты; B33. |
| `USB-FS-CDC_User/Inc/hw_config.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Inc/usb_conf.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Inc/usb_desc.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Inc/usb_istr.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Inc/usb_prop.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Inc/usb_pwr.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `USB-FS-CDC_User/Src/hw_config.c` | Да | Полностью: application/BOOTLOADER ветви, USB enqueue/debug/reset/asynch, clocks/serial; B06/B33, tx PASS. |
| `USB-FS-CDC_User/Src/usb_desc.c` | Да | Все byte descriptors/string lengths/endpoints/PMA ограничения сверены с заголовками; новых ошибок не найдено. |
| `USB-FS-CDC_User/Src/usb_endp.c` | Да | Полностью: IN/OUT/USB callback контракты, длины RX64/TX64, USART alternative; tx/rx PASS. |
| `USB-FS-CDC_User/Src/usb_istr.c` | Да | Полностью: CTR/reset/error/SOF/ESOF и условные WKUP/SUSP; app recovery bounded, boot B36; callback-таблицы const, 56 байт RAM освобождены. |
| `USB-FS-CDC_User/Src/usb_prop.c` | Да | Полностью: reset/setup/line coding/descriptors; B05; тест всех string indices PASS. |
| `USB-FS-CDC_User/Src/usb_pwr.c` | Да | Полностью: power/suspend/resume/STOP; B06/B36; SUSP/WKUP отключены текущим IMR. |
| `User/crc.c` | Да | Обе CRC16 и CRC8; CRC-тесты/входные длины просмотрены, новых дефектов алгоритма не найдено. |
| `User/crc.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `User/flash_w25.c` | Да | Все read/page split/status/erase/busy/write-enable; B23/B34; реальное W25-тiming открыто. |
| `User/flash_w25.h` | Да | API страниц 256/секторов, status/void результаты проверены; B23. |
| `User/indication.c` | Да | Все шаблоны/сдвиг/LEDSCOUNT и индексы проверены; третий LED управляется отдельно, не объявлен ошибкой. |
| `User/indication.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `User/memory.c` | Да | Все cfg_prepare/select/publish/mirror/recovery и публичные операции; storage PASS; B23–B25. |
| `User/memory.h` | Да | Сектора/слоты/размеры, void API; границы проверены существующим storage-тестом. |
| `User/outs.c` | Да | Все mode/импульс/пауза/периоды/индексы выходов проверены; новые доказанные ошибки не найдены. |
| `User/outs.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `User/photo.c` | Да | Весь автомат capture/ACK/очистки/recovery; photo/heap тесты PASS; взаимодействие RS485/питания остаётся аппаратным ограничением. |
| `User/photo.h` | Да | Размеры записи/ACK и все интерфейсы проверены; резерв scratch и оба Flash-профиля. |
| `User/user_fun.c` | Да | Все функции и каждый case команды: B01–B04/B07–B14/B19/B25–B30/B39–B42; Q1–Q5; крупные сканеры/heap/временные строки. |
| `User/user_fun.h` | Да | Все контракты проверены; void сохранение, несайзированный parser, mem[16]; B12/B25. |
| `User/user_gsm.c` | Да | Все состояния/соединения/callback; DATA/timeout/AT reentry, фактический PREINIT пустой; стек проверен на выбранных путях. |
| `User/user_gsm.h` | Да | Все структуры, enum и callback проверены; сроки/разрядности и размер MAX_CONNECTIONS. |
| `User/user_str_fun.c` | Да | Все строковые/hex/UCS2/translit функции; требуют NUL/ёмкость от вызывающего кода; B10/B11/B16. |
| `User/user_str_fun.h` | Да | Проверены контракты: ряд функций не принимает размер destination; ответственность caller. |
| `User/wiegand.c` | Да | Все EXTI/process, сброс состояния и счётчик; B38; Q6 (паритет WG26). |
| `User/wiegand.h` | Да | Все объявления/константы и связи с соответствующей реализацией проверены; отдельной новой находки нет. |
| `board.c` | Да | GPIO/клоки/PVD/питание; B31, аппаратные проверки питания открыты. |
| `board.h` | Да | Оба аппаратных профиля, определения GPIO/адресов; проверены связность с Modules и Bootloader. |
| `includes.h` | Да | Порядок включений и общие макросы/адреса; конфликт адресации не найден. |
| `main.c` | Да | Запуск, VTOR, heap/stack paint, порядок обслуживания цикла; stack-mark/Unicorn PASS; адреса сохранены. |
| `main.h` | Да | Интерфейсы запуска и измерения стека проверены. |
| `settings/DomovoiNext.icf` | Да | Просмотрен; не выбран .ewp. Старое размещение от 0x08000000; не использовать для приложения с загрузчиком. |
| `settings/DomovoiNext2.icf` | Да | Активный: ROM/vector 0x08008200, RAM до 0x20004FBF, fault 0x20004FC0, stack/heap; map подтверждён. |
| `stm32_it.c` | Да | Все IRQ/fault_capture, сохранение fault_record, проверка границ стека; B31; реальные faults не инжектировались на плате. |
| `stm32_it.h` | Да | Объявления обработчиков, volatile fault_record и magic проверены. |
| `stm32f10x_conf.h` | Да | Включённые ST-модули; assert отключён — неверные параметры не защищены assert_param. |
| `structures.h` | Да | Все enum/packed структуры; B11, Q1/Q3. Статические размеры взяты из map, а не комментариев sizeof. |

## Ошибки и открытые риски

Строки относятся к итоговому дереву; для удалённой строки указано место функции и исходный фрагмент. «Код» означает подтверждённую последовательность операций в исходнике; аппаратные последствия не воспроизводились на плате. «План» означает необходимый новый тест перед будущим исправлением, а не уже пройденный тест. Все перечисленные открытые проблемы остаются в этой ветке.

| ID | Файл:строка / сценарий | Что не так и чем грозит | Тест / доказательство | Статус |
|---|---|---|---|---|
| B01 | `User/user_fun.c:2845`, команды 13–16 | При плохой второй записи прежний `break` выходил только из цикла: уже изменённый сектор записывался, после PARAMERROR отправлялись timestamp/успех. Индексы за областью также не отвергались. | `sync`: плохая вторая запись, переход сектора, нулевые/предельные индексы, группа 0, длины, отказ heap, правильная запись; каждый case падал до правки. | Исправлено, `e0314a5`: полная валидация до изменений/выделения памяти. |
| B02 | `User/user_fun.c:491`, вложенный RX при полном кадре | При удерживаемом обработчиком кадре 2048 байт следующий байт записывался за `server_rx_buffer`. | `serverrx`: полный кадр + вложенный байт; до правки счётчик выходил за 2048 и кадр повреждался. | Исправлено, `5390d7c`. Непоместившийся хвост отбрасывается, требуется повтор сервера. |
| B03 | `User/user_fun.c:1774` | Адрес ID-флеш хранился в `uint8_t`: вычислялся CRC посторонних адресов, мешая синхронизации. | `crcblocks`: 100 адресов блоков, оба канала и CRC пакета; до правки неверен первый адрес. | Исправлено, `0d84792`, адрес `uint32_t`. |
| B04 | `User/user_fun.c:2217`, удалённое `memcpy(bf, send_buffer,128)` | Для одной записи пакет выделен на 48 байт; ненужная копия читала 80 байт за heap-объектом. | `logupload`: инструментированный memcpy и выгрузка 1–32 записей по USB/серверу; до правки выход за выделение. | Исправлено, `64a2fd4`. Убрана копия 128 байт; общий буфер 128 байт ещё нужен другим функциям. |
| B05 | `USB-FS-CDC_User/Src/usb_prop.c:360` | Индекс строкового дескриптора 4 принимался при таблице из четырёх элементов 0–3. | `usbdesc`: все 256 индексов; до правки обращение к пятому элементу. | Исправлено, `f0eb5c2`. |
| B06 | `USB-FS-CDC_User/Src/hw_config.c:138` → `CMSIS/STM32F10x/system_stm32f10x.c:240` | `Leave_LowPowerMode` через SystemInit возвращал VTOR к `0x08000000`; следующий IRQ мог попасть в загрузчик. В текущем IMR_MSK suspend/wakeup выключены: дефект условного пути, не воспроизведённый текущий сбой платы. | `usbresume`: stub SystemInit сбрасывает VTOR; проверены конфигурация/отсутствие конфигурации и базы приложения/загрузчика. До правки падает приложение. | Исправлено, `8448506`, исходный VTOR восстанавливается. |
| B07 | `User/user_fun.c:235,256,277` | `sprintf("00%08X")` записывает 10 символов и NUL в `tb[10]`: запись одного байта за стеком при каждом WG/RF433/RFID ID. | `idtext`: проверка доступной ёмкости реального локального массива перед sprintf; 0, обычный и максимальный ID, ответы/управление/отладка. До правки переполнение. | Исправлено, `bf4cb38`, `tb[11]`; передаваемые 10 байт прежние. |
| B08 | `User/user_fun.c:541–542` | DEBUG_USER включён; произвольный текст SMS вставляется sprintf в общий буфер 128 байт. Длинная SMS переписывает соседние глобальные данные ещё до проверки PIN. | Тест `Tests/test_sms.c`, группа `sms debug`: SMS 0/1/110/111/112/160/2048, guards, CRLF и PIN; FAIL → PASS. | Исправлено, этап 1: `39f02fe`. |
| B09 | `User/user_fun.c:587` и остальные вызовы в SMS-обработчике; `:678` | Вызывается `user_sendsms(num,v)`, хотя контракт `(mes,num)`. Модем получает текст ответа вместо номера и номер вместо сообщения; ещё и журналируется неверный номер. Команда 11 использует правильный порядок. | Тест `Tests/test_sms.c`, группа `sms`: все места отправки, реальные SMS helpers, номер/текст gsm_sendsms и номер журнала; FAIL → PASS. | Исправлено, этап 1: `90fed58`. |
| B10 | `User/user_fun.c:592–597` | `strncpy(...,arg,10)` не ставит NUL для ID из десяти символов; последующий strcat читает продолжение общего буфера. Есть копирование 10 байт из короткого SMS-аргумента без отдельного ограничения. | Tests/test_sms.c: sms addbounds/addshort, длины 0–10, хвост без NUL и точные границы аргумента; FAIL → PASS. | Исправлено, этап 2: `6d9d145`. |
| B11 | `User/user_fun.c:1143–1162,1185,1294,3195` | Правильный CRC настроек/device не гарантирует NUL в server1/server2/NTP/netOperator/IMEI. Затем strcpy/sprintf/skipchar читают за полем и могут переполнить временную строку или device. | Tests/test_config_strings.c: cfgstrings (load/device/command12), правильный/плохой CRC, NUL, сохранность нормальных строк и reboot; FAIL → PASS. | Исправлено, этап 2: `c805f9e`. |
| B12 | `User/user_fun.c:2886–2900,2925–2928` | У команд 1,2,3,6,18,23,102,105,108 нет единой проверки минимальной/точной длины аргументов перед чтением. CRC всего кадра этого не исправляет: короткий кадр подставляет CRC/старые байты вместо времени, PIN и параметров. | Tests/test_command_lengths.c: cmdlengths, все короткие длины девяти команд, USB/сервер, штатные/расширенные кадры, CRC/PIN, без действий; FAIL → PASS. | Исправлено, этап 2: `65cb20b`. |
| B13 | `User/user_fun.c:2776–2800,3522` | Номер блока читается до проверки `n`; при ошибочном блоке `startWaitFWtime` сбрасывается до BADFRAME. Короткие кадры читают байты вне полезной нагрузки, мусор может откладывать повтор запроса. | Tests/test_fw_block_bounds.c: fwblockbounds/errors, n=0/1/2/1025/1026, индекс/повтор/BADWRITE, таймер и DEBUG_USER; FAIL → PASS; fwstart PASS. | Исправлено, этап 2: `70dc487`. |
| B14 | `User/user_fun.c:486–487` | Таймаут RX вызывает server_rx_reset даже при `server_rx_busy`; вложенный RX спустя 30 с может освободить/перезаписать ещё используемый буфер команды. B02 защищает только переполнение ёмкости. | Tests/test_server_rx.c: serverrx, reentry 29999/30000/30001 мс, deferred, wrap, zero input, полный буфер; FAIL → PASS. | Исправлено, этап 2: `83dd5b0`. |
| B15 | `Modules/gsm.c:539–550,647–655` | Разбор объявленной длины +IPD/+QIURC не доказывает наличие всего тела в `size`. Объявленная длина передаётся в gsm_dataread/серверный memcpy: неполный URC читает за входными данными. | Код; план: настоящий gsm_read с каждым разрывом заголовка/тела, длинами 0/2048/65535, а не stub parser. | Не исправлено. |
| B16 | `Modules/gsm.c:506–523,702–705,759–760` | Смещения CREG/CGREG читаются без достаточной длины строки; для CLOSED/CONNECT применяется `ps-3` без проверки начала. Короткие/нестандартные ответы читают чужие байты. | Код; план: короткие URC и ключевое слово с позиции 0, защитные области. | Не исправлено. |
| B17 | `Modules/gsm.c:400–765`, `:209–215` | Поиск текстовых признаков по всему буферу может принять бинарные данные TCP за ERROR/RING/CLOSED. Нет полноценного последовательного разбора нескольких бинарных URC; stash молча отбрасывает данные при нехватке места/heap. | Код; план: два URC подряд, NUL внутри payload, слова модема внутри payload, заполненный stash. `gsmurc` проверяет stash, но gsm_read там заменён stub. | Не исправлено. |
| B18 | `Modules/uart_stm32f10x.c:268–272` | Из-за отсутствия фигурных скобок receiving=0 выполняется каждый миллисекундный тик, а не после twait. Парсер видит ложную паузу и забирает ещё незаконченный кадр. | Тест `Tests/test_uart_timer.c`, группа `uarttimer`: все порты, тики 1..10, новый байт, нулевой twait; FAIL → PASS. | Исправлено, этап 1: `1e363a4`. |
| B19 | `Modules/uart_stm32f10x.c:11–24,225–234,348–362`; `User/user_fun.c:1380–1407` | Поля UART, изменяемые ISR/SysTick, не volatile; нет атомарного снимка счётчика/передачи владения. При приёме во время memcpy/uart_clear новый хвост теряется, а изменение счётчика во время очистки гоняется с ISR. | Код; план: управляемый RX между snapshot/copy/clear плюс IAR-проверка чтений. volatile в одиночку гонку не устранит. | Не исправлено; при разборе на месте обязательно решить владение. |
| B20 | `Modules/timers.c:6–7,23,35–45` | milliSeconds/timer[] разделяются с IRQ без volatile и защиты read-modify-write. Возможны неверная оптимизация ожидания и потеря тика при timers_set; наличие ошибки в конкретном машинном цикле IAR отдельно не доказано. | Код: нарушенный контракт IRQ; план: Unicorn с внедрением SysTick и анализ свежего ELF. | Открытый риск, не исправлено. |
| B21 | `Modules/uart_stm32f10x.c:286–307,388–469` | У полного RX-буфера нет гарантированного завершающего NUL, но ответы ищутся строковыми функциями. Поиск может выйти из UART-области в соседние данные. | Код; план: ровно 2048 ненулевых байт, pattern отсутствует, guard после буфера. | Не исправлено. |
| B22 | `Modules/uart_stm32f10x.c:126,146` | RCC_APB2Periph_GPIOA/B переданы в RCC_APB1PeriphClockCmd. Это устанавливает биты другого регистра и не включает GPIO. Сейчас скрыто предварительным board_init. | Проверена реализация RCC и константы; план: одиночный uart_init для USART2/3, маски APB1/APB2. | Не исправлено; латентный неверный вызов ST. |
| B23 | `User/flash_w25.c:59,146–198`; `User/memory.c:203–253` | Результат ожидания W25 не передаётся наверх; write/erase void, нет общего подтверждения WEL/программирования/стирания. Обычные команды могут ответить успехом при отказе памяти. Фото/FW/config в ряде путей делают собственный readback — это не защита всех записей. | Код; план: stuck BUSY, WEL=0, bit-flip после program/erase и запрет SUCCESS. | Не исправлено. |
| B24 | `User/memory.c:246–253`; `User/user_fun.c:3203–3230` и cases 14–16 | Перезапись ID/групп/расписаний/праздников стирает единственную копию сектора. Потеря питания уничтожает и изменяемую, и соседние записи. Валидация B01 не даёт атомарности при выключении питания. | Код; план: power-cut на каждом erase/page/commit, восстановление целого сектора. Тесты config/photo/boot не покрывают эту область. | Не исправлено. |
| B25 | `User/user_fun.c:1340,3152,4445`; `User/memory.c:209` | NULL проверяется, но void API скрывает неудачу сохранения. cmd12 может дать timestamp при несохранённых настройках; user_updateID может не записать уменьшенный passCount, хотя выход уже активирован. Это обход ограничения проходов при повторении OOM. | Код; `test_iar_mcu_memory.py` доказывает реальную нехватку/фрагментацию heap; план: отказ malloc в применении config/passCount и контроль ответа/выходов. | Не исправлено. |
| B26 | `User/user_fun.c:4501–4520,4565–4582` | Журнал без CRC/commit записи: частично записанный index принимается за событие. При flushPending/setTime результат log_addEvent игнорируется, pendingCount очищается даже при полном журнале. | Код; план: torn-record и заполненный журнал/отказ записи при flush, события должны оставаться в очереди. | Не исправлено. |
| B27 | `User/user_fun.c:4525–4548,4338–4358` | При полностью занятом журнале/tempID сканирование возвращает count, но не ставит newAddress=end; остаётся прежний курсор. Возможны неверное окончание выгрузки/попытка записи в занятую область. | Код; план: полностью занятая область с начальным курсором start и перезагрузка. | Не исправлено. |
| B28 | `User/user_fun.c:2282–2297,2366–2381,2239–2242` | Курсор выгрузки журнала/tempID двигается независимо от результата usb_send/connection_send. После конца серверного журнала flash очищается без проверки доставки завершающего пакета. При BUSY/ошибке записи теряются для текущего сеанса; завершение может удалить недоставленное. | Код; план: transport=false/CE_BUSY на каждом пакете и конце. Семантику подтверждения конца также сверить с Протокол2. | Не исправлено. |
| B29 | `User/user_fun.c:920–939,1030–1046`; `Bootloader/app_update.c:88` | Главный цикл после долгой операции обрабатывает только одну секунду и обнуляет накопленный таймер. datetime/worktime, срок FW и trial отстают от реального времени; 600 секунд загрузки растягиваются при блокирующих AT/flash операциях. | Код; план: задержки 5/60 секунд и восстановление после reset; проверка E97/E442. | Не исправлено. |
| B30 | `User/user_fun.c:2458–2467` | Абсолютные Unix-сроки вычисляются `firstDate + useTime*60` в uint32 без защиты переполнения. При времени близко к UINT32_MAX ID может стать истёкшим раньше срока. Это отдельная проблема Unix-времени, не сам wrap millis на 49-й день. | Код; план: UINT32_MAX−60 и границы useTime. | Не исправлено. |
| B31 | `board.c:93–107`, `stm32_it.c:282` | Включены PVD и NVIC, но нет конфигурации EXTI_Line16; PWR_PVDCmd только включает детектор. Предполагаемая аварийная реакция PVD через IRQ не обеспечена этой инициализацией. | Проверены PWR/NVIC/EXTI вызовы; план: модель IMR/RTSR/FTSR и измерение питания на плате. | Не исправлено. |
| B32 | `STM32_USB-FS-Device_Driver/src/usb_core.c:278–293,349–364,743–757` | Регистры endpoint читаются до проверки номера endpoint из USB SETUP. Некорректный wIndex адресует регистры вне EP0..EP7, в том числе управляющую область USB. | Код; план: все wIndex для GET_STATUS/CLEAR_FEATURE/SET_FEATURE, запрет MMIO до проверки. | Не исправлено. |
| B33 | `STM32_USB-FS-Device_Driver/src/usb_mem.c:50–61`; `USB-FS-CDC_User/Src/hw_config.c:345` | UserToPMABufferCopy на нечётной длине читает лишний байт. При n=1 и хвосте кольца в индексе 1279 это чтение за USART_Rx_Buffer. Обратная функция также пишет округлённую длину; текущие RX64/LINE_CODING8 дают padding, но контракт не безопасен для произвольного буфера. | Код; план: нечётные длины, хвост кольца, защищённая область; существующий tx использует stub PMA. | Не исправлено. |
| B34 | `Modules/spi_stm32f10x.c:52,80–101`, `Modules/adc_stm32f10x.c:73,78,121–133`, `Modules/uart_stm32f10x.c:167–191`, `CMSIS/STM32F10x/system_stm32f10x.c:847,856` | Ожидания TXE/RXNE/BSY/калибровки/EOC/PLL не имеют лимита/обработки ошибки. Зависание до включения IWDG может быть постоянным, после включения — reset в середине операции. Во flash-driver watchdog внутри некоторых ожиданий есть, но не в зависшем нижнем SPI. | Код; план: каждый флаг stuck; аппаратные времена/IWDG проверять отдельно. | Не исправлено. |
| B35 | `Modules/uart_stm32f10x.c:286–307`; `User/user_fun.c:1384–1388` | Ожидание ответа перезапускает срок при каждом байте, а ожидание окончания RX кормит watchdog. Непрерывный шум/поток способен удерживать главный цикл без конечного срока. | Код; план: постоянные байты без pattern/паузы, общий дедлайн должен сработать. | Не исправлено. |
| B36 | `USB-FS-CDC_User/Src/usb_istr.c:237`; `USB-FS-CDC_User/Src/usb_pwr.c:147` | В boot-ветви ESOF восстановление ждёт RESET без лимита в IRQ. При отсутствии RESET main и boot_service не исполняются, IWDG может оборвать текущую flash-операцию. В Suspend есть ещё бессрочное ожидание, но текущая маска SUSP отключена. | Код; план: сборка BOOTLOADER с RESET stuck и отдельный условный suspend-путь. App recovery имеет ограниченный срок. | Не исправлено. |
| B37 | `Bootloader/hw_config.c:55–59,408–431` | Собственная TX-очередь загрузчика разделяется с USB IRQ без volatile/atomic enqueue. При заполнении и USB_Tx_State=1 выражение timeout-- после нуля переводит uint32 в UINT32_MAX, поэтому проверка timeout==0 не завершает внешний цикл. При USB_Tx_State=0 и полной очереди тоже возможен цикл без продвижения. Если IRQ не освобождает очередь, функция зависает; возможен reset IWDG. | Код; план: stalled host при TX_State=0/1, burst replies, reset/IRQ между шагами enqueue. App tx-тест не проверяет этот отдельный файл. | Не исправлено. |
| B38 | `User/wiegand.c:4–8,68–82` | IRQ-поля не volatile, snapshot/reset не защищён. Импульс между чтением и очисткой теряется; uint8 bitCount после 256 импульсов снова может стать 26 и принять длинный шум за кадр. | Код; план: EXTI между шагами process и 282 импульса без паузы, защита границ. | Не исправлено. |
| B39 | `User/user_fun.c:3798–3813` | Новый tempID создаётся из стёртой ячейки 0xFF; устанавливается один type-bit, но остальные не обнуляются и остаются 1. Метаданные типа неверны. Сейчас выгрузка tempID использует только десять байт ID, поэтому отдельный отказ доступа от этих флагов не доказан. | Код; план: пустая flash, все codeType, проверка ровно одного нужного флага и reserved/default полей. | Не исправлено; латентный дефект данных. |
| B40 | `User/user_fun.c:733–768,1111–1124` | Автоматические ConnectParam_t/GsmParam_t не обнулены; cp.Error/param.ComandError копируются неинициализированными. Активные ветви сейчас не вызывают их (maxConnectCount=0/ATERROR пустой), но включение callback-пути приведёт к прыжку по мусору. | Код; план: захват передаваемых структур и error-callback path. | Не исправлено; текущий runtime-сбой не заявляется. |
| B41 | `User/user_fun.c:1435–1438` | У публичного user_send_anyData `uint16_t len=size+3` переполняется при size>=65533, затем маленькое выделение получает большую memcpy. Найденные текущие callers передают малые размеры; внешняя достижимость такого size не найдена. | Код; план: граничные size, запрет переполнения до malloc. | Не исправлено; латентная ошибка API. |
| B42 | `User/user_fun.c:4248` | Unix datetime uint32 передаётся в div(int,int). После границы INT32_MAX сравниваются signed-частные вместо unsigned номера UTC-дня; праздник может определяться неверно. | Код; план: даты/полночь по обе стороны 19.01.2038 и unsigned reference. | Не исправлено. |

Не найдено явного разыменования результата malloc без проверки в просмотренных активных местах выделения. Не найдено доказанной постоянной утечки: photo-контекст освобождается в idle, URC-buffer после обработки, packet/sector буферы в обычных выходах. Это не исключает описанных отказов из-за одновременно живых объектов и фрагментации. malloc/free из аппаратных ISR не обнаружены.

## ОЗУ: карта IAR и варианты экономии

Источник статических размеров — **только `Release/List/DomovoiNext.map`** свежей Release-сборки, без `sizeof` GCC. Снимок чисел и SHA256 итоговой карты: [MCU_AUDIT_MEMORY.json](MCU_AUDIT_MEMORY.json); снимок до оптимизации таблиц: [MCU_AUDIT_MEMORY_BEFORE.json](MCU_AUDIT_MEMORY_BEFORE.json), код `bf4cb38`. ELF используется для запуска ARM-кода; DWARF в новом тесте нужен только для адресации полей состояния GSM, не для оценки статической ОЗУ. Динамические сценарии измерены настоящим IAR allocator в Unicorn и отдельно обозначены как измерения, а не данные карты.

До оптимизации карта давала readwrite DATA **19 804** и CODE **636**, обычные данные без stack/heap/fault **10 540**, с fault_record **10 588**, щель перед fault **24 байта**. После переноса pEpInt_IN/pEpInt_OUT: readwrite DATA **19 748** (включает CSTACK 4096, HEAP 5120 и fault_record 48), readwrite CODE прежние **636** — необходимые функции записи загрузчика из RAM. Обычные данные без stack/heap/fault теперь **10 484**; с fault_record — **10 532**. Сумма DATA+CODE = **20 384** из 20 480. Непрерывная щель перед fault_record выросла до **80 байт**; ещё 16 остаются после фиксированной аварийной записи. Эти две щели нельзя считать единым расширением heap. Число «10 584 переменных» из постановки не подменяет числа карты; 636 байт RAM-кода также надо учитывать. Резервы heap/stack не изменены.

Крупные символы карты: gsm_buffer **2048**, server_rx_buffer **2048**, **usb_buffer 2048** (ещё один крупный потребитель), USART_Rx_Buffer **1280**, log_pending **512** + log_pendingAt **64**, config/device по **512**, mem **128**, общий tempStrBuffer/bf **128** (union, не два независимых массива), gsm_strbuf **128**, uart **48**, port_buffer **32**, fault_record **48**.

Измерение heap: на пустой куче максимальный запрос **5112** при резерве 5120. Две копии по 2056 помещаются, но оставляют максимальный запрос только **984**. URC 2056 + запрос сектора 4096: второй запрос **не проходит**, максимальный свободный запрос 3048. URC 2056 + scratch 256: оба проходят, максимальный свободный запрос 2784. После четырёх выделений по 1000 и освобождения 1-го/3-го остаются два живых объекта, но максимальное непрерывное выделение только **1080**. Дополнительный существующий тест фото измерил peak занятых allocator-chunks **4864/5120**; это не величина статических данных из карты.

Ниже потенциальные выигрыши рассчитаны от размеров объектов/резервов карты и выбранной новой ёмкости. Это **проектные варианты**, не подтверждённые размеры ещё не собранной реализации. Статическая ОЗУ и peak heap не складываются автоматически; одни и те же освобождённые байты нельзя посчитать дважды.

| Вариант | Потенциальные байты | Что меняется / необходимые проверки | Риск | Сделано |
|---|---:|---|---|---|
| Неизменяемые USB IRQ callback-таблицы в Flash | **56 статической ОЗУ, фактически** | const у pEpInt_IN/pEpInt_OUT и согласованные extern. Новый тест карты до правки падал на RAM-адресе; после проверяет Flash-адреса, размеры 28+28 и все 14 указателей callback. | Низкий: записей в эти таблицы в проекте нет; IAR/host/boot/Unicorn проверены. | **Да, `a7d598b`**. |
| Остальные неизменяемые USB tables в Flash | До **134 статической ОЗУ**: Device_Property 48 + User_Standard_Requests 36 + descriptors 8+8+32 + Device_Table 2 | Нужны const-qualified контракты библиотеки; Device_Info/LINE_CODING/serial остаются изменяемыми. Проверять обе ветви USB/boot и карту. | Низкий/средний: больше мест смены типов, сохранение callback API. | Нет. |
| log_pendingAt: использовать datetime внутри ещё не записанной pending-записи | До **64 статической ОЗУ** | Временно хранить worktime в cl.datetime, при получении времени преобразовать в epoch, при dateless flush вернуть 0. Ёмкость очереди/выходной формат сохраняются. | Средний: доказать все flush/time/timeout пути существующими log-тестами и исправить B26; pending-время не должно попасть в flash без преобразования. | Нет; перспективнее уменьшения числа записей. |
| ucon: четыре соединения → два | До **72 статической ОЗУ** от ucon 144 карты | Видимый код создаёт основной серверный канал, но библиотека допускает четыре; нужны reconnect/handle lifetime/все модемы. | Средний/высокий: ограничение ёмкости API, сохранение поведения пока не доказано. | Нет. |
| RCC lookup tables без volatile в Flash | До **20 статической ОЗУ** от APBAHBPrescTable 16 + ADCPrescTable 4 карты | Таблицы заданы константами, но __I (volatile const) IAR разместил в RAM; кандидат const-only с повторной проверкой clock calculations/map. | Низкий/средний, но затрагивает vendor ST за рамками аудита неверных вызовов; не выполнялось. | Нет. |
| 1. Разбирать UART на месте | До **2056 peak heap** запроса копии; измеренный chunk 2064 | Убрать calloc(count+8)/memcpy/free в gsm_check_data; ввести bounded parser с длиной, передачу владения ISR→main, обработку URC без вложенной очистки того же буфера. | Высокий: RX продолжается, gsm_read меняет SMS, вложенный AT очищает UART, указатели callback живут в нём. Просто заменить указатель нельзя. | Нет; идея проверена, B15–B21 мешают безопасной замене. |
| Убрать/перепроектировать stash URC | До ещё **2056 peak heap** (chunk 2064) | Один последовательный parser обслуживает ответ AT и URC; маленькое состояние неполного заголовка вместо дополнительного heap-буфера. | Высокий: два URC, бинарный NUL, неполный TCP и вызов AT внутри callback; нельзя объединять gsm_buffer и server_rx_buffer, пока кадр сервера удерживается. | Нет. |
| 2. Сектор 4096 → страница 256 + транзакция во flash | **3840 peak heap** на один одновременно живой секторный scratch | Потоковая сборка сектора, overlay изменённых записей, readback/CRC, commit и recovery до использования данных. Существующий config publisher уже использует страницы, но его вызывающие функции всё ещё требуют 4096. | Средний/высокий: дополнительный износ и время стирания. Один запасной сектор не хранит одновременно 4096 данных и метаданные; без journal повторная запись после power-cut небезопасна. | Нет; allocator-модель нового scratch проходит. |
| Потоковое сохранение config/device | До **3840 peak heap** для вызывающего буфера; не добавлять к предыдущему варианту, если это тот же объект | Собирать страницу из flash + config/device, сохраняя текущие два слота, CRC-last и legacy mirror. Убрать внутренний malloc(4096) для мелкой config-записи. | Средний: recovery и совместимость со старым загрузчиком; нужны существующие storage power-cut тесты для всех новых путей. | Нет. |
| 3. Уменьшить CSTACK 4096 → 3072 | **1024 статической ОЗУ**, только если доказан максимум | Устранить неоднозначность call graph и измерить худшие вложенные пути/IRQ. Из карты нельзя объявить 2400 окончательным максимумом. | Сейчас высокий; прежние 2048 меньше даже отмеченной IAR оценки 2096 без IRQ. | **Не сделано; уменьшение пока не обосновано.** |
| Блоки чтения/CRC 1024 → 256 | До **768 на один живой стековый буфер**, не статическая экономия сама по себе | CRC потоковый; сканеры должны сохранить порядок, границы записей, счётчики и обслуживание watchdog. Типы записей кратны странице. | Низкий/средний после эквивалентностных тестов; больше SPI-вызовов может замедлить чтение. CSTACK уменьшать отдельно после измерений. | Нет. |
| mem[16] → mem[10] | **48 статической ОЗУ**, от 128 байт карты и 6 неиспользуемых из 16 слотов | Enum заканчивается MB_PHOTO=9; во всех найденных обращениях используются эти десять блоков. Согласовать extern и добавить проверку диапазона API. | Низкий; проверить отсутствующий внешний код/будущие резервные блоки. | Нет. |
| TX USB 1280 → 1088 | **192 статической ОЗУ** | Наибольший штатный пакет журнала 1038 помещается с одним пустым слотом кольца; тесты burst/debug/reset/ошибок обязательны. | Средний: меньше места для очереди отладки; поведение при перегрузке меняется. Не считать безусловной оптимизацией. | Нет. |
| log_pending: 16 → 8 записей | **288 статической ОЗУ** от 576 карты | Раньше flush, меняется поведение событий до получения времени. | Средний/высокий: потеря timestamp/событий при полном журнале. Не подходит под «не менять поведение» без иного хранения. | Нет; не рекомендовано первым шагом. |
| Убрать usb_buffer 2048 либо server_rx_buffer 2048 | До **2048** каждого, только при новой потоковой схеме | Транспортные буферы нужны для независимых неполных кадров и заявленной максимальной длины. Sharing возможен лишь с явным запретом конкурентных владельцев/реэнтрантности. | Высокий; простое объединение меняет допустимые кадры и обработку каналов. | Нет; не рекомендовано простое сокращение. |
| Убрать ненужный memcpy из выгрузки журнала | **0 статической ОЗУ**; на 128 байт меньше копирования/чтения на пакет | Удалён неиспользуемый результат копирования; wire-data и CRC проверены. | Низкий. | Да, B04. |
| Сократить heap 5120 | Пока **0 доказанных байт** | Сначала убрать секторные выделения/двойные копии, затем измерить одновременные объекты и фрагментацию. | Высокий: уже сейчас сектор не помещается рядом с URC; фото peak 4864 оставляет малый запас. | Нет. |
| Убрать RAM-код записи загрузчика / fault_record / port_buffer | **0 безопасно обоснованных байт** | RAM-код нужен при стирании internal Flash, fault_record нужен диагностике, port_buffer уже сокращён и нужен RS485. | Удаление нарушит имеющиеся функции. | Не предлагается. |

Для общего scratch-журнала по нынешним округлённым erase-границам mem[] свободны, например, **0x1F9000 и 0x1FA000**: после config slots 0x1F5000..0x1F8FFF, до конца 2-МБ области и до фото с 0x200000. Это предложение распределения **внешней flash**, а не уже занятые или изменённые адреса. До реализации проверить все старые инструменты/образы и добавить storage-тест резервирования. Нельзя использовать boot-journal или photo scratch как общий сектор: у них отдельные владельцы и recovery.

Порядок безопасной секторной операции: восстановить/завершить прежнюю транзакцию → инвалидировать/стереть старые metadata и проверить это до повторного использования scratch → подготовить scratch-data страницами → проверить весь образ → последним записать commit metadata с адресом/CRC → стереть/восстановить destination → проверить destination → завершить/очистить journal. Recovery должен повторять перенос из уже подтверждённого scratch при любом обрыве. Это атомарность одного сектора, а не всей команды из нескольких секторов; timestamps подтверждать только после всех проверенных и сохранённых записей. Прямой overlay без journal экономит RAM, но оставляет B24.

### Стек и рекурсия

Свежая карта показывает **2096** Program entry и **304** interrupt Total Use, а не 2088 из исходной оценки. Арифметическая сумма **2400**, но строки Maximum call chain помечены `*?*`, есть Ls016/Ls017 и неизвестные indirect calls. Это оценка неполного графа, не доказательство пригодности CSTACK=2400 или 3072.

IAR перечисляет цикл `gsm_dataread → connection_received → user_dataconnection → server_rx_process → user_parse_comand → user_savereset → gsm_pwrProc → gsm_dataread`. В исходнике обратный вызов gsm_pwrProc — PREINIT, а обработчик PREINIT сейчас пустой; PO_OFF при user_savereset также меняет достижимость. Поэтому предупреждение IAR не доказывает бесконечную фактическую рекурсию. При этом исключение ложных дуг, привязки callback и возможное вложение IRQ должны быть доказаны до сокращения стека.

Новый Unicorn-тест запускает машинный код Release: user_calcCRC16InFlash для 0/1/257/1024/16000 байт и проверяет результат независимым CRC; измерены 1048/1056 байт стека с stub mem_read. PREINIT отдельно использует 64, реальная цепочка gsm_pwrProc(PO_ON,5000)→PREINIT — 80. Эти конкретные пути **не моделируют максимальный main+все IRQ**. Существующая stack-mark проверка проходит: весь резерв 4096, после вызова 4024 не затронуто, метка на +1000 возвращает 1000, полностью затёртая — 0. Команда 111/поле 57 остаётся способом собрать high-water mark на плате; это не выполненное аппаратное измерение.

## Оптимизация времени и повторяющегося кода

- В cases 13–16 одинаковые циклы sector read/patch/write. Общий движок overlay может убрать дублирование и реализовать один проверяемый recovery. Он должен сохранить wire-layout, ответы и порядок штампов. B01 уже вынесла общую валидацию; большой рефакторинг не выполнялся.
- После каждой команды ID пересчитываются число записей и CRC всей ID-области, а lookup/проверка срока сканируют flash. Это длинные синхронные операции. Можно считать изменение numbersCount по старой/новой ячейке и отдельно поддерживать id_cells_end, но удаление последней записи и старые неплотные базы требуют полного fallback-сканирования. Изменение CRC на инкрементальное без отдельной математической проверки не предлагается.
- user_getGroup/user_getShedule ищут индекс сканированием, хотя синхронизация пишет в индексную ячейку. Прямое чтение ускорит поиск, но изменит работу старых баз с несоответствием физического места/index. Нужен быстрый проверяемый путь + прежний fallback, либо подтверждённый инвариант данных.
- Стирание областей и ожидания модема задерживают обработку доступа, USB, камеры и секундного времени. Разнести стирание по страницам/секторам с отдельным состоянием полезно, однако прежний синхронный SUCCESS нельзя выдавать до завершения. Это изменение внутреннего планирования требует тестов порядка команд и занятости, не простая косметическая правка.
- В packet builders повторяются SLDN/header/CRC/switch-channel. Общий helper способен уменьшить Flash-код и единообразно обработать send failure. Статический выигрыш RAM из этой идеи не доказан; нельзя обещать ускорение без сравнения IAR map/машинного кода.
- User/crc.c уже содержит табличный CRC под CRC_TABLE_CALC; в текущей сборке используется побитовый вариант. Включение таблицы — кандидат ускорения CRC больших областей. Нужны тесты равенства всех CRC и новая карта IAR, подтверждающая размещение таблицы во Flash и её цену; эта оптимизация не реализована.
- gsm_check_data копирует UART, gsm_stashurc копирует URC, gsm_takeurc пытается realloc. Устранение двойных копий — основной кандидат снижения peak heap. Сначала исправлять границы и владение parser, затем менять хранение.
- `user_send_logRecords` больше не делает бесполезную копию 128 байт. В cfg_equal/cfg_equal_flash уже есть отказ от записи одинакового сектора; убирать эти сравнения ради скорости нельзя без оценки износа и потери поведения.
- Переход millis через UINT32_MAX рассмотрен: вычитание unsigned в server RX/status/photo и delay допускает wrap при интервалах меньше полного периода. Host-сценарии RX/photo включают переход границы. Универсальная ошибка «после 49 дней всё зависает» не подтверждена. Отдельно открыты B20, B29, B30, нулевой sentinel startWaitFWtime и переполнение долгосрочного Unix-времени; их не следует смешивать.

## Сверка протокола: что подтверждено и что открыто

Файл первичного «Протокол2» (xlsx/xls/ods) не найден в репозитории и доступном дереве СофтЭксперт. Проверены сохранённые требования в PROJECT_STATUS.md, структуры и Tests/. Это **не полная независимая сверка первичного документа**. Отчёт не выдаёт новые решения о протоколе за согласованные.

- Существующие `log`, `access`, `rules`, `commands`, `fwstart`, `photo`, `rs485cfg`, `crc` проверяют закреплённые правила событий, полей журнала, команд 8/11/12, загрузки, ack фотографий, CRC и RS485. Новые тесты B01/B03/B04 сохраняют формат корректных пакетов. Порядок ARG2=0 и изменённые поля журнала проверяются существующими тестами.
- **Q1 — праздники:** cellHoliday_s содержит только datetime, но case16 использует первый байт этой же структуры как номер ячейки. Нужен первичный формат записи cmd16: индекс отдельно или иной смысл byte0. Не исправлять «по догадке»; текущий годный CRC ещё не доказывает правильную адресацию праздника.
- **Q2 — cmd6:** idx=0 считает 100 блоков, отдельный idx использует `base + 16000*idx` и не ограничивается 100. Если протокол задаёт номера 1..100, это сдвиг на один блок; требуется подтвердить нумерацию и хвост ID-области. B03 исправляет только несомненное усечение адреса.
- **Q3 — события:** историческая запись PROJECT_STATUS.md о IDTIMEEND=130 отличается от текущего enum 138/139 и новых FW событий 132..137. Верхние записи статуса говорят о продолжающейся правке Протокол2. Нужна окончательная таблица; существующие тесты подтверждают принятое дерево c7046d1, а не все исторические редакции сразу.
- **Q4 — срок FW:** сохранены договорённые 600 секунд и восстановление после reset. В статусе остаётся вопрос E97 «с первого запроса» против E442 «с приёма cmd19». B29 дополнительно показывает зависимость счёта секунд от работы loop.
- **Q5 — журнал/tempID:** подтвердить, служит ли следующий запрос подтверждением предыдущей посылки и когда разрешено стирание. Даже при такой трактовке B28 — игнорирование отказа локального транспорта — остаётся ошибкой.
- **Q6 — Wiegand:** для принятой трактовки 26 бит как 24 данных + два бита паритета `(wgData>>1)&0x03FFFFFF` оставляет старший паритет внутри ID. Требуется подтвердить формат считывателя и правила проверки паритета; это пока гипотеза о формате, отдельная от подтверждённого переполнения строки B07 и гонок B38.

## Проверки и коммиты

Для **каждой** из семи правок и оптимизации: сначала соответствующий test падал на прежнем коде/карте, после правки проходил; затем все существовавшие на тот момент host-группы и четыре требуемые IAR-конфигурации проходили. В конце 21 группа: storage, fwstart, log, access, rules, serverrx, commands, gsmreg, gsmurc, crc, recovery, rx, photo, rs485cfg, tx, sync, crcblocks, logupload, usbdesc, usbresume, idtext. TX дополнительно проверен в idle/reset/esof/busy/large. Для sync дополнительно пройдены варианты неверных индексов команд 13–16.

Сборки: DomovoiNext Release/Release_IPv3 — **0 ошибок, 3 предупреждения**; DomovoiBoot Release/Modem_Release — **0 ошибок, 2 предупреждения**. Предупреждения приложения: Pe550 (`Modules/spi_stm32f10x.c:121`, неиспользуемая dt), Ls016/Ls017 (косвенные вызовы/рекурсия). Загрузчик: Ls016 и Pa082 (`Bootloader/boot_main.c:89`, неопределённый порядок двух volatile чтений tail/head). Порядок этих двух чтений сам по себе не доказывает функциональную ошибку SPSC-очереди; потребуется отдельная проверка interleaving. Предупреждения не скрыты и не объявлены устранёнными.

Host-тесты Bootloader/tests/run_tests.py: установка 206 power-cut позиций, откат 260, исчерпание trial 302, запрос trial rollback 300, torn journal/confirmation, ошибки backup, CRC/HW/size, восстановление, USB разрывы/overflow/retry/commit; manual110 — 129 reset позиций, плюс 6 тестов утилиты: PASS. Эти числа относятся к модели, не физическим выключениям питания.

Окончательные IAR-образы: `test_iar_cmd13_heap.py`, `test_iar_photo_heap.py`, `test_iar_stack_mark.py`, новый `test_iar_mcu_memory.py`, `Bootloader/tests/test_iar_boot_emu.py`, `test_iar_ram_writer_emu.py`: PASS. IAR boot emulator проверяет обычный запуск, установку/backup, три пробных запуска, откат и подтверждённую FW. RAM writer emulator: побайтовая замена 32-КБ загрузчика, выполнение остаётся в RAM, ошибки SPI на страницах 0/7 останавливают стирание повреждённой страницы, пять остановок не меняют область приложения.

Запуск отдельной группы: `py -3.11 Tests/run_host.py work <группа>`; TX: `... work tx <сценарий>`. Память: `py -3.11 Tests/test_iar_mcu_memory.py` после Release; зависимости Unicorn/pyelftools через `DOMOVOI_TEST_DEPS` либо существующий Release/photo_test_deps. Результат записывается в ignored `Release/audit-memory.json`; фиксированный снимок этого аудита приложен в Tests/. На данной машине launcher использовался абсолютным путём, потому что `py` отсутствует в PATH.

IAR запускался указанным iarbuild.exe с **абсолютным путём к .ewp**, `-build <конфигурация> -log warnings`. Числа карты относятся к Release, а не к сумме/смеси конфигураций.

Коммиты исправлений: `e0314a5`, `5390d7c`, `0d84792`, `64a2fd4`, `f0eb5c2`, `8448506`, `bf4cb38`. Оптимизация с измерительным тестом: `a7d598b`. Отчёт и оба снимка измерений оформлены следующим отдельным коммитом.

## Что нельзя подтвердить без платы

1. Настоящий максимальный stack high-water с IRQ, fault, одновременными USB/UART/RS485 и долгим AT; оценка IAR и выбранные Unicorn-пути его не доказывают.
2. Пики/фрагментацию heap на многосуточном смешанном потоке реальных URC, SMS, фото и синхронизации; эмуляция подтверждает конкретные отказы, но не распределение нагрузок.
3. Обмен всеми моделями модемов: реальные URC, разрывы, шум, пачки кадров, binary payload, задержки QISEND, SMS и переключение SIM. Подмена gsm_read в старом тесте не заменяет эту проверку.
4. Потерю питания в реальных erase/program: электрическое завершение операций W25/internal Flash, PVD/EXTI, сохранение напряжения, BOR и последующее восстановление. Модели power-cut покрывают алгоритм, не электронику.
5. Команду 110 при исчезновении питания во время стирания/замены самого загрузчика. После стирания его первых страниц нет независимой исполняемой копии для recovery; может потребоваться SWD. Программный тест это ограничение не устраняет.
6. Реальные сроки IWDG от LSI, максимальное время erase/program/ADC/PLL и reset посреди операции, в том числе при отказах SPI/модема. Не считать обслуживание watchdog доказательством допустимой длительности.
7. USB enumeration/ESOF/reset/подключение/отключение на разных хостах, PMA аппаратное поведение, пропускная способность кольца и suspend при включении соответствующей маски.
8. Wiegand/parity/электрические импульсы, дребезг входов, реальное управление реле, RS485-коллизии и физическую камеру обеих аппаратных ревизий.
9. Серверную совместимость и окончательное соответствие Протокол2 по Q1–Q6. Сервер не подключался, реальные SMS/команды не отправлялись.

Рекомендуемый следующий порядок: B08/B11/B12/B15–B21 (границы и владение) → B23–B28 (результаты записи/отправки и recovery) → потоковый scratch → повторные измерения heap/стека → только затем изменение резервов ICF. Реализована экономия **56 байт**; большое сокращение RAM в этом аудите **не реализовано**: варианты и их цена отделены от выполненных исправлений.
