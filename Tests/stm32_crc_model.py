"""Модель словного CRC STM32 F1: сброс, сохранение DR, полином 0x04c11db7 без отражения/XOR."""
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE


class CRCModel:
    def __init__(self, uc):
        self.value = 0xffffffff
        uc.hook_add(UC_HOOK_MEM_WRITE, self.write, begin=0x40023000, end=0x4002300b)
        uc.hook_add(UC_HOOK_MEM_READ, self.read, begin=0x40023000, end=0x4002300b)

    def write(self, uc, access, address, size, value, unused):
        if address == 0x40023008:
            if value & 1:
                self.value = 0xffffffff
        elif address == 0x40023000:
            assert size == 4, ('STM32 F1 CRC requires word writes', size)
            crc = self.value ^ value
            for _ in range(32):
                crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
            self.value = crc

    def read(self, uc, access, address, size, value, unused):
        if address == 0x40023000:
            uc.mem_write(address, self.value.to_bytes(4, 'little'))
        elif address == 0x40023008:
            uc.mem_write(address, bytes(4))
