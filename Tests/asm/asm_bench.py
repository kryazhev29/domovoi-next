"""Прогон функций Cortex-M3 из ELF в эмуляторе: инструкции и оценка тактов.
Оценка тактов по таблице Cortex-M3 TRM: обычная 1, загрузка 2, LDM/POP 1+N,
взятый переход 1+2, деление 2..12 (берём 6)."""
import sys
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
                               UC_ARM_REG_R3, UC_ARM_REG_LR, UC_ARM_REG_SP)

STOP = 0x0FFFFFF0


class Machine:
    def __init__(self, path):
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        self.uc.mem_map(0x08000000, 0x40000)
        self.uc.mem_map(0x20000000, 0x10000)
        self.uc.mem_map(0x40000000, 0x40000)  # периферия как ОЗУ
        self.uc.mem_map(STOP & ~0xFFF, 0x1000)
        self.sym = {}
        with open(path, 'rb') as f:
            elf = ELFFile(f)
            for seg in elf.iter_segments():
                if seg['p_type'] == 'PT_LOAD' and seg['p_filesz']:
                    self.uc.mem_write(seg['p_paddr'], seg.data())
                    if seg['p_vaddr'] != seg['p_paddr']:
                        self.uc.mem_write(seg['p_vaddr'], seg.data())
            for s in elf.get_section_by_name('.symtab').iter_symbols():
                if s.name:
                    self.sym[s.name] = (s['st_value'], s['st_size'])
        self.cs = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
        self.uc.hook_add(UC_HOOK_CODE, self._hook)

    def _hook(self, uc, addr, size, _):
        if self.prev is not None:
            pa, ps, pm = self.prev
            if addr != pa + ps and pm.startswith(('b', 'cb', 'pop', 'ldm')) or (
                    addr != pa + ps and 'pc' in pm):
                self.cycles += 2
        code = bytes(uc.mem_read(addr, size))
        ins = next(self.cs.disasm(code, addr), None)
        m = ins.mnemonic if ins else '?'
        op = ins.op_str if ins else ''
        c = 1
        if m.startswith(('ldr', 'ldrb', 'ldrh', 'ldrsb', 'ldrsh')):
            c = 2
        elif m.startswith(('ldm', 'pop', 'push', 'stm')):
            c = 1 + op.count(',') + 1
        elif m.startswith(('udiv', 'sdiv')):
            c = 6
        self.cycles += c
        self.count += 1
        self.prev = (addr, size, m + (' pc' if 'pc' in op else ''))

    def call(self, name, *args):
        addr = self.sym[name][0] | 1
        regs = (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)
        for r, v in zip(regs, args):
            self.uc.reg_write(r, v)
        self.uc.reg_write(UC_ARM_REG_SP, 0x20008000)
        self.uc.reg_write(UC_ARM_REG_LR, STOP | 1)
        self.count = self.cycles = 0
        self.prev = None
        self.uc.emu_start(addr, STOP, count=50_000_000)
        return self.uc.reg_read(UC_ARM_REG_R0), self.count, self.cycles

    def size(self, name, nxt=None):
        a, s = self.sym[name]
        if s:
            return s
        return self.sym[nxt][0] - a if nxt else None
