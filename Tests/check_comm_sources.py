"""Доработка: Кряжев А., 2026. Оформление затронутых исходников связи."""
from pathlib import Path
import re
import argparse
import io
import tokenize

ROOT=Path(__file__).resolve().parents[1]
C_FILES=(
    'Bootloader/app_update.c',
    'Bootloader/app_update.h',
    'Modules/comm_at.c',
    'Modules/comm_at.h',
    'Modules/comm_balance.c',
    'Modules/comm_balance.h',
    'Modules/comm_call.c',
    'Modules/comm_call.h',
    'Modules/comm_channel.c',
    'Modules/comm_channel.h',
    'Modules/comm_info.c',
    'Modules/comm_info.h',
    'Modules/comm_modem.c',
    'Modules/comm_modem.h',
    'Modules/comm_power_v5.c',
    'Modules/comm_power_v5.h',
    'Modules/comm_recover.c',
    'Modules/comm_recover.h',
    'Modules/comm_server.c',
    'Modules/comm_server.h',
    'Modules/comm_sms.c',
    'Modules/comm_sms.h',
    'Modules/comm_uart.c',
    'Modules/comm_uart.h',
    'Modules/modem_slm336.c',
    'Modules/modem_slm336.h',
    'Tests/test_comm_at.c',
    'Tests/test_comm_balance.c',
    'Tests/test_comm_call.c',
    'Tests/test_comm_channel.c',
    'Tests/test_comm_recover.c',
    'Tests/test_comm_review.c',
    'Tests/test_comm_server.c',
    'Tests/test_comm_slm.c',
    'Tests/test_comm_sms.c',
    'Tests/test_comm_uart.c',
    'Tests/test_comm_volte.c',
    'Tests/test_sms.c',
    'Tests/test_tempid_meta.c',
    'User/c/crc_asm.c',
    'User/c/crc_runtime.c',
    'User/c/expiry_runtime.c',
    'User/c/fast_kernels.c',
    'User/c/flash_w25_runtime.c',
    'User/c/fw_service_runtime.c',
    'User/c/id_hash_asm.c',
    'User/c/idclear_runtime.c',
    'User/c/idindex_runtime.c',
    'User/c/memory_runtime.c',
    'User/c/photo_runtime.c',
    'User/c/photo_runtime.inc',
    'User/c/runtime_internal.h',
    'User/c/sector_journal_runtime.c',
    'User/c/sector_runtime_runtime.c',
    'User/c/service_ff.inc',
    'User/c/service_runtime.c',
    'User/c/spi_stm32f10x_runtime.c',
    'User/c/table_runtime.inc',
    'User/c/upload_runtime.c',
    'User/c/usb_stm32f10x_runtime.c',
    'User/c/user_access.c',
    'User/c/user_comm_runtime.c',
    'User/c/user_commands.c',
    'User/c/user_config.c',
    'User/c/user_diag.c',
    'User/c/user_fw.c',
    'User/c/user_fw_reason.h',
    'User/c/user_ids.c',
    'User/c/user_log.c',
    'User/c/user_logic_internal.h',
    'User/c/user_main.c',
    'User/c/user_modem.c',
    'User/c/user_protocol.c',
    'User/c/user_rx.c',
    'User/c/user_schedule.c',
    'User/c/user_sms.c',
    'User/c/user_store.c',
    'User/c/user_upload.c',
    'User/comm_timeouts.c',
    'User/comm_timeouts.h',
    'User/user_comm.h',
    'User/user_format.c',
    'User/user_format.h',
    'User/user_fun.h',
    'User/user_str_fun.c',
    'User/user_str_fun.h',
    'stm32_it.c',
)
PY_FILES=(
    'Tests/asm/c_vs_asm_project.py',
    'Tests/asm/test_access_reads.py',
    'Tests/build_comm_uart.py',
    'Tests/call_mutations.py',
    'Tests/check_comm_sources.py',
    'Tests/comm_at_host.py',
    'Tests/comm_mutations.py',
    'Tests/comm_review_host.py',
    'Tests/comm_server_host.py',
    'Tests/comm_slm_host.py',
    'Tests/comm_uart_budget.py',
    'Tests/recover_mutations.py',
    'Tests/run_host.py',
    'Tests/rx_budget_mutations.py',
    'Tests/test_app_update_elf.py',
    'Tests/test_comm_settings_elf.py',
    'Tests/test_iar_fw_async.py',
    'Tests/test_logic_cleanup.py',
    'Tests/test_user_format.py',
)

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
                 or name.startswith('Tests/'))
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
                 'Modules/comm_channel.c','Modules/comm_server.c','Modules/comm_recover.c','Modules/comm_call.c','Modules/comm_sms.c','Modules/comm_balance.c','User/c/user_comm_runtime.c'):
        text=(ROOT/name).read_bytes().decode('cp1251')
        assert not any(word in text for word in ('malloc(', 'calloc(', 'realloc(', 'snprintf(')), name
        # Этап 5: связь никогда не перезагружает процессор.
        assert not any(word in text for word in ('NVIC_SystemReset', 'user_savereset', 'app_trial_rollback')), name
    print('comm sources: cp1251 C, UTF-8 Python, CRLF, author headers, new modules without heap and without CPU reset PASS')

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('--fix',action='store_true'); check(p.parse_args().fix)
