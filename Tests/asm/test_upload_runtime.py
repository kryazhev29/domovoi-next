from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Cooperative command 9/10 upload in the final IAR ELF, modeled NOR/TX only."""
import hashlib, json, struct
from storage_machine import *


def temp(i):
    return struct.pack('<HIIH', i, i * 17, i * 19, i) + bytes(20)


def log(i, legacy=False):
    body = struct.pack('<II', i, 1700000000 + i) + bytes(range(16))
    crc = crc16(body)
    return body + (bytes(8) if legacy else struct.pack('<HHI', crc, crc ^ 65535, 0x32474f4c))


class UploadMachine(StorageMachine):
    def __init__(self):
        super().__init__()
        self.write(self.addr('cfg_legacy_checked'),1,1)
        self.sent = []; self.busy = False; self.commit_fail = False; self.lease = False
        self.acquires = self.releases = self.max_read = self.total_read = 0
        self.u.mem_write(self.addr('mem') + 6*8, struct.pack('<II', 0x1cd400, 4096))
        for name, fn in [('usb_send', self.usb), ('comm_tx_acquire', self.acquire),
                         ('comm_tx_commit', self.commit), ('comm_tx_release', self.release),
                         ('comm_tx_busy', lambda:self.ret(self.busy or self.lease))]:
            self.stubs[self.addr(name)] = fn

        if 'usb_try_send' in self.symbols:
            self.stubs[self.addr('usb_try_send')] = self.usb

    def usb(self):
        p, n, _, _ = self.args()
        assert n <= 1038
        if not self.busy:
            self.sent.append(bytes(self.u.mem_read(p, n)))
        self.ret(not self.busy)

    def acquire(self):
        self.acquires += 1
        self.write(self.args()[0], 1040, 2)
        assert not self.lease
        if self.busy:
            self.ret(0)
        else:
            self.lease = True; self.ret(0x21000800)

    def commit(self):
        assert self.lease
        if self.commit_fail:
            self.ret(0); return
        self.sent.append(bytes(self.u.mem_read(0x21000800, self.args()[0])))
        self.lease = False; self.ret(1)

    def release(self):
        assert self.lease, 'released a TX frame this upload did not own'
        self.releases += 1; self.lease = False; self.ret()

    def ptr(self):
        return self.read_word(self.addr('upload_runtime_context'))

    def read_word(self, at, n=4):
        return int.from_bytes(self.u.mem_read(at, n), 'little')

    def phase(self):
        return self.read_word(self.ptr()+35, 1) if self.ptr() else 0

    def fixture(self, cmd, slots):
        self.cmd = cmd; self.base = (0x1d0000 if "log_cursor32" in self.symbols else 0x1cf400) if cmd == 10 else 0x1e8400
        size = 98304 if cmd == 10 else 49152
        self.u.mem_write(self.addr('mem') + (7 if cmd == 10 else 8)*8, struct.pack('<II', self.base, size))
        if cmd==10 and "log_cursor32" in self.symbols:
            from log_ring_fixture import install
            install(self,slots);return
        for i, cell in enumerate(slots):
            self.flash[self.base+i*32:self.base+(i+1)*32] = cell

    def request(self, channel=0, uniq=7):
        self.checked('user_send_logRecords' if self.cmd == 10 else 'user_send_tempIDs', channel, uniq)

    def tick_upload(self):
        upload_active = bool(self.ptr())
        self.now += 1
        before = self.u.reg_read(UC_ARM_REG_PRIMASK)
        self.checked('user_storage_poll')
        assert self.u.reg_read(UC_ARM_REG_PRIMASK) == before
        assert self.read_budget <= (64 if upload_active else 256) and self.io_budget <= 1, (upload_active,self.read_budget,self.io_budget,self.phase())
        assert not self.lease, 'shared TX frame escaped a single poll'
        self.max_read = max(self.max_read, self.read_budget)
        self.total_read += self.read_budget

    def drain_upload(self, limit=12000):
        for _ in range(limit):
            if not self.ptr():
                return
            self.tick_upload()
        raise AssertionError(('upload stuck', self.phase()))

    def set_cursor(self,channel,ordinal):
        if self.cmd==10 and "log_cursor32" in self.symbols:self.write(self.addr("log_cursor32")+4*channel,ordinal+1 if ordinal else 0)
        else:self.checked("upload_setIndex",self.cmd,channel,ordinal)

    def cursor(self,channel=0):
        if self.cmd==10 and "log_cursor32" in self.symbols:return max(0,self.read_word(self.addr("log_cursor32")+4*channel)-1)
        return self.checked("upload_index",self.cmd,channel)

    def seed_pending(self,ordinal):
        if self.cmd==10 and "log_pending32" in self.symbols:self.write(self.addr("log_pending32"),ordinal+1)
        else:self.write(self.addr("log_unacked" if self.cmd==10 else "temp_unacked"),ordinal,2)

    def pending_cursor(self, channel=0):
        # Legacy wire scenarios use sequential record IDs, starting at 1.
        if self.cmd==10 and "log_pending32" in self.symbols:return max(0,self.read_word(self.addr("log_pending32")+4*channel)-1)
        return self.read_word(self.addr('log_unacked' if self.cmd == 10 else 'temp_unacked')+channel*2, 2)


def wire(cmd, uniq, first, cells, total=None):
    if not cells:
        data = b'\0'*3  # Protocol2: left (2) + count (1), answers 9 and 10 alike
        body = struct.pack('<HBBB', len(data)+3, 255, uniq, cmd)+data
    else:
        count = len(cells); total = total if total is not None else count
        payload = b''.join(c[:24]+bytes(8) if cmd == 10 else c[2:12] for c in cells)
        body = struct.pack('<HBBBHB', len(payload)+6, 255, uniq, cmd, total-first-count, count)+payload
    return b'SLDN'+body+struct.pack('<H', crc16(body))


def test_normal():
    results = {}
    for cmd, n, first, ch in [(9,1,0,0),(9,1536,0,1),(9,1536,1500,0),
                             (10,1,0,0),(10,3048,0,1),(10,3048,3047,0)]:
        m=UploadMachine(); cells=[(log(i+1) if cmd==10 else temp(i+1)) for i in range(n)]
        m.fixture(cmd,cells); m.set_cursor(ch,first)
        m.u.reg_write(UC_ARM_REG_PRIMASK,1)
        m.request(ch); assert m.ptr() and not m.sent and not m.counts['read']
        m.drain_upload(); limit=32 if cmd==10 else 100
        assert m.sent == [wire(cmd,7,first,cells[first:first+limit],n)], (cmd,n,first,ch,[p.hex() for p in m.sent])
        assert m.pending_cursor(ch)==min(n,first+limit)
        assert m.cursor(ch)==first, 'send advanced an unacknowledged cursor'
        results[str((cmd,n,first,ch))]=dict(passes=len(m.costs),maximum=max(m.costs),total=sum(m.costs),
            stack=m.stack_peak,read_bytes=m.total_read,max_read=m.max_read)
    return results


def test_gaps_and_faults():
    m=UploadMachine(); cells=[log(1),bytes(24)+b'\xff'*8,log(2),b'\xff'*32,log(3)]
    m.fixture(10,cells);m.request();m.drain_upload()
    assert m.sent == [wire(10,7,0,[log(1),log(2)],2)]
    count=1
    for cmd in (9,10):
        for point in (1,2,4,5,6):
            m=UploadMachine();m.fixture(cmd,[(log(i) if cmd==10 else temp(i)) for i in (1,2,3)])
            m.fault=('read',point);m.request();m.drain_upload()
            if m.counts['read']<point:continue
            assert not m.sent and not m.pending_cursor() and not m.ptr()
            count+=1
    return count


def test_parser():
    # Existing wire protocol is deliberately asymmetric: request 9 -> reply 10
    # (logs), request 10 -> reply 9 (temporary IDs).
    for reply in (9,10):
        m=UploadMachine();cell=temp(1) if reply==9 else log(1);m.fixture(reply,[cell])
        body=struct.pack('<HBBB',3,0,7,19-reply);body+=struct.pack('<H',crc16(body))
        m.u.mem_write(0x21000000,body);m.checked('user_parse_comand',0x21000000,0)
        m.u.mem_write(0x21000000,b'\xA5'*len(body));assert m.ptr()
        m.drain_upload();assert m.sent==[wire(reply,7,0,[cell])]
    return 2


def test_media_gate():
    count=0
    for blocked in ('not-ready','journal-fault'):
        for reply in (9,10):
            m=UploadMachine();m.fixture(reply,[temp(1) if reply==9 else log(1)])
            if blocked=='not-ready':m.write(m.addr('mem_storage_ready'),0,1)
            else:m.stubs[m.addr('sj_runtime_blocked')]=lambda:m.ret(1)
            assert not m.checked('mem_tables_ready')
            before=bytes(m.flash);m.request()
            assert not m.ptr() and not any(m.counts.values()) and not m.sent
            body=struct.pack('<HBBB',3,0,7,19-reply);body+=struct.pack('<H',crc16(body))
            m.u.mem_write(0x21000000,body);m.checked('user_parse_comand',0x21000000,0)
            m.tick_upload()
            assert not m.ptr() and not any(m.counts.values()) and not m.sent and bytes(m.flash)==before
            assert largest_free(m)==2032
            count+=1
    return count


def test_busy_empty_cancel():
    count=0
    for cmd in (9,10):
        for ch in (0,1):
            for empty in (False,True):
                m=UploadMachine();cells=[] if empty else [log(1) if cmd==10 else temp(1)]
                m.fixture(cmd,cells);m.busy=True;m.request(ch)
                for _ in range(400):
                    m.tick_upload()
                    if m.phase()==5:break
                assert m.ptr() and m.phase()==5 and not m.sent and not m.pending_cursor(ch)
                pointer=m.ptr();packet=m.read_word(pointer);before=bytes(m.u.mem_read(packet,1040))
                m.request(ch);assert m.ptr()==pointer,'duplicate request replaced active snapshot'
                assert bytes(m.u.mem_read(packet,1040))==before
                m.busy=False
                if ch:
                    m.commit_fail=True;m.tick_upload();assert m.ptr() and not m.pending_cursor(ch) and m.releases==1
                    m.commit_fail=False
                m.drain_upload();assert m.sent==[wire(cmd,7,0,cells)]
                assert m.pending_cursor(ch)==(0 if empty else 1)
                count+=1
    for ch, phase in [(ch,phase) for ch in (0,1) for phase in (1,2,3,4,5)]:
        m=UploadMachine();m.fixture(10,[log(i) for i in range(1,40)]);m.request(ch)
        for _ in range(300):
            if m.phase()==phase:break
            m.tick_upload()
        else:raise AssertionError(phase)
        if ch:m.checked('user_comm_session',0)
        else:m.write(m.addr('usb_bus_resets'),1);m.tick_upload()
        assert not m.ptr() and not m.sent and not m.pending_cursor(ch) and not m.releases
        m.u.mem_write(0x21000800,b'\xA5'*1040)
        m.tick_upload();assert bytes(m.u.mem_read(0x21000800,1040))==b'\xA5'*1040
        count+=1
    m=UploadMachine();m.fixture(9,[temp(1)]);m.request();m.checked('user_savereset')
    assert not m.ptr() and not m.pending_cursor();count+=1
    return count


def largest_free(m):
    p=take(m,REPLY)
    if not p:return 0
    drop(m,REPLY,p)
    return 2032


def test_heap_ack():
    from unicorn import UC_HOOK_CODE
    results={}
    # A conflicting owner rejects before ACK cursor, NOR or wire changes.
    for slot in (REPLY,CONFIG,TABLE):
        m=UploadMachine();m.fixture(10,[log(1)])
        p=take(m,slot);assert p
        m.seed_pending(1);before=bytes(m.flash);m.request()
        assert not m.ptr() and not any(m.counts.values()) and bytes(m.flash)==before
        assert m.pending_cursor()==1 and m.cursor()==0
        drop(m,slot,p);assert owned(m)==0
    m=UploadMachine();m.fixture(10,[log(1),log(2)])
    photo=take(m,PHOTO);assert photo
    snapshots=[];release=m.addr('work_release')
    m.u.hook_add(UC_HOOK_CODE,lambda u,a,n,x:snapshots.append(owned(m)) if a==release else None)
    m.seed_pending(1);m.request()
    assert not m.ptr() and m.checked('mem_config_result')==1
    assert m.cursor()==0 and m.pending_cursor()==1
    assert not m.sent and not m.counts['start']
    assert owned(m)==(1<<(PHOTO-1))|(1<<(CONFIG-1))
    # Upload buffers were admitted together, then released before snapshot.
    assert snapshots[0] & (1<<(UP_CONTEXT-1)) and snapshots[0] & (1<<(UP_PACKET-1))
    for _ in range(1000):
        m.tick_upload()
        if m.sent:break
    assert m.sent and m.cursor()==1
    assert m.pending_cursor()==2 and m.sent[-1]==wire(10,7,1,[log(2)],2)
    drop(m,PHOTO,photo);assert owned(m)==0
    results.update(conflicts=3,preflight_slots=snapshots[0],storage_bytes=2224,
                   config_after_release=True,wire_and_cursor_verified=True)
    return results


def test_ack_power():
    m=UploadMachine();m.fixture(10,[log(1),log(2)])
    m.checked('upload_store')
    for name,off in [('config',0),('device',512)]:
        data=bytearray(m.u.mem_read(m.addr(name),512));data[510:512]=struct.pack('<H',crc16(data[:510]))
        m.flash[0x1cd400+off:0x1cd600+off]=data
    original=bytes(m.flash[m.base:m.base+64]);m.record_changes=True
    m.seed_pending(1);m.request()
    boot=UploadMachine();initial=bytes(boot.u.mem_read(0x20000000,0x5000));count=0;seen=0
    for _ in range(1500):
        before=bytes(m.flash);m.tick_upload()
        if len(m.changes)!=seen:
            seen=len(m.changes);kind,a,data=m.changes[-1]
            images=[bytes(m.flash)]
            for cut in (1,len(data)//2,len(data)-1):
                torn=bytearray(before);torn[a:a+cut]=m.flash[a:a+cut];images.append(bytes(torn))
            for image in images:
                boot.u.mem_write(0x20000000,initial);boot.flash=bytearray(image)
                selected=boot.checked('cfg_select',0x21000000)
                saved=image[selected+512:selected+1024]
                assert crc16(saved[:510])==int.from_bytes(saved[510:512],'little')
                cursor=(max(0,int.from_bytes(saved[boot.d['reserv']+20:boot.d['reserv']+24],'little')-1) if 'log_cursor32' in m.symbols else int.from_bytes(saved[boot.d['reserv']+4:boot.d['reserv']+6],'little'))
                assert cursor in (0,1),('torn ACK cursor',cursor)
                assert image[m.base:m.base+64]==original,'ACK erased unacknowledged rows'
                count+=1
        if m.sent:break
    else:raise AssertionError('ACK did not finish')
    assert m.cursor()==1 and m.pending_cursor()==2
    assert m.sent[-1]==wire(10,7,1,[log(2)],2)
    return count


if __name__=='__main__':
    r=dict(normal=test_normal(),parser=test_parser(),media_gate=test_media_gate(),faults_and_gaps=test_gaps_and_faults(),busy_empty_cancel=test_busy_empty_cancel(),
           heap_ack=test_heap_ack(),ack_power_cuts=test_ack_power())
    r['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest();r['status']='PASS'
    (ROOT/'Tests/asm/upload_runtime_results.json').write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({k:v for k,v in r.items() if k!='heap_ack'},indent=2))
