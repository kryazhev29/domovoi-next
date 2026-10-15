"""B2: временные ограничения закрыты до синхронизации; обычные ID работают."""
import json, struct
from test_access_reads import AccessMachine, fields, IDS, GROUP, SHEDULE, TODAY
from test_sched16 import record, tail
from iar_image import result_path


def run():
    cases = 0
    for now in (0, 10, 1599999999, TODAY):
        for kind in ('plain', 'past', 'future', 'guest-new', 'guest-expired', 'daily', 'schedule'):
            m = AccessMachine(); m.put_id(1, b'9991234567'); m.put_group(1)
            m.write(m.device + m.d['datetime'], now)
            c, g = fields('cellID_s'), fields('cellGroup_s')
            if kind in ('past', 'future'):
                m.flash[IDS+c['blockTime']:IDS+c['blockTime']+4] = struct.pack('<I', TODAY+(-1 if kind == 'past' else 86400))
            if kind.startswith('guest'):
                m.flash[IDS+c['useTime']:IDS+c['useTime']+2] = struct.pack('<H', 1)
                if kind == 'guest-expired':
                    m.flash[IDS+c['reserve']:IDS+c['reserve']+4] = struct.pack('<I', TODAY-120)
            if kind == 'daily': m.flash[IDS+c['cntPassOnDay']] = 8
            if kind == 'schedule':
                m.flash[GROUP+g['sheduleNumber']] = 1
                m.flash[SHEDULE:SHEDULE+16] = record(1, start=(0, 0), stop=(23, 59))
                m.flash[SHEDULE+1024:SHEDULE+1040] = tail()
            m.u.mem_write(0x21000000, b'9991234567')
            # Запись счётчика/старта гостя проверяется отдельными сценариями;
            # здесь ограничиваемся запретами и чистым проходом.
            before = bytes(m.flash)
            m.checked('user_checkIdAndControl', 0x21000000, 2)
            denied = (now < 1600000000 and kind != 'plain') or kind in ('past', 'guest-expired')
            if denied:
                assert not m.loads and before == bytes(m.flash), (now, kind, m.loads)
            elif kind not in ('guest-new', 'daily'):
                assert m.loads, (now, kind)
            cases += 1
    return dict(cases=cases, status='PASS')


if __name__ == '__main__':
    result = run(); result_path('review-time.json').write_text(json.dumps(result)+'\n')
    print(json.dumps(result))
