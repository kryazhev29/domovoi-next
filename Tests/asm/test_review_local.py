"""B3/B4/B8: границы номера, принятие SMS и владение ответом отката на IAR ELF."""
import json
from test_protocol_guards import execute
from test_access_reads import AccessMachine
from test_fw_service_runtime import FirmwareMachine, owned, FW_CONTEXT
from test_service_runtime import ServiceMachine
from storage_machine import fields
from unicorn import UC_HOOK_MEM_WRITE
from iar_image import result_path


def caller():
    invalid = [b'', *[b'9'*n for n in range(1, 10)], b'+', b'++79991234567',
               b'7999+1234567', b'+7999x1234567', b'9'*21, b'+'+b'9'*20]
    valid = [b'9991234567', b'89991234567', b'+79991234567', b'9'*20]
    for number in invalid + valid:
        m = AccessMachine(); ready, actions = [], []
        def check():
            ready.append(bytes(m.u.mem_read(m.args()[0], 10))); m.ret(1)
        def action():
            actions.append(bytes(m.u.mem_read(m.args()[0], 10))); m.ret()
        m.stubs[m.addr('user_access_ready')] = check
        m.stubs[m.addr('user_checkIdAndControl')] = action
        m.u.mem_write(0x21000000, number + b'\0' + b'\xa5'*32)
        assert m.checked('user_incomingcall', 0x21000000, 1) == 1
        expected = [number[-10:]] if number in valid else []
        assert ready == actions == expected, (number, ready, actions)
        if expected:
            m.stubs[m.addr('user_access_ready')] = lambda: m.ret(0)
            assert m.checked('user_incomingcall', 0x21000000, 1) == 0
            assert actions == expected, 'busy must not execute access'
    return len(invalid) + len(valid)


def sms():
    cases = 0
    for channel in (0, 1):
        for number in (b'9991234567', b'9\0'+b'9'*8, b'+999123456', b'99912x4567'):
            for accepted in (0, 1):
                m = ServiceMachine(); sent, logs = [], []
                m.u.mem_write(m.addr('config') + fields('config_s')['countryCode'], b'+7\0'+bytes(5))
                def send():
                    sent.append(bytes(m.u.mem_read(m.args()[0], 13)))
                    assert not m.answers, 'answer precedes SMS queue result'
                    m.ret(accepted)
                def log():
                    logs.append(m.args()[0]); m.ret(1)
                m.stubs[m.addr('user_comm_sms')] = send
                m.stubs[m.addr('log_addSystemEventID')] = log
                execute(m, 11, number+b'hello', channel)
                valid = number == b'9991234567'
                assert len(sent) == valid and len(logs) == (valid and accepted)
                assert m.answers == [(11, (0 if accepted else 2) if valid else 1, channel, 42)]
                assert not owned(m)
                cases += 1
    return cases


def rollback():
    cases = 0
    for channel in (0, 1):
        for fault in ('start', 'poll'):
            m = FirmwareMachine(); m.fixture(); m.begin(1, channel)
            m.write(m.device + m.d['registredOnServer'], 1, 1)
            m.until(lambda: m.phase() == 10)
            ctx = m.ptr(); m.checked('user_storage_poll')
            assert m.exec_active(3)
            writes = []
            def watch(u, access, address, size, value, data):
                if ctx <= address < ctx+128:
                    assert owned(m) & (1 << (FW_CONTEXT-1)), 'write after release'
                    writes.append(address)
            hook = m.u.hook_add(UC_HOOK_MEM_WRITE, watch, begin=ctx, end=ctx+127)
            if fault == 'poll': m.checked('fw_service_poll')
            m.fault = (fault, m.counts[fault]+1)
            m.busy = True
            for _ in range(12): m.tick_fw()
            assert writes and m.ptr() == ctx and owned(m) & (1 << (FW_CONTEXT-1))
            m.busy = False; m.drain()
            assert m.sent[-1][8] == 4 and m.sent[-1][10] == 253
            assert not owned(m) & (1 << (FW_CONTEXT-1)) and not m.checked('fw_service_busy')
            m.u.hook_del(hook)
            m.until(lambda: not owned(m))
            cases += 1
    return cases


if __name__ == '__main__':
    result = dict(caller=caller(), sms=sms(), rollback=rollback(), status='PASS')
    result_path('review-local.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
