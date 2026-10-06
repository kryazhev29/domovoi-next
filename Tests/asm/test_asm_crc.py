"""Cortex-M3 CRC: IAR C/ASM equivalence, ABI, bounds and instruction counts.

Run with Release/test-venv/Scripts/python.exe. No board or peripheral access.
Counts are executed instructions in Unicorn, NOT measured hardware cycles.
"""
import argparse
import json
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zlib

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
    UC_ARM_REG_R3, UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
    UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11,
    UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC)

ROOT = Path(__file__).resolve().parents[2]
IAR = Path(os.environ.get('IAR_ARM_DIR', r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\arm')) / 'bin'
ARGS = (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)
SAVED = (UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
         UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11)
FUNCS = ('crc16_calc', 'crc16_calcInit', 'crc32_update', 'CRC32')
BUF, STOP, STACK = 0x10000000, 0x0FFFFFF0, 0x20004000


def function(text, name):
    m = re.search(r'(?m)^(?:static )?(?:uint32_t|unsigned short) ' + name + r'\([^;]*?\)\s*\{', text)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[m.start():end]


def run(cmd):
    p = subprocess.run([str(x) for x in cmd], cwd=ROOT, capture_output=True)
    if p.returncode:
        raise RuntimeError(p.stdout.decode('cp1251', errors='replace') + p.stderr.decode('cp1251', errors='replace'))


def nibble_reference():
    """Same table algorithm in C, to separate algorithm gain from assembly gain."""
    tables = []
    for name, poly in [('t16', 0xA001), ('t32', 0xEDB88320), ('tstm', 0x04C11DB7)]:
        values = []
        for i in range(16):
            if name == 'tstm':
                i <<= 28
            for _ in range(4):
                if name == 'tstm':
                    i = ((i << 1) ^ (poly if i & 0x80000000 else 0)) & 0xFFFFFFFF
                else:
                    i = (i >> 1) ^ (poly if i & 1 else 0)
            values.append(hex(i) + 'u')
        ctype = 'uint16_t' if name == 't16' else 'uint32_t'
        tables.append('static const ' + ctype + ' ' + name + '[] = {' + ','.join(values) + '};')
    return '#include <stdint.h>\n' + '\n'.join(tables) + '''
uint16_t nibble_crc16_calc(uint8_t *p, uint16_t n) {
  uint16_t c = 0xffff;
  while (n--) { c ^= *p++; c = (c >> 4) ^ t16[c & 15]; c = (c >> 4) ^ t16[c & 15]; }
  return c;
}
uint32_t nibble_crc32_update(uint32_t c, const uint8_t *p, uint16_t n) {
  while (n--) { c ^= *p++; c = (c >> 4) ^ t32[c & 15]; c = (c >> 4) ^ t32[c & 15]; }
  return c;
}
uint32_t nibble_CRC32(uint32_t c, uint32_t word) {
  c ^= word;
  for (unsigned i = 0; i < 8; i++) c = (c << 4) ^ tstm[c >> 28];
  return c;
}
'''


def build(out):
    source = lambda p: (ROOT / p).read_text(encoding='cp1251')
    journal = source('User/sector_journal.c')
    name = 'crc32_update' if 'uint32_t crc32_update(' in journal else 'sj_crc'
    c = function(journal, name).replace('static ', '').replace(name, 'crc32_update')
    c += '\n' + function(source('Tests/asm/reference/User/user_fun.c'), 'CRC32')
    (out / 'ref.c').write_text('#include <stdint.h>\n' + c, encoding='ascii')
    (out / 'nibble.c').write_text(nibble_reference(), encoding='ascii')
    objs = []
    for i, src in enumerate([ROOT / 'Tests/asm/reference/User/crc.c', out / 'ref.c', out / 'nibble.c']):
        obj = out / f'c{i}.o'
        run([IAR / 'iccarm.exe', src, '--cpu', 'Cortex-M3', '--thumb', '-Ohz', '-e',
             '-I', ROOT / 'User', *('-D' + f + '=ref_' + f for f in FUNCS), '-o', obj])
        objs.append(obj)
    fast = ['crc16_calc', 'crc32_update', 'CRC32']
    run([IAR / 'iccarm.exe', out / 'nibble.c', '--cpu', 'Cortex-M3', '--thumb', '-Ohs', '-e',
         *('-Dnibble_' + f + '=speed_' + f for f in fast), '-o', out / 'speed.o'])
    objs.append(out / 'speed.o')
    run([IAR / 'iasmarm.exe', ROOT / 'User/asm/crc_asm.s', '--cpu', 'Cortex-M3', '-o', out / 'asm.o'])
    keep = [*FUNCS, *('ref_' + f for f in FUNCS), *(p + f for f in fast for p in ('nibble_', 'speed_'))]
    run([IAR / 'ilinkarm.exe', *objs, out / 'asm.o', '--config', ROOT / 'Tests/asm/asm_link.icf',
         '--no_entry', '--keep', ','.join(keep), '--map', out / 'crc.map', '-o', out / 'crc.out'])
    return out / 'crc.out'


class Machine:
    def __init__(self, path):
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        self.uc.mem_map(0x08000000, 0x20000)
        self.uc.mem_map(0x20000000, 0x5000)
        self.uc.mem_map(0x40023000, 0x1000)
        from stm32_crc_model import CRCModel
        self.crc_model = CRCModel(self.uc)
        # Separate synthetic input region covers the full uint16_t length contract.
        self.uc.mem_map(BUF, 0x11000)
        self.uc.mem_map(STOP & ~0xFFF, 0x1000)
        with open(path, 'rb') as f:
            elf = ELFFile(f)
            for seg in elf.iter_segments():
                if seg['p_type'] == 'PT_LOAD' and seg['p_filesz']:
                    self.uc.mem_write(seg['p_paddr'], seg.data())
            self.sym = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
        self.checks = 0

    def call(self, name, *args, check_abi=False):
        for r, v in zip(ARGS, args):
            self.uc.reg_write(r, v)
        for i, r in enumerate(SAVED):
            self.uc.reg_write(r, 0x12340000 + i)
        self.uc.reg_write(UC_ARM_REG_SP, STACK)
        self.uc.reg_write(UC_ARM_REG_LR, STOP | 1)
        self.uc.emu_start(self.sym[name] | 1, STOP, count=20_000_000)
        assert self.uc.reg_read(UC_ARM_REG_PC) == STOP, name + ' did not return'
        if check_abi:
            assert self.uc.reg_read(UC_ARM_REG_SP) == STACK, name + ' changed SP'
            assert [self.uc.reg_read(r) for r in SAVED] == [0x12340000 + i for i in range(8)], name + ' broke ABI'
        return self.uc.reg_read(UC_ARM_REG_R0)

    def equal(self, name, args):
        result = self.call(name, *args, check_abi=True)
        assert result == self.call('ref_' + name, *args), (name, args)
        self.checks += 1
        return result


def cases(m):
    rng = random.Random(20261004)
    lengths = [0, 1, 2, 3, 7, 15, 16, 31, 32, 63, 64, 255, 256, 257, 1024, 2048, 4096, 65535]
    samples = [(bytes([v]) * n, n % 4) for n in lengths for v in (0, 0xFF, 0x55, 0x80)]
    samples += [(rng.randbytes(rng.randrange(600)), rng.randrange(4)) for _ in range(2000)]
    for data, align in samples:
        p, n, initial = BUF + align, len(data), rng.randrange(65536)
        m.uc.mem_write(p, data or b'\0')
        m.equal('crc16_calc', (p, n))
        m.equal('crc16_calcInit', (p, n, initial))
        initial32 = rng.getrandbits(32)
        value = m.equal('crc32_update', (initial32, p, n))
        assert value ^ 0xFFFFFFFF == zlib.crc32(data, initial32 ^ 0xFFFFFFFF)
        if n:
            split = rng.randrange(n)
            crc = m.call('crc16_calcInit', p, split, initial)
            assert m.call('crc16_calcInit', p + split, n - split, crc) == m.call('crc16_calcInit', p, n, initial)
            crc = m.call('crc32_update', initial32, p, split)
            assert m.call('crc32_update', crc, p + split, n - split) == value
    for _ in range(3000):
        m.equal('CRC32', (rng.getrandbits(32), rng.getrandbits(32)))
    m.uc.mem_write(BUF, b'123456789')
    assert m.call('crc16_calc', BUF, 9) == 0x4B37
    assert m.call('crc32_update', 0xFFFFFFFF, BUF, 9) ^ 0xFFFFFFFF == 0xCBF43926
    assert m.call('CRC32', 0xFFFFFFFF, 0x12345678) == 0xDF8A8A2B


def bounds(m):
    """Assembly must not touch the stack, write memory or read outside the input."""
    allowed = [0, 0]
    def read(uc, access, address, size, value, user):
        assert 0x08000000 <= address < 0x08020000 or allowed[0] <= address and address + size <= allowed[1], hex(address)
    def write(*args):
        raise AssertionError('CRC assembly wrote to memory')
    hooks = [m.uc.hook_add(UC_HOOK_MEM_READ, read), m.uc.hook_add(UC_HOOK_MEM_WRITE, write)]
    try:
        for n in (0, 1, 3, 16, 255, 2048, 65535):
            for a in range(4):
                p = BUF + a if n else 0
                allowed[:] = [p, p + n]
                for name, args in [('crc16_calc', (p, n)), ('crc16_calcInit', (p, n, 0x1234)),
                                   ('crc32_update', (0x87654321, p, n)), ('CRC32', (0x87654321, 0x12345678))]:
                    m.call(name, *args, check_abi=True)
    finally:
        for h in hooks:
            m.uc.hook_del(h)


def benchmark(m):
    m.uc.mem_write(BUF, random.Random(7).randbytes(2048))
    counts, rows = [0], []
    def count(*args):
        counts[0] += 1
    hook = m.uc.hook_add(UC_HOOK_CODE, count)
    try:
        for f, args in [('crc16_calc', (BUF, 2048)), ('crc32_update', (0xFFFFFFFF, BUF, 2048)),
                        ('CRC32', (0xFFFFFFFF, 0x12345678))]:
            row = {'function': f}
            names = [('C_bitwise', 'ref_' + f), ('ASM', f)]
            names += [('C_nibble_size', 'nibble_' + f), ('C_nibble_speed', 'speed_' + f)]
            results = []
            for label, name in names:
                counts[0] = 0
                results.append(m.call(name, *args))
                row[label] = counts[0]
            assert len(set(results)) == 1, row
            assert row['ASM'] < row['C_bitwise'], row
            assert row['ASM'] <= row['C_nibble_speed'], row
            row['instructions_reduction'] = round(row['C_bitwise'] / row['ASM'], 2)
            rows.append(row)
    finally:
        m.uc.hook_del(hook)
    return rows


def mutations(elf):
    # Force zero length (MOVS r1/r2,#0), or an early zero return (MOVS r0,#0; BX lr).
    for name, patch, args in [
        ('crc16_calcInit', b'\x00\x21', (BUF, 1, 0)),
        ('crc32_update', b'\x00\x22', (0, BUF, 1)),
        ('CRC32', b'\x00\x20\x70\x47', (0, 0x10000000)),
    ]:
        m = Machine(elf)
        m.uc.mem_write(BUF, b'\x01')
        m.uc.mem_write(m.sym[name] & ~1, patch)
        try:
            m.equal(name, args)
        except AssertionError:
            continue
        raise AssertionError('undetected machine-code mutation: ' + name)


def linked_firmware(path, reference):
    actual, rng = Machine(path), random.Random(17)
    checks = 0
    for i in range(250):
        data = rng.randbytes(rng.randrange(2049))
        pointer, initial = BUF + i % 4, rng.getrandbits(32)
        actual.uc.mem_write(pointer, data or b'\0')
        reference.uc.mem_write(pointer, data or b'\0')
        for name, args in [('crc16_calc', (pointer, len(data))),
                           ('crc16_calcInit', (pointer, len(data), initial & 0xFFFF)),
                           ('crc32_update', (initial, pointer, len(data))),
                           ('CRC32', (initial, rng.getrandbits(32)))]:
            assert actual.call(name, *args, check_abi=True) == reference.call('ref_' + name, *args), name
            checks += 1
    bounds(Machine(path))
    return checks


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--json', type=Path)
    p.add_argument('--firmware', type=Path, help='Also check kernels relocated into the linked application ELF')
    args = p.parse_args()
    project = ET.parse(ROOT / 'DomovoiNext.ewp').getroot()
    for config in project.findall('configuration'):
        defines = [s.text for o in config.findall('.//option') if o.findtext('name') == 'CCDefines' for s in o.findall('state')]
        assert 'ASM_CRC' in defines, config.findtext('name')
    assert any(f.findtext('name', '').endswith('User\\asm\\crc_asm.s') for f in project.findall('.//file'))
    with tempfile.TemporaryDirectory(prefix='domovoi-asm-crc-') as t:
        elf = build(Path(t))
        m = Machine(elf)
        cases(m)
        # Fresh engines ensure all translations are made with the active hooks.
        bounds(Machine(elf))
        rows = benchmark(Machine(elf))
        mutations(elf)
        result = {'differential_cases': m.checks, 'mutations_detected': 3,
                  'unit': 'Unicorn executed instructions; NOT hardware cycles', 'benchmarks': rows}
        if args.firmware:
            result['linked_firmware_cases'] = linked_firmware(args.firmware, m)
        if args.json:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(result))
        print('asm CRC: equivalence, zlib, known answers, streaming, ABI, read bounds, no writes PASS')


if __name__ == '__main__':
    main()
