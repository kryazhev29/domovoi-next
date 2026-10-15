ДОМОВОЙ NEXT — самостоятельный проект IAR
Дата экспорта: 15.10.2026
Коммит: 49b4d1d2699fc95b7bb1c31ce626e5878c14617b
Это коммит, прошитый на плату №1. Исправления объявлений 8b6ec2f в этот архив не входят.
compare/c-asm остаётся на 5d0cf6e; ветки не перемещались. Источник выбран по журналу прошивки платы.

Открыть DomovoiNext.eww в IAR: приложение DomovoiNext и загрузчик DomovoiBoot.
Проверено: IarBuild 9.3.2.391; ICCARM 9.50.1.380/W64.
Установленная среда: C:\Program Files\IAR Systems\Embedded Workbench 9.2
Требуется доступная лицензия компилятора. Стандартная библиотека DLIB берётся из установки IAR.

Из этой папки, PowerShell:
$iar = 'C:\Program Files\IAR Systems\Embedded Workbench 9.2\common\bin\IarBuild.exe'
& $iar DomovoiNext.ewp -build ReleaseCustomer
& $iar DomovoiNext.ewp -build Release
& $iar Bootloader\DomovoiBoot.ewp -build Release

Результаты приложения: build\ASM\<конфигурация>\Exe и List.
Результаты загрузчика: Bootloader\build\Release\Exe и List.
Проверенные BIN/MAP и SHA-256: Binaries. Файл сервера: Binaries\Server\139.71.1792067616.bin.
Он содержит ReleaseCustomer BIN и два завершающих FF для кратности четырём; это не пакет смены загрузчика.
Здесь ничего не отправлялось на сервер и не прошивалось.

Исходники выгружены git archive. Tests, AGENTS.md, .git, worktree, эмуляторы и evidence не включены.
Единственная адаптация .ewp: Tests\iar_stack.suc перенесён в IAR\iar_stack.suc;
содержимое .suc и параметры компиляции/линковки сохранены.
IarBuild -build дал побайтно те же три BIN, что Tests/build_iar.py того же коммита.
Полная таблица настроек каждого модуля: Optimizations.csv.
High/Balanced для user_commands.c намеренный: бюджет инструкций ответа 17 (решение 9fa129e).
Загрузчик сохранён с исходной оптимизацией None (-On); его настройки не унифицировались с приложением.
Прототипы старого снимка вызывают предупреждения declared implicitly; исправления вынесены в отдельный 8b6ec2f.
Лицензии компонентов: THIRD_PARTY_NOTICES.txt.
