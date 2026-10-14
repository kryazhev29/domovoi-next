"""Штатное фото прохода: связанный IAR-код, модель UART/NOR, настоящий журнал."""
from test_photo_runtime import PhotoMachine, prefix_flash, fields, struct
from test_access_reads import AccessMachine, TODAY
from test_work_memory import take, PHOTO


class PassageMachine(PhotoMachine):
    put_id = AccessMachine.put_id
    put_group = AccessMachine.put_group
    read16 = AccessMachine.read16

    def __init__(self, flash=None):
        super().__init__(flash)
        self.cfg = self.addr('config'); self.c = fields('config_s')
        self.rele = fields('releMode_s'); self.loads = []
        self.stubs[self.addr('out_load')] = self.load
        self.stubs[self.addr('out_getmodestate')] = lambda: self.ret(0)
        self.stubs.pop(self.addr('log_addPhotoEvent'))
        self.write(self.addr('log_timeKnown'), 1, 1)
        self.write(self.device + self.d['datetime'], TODAY, 4)
        # Interceptor впереди двух C328: выбирается только первая C328, адрес 5.
        self.u.mem_write(self.cfg + self.c['rs485Periphery'], bytes([2, 2, 0, 0, 0, 0, 5, 3, 6, 3, 0, 0, 0, 0, 0, 0]))
        for i in range(2):
            for name in ('onWiegand', 'onOther', 'onCmdServer', 'tImpulse', 'cnt'):
                self.write(self.cfg + self.c['rele'] + i * 11 + self.rele[name], 1, 1)
        self.put_group(1); self.put_id(1, b'0000000042')

    def load(self):
        self.loads.append((tuple(self.args()), self.steps, self.active()))
        self.ret()

    def logs(self):
        f = fields('cellLog_s'); n = self.u.mem_read(self.addr('log_pendingCount'), 1)[0]
        data = bytearray(); address = self.checked('log_ring_seek',0)
        while address:
            data.extend(self.flash[address:address + 32])
            address = self.checked('log_ring_next', address)
        data.extend(self.u.mem_read(self.addr('log_pending'), n * 32))
        return [(data[i + f['logType']], data[i + f['ID']:i + f['ID'] + 10]) for i in range(0, len(data), 32)]

    def flush_logs(self):
        for _ in range(1000):
            if not self.u.mem_read(self.addr('log_pendingCount'), 1)[0]: return
            self.now += 1; self.checked('user_storage_poll')
        raise AssertionError('журнал не записался')


def ready(flash=None):
    m = PassageMachine(flash); m.checked('log_getRecordsCount')
    m.write(m.addr('store_save_requested'), 0, 1)
    m.boot_media(); m.port()
    return m


def finish(m, data):
    m.drain(); assert m.pget('stage') == 1
    m.flush_logs()
    m.port(); m.feed(bytes([0xaa, 0x0e, 0x0d, 0, 0, 0])); m.feed(bytes([0xaa, 0x0d, 0, 0, 0, 0]))
    for ack in (1, 6, 5):
        m.now += 50; m.port(); m.feed(bytes([0xaa, 0x0e, ack, 0, 0, 0]))
    m.now += 50; m.port(); m.feed(bytes([0xaa, 0x0a, 1, len(data), 0, 0])); m.drain()
    for off in range(0, len(data), m.packet_size-6):
        m.data(data[off:off + m.packet_size-6])
    assert m.pget('stage') == 0
    assert m.flash[0x200000:0x200008] == struct.pack('<II', TODAY, len(data))
    assert m.flash[0x20000d:0x20000d + len(data)] == data
    assert [row for row in m.logs() if row[0] == 129] == [(129, struct.pack('<I', TODAY) + bytes(6))]
    assert int.from_bytes(m.u.mem_read(m.device + m.d['status'], 2), 'little') & (1 << 13)
    assert int.from_bytes(m.u.mem_read(m.device + m.d['countPhoto'], 2), 'little') == 1
    m.flush_logs()
    assert m.u.mem_read(m.addr('log_pendingCount'), 1) == b'\0'
    assert [row for row in m.logs() if row[0] == 129] == [(129, struct.pack('<I', TODAY) + bytes(6))]


def passage():
    m = ready(); m.checked('WG_readedID', 0x42)
    assert m.loads and not any(load[2] for load in m.loads), 'реле сработали до запуска фото'
    assert m.active() and m.rget('address') == 5 and not m.commands
    assert not any(row[0] == 129 for row in m.logs()), 'событие только после записи'
    m.drain(); before = bytes(m.u.mem_read(m.photo_ptr(), 180))
    m.checked('WG_readedID', 0x42)
    assert len(m.loads) == 4 and bytes(m.u.mem_read(m.photo_ptr(), 180)) == before
    assert not m.active(), 'повторный проход не запускает вторую съёмку'
    finish(m, b'jpeg' * 15)
    for tp in (1, 2, 3, 4, 6, 7):
        m = ready(); assert m.checked('user_activateOut', 1, 1, tp, 0)
        assert len(m.loads) == 2 and m.active() and m.rget('address') == 5
    m = ready(); m.packet(8, bytes([1, 1, 1]))
    assert m.answers[-1][1] == 0 and m.active() and len(m.loads) == 2
    return {'ID_event': 129, 'ready_bit': 13, 'sources': 6, 'repeated_pass': 'one photo'}


def skipped():
    for reason in ('no_camera', 'type2', 'passive', 'time', 'FW', 'memory', 'upload', 'cleanup'):
        m = ready(prefix_flash(dirty=False) if reason == 'upload' else None)
        if reason in ('no_camera', 'type2'):
            m.u.mem_write(m.cfg + m.c['rs485Periphery'], bytes([2, 2] * 8) if reason == 'type2' else bytes(16))
        elif reason == 'passive': m.write(m.cfg + m.c['rs485Mode'], 0, 1)
        elif reason == 'time': m.write(m.device + m.d['datetime'], 0xffffffff)
        elif reason == 'FW': m.write(m.device + m.d['status'], 1 << 11, 2)
        elif reason == 'memory': assert take(m, PHOTO)
        elif reason == 'upload': m.packet(21); assert m.active()
        elif reason == 'cleanup':
            m.flash[0x200000:0x200010] = struct.pack('<I', 1) + bytes(12)
            m.checked('photo_storage_reset'); m.port(); m.tick()
            assert m.rget('cleanup') and m.active()
        before = m.exec_state(), len(m.answers), len(m.commands)
        m.checked('WG_readedID', 0x42)
        assert len(m.loads) == 2 and m.exec_state() == before[0], reason
        assert (len(m.answers), len(m.commands)) == before[1:], reason
        assert [row[0] for row in m.logs()] == [71], ('проход без ошибки фото', reason, m.logs())
        if reason == 'FW': assert m.checked('photo_start', 5) == 2
    m = ready(); m.write(m.cfg + m.c['rele'] + m.rele['useInBlock'], 1, 1)
    m.write(m.cfg + m.c['rele'] + 11 + m.rele['useInBlock'], 1, 1)
    assert not m.checked('user_activateOut', 1, 1, 6, 0) and not m.loads and not m.active()
    m = ready(); m.stubs[m.addr('out_getmodestate')] = lambda: m.ret(1)
    assert not m.checked('user_activateOut', 1, 1, 6, 0) and not m.loads and not m.active()
    m = ready(); m.checked('user_access_complete', 0, 7, 0, 255, 0)
    assert not m.loads and not m.active()
    m = ready(); m.write(m.cfg + m.c['rele'] + 11 + m.rele['useInBlock'], 1, 1)
    m.write(m.device + m.d['triggers'] + fields('triggers_s')['handUnBlock'], 1, 1)
    m.checked('user_unblock_recheck', 1)
    assert m.loads and not m.active() and not m.commands
    m.now += 1000; m.checked('user_unblock_poll', m.now)
    assert not m.active() and not m.commands
    m.checked('WG_readedID', 0x42); assert not m.active() and not any(row[0] == 129 for row in m.logs())
    return {'silent_skips': 8, 'IR_and_unblock': 'no photo'}


def same_camera_path():
    auto, command = ready(), ready()
    auto.checked('WG_readedID', 0x42); command.packet(7, b'\5')
    assert command.answers[-1][1] == 0
    start = (len(auto.poll_costs), len(auto.port_costs))
    finish(auto, b'x' * 58); finish(command, b'x' * 58)
    assert auto.commands == command.commands
    assert auto.poll_costs[start[0]:] == command.poll_costs[start[0]:]
    assert auto.port_costs[start[1]:] == command.port_costs[start[1]:]
    # Wiegand и серверный разбор обслуживаются между шагами активной камеры.
    m = ready(); m.packet(7, b'\5'); m.drain(); m.port()
    m.checked('WG_readedID', 0x42); assert len(m.loads) == 2 and m.pget('stage') == 1
    m.packet(8, bytes([1, 1, 1])); assert m.answers[-1][1] == 0 and len(m.loads) == 4
    assert m.pget('stage') == 1
    return {'UART': 'identical to command 7', 'step_instructions': 'identical', 'units': 'Unicorn instructions, not board time'}
