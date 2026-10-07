import argparse
from pathlib import Path
import struct
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from boot_tool import crc32
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_R0, UC_ARM_REG_PC, UC_ARM_REG_XPSR

p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,required=True);p.add_argument('--binary',type=Path,required=True)
p.add_argument('--image',type=Path,required=True);p.add_argument('--nm',required=True);args=p.parse_args()
symbols={line.split()[-1]:int(line.split()[0],16) for line in subprocess.check_output([args.nm,str(args.elf)],text=True).splitlines() if len(line.split())==3}
image=args.image.read_bytes();ram=args.binary.read_bytes();assert len(image)==32768

def run(corrupt_page=None,cut=None):
    emu=Uc(UC_ARCH_ARM,UC_MODE_THUMB)
    emu.mem_map(0x20000000,0x5000);emu.mem_write(0x20000000,ram)
    emu.mem_map(0x08000000,0x20000);emu.mem_write(0x08000000,b'\xa5'*0x20000)
    emu.mem_map(0x40000000,0x30000);emu.mem_map(0xe000e000,0x2000)
    checks=b''.join(struct.pack('<I',crc32(image[i:i+1024])) for i in range(0,32768,1024))
    emu.mem_write(0x20003000,checks)
    state=dict(index=0,address=0,rx=0,flash_address=0,reset=False,mutations=0,erased=[])
    def write(uc,access,address,size,value,user):
        if address==0x40010c14 and value&4096:
            state['index']=0;state['address']=0
        elif address==0x4001300c:
            i=state['index'];state['index']+=1
            if i==0:assert value==3
            if 1<=i<=3:state['address']=(state['address']<<8)|(value&255)
            if i>=4:
                off=state['address'];assert 0<=off<32768
                state['rx']=image[off]^(1 if off//1024==corrupt_page else 0);state['address']+=1
            else:state['rx']=0
        elif address==0x40022014:state['flash_address']=value
        elif address==0x40022010 and value==0x42:
            page=state['flash_address'];assert 0x08000000<=page<0x08008000
            state['erased'].append(page);uc.mem_write(page,b'\xff'*1024);state['mutations']+=1
        elif 0x08000000<=address<0x08020000:
            assert address+size<=0x08008000, 'Application overwritten'
            state['mutations']+=1
        elif address==0xe000ed0c:
            assert value==0x05fa0004;state['reset']=True;uc.emu_stop()
        if cut and state['mutations']>=cut:uc.emu_stop()
    def read(uc,access,address,size,value,user):
        if address==0x40013008:uc.mem_write(address,struct.pack('<I',3))
        elif address==0x4001300c:uc.mem_write(address,struct.pack('<I',state['rx']))
        elif address==0x4002200c:uc.mem_write(address,bytes(4))
    def code(uc,address,size,user):
        assert 0x20000000<=address<0x20000000+len(ram), 'Execution escaped RAM'
    emu.hook_add(UC_HOOK_MEM_READ,read);emu.hook_add(UC_HOOK_MEM_WRITE,write);emu.hook_add(UC_HOOK_CODE,code)
    emu.reg_write(UC_ARM_REG_SP,0x20005000);emu.reg_write(UC_ARM_REG_R0,0x20003000)
    emu.reg_write(UC_ARM_REG_XPSR,0x01000000)
    emu.emu_start(symbols['replace_ram']|1,0,count=6000000)
    assert emu.mem_read(0x08008000,0x18000)==b'\xa5'*0x18000
    if cut:assert state['mutations']>=cut
    else:
        assert state['reset'],'Writer did not reset'
        if corrupt_page is None:assert emu.mem_read(0x08000000,32768)==image
        else:assert len(state['erased'])==corrupt_page
    return state['mutations']

total=run()
run(corrupt_page=0);run(corrupt_page=7)
for cut in (1,2,513,514,total-1):run(cut=cut)
print('RAM writer: complete image, SPI corruption, 5 interruption points, RAM-only execution and unchanged application passed')
