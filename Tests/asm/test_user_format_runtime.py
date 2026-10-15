"""Побайтная проверка форматтеров ELF, включая шесть аргументов через стек."""
import struct
from iar_irq_support import Machine, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_PC


def run():
    m=Machine();m.init();dst=0x21000000;network=0x21000200;cases=0
    values={0,1,9,10,99,100,999,1000,0x7fffffff,0x80000000,0xfffffffe,0xffffffff}
    for bit in range(32):
        values.update(((1<<bit)-1,1<<bit,min(0xffffffff,(1<<bit)+1)))
    for value in sorted(values):
        for name,expected in [('format_id',f'00{value:08X}'),('format_i32',str(value if value<0x80000000 else value-0x100000000))]:
            m.u.mem_write(dst,b'\xA7'*128)
            length=m.call(name,dst,value)
            assert length==len(expected)
            assert bytes(m.u.mem_read(dst,length+2))==expected.encode()+b'\x00\xA7'
            cases+=1


    signals=(0,1,31,99,100,255,0xffffffff)
    networks=('Not network','gprs','edge','3G','4G','5G')
    for signal in signals:
        for net in networks:
            for bits in range(16):
                firmware=(0,1,9999,65535,0xffffffff)[bits%5]
                connected=bits&1;server=(bits>>1)&1
                triggers=[(bits>>i)&1 for i in range(4)]
                m.u.mem_write(dst,b'\xA7'*128);m.u.mem_write(network,net.encode()+b'\x00')
                sp=m.top-24;m.u.mem_write(sp,struct.pack('<6I',server,firmware,*triggers))
                m.u.reg_write(UC_ARM_REG_SP,sp);m.u.reg_write(UC_ARM_REG_LR,m.stop|1)
                for reg,value in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),(dst,signal,network,connected)):m.u.reg_write(reg,value)
                m.minimum=sp;m.u.emu_start(m.addr('format_status')|1,m.stop,count=20000)
                assert m.u.reg_read(UC_ARM_REG_PC)==m.stop
                assert m.u.reg_read(UC_ARM_REG_SP)==sp
                expected=f'STATUS:\r\n-SIGNAL: {signal}\r\n-NET: {net}\r\n-CLOUD: {connected}/{server}\r\n-FW: {firmware}\r\n-TRIG: '+ '/'.join(map(str,triggers))
                assert m.u.reg_read(UC_ARM_REG_R0)==len(expected)
                assert bytes(m.u.mem_read(dst,len(expected)+2))==expected.encode()+b'\x00\xA7'
                cases+=1
    comparisons=0
    if '__iar_small_memcmp' in m.symbols:
        for left_align in range(8):
            for right_align in range(8):
                left=0x21000300+left_align;right=0x21000400+right_align
                for changed in range(-1,10):
                    a=bytes((i*23+17)&255 for i in range(10));b=bytearray(a)
                    if changed>=0:b[changed]^=0x81
                    m.u.mem_write(left,a);m.u.mem_write(right,bytes(b))
                    actual=m.call('__iar_small_memcmp',left,right,10)
                    signed=actual if actual<0x80000000 else actual-0x100000000
                    assert (signed>0)-(signed<0)==(a>bytes(b))-(a<bytes(b))
                    comparisons+=1
        assert m.call('__iar_small_memcmp',0,0,0)==0
    return {'cases':cases,'string_library_cases':comparisons,'wire':'exact bytes, NUL, end canary','abi':'AAPCS registers and six stack arguments'}
