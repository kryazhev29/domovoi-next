# Слияние compare/c-asm в этап 3 task/exec — 12.10.2026

Рабочее дерево: `C:\Users\kryaz\Desktop\мк\_worktrees\domovoi-exec`.
Первый родитель: `d21118d6b13329b01086cedab009fd805332587c` (`task/exec`).
Второй родитель: `53110634c9773551fafb87507fa9e4b8c665eca7` (`compare/c-asm`).
Обычное слияние с двумя родителями, без rebase. Исходное дерево было чистым.
Плата не прошивалась, сервер не использовался; `compare/c-asm` не перемещалась.

## Разрешение всех 12 конфликтов

Строки относятся к итоговым файлам этого слияния.

| № | Файл:строка | Принятая логика |
|---|---|---|
| 1 | `User/c/user_comm_runtime.c:41` | Комментарий и NULL-контракт из compare: `readyToUpdate` возвращает false раньше проверки NULL; NULL вне FW возвращает true без чтения флеш. |
| 2 | `User/c/user_comm_runtime.c:48` | Проверка `user_access_ready(last10)` из exec перед `user_incomingcall`; false означает повтор позже. |
| 3 | `User/c/user_ids.c:143` | Из exec: добавление temp-ID допускается только при `flash_exec_idle()`. |
| 4 | `User/c/user_ids.c:152` | Из exec: результат поиска 0 — нет, 1 — найден, 2 — ошибка чтения. Ошибка не считается отсутствием. |
| 5 | `User/c/user_ids.c:169` | Из exec: `flash_exec_begin(FX_STORE, FS_SYNC)` → `mem_write` → `flash_exec_finish` → `flash_exec_take_result`; курсор и readyID меняются только после успеха. Остальной код согласован с compare; итоговый user_ids.c совпадает с исходным exec. |
| 6 | `User/c/user_sms.c:55` | Разбор #add из compare: ровно 10 цифр, иначе format_error. Вызов `user_addIdToTempBuffer` использует реализацию через исполнитель из exec. При отказе — return true из compare, без ответа об успехе и события добавления. |
| 7 | `Tests/comm_server_host.py:64` | Объединение: модель `flash_exec_quiet` из exec плюс USB 112 v3 (`test_link_diag`) и настоящий `user_comm_call` из compare. |
| 8 | `Tests/comm_server_host.py:76` | Объединение ARM-заглушек: device.status.readyToUpdate из compare, flash_exec_read_allowed из exec; добавлены объявления user_access_ready, user_last10digitsnumber и strcpy для объединённого callback. |
| 9 | `Tests/test_comm_call.c:7` | Обработчик модели user_incomingcall из compare вместо прямого call_caller из exec; настоящий callback приложения подключается стендом. Сохранены старые сценарии, добавлены независимые от read-gate повторы занятого исполнителя. |
| 10 | `Tests/test_comm_server.c:53` | Прототип и инициализация user_comm_call из compare; проверки освобождения исполнителя из exec сохранены. |
| 11 | `Tests/test_sms.c:60` | Проверки add_ok и общего user_activateOut из compare; store_factory_request из exec вместо синхронного factory reset. Немедленного ответа DEFAULT нет: завершение/ошибка проверяются сохранёнными ELF-тестами исполнителя. |
| 12 | `Tests/test_tempid_meta.c:44` | Ошибка user_findTempID равна 2 из exec; сохранена проверка false от следующего добавления из compare. Сохранены контроль записей, курсора, readyID, флагов и резерва. |

## Автоматическое слияние и связанные проверки

- `Modules/comm_call.c:144`: запрет caller(NULL) сбрасывает pending до проверки номера. `:152`: caller(number)==false оставляет pending=1. NULL никогда не проверяет занятость флеш.
- `User/c/user_comm_runtime.c:78`: comm_call_poll и свежий допуск comm_sms_poll используют flash_exec_read_allowed из exec.
- Автомат CHUP/CLCC целиком из compare. Относительно compare в comm_call.c отличается только условное завершение caller(number). ATA → около 1 с → CHUP, подтверждение пустым CLCC, ограничение повторов и ограждение поздних ответов сохранены.
- `Modules/comm_channel.c:46`: обновление срока TCP ACK после голоса сохранено; весь файл совпадает с compare.
- `User/c/user_commands.c:215`: команда 8 при readyToUpdate отвечает CMD_RUNERROR. Отложенное завершение записи ID остаётся из exec.
- D02/D09/D13/D14/D21/D31/D33 и USB 112 v3 включены вместе с их тестами. user_diag.c, user_protocol.c, packet.c и outs.c совпадают с compare. Правки доступа из compare объединены с отложенной записью и публикацией реле из exec.
- `Tests/asm/test_fixes1_mutations.py:44`: мутация ложного успеха записи D14 перенесена со старого прямого mem_write на `if (!ok) return false` после finish/take_result. Проверка сохранена и обнаруживает ложный успех.

**Различие SMS при отказе записи:** compare возвращает true (SMS обработана) без ответной SMS и без события 67. Exec возвращал false (оставить SMS для повтора). По заданию сохранён вариант compare. Поэтому занятость исполнителя, отсутствие места и ошибка записи здесь завершают обработку без повторной попытки; это сознательный выбор семантики compare, а не обещание успешного добавления.

## Новые и усиленные тесты

- `Tests/test_comm_call.c:103`: чтение разрешено, но user_access_ready=false. Проверяются несколько попыток, pending=1, отсутствие ATA/CHUP/доступа в течение 2 с; после освобождения — ровно один доступ, ATA и CHUP.
- `Tests/test_comm_call.c:112`: бит 11 появляется во время повторов caller(number). Pending сбрасывается, число проверок доступа больше не растёт, выполняется CHUP/CLCC без ATA; снятие бита не оживляет отклонённый вызов.
- `Tests/test_comm_call.c:85`: сохранённая матрица FW × занятость чтения × наличие CLIP дополнена проверкой отсутствия pending и обращений к доступу, а также CLCC.
- `Tests/test_comm_call.c:149`: прежнее ожидание занятого чтения дополнено явным запретом CHUP и проверкой pending=1.
- Сохранены проверки активного вызова после ложного OK CHUP, ошибок/тишины CLCC, повторных RING, TCP ACK, SMS-форматов/отказов и ошибок temp-ID обеих веток. Ожидания ATH заменены на CHUP согласно compare; ошибка чтения temp-ID проверяется как 2 согласно exec.

## Проверки и размеры

Целевые host-тесты `commserver`, `sms`, `tempidmeta`: PASS.
Целевые Release ELF `call FW`, `unblock`, `fixes1 mutations`: PASS.
`Tests/check.py --full` запущен **один раз**: **PASS FULL, 244,28 с**.
Release — **122/122**, ReleaseCustomer — **122/122**. Загрузчик собран.
`dead_code_regression` и `dead_code` — PASS; 0 мёртвых функций / 0 данных.
`git diff --check` — PASS; изменённые исходники UTF-8 без BOM, CRLF.
В ReleaseCustomer штатно пропущены DIAG-команды 111/113 и внутренние снимки 111; USB 112 v3 проверена.
C/H/ASM/Python/EWP-файлы итогового дерева побайтно совпали с копиями, использованными полным прогоном обоих профилей.

SHA-256 итоговых ELF:
- Release: `c09467387fea4f368303199fcdc5c09bca0f995cfc100d715ae83e5e6120f068`.
- ReleaseCustomer: `9b2c3ccac69b7aeeb179e8bf1e943ad34504c63d495a894ac59d6a07a24ebb2d`.

| BIN, байты | Release | ReleaseCustomer |
|---|---:|---:|
| compare/c-asm 5311063 | 80 711 | 78 752 |
| task/exec d21118d до слияния | 83 823 | 81 372 |
| После слияния | 84 527 | 82 564 |
| Изменение к task/exec | +704 | +1 192 |
| Изменение к compare/c-asm | +3 816 | +3 812 |

Исходный exec собран заново до слияния для обоих профилей. Размеры compare сверены с сохранёнными BIN `domovoi-slm336-hangup-20261012/call-full` и отчётом SLM336_HANGUP_20261012.md; compare заново не собиралась. Итоговые размеры сняты с BIN полного прогона. Резерв заголовка 512 Б в BIN не включён.

Артефакты: `C:\Users\kryaz\AppData\Local\Temp\domovoi-exec-merge-20261012`:
`before/` — две исходные сборки; `targeted/` — целевые логи/ELF; `full/` — полный прогон, check-summary.json, run-all-*.json, MAP/ELF/BIN и логи.
После проверок удалены временные тестовые деревья full/runs, мутационные сборки, ObjPath и Python-кэши. Основные логи, результаты и сравниваемые BIN/ELF/MAP сохранены.

## Сомнения и ограничения

Не осталось известных неразрешённых смысловых конфликтов. Проверки выполнены на host и связанном IAR ELF/модели NOR; аппаратная работоспособность объединённой прошивки в этой задаче не проверялась. Проверка compare на плате 12.10 указана владельцем и не подменяет проверку слияния. Для платы остаётся особенно важным сочетание реальной занятости NOR, звонка и загрузки FW. Прежние ограничения этапа 3 (в том числе неатомарность полного factory reset при потере питания) этим слиянием не менялись.
