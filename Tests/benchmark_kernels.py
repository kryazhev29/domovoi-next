"""Польза оставшегося ASM-ядра в настоящих операциях ELF относительно C-эталона."""
import argparse
import json
import os
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from build_iar import build
from firmware_suite import select_image
from protocol_contract import Profile
from iar_image import image_paths,result_path
from iar_irq_support import Machine,UC_HOOK_MEM_READ
from test_service_runtime import ServiceMachine
from test_access_reads import AccessMachine
from storage_machine import crc16

ROOT=Path(__file__).resolve().parents[1]

def operations():
    result={}
    def execute(name,m,fn):
        p=Profile(m,{});value=p.run(fn)
        result[name]={**p.last,'result':value}
    m=ServiceMachine(8192,1601536)
    def request():
        m.packet(17);m.drain(60000);assert m.sent==[m.expected(17)]
        return m.sent[0].hex()
    execute('command17',m,request)
    m=ServiceMachine(0,1601536);execute('command17-empty',m,request)
    m=ServiceMachine(8192,1601536)
    def receive():
        body=struct.pack('<HBBB',3,0,73,17);frame=b'SLDN'+body+struct.pack('<H',crc16(body))
        m.u.mem_write(0x21000000,frame);m.checked('server_rx_begin_pass')
        assert m.checked('user_comm_rx',0x21000000,len(frame))==len(frame)
        m.checked('user_comm_rx_done',len(frame),0);m.checked('server_rx_process');m.drain(60000)
        assert m.sent==[m.expected(17)];return m.sent[0].hex()
    execute('server-frame',m,receive)
    from test_comm_settings_elf import accepted,configuration,machine
    m=machine(configuration())
    def settings():
        accepted(m,configuration());return [frame.hex() for frame in m.sent]
    execute('command12',m,settings)
    m=AccessMachine();m.put_group(1)
    for cell in range(1,257):m.put_id(cell,b'%010d'%cell)
    m.write(m.addr('id_used_bytes'),256*32);m.u.mem_write(0x21000000,b'0000000256')
    execute('access-last256',m,lambda:m.checked('user_checkIdAndControl',0x21000000,2,budget=12000000))
    from test_idindex_runtime import bucket
    m.write(m.addr('ix')+10,1,1);at=bucket(b'0000000256');m.flash[at:at+2]=struct.pack('<H',256)
    execute('access-indexed256',m,lambda:m.checked('user_checkIdAndControl',0x21000000,2,budget=12000000))
    for name,address in [('database-page',0x41000),('journal-page',0x1d0000)]:
        m=Machine();m.init();m.u.mem_map(0x42000000,0x600000)
        m.write(0x40013000,0x40,2);m.write(0x40013008,3,2)
        def spi(u,access,a,n,value,unused):
            if a==0x4001300c:m.write(a,0,2)
        m.u.hook_add(UC_HOOK_MEM_READ,spi)
        def read():
            assert m.call('w25_read_data',address,0x21000000,256)
            assert bytes(m.u.mem_read(0x21000000,256))==bytes(256)
            return bytes(m.u.mem_read(0x21000000,256)).hex()
        execute(name,m,read)
    return result

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=result_path('kernel-benefit-current.json'));a=p.parse_args()
    config=os.environ.get('DOMOVOI_COMPARE_CONFIG','Release');select_image()
    original=dict(os.environ);actual=operations();actual_bin=image_paths()[1].read_bytes()
    base=result_path('kernel-reference')
    base.mkdir(parents=True,exist_ok=True);tree=ET.parse(ROOT/'DomovoiNext.ewp')
    for n in tree.findall('.//state')+tree.findall('.//file/name'):
        if n.text:n.text=n.text.replace('$PROJ_DIR$',str(ROOT))
    for n in tree.findall('.//file/name'):
        if n.text.endswith('\\asm\\crc_asm.s'):n.text=str(ROOT/'Tests/reference/crc_asm.c')
    project=base/'DomovoiNext.ewp';tree.write(project,encoding='utf-8',xml_declaration=True)
    folder=build(base/'build',config,project_path=project)
    try:
        select_image(base/'build');reference=operations()
        reference_bin=image_paths()[1].read_bytes()
    finally:
        os.environ.clear();os.environ.update(original)
    rows={}
    for name,value in actual.items():
        c=reference[name];assert value['result']==c['result'],name
        rows[name]={'asm':value['instructions'],'c':c['instructions'],
                    'gain_percent':round((c['instructions']-value['instructions'])/c['instructions']*100,3),
                    'asm_fetch_estimate':value['fetch_estimate'],'c_fetch_estimate':c['fetch_estimate']}
    assert max(r['gain_percent'] for r in rows.values())>=5,'ASM-ядро больше не оправдывает себя'
    a.output.write_text(json.dumps({'core':'crc_asm','configuration':config,'operations':rows,
        'asm_bin_bytes':len(actual_bin),'c_reference_bin_bytes':len(reference_bin),
        'bin_saving_bytes':len(reference_bin)-len(actual_bin),
        'method':'Инструкции Unicorn; оценка выборки +2 ожидания на слово Flash. Без конвейера, IRQ и физической задержки NOR; не такты платы.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('PASS kernel benefit',config,max(r['gain_percent'] for r in rows.values()),'%;',len(reference_bin)-len(actual_bin),'bytes')

if __name__=='__main__':main()
