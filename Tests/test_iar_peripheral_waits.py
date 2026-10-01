"""B34: stuck flags в настоящем коде IAR; отказ не становится успехом."""
import re,sys
from iar_irq_support import Machine
from iar_layout import fields

mode=sys.argv[1] if len(sys.argv)>1 else 'all'
if mode in ('spi','all'):
    for sr in (0,2,0x83):
        m=Machine();m.init();m.u.mem_map(0x42000000,0x600000);m.write(0x40013000,0x40,2);m.write(0x40013008,sr,2)
        m.call('spi_read_byte',budget=1500000)
        assert not m.read(0x40013000,2)&0x40,'B34 SPI fault must disable peripheral'
        assert not m.call('w25_read_data',0,0x21000000,1,budget=1500000),'B34 SPI failure reported as read success'
    print('B34 IAR SPI TXE/RXNE/BSY stuck: bounded, W25 returns failure PASS')
if mode in ('uart','all'):
    for port in range(3):
        m=Machine();m.init();base=(0x40013800,0x40004400,0x40004800)[port]
        m.write(base+12,0x2000,2);m.u.mem_write(0x21000000,b'abc\0')
        m.call('uart_sendstr',port,0x21000000,budget=1500000)
        assert m.read(m.addr('uart')+port*16+m.members['txfailed'],1),'B34 UART failure not latched'
        assert not m.read(base+4,2),'B34 UART sent despite TXE timeout'
    print('B34 IAR UART1/2/3 TXE stuck: bounded, no transmission, error latched PASS')
if mode in ('adc','all'):
    m=Machine();m.init();m.call('adc_init',budget=1500000)
    assert not m.read(0x40012408)&1,'B34 failed calibration left ADC enabled'
    from unicorn import UC_HOOK_MEM_READ
    m=Machine();m.init()
    def reset_calibration(u,access,a,n,value,_):
        if a==0x40012408:m.write(a,m.read(a)&~8)
    m.u.hook_add(UC_HOOK_MEM_READ,reset_calibration);m.call('adc_init',budget=1500000)
    assert not m.read(0x40012408)&1,'B34 second calibration stuck'
    for cr2 in (1,1|(1<<22)):
        m=Machine();m.init();m.write(0x40012408,cr2)
        assert not m.call('adc_startInjectedConvertion',budget=1500000)
        assert not m.read(0x40012408)&1,'B34 failed conversion left ADC enabled'
    print('B34 IAR ADC calibration/EOC/JEOC stuck: bounded, conversion rejected PASS')
if mode in ('clock','all'):
    for cr in (0,0x20000,0x2020000):
        m=Machine();m.init();m.write(0x40021000,cr);resets=[]
        for value in re.findall(r"^NVIC_SystemReset\s+0x([0-9a-f']+)",m.maptext,re.M):
            m.stubs[int(value.replace("'",''),16)&~1]=lambda:(resets.append(1),m.ret())
        m.call('SetSysClockTo72',budget=2000000)
        assert len(resets)==1,'B34 startup clock failure must request reset'
    print('B34 IAR HSE/PLL lock/SYSCLK switch stuck: bounded, startup reset requested PASS')
if mode in ('send','all'):
    m=Machine();m.init();off=fields('Connection_t');ucon=m.addr('ucon')
    m.write(ucon+off['count']-1,1,1)
    m.stubs[m.addr('gsm_senddata')]=lambda:m.ret(0)
    assert m.call('connection_send',0,0x21000000,10)==5,'B34 failed modem send returned CE_OK'
    m=Machine();m.init();prompt=[True];reply=[True];txfail=[False];calls=[];writes=[]
    def command():
        from iar_irq_support import UC_ARM_REG_R0
        calls.append(m.u.reg_read(UC_ARM_REG_R0))
        m.ret(0x21000000 if (prompt[0] if len(calls)==1 else reply[0]) else 0)
    m.stubs[m.addr('gsm_runcmd')]=command
    m.stubs[m.addr('uart_send')]=lambda:(writes.append(1),m.ret())
    m.stubs[m.addr('uart_tx_failed')]=lambda:m.ret(int(txfail[0]))
    for p,t,r,expected in ((False,False,True,0),(True,True,True,0),(True,False,False,0),(True,False,True,1)):
        prompt[0]=p;txfail[0]=t;reply[0]=r;calls.clear();writes.clear()
        assert m.call('gsm_senddata',0x21000000,10,0)==expected
        assert len(writes)==int(p) and len(calls)==(2 if p and not t else 1)
    print('B34 IAR: failed modem/UART send returns CE_BUSY PASS')
if mode in ('healthy','all'):
    from unicorn import UC_HOOK_MEM_READ
    m=Machine();m.init();m.write(0x40013000,0x40,2);m.write(0x40013008,3,2)
    assert m.call('spi_send_byte',0x5a)==0x5a and m.call('spi_ready')
    m.call('spi_read',0x21000000,3);assert bytes(m.u.mem_read(0x21000000,3))==b'\xff'*3
    m.write(0x4001380c,0x2000,2);m.write(0x40013800,0x80,2)
    m.u.mem_write(0x21000000,b'abc\0');m.call('uart_sendstr',0,0x21000000)
    assert not m.call('uart_tx_failed',0) and m.read(0x40013804,2)==ord('c')
    def calibration(u,access,a,n,value,_):
        if a==0x40012408:m.write(a,m.read(a)&~12)
    m.u.hook_add(UC_HOOK_MEM_READ,calibration);m.call('adc_init');assert m.call('adc_ready')
    m.write(0x40012400,6);assert m.call('adc_startInjectedConvertion')
    m.write(0x40012400,0);before=bytes(m.u.mem_read(m.addr('device'),m.map_size('device')))
    m.call('user_readinputs',budget=1500000)
    assert bytes(m.u.mem_read(m.addr('device'),m.map_size('device')))==before
    print('B34 IAR: healthy SPI/UART/ADC, input states retained on ADC failure PASS')
