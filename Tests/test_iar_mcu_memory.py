"""Аллокатор и выполненные пути стека IAR; без обращения к периферии и плате."""
import hashlib, json, os, re, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
deps=os.environ.get('DOMOVOI_TEST_DEPS') or str(root/'Release/photo_test_deps')
if not Path(deps).exists():
    deps=r'C:\Users\kryaz\Desktop\мк\СофтЭксперт\Для новой системы\Domovoi Next_IAR95\Domovoi Next_IAR95\Release\photo_test_deps'
sys.path.insert(0,deps)
from elftools.elf.elffile import ELFFile
from elftools.dwarf.dwarf_expr import DWARFExprParser
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_PC, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3
with (root/'Release/Exe/DomovoiNext.out').open('rb') as f:
    elf=ELFFile(f)
    sym={s.name:(s['st_value'],s['st_size']) for s in elf.get_section_by_name('.symtab').iter_symbols()}
    gsm_members={}
    for cu in elf.get_dwarf_info().iter_CUs():
        for die in cu.iter_DIEs():
            name=die.attributes.get('DW_AT_name')
            if name and name.value==b'GsmStr' and die.tag=='DW_TAG_typedef':
                for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                    if 'DW_AT_name' in member.attributes and 'DW_AT_data_member_location' in member.attributes:
                        offset=member.attributes['DW_AT_data_member_location'].value
                        if not isinstance(offset,int):
                            operations=DWARFExprParser(cu.structs).parse_expr(offset)
                            assert len(operations)==1 and operations[0].op_name=='DW_OP_plus_uconst'
                            offset=operations[0].args[0]
                        gsm_members[member.attributes['DW_AT_name'].value.decode()]=offset
    assert 'operation' in gsm_members and 'timer' in gsm_members
def addr(n):return sym[n][0]
map_path=root/'Release/List/DomovoiNext.map'
map_text=map_path.read_text(errors='replace')
def mapped_size(n):
    m=re.search(r'^(?:_A_)?'+re.escape(n)+r'\s+0x[0-9a-f\']+\s+(0x[0-9a-f]+)\s+Data',map_text,re.M|re.I)
    assert m,n
    return int(m[1],16)

def mapped_address(n):
    m=re.search(r'^'+re.escape(n)+r'\s+(0x[0-9a-f\']+)',map_text,re.M|re.I)
    assert m,n
    value=int(m[1].replace("'",''),16)
    assert value==addr(n)
    return value
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
u.mem_map(0x08000000,0x20000);u.mem_write(0x08008200,(root/'Release/Exe/DomovoiNext.bin').read_bytes())
u.mem_map(0x20000000,0x5000);u.mem_map(0x21000000,0x1000)
u.mem_map(0x40000000,0x30000);u.mem_map(0xE000E000,0x1000)
stop=0x0801fff0
top=mapped_address('CSTACK$$Limit');bottom=mapped_address('CSTACK$$Base')
heap=mapped_address('HEAP$$Limit')-mapped_address('HEAP$$Base')
assert heap==5120 and top-bottom==4096
assert addr('HEAP$$Limit')<=addr('fault_record')==0x20004fc0
for table in ('pEpInt_IN','pEpInt_OUT'):
    assert mapped_size(table)==28
    assert 0x08008200<=mapped_address(table)<0x08020000,('USB callback table consumes RAM',table)
    callbacks=struct.unpack('<7I',u.mem_read(mapped_address(table),28))
    expected=[addr('NOP_Process')]*7
    expected[0 if table=='pEpInt_IN' else 2]=addr('EP1_IN_Callback' if table=='pEpInt_IN' else 'EP3_OUT_Callback')
    assert callbacks==tuple(expected),(table,callbacks,expected)
u.reg_write(UC_ARM_REG_XPSR,0x01000000)
minimum=top;executed=0;readbacks=[];stubs={}
def ret(v=0):
    u.reg_write(UC_ARM_REG_R0,v);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
def memory_read():
    a,p,n=[u.reg_read(r) for r in (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2)]
    readbacks.append((a,n));u.mem_write(p,bytes(((a+i)&255) for i in range(n)));ret()
def hook(emu,address,size,data):
    global minimum,executed
    minimum=min(minimum,u.reg_read(UC_ARM_REG_SP));executed+=1
    assert minimum>=bottom,('stack overflow',hex(minimum))
    if address in stubs:stubs[address]()
u.hook_add(UC_HOOK_CODE,hook)
def call(n,*args,budget=3000000):
    global minimum,executed
    minimum=top;executed=0
    u.reg_write(UC_ARM_REG_SP,top)
    for r,v in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3),args):u.reg_write(r,v)
    u.reg_write(UC_ARM_REG_LR,stop|1);u.emu_start(addr(n)|1,stop,count=budget)
    assert u.reg_read(UC_ARM_REG_PC)==stop,(n,hex(u.reg_read(UC_ARM_REG_PC)))
    return u.reg_read(UC_ARM_REG_R0),top-minimum
def reset():call('__iar_data_init3')
def largest():
    lo,hi=0,heap
    while lo<hi:
        mid=(lo+hi+1)//2;p,_=call('__basic_malloc',mid)
        if p:call('__basic_free',p);lo=mid
        else:hi=mid-1
    return lo
reset();cold=largest()
uart=mapped_size('gsm_buffer');server=mapped_size('server_rx_buffer')
models=[]
for title,requests in [('UART copy plus pending URC',[uart+8,uart+8]),('pending URC plus old sector',[uart+8,4096]),('pending URC plus page scratch',[uart+8,256]),('fragmented sector',[1000,1000,1000,1000])]:
    reset();pointers=[]
    for n in requests:
        p,_=call('__basic_malloc',n);pointers.append(p)
    allocated=[bool(p) for p in pointers]
    if title=='fragmented sector':
        for i in (0,2):call('__basic_free',pointers[i]);pointers[i]=0
    model={'scenario':title,'requests':requests,'allocated':allocated,'live_after_free':[bool(p) for p in pointers],'largest_free_request':largest()}
    models.append(model)
assert models[0]['allocated']==[True,True]
assert models[1]['allocated']==[True,False]
assert models[2]['allocated']==[True,True]
assert models[3]['allocated']==[True]*4 and models[3]['live_after_free']==[False,True,False,True]
assert models[3]['largest_free_request']<4096
reset();stubs[addr('mem_read')&~1]=memory_read
for name in ('wdt_reload','user_debug','usb_send','connection_send'):
    if name in sym:stubs[addr(name)&~1]=lambda:ret(1)
stack=[]
for n in (0,1,257,1024,16000):
    readbacks.clear();value,used=call('user_calcCRC16InFlash',266240,n,budget=20000000)
    data=bytes(((266240+i)&255) for i in range(n));crc=0xffff
    for b in data:
        crc^=b
        for _ in range(8):crc=(crc>>1)^((crc&1)*0xa001)
    assert value==crc
    stack.append({'function':'user_calcCRC16InFlash','length':n,'executed_stack':used,'max_read':max((z for _,z in readbacks),default=0)})
value,used=call('gsm_dataread',25,0,0,0)
stack.append({'function':'gsm_dataread(PREINIT)','executed_stack':used})
stubs[addr('gsm_runcmd')&~1]=lambda:ret(1)
u.mem_write(addr('GSM')+gsm_members['operation'],bytes([2]))
u.mem_write(addr('GSM')+gsm_members['timer'],struct.pack('<H',5000))
value,used=call('gsm_pwrProc',0)
stack.append({'function':'gsm_pwrProc(PO_ON,5000)->gsm_dataread(PREINIT)','executed_stack':used})
recurse=map_text[map_text.index('The following functions make up recursion nest'):map_text.index('The following functions perform unknown indirect calls')]
totals={kind:int(re.search(r"([0-9']+) bytes of readwrite "+kind+r' memory',map_text)[1].replace("'",'')) for kind in ('code','data')}
result={'map_sha256':hashlib.sha256(map_path.read_bytes()).hexdigest(),'map_readwrite_bytes':totals,'heap_bytes':heap,'stack_bytes':top-bottom,'cold_largest_request':cold,'placement_gap_bytes':mapped_address('fault_record')-mapped_address('HEAP$$Limit'),'readonly_usb_tables':{n:{'address':hex(mapped_address(n)),'bytes':mapped_size(n)} for n in ('pEpInt_IN','pEpInt_OUT')},'mapped_symbols':{n:mapped_size(n) for n in ('gsm_buffer','server_rx_buffer','usb_buffer','USART_Rx_Buffer','log_pending','log_pendingAt','port_buffer','config','device','mem','tempStrBuffer','GSM','gsm_strbuf','uart','fault_record') if n in sym},'allocator_models':models,'executed_stack_paths':stack,'recursion_nest':recurse,'limitations':'Peripheral calls are stubbed. These paths and allocator models do not prove a worst-case stack bound or hardware timings.'}
(root/'Release/audit-memory.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
print(json.dumps({k:result[k] for k in ('heap_bytes','stack_bytes','cold_largest_request','placement_gap_bytes','allocator_models','executed_stack_paths')},ensure_ascii=False,indent=2))
