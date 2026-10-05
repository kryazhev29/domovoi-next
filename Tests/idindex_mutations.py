"""Доработка: Кряжев А., 2026. Мутации индекса базы ID: тест idindex обязан ловить каждую."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Tests/asm/reference/User/idindex.c'
CASES = (
    ('no-invalid-mark', b'if (ix.state == IX_READY && !ix_mark(false, 0))', b'if (0 && !ix_mark(false, 0))'),
    ('fill-put-skip', b'!(ix.state == IX_FILL && index < ix.step)', b'!(ix.state == IX_FILL && index > ix.step)'),
    ('boot-trust-any', b'ix_last(&marked) && marked == sum', b'ix_last(&marked)'),
    ('commit-no-mark', b'if (ix.state == IX_READY && !ix_mark(true, ix.sum)) ix.state = IX_OFF;', b''),
    ('bucket-stop-early', b'i < IX_SLOTS && e[i] != 0xFFFFu', b'i < 1 && e[i] != 0xFFFFu'),
)


def main():
    original = SOURCE.read_bytes()
    missed = []
    try:
        for name, old, new in CASES:
            assert original.count(old) == 1, name
            SOURCE.write_bytes(original.replace(old, new))
            run = subprocess.run([sys.executable, str(ROOT / 'Tests/run_host.py'), 'work', 'idindex'], capture_output=True)
            print(name, 'detected' if run.returncode else 'MISSED')
            if not run.returncode:
                missed.append(name)
    finally:
        SOURCE.write_bytes(original)
    print('idindex mutations:', len(CASES) - len(missed), '/', len(CASES), 'detected')
    sys.exit(1 if missed else 0)


if __name__ == '__main__':
    main()
