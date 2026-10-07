# ДОМОВОЙ NEXT — 07.10.2026

- База задания: `16433b89d1a3ae7c558b06af46543e7fb9df1b0e`, worktree `domovoi-agent`; без коммитов, платы и сервера.
- Решения: [docs/decisions.md](docs/decisions.md). ПИН 4–5 цифр в работе; тяжёлый тест на плате отложен.
- Один `DomovoiNext.ewp`, Release / ReleaseCustomer; C + измеренное ASM-ядро CRC16/CRC32; три ядра переведены на C.
- Исходники UTF-8 без BOM, CRLF; IAR C/ASM: `--source_encoding utf8`. Адреса и границы загрузчика сохранены.
- Команды в [AGENTS.md](AGENTS.md); артефакты `%TEMP%/domovoi-agent-build`.
- Итог и ограничения: [отчёт](docs/history/AGENT_FRIENDLY_20261007.md).
- Прежние этапы — `docs/history/`; они не описывают текущий состав проекта.
