"""Actual baseline/current ELF callers with modeled NOR reads; no device access."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import tempfile

from test_asm_crc import (Machine, BUF, STACK, ARGS, UC_HOOK_CODE, UC_ARM_REG_SP,
                          UC_ARM_REG_LR, UC_ARM_REG_PC, build as build_crc, linked_firmware)
from test_fast_kernels import reference
from test_perf_scan import scan, crc16, ff_cases, hash_cases, count_call, photo, find_id


def indexed(path, query, scenario):
    m = Machine(path)
    # ix layout: sum:u32, step/end/mark:u16, state:u8; IX_READY=3.
    m.uc.mem_write(m.sym['ix']+10, b'\x03')
    m.uc.mem_write(BUF, query)
    m.uc.mem_write(BUF+32, b'\xa5'*32)
    h = reference('fnv10', query)
    bucket = 0x300000 + ((h ^ (h >> 12) ^ (h >> 24)) & 4095)*64
    cell = b'\x07\x00'+query+b'\x89'*20
    entries = b'\x07\x00'+b'\xff'*62
    if scenario == 'empty':
        entries = b'\xff'*64
    if scenario == 'mismatch':
        cell = cell[:11]+bytes([cell[11]^1])+cell[12:]
    if scenario == 'deleted':
        cell = b'\xff\xff'+cell[2:]
    result = {'instructions': 0, 'reads': [], 'stack_bytes': 0}
    def hook(u, p, n, d):
        result['instructions'] += 1
        result['stack_bytes'] = max(result['stack_bytes'], STACK-u.reg_read(UC_ARM_REG_SP))
        if p == (m.sym['mem_read'] & ~1):
            a, dst, length = [u.reg_read(r) for r in ARGS[:3]]
            result['reads'].append([a, length])
            assert (a, length) in ((bucket, 64), (0x41000+6*32, 32)), (hex(a), length, hex(bucket))
            fail = (a == bucket and scenario == 'bucket_fault') or (a != bucket and scenario == 'cell_fault')
            if not fail:
                u.mem_write(dst, entries if a == bucket else cell)
            u.reg_write(ARGS[0], int(not fail))
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    value = m.call('idindex_find', 0x41000, BUF, BUF+32, check_abi=True)
    expected = 0xffffffff if 'fault' in scenario else int(scenario == 'match')
    # IAR may return a sign-extended or low-byte int8_t.
    assert value & 255 == expected & 255, (scenario, value)
    result['value'] = value & 255
    result['cell'] = bytes(m.uc.mem_read(BUF+32, 32)).hex()
    return result


def blank_sector(path, dirty=-1, fault=-1):
    m = Machine(path)
    result = {'instructions': 0, 'reads': [], 'stack_bytes': 0}
    data = bytearray(b'\xff'*4096)
    if dirty >= 0:
        data[dirty] ^= 0x80
    def hook(u, p, n, d):
        result['instructions'] += 1
        result['stack_bytes'] = max(result['stack_bytes'], STACK-u.reg_read(UC_ARM_REG_SP))
        if p == (m.sym['w25_read_data'] & ~1):
            a, dst, length = [u.reg_read(r) for r in ARGS[:3]]
            off = a-0x1cd000
            assert 0 <= off and off+length <= len(data) and length == 64
            result['reads'].append([off, length])
            if off != fault:
                u.mem_write(dst, bytes(data[off:off+length]))
            u.reg_write(ARGS[0], int(off != fault))
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    result['value'] = m.call('cfg_blank_sector', 0x1cd000, check_abi=True)
    assert result['value'] == int(dirty < 0 and fault < 0)
    return result


def scan_checks(old, new):
    rng = random.Random(607)
    samples = [b'', b'S', b'SL', b'SLD', b'SLDN', b'SSLDN', b'SLDSLDN', b'SLDX'*512]
    for _ in range(256):
        payload = rng.randbytes(rng.randrange(3, 150))
        frame = b'SLDN'+len(payload).to_bytes(2, 'little')+payload
        frame += crc16(frame[4:]).to_bytes(2, 'little')
        if rng.randrange(3) == 0:
            frame = frame[:-1]+bytes([frame[-1]^1])
        samples.append(rng.randbytes(rng.randrange(64))+frame[:rng.randrange(len(frame)+1)]+b'SLD'[:rng.randrange(4)])
    for data in samples:
        a, b = scan(old, data), scan(new, data)
        for key in ('left', 'ready', 'tail_hex'):
            assert a[key] == b[key], (data.hex(), key)
        assert b['move_calls'] <= 1
    return len(samples)


def scan_samples():
    bad = b'SLDN\xf8\x07'+b'\xa5'*2042
    frame = b'SLDN\x0c\x00'+b'\xa5'*12
    frame += crc16(frame[4:]).to_bytes(2, 'little')
    nested = bytearray(b'\xa5'*2048)
    offsets = range(0, 2038, 6)
    for off in offsets:
        nested[off:off+6] = b'SLDN'+(2040-off).to_bytes(2, 'little')
    rejected = {crc16(nested[off+4:-2]) for off in offsets}
    nested[-2:] = next(v for v in range(65536) if v not in rejected).to_bytes(2, 'little')
    return {'bad_full': bad, 'bad_partial_tail': bad[:-3]+b'SLD', 'garbage': b'X'*2048,
            'valid': frame, 'partial': frame[:5], 'nested_bad_crc': bytes(nested)}


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--firmware', type=Path, required=True)
    p.add_argument('--json', type=Path, required=True)
    args = p.parse_args()
    result = {'units': 'Unicorn instructions, not board cycles/time; NOR reads modeled'}
    scans = scan_samples()
    for label, path in (('before', args.baseline), ('after', args.firmware)):
        values = result[label] = {'elf_sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
        values['index'] = {s: indexed(path, b'7900000007', s) for s in ('match', 'empty', 'mismatch', 'deleted', 'bucket_fault', 'cell_fault')}
        rng = random.Random(1025)
        for i in range(128):
            indexed(path, rng.randbytes(10), 'match')
        values['index_random_checks'] = 128
        values['blank_sector'] = {str(d)+'/'+str(f): blank_sector(path, d, f)
                                  for d, f in ((-1,-1),(0,-1),(1,-1),(3,-1),(4,-1),(63,-1),(64,-1),(4095,-1),(-1,0),(-1,2048))}
        values['ff_checks'] = ff_cases(path)
        values['ff'] = {str(n): count_call(path, 'crc16_ffInit', (n, 0xffff)) for n in (0,1,16000,1601536,0xffffffff)}
        values['scan'] = {k: scan(path, data) for k, data in scans.items()}
        values['photo'] = {str(n): photo(path, n) for n in (0,1,64,65,4095)}
        values['find_id'] = {str(n)+'/'+str(match)+'/'+str(prefix): find_id(path,n,match,prefix)
                             for n,match,prefix in ((32,-1,False),(1024,-1,False),(1024,-1,True),(1024,0,False),(1024,511,False),(1024,1023,False))}
        values['log_blank'] = {}
        for pos in range(-1, 32):
            data = bytearray(b'\xff'*32)
            if pos >= 0:
                data[pos] ^= 0x80
            values['log_blank'][str(pos)] = count_call(path, 'log_blank', (BUF,), bytes(data))
            assert values['log_blank'][str(pos)]['result'] == int(pos < 0)
    for group in ('index', 'blank_sector'):
        for case, before in result['before'][group].items():
            after = result['after'][group][case]
            for key in ('reads', 'value', 'cell') if group == 'index' else ('reads', 'value'):
                assert before[key] == after[key], (group, case, key)
    for group, keys in (('ff', ('result',)), ('scan', ('left','ready','tail_hex')),
                        ('photo', ('reads','read_bytes','watchdog')), ('find_id', ('reads','read_bytes')),
                        ('log_blank', ('result','instructions','stack_bytes'))):
        for case, before in result['before'][group].items():
            for key in keys:
                assert before[key] == result['after'][group][case][key], (group, case, key)
    result['linked_scan_differential'] = scan_checks(args.baseline, args.firmware)
    result['linked_previous_hash_checks'] = hash_cases(args.firmware)
    with tempfile.TemporaryDirectory(prefix='domovoi-final-crc-') as temp:
        result['linked_previous_crc_checks'] = linked_firmware(args.firmware, Machine(build_crc(Path(temp))))
    args.json.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('PASS final ELF callers: index match/miss/deleted/read faults + 256 random buckets; blank/dirty/read faults; 264 differential scans')


if __name__ == '__main__':
    main()
