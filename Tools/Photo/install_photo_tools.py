# Доработка: Кряжев А., 2026
"""Копирование инструментов в Стенд_Domovoi. Не открывает COM/TCP и не меняет fw_server.py."""
import argparse
import hashlib
from pathlib import Path
import shutil

HERE = Path(__file__).resolve().parent

def install(stand):
    target = Path(stand) / 'Скрипты'
    if not target.is_dir():
        raise ValueError('Не найден каталог Стенд_Domovoi/Скрипты')
    old = target / 'fw_server.py'
    digest = hashlib.sha256(old.read_bytes()).hexdigest()
    for name in ['photo_server_gui.py', 'camera_emulator.py', 'sample_640x480.jpg', 'rs485_config.py', 'periphery.ini']:
        shutil.copyfile(HERE/name, target/name)
    manual = target / 'КАК_ПОЛЬЗОВАТЬСЯ.txt'
    content = manual.read_text(encoding='utf-8-sig')
    addition = (HERE / 'КАК_ПОЛЬЗОВАТЬСЯ.txt').read_text(encoding='utf-8-sig')
    start = addition.splitlines()[0]
    end = 'КОНЕЦ РАЗДЕЛА ФОТО C328'
    if start in content:
        before, tail = content.split(start,1)
        if end not in tail:
            raise ValueError('Незавершённый раздел фото: инструкция не переписана')
        content = before + addition.rstrip() + tail.split(end,1)[1]
    else:
        content = content.rstrip() + '\n\n------------------------------------------------------------\n' + addition
    manual.write_bytes(content.replace('\r\n','\n').replace('\n','\r\n').encode('utf-8'))
    assert hashlib.sha256(old.read_bytes()).hexdigest() == digest
    print(f'Инструменты: {target}; fw_server.py не изменён, SHA256={digest}')

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stand',type=Path,default=HERE.parents[5]/'Стенд_Domovoi')
    install(parser.parse_args().stand)
