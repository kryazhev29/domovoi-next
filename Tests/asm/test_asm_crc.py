"""Дифференциальный тест: User/asm/crc_asm.s против эталона User/crc.c (IAR -Ohz).
Собирает оба варианта в один ELF, гоняет в эмуляторе на тысячах входов, печатает размер и такты."""
import pathlib, random, subprocess, sys, tempfile
HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
from asm_bench import Machine
BIN = pathlib.Path(r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\arm\bin')
FUNCS = ('crc16_calc', 'crc16_calcInit', 'crc8_calc')

def build(tmp):
    ref = [f'-D{f}=ref_{f}' for f in FUNCS]
    subprocess.run([str(BIN / 'iccarm.exe'), str(ROOT / 'User/crc.c'), '--cpu', 'Cortex-M3', '--thumb', '-Ohz', '-e',
                    '-I', str(ROOT / 'User'), *ref, '-o', str(tmp / 'c.o')], check=True, capture_output=True)
    subprocess.run([str(BIN / 'iasmarm.exe'), str(ROOT / 'User/asm/crc_asm.s'), '--cpu', 'Cortex-M3',
                    '-o', str(tmp / 'a.o')], check=True, capture_output=True)
    keep = ','.join([*FUNCS, *('ref_' + f for f in FUNCS)])
    subprocess.run([str(BIN / 'ilinkarm.exe'), str(tmp / 'c.o'), str(tmp / 'a.o'), '--config', str(HERE / 'asm_link.icf'),
                    '--no_entry', '--keep', keep, '--map', str(tmp / 'm.map'), '-o', str(tmp / 'x.out')],
                   check=True, capture_output=True)
    return tmp / 'x.out'

def main():
    random.seed(7)
    with tempfile.TemporaryDirectory() as t:
        tmp = pathlib.Path(t)
        m = Machine(str(build(tmp)))
        buf = 0x20001000
        n_cases = 0
        for _ in range(3000):
            n = random.choice([0, 1, 2, 3, 7, 16, 255, random.randrange(600)])
            data = bytes(random.randrange(256) for _ in range(n))
            init = random.randrange(0x10000)
            m.uc.mem_write(buf, data + b'\0')
            for f, args, mask in (('crc16_calc', (buf, n), 0xFFFF), ('crc16_calcInit', (buf, n, init), 0xFFFF),
                                  ('crc8_calc', (buf, n & 0xFF), 0xFF)):
                a = m.call(f, *args)[0] & mask
                r = m.call('ref_' + f, *args)[0] & mask
                assert a == r, (f, n, hex(a), hex(r))
                n_cases += 1
        data = bytes(random.randrange(256) for _ in range(2048))
        m.uc.mem_write(buf, data)
        _, _, ca = m.call('crc16_calc', buf, 2048)
        _, _, cr = m.call('ref_crc16_calc', buf, 2048)
        text = (tmp / 'm.map').read_text(errors='replace')
    print(f'asm crc: {n_cases} cases equal; crc16 2048 B: C ~{cr} cycles, asm ~{ca} cycles; PASS')

if __name__ == '__main__':
    main()
