"""Пути в ELF: кадр Wiegand до записи GPIO и серверный поток RX до отправки ответа."""
import argparse,hashlib,json,struct
from pathlib import Path
from protocol_contract import Profile,serial
from firmware_suite import select_image
from test_access_reads import AccessMachine
from test_service_runtime import ServiceMachine
from storage_machine import fields,crc16
from iar_image import image_paths

def run():
    totals={};out={}
    m=AccessMachine();m.put_group(1)
    m.write(m.addr('mem')+2*8+4,1601536)
    for cell in range(1,50049):m.put_id(cell,b'id%08d'%cell)
    m.put_id(50048,b'0000123456')
    m.write(m.addr('config')+fields('config_s')['rele']+fields('releMode_s')['mode'],1,1)
    for name in ('out_load','out_getmodestate'):m.stubs.pop(m.addr(name),None)
    value=0x123456
    raw=(value<<1)|(((value&4095).bit_count()^1)&1)|(((value>>12).bit_count()&1)<<25)
    m.write(m.addr('wgData'),raw);m.write(m.addr('wgBitCount'),26,1)
    m.write(m.addr('wgTime'),100);m.write(m.addr('milliSeconds'),400)
    # Маркер измерения присутствует в диагностическом профиле; GPIO проверяется в обоих.
    if 'prof_card_mark' in m.symbols:
        m.write(m.addr('prof_card_mark'),1)
    p=Profile(m,totals);relay=[]
    def gpio(u,access,a,n,value,_):
        if value==8:relay.append({'instructions':p.n,'fetch_estimate':p.n+p.wait,'register':hex(a),'value':value})
    m.watch_writes(0x40010c10,0x40010c10,gpio)
    def access():
        for _ in range(400):
            m.checked('WG_process')
            if relay: return
            m.now+=1
        raise AssertionError('Wiegand lookup did not finish')
    p.run(access)
    assert len(relay)==1 and m.u.mem_read(m.addr('out_st'),1)[0]&1
    assert m.events[-1][0]==71
    out['wiegand-full-last-to-gpio']={'cost':{k:relay[0][k] for k in ('instructions','fetch_estimate')},
        'result':{'relay':{k:relay[0][k] for k in ('register','value')},'event':serial(m.events),
                  'nor_sha256':hashlib.sha256(m.flash).hexdigest()},
        'scope':'Repeated WG_process with one queued decoded frame; 50048 IDs, last match; until GPIOB BSRR bit3 write. Bit reception and 25 ms framing wait excluded.'}
    m=ServiceMachine(8192,1601536);p=Profile(m,totals)
    body=struct.pack('<HBBB',3,0,73,17);frame=b'SLDN'+body+struct.pack('<H',crc16(body))
    m.u.mem_write(0x21000000,frame)
    def request():
        m.checked('server_rx_begin_pass')
        assert m.checked('user_comm_rx',0x21000000,len(frame))==len(frame)
        m.checked('user_comm_rx_done',len(frame),0)
        m.checked('server_rx_process')
        assert m.active(),'server frame did not reach storage owner'
        for _ in range(40000):
            if not m.active():break
            m.now+=1;m.checked('user_storage_poll')
        else:raise AssertionError('server request did not complete')
        assert m.sent==[m.expected(17)]
    p.run(request)
    out['server-stream-17-to-reply']={'cost':p.last,'result':serial(m.sent),
        'scope':'user_comm_rx -> server_rx_process -> storage owner -> successful comm_tx_commit; physical modem transport modeled'}
    return {'functions':totals,'scenarios':out,'elf_sha256':hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();select_image();result=run()
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print('PASS paths')
