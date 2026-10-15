"""Контракт поведения одного ELF: пакеты, события, NOR и сквозные пути."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import sys
import tempfile
from collections import defaultdict
from functools import lru_cache

from firmware_suite import ROOT, select_image
import iar_irq_support as support
from iar_image import image_paths
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC
from elftools.elf.elffile import ELFFile

sys.path.insert(0, str(Path(tempfile.gettempdir())/'codex-deps'))
from capstone import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_MCLASS


@lru_cache(None)
def public_inventory():
    result = {}
    for path in sorted((ROOT/'User/asm').glob('*.s')):
        for name in re.findall(r'^\s+PUBLIC\s+(\S+)',path.read_text(encoding='utf-8'),re.M):
            result[name] = path.stem
    return result


class Profile:
    """Счётчики экспортируемых функций, включая внутренние вызовы и библиотечный код.

    Две оценки: одна инструкция за такт и консервативная выборка без предзагрузки
    с двумя тактами ожидания на 32-битное слово Flash. Это не потактовая модель
    Cortex-M3; время внешней NOR сюда не входит."""
    def __init__(self, machine, totals):
        self.m=machine; self.totals=totals; self.stack=[]; self.n=0; self.wait=0
        self.enabled=False; self.cache={}; self.word=None
        self.md=Cs(CS_ARCH_ARM,CS_MODE_THUMB|CS_MODE_MCLASS)
        inventory=public_inventory()
        self.names={machine.addr(n):n for n in inventory if n in machine.symbols and machine.symboltypes[n]=='STT_FUNC'}
        machine.u.hook_add(UC_HOOK_CODE,self.hook)

    def close(self):
        while self.stack:self.finish()

    def finish(self):
        name,ret,start,wait=self.stack.pop()
        n=self.n-start; estimate=n+self.wait-wait
        r=self.totals.setdefault(name,{'calls':0,'instructions':0,'fetch_estimate':0,'min':n,'max':n})
        r['calls']+=1; r['instructions']+=n; r['fetch_estimate']+=estimate
        r['min']=min(r['min'],n);r['max']=max(r['max'],n)

    def hook(self,u,a,size,_):
        if not self.enabled:return
        while self.stack and a==self.stack[-1][1]:self.finish()
        if a in self.names and a not in self.m.stubs:
            self.stack.append((self.names[a],u.reg_read(UC_ARM_REG_LR)&~1,self.n,self.wait))

        if a in self.m.stubs:return
        self.n+=1
        if 0x08000000<=a<0x08100000:
            first=a&~3;last=(a+size-1)&~3
            self.wait+=2*(int(first!=self.word)+int(last!=first))
            self.word=last

    def run(self, fn):
        start=self.n;wait=self.wait;self.enabled=True
        try:return fn()
        finally:self.close();self.enabled=False;self.last={'instructions':self.n-start,'fetch_estimate':self.n-start+self.wait-wait}


def snapshot(m):
    """Только наблюдаемые снаружи байты; адреса RAM различаются между сборками."""
    result={'nor':bytes(m.flash)} if hasattr(m,'flash') else {}
    for name in ('sent','wires','answers','timestamps','events','loads','logs','expiry_events','commands'):
        if hasattr(m,name):result[name]=copy.deepcopy(getattr(m,name))
    return result


def serial(value):
    if isinstance(value,(bytes,bytearray)):
        return {'bytes':len(value),'sha256':hashlib.sha256(value).hexdigest()}
    if isinstance(value,(tuple,list)):return [serial(v) for v in value]
    if isinstance(value,dict):return {k:serial(v) for k,v in value.items()}
    return value


def scenarios(totals):
    from storage_machine import StorageMachine
    from test_table_runtime import rows
    from test_access_reads import AccessMachine
    from test_upload_runtime import UploadMachine,log
    from test_photo_runtime import PhotoMachine,prefix_flash
    from test_fw_service_runtime import FirmwareMachine
    from test_expiry_runtime import ExpiryMachine,record
    from test_idclear_runtime import ClearMachine
    from test_service_runtime import ServiceMachine
    from test_memory_runtime import config_machine
    from test_idindex_runtime import IndexMachine
    from test_usb_source import init_transport
    out={}
    def execute(name,m,fn):
        p=Profile(m,totals);value=p.run(fn)
        out[name]={'result':snapshot(m),'returned':value,'cost':p.last}
        print('scenario',name,p.last['instructions'],flush=True)
        return value

    for cmd,indices in ((13,[1,128,129]),(14,[1,64]),(15,[1,128]),(16,[1,2])):
        m=StorageMachine();data=rows(indices,16 if cmd in (14,15) else 32)
        if cmd==14:data+=bytes(range(8))+b"\1"+bytes(7)
        if cmd==16:data=b'\x01\x02\x03\x04'*2
        execute('sync-'+str(cmd),m,lambda:(m.command(cmd,data,len(indices)),m.drain()))

    from test_sched16 import record as schedule_record, tail, legacy, migration_machine, restore
    for count in (1,25,64):
        m=StorageMachine()
        data=b''.join(schedule_record(i) for i in range(1,count+1))+tail()
        execute('sched16-N'+str(count),m,lambda:(m.command(14,data,count),m.drain()))
    execute('sched16-repeat64',m,lambda:(m.command(14,data,count),m.drain()))
    execute('sched16-partial-tail',m,lambda:(m.command(14,schedule_record(64,blocked=1)+tail(bytes([64])*8,3),1),m.drain()))
    def assembled():
        assert m.checked('user_getShedule',64,0x21000300)
        return bytes(m.u.mem_read(0x21000300,32))
    execute('sched16-assembled',m,assembled)
    m=migration_machine(legacy())
    execute('sched16-migration',m,lambda:restore(m))

    from test_id_sync_fast import protocol_scenarios
    out.update(protocol_scenarios())

    from test_id_sync_fast import SyncMachine,fixture,cell
    from test_stage5_crc import warm,get
    m=fixture(SyncMachine(),1,total=8192)
    execute('CRC-RAM-cold',m,lambda:(get(m),warm(m)))
    execute('CRC-RAM-delta',m,lambda:(m.command(13,cell(1,1),1),m.drain(),get(m)))
    # table_runtime_cancel возвращает void: случайный R0 не является контрактом.
    def cancel_crc():
        m.checked('table_runtime_cancel')
    execute('CRC-RAM-cancel',m,lambda:(m.command(13,cell(1,2),1),cancel_crc(),m.drain(),get(m),warm(m)))
    from test_stage5_log import boot,append,records
    m=UploadMachine();m.cmd=10;m.base=0x1d0000
    m.flash[0x1cf400:0x1cf400+128*32]=b''.join(log(i,True) for i in range(1,129))
    execute('log-old-format-discard',m,lambda:(boot(m)[0],m.request(),m.drain_upload()))
    execute('log-after-format-upload',m,lambda:(append(m,129),records(m),m.request(0,8),m.drain_upload()))


    m=StorageMachine();m.record_changes=True;initial=bytes(m.flash)
    m.command(13,rows([1]),1);m.drain();flash=bytearray(initial)
    recover=[]
    for kind,a,b in m.changes:
        for n in sorted({0,1,len(b)//2,len(b)}):
            cut=bytearray(flash)
            for i,x in enumerate(b[:n]):cut[a+i]=x if kind=='erase' else cut[a+i]&x
            r=StorageMachine(cut);r.limit_reads=1000000
            assert r.checked('sj_recover',budget=2000000)
            recover.append(hashlib.sha256(r.flash).hexdigest())
        for i,x in enumerate(b):flash[a+i]=x if kind=='erase' else flash[a+i]&x
    out['journal-power']={'result':recover,'cost':None}

    for cmd in (10,20):
        for count in (0,1,32):
            cells=[log(i+1) for i in range(count)]
            if cmd==20:
                # Тот же полезный ID в действующем TID2, без совместимости с LOG2 в TEMPID.
                from storage_machine import crc16
                for i,cell in enumerate(cells):
                    body=struct.pack('<H',0x8000)+cell[2:12]+bytes(12)
                    crc=crc16(body[2:]);cells[i]=body+struct.pack('<HHI',crc,crc^65535,0x32444954)
            m=UploadMachine();m.fixture(cmd,cells);before=bytes(m.flash)
            execute(f'upload-{cmd}-{count}',m,lambda:(m.request(),m.drain_upload()))
            assert bytes(m.flash)==before, 'upload modified NOR before ACK'

    m=PhotoMachine(prefix_flash());m.boot_media()
    address,stamp=execute('photo-capture',m,lambda:m.capture(bytes(range(129))))
    def photo_upload():
        m.packet(21,b'');m.drain()
        for _ in range(20):
            m.port();m.drain()
            if len(m.sent)>=2:break
        m.packet(21,struct.pack('<IH',stamp,129));m.drain()
    execute('photo-upload-ack',m,photo_upload)

    for number in (1,2,0):
        m=FirmwareMachine();m.fixture()
        execute('fw-'+str(number),m,lambda:(m.begin(number),m.drain()))
    for stage in ('start','read','poll'):
        m=FirmwareMachine();m.fixture();m.fault=(stage,1)
        execute('fw-failure-'+stage,m,lambda:(m.begin(1),m.drain()))

    m=ExpiryMachine();m.install([record(1),record(2,first=1700000400),record(129)],(0,1,128))
    execute('expiry',m,lambda:(m.start_expiry(),m.expiry_drain()))
    m=ClearMachine();execute('clear',m,lambda:(m.parse(),m.clear_drain()))
    m=IndexMachine();m.setup((1,2,128,256))
    execute('index',m,m.drain_index)



    for match in (True,False):
        m=AccessMachine();m.put_group(1)
        for cell in range(1,50049):m.put_id(cell,b'%010d'%cell)
        ident=0x21000000;m.u.mem_write(ident,b'0000050048' if match else b'9999999999')
        execute('card-full-'+('last' if match else 'miss'),m,
                lambda:m.checked('user_checkIdAndControl',ident,2,budget=12000000))

    for cmd in (17,18):
        m=ServiceMachine(8192,1601536)
        m.stubs.pop(m.addr('usb_try_send'),None);init_transport(m,epoch=0)
        def server():
            m.packet(cmd)
            for _ in range(40000):
                if not m.active():break
                m.now+=1;m.checked('user_storage_poll')
            else:raise AssertionError('service did not complete')
            p=int.from_bytes(m.u.mem_read(m.addr('usb_tx_source'),4),'little')
            n=int.from_bytes(m.u.mem_read(m.addr('usb_tx_length'),2),'little')
            return bytes(m.u.mem_read(p,n)) if p else b''
        execute('packet-'+str(cmd)+'-usb-source',m,server)
    return out


def kernel_scenarios(totals):
    m=support.Machine();m.init();p=Profile(m,totals)
    rng=random.Random(4883384);buf=0x21000000;matrix=0x21000800
    out={};mat=[rng.randrange(65536) for _ in range(16)]
    m.u.mem_write(matrix,struct.pack('<16H',*mat))
    tests=[]
    for off in range(4):
        for n in (0,1,2,3,4,10,31,64,256,1024):
            tests += [('fast_all_ff',(buf+off,n),b'\xff'*(n+4)),
                      ('fast_sldn',(buf+off,n),b'X'*(n+4)),
                      ('crc16_calc',(buf+off,n),rng.randbytes(n+4))]
    for x in (0,1,3,0x8000,0x8080,0xffff):tests.append(('fast_gf2_apply',(matrix,x),b''))
    for _ in range(20):
        data=rng.randbytes(10)
        tests.append(('fast_fnv10',(buf,),data))
        tests.append(('idindex_hash',(rng.randrange(1,50049),buf),data))
    for i,(name,args,data) in enumerate(tests):
        if data:m.u.mem_write(buf,data)
        value=p.run(lambda:m.call(name,*args))
        out[f'{name}/{i}']={'result':value,'cost':p.last,'arguments':args[1:]}
    return out


def main_iteration(totals):
    from storage_machine import StorageMachine
    m=StorageMachine();p=Profile(m,totals)


    for name in ('board_init','NVIC_SetVectorTable','led_load',
                 'timers_init','spi_init','adc_init','delay_ms','timers_pause','wdt_init','user_init','usb_init'):
        if name in m.symbols:m.stubs[m.addr(name)]=m.ret
    visits=[]
    def boundary(u,a,n,_):
        if a!=m.addr('gsm_check_data'):return
        visits.append(p.n)
        if len(visits)==1:
            m.stubs.pop(m.addr('timers_pause'),None)
            p.close();p.n=p.wait=0;p.enabled=True
        else:
            p.close();p.enabled=False;u.reg_write(UC_ARM_REG_PC,m.stop);u.emu_stop()
    m.u.hook_add(UC_HOOK_CODE,boundary)
    m.call('main',budget=2000000)
    assert len(visits)==2
    return {'result':snapshot(m),'cost':{'instructions':p.n,'fetch_estimate':p.n+p.wait},
            'scope':'Actual main: first gsm_check_data entry to next entry; idle peripherals, initialization calls stubbed'}


def coverage(totals):
    """Дополнительные вызовы из существующих граничных сценариев и отказов."""
    import importlib
    original_init=support.Machine.__init__;original_call=support.Machine.call
    def init(m,*args,**kw):
        original_init(m,*args,**kw);m.compare_profile=Profile(m,totals);m.compare_profile.enabled=True
    def call(m,*args,**kw):
        try:return original_call(m,*args,**kw)
        finally:m.compare_profile.close()
    support.Machine.__init__=init;support.Machine.call=call
    out={}
    plans={
        'test_access_reads':('table_cells','holidays','find_id','access'),
        'test_memory_runtime':('cached_reads','config_ownership','config_faults'),
        'test_idindex_runtime':('test_faults','test_cancel_restart','test_real_w25'),
        'test_service_runtime':('faults_and_lifetime','config_reply','ff_affine','pending_timeout','real_w25'),
        'test_upload_runtime':('test_parser','test_gaps_and_faults','test_busy_empty_cancel'),
        'test_idclear_runtime':('sessions_cancel_faults','guards'),
        'test_expiry_runtime':('cancel_and_sessions','faults'),
        'test_fw_service_runtime':('boundaries_errors','io_faults','busy_cancel','real_spi','exact_failure_logs'),
        'test_photo_edges':('cancellation','corrupt','failures','stream','transport_and_heap','ownership','headers_and_space'),
        'elf_contracts':('fw_bridge','usb_queue'),
    }
    try:
        for module,names in plans.items():
            for name in names:
                key=module+'.'+name
                try:
                    import contextlib,io
                    with contextlib.redirect_stdout(io.StringIO()):getattr(importlib.import_module(module),name)()
                    out[key]='PASS'
                except Exception as exc:out[key]=type(exc).__name__+': '+str(exc)
            print('profile',module,len(totals),'exports',flush=True)
    finally:support.Machine.__init__=original_init;support.Machine.call=original_call
    return out


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();select_image();totals={}
    raw=kernel_scenarios(totals);raw.update(scenarios(totals));raw['main-idle']=main_iteration(totals)
    from protocol_paths import run as complete_paths
    raw.update(complete_paths()['scenarios'])
    from test_service_runtime import ServiceMachine
    for channel in (0, 1):
        m = ServiceMachine(); m.stubs.pop(m.addr('user_send_answer_comand'), None)
        for code in (0, 1, 2, 254, 255):
            m.checked('user_send_answer_comand', 21, code, channel, 42)
            raw[f'answer11-photo-{channel}-{code}'] = {'result': m.sent[-1]}
    config=os.environ.get('DOMOVOI_COMPARE_CONFIG','Release')
    expected=json.loads((ROOT/'Tests/fixtures'/f'protocol-{config}.json').read_text(encoding='utf-8'))
    actual={k:{field:serial(v.get(field)) for field in ('result','returned')} for k,v in raw.items()}
    # Меняются только команды камеры; сервер, NOR, события и ответы имеют прежний эталон.
    from test_photo_runtime import pf
    packet_size = pf()['frame'] - pf()['rx']
    commands = [bytes.fromhex(x) for x in (
        'aa0d00000000','aa0e0d000000','aa0100070007')]
    commands += [bytes([0xaa,6,8,packet_size & 255,packet_size >> 8,0]),
                 bytes.fromhex('aa0500000000'),bytes.fromhex('aa0401000000')]
    commands += [bytes([0xaa,14,0,0,i,0]) for i in range((129+packet_size-7)//(packet_size-6))]
    commands.append(bytes.fromhex('aa0e0000f0f0'))
    for name in ('photo-capture','photo-upload-ack'):
        expected[name]['result']['commands'] = serial(commands)
    # Протокольный эталон не меняется. Единственное внутреннее NOR-ожидание:
    # старт больше не стирает всю область неизвестного старого формата.
    from test_upload_runtime import log
    old_image=bytearray(b'\xff'*0x400000)
    old_image[0x1cf400:0x1cf400+128*32]=b''.join(log(i,True) for i in range(1,129))
    expected['log-old-format-discard']['result']['nor']={'bytes':len(old_image),'sha256':hashlib.sha256(old_image).hexdigest()}
    # Не сравниваем хеши разных входных форматов стенда. Неизменность NOR выше
    # проверена побайтно; все байты выдаваемого пакета сверяются с прежним эталоном.
    for count in (0,1,32):
        key=f'upload-20-{count}'
        del actual[key]['result']['nor'];del expected[key]['result']['nor']
    if actual != expected:
        differences = {k: {'actual': actual.get(k), 'expected': expected.get(k)}
                       for k in set(actual) | set(expected) if actual.get(k) != expected.get(k)}
        args.output.with_suffix('.actual.json').write_text(json.dumps(differences, indent=2), encoding='utf-8')
    assert actual == expected, list(differences) if actual != expected else []
    artifact={'elf_sha256':hashlib.sha256(image_paths()[0].read_bytes()).hexdigest(),'cases':len(actual),'mismatches':0}
    args.output.write_text(json.dumps(artifact,indent=2)+'\n',encoding='utf-8')
    print('PASS protocol contract',len(actual))

if __name__=='__main__':main()
