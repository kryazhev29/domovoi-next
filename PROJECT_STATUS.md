# ДОМОВОЙ NEXT — 07.10.2026

- 14.10.2026: USB-журнал 108/109 для монтажника в ReleaseCustomer/Release; протокол [usb_log.md](docs/protocol/usb_log.md). Отдельная PMA-передача без памяти ответа, потери видны; NextConfig реализован в соседнем worktree. Локальные проверки и память — в `../CODEX_REPORT.md`; плата не проверялась.

- База задания: `16433b89d1a3ae7c558b06af46543e7fb9df1b0e`, worktree `domovoi-agent`; без коммитов, платы и сервера.
- Решения: [docs/decisions.md](docs/decisions.md). ПИН 4–5 цифр и универсальный ПИН из IMEI реализованы: [отчёт](docs/history/PIN_20261007.md); тяжёлый тест на плате отложен.
- Один `DomovoiNext.ewp`, Release / ReleaseCustomer; C + измеренное ASM-ядро CRC16/CRC32; три ядра переведены на C.
- Исходники UTF-8 без BOM, CRLF; IAR C/ASM: `--source_encoding utf8`. Адреса и границы загрузчика сохранены.
- Команды в [AGENTS.md](AGENTS.md); артефакты `%TEMP%/domovoi-agent-build`.
- Итог и ограничения: [отчёт](docs/history/AGENT_FRIENDLY_20261007.md).
- Прежние этапы — `docs/history/`; они не описывают текущий состав проекта.
