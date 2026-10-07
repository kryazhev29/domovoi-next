"""Короткие сценарии протокола, атомарного хранения и выборочного обрыва питания."""
import struct
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from storage_machine import StorageMachine,crc16
from test_service_runtime import ServiceMachine
from test_comm_settings_elf import machine,configuration,accepted,rejected
from test_table_runtime import rows

def protocol():
    m=machine(configuration());accepted(m,configuration())
    bad=configuration();bad[:32]=bytes(32);rejected(m,bad)
    m=ServiceMachine(129,8192)
    body=struct.pack('<HBBB',3,0,73,17);frame=b'SLDN'+body+struct.pack('<H',crc16(body))
    m.u.mem_write(0x21000000,frame);m.checked('server_rx_begin_pass')
    assert m.checked('user_comm_rx',0x21000000,len(frame))==len(frame)
    m.checked('user_comm_rx_done',len(frame),0);m.checked('server_rx_process');m.drain()
    assert m.sent==[m.expected(17)]
    return 3

def storage():
    payload=rows([1,129])
    m=StorageMachine();m.command(13,payload,2);m.drain()
    assert m.flash[0x41000:0x41020]==payload[:32]
    assert m.flash[0x42000:0x42020]==payload[32:]
    assert m.checked('flash_exec_read_allowed')
    return 2

def power():
    m=StorageMachine();m.flash[0x4101f]=0;m.record_changes=True
    initial=bytes(m.flash);m.command(13,rows([1]),1);m.drain()
    final=bytes(m.flash[0x41000:0x42000]);flash=bytearray(initial);checks=0
    # Начало журнала, середина операции и публикация результата; три места разрыва.
    chosen={0,len(m.changes)//2,len(m.changes)-1}
    for index,(kind,address,data) in enumerate(m.changes):
        if index in chosen:
            for n in sorted({0,len(data)//2,len(data)}):
                cut=bytearray(flash)
                for i,value in enumerate(data[:n]):cut[address+i]=value if kind=='erase' else cut[address+i]&value
                recovered=StorageMachine(cut);recovered.limit_reads=1000000
                assert recovered.checked('sj_recover',budget=2000000)
                assert bytes(recovered.flash[0x41000:0x42000]) in (initial[0x41000:0x42000],final)
                checks+=1
        for i,value in enumerate(data):flash[address+i]=value if kind=='erase' else flash[address+i]&value
    return checks
