"""Sequential mutations of linked Thumb instructions (not the C specification)."""
import test_photo_edges as edge
import test_photo_runtime as runtime
Original=runtime.PhotoMachine

for mode in ('pending','checksum','backup-crc','packet-advance','cancel'):
    applied=[]
    class Mutant(Original):
        def __init__(self,*args):
            super().__init__(*args)
            name='pr_frame' if mode=='checksum' else 'photo_storage_cancel' if mode=='cancel' else 'photo_storage_poll'
            a=self.addr(name);code=bytes(self.u.mem_read(a,200 if mode in ('checksum','pending') else 4000 if mode in ('backup-crc','packet-advance') else 40))
            if mode=='cancel':
                off=4;assert code[off:off+2]==b'\x01\x21';replacement=b'\0\x21'
            elif mode=='packet-advance':
                pattern=b'\xc1\x8b\x49\x1c\xc1\x83'
                assert code.count(pattern)==1;off=code.index(pattern)+2;replacement=b'\x89\x1c'
            else:
                pattern={'pending':b'\x01\x28','checksum':b'\xb5\x42',
                         'backup-crc':b'\x39\x69\x78\x69\x81\x42'}[mode]
                assert code.count(pattern)==1,(mode,'pattern count',code.count(pattern))
                off=code.index(pattern)+len(pattern);word=int.from_bytes(code[off:off+2],'little')
                assert word&0xf000==0xd000,(mode,hex(word))
                replacement=(word^0x100).to_bytes(2,'little') if mode=='pending' else b'\0\xbf'
            self.u.mem_write(a+off,replacement);applied.append(a+off)
    # Patch setup is audited independently of the failure expected below.
    probe=Mutant();assert len(applied)==1;applied.clear()
    edge.PhotoMachine=runtime.PhotoMachine=Mutant
    try:
        if mode=='cancel':edge.cancellation()
        elif mode=='backup-crc':edge.corrupt()
        elif mode=='checksum':edge.stream()
        else:
            m=Mutant();m.boot_media();m.capture(b'x'*116)
    except AssertionError:pass
    else:raise AssertionError((mode,'survived'))
    finally:edge.PhotoMachine=runtime.PhotoMachine=Original
    assert applied,(mode,'not applied')
    print(mode,'DETECTED in linked Thumb-2',flush=True)
print('PASS 5/5 photo ASM instruction mutations')
