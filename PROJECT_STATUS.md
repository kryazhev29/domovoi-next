# ДОМОВОЙ NEXT — 07.10.2026

- 14.10.2026: исправление первой попытки fast-start — готовность AT/SIM, ограниченные повторы и ранний USB. Изменено, не проверено на плате; [отчёт](docs/history/FAST_START_FIX_20261014.md).

- 14.10.2026: USB-журнал 108/109 для монтажника в ReleaseCustomer/Release; протокол [usb_log.md](docs/protocol/usb_log.md). Очередь журнала на 4 кадра без памяти ответа, потери видны; NextConfig — ветка task/usblog. Плата №1: потерь 0, опрос статуса и чтение настроек работают.

- База задания: `16433b89d1a3ae7c558b06af46543e7fb9df1b0e`, worktree `domovoi-agent`; без коммитов, платы и сервера.
- Решения: [docs/decisions.md](docs/decisions.md). ПИН 4–5 цифр и универсальный ПИН из IMEI реализованы: [отчёт](docs/history/PIN_20261007.md); тяжёлый тест на плате отложен.
- Один `DomovoiNext.ewp`, Release / ReleaseCustomer; C + измеренное ASM-ядро CRC16/CRC32; три ядра переведены на C.
- Исходники UTF-8 без BOM, CRLF; IAR C/ASM: `--source_encoding utf8`. Адреса и границы загрузчика сохранены.
- Команды в [AGENTS.md](AGENTS.md); артефакты `%TEMP%/domovoi-agent-build`.
- Итог и ограничения: [отчёт](docs/history/AGENT_FRIENDLY_20261007.md).
- Прежние этапы — `docs/history/`; они не описывают текущий состав проекта.

- 15.10.2026: унифицированы объявления ID и подключены прототипы шести неявных вызовов; обычные IAR-сборки без declared implicitly, ΔBIN/ΔRAM 0/0. High/Balanced для user_commands сохранён намеренно (бюджет ответа 17). Эта сборка на плате не проверена.
