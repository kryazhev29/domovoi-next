"""Явный выбор собранного IAR-образа и его карты для тестов."""
import os
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def result_path(name):
    """Результаты новых прогонов хранятся рядом со сборками в TEMP."""
    folder = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir())/'domovoi-tests-build'))
    folder.mkdir(parents=True, exist_ok=True)
    return folder / name


def image_paths(boot=False):
    folder = ROOT / ('Bootloader/build/Release' if boot else 'Release')
    stem = 'DomovoiBoot' if boot else 'DomovoiNext'
    prefix = 'DOMOVOI_BOOT_' if boot else 'DOMOVOI_'
    return tuple(Path(os.environ.get(prefix + kind, default)) for kind, default in (
        ('ELF', folder / 'Exe' / (stem + '.out')),
        ('BIN', folder / 'Exe' / (stem + '.bin')),
        ('MAP', folder / 'List' / (stem + '.map')),
    ))
