# Один исполнитель флеш — проект этапа 3

07.10.2026; `domovoi-flash3`, HEAD `593f01d`. Прочитаны `AGENTS.md`, `PROJECT_STATUS.md`, `decisions.md` и [опись этапа 1](../../domovoi-flash/docs/flash_executor_plan.md), база `5fe9af1`. Только проект. E01–E09 исправляются отдельно; перед реализацией сверить их итог и тесты. Соседнее дерево только читалось.

| Решение | Граница |
|---|---|
| Один исполнитель | `job = { kind, step, result, union data }`; один `switch (kind)` в `flash_exec_poll`. `user_storage_poll` вызывает его и обслуживает результаты. |
| Без очереди/вытеснения | Нет token, mailboxes, приостановленных работ, квот и диапазонных блокировок. Между шагами принятой работы разрешены безопасные чтения. |
| Ожидающий запрос | Непринятый запрос остаётся у источника. Прежние save/reset, сроки ID, установка FW, pending сцены и RAM-журнал — предметные намерения, не активные job. |
| Состояние | Union содержит transient-данные или указатель на прежний WORK-контекст. Постоянные данные и удержанный TX-буфер остаются отдельно. |

## Интерфейс и жизнь работы

Эскиз `User/flash_exec.h`, 18 строк. Тип аргументов и union определяются во внутреннем заголовке; существующие типизированные `*_begin(...)` готовят аргументы, внешние обработчики не собирают union вручную.

```c
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t flash_kind_t;
enum { FX_NONE, FX_IDCLEAR, FX_EXPIRY, FX_FWSERVICE, FX_UPLOAD,
       FX_INDEX, FX_CRC, FX_TABLE, FX_PHOTO, FX_FWBLOCK, FX_LOG,
       FX_CONFIG, FX_STORE };
typedef uint8_t flash_result_t;
enum { FX_IDLE, FX_PENDING, FX_OK, FX_ERROR, FX_CANCELLED };
typedef struct { uint8_t kind, step, result; } flash_exec_state_t;
typedef struct flash_exec_args flash_exec_args_t;
bool flash_exec_begin(flash_kind_t kind, const flash_exec_args_t *args);
void flash_exec_poll(uint32_t now);
flash_exec_state_t flash_exec_state(void);
bool flash_exec_idle(void);
bool flash_exec_read_allowed(void);
flash_result_t flash_exec_take_result(void);
void flash_exec_cancel(void);
void flash_exec_quiet(uint8_t channel);
```

| Момент | Контракт |
|---|---|
| Приём | `idle = NONE/IDLE && !w25_async_active() && w25_read_available()`. `begin` дополнительно проверяет допустимость WORK/аргументов; копирует/удерживает их до публикации kind. `false` ничего не занимает, не означает IO_ERROR; запрос ждёт у источника. |
| Шаг | PENDING; прежний ограниченный квант. Storage-poll только из исполнителя; убрать вторые вызовы из `user_loop`/`photo_poll`. Ввод камеры и транспорт обслуживаются отдельно. |
| Ожидание | Верхний waiting → общий step WRITE/WRITE_WAIT. Фазы `w25_job`, `sjr`, CRC-pipeline записи ID сохраняются как вложенные механизмы. |
| A/B | CONFIG для самостоятельного сохранения; у STORE/FW-service/ACK — подшаг того же kind с полем продолжения. Второго begin и передачи владельца нет. |
| Память A/B | Как сейчас: FW reserve освобождается до config snapshot; резерв upload возвращается до ACK/A/B. Вложенность не разрешает перекрыть живые участки WORK. |
| Завершение | После verify — OK/ERROR/CANCELLED; обработчик kind публикует данные и готовит ответ. `take_result` снимает конечный результат один раз → NONE/IDLE; PENDING/IDLE не снимает. До снятия union не переиспользуется. |
| Ответ | Channel/uniq/epoch и готовый кадр живут в прежнем TX-контексте, вне очищаемого union. Kind свободен; borrowed-буфер удерживается до последнего USB IN. Допуск WORK проверяется даже при NONE; массива результатов нет. |
| Отмена | Quiet подавляет ответ сеанса. Незавершённую W25-команду/committed-журнал довести до безопасной границы. Источник до verify удерживается; прежний fault таблиц сохраняется. |

Выбор следующей работы — `pick_next()` только при NONE, без `*_active()`. Принятая команда устанавливает kind; непринятая ждёт у источника. Сохранить порядок `user_store.c:172–288`: CRC ответа → ACK журнала (STORE/A/B) → квант CRC-фона → index → expiry → upload → ID-clear → FW-service → table → photo → FW-block → log → config → store/recovery; в хвосте прежнее условие RAM-журнала относительно reset/save, затем reset, save. Принятая работа не вытесняется. CRC-фон/index заканчивают квант/проверенную физическую команду, сохраняя прогресс отдельно.

Существенная деталь: нынешний `idcrc_background()` всегда возвращает false, после его чтения диспетчер продолжает проход. Сохранить это: после завершённого CRC-кванта выбор продолжается ниже по списку **в том же проходе**, с прежними бюджетами; не начинать список заново. Один switch допускает последовательные завершённые работы, но одновременно только один kind. Иначе полный CRC-скан мог бы вытеснить все нижние намерения. Прежние условия запрета фона при save/reset/готовом RAM-журнале входят в pick_next.

## Двенадцать нынешних типов

В `flash_owner.h` **10 кодов**, ещё два типа — config и store без кода. CRC ответа и фоновый CRC делят FO_CRC; ACK журнала не тринадцатый владелец. Последний столбец — оценка удаляемых строк C, включая свои проверки/переходы владения; не размер всего автомата. Общие gates/диспетчер посчитаны отдельно ниже.

| Сейчас; файлы в `User/` | Kind; где данные | Что уходит / что остаётся | Строк уходит, оценка |
|---|---|---|---:|
| FO_IDCLEAR=0x81; `c/idclear_runtime.c` | IDCLEAR; union-указатель на WORK_IDCLEAR (40 Б) | Active/указатель-владелец, claim/release, phase/waiting → kind/step. Touched/cancel/quiet/epoch и ответ сохранить. | 6–10 |
| FO_EXPIRY=0x82; `c/expiry_runtime.c` | EXPIRY; указатель WORK_EXPIRY (132 Б) | Active/владелец, claim/release, phase. Сроки ID, snapshot и событие 138 после commit остаются. | 5–9 |
| FO_FWSERVICE=0x83; `c/fw_service_runtime.c` | FWSERVICE; указатель WORK_FW_CONTEXT (128 Б) + данные A/B | Active/busy, claim/release, phase/pending/config_result, мост `fw_service_config_result`. Резерв/отмена и запрет раннего request13 сохраняются; A/B не меняет kind. | 18–28 |
| FO_UPLOAD=0x84; `c/upload_runtime.c` | UPLOAD; указатель WORK_UPLOAD_CONTEXT (32 Б), прежний кадр 1040 Б | Active/владелец, claim/release вокруг finish, phase. Cursor/epoch/packet сохранить; ACK/очистка — последовательное продолжение STORE. | 8–14 |
| FO_INDEX=0x85; `c/idindex_runtime.c` | INDEX; transient source/bucket в union | Active, pending → step, claim/release, второй polling. Постоянные ix: сумма/готовность/mark/end/прогресс; fallback сохраняется. | 5–9 |
| FO_CRC=0x86; `c/service_runtime.c`, `id_crc_cache.c` | CRC, mode SERVICE/BACKGROUND; указатель WORK_SERVICE (248 Б) либо квант | Active/владелец, claim/release, phase/gates. Waiting сервиса — таймер чтения: сохранить его смысл. `ic` (12 Б), CRC/scan/partial остаются; CRC-дельта ID вложена в писателя, не FX_CRC. | 9–15 |
| FO_TABLE=0x87; `c/table_runtime.inc` | TABLE; `table_job` (64 Б) в union | Active, claim/release, phase, конечный failed → result. Промежуточные ошибки/records/CRC/recovery сохранить; table_fault и sjr вне union. | 7–12 |
| FO_PHOTO=0x08; `c/photo_runtime.c/.inc`, `photo_runtime.h` | PHOTO; transient-параметры/указатели в union | Busy/claim/release, phase/waiting. Постоянные pr: initialized/fault/очередь/resume/ACK; RS485/TX отдельно; after — нужное продолжение. | 12–20 |
| FO_FWBLOCK=0x09; `c/user_fw.c` | FWBLOCK; `fw_job` (40 Б) в union | Active/claim/release, phase/waiting, дубли busy. Блок/offset/CRC/epoch/числовой ответ сохранить; FWinProgress вне union. | 10–17 |
| FO_LOG=0x0A; `log_ring.c`, `c/user_log.c` | LOG; `log_job` (44 Б) в union | Active/claim/release, phase/waiting/result. Кольцо lr, RAM-журнал 16 событий, retry/repair остаются; init — подшаг LOG/UPLOAD/STORE. | 8–14 |
| Без FO; `c/memory_runtime.c` | CONFIG; cfg (36 Б) в union, вложенные варианты STORE/FWSERVICE | State/result/waiting, независимые result/reset. Snapshot 1024 Б, generation/cached_active и A/B-порядок сохранить. | 10–18 |
| Без FO; `c/user_store.c`, `log_cursor.c` | STORE, op SAVE/RESET/ACK/CLEAR/RECOVER; transient store + A/B в union | Store.job/waiting, ack.phase/poll, перехваты config-result. Last/seen/epoch, курсоры/маркеры и save/reset остаются; ACK cursor после OK. | 28–44 |

Удалить `flash_owner`, FO_READ_BLOCK/FO-коды, claim/release/read_allowed и `asm/flash_owner.h`. Побочный эффект `idcrc_owner_released()` (`idcrc_release_tail`) перенести на конец: проверять **постоянные ошибки** после публикации/снятия временного запрета; не инвалидировать исправный CRC из-за занятого kind. Kind — uint8_t, без битов блокировки и 32-битных enum-полей.

## Одна проверка чтения и все старые gates

`flash_exec_read_allowed()` — единственное разрешение внешнего чтения: нет `w25_async_active()` (включая verify), W25 доступна, таблицы восстановлены (`mem_storage_ready`, нет `table_fault/sj_runtime_blocked`), нет логического запрета kind/step и очистки temp-ID. Нового lock-байта нет. TABLE/IDCLEAR/EXPIRY и STORE/FACTORY закрывают таблицы до согласованной публикации: ни между erase/commit, ни между секторами пакета, ни до метаданных. CRC/индекс/фото/FW/журнал допускают чтение между физическими командами; индекс использует прежний fallback. Временный запрет оставляет сцену pending; настоящая неисправность сохраняет безопасное поведение.

| Удаляемый API; определения/объявления | Все нынешние потребители (строки этой базы) | Замена |
|---|---|---|
| `user_storage_command_ready`; `c/user_store.c:21`, `user_fun.h:148` | `store_command_allowed:26`, background-ready:36; `c/idclear_runtime.c:29`, `c/fw_service_runtime.c:59`, `c/photo_runtime.inc:143`; `c/user_main.c:107,227,248,254`; `USB-FS-CDC_User/Src/hw_config.c:122–123` | Приём пишущей работы: `flash_exec_idle()` + прежняя проверка WORK/канала/USB-source. Исключения команд 24/27 и cleanup 21 сохраняются в `store_command_allowed`. Команда 8 может читать через read_allowed, но при необходимости записи ждёт idle. Index выбирает исполнитель; FW/проба — работа либо оговорённая свободная точка ниже. |
| `user_storage_flash_ready`; `c/user_store.c:29`, `user_fun.h:152` | `c/service_runtime.c:43`, `log_ring.c:261`; `c/user_access.c:29`; `c/user_comm_runtime.c:74,76`; `c/user_main.c:105,107,185,281` | Чтения карты/звонка/SMS/сцены → read_allowed. Начало CRC сервиса → begin/idle. `log_addEvent` синхронно — только до runtime; в работе RAM → LOG. Index/expiry стартуют через pick_next. `user_main:281` (проверка сроков) оставляет намерение expiry, не polling владельца. |
| `user_storage_background_ready`; `c/user_store.c:34`, `user_fun.h:150` | `c/expiry_runtime.c:34`, `id_crc_cache.c:123`, `c/photo_runtime.inc:551` | Условие в `pick_next`: NONE, нет более приоритетного save/reset/готового RAM-журнала, рабочая память совместима. Проверка не размножается в модулях. |
| `flash_read_allowed`; `c/memory_runtime.c:357`, `c/runtime_internal.h:32` | `mem_tables_ready:32` | read_allowed; убрать зависимость от бита 7. Временный запрет и постоянные признаки повреждения не смешивать с успешным пустым чтением. |
| `mem_tables_ready`; `c/memory_runtime.c:32`, `memory.h:30` | `c/memory_runtime.c:40,233`; `c/expiry_runtime.c:34`, `c/fw_service_runtime.c:59`, `c/idclear_runtime.c:29`, `id_crc_cache.c:84,123`, `c/service_runtime.c:44`, `c/table_runtime.inc:86`, `c/upload_runtime.c:60`; `c/user_access.c:27,115`, `c/user_schedule.c:74`, `table_runtime.c:78` | Внешние читатели → read_allowed; admission работ → idle + прежние признаки исправности. Внутренний адресный `mem_guard` и boot-форматирование проверяют восстановление/ошибки напрямую, без запрета собственной операции. Внутренние шаги журналирования читают через прежний нижний API; глобальный read_allowed не должен запрещать им самим восстановление. |

Побочные действия старого command-ready (`store_usb_session`) перенести в начало poll и границу приёма, до обработки epoch; новая idle/read_allowed не должна скрыто отменять сеанс. USB/TCP admission и совместимость WORK остаются отдельными транспортными/ресурсными условиями, а не ещё одним разрешением чтения. При расширении допуска command 8 поправить оба входа: `store_command_allowed` вызывают `user_commands` и `user_rx`, USB дополнительно имеет общий gate в `hw_config`; нельзя обходить его для всех пишущих команд.

Чтение доступа целиком между шагами: ID → группа → расписание/праздник → решение; IRQ не обращается к флеш. Если нужна запись, удержать ввод, повторить чтение при idle/продолжить принятую работу; реле после OK, счётчик один раз. Результат WAIT не потребляет кадр в `server_rx_process`/USB, карту или звонок. Одной замены gate перед `WG_process` недостаточно: на промежуточном шаге пишущие ветки требуют idle, затем переводятся в работу.

## Синхронные записи: конкретные границы

| Путь описи этапа 1; нынешнее место | Решение без новых верхних kind |
|---|---|
| I12 temp-ID; `user_ids.c:166` | STORE/TEMP_APPEND: 32 Б, duplicate/free check, write/verify, cursor. IO_ERROR отличается от NOT_FOUND; исходный ввод ждёт при занятости. |
| I13 ID счётчика/first-pass; `user_ids.c:275` | TABLE/ID_PATCH: повторное чтение ID, прежний сектор-журнал/CRC; реле после OK, ответ/журнал один раз. |
| I21–I23 factoryReset/default/config; `user_config.c:54,134,278`, `table_runtime.c` | CONFIG или STORE/FACTORY: A/B/очистки подшагами, успех после конца. Стартовая миграция синхронная; полный factoryReset не атомарен. |
| I01 install; `Bootloader/app_update.c:54` | STORE/INSTALL: порционная проверка, erase журнала, magic последней. RebootToUpdate удерживается; reset после OK/flush. |
| I02/I03/I05 confirm/rollback/reported; `Bootloader/app_update.c:92,99,130` | Короткие синхронные свободные точки исполнителя: idle → занять STORE/BOOT_MARK → 4 Б/verify → результат/release, без вложенного poll. Trial/registration/report удерживают намерение без mailbox; rollback-reset после метки, reported после долговечного события. I04 чтения — read_allowed. |
| I06–I08 manual stage; `Bootloader/app_update.c:148,153,158` | Read — read_allowed/реальный результат; write/erase — STORE/MANUAL_STAGE с удержанным USB-вводом. Восемь erase асинхронно; сверить E07–E09. |
| I09 RAM replacement; `Bootloader/app_update.c`, `boot_manual.c` | Эксклюзивный STORE/RAM_APPLY: drain/verify, проверенный образ, память/транспорт, прежний RAM-writer/IRQ off. Алгоритм не менять. |
| I24–I30 `mem_*`, `sj_io_*`, CRC I/O; I39–I43 log/ACK/recovery | Только текущий kind/вложенный механизм. Sync_finish/log_wait/log_addEvent только до runtime; внутренние чтения без внешнего gate. |

Свободная точка — не spin-wait: если idle=false, возврат в главный цикл с сохранённым намерением. Одного проверки idle недостаточно: до синхронной записи занять текущий STORE, а после реального результата освободить его. Не использовать такую точку для секторного `mem_patch` доступа, длинного erase или целого сохранения A/B. Исправления E01–E09 не заменяются этими проектными рекомендациями и не считаются здесь выполненными.

## Семь небольших шагов реализации

**Каждый шаг — собираемая версия и `Tests/check.py --full` до следующего шага:** обе конфигурации, загрузчик, все группы, 212 контрактов каждого профиля, CRC, power-cuts и dead-code. Один отдельный `DOMOVOI_COMPARE_BUILD`; не запускать два прогона в нём одновременно. Сначала принять проверенный итог E01–E09 и переснять baseline; переходные адаптеры допустимы только с указанным удалением.

| № | Изменение | Что особенно проверяет полный прогон |
|---:|---|---|
| 1 | Header/job/switch/pick_next; все begin/finish через временные адаптеры. Чтения пока как сейчас. | Один kind/команда; второй begin без I/O; прежний порядок; результат один раз. |
| 2 | CRC/index/expiry/ID-clear; удалить их claim/active, второй polling index. | CRC oracle, fallback/фон, отмена/commit/138, бюджеты. |
| 3 | TABLE/PHOTO/FW-block/LOG; transient в union, PHOTO persistent отдельно. | Power-cuts/repair/CRC; источник до verify; камера/USB; публикация после OK. |
| 4 | CONFIG/STORE/FW-service/ACK: вложенный A/B; ответ отделить от kind. | A/B faults, cursor/epoch, 19 перед request13, кадр до последнего IN, WORK при NONE. |
| 5 | Read_allowed/gates; чистые чтения между шагами. Пишущие ветки временно требуют idle; WAIT не потребляет ввод. | Карта/звонок/8/сцены без записи; запрет verify/перезаписи; pending, удержание ввода при требуемой записи. |
| 6 | Runtime-записи → op/свободные точки; убрать обходные записи обработчиков. | Проба/install/reset BUSY и IO-fault, factoryReset, ID/temp-ID, manual/RAM, E01–E09. |
| 7 | Удалить адаптеры/FO/gates/active/result/runtime-sync. | Поиск callsites; оба BIN/MAP/CSTACK; dead-code. BIN/ОЗУ ≤ baseline. |

### Тесты, связанные со старым владельцем

Прямых обращений к байту `flash_owner` в Python-тестах — пять файлов; ещё один файл называет прежний мост в отчёте. Проверок именно констант FO_ в тестах нет: тесты записывают 0/1 или проверяют ноль. Заменять их работающим сценарием/состоянием исполнителя, а не другим произвольным числом в RAM.

| Файл/группа | Сейчас → новая проверка без ослабления |
|---|---|
| `asm/test_fw_install_wait.py:27,34,49,52` | Owner=1/0 → конфликтующая работа/завершение. Намерение удержано, нет ранних write/reset/133; после одна операция. |
| `test_comm_settings_elf.py:150,156` | Owner → реальный kind/несовместимый WORK. Кадр/config/epoch/таймауты сохраняются; read-only admission отдельно. |
| `asm/test_stage5_crc.py:91,94` | Блокировать warm настоящей работой: нет чтений/ложного CRC, после C-oracle; IO-fault отдельно. |
| `asm/test_id_sync_fast.py:100` | Owner=0 → NONE/IDLE, нет W25/утечки, прежние данные/CRC; конечный контракт ID_PATCH тот же. |
| `asm/test_stage5_log.py:126` | Drain → job/result завершены, save и RAM-журнал пусты; одного NONE недостаточно. |
| `asm/verification_fw_bridge.py:25–37`, `fw_bridge` | Active=false/busy=true при A/B → FWSERVICE/PENDING. Оставить A/B-fault, quiet/cancel, запрет раннего request13. |
| `asm/test_idindex_runtime.py:43–48`; модели storage/service/upload/clear/expiry/table/photo/FW | Owner() читает **ix.pending**, не FO. Наблюдать kind/step, обновить смещения; сохранить NOR/fallback, двигать настоящий poll. |
| `owners`, ready assertions; `quick_contracts`, sync-entry, stage4-USB, `comm_server_host` | Вместо запрета всего при active: второй писатель без I/O + read_allowed каждого step + запрет до commit/публикации. Старые asserts заменить доказательствами. |
| `owner_lifetimes`, `usb_replies`, `storage_boundary`, `workspace`, memory/USB-source | Курсоры/epoch/источник/кадр до IN неизменны; перекрывающий WORK отвергается. Kind свободен раньше TX, borrowed-буфер — после TX. |

Дополнить покрытие матрицей всех 12 kind × чтение/второй begin/отмена/физическое ожидание и границами вложенного A/B. BUSY и настоящий отказ SPI моделировать раздельно: обычный `StorageMachine.read` сам BUSY не моделирует. Новые сценарии включить в существующий `--full`; не сокращать старые fault/power-cut точки, группы и 212×2 протокольных контрактов.

## Размер и питание

База **этого HEAD** собрана прямыми командами IAR JSONDB через `Tests/build_iar.py`, последовательно Release/ReleaseCustomer. Артефакты: `%TEMP%\domovoi-flash3-design-baseline-593f01d`. Это проверка базовых сборок для оценки; проектируемая реализация и `--full` на этом этапе не выполнялись.

Заголовок `0x08008000`, векторы `0x08008200`, предел кода `BOOT_MAX_CODE=97 792 Б` и загрузчик ≤32 КиБ сохраняются.

| Метрика | До, измерено | После, оценка/критерий |
|---|---:|---|
| BIN Release / ReleaseCustomer | 78 403 / 75 960 Б | Целевой выигрыш 256–1024 Б: 77 379–78 147 / 74 936–75 704 Б. Прогноз, не результат компоновки. Обязательный предел при приёмке — ≤78 403 / ≤75 960 Б после интеграции переснять для её базы. |
| Readwrite data, включая прежний CSTACK | 16 862 / 16 476 Б | Ориентир −128…−224 Б: 16 638–16 734 / 16 252–16 348 Б. Финальное условие — не выше соответствующего baseline; проверить stack-canary. |
| Readwrite code; CSTACK limit | 516 Б обоих; `0x200043B8` / `0x20004230` | RAM-writer и 4 КиБ CSTACK сохраняются; не занимать стек новыми payload-копиями. Общий запас оценивать по MAP с readwrite code, а не только data. |
| Основные transient-объекты | table 64 + FW-block 40 + cfg 36 + log 44 = **184 Б** | Один union ориентировочно 64–76 Б + header/alignment 4 Б = **68–80 Б**. TABLE пока задаёт максимум; STORE с cfg/продолжением и новые op должны уложиться в тот же предел. Не складывать STORE и самостоятельный CONFIG как две работы. |
| Дополнительная экономия/сохраняемое | `ix` 24, store 32, ack 12 Б; 5 указателей контекстов по 4 Б; owner 1 Б | В union только transient-части ix/store/ack и указатели; постоянные части остаются. `pr` 96, `ic` 12, `sjr` 128 и WORK 2224 Б не считать целиком освобождёнными. WORK-контексты 248/132/128/40/32 Б и большие кадры остаются в прежней памяти; не переносить их целиком в постоянный union. |
| Строки участка перепроектирования | **3065 строк** в 14 модулях: 12 строк таблицы владельцев (CRC — два файла, STORE — два); PHOTO здесь `.inc`, LOG — `log_ring.c` | Удалить ≈126–210 строк своих переходов и ≈60–85 общих gates/диспетчера; добавить ≈85–125 строк исполнителя/адаптации. Участок вместе с новым исполнителем ≈2855–3004 строки, чистое −61…−210. Заголовки/внешние callsites вне этого счёта; диапазоны инженерные. |

| Риск питания/целостности | Неизменный барьер и проверка |
|---|---|
| Сектор-журнал | Формат/DATA/COMMIT/копирование/recovery/sjr не менять. После COMMIT отмена ждёт безопасного конца; прежние power-cuts + смена kind/step. |
| A/B | Generation/CRC/header-last, выбор слота/cached_active после verify сохраняются; вложенность не меняет байты/публикацию. |
| W25 | WREN/таймауты/readback/FF verify и драйвер не менять. Read_allowed=false до конца active; источник/sjr.data до verify не переиспользовать. |
| Курсоры/очистки/фото/FW | Маркеры/публикация/retry/repair/CRC прежние; подтверждение после проверки. Reset ждёт запись/rollback marker. |
| RAM/factoryReset | RAM-запрос до долговечной записи теряется при питании. Полный factoryReset не атомарен; новых форматов не вводить. |
| Без вытеснения | Длинная работа задерживает следующую запись, TX — новый payload; RAM-журнал конечный. Erase/verify/перезапись таблиц заставляют ждать; новых гарантий задержки нет. |

**Итог проекта:** участок 3065 → ≈2855–3004 строки; BIN 78 403/75 960 → целевые ≈77 379–78 147/74 936–75 704 Б; readwrite data 16 862/16 476 → ≈16 638–16 734/16 252–16 348 Б; **7 шагов**, каждый с `--full`. Размеры «после» — оценки; платы/сервера и коммита нет, проверка платы остаётся отдельным сценарием.
