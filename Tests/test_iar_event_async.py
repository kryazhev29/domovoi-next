"""Доработка: Кряжев А., 2026.
Запись события на ELF: моделируемый NOR; измерения относятся к эмулятору.
Источники данных и отказы задаются моделью; результат проверяется утверждениями.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from test_asm_crc import Machine, ARGS, STACK, BUF, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR
from iar_layout import fields

def measure(path,bounded,occupied):
    m=Machine(path);offsets=fields('device_s')
    flash=bytearray(bytes(occupied*32)+b'\xff'*(98304-occupied*32))
    m.uc.mem_write(m.sym['mem']+7*8,struct.pack('<II',4096,98304))
    m.uc.mem_write(m.sym['device']+offsets['newAddressLog'],struct.pack('<I',4096))
    m.uc.mem_write(m.sym['log_pendingCount'],b'\x01')
    m.uc.mem_write(m.sym['log_pending'],struct.pack('<II',100,100)+bytes(24))
    if bounded:m.uc.mem_write(m.sym['store_runtime'],b'\x01')
    steps=reads=starts=polls=0;low=STACK;job=None
    hooknames=('mem_read','mem_write','wdt_reload','user_storage_flash_ready','w25_async_start','w25_async_poll','w25_read_available','w25_async_active')
    hooks={m.sym[n]&~1:n for n in hooknames if n in m.sym}
    def hook(u,a,n,_):
        nonlocal steps,reads,starts,polls,low,job
        steps+=1;low=min(low,u.reg_read(UC_ARM_REG_SP))
        name=hooks.get(a)
        if not name:return
        x,p,count=[u.reg_read(r) for r in ARGS[:3]];ret=1
        if name=='w25_async_active':ret=int(job is not None)
        elif name=='mem_read':
            reads+=1;assert 4096<=x<=x+count<=102400
            u.mem_write(p,bytes(flash[x-4096:x-4096+count]))
        elif name=='mem_write':
            starts+=1
            for i,b in enumerate(u.mem_read(p,count)):flash[x-4096+i]&=b
        elif name=='w25_async_start':
            starts+=1;assert job is None
            job=[x,p,count,3,bytes(u.mem_read(p,count))]
        elif name=='w25_async_poll':
            polls+=1;assert job is not None
            x,p,count,busy,data=job;assert bytes(u.mem_read(p,count))==data
            if busy:job[3]-=1
            else:
                for i,b in enumerate(data):flash[x-4096+i]&=b
                job=None;ret=2
        u.reg_write(ARGS[0],ret);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE,hook)
    results=[]
    for i in range(10000):
        steps=reads=starts=polls=0
        m.call('log_storage_poll' if bounded else 'log_flushPending',i,check_abi=True)
        if bounded:assert reads<=1 and starts<=1 and polls<=1,(reads,starts,polls)
        results.append(steps)
        if not m.uc.mem_read(m.sym['log_pendingCount'],1)[0]:break
    else:raise AssertionError('event never finished')
    assert int.from_bytes(m.uc.mem_read(m.sym['device']+offsets['newAddressLog'],4),'little')==4096+(occupied+1)*32
    return {'max_instructions':max(results),'total_instructions':sum(results),'passes':len(results),
            'stack_bytes':STACK-low,'record':flash[occupied*32:(occupied+1)*32].hex()}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--firmware',type=Path,required=True);p.add_argument('--json',type=Path,required=True)
    a=p.parse_args();out={'units':'Unicorn instructions, modeled memory calls; not hardware time','cases':{}}
    for count in (0,3071):
        old,new=measure(a.baseline,False,count),measure(a.firmware,True,count)
        assert old['record']==new['record'];assert new['max_instructions']<6000
        out['cases'][str(count)]={'before':old,'after':new}
        print('occupied',count,old['max_instructions'],'->',new['max_instructions'],'passes',new['passes'])
    out['sha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (a.baseline,a.firmware)}
    a.json.write_text(json.dumps(out,indent=2)+'\n');print('PASS event caller ELF: equivalent record, bounded work, ABI/stack')
