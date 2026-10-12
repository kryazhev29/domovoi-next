"""Бит 11 на IAR ELF: запрет голоса/команды 8, обычный доступ по Wiegand."""
from test_unblock import UnblockMachine, parse
from test_access_reads import GROUP
from storage_machine import fields
from unicorn import UC_HOOK_CODE


def run():
    m = UnblockMachine()
    m.checked('user_unblock_recheck', 1)
    m.loads.clear()
    m.events.clear()
    m.set_relay(0, 'tPause', 20)
    m.put_group(1)
    m.flash[GROUP + fields('cellGroup_s')['onPhone']] = 1
    m.put_id(1, b'9161234567')
    number = 0x21000600
    m.u.mem_write(number, b'+79161234567\0')
    visited = []
    for name in ('user_incomingcall', 'user_checkIdAndControl'):
        m.u.hook_add(UC_HOOK_CODE, lambda u, a, n, _: visited.append(a),
                     begin=m.addr(name), end=m.addr(name))
    m.write(m.device + m.d['status'], 1 << 11, 2)
    before = bytes(m.flash), m.counts['read'], m.outputs()
    assert m.checked('user_comm_call', 0) == 0
    assert m.checked('user_comm_call', number) == 0
    assert not visited and not m.loads and not m.events
    assert (bytes(m.flash), m.counts['read'], m.outputs()) == before
    m.write(m.device + m.d['status'], 0, 2)
    assert m.checked('user_comm_call', 0) == 1
    assert not visited and not m.loads and not m.events
    assert m.checked('user_comm_call', number) == 1
    assert len(visited) == 2 and len(m.loads) == 1 and len(m.events) == 1
    assert m.outputs()[0] == 1
    m.advance(8200)
    assert m.checked('out_getmodestate', 0) and m.outputs()[0] == 0
    m.checked('user_comm_call', number)
    assert len(m.loads) == 1 and len(m.events) == 1 and m.outputs()[0] == 0, (m.loads, m.events, m.outputs())
    m.advance(14000)
    m.checked('user_comm_call', number)
    assert len(m.loads) == 2 and len(m.events) == 2 and m.outputs()[0] == 1
    # Отказ не переключает ни включённый, ни выключенный выход и не пишет журнал.
    for payload in (b'\x01\x01\x00', b'\x01\x00\x00'):
        m = UnblockMachine(mask=0)
        m.checked('out_load', 0, 1, 0, 0, 0)
        m.write(m.device + m.d['status'], 1 << 11, 2)
        before = m.outputs(), list(m.loads), list(m.events), bytes(m.flash)
        parse(m, 8, payload, channel=1)
        assert m.answers[-1] == (8, 2, 1, 37), m.answers
        assert (m.outputs(), m.loads, m.events, bytes(m.flash)) == before
        m.write(m.device + m.d['status'], 0, 2)
        parse(m, 8, b'\x01\x00\x00', channel=1)
        assert m.answers[-1] == (8, 0, 1, 37) and m.outputs() == (0, 0)
        parse(m, 8, b'\x01\x01\x00', channel=1)
        assert m.answers[-1] == (8, 0, 1, 37) and m.outputs()[0] == 1
    m = UnblockMachine()
    m.put_group(1)
    m.put_id(1, b'0000123456')
    m.write(m.device + m.d['status'], 1 << 11, 2)
    m.checked('WG_readedID', 0x123456)
    assert len(m.loads) == 1 and m.events[-1][0] == 71 and m.outputs()[0] == 1
    return {'FW_call_no_access_or_log_cmd8_refusal_Wiegand_works_resume': 'PASS'}
