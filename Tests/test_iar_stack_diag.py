"""Команда 111/поле 57: полный USB-кадр, настоящий IAR, никакого COM."""
import struct
from iar_irq_support import Machine,UC_ARM_REG_R0,UC_ARM_REG_R1

def crc16(data):
    crc=0xffff
    for ch in data:
        crc^=ch
        for _ in range(8):crc=(crc>>1)^((crc&1)*0xa001)
    return crc

m=Machine();m.init();m.call('stack_paint');m.u.mem_write(m.bottom+1024,b'\0\0\0\0')
frame=b'SLDN'+b'\x03\0\0\1\x6f';frame+=struct.pack('<H',crc16(frame[4:]))
captured=[]
def sent():
    p=m.u.reg_read(UC_ARM_REG_R0);n=m.u.reg_read(UC_ARM_REG_R1)
    captured.append(bytes(m.u.mem_read(p,n)));m.ret(1)
m.stubs[m.addr('usb_send')]=sent
m.u.mem_write(0x21000000,frame);m.call('usb_data_reading',0x21000000,len(frame),budget=3000000)
assert len(captured)==1
reply=captured[0];assert reply[:4]==b'SLDN' and reply[7:9]==b'\1\x6f'
body=int.from_bytes(reply[4:6],'little');assert len(reply)==body+8
assert crc16(reply[4:-2])==int.from_bytes(reply[-2:],'little')
assert int.from_bytes(reply[9+57*4:13+57*4],'little')==1024
print('Stack diagnostics: request',frame.hex(' '),'field 57=1024, CRC/frame PASS; no hardware')
