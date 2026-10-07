# ДОМОВОЙ NEXT

Контроллер доступа: только плата v5 HW139, STM32F103CB, модем SLM336.
Перед работой прочитать PROJECT_STATUS.md; перед изменением поведения — docs/decisions.md.
Ответы и комментарии — по-русски. Проверки на модели не заменяют проверку платы.

## Карта проекта

- DomovoiNext.ewp / DomovoiNext.eww — единственный проект, Release / ReleaseCustomer; пространство включает загрузчик.
- main.*, board.*, stm32_it.*, structures.h — запуск, плата, прерывания и общие структуры.
- User/c/ — команды, доступ, ID, расписания, журнал, фото, обновление и автоматы хранения.
- User/asm/crc_asm.s — измеренное ядро CRC16/CRC32; C-эталоны только в Tests/reference/.
- User/ — контракты, рабочая память, форматирование, входы/выходы, Wiegand и диагностика.
- Modules/ — SLM336, UART/DMA, AT, TCP, звонки/SMS и периферия.
- CMSIS/ / STM32F10x_StdPeriph_Driver/ — ядро STM32 и используемые функции ST.
- STM32_USB-FS-Device_Driver/ / USB-FS-CDC_User/ — USB CDC и конечные точки.
- Bootloader/ — загрузчик v5, обновление через сервер/USB, настольная утилита и её тесты.
- Tests/ — host-модели, ELF, дифференциальные тесты, замер ядер, мёртвый код; asm/ — стенды исполнения ARM.
- Tests/fixtures/ — входы модема и контракты 212 сценариев для каждого профиля.
- Tools/Photo/ — эмулятор камеры, сервер фото, RS-485; запуск по отдельному сценарию.
- docs/decisions.md — решения; docs/protocol/ — форматы; docs/history/ — исторические отчёты.
- docs/cleanup_findings.md — исходный аудит, сверять с решениями; docs/third_party_notices.md — лицензии.

## Команды PowerShell из корня

IAR EWARM 9: C:\Program Files\IAR Systems\Embedded Workbench 9.2; инструменты в TEMP.

```powershell
$env:PYTHONPATH = "$env:TEMP\codex-deps"
$env:DOMOVOI_ZIG = "$env:TEMP\codex-zig\zig.exe"
$env:DOMOVOI_COMPARE_BUILD = "$env:TEMP\domovoi-agent-build"
& "$env:TEMP\codex-py311\python.exe" Tests/build_iar.py --config Release
& "$env:TEMP\codex-py311\python.exe" Tests/build_iar.py --config ReleaseCustomer
& "$env:TEMP\codex-py311\python.exe" Tests/build_iar.py --boot
& "$env:TEMP\codex-py311\python.exe" Tests/check.py --quick
& "$env:TEMP\codex-py311\python.exe" Tests/check.py --full
```

--quick: Release, связь, команды 12/17, запись ID, выборочные обрывы питания, ≤5 минут.
--full: обе сборки, загрузчик, все группы обоих профилей, CRC против C, 212 контрактов, мёртвый код; одна итоговая строка.
Логи/ELF/MAP/BIN/JSON — в DOMOVOI_COMPARE_BUILD; проверяется прямой запуск команд IAR JSONDB.
Не запускать две проверки одновременно с одной папкой сборок.

## Правила

- Один проект — гибрид C + ASM-ядра под замер; оставлять ядро при выигрыше ≥5% в реальной операции.
- Заголовок приложения 0x08008000 и векторы 0x08008200 не менять; загрузчик ≤32 КиБ.
- Не прошивать плату, не сбрасывать устройство, не управлять доступом и не трогать сервер без сценария.
- «Посмотри/проверь» — анализ; изменения и дополнительные агенты требуют поручения.
- Перед правками git status; сохранять чужую работу. Коммит — по условиям текущего задания.
- Исходники UTF-8 без BOM, CRLF; C и ASM IAR: --source_encoding utf8.
- Каждая строка доказывает нужность. Мёртвый код удалять; исключение в Tests/dead_code_allowlist.json требует конкретного основания.
