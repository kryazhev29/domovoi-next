"""Доработка: Кряжев А., 2026. Оформление затронутых исходников связи."""
from pathlib import Path
import re
import argparse
import io
import tokenize

ROOT=Path(__file__).resolve().parents[1]
C_FILES=(
    'User/user_fun.h','User/user_str_fun.c','User/user_str_fun.h','User/user_format.c','User/user_format.h',
    'Bootloader/app_update.c','Bootloader/app_update.h',
    'stm32_it.c','Modules/comm_uart.c','Modules/comm_uart.h','Modules/comm_power_v5.c','Modules/comm_power_v5.h',
    'Modules/comm_modem.c','Modules/comm_modem.h','Tests/test_comm_uart.c','Tests/test_comm_at.c',
    'Modules/comm_at.c','Modules/comm_at.h','Modules/modem_slm336.c','Modules/modem_slm336.h',
    'Modules/comm_channel.c','Modules/comm_channel.h','Modules/comm_server.c','Modules/comm_server.h','Modules/comm_recover.c','Modules/comm_recover.h','Modules/comm_info.c','Modules/comm_info.h','Modules/comm_call.c','Modules/comm_call.h','Modules/comm_sms.c','Modules/comm_sms.h','Tests/test_comm_sms.c','Modules/comm_balance.c','Modules/comm_balance.h','Tests/test_comm_balance.c','Tests/test_comm_volte.c','Tests/test_comm_recover.c','Tests/test_comm_call.c',
    'User/comm_timeouts.c','User/comm_timeouts.h','Tests/asm/reference/User/user_comm.c','User/user_comm.h',
    'Tests/asm/reference/User/user_fun.c','Tests/asm/reference/User/photo.c','Tests/test_comm_server.c','Tests/test_comm_channel.c','Tests/comm_photo_slot.c','Tests/comm_upload_slot.c',
)
C_FILES += ('Tests/test_access.c', 'Tests/test_count_ends.c', 'Tests/test_log_upload.c', 'Tests/test_sms.c', 'Tests/test_tempid_meta.c', 'Tests/test_update_rules.c', 'Tests/test_upload_retry.c', 'Tests/event_async_model.inc', 'Tests/fw_async_model.inc', 'Tests/store_async_cases.inc', 'Tests/store_callers_cases.inc')
C_FILES += tuple(str(p.relative_to(ROOT)).replace('\\','/') for p in sorted((ROOT/'User/c').iterdir()) if p.suffix in ('.c','.h','.inc'))
C_FILES += tuple(str(p.relative_to(ROOT)).replace('\\','/') for p in sorted((ROOT/'Tests').glob('test_comm_*.c')) if str(p.relative_to(ROOT)).replace('\\','/') not in C_FILES)
PY_FILES=('Tests/comm_review_host.py','Tests/comm_at_host.py','Tests/test_comm_settings_elf.py','Tests/test_app_update_elf.py','Tests/test_logic_cleanup.py','Tests/run_host.py','Tests/comm_server_host.py','Tests/comm_slm_host.py',
          'Tests/build_comm_uart.py','Tests/comm_uart_budget.py','Tests/comm_budget.py','Tests/comm_mutations.py','Tests/call_mutations.py','Tests/test_photo_mutations.py','Tests/check_comm_sources.py','Tests/recover_mutations.py')

PY_FILES += ('Tests/test_event_async.py', 'Tests/test_fw_async.py', 'Tests/test_store_async.py', 'Tests/test_store_callers.py', 'Tests/event_async_mutations.py', 'Tests/fw_async_mutations.py', 'Tests/rx_budget_mutations.py', 'Tests/store_async_mutations.py', 'Tests/test_iar_event_async.py', 'Tests/test_iar_fw_async.py', 'Tests/asm/test_access_reads.py', 'Tests/test_user_format.py', 'Tests/asm/c_vs_asm_project.py')

# Выделить только комментарии C, не принимая содержимое строк за исходный текст.
def comments(text):
    token = r""""(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|//[^\r\n]*|/\*[\s\S]*?\*/"""
    return (m.group() for m in re.finditer(token, text) if m.group().startswith(('//', '/*')))


# Проверить кодировку, оформление и договор внешних границ модулей.
def check(fix=False):
    for name in (*C_FILES,*PY_FILES):
        p=ROOT/name; data=p.read_bytes()
        if fix:
            try: text=data.decode('utf-8')
            except UnicodeDecodeError: text=data.decode('cp1251')
            data=text.replace('\r\n','\n').replace('\n','\r\n').encode('cp1251' if name in C_FILES else 'utf-8')
            p.write_bytes(data)
        assert b'\n' not in data.replace(b'\r\n',b'') and b'\r' not in data.replace(b'\r\n',b''), name
        text=data.decode('cp1251' if name in C_FILES else 'utf-8')
        assert 'Доработка: Кряжев А., 2026' in text[:350], name
        owned = (name.startswith('User/c/user_') or name.startswith('Modules/comm_')
                 or name.startswith('Modules/modem_slm336') or name.startswith('User/comm_timeouts')
                 or name in ('User/user_fun.h','User/user_str_fun.c','User/user_str_fun.h','User/user_format.c','User/user_format.h','Bootloader/app_update.c','Bootloader/app_update.h')
                 or name.startswith('Tests/test_comm_') or name in ('Tests/test_access.c', 'Tests/test_count_ends.c', 'Tests/test_log_upload.c', 'Tests/test_sms.c', 'Tests/test_tempid_meta.c', 'Tests/test_update_rules.c', 'Tests/test_upload_retry.c', 'Tests/event_async_model.inc', 'Tests/fw_async_model.inc', 'Tests/store_async_cases.inc', 'Tests/store_callers_cases.inc'))
        if name in C_FILES and owned:
            for comment in comments(text):
                assert not re.search('[A-Za-z]', comment) or re.search('[А-Яа-яЁё]', comment), ('английский комментарий', name, comment[:80])
        if name in PY_FILES:
            # Проверять комментарии Python отдельно от строк генераторов C.
            for token in tokenize.generate_tokens(io.StringIO(text).readline):
                if token.type == tokenize.COMMENT:
                    assert not re.search('[A-Za-z]', token.string) or re.search('[А-Яа-яЁё]', token.string), ('английский комментарий', name, token.start[0])
        if name.startswith('User/c/'):
            assert not re.search(r'\b(?:asm|__asm)\s*\(',text), ('inline assembler in C mirror',name)
    for name in ('Modules/comm_uart.c','Modules/comm_at.c','Modules/comm_modem.c','Modules/modem_slm336.c',
                 'Modules/comm_channel.c','Modules/comm_server.c','Modules/comm_recover.c','Modules/comm_call.c','Modules/comm_sms.c','Modules/comm_balance.c','Tests/asm/reference/User/user_comm.c'):
        text=(ROOT/name).read_bytes().decode('cp1251')
        assert not any(word in text for word in ('malloc(', 'calloc(', 'realloc(', 'snprintf(')), name
        # Этап 5: связь никогда не перезагружает процессор.
        assert not any(word in text for word in ('NVIC_SystemReset', 'user_savereset', 'app_trial_rollback')), name
    print('comm sources: cp1251 C, UTF-8 Python, CRLF, author headers, new modules without heap and without CPU reset PASS')

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--fix',action='store_true'); check(p.parse_args().fix)
