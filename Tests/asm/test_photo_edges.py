"""Доработка: Кряжев А., 2026. Границы, ошибки и владение флеш фото."""
import hashlib,json
from test_photo_runtime import *
from test_photo_power import clean

def reach(m,phase,limit=2500):
    for _ in range(limit):
        if m.rget('phase')==phase and m.io is None:return
        assert m.active() and not m.rget('fault'),(phase,m.rget('phase'))
        m.tick()
    raise AssertionError(('unreachable',phase))

def frame(data,packet=0):
    b=struct.pack('<HH',packet,len(data))+data
    return b+bytes([sum(b)&255,0])

def cancellation():
    for name in ('DATA_DONE','PHOTO_MARK','PHOTO_DONE'):
        m=PhotoMachine(prefix_flash());m.boot_media();old=bytes(m.flash[0x200000:0x201000])
        m.begin_camera(58);m.next_request();m.feed(frame(b'x'*58))
        reach(m,PHASE[name]);committed=m.rget('committed')
        m.checked('photo_storage_cancel');m.drain()
        assert len(m.events)==int(bool(committed)) and m.io is None
        assert m.flash[0x200000:0x201000]==old
        assert int.from_bytes(m.u.mem_read(m.device+m.d['countPhoto'],2),'little')==1+int(bool(committed))
    return 3

def corrupt():
    for offset,value in ((8,0),(4,0),(4,65536)):
        f=prefix_flash();f[0x200000+offset:0x200004+offset]=struct.pack('<I',value)
        m=PhotoMachine(f);m.boot_media();assert not m.rget('fault')
        assert m.flash[0x200000:0x201000]==b'\xff'*4096
    for number in (49,50,0xffffffff):
        f=prefix_flash();f[0x201000:0x201000+HEAD]=struct.pack('<IIIB',number,58,FORMAT,255)
        m=PhotoMachine(f);m.boot_media()
        assert int.from_bytes(m.u.mem_read(m.device+m.d['countPhoto'],2),'little')==1
        assert int.from_bytes(m.u.mem_read(m.device+m.d['newAddressPhoto'],4),'little')==0x201000
    return 6

def failures():
    for kind in ('read','start','poll'):
        m=PhotoMachine();m.boot_media();_,stamp=m.capture(b'x'*700)
        m.packet(21,struct.pack('<IH',stamp,700));before=len(m.answers)
        m.fault=(kind,m.counts[kind]+1);m.drain()
        assert m.rget('fault') and len(m.answers)==before+1 and m.answers[-1][1]==2
        assert not m.pget('pending') and m.io is None
    for kind in ('start','poll'):
        m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request()
        m.fault=(kind,m.counts[kind]+1);m.feed(frame(b'x'*58));m.drain()
        assert m.rget('fault') and not m.events and m.io is None
    return 5

def stream():
    m=PhotoMachine();m.boot_media();m.begin_camera(116);m.next_request();good=frame(b'x'*58)
    bad=bytearray(good);bad[-2]^=1
    for b in (bad,frame(b'x'*58,1),frame(b'')):
        m.feed(b);assert not m.active() and m.pget('written',2)==0 and not m.events
    for part in (good[:3],good[3:31],good[31:]):m.feed(part)
    assert m.active();m.drain();assert m.pget('written',2)==58
    m.now+=60001;m.port();assert not m.events and not m.photo_ptr()
    assert m.flash[0x200000:0x200008]==b'\xff'*8
    for length in (59,255,256,65529,65530,65531,65532,65533,65534,65535):
        m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request()
        m.feed(struct.pack('<HH',0,length))
        assert not m.photo_ptr() and not m.events
        assert m.flash[0x200000:0x200000+HEAD]==b'\xff'*HEAD
    return 15

def transport_and_heap():
    m=PhotoMachine();m.boot_media();_,stamp=m.capture(b'x'*768)
    for args in (struct.pack('<IH',stamp,769),struct.pack('<IH',stamp+1,0)):
        m.packet(21,args);m.drain();assert m.answers[-1][1]==1
    m.packet(21,b'',1);m.transport_busy=True;before=len(m.answers);m.drain()
    assert len(m.answers)==before and not m.pget('pending')
    m.transport_busy=False;m.packet(21,b'',1);m.drain();m.port();reach(m,PHASE['SEND_COMMIT'])
    assert m.rget('tx',4) and m.pget('offset',2)==0
    m.u.mem_write(0x21000800,b'\xa5'*534);m.transport_busy=True;m.drain()
    assert not m.sent and m.pget('offset',2)==0 and not m.rget('tx',4)
    m.transport_busy=False;m.port();reach(m,PHASE['SEND_COMMIT']);m.checked('user_comm_session',0);m.drain()
    assert not m.sent and not m.pget('pending') and not m.rget('tx',4)
    m.packet(21,b'');m.drain();held=take(m,REPLY);assert held
    m.port();m.drain();assert not m.sent and not m.rget('tx',4)
    drop(m,REPLY,held);m.port();m.drain();assert len(m.sent)==1
    return 6

def ownership():
    m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request();m.feed(frame(b'x'*58));m.tick()
    assert m.io and not m.checked('user_storage_command_ready') and not m.checked('user_storage_flash_ready')
    for cmd in (12,13,19,20,21):
        m.packet(cmd);assert m.answers[-1][1]==2 and m.io
    m.checked('user_savereset');m.drain()
    assert m.u.mem_read(m.addr('store_reset_requested'),1)==b'\1' and not m.events
    for name in ('store_save_requested','log_pendingCount'):
        m=PhotoMachine();m.boot_media();m.capture(b'x'*58);m.packet(21,b'');m.drain()
        m.write(m.addr(name),1,1);m.write(m.addr('log_timeKnown'),1,1);m.write(m.addr('cfg_boot_done' if 'cfg_boot_done' in m.symbols else 'cfg_legacy_checked'),1,1)
        m.port();assert not m.active(),name
        m.tick()
        assert m.checked('mem_config_result')==1 if name=='store_save_requested' else m.checked('log_async_active')
    return 8

def full_cleanup():
    f=prefix_flash(dirty=False);f[0x200000+HEAD-1]=0
    f[0x201000:0x300000]=b'\x55'*(255*4096)
    m=PhotoMachine(f);m.boot_media();m.record_changes=True;clean(m)
    assert m.flash[0x200000:0x201000]==b'\xff'*4096
    assert m.flash[:0x200000]==f[:0x200000] and m.flash[0x201000:]==f[0x201000:]
    assert [(kind,a) for kind,a,b in m.changes]==[('erase',0x200000)]
    return {'sectors':1,'passes':len(m.costs),'stack':m.stack_peak}

def headers_and_space():
    for prefix in (1034,4087,4094,4095):
        f=prefix_flash(prefix);m=PhotoMachine(f);m.boot_media();old=bytes(f[0x200000:0x201000])
        a,stamp=m.capture(b'q'*129)
        assert a==0x201000 and stamp==51 and m.flash[0x200000:a]==old
    for size in (0,65536):
        m=PhotoMachine();m.boot_media();m.packet(7,b'\5');m.drain()
        assert m.pget('stage')==1 and m.checked('pr_prepare',size)==0 and not m.counts['start']
    f=prefix_flash();f[0x200000:0x200004]=struct.pack('<I',0xfffffffe)
    m=PhotoMachine(f);m.boot_media();m.packet(7,b'\5');assert m.answers[-1][1]==2
    m=PhotoMachine();m.boot_media();m.write(m.device+m.d['newAddressPhoto'],0x300000)
    m.packet(7,b'\5');assert m.answers[-1][1]==2
    f=bytearray(b'\xff'*0x400000)
    f[0x201000:0x201000+HEAD]=struct.pack('<IIIB',0xfffffffe,58,FORMAT,0)
    m=PhotoMachine(f);m.boot_media();m.begin_camera(58);m.next_request();m.feed(frame(b'x'*58));m.drain()
    assert m.rget('fault') and not m.events and m.flash[0x200000:0x200000+HEAD]==b'\xff'*HEAD
    assert m.flash[0x201000:]==f[0x201000:]
    return 9

if __name__=='__main__':
    out={fn.__name__:fn() for fn in (cancellation,corrupt,failures,stream,transport_and_heap,ownership,full_cleanup,headers_and_space)}
    print(json.dumps(out,indent=2));print('PASS photo edges')
