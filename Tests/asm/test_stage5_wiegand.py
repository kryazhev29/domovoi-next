"""Linked W26 parity, delivery, noise and bounded snapshot instructions."""
from iar_irq_support import Machine, UC_ARM_REG_PRIMASK


def frame(value):
    return (value << 1) | (((value & 4095).bit_count() ^ 1) & 1) | (((value >> 12).bit_count() & 1) << 25)


def run(parity=True):
    m = Machine(); m.init()
    ids = []
    m.stubs[m.addr('WG_readedID')] = lambda: (ids.append(m.read(m.addr('wgID'))), m.ret())
    maximum = 0
    values = [0, 1, 4095, 4096, 0x555555, 0xaaaaaa, 0xffffff] + [((i * 0x12345) & 0xffffff) for i in range(32)]
    for mask in (0, 1):
        for value in values:
            raw = frame(value)
            for bit in (-1, *range(26)):
                m.write(m.addr('wgData'), raw if bit < 0 else raw ^ (1 << bit))
                m.write(m.addr('wgBitCount'), 26, 1)
                m.write(m.addr('wgTime'), 100)
                m.write(m.addr('milliSeconds'), 400)
                m.write(m.addr('wgDataAvalible'), 0, 1)
                m.u.reg_write(UC_ARM_REG_PRIMASK, mask)
                m.call('WG_process'); maximum = max(maximum, m.steps)
                assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
                accepted = int(bit < 0 or not parity)
                assert m.read(m.addr('wgDataAvalible'), 1) == accepted
                before = len(ids)
                m.call('WG_process')
                assert len(ids) == before + accepted
                if accepted:
                    assert ids[-1] == (((raw if bit < 0 else raw ^ (1 << bit)) >> 1) & 0x03ffffff)
        for count in (0, 1, 25, 27, 255):
            m.write(m.addr('wgData'), frame(0x123456))
            m.write(m.addr('wgBitCount'), count, 1)
            m.write(m.addr('wgTime'), 100)
            m.call('WG_process')
            assert not m.read(m.addr('wgDataAvalible'), 1)
    return {'valid_cases': len(values) * 2, 'single_errors': len(values) * 52, 'maximum_snapshot_instructions': maximum, 'static_RAM_added': 0}
