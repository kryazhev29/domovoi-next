# Модель clang для ELF-стендов

Собирает прошивку clang+lld под thumbv7m с теми же именами символов и DWARF, чтобы стенды
`Tests/asm` работали без IAR (облако, проверка правок до сборки IAR). Нужны clang 18, lld,
llvm-objcopy, unicorn, pyelftools.

```
python3 Tests/clang_model/build.py
python3 Tests/clang_model/run_cases.py            # все случаи CORE
python3 Tests/clang_model/run_cases.py firmware upload
```

Ограничения: размеры и стек по этой модели не мерить (clang без inline); мутационные случаи
и проверки машинного кода IAR (`id_sync_fast`, команда 111) на модели не проходят.
