"""Exercise IAR fault handlers/status frames in Unicorn; no device access.
Run with Unicorn installed (or its directory on PYTHONPATH), after Release build.
"""
from pathlib import Path
import re, struct
from functools import lru_cache
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.arm_const import *
R=Path(__file__).resolve().parents[1]
text=(R/'Release/List/DomovoiNext.map').read_text()
@lru_cache(None)
def sym(name):
    m=re.search(r'^'+re.escape(name)+r'\s+(0x[0-9a-f\x27]+)',text,re.M)
    assert m,name
    return int(m[1].replace("'",''),16)
def machine():
    u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
    for a,n in [(0x08000000,0x20000),(0x20000000,0x5000),(0x40000000,0x24000),(0xe000e000,0x2000)]:u.mem_map(a,n)
    u.mem_write(0x08008200,(R/'Release/Exe/DomovoiNext.bin').read_bytes())
    return u
record=sym('fault_record');assert record==0x20004fc0
handlers=['HardFault_Handler','MemManage_Handler','BusFault_Handler','UsageFault_Handler']
for handler in handlers:
    for psp in [False,True]:
        for mode in ['valid','unaligned','outside','stack_error']:
            u=machine();sp=0x20002000 if mode in ['valid','stack_error'] else (0x20002001 if mode=='unaligned' else 0x1ffffff0)
            u.reg_write(UC_ARM_REG_MSP,0x20003000 if psp else sp)
            u.reg_write(UC_ARM_REG_PSP,sp if psp else 0x20003000)
            exc=0xfffffffd if psp else 0xfffffff9
            u.reg_write(UC_ARM_REG_LR,exc)
            u.reg_write(UC_ARM_REG_XPSR,0x01000000+3+handlers.index(handler))
            frame=[0,1,2,3,12,0x0800abcd,0x08001234,0x21000000]
            if mode in ['valid','stack_error']:u.mem_write(sp,struct.pack('<8I',*frame))
            cfsr=0x1000 if mode=='stack_error' else 0x8200
            u.mem_write(0xe000ed28,struct.pack('<5I',cfsr,0x40000000,0,0x20001234,0xdeadbeef))
            writes=[]
            u.hook_add(UC_HOOK_MEM_WRITE,lambda uc,t,a,n,v,d:writes.append((a,n)))
            u.emu_start(sym(handler),0,count=180)
            got=struct.unpack('<12I',u.mem_read(record,48))
            assert got[:4]==(0x31544c46,3+handlers.index(handler),exc,sp),got
            assert got[4:7]==(tuple(frame[6:7]+frame[5:6]+frame[7:8]) if mode=='valid' else (0,0,0)),got
            assert got[7:]==(cfsr,0x40000000,0x20001234,0xdeadbeef,int(mode=='valid')),got
            assert all(record<=a and a+n<=record+48 for a,n in writes),writes
            assert u.reg_read(UC_ARM_REG_PRIMASK)==1
print('Fault handlers: 32 MSP/PSP/frame cases; no stack or watchdog writes')
# Actual IAR C runtime initialization must retain the record across a warm start.
u=machine();saved=struct.pack('<12I',0x31544c46,*range(1,12));u.mem_write(record,saved)
u.reg_write(UC_ARM_REG_MSP,sym('CSTACK$$Limit'))
reached=[]
def at_main(uc,a,n,d):
    if a==(sym('main')&~1):reached.append(1);uc.emu_stop()
h=u.hook_add(UC_HOOK_CODE,at_main)
u.emu_start(sym('__iar_program_start'),0,count=200000)
assert reached and bytes(u.mem_read(record,48))==saved
u.hook_del(h)
# Read the actual command 101 wire buffer, including its CRC.
def crc16(b):
    c=0xffff
    for x in b:
        c^=x
        for _ in range(8):c=(c>>1)^(0xa001 if c&1 else 0)
    return c
for present in [False,True]:
    u.mem_write(record,saved if present else bytes(48));frames=[]
    def capture(uc,a,n,d):
        if a==(sym('usb_send')&~1):
            frames.append(bytes(uc.mem_read(uc.reg_read(UC_ARM_REG_R0),uc.reg_read(UC_ARM_REG_R1))));uc.emu_stop()
    h=u.hook_add(UC_HOOK_CODE,capture)
    u.reg_write(UC_ARM_REG_MSP,sym('CSTACK$$Limit'));u.reg_write(UC_ARM_REG_R0,0)
    # CH_USB is defined in user_fun.h; check the enumerator below if this changes.
    u.emu_start(sym('user_send_statusPackToConfigurator'),0,count=10000)
    u.hook_del(h)
    assert len(frames)==1,'CH_USB enumeration / status output'
    f=frames[0];assert len(f)==(90 if present else 42)
    assert f[:4]==b'SLDN' and f[8]==101
    assert struct.unpack_from('<H',f,4)[0]==len(f)-8
    assert struct.unpack_from('<H',f,len(f)-2)[0]==crc16(f[4:-2])
    if present:assert f[40:88]==saved
print('IAR warm startup retains dump; command 101 legacy/FLT1 payload and CRC passed')
