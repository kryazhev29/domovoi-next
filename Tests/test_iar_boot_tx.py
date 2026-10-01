"""B37: настоящая очередь TX загрузчика, stalled host и IRQ между инструкциями."""
import sys
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R2

mode=sys.argv[1] if len(sys.argv)>1 else 'both'
def fixture():
    m=Machine(boot=True);m.init();m.write(m.addr('bDeviceState'),5)
    m.u.mem_write(0x21000000,bytes(range(128)))
    return m
if mode in ('stall','both'):
    for state in (1,0):
        m=fixture();size=m.map_size('USART_Rx_Buffer')
        m.write(m.addr('USART_Rx_ptr_in'),size-1);m.write(m.addr('USB_Tx_State'),state,1)
        before=bytes(m.u.mem_read(m.addr('USART_Rx_Buffer'),size))
        assert not m.call('USART_To_USB_Send_Data',0x21000000,128,budget=7000000)
        assert m.read(m.addr('USART_Rx_ptr_in'))==size-1
        assert bytes(m.u.mem_read(m.addr('USART_Rx_Buffer'),size))==before
    print('B37 IAR: stalled host TX active/idle returns without partial enqueue PASS')
if mode in ('irq','both'):
    for edge in range(450):
        m=fixture();packets=[]
        def pma():
            p=m.u.reg_read(UC_ARM_REG_R0);n=m.u.reg_read(UC_ARM_REG_R2)
            packets.append(bytes(m.u.mem_read(p,n)));m.ret()
        m.stubs[m.addr('UserToPMABufferCopy')]=pma
        assert m.call('USART_To_USB_Send_Data',0x21000000,63,inject_at=edge,irq=('Handle_USBAsynchXfer',()))
        m.call('Handle_USBAsynchXfer')
        assert packets==[bytes(range(63))], ('B37 partial packet visible to IRQ',edge,packets)
    for name in ('USART_Rx_ptr_in','USART_Rx_ptr_out','USART_Rx_length','USB_Tx_State'):
        assert m.volatile[name], ('B37 missing volatile',name)
    m.call('USB_Tx_Reset')
    assert all(not m.read(m.addr(n),1 if n=='USB_Tx_State' else 4) for n in ('USART_Rx_ptr_in','USART_Rx_ptr_out','USART_Rx_length','USB_Tx_State'))
    print('B37 IAR: 450 IRQ boundaries, atomic publication, volatile and queue reset PASS')
