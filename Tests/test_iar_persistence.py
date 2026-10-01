"""B25: функции IAR, настоящий allocator, отказы и отсутствие утечек."""
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
m=Machine();m.init()
failure=0;writes=0
def load():
    address=m.u.reg_read(UC_ARM_REG_R0);p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R2)
    if failure==2:m.ret(0);return
    m.u.mem_write(p,(b'\x07\x00' if n==2 else bytes(n)));m.ret(1)
def save():
    global writes
    if failure==2:m.ret(0);return
    writes+=1;m.ret(int(failure!=3))
m.stubs[m.addr('mem_read')]=load
m.stubs[m.addr('mem_write')]=save
for n in ('mem_rewrite','mem_patch','mem_patch2'):
    if n in m.symbols:m.stubs[m.addr(n)]=save
m.stubs[m.addr('wdt_reload')]=lambda:m.ret()
mem=m.addr('mem')
m.write(mem+6*8,0x1CD400);m.write(mem+6*8+4,4096)
m.write(mem+2*8,0);m.write(mem+2*8+4,4096)
cell=0x21000000;m.u.mem_write(cell,b'\x07\x00'+bytes(30))
def largest():
    lo,hi=0,m.heap_bytes
    while lo<hi:
        mid=(lo+hi+1)//2;p=m.call('__basic_malloc',mid)
        if p:m.call('__basic_free',p);lo=mid
        else:hi=mid-1
    return lo
cold=largest();maximum=0
for failure in (0,2,3):
    for function,args in [('user_writeConfigAndDevice',()),('user_updateID',(cell,))]:
        writes=0;result=m.call(function,*args);maximum=max(maximum,m.top-m.minimum)
        assert bool(result)==(failure==0),(function,failure,result)
        if failure==2:assert writes==0
        assert largest()==cold,(function,'heap leak')
failure=0;held=m.call('__basic_malloc',cold);assert held
assert m.call('user_writeConfigAndDevice') and m.call('user_updateID',cell)
m.call('__basic_free',held);assert largest()==cold
print(f'IAR persistence PASS: HEAP {m.heap_bytes} from map, exhausted heap tolerated, read/write errors, no leaks; executed stack {maximum} bytes with memory callbacks stubbed')
