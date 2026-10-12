"""Мутации приёма SMS SLM336: возврат прежних потерь обязан ломать модель."""
from pathlib import Path
import os
import subprocess
import tempfile
from temp_cleanup import TemporaryDirectory
from comm_server_host import unit, ZIG


def run():
    out = Path(tempfile.gettempdir()) / 'domovoi-sms-mutations'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = unit().replace('\r\n', '\n')
    faults = (
        ('unsupported-cnmi', '"AT+CNMI=0,1,0,0,0" };', '"AT+CNMI=2,1,0,0,0" };'),
        ('setup-forever', 'r != COMM_AT_OK && msg.step != 2u', 'r != COMM_AT_OK'),
        ('index-zero', '*index = (uint16_t)v; return true;', '*index = (uint16_t)v; return v != 0;'),
        ('silent-delete', 'sms_deliver();\n  } else if (op == SMS_OP_DELETE)', 'if (msg.field == P_END) sms_deliver();\n  } else if (op == SMS_OP_DELETE)'),
        ('full-sim-no-scan', 'else if (msg.scan && sms_command', 'else if (0 && msg.scan && sms_command'),
        ('scan-lost-when-busy', 'else if (msg.scan && sms_command(SMS_OP_LIST', 'else if (msg.scan && (msg.scan = 0, sms_command(SMS_OP_LIST'),
        ('queue-overflow', 'msg.count < sizeof(msg.queue) / sizeof(msg.queue[0])', 'msg.count < sizeof(msg.queue)'),
        ('repeat-event', 'bool first = msg.deliver == 1u;', 'bool first = true;'),
    )
    # Последняя скобка нужна только составной мутации занятого слота.
    with TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for name, old, new in faults:
            assert code.count(old) == 1, (name, code.count(old))
            mutant = code.replace(old, new, 1)
            if name == 'scan-lost-when-busy':
                mutant = mutant.replace('0, COMM_AT_PDU, COMM_T_SMS)) msg.scan &=',
                                        '0, COMM_AT_PDU, COMM_T_SMS))) msg.scan &=', 1)
            (tmp / 'sms.c').write_text(mutant, encoding='utf-8')
            subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            str(tmp / 'sms.c'), '-o', str(tmp / 'sms.exe')], check=True)
            r = subprocess.run([str(tmp / 'sms.exe')], capture_output=True, timeout=300)
            (out / (name + '.log')).write_bytes(r.stdout + r.stderr)
            assert r.returncode != 0, ('undetected', name)
            print(name, 'detected')
    print('sms mutations: 8/8 detected')


if __name__ == '__main__':
    run()
