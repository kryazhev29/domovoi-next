"""Уборка стендов: блокировка файла в Windows не меняет результат проверки."""
from pathlib import Path
import runpy
import sys
import tempfile
import time


class TemporaryDirectory(tempfile.TemporaryDirectory):
    @classmethod
    def _rmtree(cls, name, ignore_errors=False, repeated=False):
        for delay in (0, 0.1, 0.2, 0.4, 0.8, 1.6):
            if delay:
                time.sleep(delay)
            try:
                return super()._rmtree(name, ignore_errors=ignore_errors, repeated=repeated)
            except PermissionError as exc:
                error = exc
            except OSError as exc:
                error = exc
                break
        print(f'Уборка TEMP: оставлен каталог {name}: {error}', file=sys.stderr, flush=True)


def main():
    # Защищаем также стенды в asm, не меняя их исходники и проверки.
    sys.modules.setdefault('temp_cleanup', sys.modules[__name__])
    tempfile.TemporaryDirectory = TemporaryDirectory
    args = sys.argv[1:]
    if args[0] == '-m':
        sys.path.insert(0, str(Path.cwd()))
        sys.argv = args[1:]
        runpy.run_module(args[1], run_name='__main__', alter_sys=True)
    else:
        sys.path.insert(0, str(Path(args[0]).resolve().parent))
        sys.argv = args
        runpy.run_path(args[0], run_name='__main__')


if __name__ == '__main__':
    main()
