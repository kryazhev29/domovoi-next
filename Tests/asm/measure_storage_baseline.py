"""Baseline real IAR caller paths. NOR/SPI modeled; instructions are not time."""
import sys,json,struct,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'Tests'))
from iar_irq_support import Machine, UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2
from iar_layout import fields
m=Machine();m.init();flash=bytearray(b'\xff'*0x400000)
def args():return [m.u.reg_read(r) for r in (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2)]
def read():
    a,p,n=args();assert a+n<=len(flash);m.u.mem_write(p,bytes(flash[a:a+n]));m.ret(1)
def write():
    a,p,n=args();assert a+n<=len(flash)
    for i,v in enumerate(m.u.mem_read(p,n)):flash[a+i]&=v
    m.ret(1)
def erase():
    a=args()[0];assert not a%4096;flash[a:a+4096]=b'\xff'*4096;m.ret(1)
for name,fn in [('w25_read_data',read),('w25_write_data',write),('w25_erase_sector',erase),('wdt_reload',m.ret),('user_wait_stable_power',m.ret)]:
    m.stubs[m.addr(name)]=fn
m.u.mem_write(m.addr('mem')+2*8,struct.pack('<II',0x41000,8192))
m.u.mem_write(m.addr('mem')+5*8,struct.pack('<II',0x1cc000,512))
out={'units':'Unicorn instructions; modeled NOR, NOT cycles or milliseconds','cases':{}}
for count in (1,63):
    flash[:]=b'\xff'*len(flash);rows=bytearray(count*32)
    for i in range(count):rows[i*32:i*32+2]=struct.pack('<H',i+1);rows[i*32+12]=1
    m.u.mem_write(0x21000000,bytes(rows));assert m.call('sync_write_cells',0x21000000,count,2,32,budget=10000000)
    assert flash[0x41000:0x41000+len(rows)]==rows
    out['cases']['table_'+str(count)]={'instructions':m.steps,'stack':m.top-m.minimum}
for n in (1,4095):
    flash[0x50000:0x51000]=bytes((i*17)&255 for i in range(4096))
    assert m.call('photo_backup_crc',0x50000,n,0x21000000)
    out['cases']['photo_crc_'+str(n)]={'instructions':m.steps,'stack':m.top-m.minimum}
p=m.call('__basic_malloc',192);m.write(m.addr('photo'),p)
layout=fields('photo_s');d=fields('device_s')
m.u.mem_write(m.addr('mem')+8*8,struct.pack('<II',0x200000,0x100000))
out['photo_layout']=layout
out['sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
(ROOT/'Tests/asm/storage_baseline_20261005.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out['cases']))
