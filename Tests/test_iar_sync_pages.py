"""Настоящий IAR: команда 13, 1..63 ID, повторы и смена сектора без heap."""
import pathlib,runpy,struct
from iar_layout import fields
env=runpy.run_path(str(pathlib.Path(__file__).with_name('test_iar_page_heap.py')))
m,flash,requests=env['m'],env['flash'],env['requests'];base=0x41000
m.u.mem_write(m.addr('mem')+2*8,struct.pack('<II',base,8192))
flash[base:base+8192]=b'\xff'*8192
peak=0
for count in range(1,64):
    rows=bytearray(32*count)
    for i in range(count):rows[i*32:i*32+2]=struct.pack('<H',i+1);rows[i*32+12]=1
    m.u.mem_write(0x21000000,bytes(rows))
    assert m.call('sync_write_cells',0x21000000,count,2,32,budget=16000000)
    assert flash[base:base+len(rows)]==rows
    peak=max(peak,m.top-m.minimum)
    if count%16==0:print('IAR sync',count,'ID PASS',flush=True)
reserve=fields('cellID_s')['reserve']
old=bytearray(32);old[:2]=b'\1\0';old[2:12]=b'A'*10;old[12]=1;old[reserve:reserve+4]=struct.pack('<I',1700000000);old[reserve+6]=0x38
flash[base:base+32]=old
rows=bytearray(old+old+old);rows[32:34]=struct.pack('<H',129)
for i in range(3):rows[i*32+reserve:i*32+reserve+7]=b'\0'*7
counter=fields('cellID_s')['cntPass'];rows[64+counter]=7;m.u.mem_write(0x21000000,bytes(rows))
assert m.call('sync_write_cells',0x21000000,3,2,32,budget=16000000)
assert flash[base+reserve:base+reserve+4]==old[reserve:reserve+4] and flash[base+reserve+6]==0x38
assert flash[base+counter]==7 and flash[base+4096:base+4108]==rows[32:44]
peak=max(peak,m.top-m.minimum)
assert not requests and peak+env['irq']<=m.top-m.bottom
print('IAR sync 1..63 ID: no heap, sector order/duplicates/first-pass metadata PASS; tested stack',peak)
