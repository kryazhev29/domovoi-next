"""Independent final-ELF boundary/IRQ contracts for nonblocking usb_try_send."""
import hashlib, json, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from iar_irq_support import *
from unicorn import UC_HOOK_MEM_READ


def init_transport(m, *, inp=0, out=0, enabled=1, configured=5,
                   epoch=7, stored_epoch=None, connected=True):
    """Initialize only linked transport state; works with any Machine subclass."""
    for name,value in [('USART_Rx_ptr_in',inp),('USART_Rx_ptr_out',out),
                       ('bDeviceState',configured),('usb_bus_resets',epoch)]:
        m.write(m.addr(name),value)
    m.write(m.addr('usb_enabled'),enabled,1)
    m.write(m.addr('store')+12,epoch if stored_epoch is None else stored_epoch)
    m.write(0x40010c08,32768 if connected else 0)


class UsbTryMachine(Machine):
    expected_stack = 16
    instruction_limit = 15000
    def __init__(self):
        super().__init__();self.size=self.map_size('USART_Rx_Buffer');self.init();self.observations=[];self.writes=[];self.reads=[]
        self.observe=False;self.copy_started=False;self.last_source=0;self.pending_action=None
        # Hook the observed regions. Unicorn 2.1.4 global memory hooks can
        # misexecute conditional Thumb POP/IT instructions in compiler output.
        self.u.hook_add(UC_HOOK_MEM_WRITE,self.memory_write,begin=0x20000000,end=self.bottom-1)
        self.u.hook_add(UC_HOOK_MEM_READ,self.memory_read,begin=0x21000000,end=0x21000fff)
        self.u.hook_add(UC_HOOK_CODE,self.check_publish)
    def memory_write(self,u,access,p,n,value,x):
        if not self.observe:return
        ring=self.addr('USART_Rx_Buffer');ip=self.addr('USART_Rx_ptr_in')
        assert ring<=p<p+n<=ring+self.size or p==ip and n==4 or self.bottom<=p<p+n<=self.top,hex(p)
        if ring<=p<ring+self.size or p==ip:
            assert self.u.reg_read(UC_ARM_REG_PRIMASK)==1,'unprotected ring mutation'
            self.writes.append((p,n,value))
    def memory_read(self,u,access,p,n,value,x):
        if self.observe and 0x21000000<=p<0x21001000:
            self.reads.append((p,n))
    def check_publish(self,u,pc,n,x):
        if not self.observe:return
        current=self.read(self.addr('USART_Rx_ptr_in'))
        expected=(self.start_in+self.length)%self.size
        assert current in (self.start_in,expected) or self.reset_fired and current==0,'partial in-pointer publication'
        if not self.u.reg_read(UC_ARM_REG_PRIMASK):
            if current==expected and self.length and expected!=self.start_in:
                ring=bytes(self.u.mem_read(self.addr('USART_Rx_Buffer'),self.size))
                assert ring==self.expected_ring,'IRQ observed pointer before payload'
            self.observations.append((self.steps,current))
            if self.pending_action is not None:
                at,kind=self.pending_action
                if self.steps>=at:
                    self.pending_action=None
                    if kind=='reset':
                        self.reset_fired=True
                        self.write(self.addr('usb_bus_resets'),8)
                        self.write(self.addr('USART_Rx_ptr_in'),0)
                        self.write(self.addr('USART_Rx_ptr_out'),0)
                    else:
                        self.observed_irq=bytes(self.u.mem_read(self.addr('USART_Rx_Buffer'),self.size)),current
    def run(self,inp=0,out=0,length=64,mask=0,enabled=1,configured=5,epoch=7,stored_epoch=7,connected=True,null=False,irq=None):
        self.observe=False
        init_transport(self,inp=inp,out=out,enabled=enabled,configured=configured,
                       epoch=epoch,stored_epoch=stored_epoch,connected=connected)
        self.before=bytes((i*37+11)&255 for i in range(self.size))
        self.u.mem_write(self.addr('USART_Rx_Buffer'),self.before)
        self.payload=bytes((i*19+83)&255 for i in range(self.size))
        self.u.mem_write(0x21000000,self.payload+b'GUARD')
        self.start_in=inp;self.length=length
        expectation=bytearray(self.before)
        if inp<self.size and length<self.size:
            for i in range(length):expectation[(inp+i)%self.size]=self.payload[i]
        self.expected_ring=bytes(expectation)
        self.observations=[];self.writes=[];self.reads=[];self.pending_action=irq;self.observed_irq=None;self.reset_fired=False
        regs=(UC_ARM_REG_R4,UC_ARM_REG_R5,UC_ARM_REG_R6,UC_ARM_REG_R7,UC_ARM_REG_R8,UC_ARM_REG_R9,UC_ARM_REG_R10,UC_ARM_REG_R11)
        saved=[0x13572468+i*67 for i in range(8)]
        for r,v in zip(regs,saved):self.u.reg_write(r,v)
        self.u.reg_write(UC_ARM_REG_PRIMASK,mask);self.observe=True
        try:result=self.call('usb_try_send',0 if null else 0x21000000,length,budget=15000)
        finally:self.observe=False
        assert self.u.reg_read(UC_ARM_REG_PRIMASK)==mask
        assert self.u.reg_read(UC_ARM_REG_SP)==self.top
        if self.expected_stack is not None:assert self.top-self.minimum==self.expected_stack
        assert [self.u.reg_read(r) for r in regs]==saved
        assert bytes(self.u.mem_read(0x21000000,(self.size+5)))==self.payload+b'GUARD'
        if self.instruction_limit is not None:assert self.steps<self.instruction_limit,'busy wait or unbounded copy'
        ring=bytes(self.u.mem_read(self.addr('USART_Rx_Buffer'),self.size));new_in=self.read(self.addr('USART_Rx_ptr_in'))
        if result:
            assert len(self.reads)==length and all(n==1 for p,n in self.reads)
            assert [p for p,n in self.reads]==list(range(0x21000000,0x21000000+length))
            assert ring==self.expected_ring
            assert sum(1 for p,n,v in self.writes if p==self.addr('USART_Rx_ptr_in'))==1
            # A reset delivered after atomic publish may immediately clear ptr.
            assert new_in==(inp+length)%self.size or irq and irq[1]=='reset' and new_in==0
            assert sum(n for p,n,v in self.writes if p!=self.addr('USART_Rx_ptr_in'))==length
        else:
            assert not self.reads and not self.writes
            assert ring==self.before
            assert new_in==inp or irq and irq[1]=='reset' and new_in==0
        return result,self.steps


def mutations():
    m=UsbTryMachine()
    plans=[
        ('mask',b'\x72\xb6',b'\x00\xbf',dict(),1),
        ('epoch',b'\x9a\x42',b'\x00\xbf',dict(epoch=8),0),
        ('wrap',b'\x08\xbf\x00\x23',b'\x08\xbf\x01\x23',dict(inp=m.size-1,out=m.size-1,length=2),1),
        ('full_queue',b'\xaa\x42\x11\xd3',b'\xaa\x42\x00\xbf',dict(inp=0,out=1,length=1),0),
        ('configured',b'\x05\x2a',b'\x04\x2a',dict(),1),
    ]
    out=[]
    for label,before,after,args,expected in plans:
        m=UsbTryMachine();a=m.addr('usb_try_send');raw=bytes(m.u.mem_read(a,164))
        assert raw.count(before)==1,(label,'mutation signature')
        m.u.mem_write(a+raw.index(before),after)
        try:
            result,_=m.run(**args)
            assert result==expected
        except AssertionError:out.append(label)
        else:raise AssertionError(('mutation escaped',label))
    return dict(detected=len(out),total=len(plans),cases=out,
        scope='instruction copies only in Unicorn RAM; production files and ELF unchanged')


def run():
    m=UsbTryMachine();cases=0;accepted=[];refused=[]
    raw=bytes(m.u.mem_read(m.addr('usb_try_send'),512))
    # This leaf has exactly two BX LR returns, before its literal pool.
    first=raw.index(b'\x70\x47');end=raw.index(b'\x70\x47',first+2)+2
    code=raw[:end]
    assert code.count(b'\xbf\xf3\x5f\x8f')==1,'linked DMB SY is missing'
    assert code.count(b'\x72\xb6')==1,'linked CPSID I is missing'
    barrier=code.index(b'\xbf\xf3\x5f\x8f')
    assert b'\x13\x60' in code[barrier+4:barrier+12],'publish STR R3,[R2] must follow DMB'
    for mask in (0,1):
        for inp in (0,1,m.size//2,m.size-2,m.size-1):
            for out in (0,1,m.size//2,m.size-2,m.size-1,m.size):
                for length in (0,1,64,m.size-2,m.size-1,m.size,65535):
                    result,steps=m.run(inp,out,length,mask)
                    free=((0 if out==m.size else out)+(m.size-1)-inp)%m.size
                    assert result==int(length<m.size and length<=free),(inp,out,length,result,free)
                    (accepted if result else refused).append(steps);cases+=1
    failures=0
    for opts in [dict(enabled=0),dict(configured=0),dict(configured=4),dict(epoch=8),dict(stored_epoch=6),dict(connected=False),dict(null=True),dict(inp=m.size),dict(inp=0xffffffff),dict(out=(m.size+1)),dict(out=0xffffffff)]:
        for mask in (0,1):
            assert m.run(mask=mask,**opts)[0]==0;failures+=1
    for mask in (0,1):assert m.run(length=0,null=True,mask=mask)[0]==1
    # Every instruction boundary on a short wrap copy: ISR reset either occurs
    # before lock (epoch mismatch rejects) or after complete atomic publish.
    baseline=m.run((m.size-3),(m.size-3),8)[1];irqcases=0
    for point in range(baseline+1):
        m.run((m.size-3),(m.size-3),8,irq=(point,'reset'));irqcases+=1
    for point in (0,10,20,35,100,7000,11500):
        m.run(0,0,(m.size-1),irq=(point,'observe'));irqcases+=1
    return dict(status='PASS',queue_boundary_cases=cases,rejected_conditions=failures,
        nullable_zero_cases=2,irq_boundary_cases=irqcases,maximum_instructions=max(accepted),
        maximum_rejection_instructions=max(refused),stack=16,maximum_copy_bytes=(m.size-1),
        atomic_publish='all payload stores under PRIMASK1; pointer published once after payload and linked DMB',
        code_sha256=hashlib.sha256(code).hexdigest(),mutations=mutations(),
        units='Unicorn instructions; not board cycles/time',
        elf_sha256=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest())


if __name__=='__main__':
    out=run();Path(__file__).with_name('verification_usb_try_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out,indent=2))
