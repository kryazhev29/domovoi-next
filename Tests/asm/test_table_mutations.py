"""Sequential actual Thumb instruction mutations of journal critical conditions."""
import test_table_runtime as test
from storage_machine import *
Original=test.StorageMachine
for mode in ('pending','cancel','crc'):
    applied=[]
    class Mutant(Original):
        def __init__(self):
            super().__init__()
            if mode=='cancel':
                a=self.addr('sj_runtime_cancel');assert bytes(self.u.mem_read(a,2))==b'\x01\x21'
                self.u.mem_write(a,b'\0\x21');applied.append(a);return
            a=self.addr('sj_runtime_poll');code=bytes(self.u.mem_read(a,700))
            pattern=b'\x01\x28' if mode=='pending' else b'\x88\x42'
            for off in range(0,len(code)-4,2):
                if code[off:off+2]!=pattern:continue
                word=int.from_bytes(code[off+2:off+4],'little')
                if (word&0xff00)!=(0xd000 if mode=='pending' else 0xd100):continue
                self.u.mem_write(a+off+2,(word^0x100).to_bytes(2,'little') if mode=='pending' else b'\0\xbf')
                applied.append(a+off+2);break
    test.StorageMachine=Mutant
    try:
        if mode=='cancel':test.test_cancel_and_reconnect()
        elif mode=='crc':test.test_corrupt()
        else:test.test_normal()
    except AssertionError:pass
    else:raise AssertionError((mode,'survived'))
    finally:test.StorageMachine=Original
    assert applied,(mode,'not applied')
    print(mode,'DETECTED in linked ASM',flush=True)
print('PASS 3/3 table instruction mutations')
