"""Linked sizes and conservative photo attribution; outputs only to --output."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

from iar_irq_support import ELFFile
from audit_checks import FILES, inventory


def module_summary(text, name):
    section = text[text.index('MODULE SUMMARY'):]
    header = next(l for l in section.splitlines() if 'Module ' in l and 'ro code' in l)
    positions = [header.index(n) - 2 for n in ('ro code', 'rw code', 'ro data', 'rw data')]
    row = next(l for l in section.splitlines() if l.startswith('    ' + name + ' '))
    values = []
    for a, b in zip(positions, positions[1:] + [positions[-1] + 9]):
        cell = row[a:b].strip().replace("'", '')
        values.append(int(cell or 0))
    return dict(zip(('ro_code', 'rw_code', 'ro_data', 'rw_data'), values))


def symbols(elf):
    return {s.name: s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}


def photo_detail(elf, text):
    rows = inventory()
    # IAR emits one line-program CU per function as well as type-only CUs.
    by_basename = {Path(f).name: f for f in FILES}
    predicates = {}
    for row in rows:
        for line in range(row['line'], row['last'] + 1):
            predicates.setdefault((row['file'], line), set()).add(row['label'])
    locations = {}
    dwarf = elf.get_dwarf_info()
    for cu in dwarf.iter_CUs():
        if 'DW_AT_stmt_list' not in cu.get_top_DIE().attributes:
            continue
        program = dwarf.line_program_for_CU(cu)
        if not program:
            continue
        previous = None
        for entry in program.get_entries():
            state = entry.state
            if state is None:
                continue
            if previous and not previous.end_sequence and state.address > previous.address:
                filename = program['file_entry'][previous.file - 1].name.decode('utf-8', errors='replace')
                filename = by_basename.get(filename.replace('\\', '/').split('/')[-1])
                if filename:
                    for address in range(previous.address, state.address):
                        locations.setdefault(address, set()).add((filename, previous.line))
            previous = state
    sym = symbols(elf)
    module_functions = set()
    module_memory = set()
    for line in text.splitlines():
        if 'photo_runtime.o [' not in line:
            continue
        match = re.match(r'^(\S+)\s+0x[\da-fA-F\']+\s+0x[\da-fA-F]+\s+(Code|Data)\b', line)
        if match and match[1] in sym:
            s = sym[match[1]]
            start = s['st_value'] & ~1
            if match[2] == 'Code':
                module_functions.update(range(start, start + s['st_size']))
            elif s['st_value'] < 0x20000000:
                module_memory.update(range(s['st_value'], s['st_value'] + s['st_size']))
    extra_names = ('user_send_Photos', 'log_addPhotoEvent', 'user_getPhotoCount')
    extra = set()
    for name in extra_names:
        if name not in sym:
            continue
        s = sym[name]
        extra.update(range(s['st_value'] & ~1, (s['st_value'] & ~1) + s['st_size']))
    caller = set()
    source = (Path(__file__).resolve().parents[1] / 'User/c/user_fun_runtime.c').read_bytes().decode('cp1251')
    photo_lines = {i for i, line in enumerate(source.splitlines(), 1)
                   if re.search(r'\b(?:photo_\w+|user_getPhotoCount|user_send_Photos|log_addPhotoEvent)\s*\(', line)}
    for address, sites in locations.items():
        if address not in extra and any(f == 'User/c/user_fun_runtime.c' and l in photo_lines for f, l in sites):
            caller.add(address)
    attributable = module_functions | extra | caller
    tagged = Counter()
    check_addresses = set()
    for address in attributable:
        labels = set().union(*(predicates.get(site, set()) for site in locations.get(address, ())))
        if labels:
            check_addresses.add(address)
            tagged['защита' if 'защита' in labels else 'нужна'] += 1
    summary = module_summary(text, 'photo_runtime.o')
    module_total = summary['ro_code'] + summary['ro_data']
    photo_rows = [r for r in rows if r['file'] in ('User/c/photo_runtime.c', 'User/c/photo_runtime.inc')]
    # Addresses assigned to both labels count once, with protection taking priority.
    return {'module': summary, 'module_flash': module_total,
            'module_unique_function_bytes': len(module_functions),
            'user_function_bytes': {n: sym[n]['st_size'] if n in sym else 0 for n in extra_names},
            'user_unique_function_bytes': len(extra), 'other_user_callsite_bytes': len(caller),
            'attributable_flash': module_total + len(extra) + len(caller),
            'upload_runtime_photo_bytes': 0,
            'static_module_ram': summary['rw_data'], 'device_photo_fields_bytes': 9,
            'device_ready_photo_shared_bits': 1,
            'shared_workspace_bytes': 2224, 'photo_workspace_peak_slots': {'context': 180, 'packet': 534},
            'predicate_line_code_bytes': len(check_addresses), 'predicate_line_code_by_label': dict(tagged),
            'audit_rows': len(photo_rows), 'audit_labels': dict(Counter(r['label'] for r in photo_rows)),
            'method': 'IAR MODULE SUMMARY + unique ELF symbols + DWARF line programs. Predicate-line bytes include any same-line action and are not a removable-size claim. Shared CRC/UART/journal/workspace are not charged in full. Module total includes the shared RS485 configuration functions.'}


def collect(base):
    builds = {}
    photo = None
    c_maps = {}
    for config in ('Release', 'ReleaseCustomer'):
        for variant in ('ASM', 'C', 'HYBRID'):
            folder = base / (variant + '-' + config)
            binary = folder / 'ExePath' / ('DomovoiNext_' + variant + '.bin')
            image = folder / 'ExePath/DomovoiNext.out'
            text = (folder / 'ListPath/DomovoiNext.map').read_text(encoding='cp1251', errors='replace')
            if variant == 'C':
                c_maps[config] = text
            with image.open('rb') as stream:
                elf = ELFFile(stream)
                sym = symbols(elf)
                limit = sym['CSTACK$$Limit']['st_value']
                fault = sym['fault_record']['st_value']
                assert sym['__vector_table']['st_value'] == 0x08008200
                assert fault == 0x20004FC0
                assert sym['CSTACK$$Limit']['st_value'] - sym['CSTACK$$Base']['st_value'] == 4096
                key = variant + '-' + config
                builds[key] = {'bin_bytes': binary.stat().st_size, 'ram_free': fault - limit,
                               'stack_bytes': 4096, 'cstack_base': hex(sym['CSTACK$$Base']['st_value']),
                               'cstack_limit': hex(limit), 'fault_record': hex(fault),
                               'bin_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                               'elf_sha256': hashlib.sha256(image.read_bytes()).hexdigest()}
                if variant == 'C' and config == 'Release':
                    photo = photo_detail(elf, text)
    deltas = {}
    for name in ('main', 'prof', 'wiegand', 'outs', 'user_fun_runtime', 'usb_stm32f10x_runtime',
                 'memory_runtime', 'usb_istr', 'hw_config', 'comm_modem', 'comm_channel',
                 'comm_server', 'comm_recover', 'comm_uart'):
        before = module_summary(c_maps['Release'], name + '.o')
        try:
            after = module_summary(c_maps['ReleaseCustomer'], name + '.o')
        except StopIteration:
            after = {k: 0 for k in before}
        deltas[name] = {'flash_before': before['ro_code'] + before['ro_data'],
                        'flash_after': after['ro_code'] + after['ro_data'],
                        'ram_before': before['rw_data'], 'ram_after': after['rw_data']}
    return {'builds': builds, 'photo_C': photo, 'diagnostic_module_deltas_C': deltas}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = collect(args.build_root)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    for name, value in result['builds'].items():
        print(name, 'BIN', value['bin_bytes'], 'RAM free', value['ram_free'])
    print('Photo C:', result['photo_C']['attributable_flash'], 'Flash;', result['photo_C']['static_module_ram'], 'static module RAM')
