"""Функции и данные вне связанных образов: карта IAR, граф исходников и контракты тестов."""
import argparse
import json
import re
from pathlib import Path
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\r\n]*|/\*[\s\S]*?\*/')

def masked(text):
    return TOKEN.sub(lambda m: re.sub(r'[^\r\n]', ' ', m.group()), text)

def definitions(path):
    """Выделить тела функций, сохраняя смещения для точного удаления и отчёта."""
    text = path.read_text(encoding='utf-8')
    clean = masked(text)
    pattern = re.compile(r'(?m)^[ \t]*(?:[\w]+[ \t\r\n*]+)+(?P<name>[A-Za-z_]\w*)[ \t]*\([^;{}]*\)[ \t\r\n]*\{')
    result = []
    for m in pattern.finditer(clean):
        if m.group('name') in ('if','while','for','switch'): continue
        end, depth = m.end(), 1
        while depth and end < len(clean):
            depth += (clean[end] == '{') - (clean[end] == '}'); end += 1
        if not depth:
            result.append({'name': m.group('name'), 'start': m.start(), 'end': end,
                           'line': text.count('\n', 0, m.start()) + 1,
                           'body': clean[m.end():end], 'static': bool(re.search(r'\bstatic\b', m.group()))})
    return text, result

def inspect(build_root, source_root=ROOT, tests_root=None):
    tests_root=tests_root or source_root/'Tests'
    symbols = set(); objects = set(); data_objects = set(); maps = []
    for folder,stem in [('Release','DomovoiNext'),('ReleaseCustomer','DomovoiNext'),('BOOT-Release','DomovoiBoot')]:
        elf = build_root/folder/'ExePath'/f'{stem}.out'
        map_path = build_root/folder/'ListPath'/f'{stem}.map'
        maps.append(map_path)
        # Карта обязательна: отсутствие сборки не должно давать пустой зелёный отчёт.
        map_text = map_path.read_text(encoding='utf-8-sig', errors='replace')
        assert 'Errors: none' in map_text, map_path
        with elf.open('rb') as f:
            symbols.update(s.name for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols()
                           if s['st_shndx'] != 'SHN_UNDEF')
        commands=json.loads((build_root/folder/'commands.json').read_text(encoding='utf-8-sig'))
        active_objects=[Path(c['output']) for c in commands if Path(c['arguments'][0]).stem.lower() in ('iccarm','iasmarm')]
        for obj in active_objects:
            with obj.open('rb') as f:
                table = ELFFile(f).get_section_by_name('.symtab')
                if table:
                    emitted=[s for s in table.iter_symbols() if s['st_shndx'] != 'SHN_UNDEF']
                    objects.update(s.name for s in emitted)
                    data_objects.update(s.name for s in emitted if s['st_info']['type']=='STT_OBJECT')
    source = {}; funcs = {}; by_name = {}
    for path in source_root.rglob('*'):
        if path.suffix not in ('.c','.h','.inc') or 'Tests' in path.relative_to(source_root).parts: continue
        text, defs = definitions(path); source[path] = (text, defs)
        for d in defs:
            key = (path, d['name']); funcs[key] = d; by_name.setdefault(d['name'], []).append(key)
    # Контракт — явный вызов/адрес символа в стенде; упоминание в комментарии не считается.
    contracts = set()
    for path in tests_root.rglob('*'):
        if path.suffix not in ('.py','.c','.h'):continue
        if path.name == Path(__file__).name: continue
        test_text=path.read_text(encoding='utf-8')
        contracts.update(re.findall(r'(?:call|checked|addr|map_size)\(\s*[\'"]([A-Za-z_]\w*)[\'"]', test_text))
        contracts.update(re.findall(r'\b([A-Za-z_]\w*)\s*\(', test_text))
    live = {key for key in funcs if key[1] in symbols or key[1] in contracts}
    queue = list(live)
    while queue:
        key = queue.pop()
        for name in set(re.findall(r'\b[A-Za-z_]\w*\b', funcs[key]['body'])):
            local = (key[0], name)
            targets = [local] if local in funcs and funcs[local]['static'] else [k for k in by_name.get(name,[]) if not funcs[k]['static']]
            for target in targets:
                if target not in live: live.add(target); queue.append(target)
    whitelist_path = source_root/'Tests/dead_code_allowlist.json'
    allowlist = json.loads(whitelist_path.read_text(encoding='utf-8')) if whitelist_path.exists() else {}
    dead = []
    for key,d in funcs.items():
        if key in live: continue
        # Inline без объектного символа не выбрасывал линкер: он встроен или не компилировался.
        if key[0].suffix=='.h' and key[1] not in objects:continue
        label = key[0].relative_to(source_root).as_posix()+':'+key[1]
        if label in allowlist:
            assert allowlist[label].strip(), label; continue
        dead.append({'file': key[0].relative_to(source_root).as_posix(),'name': key[1], 'line': d['line'],
                     'emitted_object':key[1] in objects})
    # Чтения в телах функций и передача адреса; простое присваивание чтением не считается.
    bodies='\n'.join(d['body'] for key,d in funcs.items() if key in live)
    initializers={}
    outside=[]
    for text,defs in source.values():
        clean=masked(text)
        for d in sorted(defs,key=lambda d:d['start'],reverse=True):clean=clean[:d['start']]+' '*(d['end']-d['start'])+clean[d['end']:]
        outside.append(clean)
    globals_text='\n'.join(outside)
    for name in data_objects:
        matches=re.finditer(r'\b'+re.escape(name)+r'\s*(?:\[[^\]]*\]\s*)*=\s*([^;]*);',globals_text)
        initializers[name]='\n'.join(m.group(1) for m in matches)
    assembly='\n'.join(re.sub(r';[^\r\n]*','',p.read_text(encoding='utf-8')) for p in source_root.rglob('*.s') if 'Tests' not in p.parts)
    unread = [];read_data=set(contracts)
    for name in data_objects:
        if re.search(r'\b'+re.escape(name)+r'\b',assembly):read_data.add(name)
        for use in re.finditer(r'\b'+re.escape(name)+r'\b',bodies):
            prefix=bodies[:use.start()].rstrip();tail=bodies[use.end():]
            write_only=re.match(r'(?:\s*(?:\[[^\]]*\]|\.\w+))*\s*=(?!=)',tail)
            if prefix.endswith('&') or not write_only:read_data.add(name);break
    pending=list(read_data&data_objects)
    while pending:
        parent=pending.pop()
        for name in set(re.findall(r'\b[A-Za-z_]\w*\b',initializers.get(parent,'')))&data_objects:
            if name not in read_data:read_data.add(name);pending.append(name)
    for name in sorted(data_objects):
        if not re.fullmatch(r'[A-Za-z_]\w*',name) or name in by_name or name in contracts: continue
        if name in read_data:continue
        label='data:'+name
        if label not in allowlist:unread.append(name)
    return {'functions':dead,'unread_data':unread,'source_functions':len(funcs),'reachable_functions':len(live),
            'maps':[str(p) for p in maps],'allowlist':allowlist}

def main():
    from iar_image import result_path
    p=argparse.ArgumentParser();p.add_argument('--build-root',type=Path,default=result_path('.'))
    p.add_argument('--output',type=Path,default=result_path('dead-code.json'));a=p.parse_args()
    result=inspect(a.build_root);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    n=len(result['functions'])+len(result['unread_data'])
    print(('FAIL' if n else 'PASS')+f' dead code: {len(result["functions"])} functions, {len(result["unread_data"])} data; '+str(a.output))
    return int(bool(n))

if __name__=='__main__':raise SystemExit(main())
