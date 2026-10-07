"""Сборка C/HYBRID из настроек проекта IAR в TEMP."""
import argparse
import os
from pathlib import Path
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build_iar import build
if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--variant', choices=('C', 'HYBRID'), required=True)
    p.add_argument('--configuration', choices=('Release', 'ReleaseCustomer'), default='Release')
    p.add_argument('--build-root', type=Path, default=Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir())/'domovoi-tests-build')))
    a = p.parse_args()
    build(a.variant, a.build_root, a.configuration)
