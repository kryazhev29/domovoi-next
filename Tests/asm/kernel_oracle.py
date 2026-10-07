"""Независимый эталон CRC по FF и подсчёт инструкций ядер."""
from test_asm_crc import Machine, BUF, STACK, UC_HOOK_CODE, UC_ARM_REG_SP

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

def crc16(data, c=65535):
    for b in data:
        c ^= b
        for _ in range(8):
            c = (c >> 1) ^ (0xa001 if c & 1 else 0)
    return c

def ff_reference(n, value):
    """Независимое возведение аффинного преобразования CRC в степень."""
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
