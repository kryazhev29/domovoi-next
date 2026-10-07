"""Сборка команд исходного IAR-проекта в TEMP: компиляция, линковка, карта и BIN.

IarBuild -jsondb сохраняет параметры проекта; прямой запуск инструментов обходит
ошибку разрешения путей в песочнице. Успех IarBuild -build этим не подтверждается."""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def build(base, config='Release', project_path=None, boot=False):
    project_path = Path(project_path or ROOT / ('Bootloader/DomovoiBoot.ewp' if boot else 'DomovoiNext.ewp'))
    stem = project_path.stem
    base = Path(base)
    folder = base / (('BOOT-' if boot else '') + config)
    folder.mkdir(parents=True, exist_ok=True)
    tree = ET.parse(project_path)
    for node in tree.findall('.//state') + tree.findall('.//file/name'):
        if node.text:
            node.text = node.text.replace('$PROJ_DIR$', str(project_path.parent))
    for opt in tree.findall('.//option'):
        if opt.findtext('name') in ('ExePath', 'ObjPath', 'ListPath'):
            opt.find('state').text = str(folder / opt.findtext('name'))
    project = folder / (stem + '.ewp')
    tree.write(project, encoding='utf-8', xml_declaration=True)
    iar = Path(os.environ.get('DOMOVOI_IAR', r'C:\Program Files\IAR Systems\Embedded Workbench 9.2'))
    database = folder / 'commands.json'
    result = subprocess.run([str(iar/'common/bin/IarBuild.exe'), str(project),
                             '-jsondb', config, '-output', str(database)], capture_output=True)
    (folder/'jsondb.log').write_bytes(result.stdout+result.stderr)
    result.check_returncode()
    commands = json.loads(database.read_text(encoding='utf-8-sig'))
    def execute(command):
        outputs = command.get('outputs') or [command['output']]
        for output in outputs:
            Path(output).parent.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(command['arguments'], cwd=ROOT, capture_output=True)
        return command, r

    compilers = [c for c in commands if Path(c['arguments'][0]).stem.lower() in ('iccarm', 'iasmarm')]
    logs = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for command, r in pool.map(execute, compilers):
            logs.append(r.stdout+r.stderr)
            if r.returncode:
                (folder/'direct-build.log').write_bytes(b'\n'.join(logs))
                print((r.stdout+r.stderr).decode(errors='replace')[-3000:])
                raise RuntimeError(command['file'])
    for command in commands:
        if command in compilers:
            continue
        command, r = execute(command)
        logs.append(r.stdout+r.stderr)
        if r.returncode:
            (folder/'direct-build.log').write_bytes(b'\n'.join(logs))
            print((r.stdout+r.stderr).decode(errors='replace')[-5000:])
            raise RuntimeError(command.get('file') or command.get('output') or command.get('outputs'))
    (folder/'direct-build.log').write_bytes(b'\n'.join(logs))
    print(stem, config, 'BIN', (folder/'ExePath'/(stem+'.bin')).stat().st_size, flush=True)
    return folder


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--boot', action='store_true', help='Собрать загрузчик v5')
    parser.add_argument('--build-root', type=Path, default=Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir())/'domovoi-agent-build')))
    parser.add_argument('--config', choices=('Release','ReleaseCustomer'), default='Release')
    args = parser.parse_args()
    if args.boot and args.config != 'Release': parser.error('Загрузчик имеет только профиль Release')
    build(args.build_root, args.config, boot=args.boot)
