"""CPU instruction comparisons, NOT cycle/time measurements on a board.

Uses real IAR output; full-firmware photo I/O and watchdog are stubbed.
Доработка: Кряжев А., 2026.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import tempfile
import zlib
import xml.etree.ElementTree as ET

from test_asm_crc import (ROOT, IAR, Machine, function, run, BUF, ARGS, STACK, UC_ARM_REG_SP,
    UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_ARM_REG_LR,
    UC_ARM_REG_PC)


def build(out, assembly=None):
    source = (ROOT / 'User/idindex.c').read_text(encoding='cp1251')
    c = '#include <stdint.h>\n' + function(source, 'ix_fnv') + '\n' + function(source, 'idindex_hash')
    c += '\nuint32_t unrolled_hash(uint16_t index, const uint8_t *id) {\n'
    c += 'uint32_t h = (2166136261u ^ (uint8_t)index) * 16777619u;\n'
    c += 'h = (h ^ (index >> 8)) * 16777619u;\n'
    c += ''.join('h = (h ^ id[%d]) * 16777619u;\n' % i for i in range(10))
    c += 'return h; }\n'
    # Portable, alias-safe word grouping. No unaligned uint32_t pointer casts.
    c += '''
uint32_t blank_bytes(const uint8_t *p, uint16_t n) {
  while(n--) if(*p++ != 255) return 0; return 1;
}
uint32_t blank_words(const uint8_t *p, uint16_t n) {
  while(n >= 4) {
    uint32_t v = p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
    if(v != 0xffffffffu) return 0; p += 4; n -= 4;
  }
  while(n--) if(*p++ != 255) return 0; return 1;
}
uint32_t id_equal(const uint8_t *a, const uint8_t *b) {
  for(unsigned i=0; i<10; i++) if(a[i]!=b[i]) return 0; return 1;
}
'''
    (out / 'ref.c').write_text(c, encoding='ascii')
    names = ('idindex_hash', 'unrolled_hash', 'blank_bytes', 'blank_words', 'id_equal')
    objs, keep = [], ['idindex_hash']
    for prefix, opt in [('size_', '-Ohz'), ('speed_', '-Ohs')]:
        obj = out / (prefix + '.o')
        run([IAR / 'iccarm.exe', out / 'ref.c', '--cpu', 'Cortex-M3', '--thumb', opt, '-e',
             *('-D' + n + '=' + prefix + n for n in names), '-o', obj])
        objs.append(obj)
        keep.extend(prefix+n for n in names)
    run([IAR / 'iasmarm.exe', assembly or ROOT / 'User/asm/id_hash_asm.s', '--cpu', 'Cortex-M3', '-o', out / 'asm.o'])
    run([IAR / 'ilinkarm.exe', *objs, out / 'asm.o', '--config', ROOT / 'Tests/asm/asm_link.icf',
         '--no_entry', '--keep', ','.join(keep), '--map', out / 'scan.map', '-o', out / 'scan.out'])
    return out / 'scan.out'


def fnv(index, data):
    h = 2166136261
    for b in index.to_bytes(2, 'little') + data:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


def hash_cases(path, names=('idindex_hash',)):
    m, rng, checks = Machine(path), random.Random(20261005), 0
    for i in range(4096):
        index = [0, 1, 255, 256, 50047, 65534, 65535][i % 7] if i < 28 else rng.randrange(65536)
        data = bytes([0 if i < 4 else 255])*10 if i < 8 else rng.randbytes(10)
        ptr = BUF + i % 4
        m.uc.mem_write(ptr, data)
        expected = fnv(index, data)
        for name in names:
            assert m.call(name, index, ptr, check_abi=True) == expected, (name, i)
            checks += 1
    # Fresh engines: hooks must be installed before translated blocks exist.
    for align in range(4):
        m = Machine(path)
        ptr = BUF + align
        m.uc.mem_write(ptr, b'0123456789')
        reads, writes = [], []
        m.uc.hook_add(UC_HOOK_MEM_READ, lambda u, a, p, n, v, d: reads.append((p, n)))
        m.uc.hook_add(UC_HOOK_MEM_WRITE, lambda u, a, p, n, v, d: writes.append((p, n)))
        assert m.call('idindex_hash', 0xabcd, ptr, check_abi=True) == fnv(0xabcd, b'0123456789')
        assert not writes, writes
        data_reads = [(p, n) for p, n in reads if BUF <= p < BUF + 0x11000]
        assert data_reads == [(ptr+i, 1) for i in range(10)], data_reads
    return checks


def count_call(path, name, args, data=b'\xff'*256):
    m, count, min_sp = Machine(path), [0], [STACK]
    m.uc.mem_write(BUF, data)
    m.uc.mem_write(BUF+0x10000, data)
    def hook(u, address, size, unused):
        count[0] += 1
        min_sp[0] = min(min_sp[0], u.reg_read(UC_ARM_REG_SP))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    value = m.call(name, *args, check_abi=True)
    return {'instructions': count[0], 'result': value, 'stack_bytes': STACK-min_sp[0]}


def photo(path, n):
    m, counts = Machine(path), {'instructions': 0, 'reads': 0, 'read_bytes': 0, 'watchdog': 0}
    data = bytes((i*37+11)&255 for i in range(n))
    read, wdt = m.sym['mem_read'] & ~1, m.sym['wdt_reload'] & ~1
    def hook(u, address, size, unused):
        counts['instructions'] += 1
        if address == read:
            src, dst, length = [u.reg_read(r) for r in ARGS[:3]]
            assert src + length <= len(data)
            u.mem_write(dst, data[src:src+length])
            u.reg_write(ARGS[0], 1)
            counts['reads'] += 1
            counts['read_bytes'] += length
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
        elif address == wdt:
            counts['watchdog'] += 1
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    assert m.call('photo_backup_crc', 0, n, BUF, check_abi=True) == 1
    assert int.from_bytes(m.uc.mem_read(BUF, 4), 'little') == zlib.crc32(data) ^ 0xffffffff
    return counts


def scan(path, data):
    m, result = Machine(path), {'instructions': 0, 'move_calls': 0, 'move_bytes': 0}
    ptr = m.sym['server_rx_buffer']
    m.uc.mem_write(ptr, data)
    m.uc.mem_write(m.sym['server_rx_count'], len(data).to_bytes(2, 'little'))
    move = m.sym['__iar_small_memmove'] & ~1
    def hook(u, address, size, unused):
        result['instructions'] += 1
        if address == move:
            result['move_calls'] += 1
            result['move_bytes'] += u.reg_read(ARGS[2])
    m.uc.hook_add(UC_HOOK_CODE, hook)
    m.call('server_rx_scan', check_abi=True)
    result['left'] = int.from_bytes(m.uc.mem_read(m.sym['server_rx_count'], 2), 'little')
    result['ready'] = int.from_bytes(m.uc.mem_read(m.sym['server_rx_ready'], 2), 'little')
    result['tail_hex'] = bytes(m.uc.mem_read(ptr, result['left'])).hex()
    return result


def crc16(data, c=65535):
    for b in data:
        c ^= b
        for _ in range(8):
            c = (c >> 1) ^ (0xa001 if c & 1 else 0)
    return c


def ff_reference(n, value):
    """Independent affine exponentiation using columns and a bias."""
    def apply(columns, v):
        out = 0
        for i in range(16):
            if v & (1 << i):
                out ^= columns[i]
        return out
    bias = crc16(b'\xff', 0)
    columns = [crc16(b'\xff', 1 << i) ^ bias for i in range(16)]
    while n:
        if n & 1:
            value = apply(columns, value) ^ bias
        bias = apply(columns, bias) ^ bias
        columns = [apply(columns, c) for c in columns]
        n >>= 1
    return value


def ff_cases(path):
    m, rng = Machine(path), random.Random(20261005)
    lengths = [0, 1, 2, 3, 255, 256, 65535, 65536, 1601536, 0x7fffffff, 0x80000000, 0xffffffff]
    samples = [(n, seed) for n in lengths for seed in (0, 1, 0xffff, 0x1234, 0x8000)]
    samples += [(rng.getrandbits(32), rng.getrandbits(16)) for _ in range(140)]
    for n, seed in samples:
        assert m.call('crc16_ffInit', n, seed, check_abi=True) == ff_reference(n, seed), (n, seed)
    return len(samples)


def find_id(path, n, match, common_prefix=False):
    """Real linked fallback, simulated Flash reads; index forced unavailable."""
    m = Machine(path)
    result = {'instructions': 0, 'reads': 0, 'read_bytes': 0}
    query = b'7900000007'
    flash = bytearray((n+31)//32*1024)
    for i in range(n):
        flash[i*32:i*32+2] = (i+1).to_bytes(2, 'little')
        flash[i*32+2:i*32+12] = b'7900000008' if common_prefix else b'1234567890'
    if match >= 0:
        flash[match*32+2:match*32+12] = query
    base = 0x41000
    # structures.h: MB_ID=2, cellMem_s={uint32_t start,size}; cellID_s=32 bytes.
    m.uc.mem_write(m.sym['mem']+2*8, base.to_bytes(4, 'little')+len(flash).to_bytes(4, 'little'))
    m.uc.mem_write(m.sym['id_cells_end'], n.to_bytes(2, 'little'))
    m.uc.mem_write(BUF, query)
    m.uc.mem_write(BUF+64, b'\x5a'*32)
    read, find = m.sym['mem_read'] & ~1, m.sym['idindex_find'] & ~1
    def hook(u, address, size, unused):
        result['instructions'] += 1
        if address == read:
            src, dst, length = [u.reg_read(r) for r in ARGS[:3]]
            src -= base
            assert 0 <= src and src + length <= len(flash)
            u.mem_write(dst, bytes(flash[src:src+length]))
            u.reg_write(ARGS[0], 1)
            result['reads'] += 1
            result['read_bytes'] += length
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
        elif address == find:
            u.reg_write(ARGS[0], 0xffffffff)
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    value = m.call('user_findID', BUF, BUF+64, check_abi=True)
    assert value == (match >= 0)
    assert bytes(m.uc.mem_read(BUF+64, 32)) == (bytes(flash[match*32:match*32+32]) if value else b'\x5a'*32)
    return result


def mutations(out):
    source = (ROOT / 'User/asm/id_hash_asm.s').read_text(encoding='cp1251')
    detected = []
    for name, old, new in [
        ('prime', '=16777619', '=16777617'),
        ('index_byte_order', 'r0, r0, #8', 'r0, r0, #7'),
        ('last_id_byte', '[r1, #9]', '[r1, #8]')]:
        assert source.count(old) == 1
        asm = out / 'mutant.s'
        asm.write_bytes(source.replace(old, new).encode('cp1251'))
        path = build(out, asm)
        m = Machine(path)
        m.uc.mem_write(BUF, b'0123456789')
        assert m.call('idindex_hash', 0xabcd, BUF) != fnv(0xabcd, b'0123456789'), name
        detected.append(name)
    return detected


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--firmware', type=Path)
    parser.add_argument('--json', type=Path, required=True)
    args = parser.parse_args()
    project = ET.parse(ROOT / 'DomovoiNext.ewp').getroot()
    for config in project.findall('configuration'):
        defines = [s.text for o in config.findall('.//option') if o.findtext('name') == 'CCDefines' for s in o.findall('state')]
        assert 'ASM_ID_HASH' in defines, config.findtext('name')
    assert 'ASM_ID_HASH' not in (ROOT / 'Bootloader/DomovoiBoot.ewp').read_text()
    results = {'units': 'Unicorn executed instructions, not hardware cycles; photo I/O stubbed'}
    with tempfile.TemporaryDirectory(prefix='domovoi-perf-') as tmp:
        path = build(Path(tmp))
        names = ('idindex_hash', 'size_idindex_hash', 'speed_idindex_hash', 'size_unrolled_hash', 'speed_unrolled_hash')
        results['hash_checks'] = hash_cases(path, names)
        results['hash'] = {n: count_call(path, n, (1234, BUF)) for n in names}
        results['blank'] = {p+n: count_call(path, p+n, (BUF, 256))
            for n in ('blank_bytes', 'blank_words') for p in ('size_', 'speed_')}
        results['blank_dirty_first'] = {p+n: count_call(path, p+n, (BUF, 256), b'\0'+b'\xff'*255)
            for n in ('blank_bytes', 'blank_words') for p in ('size_', 'speed_')}
        results['id_equal'] = {p: count_call(path, p+'id_equal', (BUF, BUF+0x10000))
            for p in ('size_', 'speed_')}
        results['hash_mutations_detected'] = mutations(Path(tmp))
    if args.firmware:
        results['linked_hash_checks'] = hash_cases(args.firmware)
    for label, path in [('baseline', args.baseline), ('current', args.firmware)]:
        if not path:
            continue
        results[label] = {'photo': {n: photo(path, n) for n in (0, 1, 64, 65, 4095)}}
        results[label]['elf_sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
        results[label]['hash'] = count_call(path, 'idindex_hash', (1234, BUF))
        results[label]['ff_checks'] = ff_cases(path)
        results[label]['ff'] = {str(n): count_call(path, 'crc16_ffInit', (n, 0xffff))
            for n in (0, 1, 16000, 1601536, 0xffffffff)}
        bad = b'SLDN\xf8\x07' + b'\xa5'*2042
        assert crc16(bad[4:-2]) != int.from_bytes(bad[-2:], 'little')
        frame = b'SLDN\x0c\x00'+b'\xa5'*12
        frame += crc16(frame[4:]).to_bytes(2, 'little')
        results[label]['scan'] = {n: scan(path, data) for n, data in {
            'bad_full': bad, 'bad_partial_tail': bad[:-3]+b'SLD',
            'garbage': b'X'*2048, 'valid': frame, 'partial': frame[:5]}.items()}
        results[label]['lib_id_equal'] = count_call(path, '__iar_small_memcmp', (BUF, BUF+0x10000, 10))
        results[label]['find_id'] = {str(n)+'/'+str(match): find_id(path, n, match)
            for n, match in ((32, -1), (1024, -1), (1024, 0), (1024, 511), (1024, 1023))}
        results[label]['find_id']['1024/-1/common_prefix'] = find_id(path, 1024, -1, True)
    if args.baseline and args.firmware:
        for n, old in results['baseline']['ff'].items():
            assert old['result'] == results['current']['ff'][n]['result'], n
        for case, old in results['baseline']['scan'].items():
            new = results['current']['scan'][case]
            for key in ('left', 'ready', 'tail_hex'):
                assert old[key] == new[key], (case, key)
        for length, old in results['baseline']['photo'].items():
            new = results['current']['photo'][length]
            for key in ('reads', 'read_bytes', 'watchdog'):
                assert old[key] == new[key], (length, key)
        for case, old in results['baseline']['find_id'].items():
            new = results['current']['find_id'][case]
            for key in ('reads', 'read_bytes'):
                assert old[key] == new[key], (case, key)
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in results.items() if k not in ('baseline', 'current')}, indent=2))
    print('Full firmware comparisons:', ', '.join(k for k in ('baseline', 'current') if k in results))


if __name__ == '__main__':
    main()
