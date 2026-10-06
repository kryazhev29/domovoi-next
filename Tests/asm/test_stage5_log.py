"""Final-ELF sector ring, including old cursor-loss and torn-erase counterexamples."""
from storage_machine import StorageMachine,struct
from log_ring_fixture import install,valid,BASE,CAPACITY
from test_upload_runtime import log

def boot(m):
    before=m.counts['read'];n=m.checked('log_getRecordsCount',budget=20000000)
    assert n!=65535
    return n,m.counts['read']-before

def append(m,index):
    m.u.mem_write(0x21000400,log(index))
    m.checked('log_async_begin',0x21000400)
    for _ in range(400):
        m.now+=1;r=m.checked('log_async_step',m.now)
        assert m.read_budget<=64 and m.io_budget<=1
        if r==2:return
        assert r!=3,('append failure',m.now)
    raise AssertionError('append stuck')

def records(m):
    result=[];a=m.checked('log_ring_first')
    while a:
        cell=bytes(m.flash[a:a+32])
        if valid(cell):result.append(int.from_bytes(cell[:4],'little'))
        a=m.checked('log_ring_next',a)
    return result

def run():
    m=StorageMachine();assert boot(m)[0]==0
    append(m,1);assert records(m)==[1]
    m=StorageMachine();install(m,[log(i) for i in range(1,CAPACITY+1)])
    n,reads=boot(m);assert n==CAPACITY and reads<=24+7+127+2,(n,reads)
    append(m,CAPACITY+1)
    expected=list(range(128,CAPACITY+2));assert records(m)==expected
    assert boot(StorageMachine(m.flash))[0]==len(expected)
    for i in range(CAPACITY+2,CAPACITY+130):append(m,i)
    result=records(m);assert result==list(range(255,CAPACITY+130))
    return {'capacity':CAPACITY,'boot_read_calls':reads,'rotations':2,'sequence':result[-1]}

def power():
    initial=StorageMachine();install(initial,[log(i) for i in range(1,CAPACITY+1)]);boot(initial)
    before=bytes(initial.flash);initial.record_changes=True;append(initial,CAPACITY+1)
    image=bytearray(before);cuts=0
    for kind,a,b in initial.changes:
        for n in sorted({0,1,len(b)//2,len(b)-1,len(b)}):
            cut=bytearray(image)
            for j,v in enumerate(b[:n]):cut[a+j]=v if kind=='erase' else cut[a+j]&v
            m=StorageMachine(cut);count,_=boot(m);ids=records(m)
            assert ids in [list(range(1,CAPACITY+1)),list(range(128,CAPACITY+1)),list(range(128,CAPACITY+2))],(kind,hex(a),n,ids[:3],ids[-3:])
            assert count==len(ids)
            append(m,1);out=records(m);assert out[-1]>CAPACITY and out==sorted(set(out))
            cuts+=1
        for j,v in enumerate(b):image[a+j]=v if kind=='erase' else image[a+j]&v
    return {'rotation_cuts':cuts}

def migration():
    totals=[]
    for count in (0,1,126,127,128,3048,3072):
        m=StorageMachine();cells=[log(i) for i in range(1,count+1)]
        m.flash[0x1cf400:0x1cf400+count*32]=b''.join(cells)
        n,_=boot(m)
        assert n==0 and records(m)==[],(count,n)
        assert m.flash[BASE:BASE+98304]==b'\xff'*98304
        append(m,count+1);assert records(m)==[count+1]
        r=StorageMachine(m.flash);assert boot(r)[0]==1 and records(r)==[count+1]
        totals.append(count)
    return {'legacy_counts':totals}


def cursor_loss():
    from test_upload_runtime import UploadMachine,wire
    # Full-ring replacement erased the first sector. A stale logical cursor 129
    # must upload record 129, not ordinal 129 of the surviving ring (=256).
    m=UploadMachine();m.cmd=10;m.base=BASE
    install(m,[log(i) for i in range(1,CAPACITY+1)]);boot(m);append(m,CAPACITY+1)
    m.write(m.addr('log_cursor32'),129)
    m.request(0,7);m.drain_upload()
    assert m.sent==[wire(10,7,0,[log(i) for i in range(129,161)],CAPACITY+1-128)]
    assert int.from_bytes(m.u.mem_read(m.addr('log_pending32'),4),'little')==161
    m.request(0,8)
    for _ in range(12000):
        m.tick_upload()
        if len(m.sent)==2:break
    else:raise AssertionError('logical ACK stuck')
    assert int.from_bytes(m.u.mem_read(m.addr('log_cursor32'),4),'little')==161
    # Confirmed checkpoint is actually persisted and restored after reboot.
    r=UploadMachine();r.flash[:]=m.flash;r.cmd=10;r.base=BASE
    selected=r.checked('cfg_select',0x21000000)
    assert selected
    r.u.mem_write(0x21000000,bytes(r.flash[selected+512:selected+1024]))
    r.checked('upload_restore',0x21000000)
    assert int.from_bytes(r.u.mem_read(r.addr('log_cursor32'),4),'little')==161
    # Exact original example: numbers 1..128 are gone (1 was already absent,
    # the 127-slot tail held 2..128). An erased cursor replays from 129, not 257.
    t=UploadMachine();t.cmd=10;t.base=BASE
    install(t,[log(i) for i in range(2,CAPACITY+2)]);boot(t);append(t,CAPACITY+2)
    t.write(t.addr('log_cursor32'),1);t.request(0,7);t.drain_upload()
    assert t.sent==[wire(10,7,0,[log(i) for i in range(129,161)],CAPACITY+2-128)]
    return {'stale_cursor':129,'first_sent':129,'next_confirmed':161,
            'lost_numbers_1_to_128_first_sent':129}


def legacy_checkpoint():
    from test_upload_runtime import UploadMachine,wire
    m=UploadMachine();m.cmd=10;m.base=BASE
    m.flash[0x1cf400:0x1cf460]=b''.join(log(i) for i in range(101,104))
    saved=bytearray(512);off=m.d['reserv']
    struct.pack_into('<IH',saved,off,0x314c5055,2)
    struct.pack_into('<H',saved,m.d['readLogIDIndex'],1)
    m.u.mem_write(0x21000000,bytes(saved));m.checked('upload_restore',0x21000000)
    assert tuple(struct.unpack('<2I',m.u.mem_read(m.addr('log_cursor32'),8)))==(0,0)
    assert boot(m)[0]==0
    # Formatting persists empty cursors and keeps any saved logical last index.
    for _ in range(2000):
        m.now+=1;m.checked('user_storage_poll')
        if not m.u.mem_read(m.addr('store_save_requested'),1)[0] and not m.checked('mem_config_result') and not m.u.mem_read(m.addr('flash_owner'),1)[0]:break
    m.request();m.drain_upload();assert m.sent==[wire(10,7,0,[],0)]
    saved=bytearray(512);struct.pack_into('<4I',saved,off+16,0x354c5055,103,102,900)
    m=UploadMachine();m.cmd=10;m.base=BASE
    m.u.mem_write(0x21000000,bytes(saved));m.checked('upload_restore',0x21000000)
    assert boot(m)[0]==0
    append(m,1);assert records(m)==[901]
    m.request();m.drain_upload();assert m.sent==[wire(10,7,0,[struct.pack('<I',901)+log(1)[4:]],1)]
    return {'old_ordinals':'discarded','empty_upload':'verified','saved_last_index':900,'first_new':901}


def migration_power():
    original=StorageMachine();original.flash[BASE:BASE+98304]=b'\x55'*98304
    before=bytes(original.flash);original.record_changes=True;assert boot(original)[0]==0
    image=bytearray(before);cuts=0
    for kind,a,b in original.changes:
        assert kind=='erase' and BASE<=a<BASE+98304 and len(b)==4096
        for n in (0,1,2048,4095,4096):
            cut=bytearray(image);cut[a:a+n]=b[:n]
            m=StorageMachine(cut);count,_=boot(m)
            assert count==0 and records(m)==[],(hex(a),n,count)
            append(m,129);assert records(m)==[129]
            cuts+=1
        for j,v in enumerate(b):image[a+j]=v if kind=='erase' else image[a+j]&v
    assert cuts==120
    return {'format_erase_cuts':cuts,'surviving_records':0,'append_after_each_cut':'verified'}


def record_commit():
    original=StorageMachine();append(original,1);before=bytes(original.flash)
    original.record_changes=True;append(original,2);image=bytearray(before);cuts=0
    for kind,a,b in original.changes:
        assert kind=='write' and len(b) in (28,4)
        for n in range(len(b)+1):
            cut=bytearray(image)
            for j,v in enumerate(b[:n]):cut[a+j]&=v
            m=StorageMachine(cut);count,_=boot(m)
            expected=[1,2] if len(b)==4 and n==4 else [1]
            assert records(m)==expected and count==len(expected),(len(b),n,count)
            append(m,3);assert records(m)==expected+[3]
            cuts+=1
        for j,v in enumerate(b):image[a+j]&=v
    m=StorageMachine();install(m,[log(1,True),log(2)])
    assert boot(m)[0]==1 # Old cells are rejected even inside a valid sector.
    m.flash[BASE+32+5]^=1;assert boot(m)[0]==1
    m.flash[BASE+64+5]^=1;assert boot(m)[0]==0
    m.fault=('read',m.counts['read']+1);assert m.checked('log_getRecordsCount')==65535
    return {'every_record_byte_cut':cuts,'legacy_cell':'rejected','wire_reserve':'zero','CRC_corruption':'rejected','read_error':'reported'}


def generation_bytes():
    for seq in (234,255,65512,0xffffffb0):
        m=StorageMachine();install(m,[log(i) for i in range(1,CAPACITY+1)],first_seq=seq)
        assert boot(m)[0]==CAPACITY
        append(m,CAPACITY+1)
        expected=list(range(128,CAPACITY+2));assert records(m)==expected
        r=StorageMachine(m.flash);assert boot(r)[0]==len(expected) and records(r)==expected
    return {'first_generations':[234,255,65512,0xffffffb0],'full_NOR_generation':'preserved','low_byte_wrap':'verified'}

def count_ends():
    from test_upload_runtime import temp
    class BootMachine(StorageMachine):
        def read(self):
            a,p,n,_=self.args()
            if n<=256:return super().read()
            assert n==1024 and a+n<=len(self.flash)
            self.read_budget+=n
            if self.failed('read'):self.ret(0);return
            self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    for n in range(129):
        m=BootMachine();install(m,[log(i+1) for i in range(n)])
        m.u.mem_write(m.addr('mem')+8*8,struct.pack('<II',0x1e8000,4096))
        m.flash[0x1e8000:0x1e8000+32*n]=b''.join(temp(i+1) for i in range(n))
        assert boot(m)[0]==n
        expected=BASE+32 if not n else BASE+((n-1)//127)*4096+32+((n-1)%127+1)*32
        assert int.from_bytes(m.u.mem_read(m.device+m.d['newAddressLog'],4),'little')==expected
        assert m.checked('user_getTempIDRecordsCount',budget=2000000)==n
        assert int.from_bytes(m.u.mem_read(m.device+m.d['newAddressTempID'],4),'little')==0x1e8000+32*n
    return {'counts':129,'ring_boundary':127,'temporary_ID_boundary':128}


def erased_generation():
    m=StorageMachine();install(m,[log(i) for i in range(1,CAPACITY+1)])
    # An interrupted NOR erase can set a zero bit in the old generation while
    # retaining magic. With magic+seq only, sector 0 falsely overtakes seq 24.
    m.flash[BASE+4]|=128
    bare=[struct.unpack_from('<II',m.flash,BASE+4096*s) for s in range(24)]
    assert max(range(24),key=lambda s:bare[s][1])==0
    count,_=boot(m)
    assert count==CAPACITY-127 and records(m)==list(range(128,CAPACITY+1))
    append(m,1);assert records(m)[-1]==CAPACITY+1
    return {'bare_header_false_head':0,'verified_head_generation':24,'last_record_after_append':CAPACITY+1}
