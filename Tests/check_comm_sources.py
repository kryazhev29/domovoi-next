"""Доработка: Кряжев А., 2026. Оформление затронутых исходников связи."""
from pathlib import Path
import argparse

ROOT=Path(__file__).resolve().parents[1]
C_FILES=(
    'stm32_it.c','Modules/comm_uart.c','Modules/comm_uart.h','Modules/comm_power_v5.c','Modules/comm_power_v5.h',
    'Modules/comm_modem.c','Modules/comm_modem.h','Tests/test_comm_uart.c','Tests/test_comm_at.c',
    'Modules/comm_at.c','Modules/comm_at.h','Modules/modem_slm336.c','Modules/modem_slm336.h',
    'Modules/comm_channel.c','Modules/comm_channel.h','Modules/comm_server.c','Modules/comm_server.h','Modules/comm_recover.c','Modules/comm_recover.h','Modules/comm_info.c','Modules/comm_info.h','Modules/comm_call.c','Modules/comm_call.h','Tests/test_comm_recover.c','Tests/test_comm_call.c',
    'User/comm_timeouts.c','User/comm_timeouts.h','User/user_comm.c','User/user_comm.h',
    'User/user_fun.c','User/photo.c','Tests/test_comm_server.c','Tests/test_comm_channel.c','Tests/comm_photo_slot.c','Tests/comm_upload_slot.c',
)
PY_FILES=('Tests/run_host.py','Tests/comm_server_host.py','Tests/comm_slm_host.py',
          'Tests/build_comm_uart.py','Tests/comm_uart_budget.py','Tests/comm_budget.py','Tests/comm_mutations.py','Tests/test_photo_mutations.py','Tests/check_comm_sources.py','Tests/recover_mutations.py')

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
    for name in ('Modules/comm_uart.c','Modules/comm_at.c','Modules/comm_modem.c','Modules/modem_slm336.c',
                 'Modules/comm_channel.c','Modules/comm_server.c','Modules/comm_recover.c','Modules/comm_call.c','User/user_comm.c'):
        text=(ROOT/name).read_bytes().decode('cp1251')
        assert not any(word in text for word in ('malloc(', 'calloc(', 'realloc(', 'snprintf(')), name
        # Этап 5: связь никогда не перезагружает процессор.
        assert not any(word in text for word in ('NVIC_SystemReset', 'user_savereset', 'app_trial_rollback')), name
    print('comm sources: cp1251 C, UTF-8 Python, CRLF, author headers, new modules without heap and without CPU reset PASS')

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--fix',action='store_true'); check(p.parse_args().fix)
