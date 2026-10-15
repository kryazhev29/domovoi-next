# Доработка: Кряжев А., 2026
"""Владение ОЗУ, реальная выдача участков и чтение выбранных настроек в связанном IAR-коде."""
import hashlib,json,struct,zlib
from storage_machine import *
from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES

LEGACY=0x1cd400;SLOT0=0x1f5000;SLOT1=0x1f7000

# Читает свободные блоки модельной кучи.
def free_blocks(m):
    return [2032] if not owned(m)&~1 else []

# Читает состояние модельной кучи для контроля утечек.
def heap_stats(m):
    return {'storage_bytes':fields('work_memory_s')['owned']+8, 'owned_slots':owned(m)}

# Проверяет выдачу участков и сохранность соседних владельцев.
def allocator():
    from test_work_memory import run
    return run()

# Готовит два различающихся снимка настроек для проверки A/B.
def config_machine():
    m=StorageMachine();old=bytes((i*7+13)&255 for i in range(4096))
    generation=struct.pack('<I',17)
    crc=zlib.crc32(generation+old)^0xffffffff
    m.flash[SLOT0:SLOT0+4096]=old
    m.flash[SLOT0+4096:SLOT0+4112]=struct.pack('<IIII',0x32474643,17,crc,crc^0xffffffff)
    m.flash[LEGACY:LEGACY+4096]=old
    assert m.select_config()==SLOT0
    assert m.checked('cfg_boot_select',budget=1000000)
    data=bytes((i*11+91)&255 for i in range(1024))
    m.u.mem_write(0x21000000,data)
    return m,old,data

# Доводит публикацию снимка A/B до результата.
def run_config(m):
    costs=[];reads=[]
    for _ in range(2000):
        if m.checked('mem_config_result')!=1:break
        m.now+=1;m.checked('mem_config_poll',m.now)
        costs.append(m.steps);reads.append(m.read_budget)
        assert m.io_budget<=1 and m.read_budget<=192
        # Общий участок нельзя выдать прямому входу таблицы или камеры.
        assert not m.checked('sj_runtime_begin',0x41000,0x08010001,0)
        assert not m.checked('flash_exec_idle')
    else:raise AssertionError('config did not terminate')
    return {'passes':len(costs),'maximum_instructions':max(costs),'total_instructions':sum(costs),
            'maximum_read_bytes':max(reads),'stack':m.stack_peak}

# Проверяет время жизни снимка при конкурирующих владельцах.
def config_ownership():
    m,old,new=config_machine();m.record_changes=True
    assert m.checked('mem_config_begin',0x21000000,0x21000200)
    result=run_config(m);assert m.checked('mem_config_result')==2
    assert m.flash[SLOT1:SLOT1+4096]==new+old[1024:]
    assert m.io is None and m.checked('mem_config_snapshot')
    # Снимок остаётся занят до явного сброса после конечного результата.
    assert not m.checked('sj_runtime_begin',0x41000,0x08010001,0)
    m.checked('mem_config_reset');assert not m.checked('mem_config_snapshot')
    assert owned(m)==0
    from test_table_runtime import rows
    m.command(13,rows([1]),1);assert m.active()
    page=bytes(m.u.mem_read(m.addr('sjr')+61,64));before=bytes(m.flash)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    assert page==bytes(m.u.mem_read(m.addr('sjr')+61,64)) and before==bytes(m.flash)
    m.drain();assert m.timestamps and not m.io
    m.u.mem_write(0x21000000,new)
    assert m.checked('mem_config_begin',0x21000000,0x21000200)
    run_config(m);assert m.checked('mem_config_result')==2
    m.checked('mem_config_reset')
    m.write(m.addr('sjr')+59,1,1)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    # Камера также удерживает страницу на всех шагах хранения.
    from test_photo_runtime import PhotoMachine
    p=PhotoMachine();p.boot_media();p.begin_camera(58,drain=False)
    assert p.exec_active(8)
    assert not p.checked('mem_config_begin',0x21000000,0x21000200)
    return result

# Проверяет сохранение прежнего слота при отказах публикации.
def config_faults():
    cases=0
    for kind,points in [('read',(1,64,150,300)),('start',(1,2,10,65)),('poll',(1,10,128))]:
        for point in points:
            m,old,new=config_machine();m.counts={'read':0,'start':0,'poll':0};m.fault=(kind,point)
            assert m.checked('mem_config_begin',0x21000000,0x21000200)
            run_config(m)
            if m.counts[kind]>=point:
                assert m.checked('mem_config_result')==3 and m.io is None
                cases+=1
            m.checked('mem_config_reset');assert owned(m)==0
    m,_,_=config_machine();p=take(m,REPLY);assert p
    before=bytes(m.flash);counts=dict(m.counts)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    assert bytes(m.flash)==before and m.counts==counts
    return cases+1

# Проверяет чтение выбранного слота без повторной стартовой валидации.
def cached_reads():
    out=[]
    for active in (LEGACY,SLOT0,SLOT1):
        for result in (0,1,2,3):
            m=StorageMachine();m.write(m.addr('cfg_cached_active'),active)
            m.write(m.addr('cfg_job')+31,result,1)
            expected=bytes((i+result)&255 for i in range(64));m.flash[active+128:active+192]=expected
            # В работе нельзя повторно проверять весь слот или запускать перенос старого формата.
            def forbidden():raise AssertionError('runtime config rescan')
            m.stubs[m.addr('cfg_select')]=forbidden;m.stubs[m.addr('cfg_boot_select')]=forbidden
            assert m.checked('mem_read',LEGACY+128,0x21000000,64)
            assert bytes(m.u.mem_read(0x21000000,64))==expected and m.read_budget==64
            out.append({'active':active,'result':result,'instructions':m.steps,'stack':m.stack_peak})
            m.fault=('read',2);assert not m.checked('mem_read',LEGACY+128,0x21000000,64)
    return out

# Перезапускает каждый различающийся устойчивый образ после обрыва.
def power_cuts():
    m,old,new=config_machine();initial=bytes(m.flash);m.record_changes=True
    assert m.checked('mem_config_begin',0x21000000,0x21000200);run_config(m)
    state=bytearray(initial);count=0
    for kind,a,data in m.changes:
        for n in sorted(set((0,1,len(data)//2,len(data)-1,len(data)))):
            cut=bytearray(state)
            for i,v in enumerate(data[:n]):cut[a+i]=v if kind=='erase' else cut[a+i]&v
            r=StorageMachine(cut);chosen=r.select_config()
            assert bytes(r.flash[chosen:chosen+4096]) in (old,new+old[1024:])
            count+=1
        if kind=='erase':state[a:a+4096]=b'\xff'*4096
        else:
            for i,v in enumerate(data):state[a+i]&=v
    return count

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--power',action='store_true');args=parser.parse_args()
    out={'allocator':allocator(),'cached_reads':cached_reads()}
    print('allocator/cached bounded reads PASS',flush=True)
    out['config']=config_ownership();out['faults']=config_faults()
    print('shared scratch/ownership/real heap/I/O/OOM PASS',flush=True)
    if args.power:out['config_power_cuts']=power_cuts();print('config cuts',out['config_power_cuts'],'PASS',flush=True)
    out['elf_sha256']=hashlib.sha256((__import__('iar_image').image_paths()[0]).read_bytes()).hexdigest()
    m=StorageMachine()
    out['heap_bytes']=m.heap_bytes;out['stack_bytes']=m.top-m.bottom
    assert m.map_size('sjr')==128 and m.map_size('cfg_job')==36
    (__import__('iar_image').result_path('memory_runtime_results.json')).write_text(json.dumps(out,indent=2)+'\n')
