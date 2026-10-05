"""Faults, cancellation and ownership in the linked Thumb-2 photo callers."""
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
    count=0
    for phase in (8,10,11,13,14,15,17,18,20,21):
        m=PhotoMachine(prefix_flash());m.boot_media();old=bytes(m.flash[0x200000:0x200ff7])
        m.begin_camera(58,False);reach(m,phase);committed=m.rget('committed')
        m.checked('photo_storage_cancel');m.drain()
        assert not m.events and not m.pget('stage') and m.io is None
        assert m.flash[0x200000:0x200ff7]==old
        if committed:assert m.flash[0x2ff000:0x300000]==b'\xff'*4096
        count+=1
    for phase in (24,25,26):
        m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request();m.feed(frame(b'x'*58))
        reach(m,phase);committed=m.rget('committed')
        assert not m.events and m.u.mem_read(m.device+m.d['countPhoto'],2)==b'\0\0'
        m.checked('photo_storage_cancel');m.drain()
        assert len(m.events)==int(bool(committed)) and m.io is None
        assert int.from_bytes(m.u.mem_read(m.device+m.d['countPhoto'],2),'little')==int(bool(committed))
        count+=1
    return count

def corrupt():
    for phase,where in ((13,0x2fe000),(20,0x200000)):
        m=PhotoMachine(prefix_flash());m.boot_media();m.begin_camera(58,False);reach(m,phase)
        m.flash[where]^=1;m.drain()
        assert m.rget('fault') and not m.events and not m.pget('stage')
        if phase==13:assert m.flash[0x2ff00c:0x2ff010]==b'\xff'*4
        else:assert m.flash[0x2ff00c:0x2ff010]==struct.pack('<I',0x50484f54)
    return 2

def failures():
    count=0
    for kind in ('read','start','poll'):
        for index in (1,2,3,4,8,16,32,64,100):
            m=PhotoMachine(prefix_flash());m.boot_media();m.begin_camera(58,False)
            m.fault=(kind,m.counts[kind]+index);m.drain()
            assert m.rget('fault') and not m.events and not m.pget('stage'),(kind,index)
            assert m.io is None and not m.active();count+=1
    for kind in ('read','start','poll'):
        m=PhotoMachine();m.boot_media();_,stamp=m.capture(b'x'*700)
        m.packet(21,struct.pack('<IH',stamp,512));before=len(m.answers)
        m.fault=(kind,m.counts[kind]+1);m.drain()
        assert m.rget('fault') and len(m.answers)==before+1 and m.answers[-1][1]==2
        assert not m.pget('pending') and m.io is None;count+=1
    return count

def stream():
    m=PhotoMachine();m.boot_media();m.begin_camera(116);m.next_request();good=frame(b'x'*58)
    bad=bytearray(good);bad[-2]^=1
    for b in (bad,frame(b'x'*58,1),frame(b'')):
        m.feed(b);assert not m.active() and m.pget('written',2)==0 and not m.events
    m.feed(good[:3]);assert m.pget('used',2)==3 and not m.active()
    m.feed(good[3:31]);assert m.pget('used',2)==31 and not m.active()
    m.feed(good[31:]);assert m.active();m.drain();assert m.pget('written',2)==58
    m.now+=60001;m.port();assert not m.events and not m.photo_ptr()
    assert m.flash[0x200000:0x200008]==b'\xff'*8
    # Oversized frame is rejected before reading outside the 64-byte UART area.
    m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request();m.feed(struct.pack('<HH',0,59))
    assert not m.photo_ptr() and not m.events
    return 6

def transport_and_heap():
    m=PhotoMachine();m.boot_media();_,stamp=m.capture(b'x'*768)
    for args in (struct.pack('<IH',stamp,769),struct.pack('<IH',stamp+1,0)):
        m.packet(21,args);m.drain();assert m.answers[-1][1]==1
    m.packet(21,b'',1);m.transport_busy=True;before=len(m.answers);m.drain()
    assert len(m.answers)==before and not m.pget('pending')
    m.transport_busy=False;m.packet(21,b'',1);m.drain();m.port();reach(m,41)
    assert m.rget('tx',4) and m.pget('offset',2)==0
    # Private upload bytes survive unrelated transport slot writes/reconnect.
    m.u.mem_write(0x21000800,b'\xa5'*534);m.transport_busy=True;m.drain()
    assert not m.sent and m.pget('offset',2)==0 and not m.rget('tx',4)
    m.transport_busy=False;m.port();reach(m,41);m.checked('user_comm_session',0);m.drain()
    assert not m.sent and not m.pget('pending') and not m.rget('tx',4)
    m.packet(21,b'');m.drain();held=m.checked('__basic_malloc',3400);assert held
    m.port();m.drain();assert not m.sent and not m.rget('tx',4)
    m.checked('__basic_free',held);m.port();m.drain();assert len(m.sent)==1
    return 6

def ownership():
    m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request();m.feed(frame(b'x'*58));m.tick()
    assert m.io and not m.checked('user_storage_command_ready') and not m.checked('user_storage_flash_ready')
    for cmd in (12,13,19,20,21):
        m.packet(cmd);assert m.answers[-1][1]==2 and m.io
    m.checked('user_savereset');m.drain()
    assert m.u.mem_read(m.addr('store_reset_requested'),1)==b'\1' and not m.events
    # Waiting config and event persistence must win before the next photo job.
    for name in ('store_save_requested','log_pendingCount'):
        m=PhotoMachine();m.boot_media();m.capture(b'x'*58);m.packet(21,b'');m.drain()
        m.write(m.addr(name),1,1);m.write(m.addr('log_timeKnown'),1,1)
        m.write(m.addr('cfg_legacy_checked'),1,1)
        m.port();assert not m.active(),name
        m.tick()
        assert m.checked('mem_config_result')==1 if name=='store_save_requested' else m.checked('log_async_active')
    from test_table_runtime import rows
    m=PhotoMachine();m.boot_media();m.capture(b'p'*116);m.packet(21,b'');m.drain()
    m.command(13,rows([1]),1);assert m.checked('table_runtime_active')
    for _ in range(2000):
        if not m.checked('table_runtime_active'):break
        m.now+=1;m.checked('user_storage_poll');assert m.read_budget<=128 and m.io_budget<=1
    else:raise AssertionError('table sharing did not terminate')
    m.port();m.drain();assert m.sent[-1][20:-2]==b'p'*116
    return 9

def full_cleanup():
    f=bytearray(b'\xff'*0x400000);f[0x200000:0x2fe000]=b'\x55'*(254*4096)
    f[0x2ff020:0x2ff02c]=struct.pack('<III',0,0x2fe000,0xffffffff^0x2fe000)
    m=PhotoMachine(f);m.record_changes=True;m.boot_media();clean(m)
    assert m.flash[0x200000:0x300000]==b'\xff'*0x100000
    assert m.flash[:0x200000]==f[:0x200000] and m.flash[0x300000:]==f[0x300000:]
    erased=[a for kind,a,b in m.changes if kind=='erase']
    assert erased==list(range(0x200000,0x2fe000,4096))+[0x2ff000]
    return {'sectors':254,'passes':len(m.costs),'max_instructions':max(m.costs),'total_instructions':sum(m.costs),'stack':m.stack_peak}

def headers_and_space():
    for prefix in (1034,4087,4094,4095):
        f=prefix_flash(prefix,dirty=prefix<=4087)
        m=PhotoMachine(f);m.boot_media();old=bytes(f[0x200000:0x200000+prefix]);a,stamp=m.capture(b'q'*129)
        assert a==0x200000+prefix and stamp==51 and m.flash[0x200000:a]==old
    # Both invalid sizes are rejected before any erase or program starts.
    for size in (0,65536):
        m=PhotoMachine();m.boot_media();m.packet(7,b'\5');m.drain()
        assert m.pget('stage')==1 and m.checked('photo_prepare',size)==0
        assert not m.counts['start']
    m=PhotoMachine();m.boot_media();m.write(m.device+m.d['newAddressPhoto'],0x2fe000-1033)
    m.packet(7,b'\5');assert m.answers[-1][1]==2 and not m.counts['start']
    return 7

if __name__=='__main__':
    out={}
    for fn in (cancellation,corrupt,failures,stream,transport_and_heap,ownership,full_cleanup,headers_and_space):
        out[fn.__name__]=fn();print(fn.__name__,out[fn.__name__],'PASS',flush=True)
    out['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/photo_edges_results.json').write_text(json.dumps(out,indent=2)+'\n')
