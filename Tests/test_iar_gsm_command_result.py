"""B34: реальный gsm_runcmd обязан передавать отказ UART вызывающему коду."""
from iar_irq_support import Machine
m=Machine();m.init();m.u.mem_write(0x21000000,b'AT\r\0OK\0')
m.stubs[m.addr('uart_sendStrAndWaitResp')]=lambda:m.ret(0)
m.stubs[m.addr('gsm_user_debug')]=lambda:m.ret()
assert m.call('gsm_runcmd',0x21000000,0x21000004)==0,'B34: gsm_runcmd swallowed UART timeout'
m.stubs[m.addr('uart_sendStrAndWaitResp')]=lambda:m.ret(1)
assert m.call('gsm_runcmd',0x21000000,0x21000004)==m.addr('gsm_strbuf')
print('B34 real gsm_runcmd: timeout NULL / success response PASS')
