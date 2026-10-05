"""Collect actual linked resources and observed allocator lifetimes.

--root selects the final ELF tree. No build, source mutation, board or timing.
Observed traces are examples, not a proof of a global reachable heap/stack peak.
Allocator stress is reported separately from admitted production operations.
"""
import argparse,hashlib,json,re,sys
from pathlib import Path


def collect(root):
    sys.path.insert(0,str(root/'Tests'/'asm'))
    from storage_machine import StorageMachine,UC_HOOK_CODE,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_LR,UC_ARM_REG_PRIMASK
    from test_memory_runtime import free_blocks
    from test_service_runtime import ServiceMachine
    from test_upload_runtime import UploadMachine,log
    from test_expiry_runtime import ExpiryMachine,record

    def free_state(m):
        chunks=free_blocks(m);total=sum(chunks);largest=max(chunks,default=0)
        return {'used_chunks':m.heap_bytes-total,'total_free_chunks':total,
                'largest_free_chunk':largest,'largest_free_payload':max(0,largest-8),
                'free_blocks':chunks,'external_fragmentation':1-largest/total if total else 0}

    class Trace:
        def __init__(self,m):
            self.m=m;self.pending=[];self.live={};self.allocations=[];self.freed=[]
            self.stack=0;self.max_step=0;self.total_steps=0;self.calls=0;self.active=True
            p=m.checked('__basic_malloc',1);assert p;m.checked('__basic_free',p)
            self.peak=free_state(m);self.peak_live=[];self.min_largest=self.peak['largest_free_payload']
            self.peak_ram=bytes(m.u.mem_read(0x20000000,0x5000))
            self.max_fragmentation=0
            self.malloc=m.addr('__basic_malloc');self.free=m.addr('__basic_free')
            self.calloc=m.addr('__basic_calloc') if '__basic_calloc' in m.symbols else None
            self.handle=m.u.hook_add(UC_HOOK_CODE,self.hook)
        def snapshot(self):
            state=free_state(self.m)
            self.min_largest=min(self.min_largest,state['largest_free_payload'])
            self.max_fragmentation=max(self.max_fragmentation,state['external_fragmentation'])
            if state['used_chunks']>self.peak['used_chunks']:
                self.peak=state;self.peak_live=[dict(v) for v in self.live.values()]
                self.peak_ram=bytes(self.m.u.mem_read(0x20000000,0x5000))
        def hook(self,u,at,size,data):
            if not self.active:return
            if self.pending and at==self.pending[-1][0]:
                _,kind,arg=self.pending.pop()
                if kind in ('malloc','calloc'):
                    ptr=u.reg_read(UC_ARM_REG_R0)
                    if ptr:
                        chunk=int.from_bytes(u.mem_read(ptr-8,4),'little')
                        item={'request':arg,'chunk':chunk,'allocator':kind};self.live[ptr]=item;self.allocations.append(item)
                else:self.live.pop(arg,None);self.freed.append(arg)
                self.snapshot()
            if at in (self.malloc,self.free,self.calloc):
                self.snapshot()
                kind='malloc' if at==self.malloc else ('calloc' if at==self.calloc else 'free')
                arg=u.reg_read(UC_ARM_REG_R0)
                if kind=='calloc':arg*=u.reg_read(UC_ARM_REG_R1)
                self.pending.append((u.reg_read(UC_ARM_REG_LR)&~1,
                                     kind,arg))
        def step(self,fn,*args,**kw):
            before=self.m.u.reg_read(UC_ARM_REG_PRIMASK)
            result=fn(*args,**kw)
            assert self.m.u.reg_read(UC_ARM_REG_PRIMASK)==before
            self.calls+=1;self.stack=max(self.stack,self.m.stack_peak)
            self.max_step=max(self.max_step,self.m.steps);self.total_steps+=self.m.steps
            self.snapshot();return result
        def result(self):
            self.snapshot();self.active=False
            probe=StorageMachine();probe.u.mem_write(0x20000000,self.peak_ram)
            largest=self.peak['largest_free_payload']
            if largest:
                ptr=probe.checked('__basic_malloc',largest);assert ptr
                probe.checked('__basic_free',ptr)
            assert not probe.checked('__basic_malloc',largest+1)
            return {'allocation_events':self.allocations,'peak_observed':self.peak,
                    'peak_observed_live_allocations':self.peak_live,
                    'minimum_largest_free_payload':self.min_largest,
                    'maximum_observed_fragmentation':self.max_fragmentation,
                    'final':free_state(self.m),'measured_stack':self.stack,
                    'model_instructions_max_call':self.max_step,
                    'model_instructions_total':self.total_steps,'calls':self.calls,
                    'peak_largest_payload_confirmed_by_actual_allocator':True,
                    'scope':'Real admitted operation with modeled NOR/transport boundary; no synthetic reservations.'}

    base=StorageMachine();maptext=base.maptext
    counts={}
    for name,label in [('readonly_code','readonly  code'),('ram_code','readwrite code'),
                       ('readonly_data','readonly  data'),('rw_data','readwrite data')]:
        found=re.search(r"([\d']+) bytes of "+re.escape(label)+r' memory',maptext)
        if found:counts[name]=int(found[1].replace("'",''))
    irq=[]
    for block in re.split(r'\n(?=interrupt\n)',maptext)[1:]:
        found=re.match(r'interrupt\s+"([^"]+)".*?Maximum call chain\s+(\*\?\*\s+)?([\d\']+)\s+bytes',block,re.S)
        if found:
            size=int(found[3].replace("'",''))
            irq.append({'name':found[1],'iar_cfi_software_stack':size,
                        'indirect_incomplete':bool(found[2]),'plus_one_exception_frame':size+32,
                        'optional_alignment_padding':4})
    entry=re.search(r'Program entry\n.*?Maximum call chain\s+(\*\?\*\s+)?([\d\']+)\s+bytes',maptext,re.S)
    static={'elf_sha256':hashlib.sha256((root/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
            'binary_bytes':(root/'Release/Exe/DomovoiNext.bin').stat().st_size,
            'map_memory':counts,'heap_bytes':base.heap_bytes,'cstack_bytes':base.top-base.bottom,
            'static_gap_before_fault_record':base.addr('fault_record')-base.addr('HEAP$$Limit'),
            'heap_bounds':[base.addr('HEAP$$Base'),base.addr('HEAP$$Limit')],
            'cstack_bounds':[base.bottom,base.top],
            'iar_program_entry_report':None if not entry else {'bytes':int(entry[2].replace("'",'')),
                'indirect_incomplete':bool(entry[1]),'not_a_proven_reachable_maximum':True},
            'interrupts':irq,
            'stack_limitations':['IAR CFI still includes preserved unreachable historical bodies, e.g. user_checkIdTimeEnd.',
                'Indirect calls and IRQ nesting prevent treating this map as a complete global bound.',
                'Exception hardware frame is 32 bytes per level plus possible four-byte alignment.']}
    traces={}

    m=ServiceMachine();t=Trace(m);t.step(m.packet,17)
    for _ in range(10000):
        if not m.active():break
        t.step(m.tick)
    assert not m.active() and m.sent==[m.expected(17)]
    traces['service_command17']=t.result()

    m=UploadMachine();t=Trace(m);m.fixture(10,[log(1),log(2)]);m.write(m.addr('log_unacked'),1,2)
    t.step(m.request)
    for _ in range(12000):
        if m.sent and not m.ptr():break
        t.step(m.tick_upload)
    assert m.sent and not m.ptr()
    traces['log_upload_ack_then_response']=t.result()

    # Actual photo state is created by the camera parser/port, not malloc stress.
    from test_photo_runtime import PhotoMachine
    m=PhotoMachine();t=Trace(m);t.step(m.boot_media);t.step(m.begin_camera,58)
    assert m.photo_ptr() and not m.active()
    if 'usb_try_send' in m.symbols:m.stubs[m.addr('usb_try_send')]=m.usb_send
    m.write(m.addr('cfg_legacy_checked'),1,1)
    m.write(m.addr('mem')+6*8,0x1cd400);m.write(m.addr('mem')+6*8+4,4096)
    UploadMachine.fixture(m,10,[log(1),log(2)]);m.write(m.addr('log_unacked'),1,2)
    t.step(UploadMachine.request,m)
    for _ in range(12000):
        p=int.from_bytes(m.u.mem_read(m.addr('upload_runtime_context'),4),'little')
        if m.sent and not p:break
        m.now+=1;t.step(m.checked,'user_storage_poll')
    assert m.sent and not int.from_bytes(m.u.mem_read(m.addr('upload_runtime_context'),4),'little')
    traces['actual_camera_waiting_log_ack_then_response']=t.result()

    m=ExpiryMachine();t=Trace(m);m.install([record()]);assert t.step(m.start_expiry)
    for _ in range(10000):
        if not m.expiry_active():break
        t.step(m.expiry_tick)
    assert not m.expiry_active() and m.expiry_events
    traces['expiry_verified_journal']=t.result()

    if 'idclear_runtime_begin' in base.symbols:
        from test_idclear_runtime import ClearMachine
        m=ClearMachine();t=Trace(m);t.step(m.parse)
        for _ in range(10000):
            if not m.clear_active():break
            t.step(m.clear_tick)
        assert not m.clear_active() and m.counters()==(0,0,0)
        traces['idclear_parser_two_sectors']=t.result()

    # Controlled fragmentation is explicitly allocator stress, not reachability.
    m=StorageMachine();t=Trace(m)
    held=[t.step(m.checked,'__basic_malloc',n) for n in (128,512,128,768,128)]
    tail=t.step(m.checked,'__basic_malloc',free_state(m)['largest_free_payload']);assert tail
    for i in (1,3):t.step(m.checked,'__basic_free',held[i])
    fragmented=free_state(m)
    assert not t.step(m.checked,'__basic_malloc',fragmented['largest_free_payload']+1)
    for i in (0,2,4):t.step(m.checked,'__basic_free',held[i])
    t.step(m.checked,'__basic_free',tail)
    stress=t.result();stress['fragmented_holes']=fragmented
    stress['scope']='Synthetic allocator fragmentation; not a concurrent production lifetime.'

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
    for name in ('SysTick_Handler','DMA1_Channel3_IRQHandler','USART3_IRQHandler'):
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

    # These live access helpers remain synchronous. No relay path is executed.
    residual={}
    m=ExpiryMachine();m.install([record()]);m.checked('idindex_runtime_suspend')
    replacement=bytearray(record());replacement[21]=17;m.u.mem_write(0x21000000,bytes(replacement))
    synchronous={'erase':0,'program':0}
    erase=m.sync_erase;program=m.sync_write
    def er():synchronous['erase']+=1;erase()
    def wr():synchronous['program']+=1;program()
    m.stubs[m.addr('w25_erase_sector')]=er;m.stubs[m.addr('w25_write_data')]=wr
    assert m.checked('user_updateID',0x21000000,budget=3000000)
    residual['access_record_update']={'model_instructions_one_call':m.steps,'stack':m.stack_peak,
        'flash_read_bytes_one_call':m.read_budget,'synchronous_nor_calls':synchronous,
        'real_caller':'user_activateOut -> user_updateID -> mem_patch -> sector journal'}
    m=ExpiryMachine();m.write(m.addr('mem')+20,1601536);m.write(m.addr('id_cells_end'),50048,2)
    m.checked('idindex_runtime_suspend');m.u.mem_write(0x21000000,b'0123456789')
    assert not m.checked('user_findID',0x21000000,0x21000100,budget=5000000)
    residual['access_missing_key_full_fallback']={'model_instructions_one_call':m.steps,'stack':m.stack_peak,
        'flash_read_bytes_one_call':m.read_budget,'partition_bytes':1601536,
        'real_caller':'user_checkAccessID/user_activateOut -> user_findID -> 1024-byte fallback loop'}
    residual['factory_source_audit']={'executed':False,
        'real_callers':['USB/server cmd3','user_loop delayed factory reset','registration unknown/not-bound','SMS default'],
        'chain':['user_writeConfigAndDevice synchronous config journal','user_clearMemBlock on 2,3,4,5,7,8,9',
                 'user_setdefault persists config','user_initvariables rescans ID/log/temp/photo and comm boot'],
        'limitations':'Remains synchronous; response/SMS lifetime and recovery need a separate design. No factory command was executed.'}
    return {'static':static,'operation_traces':traces,'allocator_stress':stress,'shared_scratch_irq':scratch,'residual_runtime':residual,
            'largest_observed_admitted_heap':max(v['peak_observed']['used_chunks'] for v in traces.values()),
            'largest_observed_admitted_stack':max(v['measured_stack'] for v in traces.values()),
            'scope':'Final linked code, modeled device boundaries. No global maximum, milliseconds, or hardware guarantee.'}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[2])
    p.add_argument('--output',type=Path);a=p.parse_args();out=collect(a.root.resolve())
    if a.output:a.output.write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out,indent=2))
