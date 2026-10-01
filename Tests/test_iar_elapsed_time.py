"""B29: настоящий user_loop, накопленные секунды и проба по времени."""
import sys
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from iar_layout import fields

mode=sys.argv[1] if len(sys.argv)>1 else 'both'
off=fields('device_s')
if mode in ('loop','both'):
    for ms in (5250,60250):
        m=Machine();m.init();m.u.mem_map(0x42000000,0x600000)
        d=m.addr('device');timer=m.addr('timer')
        for name in ('out_process','user_indication','log_checkPending','user_checkIdTimeEnd','user_saveindex','usergsm_getcsq','connection_state','usergsm_getNetType'):
            m.stubs[m.addr(name)]=lambda:m.ret(0)
        m.write(d+off['datetime'],1600000000);m.write(d+off['worktime'],100)
        m.write(d+off['updateTimeout'],600,2);m.write(timer+12,ms)
        m.call('user_loop',budget=3000000)
        assert m.read(d+off['datetime'])==1600000000+ms//1000, 'B29 datetime lost seconds'
        assert m.read(d+off['worktime'])==100+ms//1000
        assert m.read(d+off['updateTimeout'],2)==600-ms//1000
        assert m.read(timer+12)==ms%1000, 'B29 fractional second lost'
        m.write(timer+12,600);m.call('user_loop');assert m.read(timer+12)==600
    if 'timers_take' in m.symbols:
        m.stubs[m.addr('timers_user_handle')]=lambda:m.ret()
        for edge in range(30):
            m.write(timer+12,5250)
            assert m.call('timers_take',3,1000,inject_at=edge,irq=('timers_handle',()))==5
            assert m.read(timer+12) in (250,251)
    print('B29 IAR: 5/60 second delays, timeout and fractional remainder, atomic take PASS')
if mode in ('trial','both'):
    m=Machine();m.init();m.u.mem_write(0x08007ffc,b'\x32')
    def read():
        a=m.u.reg_read(UC_ARM_REG_R0);p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R2)
        assert n==4
        m.write(p,0 if a in (0x20004,0x20018) else 0xffffffff);m.ret(1)
    m.stubs[m.addr('mem_read')]=read
    m.call('app_boot_metadata');assert m.call('app_trial_active')
    for _ in range(1801):assert not m.call('app_trial_expired'), 'B29 trial counts calls instead of time'
    m.write(m.addr('timer')+6*4,1799999);assert not m.call('app_trial_expired')
    m.write(m.addr('timer')+6*4,1800000);assert m.call('app_trial_expired')
    print('B29 IAR: trial expires at 1800 seconds, independent of poll count PASS')
