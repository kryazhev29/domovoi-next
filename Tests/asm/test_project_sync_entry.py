"""Current asynchronous entry contract replacing the inactive sync extraction."""
from storage_machine import StorageMachine
from test_table_runtime import rows


def run():
    cases = 0
    for cmd, size, part in ((13, 32, 2), (14, 16, 4), (15, 16, 3), (16, 4, 5)):
        data = rows([1], size) if cmd != 16 else b'\1\2\3\4'
        if cmd==14:data+=bytes(8)+b'\1'+bytes(7)
        for channel in (0, 1):
            for scenario in ('zero', 'short', 'second-index', 'media', 'oom', 'io', 'valid'):
                m = StorageMachine()
                before = bytes(m.flash)
                payload, count = data, 1
                expected = None
                if scenario == 'zero':
                    count = 0
                    expected = 1
                elif scenario == 'short':
                    payload = payload[:-1]
                    expected = 1
                elif scenario == 'second-index':
                    limit = (8192 if part == 2 else 1024 if part == 5 else 2048) // size
                    payload = rows([1, limit + 1], size) if cmd != 16 else data * 2
                    count = 2 if cmd != 16 else 1
                    expected = 1
                elif scenario == 'media':
                    m.stubs[m.addr('mem_tables_ready')] = lambda: m.ret(0)
                    expected = 2
                elif scenario == 'oom':
                    m.stubs[m.addr('work_acquire')] = lambda: m.ret(0)
                    expected = 2
                elif scenario == 'io':
                    m.fault = ('read', 1)
                    expected = 2
                m.command(cmd, payload, count, channel, 31)
                if scenario in ('valid', 'io'):
                    assert m.active() and bytes(m.flash) == before
                    m.drain()
                if expected is not None:
                    assert m.answers == [(cmd, expected, channel, 31)], (cmd, scenario, m.answers)
                    assert not m.timestamps and not m.active()
                    if scenario != 'io':
                        assert bytes(m.flash) == before and not m.changes
                else:
                    assert not m.answers and len(m.timestamps) == 1 and not m.active()
                cases += 1
    return {'linked_entry_cases': cases, 'commands': [13, 14, 15, 16],
            'channels': [0, 1], 'implementation': 'actual asynchronous parser/journal'}
