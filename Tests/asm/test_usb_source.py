"""Linked USB source ownership, PMA chunking and cancellation, without a board."""
import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from iar_irq_support import Machine, UC_ARM_REG_PRIMASK, UC_ARM_REG_R0, UC_ARM_REG_R2, UC_HOOK_CODE


class SourceMachine(Machine):
    def __init__(self):
        super().__init__()
        self.init()
        self.write(self.addr('pInformation'), self.addr('Device_Info'))
        self.packets = []
        self.stubs[self.addr('UserToPMABufferCopy')] = self.pma
        for name in ('SetEPTxCount', 'SetEPTxValid', 'usb_in_done'):
            self.stubs[self.addr(name)] = lambda *args: self.ret()
        self.configure()

    def configure(self, mask=0):
        self.write(self.addr('bDeviceState'), 5)
        self.write(self.addr('usb_enabled'), 1, 1)
        self.write(self.addr('usb_bus_resets'), 7)
        self.write(self.addr('store') + 12, 7)
        self.write(0x40010c08, 32768)
        self.u.reg_write(UC_ARM_REG_PRIMASK, mask)

    def pma(self):
        pointer = self.u.reg_read(UC_ARM_REG_R0)
        count = self.u.reg_read(UC_ARM_REG_R2)
        assert 0 < count <= 64
        self.packets.append(bytes(self.u.mem_read(pointer, count)))
        self.ret()

    def busy(self):
        return self.call('USB_Tx_SourceBusy')

    def drain(self):
        self.call('Handle_USBAsynchXfer')
        while self.busy():
            self.call('EP1_IN_Callback')
        return b''.join(self.packets)

    def owned(self):
        self.call('work_acquire', 0, 11)  # collect completed deferred releases
        return self.read(self.addr('work_memory') + 2216, 2)


def idle():
    cases = 0
    for mask in (0, 1):
        for length in (0, 1, 63, 64, 65, 523, 534, 1014, 1038, 1039, 1040, 65535):
            m = SourceMachine(); m.configure(mask)
            payload = bytes((i * 19 + 83) & 255 for i in range(1040))
            m.u.mem_write(0x21000000, payload)
            accepted = m.call('usb_try_send', 0x21000000, length)
            assert accepted == int(length < 1040), (mask, length, accepted)
            assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
            if accepted:
                # Stack/transient data has become the owner's immutable packet.
                m.u.mem_write(0x21000000, b'X' * 1040)
                assert m.drain() == payload[:length]
                assert not m.owned()
            else:
                assert not m.busy() and not m.owned()
            cases += 1
    for symbol, value, size in (('usb_enabled', 0, 1), ('bDeviceState', 0, 4),
                                ('usb_bus_resets', 8, 4)):
        m = SourceMachine(); m.write(m.addr(symbol), value, size)
        assert not m.call('usb_try_send', 0x21000000, 64)
        assert not m.busy() and not m.owned()
    for mask in (0, 1):
        m = SourceMachine(); m.configure(mask)
        assert not m.call('usb_try_send', 0, 1)
        assert m.call('usb_try_send', 0, 0)
    return {'boundary_cases': cases}


def large():
    cases = 0
    for slot, length in ((11, 1038), (2, 534), (10, 1038)):
        for mask in (0, 1):
            m = SourceMachine(); m.configure(mask)
            p = m.call('work_acquire', length, slot); assert p
            payload = bytes((i * 37 + 11) & 255 for i in range(length))
            m.u.mem_write(p, payload)
            assert m.call('usb_try_send', p, length)
            assert m.read(m.addr('usb_tx_source')) == p, 'owned packet was copied'
            assert not m.call('work_release', p, slot), 'premature release succeeded'
            assert not m.call('work_acquire', length, slot)
            assert bytes(m.u.mem_read(p, length)) == payload
            m.call('Handle_USBAsynchXfer')
            while m.read(m.addr('usb_tx_offset'), 2) < length:
                m.call('EP1_IN_Callback')
            assert m.busy(), 'source freed before the last IN acknowledgement'
            assert not m.call('work_acquire', length, slot)
            m.call('EP1_IN_Callback')
            assert not m.busy() and not m.owned()
            assert b''.join(m.packets) == payload
            assert m.call('work_acquire', length, slot) == p
            assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
            cases += 1
    # An address released before submission must never become a valid loan.
    m = SourceMachine(); p = m.call('work_acquire', 1038, 11)
    assert m.call('work_release', p, 11)
    assert not m.call('usb_try_send', p, 1038)
    return {'owner_lifetime_cases': cases + 1}


def busy():
    m = SourceMachine(); m.u.mem_write(0x21000000, b'A' * 1038 + b'B')
    assert m.call('usb_try_send', 0x21000000, 1038)
    p = m.read(m.addr('usb_tx_source'))
    assert m.call('usb_try_send', 0, 0)
    assert not m.call('usb_try_send', 0x2100040e, 1)
    assert m.read(m.addr('usb_tx_source')) == p
    assert m.drain() == b'A' * 1038
    m.packets.clear()
    assert m.call('usb_try_send', 0x2100040e, 1)
    assert m.drain() == b'B' and not m.owned()
    return {'ordered_backpressure_retry': True}


def reset():
    cases = 0
    for copied in (0, 1, 2, 8, 16, 17):
        m = SourceMachine(); p = m.call('work_acquire', 1038, 11)
        m.u.mem_write(p, b'R' * 1038)
        assert m.call('usb_try_send', p, 1038)
        assert not m.call('work_release', p, 11)
        for i in range(copied):
            m.call('Handle_USBAsynchXfer' if i == 0 else 'EP1_IN_Callback')
        m.call('USB_Tx_Reset')
        assert not m.busy() and not m.owned()
        assert m.call('work_acquire', 1038, 11) == p
        before = list(m.packets); m.call('EP1_IN_Callback')
        assert m.packets == before
        cases += 1
    # Inject the real USB reset at every admission instruction boundary.
    m = SourceMachine(); p = m.call('work_acquire', 1038, 11)
    m.call('usb_try_send', p, 1038); steps = m.steps
    for point in range(steps):
        m = SourceMachine(); p = m.call('work_acquire', 1038, 11)
        m.call('usb_try_send', p, 1038, inject_at=point,
               irq=('Virtual_Com_Port_Reset', ()))
        assert not m.busy(), (point, m.steps)
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == 0
    return {'reset_chunk_cases': cases, 'reset_irq_boundaries': steps}


def esof():
    # Admission becomes visible only after source/length are initialized.
    m = SourceMachine(); m.u.mem_write(0x21000000, b'E' * 1038)
    published = []
    def observe(u, pc, size, unused):
        source = m.read(m.addr('usb_tx_source'))
        if source:
            assert m.read(m.addr('usb_tx_length'), 2) == 1038
            assert bytes(u.mem_read(source, 1038)) == b'E' * 1038
            published.append(source)
    m.u.hook_add(UC_HOOK_CODE, observe)
    assert m.call('usb_try_send', 0x21000000, 1038)
    assert published
    assert m.drain() == b'E' * 1038
    return {'atomic_source_publication': True}


def run():
    return {name: globals()[name]() for name in ('idle', 'large', 'busy', 'reset', 'esof')}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--scenario', choices=('idle','large','busy','reset','esof'))
    args = parser.parse_args()
    print('PASS USB source', globals()[args.scenario]() if args.scenario else run())
