"""B41: границы размера на настоящем IAR user_send_anyData."""
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1

m = Machine(); m.init()
allocated = []; sent = []; accept = False
def alloc():
    allocated.append(m.u.reg_read(UC_ARM_REG_R0))
    m.ret(0x21000200 if accept else 0)
def send():
    p = m.u.reg_read(UC_ARM_REG_R0); n = m.u.reg_read(UC_ARM_REG_R1)
    sent.append(bytes(m.u.mem_read(p, n))); m.ret(1)
m.stubs[m.addr('__basic_malloc')] = alloc
m.stubs[m.addr('__basic_free')] = lambda: m.ret()
m.stubs[m.addr('usb_send')] = send
for size in range(65525, 65536):
    allocated.clear(); m.call('user_send_anyData', 103, 0x21000000, size, 0)
    assert not allocated and not sent, ('B41 oversized frame allocated', size, allocated)
allocated.clear(); m.call('user_send_anyData', 103, 0x21000000, 65524, 0)
assert allocated == [65535] and not sent
allocated.clear(); m.call('user_send_anyData', 103, 0, 1, 0)
assert not allocated and not sent, 'B41 NULL nonempty payload accepted'
accept = True
for data in (b'', b'abc'):
    m.u.mem_write(0x21000000, data or b'\0'); allocated.clear(); sent.clear()
    m.call('user_send_anyData', 103, 0x21000000 if data else 0, len(data), 0)
    assert allocated == [len(data)+11] and len(sent) == 1
    frame = sent[0]
    assert frame[:4] == b'SLDN' and int.from_bytes(frame[4:6], 'little') == len(data)+3
    assert frame[6] == 255 and frame[8] == 103 and frame[9:-2] == data
print('B41 IAR: frame limit, overflow, NULL, OOM and unchanged normal framing PASS')
