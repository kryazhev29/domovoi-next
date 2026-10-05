"""Final ASM USB -> command -> storage, and the strong/weak callback contract."""
from storage_machine import *
from test_table_runtime import rows

for mask in (0,1):
    m=StorageMachine();body=struct.pack('<HBBB',3,0,19,27)
    packet=b'SLDN'+body+struct.pack('<H',crc16(body))
    m.command(13,rows([1]),1);assert m.active()
    m.u.mem_write(0x21000000,packet);m.checked('usb_datareceived',0x21000000,len(packet))
    m.u.mem_write(0x21000000,b'\xa5'*len(packet))
    count=m.addr('usb_buffer_count');assert m.read_budget==0
    m.u.reg_write(UC_ARM_REG_PRIMASK,mask);m.checked('usb_check_data')
    assert m.u.reg_read(UC_ARM_REG_PRIMASK)==mask and not m.answers
    assert int.from_bytes(m.u.mem_read(count,2),'little')==len(packet),'busy owner consumed USB command'
    m.drain();m.checked('usb_check_data')
    assert m.answers[-1][:2]==(27,0),'weak callback replaced the application parser'
    assert m.u.mem_read(count,2)==b'\0\0' and m.u.reg_read(UC_ARM_REG_PRIMASK)==mask
print('PASS actual USB ASM: deferred command retained, strong parser called, CRC/snapshot, AAPCS/PRIMASK')
