"""Explicit IAR artifact selection; default paths retain existing test behavior."""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def image_paths(boot=False):
    folder = ROOT / ('Bootloader/build/Release' if boot else 'Release')
    stem = 'DomovoiBoot' if boot else 'DomovoiNext'
    prefix = 'DOMOVOI_BOOT_' if boot else 'DOMOVOI_'
    return tuple(Path(os.environ.get(prefix + kind, default)) for kind, default in (
        ('ELF', folder / 'Exe' / (stem + '.out')),
        ('BIN', folder / 'Exe' / (stem + '.bin')),
        ('MAP', folder / 'List' / (stem + '.map')),
    ))
