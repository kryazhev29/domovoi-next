"""USB/отложенная выгрузка при фото: настоящий IAR-код, NOR и TX на модели."""
import json
import struct
from test_photo_runtime import PhotoMachine, FORMAT, HEAD, pf
from test_usb_source import SourceMachine, init_transport
from test_stage4_usb import frames
from test_upload_runtime import log, temp
from storage_machine import crc16
from iar_image import result_path


def wire(cmd, body=b'', uniq=71, ttl=255):
    p = struct.pack('<HBBB', len(body)+3, ttl, uniq, cmd) + body
    return b'SLDN' + p + struct.pack('<H', crc16(p))


class BusyMachine(PhotoMachine):
    def __init__(self, flash=None):
        super().__init__(flash)
        init_transport(self)
        self.packets = []; self.usb_history = []
        for name in ('usb_send', 'usb_try_send', 'user_send_answer_comand'):
            self.stubs.pop(self.addr(name), None)
        self.stubs[self.addr('UserToPMABufferCopy')] = lambda: SourceMachine.pma(self)
        for name in ('SetEPTxCount', 'SetEPTxValid', 'usb_in_done'):
            self.stubs[self.addr(name)] = self.ret

    def usb(self, cmd, body=b'', uniq=71):
        p = wire(cmd, body, uniq)
        assert not int.from_bytes(self.u.mem_read(self.addr('usb_buffer_count'), 2), 'little')
        self.u.mem_write(self.addr('usb_buffer'), p)
        self.write(self.addr('usb_buffer_count'), len(p), 2)
        self.checked('usb_check_data', budget=1000000)
        return self.usb_drain()

    def usb_drain(self):
        self.checked('Handle_USBAsynchXfer')
        for _ in range(100):
            if not self.checked('USB_Tx_SourceBusy'): break
            self.checked('EP1_IN_Callback')
        else: raise AssertionError('USB TX stuck')
        out = frames(b''.join(self.packets)); self.packets.clear()
        self.usb_history.extend(out)
        return out

    def server(self, cmd, uniq=71, ttl=255):
        p = wire(cmd, uniq=uniq, ttl=ttl)
        self.write(self.device+self.d['connectionhandle'], 0, 1)
        self.u.mem_write(0x21000000, p)
        self.checked('user_comm_rx', 0x21000000, len(p))
        self.checked('user_comm_rx_done', len(p), 0)
        self.checked('server_rx_process')
        self.checked('server_rx_process')

    def step(self, dt=1):
        self.now += dt
        self.checked('user_storage_poll')
        self.checked('photo_poll')
        self.usb_drain()


def saved_photo(size=15689):
    f = bytearray(b'\xff'*0x400000)
    f[0x200000:0x200000+HEAD] = struct.pack('<IIIB', 12345, size, FORMAT, 255)
    f[0x200000+HEAD:0x200000+HEAD+size] = bytes(i % 251 for i in range(size))
    return f


def slow_stream(baseline=False):
    m = BusyMachine(saved_photo()); m.boot_media()
    m.packet(21, channel=1); m.drain(); m.sent.clear()
    start=m.now; next_tx=start; held=wait=stalled=0; max_usb=0; requests=0
    usb_at=None; usb_cmd=100; next_usb=start
    for elapsed in range(0, 45000, 10):
        m.now=start+elapsed
        m.transport_busy=m.now < next_tx
        if m.active():
            held += 10
            if m.transport_busy: wait += 10
        if usb_at is None and m.now >= next_usb:
            usb_at=m.now
            answer=m.usb(usb_cmd)
        else:
            m.checked('usb_check_data', budget=1000000)
            answer=m.usb_drain()
        if answer:
            assert answer[0][8] == usb_cmd, (usb_cmd,answer)
            max_usb=max(max_usb,m.now-usb_at); requests+=1
            usb_at=None; usb_cmd={100:101,101:112,112:100}[usb_cmd]; next_usb=m.now+100
        before=len(m.sent); phase=m.exec_state()
        m.checked('user_storage_poll'); m.checked('photo_poll')
        if phase==(8,13,1) and m.exec_state()==phase and m.transport_busy: stalled+=10
        if len(m.sent)>before: next_tx=m.now+1250
    chunks=[p for p in m.sent if p[8]==14 and p[9]]
    assert len(chunks)==31 and sum(len(p)-22 for p in chunks)==15689
    assert b''.join(p[20:-2] for p in chunks)==bytes(i%251 for i in range(15689))
    assert m.flash[0x20000c]==255 and not m.active(), 'no final 21: photo must survive'
    result=dict(duration_ms=45000,fx_photo_ms=held,fx_photo_while_tx_busy_ms=wait,fx_photo_tx_wait_ms=stalled,
                usb_max_ms=max_usb,usb_replies=requests,packets=len(chunks))
    if not baseline: assert max_usb <= 20 and held < 4000 and not stalled, result
    return result


def usb_owners():
    from test_work_memory import take, SIZES
    count=0
    for kind in range(1,15):
        m=BusyMachine(); m.u.mem_write(m.addr('flash_job'),bytes([kind,1,1]))
        owner=take(m,kind); assert owner
        original=bytes(m.u.mem_read(owner,SIZES[kind]))
        for cmd,body in ((100,b''),(101,b''),(102,b'4321'),(106,b''),(107,b''),(108,b'\0'),(112,b'')):
            m.write(m.device+m.d['userAutorized'],1,1)
            out=m.usb(cmd,body)
            assert out and out[0][8]==cmd,(kind,cmd,out)
            assert not m.counts['read'] and not m.io
            count+=1
        if 'user_send_usbDiag' in m.symbols:
            out=m.usb(111); assert out and out[0][8]==111
        assert bytes(m.u.mem_read(owner,SIZES[kind]))==original
        m.write(m.device+m.d['userAutorized'],1,1)
        assert not m.usb(12)
        m.now+=499; m.checked('usb_check_data'); assert not m.usb_drain()
        m.now+=2; m.checked('usb_check_data'); out=m.usb_drain()
        assert len(out)==1 and out[0][8:10]==bytes([11,2]),out
    return count


def queue_edges():
    from log_ring_fixture import install
    # Два канала независимы; переполнение серверного слота не заменяет первый uniq.
    m=BusyMachine(); m.boot_media(); install(m,[log(1)])
    m.flash[0x1e8000:0x1e8020]=temp(1)
    m.u.mem_write(m.addr('flash_job'),bytes([8,1,1]))
    assert not m.usb(9,uniq=51)
    m.server(10,uniq=52); m.server(9,uniq=53)
    assert any(p[8:10]==bytes([11,2]) and p[7]==53 for p in m.sent)
    m.u.mem_write(m.addr('flash_job'),bytes(4))
    for _ in range(100): m.step()
    assert len([p for p in m.usb_history if p[8]==10 and p[7]==51])==1
    assert len([p for p in m.sent if p[8]==9 and p[7]==52])==1
    assert not [p for p in m.sent if p[8]==10 and p[7]==53]
    # Повтор не продлевает 1 с, в том числе через переполнение миллисекунд.
    for started in (100,0xffffff00):
        m=BusyMachine(); install(m,[log(1)]); m.now=started
        m.u.mem_write(m.addr('flash_job'),bytes([8,1,1]))
        m.server(9,ttl=1); m.now=(started+900)&0xffffffff; m.server(9,ttl=1)
        m.now=(started+1001)&0xffffffff; m.u.mem_write(m.addr('flash_job'),bytes(4))
        for _ in range(100): m.checked('user_storage_poll')
        assert not m.sent
    # Сброс USB снимает ожидающий запрос, физическая операция W25 не мешает RAM-ответам.
    m=BusyMachine(); install(m,[log(1)])
    m.u.mem_write(m.addr('flash_job'),bytes([8,1,1])); m.usb(9)
    m.write(m.addr('usb_bus_resets'),8); assert m.usb(100)[0][8]==100
    m.u.mem_write(m.addr('flash_job'),bytes(4))
    for _ in range(100): m.checked('user_storage_poll')
    assert not m.usb_drain()
    m=BusyMachine()
    assert m.checked('w25_async_start',0x300000,0,0,m.now)
    for cmd in (100,101,106,112): assert m.usb(cmd)[0][8]==cmd and m.io
    return 8


def mutations():
    from test_flash_mutations import run as mutate
    return mutate({
        'usb-global-flash-gate': ('usb_owners', [('User/c/user_store.c',
            '  // Ответы из RAM не зависят от владельца W25. 9/10 сохраняют только намерение.',
            '  if (usb_input && !flash_exec_idle()) return false;\n  // Ответы из RAM не зависят от владельца W25. 9/10 сохраняют только намерение.')]),
        'upload-silent-busy-return': ('deferred', [('User/c/upload_runtime.c',
            'if(upload_current() || !flash_exec_idle() || !flash_exec_read_allowed()) return false;',
            'if(upload_current() || !flash_exec_idle() || !flash_exec_read_allowed()) return true;')]),
        'upload-ack-no-retry': ('ack_failures', [('User/c/upload_runtime.c',
            'if (upload_pending[channel].state == 2) upload_pending[channel].state = 1;',
            '(void)channel;')]),
    }, 'test_photo_busy', 'photo-busy-mutations')


def ack_failures():
    from log_ring_fixture import install
    count = 0
    for cmd in (9, 10):
      for failure in ('read', 'start', 'poll', 'begin'):
        m = BusyMachine(); install(m, [log(i) for i in range(1, 40)])
        m.write(m.addr('cfg_boot_done'), 1, 1)
        m.write(m.addr('cfg_cached_active'), 0x1cd400)
        for i in range(1, 140): m.flash[0x1e8000+(i-1)*32:0x1e8000+i*32] = temp(i)
        m.server(cmd, uniq=43)
        for _ in range(1000): m.step()
        assert len(m.sent) == 1
        if failure == 'begin': m.write(m.addr('cfg_boot_done'), 0, 1)
        else: m.fault = (failure, m.counts[failure] + 1)
        m.server(cmd, uniq=44)
        for _ in range(100): m.step()
        m.write(m.addr('cfg_boot_done'), 1, 1)
        m.server(cmd, uniq=44)
        for _ in range(1500): m.step()
        out = [p for p in m.sent if p[7] == 44]
        assert len(out) == 1 and out[0][8] == (10 if cmd == 9 else 9), (cmd, failure, out)
        m.server(cmd, uniq=44)
        for _ in range(100): m.step()
        assert len([p for p in m.sent if p[7] == 44]) == 1
        assert m.checked('flash_exec_idle')
        count += 1
    return count


def deferred():
    from log_ring_fixture import install
    count=0
    for phase in ('capture','stream','scan'):
      for cmd in (9,10):
        m=BusyMachine(saved_photo()); m.boot_media()
        install(m,[log(1),log(2)])
        base=0x1e8000; m.flash[base:base+32]=temp(1)
        if phase=='capture':
            m.packet(7,b'\5'); assert m.active()
        elif phase=='stream':
            m.packet(21,channel=1); m.drain(); m.checked('photo_poll'); assert m.active()
        else:
            m.checked('photo_storage_reset');m.port()
            assert m.active() and not m.rget('initialized')
        m.sent.clear()
        m.usb_drain()
        m.server(cmd,uniq=44); m.server(cmd,uniq=44)
        assert not [p for p in m.sent if p[8] == (10 if cmd==9 else 9)]
        for _ in range(3000):
            m.step()
            if any(p[8]==(10 if cmd==9 else 9) for p in m.sent): break
        else: raise AssertionError(('deferred upload lost',phase,cmd,m.exec_state()))
        m.server(cmd,uniq=44)
        for _ in range(100): m.step()
        out=[p for p in m.sent if p[8]==(10 if cmd==9 else 9)]
        assert len(out)==1 and out[0][7]==44,(phase,cmd,len(out))
        count+=1
    # TTL и смена TCP-сессии снимают только RAM-намерение.
    for expire in (True,False):
        m=BusyMachine(); install(m,[log(1)])
        m.u.mem_write(m.addr('flash_job'),bytes([8,1,1]))
        m.server(9,ttl=1)
        if expire: m.now+=1001
        else: m.checked('flash_exec_quiet',1)
        m.u.mem_write(m.addr('flash_job'),bytes(4))
        for _ in range(100): m.checked('user_storage_poll')
        assert not m.sent
    return count+2


def run():
    result=dict(stream=slow_stream(),usb_cases=usb_owners(),deferred=deferred(),queue_edges=queue_edges(),ack_failures=ack_failures())
    result_path('photo-busy.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__=='__main__':
    import argparse
    from firmware_suite import select_image
    p=argparse.ArgumentParser(); p.add_argument('--baseline',action='store_true'); p.add_argument('--build-root')
    a=p.parse_args(); select_image(a.build_root)
    print(json.dumps(slow_stream(True) if a.baseline else run(),indent=2))
