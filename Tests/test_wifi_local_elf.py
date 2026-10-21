"""Р›РѕРєР°Р»СЊРЅС‹Р№ Wi-Fi: СЂРµР°Р»СЊРЅС‹Р№ ELF, РїСЂРѕС‚РѕРєРѕР» USB, Р°С‚РѕРјР°СЂРЅР°СЏ NOR; РѕР±РѕСЂСѓРґРѕРІР°РЅРёРµ РјРѕРґРµР»РёСЂСѓРµС‚СЃСЏ."""
import argparse,hashlib,json,os,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'Tests'),str(ROOT/'Tests/asm')]
from firmware_suite import select_image
from iar_image import image_paths
from iar_layout import fields
from storage_machine import crc16
from test_service_runtime import ServiceMachine
from test_comm_settings_elf import configuration,accepted
from test_flash_executor_steps47 import factory_reset
from test_photo_runtime import camera_bridge_wait

class WifiMachine(ServiceMachine):
    def __init__(self):
        super().__init__();self.changed=0;self.ports=0
        self.u.mem_write(self.addr('config'),bytes(configuration()))
        self.stubs[self.addr('comm_channel_wifi_changed')]=self.change_wifi
        self.stubs[self.addr('rs485_init')]=self.port
        self.stubs[self.addr('comm_uart_init')]=self.ret
        self.stubs[self.addr('comm_uart_abort')]=self.ret
        self.write(self.device+self.d['moduleType'],2,1)
        self.checked('user_comm_boot')
    def change_wifi(self):self.changed+=1;self.ret()
    def port(self):self.ports+=1;self.ret()
    def wifi(self):return bytes(self.u.mem_read(self.device+self.d['wifiModem'],4))
    def command_wifi(self,payload,channel=0):
        body=struct.pack('<HBBB',len(payload)+3,0,73,114)+payload
        self.u.mem_write(0x21000000,body+struct.pack('<H',crc16(body)))
        self.receive_command(0x21000000,channel)
    def finish(self):
        for _ in range(5000):
            self.now+=1;self.checked('user_storage_poll')
            if self.answers:return
        raise AssertionError('USB114 did not finish')

def button():
    from unicorn import UC_HOOK_MEM_READ
    cases=0
    for csr,levels,want in ((0x08000000,(0,0,0),True),(0x08000000,(1,),False),
                            (0x08000000,(0,1),False),(0x08000000,(0,0,1),False),
                            (0x18000000,(),False),(0x28000000,(),False),(0,(),False)):
        m=WifiMachine();reads=[];pauses=[];requests=[];resets=[]
        def gpio(uc,access,address,size,value,opaque):
            level=levels[len(reads)];reads.append(level)
            uc.mem_write(address,struct.pack('<I',level<<14))
        m.u.hook_add(UC_HOOK_MEM_READ,gpio,begin=0x40010808,end=0x4001080b)
        def pause():pauses.append(m.args()[0]);m.ret()
        def request():requests.append(tuple(m.args()));m.ret(1)
        def reset():resets.append(1);m.ret()
        m.stubs[m.addr('timers_pause')]=pause
        m.stubs[m.addr('store_factory_request')]=request
        m.stubs[m.addr('user_factoryReset')]=reset
        m.checked('user_factory_button',csr)
        assert bool(requests)==want and bool(resets)==want
        assert len(reads)==len(levels) and pauses==[50]*(max(0,len(levels)-1))
        if want:assert requests==[(4,0,1,0)]
        cases+=1
    m=WifiMachine();m.checked('board_init')
    assert (int.from_bytes(m.u.mem_read(0x40010804,4),'little')>>20)&255==0x88,'PA13/14 pull-up input'
    assert int.from_bytes(m.u.mem_read(0x40010810,4),'little')&0x6000==0x6000,'PA13/14 pull-up high'
    return cases

def restored_fields():
    m=WifiMachine();d=m.d
    before=bytes([0xaa])*512;saved=bytearray((i*13+1)&255 for i in range(512))
    saved[d['IMEIGSM']:d['IMEIGSM']+16]=b'123456789012345\0'
    saved[d['wifiModem']:d['wifiModem']+4]=bytes((1,2,1,0x57))
    expected=bytearray(before)
    ranges=[(d['IMEIGSM'],16),(d['hardwareVersion'],d['datetime']-d['hardwareVersion']),
            (d['firmwareSize'],d['readLogIDIndex']-d['firmwareSize']),
            (d['triggers'],16),(d['FWblockIdx'],2),(d['FWinProgress'],1),
            (d['updateTimeout'],2),(d['moduleType'],1),(d['wifiModem'],4)]
    for start,count in ranges:expected[start:start+count]=saved[start:start+count]
    expected[d['status']+1]=(expected[d['status']+1]&127)|(saved[d['status']+1]&128)
    m.u.mem_write(m.device,before);m.u.mem_write(0x21000400,bytes(saved))
    for fn in ('upload_restore','sched16_restore'):m.stubs[m.addr(fn)]=m.ret
    m.checked('user_restore_device',0x21000400)
    assert bytes(m.u.mem_read(m.device,512))==expected,'metadata restore touched runtime fields'
    c=fields('config_s');names=('server1','server2','NTPserver','netOperator','countryCode','simPhone','ussdBalance')
    sizes=(32,32,32,32,8,16,8)
    # Все семь ограниченных строк проверяются одновременно; остальные байты не меняются.
    for terminated in (False,True):
        raw=bytearray(b'A'*512);expected=bytearray(raw)
        for name,size in zip(names,sizes):
            at=c[name]
            if terminated:raw[at+1]=expected[at+1]=0
            else:expected[at+size-1]=0
        m.u.mem_write(m.addr('config'),bytes(raw));m.checked('config_terminate_strings')
        actual=bytes(m.u.mem_read(m.addr('config'),512))
        assert actual[:510]==expected[:510], 'string bounds changed'
        if not terminated:assert int.from_bytes(actual[510:],'little')==crc16(actual[:510])
    return 'all persistent fields preserved; all seven string boundaries'

def run():
    d=fields('device_s');assert d['crc']==510 and d['wifiModem']==d['reserv']+40
    for model,policy in ((0,0),(1,0),(1,1),(1,2)):
        m=WifiMachine();before=m.wifi();m.command_wifi(bytes((model,policy)))
        assert m.wifi()==before and not m.answers,'applied before NOR verify'
        m.finish();assert m.answers[-1]==(114,0,0,73),m.answers
        expected=bytes((model,policy,0,0x57));assert m.wifi()==expected
        active=m.select_config();assert bytes(m.flash[active+512+d['wifiModem']:active+512+d['wifiModem']+4])==expected
        assert m.changed==1
        # РљРѕРјР°РЅРґР° 12 СЃ СЃРµСЂРІРµСЂР° Рё С‚Р°Р±Р»РёС†РµР№ РєР°РјРµСЂР°+РјРѕСЃС‚ РЅРµ РїРµСЂРµРїРёСЃС‹РІР°РµС‚ Р»РѕРєР°Р»СЊРЅС‹Р№ РІС‹Р±РѕСЂ.
        candidate=configuration();cf=fields('config_s');candidate[cf['rs485Periphery']:cf['rs485Periphery']+4]=bytes((1,1,5,3))
        accepted(m,candidate,1);assert m.wifi()==expected
        # РќРµР·Р°РІРёСЃРёРјС‹Р№ СЃС‚Р°СЂС‚ С‡РёС‚Р°РµС‚ С‚РѕР»СЊРєРѕ РІР°Р»РёРґРЅСѓСЋ Р»РѕРєР°Р»СЊРЅСѓСЋ Р·Р°РїРёСЃСЊ.
        saved=0x21000400;m.u.mem_write(saved,bytes(m.u.mem_read(m.device,512)))
        m.u.mem_write(m.device+d['wifiModem'],bytes(4));m.checked('user_restore_device',saved)
        assert m.wifi()==expected
        # Полная инициализация переменных должна читать локальный выбор ДО старта связи.
        def restore():m.u.mem_write(m.device+d['wifiModem'],expected);m.ret()
        m.stubs[m.addr('user_readsavedinformation')]=restore
        for fn in ('user_ids_boot','log_getRecordsCount','user_getTempIDRecordsCount'):
            m.stubs[m.addr(fn)]=lambda:m.ret(0)
        m.stubs[m.addr('w25_read_devID')]=lambda:m.ret(0x17c2)
        m.checked('user_initvariables')
        assert bool(m.checked('comm_channel_wifi'))==bool(model),'startup missed local Wi-Fi choice'
        m.write(m.addr('config')+cf['useVoLTE'],1,1)
        assert m.checked('user_comm_volte')==(0 if model else 1)
    for payload in (b'',b'\1',b'\2\0',b'\1\3',b'\0\1',b'\1\1\0'):
        m=WifiMachine();m.command_wifi(payload);assert m.answers[-1][1]==1 and not m.counts['start']
    m=WifiMachine();m.command_wifi(b'\1\2',1);assert not m.answers and not m.counts['start'] and m.wifi()==bytes(4)
    m=WifiMachine();m.fault=('poll',1);m.command_wifi(b'\1\2');m.finish();assert m.answers[-1][1]==2 and m.wifi()==bytes(4) and not m.changed
    m=WifiMachine();m.u.mem_write(m.device+d['wifiModem'],bytes((1,1,0,0x57)));m.command_wifi(b'\0\0');m.finish()
    assert m.wifi()==bytes((0,0,1,0x57)),'shutdown debt not durable'
    m.checked('user_comm_wifi_stopped',0);assert m.wifi()[2]==1
    m.checked('user_comm_wifi_stopped',1);assert m.wifi()[2]==0
    # РЎС‚Р°СЂС‹Рµ / СЃС‚С‘СЂС‚С‹Рµ reserve РЅРµ РІРєР»СЋС‡Р°СЋС‚ РјРѕРґРµРј.
    for raw in (bytes(4),b'\xff'*4,bytes((1,3,0,0x57)),bytes((2,1,0,0x57))):
        m=WifiMachine();saved=0x21000400;m.u.mem_write(saved,bytes(512));m.u.mem_write(saved+d['wifiModem'],raw)
        m.checked('user_restore_device',saved);assert not m.wifi()[0]
    return {'local':'USB114 persist/verify/failure/legacy/VoLTE', 'factory':factory_reset(),'button':button(),'metadata':restored_fields(),'camera':camera_bridge_wait()}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build-root',type=Path,required=True);p.add_argument('--config',default='ReleaseCustomer');a=p.parse_args()
    os.environ['DOMOVOI_COMPARE_CONFIG']=a.config;select_image(a.build_root)
    result=run();result['elf_sha256']=hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()
    (a.build_root/('wifi-local-'+a.config+'.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PASS local Wi-Fi ELF: persistence, server isolation, reset sources, camera and VoLTE')
