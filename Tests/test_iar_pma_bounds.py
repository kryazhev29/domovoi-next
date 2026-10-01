"""B33: реальные PMA-функции IAR, буфер заканчивается перед unmapped page."""
import sys
from iar_irq_support import Machine

m = Machine(); m.init(); mode = sys.argv[1] if len(sys.argv)>1 else 'both'
for n in range(128):
    p = 0x21001000-n
    payload = bytes((i*17+29)&255 for i in range(n))
    m.u.mem_write(0x40006000, b'\xa5'*256)
    if mode in ('tx', 'both'):
        if n: m.u.mem_write(p, payload)
        m.call('UserToPMABufferCopy', p if n else 0, 0, n)
        out = b''.join(bytes(m.u.mem_read(0x40006000+i*4,2)) for i in range((n+1)//2))
        assert out[:n] == payload and (n%2 == 0 or out[-1] == 0)
    if mode in ('rx', 'both'):
        for i in range((n+1)//2):
            chunk = payload[i*2:i*2+2].ljust(2,b'\xee')
            m.u.mem_write(0x40006000+i*4,chunk+b'\xa5\xa5')
        if n: m.u.mem_write(p,b'\xcc'*n)
        m.call('PMAToUserBufferCopy', p if n else 0, 0, n)
        assert not n or bytes(m.u.mem_read(p,n)) == payload
print('B33 IAR: PMA TX/RX 0..127 bytes, odd/even, unaligned, guarded ring tail PASS')
