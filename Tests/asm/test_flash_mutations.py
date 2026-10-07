"""Вернуть старое поведение в копиях исходников TEMP и исполнить мутированный ELF.

Компилируются только затронутые объекты; рабочее дерево и исходный ELF не меняются.
Ошибка сборки или запуска не считается обнаруженной мутацией.
"""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from firmware_suite import ROOT


MAIN = 'User/c/user_main.c'
APP = 'Bootloader/app_update.c'
MUTATIONS = {
    'E01': ('wiegand', [(MAIN, 'if (config.wiegandType != 0)\n    WG_process();',
                        'if (config.wiegandType != 0 && user_storage_flash_ready())\n    WG_process();')]),
    'E02': ('scenes', [('User/c/user_access.c',
                        'else if (device.triggers.sheduleUnBlock && log_timeKnown) {',
                        'else if (device.triggers.sheduleUnBlock && log_timeKnown && mem_tables_ready()) {')]),
    'E03': ('server_frame', [('User/c/user_rx.c', '!server_rx_busy && !server_rx_ready && server_rx_count',
                            '!server_rx_busy && server_rx_count')]),
    'E04': ('rollback', [(MAIN, 'if (app_trial_rollback()) user_savereset();',
                         'app_trial_rollback(); user_savereset();')]),
    'E05': ('factory_reset', [
        (MAIN, 'if (device.timeToDefault > 1) --device.timeToDefault;\n      else if (user_storage_command_ready()) user_factoryReset();',
         'if (--device.timeToDefault == 0) user_factoryReset();'),
        ('User/c/user_config.c', '  if (store_runtime && !user_storage_command_ready()) {\n    device.timeToDefault = 1;\n    return;\n  }\n', '')]),
    'E06': ('firmware_events', [('User/c/user_fw.c',
        'if (!fw_event(code == FW_REASON_TRIAL_FAIL ? LOG_FW_ROLLBACK : LOG_FW_FAIL, code)) return;',
        'fw_event(code == FW_REASON_TRIAL_FAIL ? LOG_FW_ROLLBACK : LOG_FW_FAIL, code);')]),
    'E06-runtime': ('firmware_events', [('User/c/user_fw.c',
        'if (!fw_event_count && fw_event(num, reason)) return;',
        'fw_event(num, reason); return;')]),
    'E07': ('manual_read', [(APP, 'return mem_read(BOOT_CANDIDATE+off,data,n);',
                            'mem_read(BOOT_CANDIDATE+off,data,n);return 1;')]),
    'E08': ('manual_write', [(APP, 'return mem_write(BOOT_CANDIDATE+off,(uint8_t*)data,n);',
                             'mem_write(BOOT_CANDIDATE+off,(uint8_t*)data,n);return 1;')]),
    'E09': ('manual_erase', [(APP, 'return mem_erase_sector(BOOT_CANDIDATE+off);',
                             'mem_erase_sector(BOOT_CANDIDATE+off);return 1;')]),
}


def run(mutations=MUTATIONS, module='test_flash_fixes', suite='flash-mutations'):
    base = Path(os.environ['DOMOVOI_COMPARE_BUILD'])
    config = os.environ.get('DOMOVOI_COMPARE_CONFIG', 'Release')
    database = next(p/config/'commands.json' for p in (base, *base.parents)
                    if (p/config/'commands.json').exists())
    folder = database.parent
    commands = json.loads(database.read_text(encoding='utf-8-sig'))
    results = {}
    for tag, (test, changes) in mutations.items():
        dest = base/suite/config/tag
        for sub in ('ObjPath', 'ExePath', 'ListPath'): (dest/sub).mkdir(parents=True, exist_ok=True)
        objects = {}
        log = bytearray()
        def execute(args):
            r = subprocess.run(args, cwd=ROOT, capture_output=True)
            log.extend(r.stdout + r.stderr)
            (dest/'build.log').write_bytes(log)
            assert r.returncode == 0, f'{tag}: ошибка сборки; {dest / "build.log"}'
        for name, old, new in changes:
            source = ROOT/name
            text = source.read_text(encoding='utf-8')
            assert text.count(old) == 1, (tag, name, 'место мутации изменилось')
            text = text.replace(old, new)
            text = re.sub(r'(#include\s+")([^"]+)(")',
                          lambda m: m[1] + ((source.parent/m[2]).resolve().as_posix()
                                           if (source.parent/m[2]).exists() else m[2]) + m[3], text)
            target = dest/source.name
            target.write_bytes(text.replace('\n', '\r\n').encode('utf-8'))
            # База собрана из рабочего дерева, проверка идёт в его копии: ищем по пути от корня.
            command = next(c for c in commands
                           if c.get('file', '').replace('\\', '/').endswith('/' + name))
            args = list(command['arguments'])
            args[args.index(command['file'])] = str(target)
            args[args.index('-o') + 1] = str(dest/'ObjPath')
            args.extend(['-I', str(source.parent)])
            execute(args)
            objects[command['output']] = str(dest/'ObjPath'/Path(command['output']).name)
        for command in commands:
            if command['type'] not in ('LINKER', 'OBJCOPY'): continue
            args = [objects.get(a, a).replace(str(folder/'ExePath'), str(dest/'ExePath'))
                    .replace(str(folder/'ListPath'), str(dest/'ListPath')) for a in command['arguments']]
            execute(args)
        env = os.environ.copy()
        for suffix, sub, ext in (('ELF','ExePath','out'), ('BIN','ExePath','bin'), ('MAP','ListPath','map')):
            env['DOMOVOI_' + suffix] = str(dest/sub/('DomovoiNext.' + ext))
        script = (f'import sys; sys.path[:0] = {repr([str(ROOT/"Tests"), str(ROOT/"Tests/asm")])}; '
                  f'import {module}; {module}.{test}()')
        r = subprocess.run([sys.executable, '-c', script], cwd=ROOT, env=env, capture_output=True)
        (dest/'test.log').write_bytes(r.stdout + r.stderr)
        assert r.returncode != 0 and b'AssertionError' in r.stderr, f'{tag}: мутация не обнаружена'
        results[tag] = {'test': test, 'detected': True}
    return results
