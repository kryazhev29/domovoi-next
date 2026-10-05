"""Collect actual linked resources and observed allocator lifetimes.

--root selects the final ELF tree. No build, source mutation, board or timing.
Observed traces are examples, not a proof of a global reachable heap/stack peak.
Allocator stress is reported separately from admitted production operations.
"""
import argparse,hashlib,json,re,sys
from pathlib import Path


def collect(root):
    sys.path.insert(0,str(root/'Tests/asm'))
    from verification_heap import run as owners
    from test_expiry_runtime import ExpiryMachine,record,UC_ARM_REG_PRIMASK
    static=owners(root)
    # Inject actual linked IRQ handlers into a journal step with local check64.
    # Shared owner bytes, delayed W25 pointers and flash must equal no-IRQ run.
    src=ExpiryMachine();src.install([record()]);assert src.start_expiry()
    for _ in range(2000):
        if src.u.mem_read(src.addr('sjr')+54,1)==b'\3' and src.io is None:break
        src.expiry_tick()
    else:raise AssertionError('journal read phase not reached')
    ram=bytes(src.u.mem_read(0x20000000,0x5000));flash=bytes(src.flash)
    def irq_step(handler=None,at=None):
        m=ExpiryMachine(flash);m.u.mem_write(0x20000000,ram);m.io=src.io
        m.call('expiry_runtime_poll',src.now+1,inject_at=at,irq=(handler,()) if handler else None)
        pointer=int.from_bytes(m.u.mem_read(m.addr('expiry_job'),4),'little')
        owner=(bytes(m.u.mem_read(pointer,1092)),bytes(m.u.mem_read(m.addr('sjr'),128)),bytes(m.flash),m.io)
        return owner,m.steps,m.top-m.minimum,m.u.reg_read(UC_ARM_REG_PRIMASK)
    expected,instructions,_,_=irq_step();irq_max=0;irq_cases=0
    for name in ('SysTick_Handler','DMA1_Channel3_IRQHandler','USART3_IRQHandler','EXTI2_IRQHandler','EXTI3_IRQHandler'):
        for at in sorted(set((0,instructions//4,instructions//2,3*instructions//4,instructions-1))):
            actual,_,used,mask=irq_step(name,at)
            assert actual==expected and mask==0
            irq_max=max(irq_max,used);irq_cases+=1
    scratch={'single_irq_injections':irq_cases,'maximum_measured_stack_with_hardware_frame':irq_max,
             'owner_and_flash_equality':'PASS','irq_nesting_tested':False,
             'ownership_audit':['sj_runtime check64 is call-local; async W25 uses persistent sjr data/header.',
                 'Config page64 aliases sjr.data only under mutual owner guards; no IRQ calls these owners.',
                 'comm_uart DMA CMAR points to its dedicated static rx ring; camera RX lives in owned photo_s.',
                 'New begin/poll/cancel paths preserve PRIMASK; IRQ collection is not a global nested-stack proof.']}

    return {'static':static,'shared_scratch_irq':scratch,
            'scope':'Current linked workspace and single IRQ injection; global nested stack maximum is not proved.'}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument('--output',type=Path);a=p.parse_args();out=collect(a.root.resolve())
    if a.output:a.output.write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out,indent=2))
