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


def build(variant, base, config='Release', mfc=False, project_path=None):
    project_path = Path(project_path or ROOT / ('DomovoiNext_' + variant + '.ewp'))
    stem = project_path.stem
    folder = base / (variant + '-' + config)
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
    if mfc:


        sources=[c for c in commands if Path(c['arguments'][0]).stem.lower()=='iccarm'
                 and Path(c['file']).parent != ROOT/'User/c']
        first=sources[0]
        def options(c):
            args=c['arguments'].copy()
            args.remove(c['file'])
            args[args.index('-o')+1]='OUTPUT'
            return args
        if any(options(c)!=options(first) for c in sources):
            raise RuntimeError('MFC files have different compiler options')
        combined=first.copy();combined['arguments']=first['arguments'].copy()
        at=combined['arguments'].index(first['file'])
        combined['arguments'][at:at+1]=[c['file'] for c in sources]
        combined['arguments'].append('--mfc')
        omitted={c['output'] for c in sources[1:]}
        commands=[c for c in commands if c not in sources]
        for command in commands:
            command['arguments']=[a for a in command['arguments'] if a not in omitted]
        commands.insert(0,combined)
        (folder/'mfc-commands.json').write_text(json.dumps(commands,indent=2),encoding='utf-8')
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
    print(variant, 'BIN', (folder/'ExePath'/(stem+'.bin')).stat().st_size, flush=True)
    return folder


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--project', choices=('C', 'HYBRID'), required=True)
    parser.add_argument('--build-root', type=Path, default=Path(tempfile.gettempdir())/'domovoi-c-asm')
    parser.add_argument('--config', default='Release')
    parser.add_argument('--mfc', action='store_true', help='Explicit compiler experiment: group identical C commands with --mfc')
    args = parser.parse_args()
    build(args.project, args.build_root, args.config, args.mfc)
