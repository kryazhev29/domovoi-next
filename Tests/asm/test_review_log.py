"""B7: отказ освобождает Flash, очередь не теряется, поздний commit не дублируется."""
import json
from test_service_runtime import ServiceMachine
from test_stage5_log import boot, records
from test_upload_runtime import log
from log_ring_fixture import install, BASE
from iar_image import result_path


class LogMachine(ServiceMachine):
    def __init__(self, fault):
        super().__init__()
        self.failure = None; self.attempts = 0; self.commit_failed = False
        self.stubs[self.addr('idcrc_background_begin')] = lambda: self.ret(0)
        self.checked('idindex_runtime_suspend')
        install(self, [log(1)]); boot(self)
        self.write(self.addr('log_timeKnown'), 1, 1)
        assert self.checked('log_addSystemEvent', 14)
        self.failure = fault

    def start(self):
        a, p, n, _ = self.args()
        if BASE <= a < BASE+98304:
            self.attempts += 1
            if self.failure == 'start' or (self.failure == 'late-commit' and self.attempts <= 2):
                self.counts['start'] += 1; self.ret(0); return
        super().start()

    def poll(self):
        a, p, n, data, step = self.io
        if BASE <= a < BASE+98304 and (self.failure == 'poll' or
                (self.failure == 'late-commit' and n == 4)):
            self.counts['poll'] += 1; self.io = None; self.commit_failed |= n == 4
            self.ret(3); return
        super().poll()

    def read(self):
        a, p, n, _ = self.args()
        if self.failure == 'read' and BASE <= a < BASE+98304:
            self.counts['read'] += 1; self.ret(0); return
        super().read()

    def tick(self, delta=1):
        self.now = (self.now+delta)&0xffffffff
        self.checked('user_storage_poll')

    def queued_count(self): return self.u.mem_read(self.addr('log_pendingCount'), 1)[0]


def run():
    result = {}
    for fault in ('start', 'poll', 'read', 'late-commit'):
        m = LogMachine(fault)
        if fault == 'read': m.checked('log_ring_invalidate')
        for _ in range(1000):
            m.tick(10)
            if not m.checked('log_write_ready'): break
        else: raise AssertionError(('owner did not yield', fault))
        assert m.queued_count() == 1 and m.checked('flash_exec_idle')
        assert m.checked('store_command_allowed', 12), 'configuration starved'
        attempts = m.attempts
        # Настоящая запись A/B проходит, пока неисправный журнал ждёт.
        m.checked('user_saveindex')
        for _ in range(1500):
            m.tick()
            if not m.u.mem_read(m.addr('store_save_requested'),1)[0] and m.checked('flash_exec_idle'): break
        else: raise AssertionError(('configuration stuck', fault))
        assert m.attempts == attempts and m.queued_count() == 1
        if fault == 'late-commit':
            sealed = bytes(m.u.mem_read(m.addr('log_pending'),32))
            m.write(m.addr('log_timeKnown'),0,1)
            m.checked('log_setTime',1792000000)
            assert bytes(m.u.mem_read(m.addr('log_pending'),32)) == sealed
        # Даже постоянный отказ даёт окно прочим владельцам, без удаления события.
        if fault == 'start':
            m.tick(30001)
            for _ in range(1000):
                m.tick(10)
                if not m.checked('log_write_ready'): break
            assert m.checked('flash_exec_idle') and m.queued_count() == 1
            assert m.attempts == 6
        m.failure = None
        m.tick(30001)
        for _ in range(1500):
            m.tick()
            if not m.queued_count(): break
        else: raise AssertionError(('recovery stuck', fault))
        assert len(records(m)) == 2, (fault, records(m))
        if fault == 'late-commit': assert m.commit_failed
        result[fault] = dict(write_attempts=m.attempts, events=len(records(m)), queue=m.queued_count())
    result['status'] = 'PASS'
    result_path('review-log.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__': print(json.dumps(run()))
