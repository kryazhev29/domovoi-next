"""Свежий ELF IAR; IRQ вызывается на границе инструкции с учётом PRIMASK."""
import pathlib, struct, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
for deps in (ROOT/'Release/photo_test_deps',ROOT.parents[1]/'СофтЭксперт/Для новой системы/Domovoi Next_IAR95/Domovoi Next_IAR95/Release/photo_test_deps'):
    if deps.exists():sys.path.insert(0,str(deps))
from elftools.elf.elffile import ELFFile
from elftools.dwarf.dwarf_expr import DWARFExprParser
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.arm_const import *

class Machine:
    def __init__(self):
        self.symbols={}
        self.members={}
        self.volatile={}
        with (ROOT/'Release/Exe/DomovoiNext.out').open('rb') as f:
            elf=ELFFile(f)
            for s in elf.get_section_by_name('.symtab').iter_symbols():
                self.symbols[s.name]=s['st_value']
            for cu in elf.get_dwarf_info().iter_CUs():
                for die in cu.iter_DIEs():
                    a=die.attributes.get('DW_AT_name')
                    if a and a.value in (b'milliSeconds',b'timer') and die.tag=='DW_TAG_variable' and 'DW_AT_type' in die.attributes:
                        item=die.get_DIE_from_attribute('DW_AT_type');qualified=False
                        for _ in range(6):
                            if item.tag=='DW_TAG_volatile_type':qualified=True;break
                            if 'DW_AT_type' not in item.attributes:break
                            item=item.get_DIE_from_attribute('DW_AT_type')
                        self.volatile[a.value.decode()]=qualified
                    if not a or a.value!=b'uart_t' or die.tag!='DW_TAG_typedef':continue
                    for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                        if 'DW_AT_name' not in member.attributes:continue
                        offset=member.attributes['DW_AT_data_member_location'].value
                        if not isinstance(offset,int):offset=DWARFExprParser(cu.structs).parse_expr(offset)[0].args[0]
                        self.members[member.attributes['DW_AT_name'].value.decode()]=offset
        self.u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
        self.u.mem_map(0x08000000,0x20000);self.u.mem_write(0x08008200,(ROOT/'Release/Exe/DomovoiNext.bin').read_bytes())
        self.u.mem_map(0x20000000,0x5000);self.u.mem_map(0x21000000,0x1000)
        self.u.mem_map(0x40000000,0x30000);self.u.mem_map(0xE000E000,0x1000)
        self.top=self.addr('CSTACK$$Limit');self.bottom=self.addr('CSTACK$$Base')
        self.stop=0x0801fff0;self.steps=0;self.minimum=self.top;self.in_irq=False;self.inject_at=None;self.pending=False;self.paused=False;self.irq=None;self.stubs={}
        self.u.reg_write(UC_ARM_REG_XPSR,0x01000000)
        self.u.hook_add(UC_HOOK_CODE,self.hook)
    def addr(self,n):return self.symbols.get(n,self.symbols.get('_A_'+n)) & ~1
    def ret(self,value=0):
        self.u.reg_write(UC_ARM_REG_R0,value);self.u.reg_write(UC_ARM_REG_PC,self.u.reg_read(UC_ARM_REG_LR))
    def hook(self,u,a,n,_):
        self.minimum=min(self.minimum,u.reg_read(UC_ARM_REG_SP))
        assert self.minimum>=self.bottom,('stack overflow',hex(self.minimum))
        if a in self.stubs:self.stubs[a]();return
        if self.in_irq:return
        if self.inject_at==self.steps:self.pending=True;self.inject_at=None
        if self.pending and not u.reg_read(UC_ARM_REG_PRIMASK):
            self.pending=False;self.paused=True;u.emu_stop();return
        self.steps+=1
    def invoke_irq(self):
        saved=self.u.context_save();sp=self.u.reg_read(UC_ARM_REG_SP)
        self.in_irq=True
        # Модель аппаратного сохранения восьми регистров Cortex-M3.
        self.u.reg_write(UC_ARM_REG_SP,(sp-32)&~7)
        name,args=self.irq
        for r,v in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args):self.u.reg_write(r,v)
        self.u.reg_write(UC_ARM_REG_LR,(self.stop-4)|1)
        self.u.emu_start(self.addr(name)|1,self.stop-4,count=200000)
        assert self.u.reg_read(UC_ARM_REG_PC)==self.stop-4,name
        self.u.context_restore(saved);self.in_irq=False
    def call(self,name,*args,inject_at=None,irq=None,budget=500000):
        self.steps=0;self.minimum=self.top;self.inject_at=inject_at;self.irq=irq;self.pending=False;self.paused=False
        self.u.reg_write(UC_ARM_REG_SP,self.top);self.u.reg_write(UC_ARM_REG_LR,self.stop|1)
        for r,v in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args):self.u.reg_write(r,v)
        pc=self.addr(name)
        for _ in range(128):
            self.paused=False;self.u.emu_start(pc|1,self.stop,count=budget)
            if not self.paused:break
            pc=self.u.reg_read(UC_ARM_REG_PC);self.invoke_irq()
        assert self.u.reg_read(UC_ARM_REG_PC)==self.stop,(name,hex(self.u.reg_read(UC_ARM_REG_PC)))
        # IRQ, отложенное последней инструкцией восстановления PRIMASK.
        if self.pending and not self.u.reg_read(UC_ARM_REG_PRIMASK):self.pending=False;self.invoke_irq()
        return self.u.reg_read(UC_ARM_REG_R0)
    def init(self):
        self.u.reg_write(UC_ARM_REG_PRIMASK,0);self.call('__iar_data_init3')
    def write(self,address,value,size=4):self.u.mem_write(address,int(value).to_bytes(size,'little'))
    def read(self,address,size=4):return int.from_bytes(self.u.mem_read(address,size),'little')
    def uart(self,packet):
        self.init();base=self.addr('uart');rx=self.addr('gsm_buffer');self.u.mem_write(rx,packet)
        for n,v,size in (('rxbuffer',rx,4),('bfsize',2048,2),('received',len(packet),2),('timetowait',10,2),('twait',0,2),('state',1,1),('receiving',0,1)):
            self.write(base+self.members[n],v,size)
        return base,rx
