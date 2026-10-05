"""Same semantic faults applied to actual Thumb code in both linked images.

Patch setup is outside the expected assertion. Production files never change.
Original ASM-specific opcode mutation sets are also retained and run for ASM.
"""
import argparse
import contextlib
import importlib
import io
import json
from pathlib import Path
import runpy
import sys
import tempfile
from c_vs_asm_suite import ROOT, select_image
from iar_irq_support import Machine
from iar_image import image_paths
from unicorn import UcError
sys.path.insert(0,str(Path(tempfile.gettempdir())/'domovoi-c-asm-deps'))
from capstone import Cs,CS_ARCH_ARM,CS_MODE_THUMB,CS_MODE_MCLASS


def patch(m,symbol,mode):
    a=m.addr(symbol);code=bytes(m.u.mem_read(a,2048));offset=0
    if mode in ('return-zero','return-one','no-op'):
        replacement=b'\x01\x20\x70\x47' if mode=='return-one' else b'\x00\x20\x70\x47' if mode=='return-zero' else b'\x70\x47'
    else:
        target=m.addr('w25_async_poll');seen=False;offset=None
        md=Cs(CS_ARCH_ARM,CS_MODE_THUMB|CS_MODE_MCLASS)
        want=1 if mode=='pending' else 2
        for ins in md.disasm(code,a):
            if ins.mnemonic=='bl' and int(ins.op_str.lstrip('#'),0)==target:seen=True
            if seen and ins.mnemonic=='cmp' and ins.size==2 and ins.op_str.endswith(', #'+str(want)):
                word=int.from_bytes(ins.bytes,'little')
                assert word&0xf800==0x2800
                offset=ins.address-a;replacement=((word&0xff00)|(0 if want==1 else 3)).to_bytes(2,'little');break
        assert offset is not None,(symbol,mode,'semantic patch not located')
    original=code[offset:offset+len(replacement)];assert original!=replacement
    m.u.mem_write(a+offset,replacement)
    m.u.ctl_remove_cache(a+offset,a+offset+len(replacement))
    assert bytes(m.u.mem_read(a+offset,len(replacement)))==replacement
    return {'symbol':symbol,'offset':offset,'original':original.hex(),'replacement':replacement.hex()}


def cases():
    from test_service_runtime import ServiceMachine
    from test_idindex_runtime import IndexMachine
    from storage_machine import StorageMachine
    from test_table_runtime import rows
    from test_photo_runtime import PhotoMachine
    from test_expiry_runtime import ExpiryMachine,record
    from test_idclear_runtime import ClearMachine
    from verification_usb_try import UsbTryMachine
    result=[]
    def add(label,m,symbol,mode,check):result.append((label,m,symbol,mode,check))
    def kernel(name,args,expected,data=b'abcdefghij'):
        m=Machine();m.init();m.u.mem_write(0x21000000,data)
        def check():assert m.call(name,*args)==expected
        check();add(name,m,name,'return-zero',check)
    m=Machine();m.init();m.u.mem_write(0x21000000,b'abcdefghij')
    for name,args in (('fast_fnv10',(0x21000000,)),('idindex_hash',(7,0x21000000)),
                      ('crc16_calc',(0x21000000,10)),('crc16_calcInit',(0x21000000,10,65535)),
                      ('CRC32',(0xffffffff,0x12345678)),('fast_sldn',(0x21000000,10))):
        expected=m.call(name,*args);assert expected;kernel(name,args,expected)
    kernel('fast_gf2_apply',(0x21000000,1),1,b'\x01\0'*16)
    m=Machine();m.init();m.u.mem_write(0x21000000,b'\x00')
    def blank(m=m):assert not m.call('fast_all_ff',0x21000000,1)
    add('blank-dirty',m,'fast_all_ff','return-one',blank)

    for mode in ('pending','verified'):
        m=IndexMachine();expected,_,_=m.setup((1,2))
        def index(m=m,expected=expected):
            m.drain_index();assert m.state()==3 and m.flash[0x300000:0x340000]==expected
        add('index-'+mode,m,'idindex_poll',mode,index)
        m=StorageMachine();m.command(13,rows([1]),1)
        def table(m=m):m.drain();assert len(m.timestamps)==1
        add('journal-'+mode,m,'sj_runtime_poll',mode,table)
        m=PhotoMachine();m.boot_media()
        def photo(m=m):m.capture(b'x'*116)
        add('photo-'+mode,m,'photo_storage_poll',mode,photo)

    m=StorageMachine();m.command(13,rows([1,129]),2)
    def cancel(m=m):
        m.checked('table_runtime_cancel');m.drain();assert not m.timestamps
    add('table-cancel',m,'table_runtime_cancel','no-op',cancel)
    m=ServiceMachine();m.packet(17)
    def quiet(m=m):
        m.checked('service_crc_quiet',0);m.drain();assert not m.sent
    add('service-session-cancel',m,'service_crc_quiet','no-op',quiet)
    m=ServiceMachine();m.packet(17,channel=1);m.commit_fail=True
    def retry(m=m):
        for _ in range(200):
            if not m.active():break
            m.tick()
        assert m.active(),'lost owned reply on rejected commit'
    add('service-commit-retry',m,'reply_send','return-one',retry)
    m=ExpiryMachine();m.install([record()]);m.start_expiry()
    def expiry(m=m):
        m.checked('expiry_runtime_cancel');m.expiry_drain();assert not m.expiry_events
    add('expiry-cancel',m,'expiry_runtime_cancel','no-op',expiry)
    m=ClearMachine();m.start_clear()
    def clear(m=m):
        m.checked('idclear_runtime_cancel');m.clear_drain();assert not m.wires
    add('clear-cancel',m,'idclear_runtime_cancel','no-op',clear)
    m=UsbTryMachine();m.expected_stack=None;m.instruction_limit=None
    def queue(m=m):assert m.run(inp=0,out=1,length=1)[0]==0
    add('usb-full-queue',m,'usb_try_send','return-one',queue)
    return result


def run(variant):
    result={}
    for label,m,symbol,mode,check in cases():
        detail=patch(m,symbol,mode)  # A setup error is NOT a detected mutant.
        try:check()
        except (AssertionError,UcError) as exc:
            detail.update(status='DETECTED',assertion=str(exc));result[label]=detail
        else:raise AssertionError((label,'survived'))
        print('DETECTED',label,flush=True)
    legacy={}
    if variant=='ASM':
        with contextlib.redirect_stdout(io.StringIO()):
            for name in ('test_table_mutations','test_photo_asm_mutations','test_runtime_port_mutations'):
                runpy.run_path(str(ROOT/'Tests/asm'/f'{name}.py'));legacy[name]='PASS'
            for name in ('test_idindex_asm_mutations','verification_service_mutations'):
                legacy[name]=importlib.import_module(name).run()
            legacy['usb_try']=importlib.import_module('verification_usb_try').mutations()
    return {'paired':result,'legacy_asm_opcode_sets':legacy,
            'legacy_limit':'Original opcode signatures target ASM only; paired mutations above apply to both variants.'}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--variant',choices=('ASM','C'),required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();select_image(a.variant);result=run(a.variant)
    result.update(variant=a.variant,elf_sha256=__import__('hashlib').sha256(image_paths()[0].read_bytes()).hexdigest())
    a.output.write_text(json.dumps(result,indent=2,default=str)+'\n',encoding='utf-8');print('PASS mutations',flush=True)
