"""IAR Thumb-2 candidates: differential tests and executed instructions (not cycles)."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import tempfile
import xml.etree.ElementTree as ET

from elftools.elf.elffile import ELFFile
from unicorn import UcError
from test_asm_crc import (ROOT, IAR, Machine, run, BUF, STACK, UC_ARM_REG_SP,
                          UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, function)
from test_perf_scan import count_call, ff_reference

KINDS = ('fnv10', 'gf2', 'all_ff', 'id_equal', 'sldn')
ASM = dict(zip(KINDS, ('fast_fnv10', 'fast_gf2_apply', 'fast_all_ff',
                       'fast_id_equal', 'fast_sldn')))
SELECTED = ('fnv10', 'gf2', 'all_ff', 'sldn')


def build(out, assembly=None, production=None):
    names = [p+k for p in ('old_', 'opt_') for k in KINDS] + ['opt_gf2_sparse']
    objs, keep = [], list(ASM.values()) + ['fast_gf2_sparse', 'fast_gf2_unrolled']
    # Also link the actual production source, separately from rejected candidates.
    prod = (production or ROOT/'User/asm/fast_kernels.s').read_text().replace('fast_', 'prod_fast_')
    (out/'prod.s').write_text(prod, encoding='ascii')
    run([IAR/'iasmarm.exe', out/'prod.s', '--cpu', 'Cortex-M3', '-o', out/'prod.o'])
    objs.append(out/'prod.o')
    keep.extend('prod_'+ASM[k] for k in SELECTED)
    for prefix, opt in [('size_', '-Ohz'), ('speed_', '-Ohs')]:
        obj = out / (prefix+'.o')
        run([IAR/'iccarm.exe', ROOT/'Tests/asm/fast_candidates.c', '--cpu',
             'Cortex-M3', '--thumb', opt, '-e',
             *('-D'+n+'='+prefix+n for n in names), '-o', obj])
        objs.append(obj)
        keep.extend(prefix+n for n in names)
    run([IAR/'iasmarm.exe', assembly or ROOT/'Tests/asm/fast_candidates.s',
         '--cpu', 'Cortex-M3', '-o', out/'asm.o'])
    body = function((ROOT/'User/crc.c').read_text(encoding='cp1251'), 'crc16_ffInit')
    wrappers = 'extern unsigned short crc16_calcInit(unsigned char*, unsigned short, unsigned short);\n'
    for name in variants('gf2'):
        wrappers += 'extern unsigned short '+name+'(const unsigned short*, unsigned short);\n'
        wrappers += body.replace('gf2_apply(', name+'(').replace('crc16_ffInit', 'ff_'+name)+'\n'
        keep.append('ff_'+name)
    (out/'ff.c').write_text(wrappers, encoding='ascii')
    run([IAR/'iccarm.exe', out/'ff.c', '--cpu', 'Cortex-M3', '--thumb', '-Ohz', '-e', '-o', out/'ff.o'])
    run([IAR/'iasmarm.exe', ROOT/'User/asm/crc_asm.s', '--cpu', 'Cortex-M3', '-o', out/'crc.o'])
    run([IAR/'ilinkarm.exe', *objs, out/'asm.o', out/'ff.o', out/'crc.o', '--config', ROOT/'Tests/asm/asm_link.icf',
         '--use_optimized_variants', 'small', '--no_entry', '--keep', ','.join(keep), '--map', out/'fast.map', '-o', out/'fast.out'])
    return out/'fast.out'


def variants(kind):
    return [p+c+kind for p in ('size_', 'speed_') for c in ('old_', 'opt_')] + [ASM[kind]] + (
        ['fast_gf2_sparse', 'fast_gf2_unrolled', 'size_opt_gf2_sparse', 'speed_opt_gf2_sparse'] if kind == 'gf2' else []) + (
        ['prod_'+ASM[kind]] if kind in SELECTED else [])


def reference(kind, a, b=None):
    if kind == 'fnv10':
        h = 2166136261
        for v in a:
            h = ((h ^ v)*16777619) & 0xffffffff
        return h
    if kind == 'gf2':
        r = 0
        for i, v in enumerate(a):
            if b & (1 << i):
                r ^= v
        return r
    if kind == 'all_ff':
        return int(all(v == 255 for v in a))
    if kind == 'id_equal':
        return int(a == b)
    for i in range(len(a)):
        if a[i:i+4] == b'SLDN'[:min(4, len(a)-i)]:
            return i
    return len(a)


def arguments(m, kind, a, b=None, align=0, align2=0):
    p, q = BUF+align, BUF+0x10000+align2
    if kind == 'gf2':
        m.uc.mem_write(p, b''.join(v.to_bytes(2, 'little') for v in a))
        return p, b
    m.uc.mem_write(p, a or b'\xa5')
    if kind == 'fnv10':
        return (p,)
    if kind == 'id_equal':
        m.uc.mem_write(q, b)
        return p, q
    return p, len(a)


def cases():
    rng = random.Random(20261006)
    for align in range(4):
        for a in [b'\0'*10, b'\xff'*10, bytes(range(10))] + [rng.randbytes(10) for _ in range(256)]:
            yield 'fnv10', a, None, align, 0
        for align2 in range(4):
            for pos in range(-1, 10):
                a = rng.randbytes(10)
                for bit in range(8):
                    b = bytearray(a)
                    if pos >= 0:
                        b[pos] ^= 1 << bit
                    yield 'id_equal', a, bytes(b), align, align2
        for n in (0, 1, 2, 3, 4, 5, 7, 8, 10, 16, 31, 32, 63, 64, 65, 255, 256, 257, 1024, 4096, 65535):
            yield 'all_ff', b'\xff'*n, None, align, 0
            for pos in set((0, 1, 2, 3, 4, 7, n//2, n-1)):
                if 0 <= pos < n:
                    for bit in range(8):
                        a = bytearray(b'\xff'*n)
                        a[pos] ^= 1 << bit
                        yield 'all_ff', bytes(a), None, align, 0
        for a in [b'', b'S', b'SL', b'SLD', b'SLDN', b'SSLDN', b'SLSLDN', b'SLDSLDN', b'X'*2048,
                  b'S'*2048, b'SLDX'*512] + [b'X'*p+b'SLDN'[:n] for p in range(17) for n in range(5)]:
            yield 'sldn', a, None, align, 0
        for _ in range(300):
            a = rng.randbytes(rng.randrange(300))
            if rng.randrange(2):
                pos = rng.randrange(len(a)+1)
                a = a[:pos]+b'SLDN'[:rng.randrange(1, 5)]+a[pos:]
            yield 'sldn', a, None, align, 0
    matrices = [[1 << i for i in range(16)], [0]*16, [65535]*16,
                [rng.getrandbits(16) for _ in range(16)]]
    for align in (0, 2):
        for a in matrices:
            for x in [0, 65535, 0xaaaa, 0x5555] + [1 << i for i in range(16)] + [rng.getrandbits(16) for _ in range(300)]:
                yield 'gf2', a, x, align, 0


def check(path, all_variants=True, bounds=False, prefix=''):
    m, counts = Machine(path), {k: 0 for k in KINDS}
    reads, writes = [], []
    if bounds:
        m.uc.hook_add(UC_HOOK_MEM_READ, lambda u, t, p, n, v, d: reads.append((p, n)))
        m.uc.hook_add(UC_HOOK_MEM_WRITE, lambda u, t, p, n, v, d: writes.append((p, n)))
    if not all_variants:
        for kind in SELECTED:
            assert prefix+ASM[kind] in m.sym, ('missing selected kernel', kind)
        assert m.call(prefix+'fast_all_ff', 0, 0, check_abi=True) == 1
        assert m.call(prefix+'fast_sldn', 0, 0, check_abi=True) == 0
        assert m.call(prefix+'fast_gf2_apply', 0, 0, check_abi=True) == 0
        assert not reads and not writes, (reads, writes)
    for kind, a, b, align, align2 in cases():
        args = arguments(m, kind, a, b, align, align2)
        expected = reference(kind, a, b)
        for name in variants(kind) if all_variants else (prefix+ASM[kind],):
            if name not in m.sym:
                continue
            reads.clear(); writes.clear()
            try:
                actual = m.call(name, *args, check_abi=True)
            except UcError as error:
                raise AssertionError((name, args, len(a), align, reads[-5:], str(error))) from error
            assert actual == expected, (kind, name, len(a), b, align)
            counts[kind] += 1
            if bounds:
                # Selected leaf kernels may only read the exact input and ROM literals.
                ranges = [(args[0], args[0]+(32 if kind == 'gf2' else len(a)))]
                if kind == 'id_equal':
                    ranges.append((args[1], args[1]+10))
                leaf = name.startswith(('fast_', 'prod_fast_'))
                assert not writes if leaf else all(STACK-256 <= p and p+n <= STACK for p, n in writes), (name, writes)
                for p, n in reads:
                    assert (0x08000000 <= p and p+n <= 0x08020000) or (not leaf and STACK-256 <= p and p+n <= STACK) or any(lo <= p and p+n <= hi for lo, hi in ranges), (name, p, n, ranges)
                    if leaf:
                        assert p % n == 0, (name, 'unaligned load', p, n)
    return counts


def exhaustive_gf2(path, prefix=''):
    m, rng = Machine(path), random.Random(913)
    a = [rng.getrandbits(16) for _ in range(16)]
    args = arguments(m, 'gf2', a, 0, 2)
    for x in range(65536):
        assert m.call(prefix+'fast_gf2_apply', args[0], x, check_abi=True) == reference('gf2', a, x), x
    return 65536


def guarded_c_bounds(path):
    """Unmapped guard pages, without read hooks that disturb IAR memcmp IT in Unicorn.

    Values/ABI cover all alignments separately. These probes place inputs at both
    page edges; selected ASM additionally gets exact byte-granularity read hooks.
    """
    m, count = Machine(path), 0
    a_page, b_page = 0x11000000, 0x12000000
    m.uc.mem_map(a_page, 4096)
    m.uc.mem_map(b_page, 4096)
    samples = [('fnv10', bytes(range(10)), None),
               ('gf2', [1 << i for i in range(16)], 0xffff),
               ('gf2', [65535]*16, 0),
               ('id_equal', b'0123456789', b'0123456789'),
               ('id_equal', b'0123456789', b'012345678X')]
    for n in (0, 1, 2, 3, 4, 5, 7, 8, 10, 32, 64, 256, 1024, 4096):
        samples.extend([('all_ff', b'\xff'*n, None), ('sldn', b'X'*n, None)])
        if n:
            samples.append(('all_ff', b'\xff'*(n-1)+b'\xfe', None))
    samples.extend(('sldn', b'X'*p+b'SLDN'[:n], None) for p in (0, 1, 3, 60) for n in range(5))
    for kind, a, b in samples:
        raw = b''.join(v.to_bytes(2, 'little') for v in a) if kind == 'gf2' else a
        for end in (False, True):
            p = a_page+4096-len(raw) if end else a_page
            q = b_page+4096-10 if end else b_page
            if raw:
                m.uc.mem_write(p, raw)
            if kind == 'id_equal':
                m.uc.mem_write(q, b)
            args = (p,) if kind == 'fnv10' else (p, q if kind == 'id_equal' else b if kind == 'gf2' else len(a))
            for name in variants(kind):
                if not name.startswith(('size_', 'speed_')):
                    continue
                assert m.call(name, *args, check_abi=True) == reference(kind, a, b), name
                assert bytes(m.uc.mem_read(p, len(raw))) == raw if raw else True
                if kind == 'id_equal':
                    assert bytes(m.uc.mem_read(q, 10)) == b
                count += 1
    return count


def mutations(out):
    source = (ROOT/'User/asm/fast_kernels.s').read_text()
    faults = [
        ('fnv-byte-bound', '[r1, #9]', '[r1, #10]'),
        ('fnv-prime', '=16777619', '=16777617'),
        ('gf-zero', 'CBZ r1, gs_done', 'NOP'),
        ('gf-high-bits', 'MOVS r3, #0', 'MOVS r3, #0\n        UXTB r1, r1'),
        ('gf-column-bound', '[r0, r2, LSL #1]', '[r0, r2, LSL #2]'),
        ('gf-xor', 'EORS r3, r3, r2', 'ORRS r3, r3, r2'),
        ('ff-empty', 'CBZ r1, ff_true', 'CBZ r1, ff_false'),
        ('ff-word-bound', 'CMP r1, #4', 'CMP r1, #3'),
        ('ff-alignment', 'TST r0, #3', 'TST r0, #1'),
        ('ff-dirty', 'BNE ff_false', 'BEQ ff_false'),
        ('ff-tail', 'ff_bytes:\n', 'ff_bytes:\n        B ff_true\n'),
        ('sldn-partial', 'CMP r1, #1\n        BEQ sig_done', 'CMP r1, #1\n        BEQ sig_skip'),
        ('sldn-bound', '[r0, #3]', '[r0, #4]'),
        ('sldn-overlap', 'ADDS r0, r0, #1', 'ADDS r0, r0, #2'),
        ('sldn-letter', 'CMP r2, #68', 'CMP r2, #69'),
    ]
    detected = []
    for label, old, new in faults:
        assert old in source, label
        folder = out/label
        folder.mkdir()
        mutant = folder/'mutant.s'
        mutant.write_text(source.replace(old, new, 1), encoding='ascii')
        path = build(folder, production=mutant)
        try:
            check(path, False, True, 'prod_')
        except (AssertionError, UcError):
            detected.append(label)
        else:
            raise AssertionError('undetected mutation: '+label)
    return detected


def measure(path):
    result = {}
    samples = [('fnv10', 'id', b'0123456789', None),
        *[('gf2', str(x), list(range(16)), x) for x in (0, 1, 0x8000, 0xaaaa, 0xffff)],
        *[('all_ff', str(n), b'\xff'*n, None) for n in (0, 1, 10, 32, 64, 256, 4096)],
        ('all_ff', 'dirty_first', b'\0'+b'\xff'*255, None),
        *[('id_equal', 'diff_'+str(i), b'0123456789', b'0123456789' if i<0 else b'0123456789'[:i]+b'X'+b'0123456789'[i+1:]) for i in (-1, 0, 1, 4, 9)],
        *[('sldn', label, a, None) for label, a in [('garbage', b'X'*2048), ('valid', b'SLDN'+b'X'*16), ('S_only', b'S'*2048), ('partial', b'X'*2045+b'SLD')]]]
    for kind, label, a, b, align in [(*s, align) for s in samples for align in ((0, 2) if s[0] == 'gf2' else range(4))]:
        result[kind+'/'+label+('/align'+str(align) if align else '')] = row = {}
        for name in variants(kind):
            m, count, low = Machine(path), [0], [STACK]
            if name not in m.sym:
                continue
            args = arguments(m, kind, a, b, align, align)
            def hook(u, p, n, d):
                count[0] += 1
                low[0] = min(low[0], u.reg_read(UC_ARM_REG_SP))
            m.uc.hook_add(UC_HOOK_CODE, hook)
            value = m.call(name, *args, check_abi=True)
            assert value == reference(kind, a, b), (name, value)
            row[name] = {'instructions': count[0], 'stack_bytes': STACK-low[0]}
    for n in (0, 1, 16000, 1601536, 0xffffffff):
        row = result['crc_ff/'+str(n)] = {}
        for name in variants('gf2'):
            row[name] = count_call(path, 'ff_'+name, (n, 0xffff))
            assert row[name]['result'] == ff_reference(n, 0xffff)
    with open(path, 'rb') as f:
        elf = ELFFile(f)
        result['symbol_bytes'] = {s.name: s['st_size'] for s in elf.get_section_by_name('.symtab').iter_symbols()
                                  if s.name.startswith(('fast_', 'size_', 'speed_')) and s['st_info']['type'] == 'STT_FUNC'}
    return result


def footprints(out):
    """Retain one function: include literal pools and any runtime-library callees."""
    result = {}
    for kind in KINDS:
        for name in variants(kind):
            elfpath = out/'one.out'
            run([IAR/'ilinkarm.exe', out/'size_.o', out/'speed_.o', out/'asm.o', out/'prod.o',
                 '--config', ROOT/'Tests/asm/asm_link.icf', '--use_optimized_variants', 'small',
                 '--no_entry', '--keep', name, '-o', elfpath])
            with elfpath.open('rb') as f:
                result[name] = sum(s['p_filesz'] for s in ELFFile(f).iter_segments() if s['p_type'] == 'PT_LOAD')
    return result


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--json', type=Path, required=True)
    p.add_argument('--firmware', type=Path)
    p.add_argument('--measure-only', action='store_true')
    p.add_argument('--mutations', action='store_true')
    args = p.parse_args()
    project = ET.parse(ROOT/'DomovoiNext.ewp').getroot()
    for config in project.findall('configuration'):
        defines = [s.text for o in config.findall('.//option') if o.findtext('name') == 'CCDefines' for s in o.findall('state')]
        assert 'ASM_FAST_KERNELS' in defines, config.findtext('name')
    assert 'ASM_FAST_KERNELS' not in (ROOT/'Bootloader/DomovoiBoot.ewp').read_text()
    results = {'units': 'Unicorn executed instructions, NOT board cycles or time; function entry through return'}
    with tempfile.TemporaryDirectory(prefix='domovoi-fast-') as tmp:
        path = build(Path(tmp))
        results['measurements'] = measure(path)
        results['retained_flash_bytes'] = footprints(Path(tmp))
        if not args.measure_only:
            results['checks'] = check(path)
            results['c_guard_page_checks'] = guarded_c_bounds(path)
            results['bounds_abi'] = check(path, False, True, 'prod_')
            results['exhaustive_gf2'] = exhaustive_gf2(path, 'prod_')
        if args.mutations:
            results['mutations_detected'] = mutations(Path(tmp))
    if args.firmware:
        results['linked_checks'] = check(args.firmware, False, True)
        results['linked_exhaustive_gf2'] = exhaustive_gf2(args.firmware)
        results['elf_sha256'] = hashlib.sha256(args.firmware.read_bytes()).hexdigest()
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print('PASS fast kernels:', {k: v for k, v in results.items() if k != 'measurements'})


if __name__ == '__main__':
    main()
