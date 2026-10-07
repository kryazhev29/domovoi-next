"""Три мутации исполнителя в изолированных IAR-проектах TEMP; рабочий код не меняется."""
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

from build_iar import ROOT, build


def main():
    base = Path(os.environ['DOMOVOI_COMPARE_BUILD']) / 'unblock-mutations'
    source = (ROOT/'User/c/user_access.c').read_text(encoding='utf-8')
    priority = source.replace('else if (device.triggers.sheduleUnBlock', 'if (device.triggers.sheduleUnBlock', 1)
    priority = priority.replace('      uint8_t scenes[8];', '      uint8_t scenes[8];\n      next = 0;', 1)
    order = source.replace('state = !next || unblock_wait;', 'state = !next;', 1)
    offline = source.replace('>= 1800u', '>= 0xffffffffu')
    results = {}
    for name, mutant, test, message in (
        ('manual-priority', priority, 'manual', 'ручная важнее сцены'),
        ('opening-order', order, 'pulse_delay', 'ИК разорвана во время импульса'),
        ('offline-30', offline, 'offline', 'оффлайн 30 минут')):
        assert mutant != source
        scope = base/name; scope.mkdir(parents=True, exist_ok=True)
        for header in ('user_logic_internal.h', '../user_format.h', '../sched16.h'):
            path = (ROOT/'User/c'/header).resolve().as_posix()
            mutant = mutant.replace(f'#include "{header}"', f'#include "{path}"')
        changed = scope/'user_access.c'
        changed.write_bytes(mutant.replace('\n', '\r\n').encode('utf-8'))
        project = ET.parse(ROOT/'DomovoiNext.ewp')
        for node in project.findall('.//state') + project.findall('.//file/name'):
            if node.text: node.text = node.text.replace('$PROJ_DIR$', str(ROOT))
        for node in project.findall('.//file/name'):
            if node.text.replace('\\', '/').endswith('/User/c/user_access.c'): node.text = str(changed)
        project_path = scope/'DomovoiNext.ewp'
        project.write(project_path, encoding='utf-8', xml_declaration=True)
        build(scope/'build', project_path=project_path)
        env = os.environ.copy()
        env.update(DOMOVOI_COMPARE_BUILD=str(scope/'build'), DOMOVOI_COMPARE_CONFIG='Release', PYTHONIOENCODING='utf-8')
        code = "import sys;sys.path.insert(0,'Tests/asm');from firmware_suite import select_image;select_image();" \
               f"from test_unblock import {test};{test}()"
        run = subprocess.run([sys.executable, '-c', code], cwd=ROOT, env=env, capture_output=True)
        log = (run.stdout+run.stderr).decode('utf-8', errors='replace')
        (scope/'test.log').write_text(log, encoding='utf-8')
        killed = run.returncode != 0 and 'AssertionError' in log and message in log
        results[name] = {'killed': killed, 'test': test, 'reason': message, 'log': str(scope/'test.log')}
        print(('PASS' if killed else 'FAIL')+' mutation '+name, flush=True)
    (base/'summary.json').write_text(json.dumps(results, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    return 0 if all(r['killed'] for r in results.values()) else 1


if __name__ == '__main__': raise SystemExit(main())
