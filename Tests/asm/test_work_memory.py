# Доработка: Кряжев А., 2026
"""Совместимость владельцев, границы участков и сохранность байтов в связанном коде."""
from storage_machine import StorageMachine

PHOTO, PHOTO_PACKET, EXPIRY, FW_CONTEXT, FW_DATA, CONFIG, CLEAR, SERVICE, UP_CONTEXT, UP_PACKET, REPLY, SMS, FW_BLOCK, TABLE = range(1,15)
SIZES = {PHOTO:180, PHOTO_PACKET:534, EXPIRY:132, FW_CONTEXT:128, FW_DATA:1024,
         CONFIG:1024, CLEAR:40, SERVICE:248, UP_CONTEXT:32, UP_PACKET:1040,
         REPLY:2032, SMS:2028, FW_BLOCK:1024, TABLE:2032}


# Занимает участок реальным владельцем рабочей памяти.
def take(m, slot, size=None):
    return m.checked('work_acquire', SIZES[slot] if size is None else size, slot)


# Возвращает участок реальным владельцем рабочей памяти.
def drop(m, slot, pointer):
    m.checked('work_release', pointer, slot)


# Читает маску занятых участков рабочей памяти.
def owned(m):
    return int.from_bytes(m.u.mem_read(m.addr('work_memory')+2216,2),'little')


# Задаёт независимый эталон совместимости двух владельцев памяти.
def compatible(a,b):
    if a==b:return False
    if PHOTO in (a,b):return True
    if PHOTO_PACKET in (a,b):return a not in (REPLY,SMS,TABLE) and b not in (REPLY,SMS,TABLE)
    return frozenset((a,b)) in (frozenset((FW_CONTEXT,FW_DATA)),
        frozenset((FW_CONTEXT,CONFIG)),frozenset((UP_CONTEXT,UP_PACKET)))


# Проверяет основные сценарии модуля и возвращает результаты.
def run():
    pairs=allowed=0
    for a in SIZES:
        for b in SIZES:
            m=StorageMachine();p=take(m,a);assert p and p%8==0
            canary=bytes([0x30+a])*SIZES[a];m.u.mem_write(p,canary)
            q=take(m,b);assert bool(q)==compatible(a,b),(a,b,q)
            assert bytes(m.u.mem_read(p,SIZES[a]))==canary,(a,b,'overwrite')
            if q:
                assert p+SIZES[a]<=q or q+SIZES[b]<=p,(a,b,'overlap')
                data=bytes([0x70+b])*SIZES[b];m.u.mem_write(q,data)
                drop(m,a,p);assert bytes(m.u.mem_read(q,SIZES[b]))==data
                drop(m,b,q);allowed+=1
            else:
                assert owned(m)==1<<(a-1)
                drop(m,a,p)
            assert owned(m)==0;pairs+=1
    m=StorageMachine()
    for slot,size in SIZES.items():
        assert not take(m,slot,0) and not take(m,slot,size+1)
        p=take(m,slot);assert p
        drop(m,slot,p+1);assert owned(m)==1<<(slot-1)
        assert not take(m,slot)
        drop(m,slot,p);assert owned(m)==0
    assert not m.checked('work_acquire',1,0) and not m.checked('work_acquire',1,15)
    # Передача прошивки сохранению настроек сохраняет живые контексты прошивки и камеры.
    photo=take(m,PHOTO);ctx=take(m,FW_CONTEXT);reserve=take(m,FW_DATA)
    assert photo and ctx and reserve
    assert not take(m,CONFIG)
    m.u.mem_write(ctx,b'X'*128);drop(m,FW_DATA,reserve)
    config=take(m,CONFIG);assert config==reserve
    assert bytes(m.u.mem_read(ctx,128))==b'X'*128
    drop(m,CONFIG,config);drop(m,FW_CONTEXT,ctx);drop(m,PHOTO,photo)
    return {'ordered_pairs':pairs,'compatible_pairs':allowed,'storage_bytes':2224}


if __name__=='__main__':
    print('PASS work memory',run())
