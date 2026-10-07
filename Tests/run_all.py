"""Доработка: Кряжев А., 2026. Полный прогон проверок без платы: python Tests/run_all.py [--fast].
--fast - без мутаций (они пересобирают модель десятки раз)."""
import subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PY = sys.executable
GROUPS = ('access', 'bootjournal', 'cfgstrings', 'cmdlengths', 'commands', 'commat', 'commserver', 'commslm', 'commuart', 'countends', 'crc', 'crcblocks', 'crcff', 'flashcmds', 'fwblockbounds', 'fwstart', 'idexpiry', 'idindex', 'idsync', 'idtext', 'log', 'logcommit', 'logupload', 'outs', 'persistence', 'photo', 'recovery', 'rs485cfg', 'rules', 'rx', 'sectorjournal', 'serverrx', 'sms', 'storage', 'sync', 'tempidmeta', 'timersirq', 'uarttimer', 'uploadretry', 'usbdesc', 'usbresetwait', 'usbresume', 'usbsetupbounds', 'utcday', 'w25', 'wiegandirq')
TX = ('idle', 'reset', 'esof', 'busy', 'large')


def main():
    arguments=sys.argv[1:]
    if '--project' not in arguments: arguments=['--project','HYBRID',*arguments]
    sys.exit(subprocess.call([PY,str(ROOT/'Tests/asm/c_vs_asm_project.py'),*arguments],cwd=ROOT))


if __name__ == "__main__":
    main()
