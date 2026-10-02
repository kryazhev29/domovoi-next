"""Доработка: Кряжев А., 2026. Оформление затронутых исходников связи."""
from pathlib import Path
import argparse

ROOT=Path(__file__).resolve().parents[1]
C_FILES=(
    'Modules/comm_at.c','Modules/comm_at.h','Modules/modem_slm336.c','Modules/modem_slm336.h',
    'Modules/comm_channel.c','Modules/comm_channel.h','Modules/comm_server.c','Modules/comm_server.h',
    'User/comm_timeouts.c','User/comm_timeouts.h','User/user_comm.c','User/user_comm.h',
    'User/user_fun.c','User/photo.c','Tests/test_comm_server.c','Tests/comm_photo_slot.c','Tests/comm_upload_slot.c',
)
PY_FILES=('Tests/run_host.py','Tests/comm_server_host.py','Tests/check_comm_sources.py')

def check(fix=False):
    for name in (*C_FILES,*PY_FILES):
        p=ROOT/name; data=p.read_bytes()
        if fix:
            try: text=data.decode('utf-8')
            except UnicodeDecodeError: text=data.decode('cp1251')
            data=text.replace('\r\n','\n').replace('\n','\r\n').encode('cp1251' if name in C_FILES else 'utf-8')
            p.write_bytes(data)
        assert b'\n' not in data.replace(b'\r\n',b''), name
        text=data.decode('cp1251' if name in C_FILES else 'utf-8')
        assert 'Доработка: Кряжев А., 2026' in text[:350], name
    for name in ('Modules/comm_channel.c','Modules/comm_server.c','User/user_comm.c'):
        text=(ROOT/name).read_bytes().decode('cp1251')
        assert not any(word in text for word in ('malloc(', 'calloc(', 'realloc(')), name
    print('comm sources: cp1251 C, UTF-8 Python, CRLF, author headers, new modules without heap PASS')

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--fix',action='store_true'); check(p.parse_args().fix)
