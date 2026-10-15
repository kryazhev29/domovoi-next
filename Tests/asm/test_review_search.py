"""B1: ограниченный поиск, отмена, смена базы и свежие счётчики на связанном ELF."""
import json, struct
from test_access_reads import AccessMachine, IDS
from storage_machine import fields
from iar_image import result_path

IDENT, OUT = 0x21000000, 0x21000300


def machine(count=50048, counters=0):
    m = AccessMachine()
    m.write(m.device+m.d['memID'], 0x14c2, 2)
    m.write(m.addr('mem')+20, count*32)
    for i in range(1, count+1): m.put_id(i, b'%010d' % i)
    if counters:
        page = 0x1fb000
        m.u.mem_write(m.addr('ct'), struct.pack('<IIIHH', page, 1, 0, counters, counters))
        for i in range(counters):
            key = count-1 if not i else i-1
            m.flash[page+12+10*i:page+22+10*i] = b'\x07\x01\x02\x03\x04\x05\x06\x07'+struct.pack('<H', key)
    return m


def find(m, ident):
    m.u.mem_write(IDENT, ident)
    reads = steps = polls = 0
    while True:
        result = m.checked('user_findID', IDENT, OUT)
        reads = max(reads, m.read_budget); steps = max(steps, m.steps); polls += 1
        assert polls <= 400
        if result != 2: return result, polls, reads, steps
        m.now += 1


def run():
    result = {}
    for counters in (0, 408):
        m = machine(counters=counters)
        r, polls, reads, steps = find(m, b'9999999999')
        assert r == 0 and polls == 391 and reads <= 4096
        assert m.counts['read'] == 6256, 'search must not read the counter journal for other IDs'
        result['absent_'+str(counters)] = dict(polls=polls, reads=m.counts['read'], bytes_per_poll=reads, instructions_per_poll=steps)
        r, polls, reads, steps = find(m, b'0000050048')
        assert r == 1 and polls == 391 and reads <= 8192
        expected = b'\x07\x01\x02\x03\x04\x05\x06\x07' if counters else bytes(m.flash[IDS+50047*32+21:IDS+50047*32+29])
        assert bytes(m.u.mem_read(OUT+21, 8)) == expected
        before = m.counts['read']; r, polls, _, _ = find(m, b'0000050048')
        assert r == 1 and polls == 1 and m.counts['read']-before <= counters+2
        result['last_'+str(counters)] = dict(polls=391, bytes_per_poll=reads, instructions_per_poll=steps, stack=m.stack_peak)
    # Другой источник не сбрасывает позицию; пропавший источник освобождает поиск.
    m = machine(1024); m.u.mem_write(IDENT, b'0000001000')
    assert m.checked('user_findID', IDENT, OUT) == 2
    before = m.counts['read']; m.u.mem_write(IDENT, b'0000000001')
    assert m.checked('user_findID', IDENT, OUT) == 2 and m.counts['read'] == before
    m.now += 101
    assert m.checked('user_findID', IDENT, OUT) == 1
    # Смена таблицы сбрасывает как отрицательный результат, так и незавершённый проход.
    for pending in (False, True):
        m = machine(1024)
        if pending:
            m.u.mem_write(IDENT, b'9999999999'); assert m.checked('user_findID', IDENT, OUT) == 2
        else: assert find(m, b'9999999999')[0] == 0
        m.checked('idindex_runtime_suspend'); m.put_id(1, b'9999999999')
        assert find(m, b'9999999999')[0] == 1
    # Ошибка чтения сохраняет позицию и никогда не превращается в «ID отсутствует».
    m = machine(1024); m.u.mem_write(IDENT, b'0000001000'); m.fault = ('read', 3)
    assert m.checked('user_findID', IDENT, OUT) == 2
    assert find(m, b'0000001000')[0] == 1
    result['status'] = 'PASS'
    result_path('review-search.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__': print(json.dumps(run()))
