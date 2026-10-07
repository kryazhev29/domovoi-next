from pathlib import Path
import argparse,subprocess,struct
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_THUMB,UC_MODE_MCLASS,UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_SP,UC_ARM_REG_LR,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3,UC_ARM_REG_PC,UC_ARM_REG_XPSR
def symbols(nm,elf):
    result={}
    for line in subprocess.check_output([nm,'-n',str(elf)],text=True).splitlines():
        fields=line.split()
        if len(fields)==3:
            try:result[fields[2]]=int(fields[0],16)
            except ValueError:pass
    return result
def check(nm,profile,kib):
    b=Path(__file__).resolve().parents[1];d=b/'build'/('gcc-'+profile)
    syms=symbols(nm,d/'DomovoiBoot.elf');binary=(d/'DomovoiBoot.bin').read_bytes()
    u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
    for addr,size in [(0x08000000,0x20000),(0x20000000,0x5000),(0x40000000,0x30000),(0xe000e000,0x2000),(0x1ffff000,0x1000)]:u.mem_map(addr,size)
    u.mem_write(0x08000000,binary)
    u.mem_write(syms['_sdata'],binary[syms['_sidata']-0x08000000:syms['_sidata']-0x08000000+syms['_edata']-syms['_sdata']])
    u.mem_write(0x1ffff7e0,struct.pack('<H',kib))
    for addr in (0x40010800,0x40010c00,0x40011000):u.mem_write(addr,struct.pack('<II',0x44444444,0x44444444))
    writes=[]
    u.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,data:writes.append((address,value)))
    stop=0x0801fff0
    def call(name,*args):

        u.reg_write(UC_ARM_REG_XPSR,0x01000000)
        u.reg_write(UC_ARM_REG_SP,0x20004ff0);u.reg_write(UC_ARM_REG_LR,stop|1)
        for reg,value in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args):u.reg_write(reg,value)
        u.reg_write(UC_ARM_REG_PC,syms[name]|1)
        u.emu_start(syms[name]|1,stop,count=100000)
        assert u.reg_read(UC_ARM_REG_PC)==stop, (name,hex(u.reg_read(UC_ARM_REG_PC)))
        return u.reg_read(UC_ARM_REG_R0)
    def word(addr):return struct.unpack('<I',u.mem_read(addr,4))[0]
    call('boot_platform_init')
    assert call('boot_platform_supported')==(kib==128)
    assert word(0x40010c04)&15==3 and (word(0x40010c00)>>24)&15==3
    mark=len(writes)
    for ticks in (0,249,250,499,500,1000):
        u.mem_write(syms['ticks'],struct.pack('<I',ticks));call('boot_service')
    assert sum(0x40010c00<=a<0x40010c1c for a,v in writes[mark:])==12, 'PWR/DATA indication must still run'
    if kib!=128:
        mark=len(writes)
        result=call('boot_int_erase',0x08008000)
        assert result==0, (result,bytes(u.mem_read(syms['flash_kib'],2)))
        assert call('boot_int_write',0x08008000,0x20004000,4)==0
        assert call('boot_read',0,0x08008000,0x20004000,4)==0
        assert not any(0x40022000<=a<0x40022024 for a,v in writes[mark:]), 'Unsupported MCU must never unlock/erase/program Flash'
    print(profile,kib,'KiB: GPIO and Flash guards passed')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--nm',required=True);args=p.parse_args()
    for profile in ('v5',):
        for kib in (128,64,0,65535):check(args.nm,profile,kib)
