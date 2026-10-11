"""Настоящие user_incomingcall и outs.c на IAR ELF: повтор в паузе выхода."""
from test_unblock import UnblockMachine
from test_access_reads import GROUP
from storage_machine import fields


def run(ignore_pause=False):
    m = UnblockMachine()
    m.checked('user_unblock_recheck', 1)
    m.set_relay(0, 'tPause', 20)
    m.put_group(1)
    m.flash[GROUP + fields('cellGroup_s')['onPhone']] = 1
    m.put_id(1, b'9161234567')
    number = 0x21000600
    m.u.mem_write(number, b'+79161234567\0')
    m.loads.clear()
    m.events.clear()
    # Бит 11 не запрещает доступ ни при загрузке, ни при откате FW.
    m.write(m.device + m.d['status'], 1 << 11, 2)
    m.checked('user_incomingcall', number, 1)
    assert len(m.loads) == 1 and len(m.events) == 1 and m.outputs()[0] == 1
    m.advance(8200)
    assert m.checked('out_getmodestate', 0) and m.outputs()[0] == 0
    if ignore_pause:
        m.stubs[m.addr('out_getmodestate')] = lambda: m.ret(0)
    m.checked('user_incomingcall', number, 1)
    assert len(m.loads) == 1 and len(m.events) == 1 and m.outputs()[0] == 0, 'second call during output pause'
    m.advance(14000)
    m.checked('user_incomingcall', number, 1)
    assert len(m.loads) == 2 and len(m.events) == 2 and m.outputs()[0] == 1
    return {'real_access_output_pause_FW': 'PASS'}
