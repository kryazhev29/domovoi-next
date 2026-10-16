"""Инструменты проверок лежат в папке «Инструменты» выше проекта (мк\Инструменты), не в TEMP."""
import os
from pathlib import Path


# Ищет папку от проекта вверх: подходит и основной репозиторий, и копия в _worktrees.
def tools():
    start = Path(os.environ.get('DOMOVOI_ROOT') or Path(__file__).resolve().parents[1])
    for folder in (start, *start.parents):
        if (folder / 'Инструменты').is_dir(): return folder / 'Инструменты'
    raise FileNotFoundError('нет папки Инструменты выше ' + str(start))


# Явная переменная окружения важнее папки.
def zig():
    return os.environ.get('DOMOVOI_ZIG') or str(tools() / 'zig' / 'zig.exe')


def python_deps():
    return str(tools() / 'python-deps')
