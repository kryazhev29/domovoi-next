"""Actual FW caller + final image CRC in IAR ELF; modeled NOR, real allocator."""
import argparse, hashlib, json, struct, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from test_asm_crc import Machine, ARGS, STACK, BUF, UC_HOOK_CODE
import test_asm_crc as arm_machine
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR
from iar_layout import fields

def reference(data):
    c=0xffffffff
    for off in range(0,len(data),4):
        c^=int.from_bytes(data[off:off+4],'little')
        for _ in range(32):c=((c<<1)^(0x04c11db7 if c&0x80000000 else 0))&0xffffffff
    return c

def measure(path,bounded,block,blocks):
    m=Machine(path);top=m.sym['CSTACK$$Limit'];arm_machine.STACK=top;m.call('__iar_data_init3')
    d=fields('device_s');h=fields('bootFWparam_s')
    flash=bytearray(b'\xff'*131072);size=blocks*1024
    image=bytes((i*37+11)&255 for i in range(size));flash[512:512+size]=image
    flash[h['firmwareSize']:h['firmwareSize']+4]=struct.pack('<I',size)
    flash[h['firmwareCRC']:h['firmwareCRC']+4]=struct.pack('<I',reference(image))
    address=512+block*1024;flash[address:address+1024]=b'\xff'*1024
    m.uc.mem_write(m.sym['mem'],struct.pack('<II',0,len(flash)))
    for key,value,n in [('FWblockIdx',block,2),('FWblockCount',blocks,2),('cchFWupdate',1,1),('FWinProgress',1,1)]:
        m.uc.mem_write(m.sym['device']+d[key],value.to_bytes(n,'little'))
    if bounded:m.uc.mem_write(m.sym['store_runtime'],b'\x01')
    m.uc.mem_write(BUF,block.to_bytes(2,'little')+image[block*1024:(block+1)*1024])
    steps=reads=starts=polls=acks=0;low=top;job=None
    names=('mem_read','mem_write','mem_erase_sector','wdt_reload','boot_manual_busy','w25_async_start','w25_async_poll',
           'w25_read_available','w25_async_active','user_send_statusUpdateFW','user_saveindex','comm_tx_busy')
    hooks={m.sym[n]&~1:n for n in names if n in m.sym}
    def hook(u,a,n,_):
        nonlocal steps,reads,starts,polls,acks,low,job
        steps+=1;low=min(low,u.reg_read(UC_ARM_REG_SP));name=hooks.get(a)
        if not name:return
        x,p,count=[u.reg_read(r) for r in ARGS[:3]];ret=1
        if name=='w25_async_active':ret=int(job is not None)
        elif name in ('boot_manual_busy','comm_tx_busy'):ret=0
        elif name=='mem_read':
            reads+=1;assert x+count<=len(flash)
            u.mem_write(p,bytes(flash[x:x+count]))
        elif name=='mem_write':
            starts+=1
            for i,b in enumerate(u.mem_read(p,count)):flash[x+i]&=b
        elif name=='mem_erase_sector':starts+=1;flash[x:x+4096]=b'\xff'*4096
        elif name=='w25_async_start':
            starts+=1;assert job is None
            job=[x,p,count,3,bytes(u.mem_read(p,count)) if p else None]
        elif name=='w25_async_poll':
            polls+=1;assert job is not None
            x,p,count,busy,data=job
            if p:assert bytes(u.mem_read(p,count))==data
            if busy:job[3]-=1
            else:
                if p:
                    for i,b in enumerate(data):flash[x+i]&=b
                else:flash[x:x+4096]=b'\xff'*4096
                job=None;ret=2
        elif name=='user_send_statusUpdateFW':acks+=1;assert p==0
        u.reg_write(ARGS[0],ret);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE,hook)
    result=m.call('fw_take_block',BUF,1026,1,check_abi=True)
    assert result==(252 if bounded else 0),result
    costs=[steps]
    if bounded:
        for t in range(10000):
            steps=reads=starts=polls=0
            m.call('fw_storage_poll',t,check_abi=True)
            assert reads<=1 and starts<=1 and polls<=1
            costs.append(steps)
            if acks:break
        else:raise AssertionError('FW never replied')
    assert bytes(flash[512:512+(block+1)*1024])==image[:(block+1)*1024]
    assert int.from_bytes(m.uc.mem_read(m.sym['device']+d['FWblockIdx'],2),'little')==block+1
    if block+1==blocks:assert m.uc.mem_read(m.sym['device']+d['rebootToUpdate'],1)[0]
    return {'max_instructions':max(costs),'total_instructions':sum(costs),'passes':len(costs),
            'begin_instructions':costs[0],'max_poll_instructions':max(costs[1:],default=0),'stack_bytes':top-low}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--firmware',type=Path,required=True);p.add_argument('--json',type=Path,required=True)
    a=p.parse_args();out={'units':'Unicorn instructions, modeled NOR; not hardware time','cases':{}}
    for block,blocks in ((0,2),(3,5),(95,96)):
        old,new=measure(a.baseline,False,block,blocks),measure(a.firmware,True,block,blocks)
        out['cases'][str(block)+'/'+str(blocks)]={'before':old,'after':new}
        assert new['max_poll_instructions']<4000
        print(block,blocks,old['max_instructions'],'->',new['max_instructions'],'poll',new['max_poll_instructions'])
    out['sha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (a.baseline,a.firmware)}
    a.json.write_text(json.dumps(out,indent=2)+'\n');print('PASS FW ELF: exact image, CRC, cursor, bounded work, ABI/stack, real heap')
