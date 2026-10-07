"""Сохранить проверку C-ядер после удаления ASM: случайные входы, границы чтений и ABI."""
import argparse
import json
from pathlib import Path
import random
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from test_asm_crc import Machine,BUF,STACK
from iar_image import image_paths,result_path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import arm_const
from capstone import Cs,CS_ARCH_ARM,CS_MODE_THUMB
from capstone.arm import ARM_OP_MEM,ARM_SFT_INVALID,ARM_SFT_LSL

def apply(columns,value):
    out=0
    for i,c in enumerate(columns):
        if value&(1<<i):out^=c
    return out

def ff_powers():
    from storage_machine import crc16
    bias=crc16(b'\xff',0);columns=[crc16(b'\xff',1<<i)^bias for i in range(16)]
    result=[]
    for _ in range(32):
        result.append((columns,bias));bias=apply(columns,bias)^bias
        columns=[apply(columns,c) for c in columns]
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=result_path('c-kernels.json'));a=p.parse_args()
    m=Machine(image_paths()[0]);rng=random.Random(20261007);allowed=[];checks=0
    decoder=Cs(CS_ARCH_ARM,CS_MODE_THUMB);decoder.detail=True;decoded={}
    def read_bounds(address,n):
        assert 0x08000000<=address and address+n<=0x08020000 or STACK-4096<=address and address+n<=STACK or any(lo<=address and address+n<=hi for lo,hi in allowed),(hex(address),n)
    def instruction(u,address,size,unused):
        # UC_HOOK_MEM_READ нарушает ITSTATE на LDR внутри IT в Unicorn 2.1.4.
        # Наблюдение за инструкцией не меняет состояние процессора.
        ins=decoded.get(address)
        if ins is None:
            ins=next(decoder.disasm(bytes(u.mem_read(address,size)),address));decoded[address]=ins
        if not ins.mnemonic.startswith('ldr'):return
        flags=u.reg_read(arm_const.UC_ARM_REG_CPSR)
        n,z,c,v=(bool(flags&(1<<bit)) for bit in (31,30,29,28))
        conditions=(True,z,not z,c,not c,n,not n,v,not v,c and not z,not c or z,n==v,n!=v,not z and n==v,z or n!=v,True)
        if not conditions[ins.cc]:return
        def reg(r):
            if not r:return 0
            name=ins.reg_name(r)
            return (address+4)&~3 if name=='pc' else u.reg_read(getattr(arm_const,'UC_ARM_REG_'+name.upper()))
        for op in ins.operands:
            if op.type!=ARM_OP_MEM:continue
            index=reg(op.mem.index)
            assert op.shift.type in (ARM_SFT_INVALID,ARM_SFT_LSL),ins.op_str
            if op.shift.type==ARM_SFT_LSL:index<<=op.shift.value
            pointer=(reg(op.mem.base)+index*op.mem.scale+op.mem.disp)&0xffffffff
            width=1 if ins.mnemonic.startswith(('ldrb','ldrsb')) else 2 if ins.mnemonic.startswith(('ldrh','ldrsh')) else 8 if ins.mnemonic.startswith('ldrd') else 4
            read_bounds(pointer,width)
    def write(u,access,address,n,value,unused):
        assert STACK-4096<=address and address+n<=STACK,(hex(address),n)
    m.uc.hook_add(UC_HOOK_CODE,instruction);m.uc.hook_add(UC_HOOK_MEM_WRITE,write)
    lengths=(0,1,2,3,4,7,10,15,16,31,32,255,256,511,512)
    for i in range(4000):
        n=lengths[i%len(lengths)] if i<120 else rng.randrange(513)
        data=b'\xff'*n if i%3==0 else rng.randbytes(n)
        if i%5==0 and n:
            at=rng.randrange(n);data=data[:at]+b'SLDN'[:min(4,n-at)]+data[at+min(4,n-at):]
        ptr=BUF+i%4 if n else 0
        if n:m.uc.mem_write(ptr,data)
        allowed[:]=[(ptr,ptr+n)] if n else []
        assert m.call('fast_all_ff',ptr,n,check_abi=True)==int(all(v==255 for v in data))
        expected=next((at for at in range(n) if data[at:at+4]==b'SLDN'[:min(4,n-at)]),n)
        actual=m.call('fast_sldn',ptr,n,check_abi=True)
        assert actual==expected,(i,n,expected,actual)
        ident=rng.randbytes(10);ptr=BUF+i%4;m.uc.mem_write(ptr,ident);allowed[:]=[(ptr,ptr+10)]
        h=2166136261
        for v in ident:h=((h^v)*16777619)&0xffffffff
        assert m.call('fast_fnv10',ptr,check_abi=True)==h
        checks+=3
    columns=[rng.randrange(65536) for _ in range(16)]
    m.uc.mem_write(BUF,struct.pack('<16H',*columns));allowed[:]=[(BUF,BUF+32)]
    for value in range(65536):
        assert m.call('fast_gf2_apply',BUF,value,check_abi=True)==apply(columns,value)
        checks+=1
    allowed[:]=[];powers=ff_powers()
    for i in range(2000):
        length=([0,1,32767,32768,65535,0x80000000,0xffffffff][i] if i<7 else rng.getrandbits(32))
        seed=rng.randrange(65536);expected=seed
        for bit,(columns,bias) in enumerate(powers):
            if length&(1<<bit):expected=apply(columns,expected)^bias
        assert m.call('crc16_ffInit',length,seed,check_abi=True)==expected
        checks+=1
    a.output.write_text(json.dumps({'checks':checks,'gf2_exhaustive':65536,'read_bounds':True,'abi':True},indent=2)+'\n',encoding='utf-8')
    print('PASS C kernels',checks,'cases; bounds and ABI')

if __name__=='__main__':main()
