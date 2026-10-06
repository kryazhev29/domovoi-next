# Доработка: Кряжев А., 2026
"""Допись ID с меткой фиксации в связанном образе и обрывы каждого тела."""
from test_id_sync_fast import SyncMachine, fixture, cell, BASE, TimedMachine
from test_access_reads import AccessMachine
from storage_machine import crc16


# Проверяет основные сценарии модуля и возвращает результаты.
def run():
    records=b''.join(cell(i) for i in range(1,31))
    m=fixture(SyncMachine(),0,total=8192);m.record_changes=True
    m.command(13,records,30);m.drain();m.verify(30,8192)
    assert m.flash[BASE:BASE+960]==records
    writes=[(a,b) for k,a,b in m.changes if k=='write' and BASE<=a<BASE+8192]
    assert len(writes)==60 and not any(k=='erase' and BASE<=a<BASE+8192 for k,a,_ in m.changes)
    assert [len(b) for a,b in writes]==[31,1]*30
    count=0
    for n in range(32):
        a=AccessMachine();a.flash[BASE:BASE+n]=cell(1)[:n]
        a.write(a.addr('id_cells_end'),1,2);a.u.mem_write(0x21000000,cell(1)[2:12])
        assert a.checked('user_findID',0x21000000,0x21000100)==0
        assert a.checked('user_getNumbersCount',budget=2000000)==0
        assert a.read16('id_cells_end')==0
        r=fixture(SyncMachine(a.flash),0,unknown=True,total=8192);r.record_changes=True
        r.command(13,cell(1),1);r.drain();r.verify(1,8192)
        assert r.flash[BASE:BASE+32]==cell(1)
        if n:assert any(k=='erase' for k,_,_ in r.changes),'torn body needs erase'
        count+=1
    for marker in (0,1,2,4,8,16,32,64,128,254):
        a=AccessMachine();a.put_id(1,cell(1)[2:12]);a.flash[BASE+31]=marker
        a.u.mem_write(0x21000000,cell(1)[2:12])
        assert a.checked('user_findID',0x21000000,0x21000100)==1
    # Предварительная проверка всех назначений находит грязную ячейку до первой дописи.
    m=fixture(SyncMachine(),0,unknown=True,total=8192);m.flash[BASE+63]=0;m.record_changes=True
    m.command(13,cell(1)+cell(2),2);m.drain();m.verify(2,8192)
    first_id=next(k for k,a,b in m.changes if BASE<=a<BASE+8192)
    assert first_id=='erase'
    m=fixture(SyncMachine(),0,total=8192);m.write(m.device+m.d['datetime'],1790000000)
    m.command(13,cell(1),1);m.drain()
    assert m.flash[BASE:BASE+32]==cell(1),'append preserves all supplied reserve bytes'
    # Входная метка FF требует журнала и пересчёта даже при
    # замене действующего ID; сырая CRC по-прежнему учитывает всё тело записи.
    for previous in (0,1):
        m=fixture(SyncMachine(),previous,total=8192);m.record_changes=True
        incoming=cell(1)[:31]+b'\xff'
        m.command(13,incoming,1);m.drain()
        assert any(k=='erase' and BASE<=a<BASE+8192 for k,a,_ in m.changes)
        assert m.flash[BASE:BASE+32]==incoming
        assert m.u.mem_read(m.device+m.d['numbersCount'],2)==b'\0\0'
        assert m.u.mem_read(m.addr('id_cells_end'),2)==b'\0\0' and int.from_bytes(m.u.mem_read(m.addr('id_used_bytes'),4),'little')==32
        assert m.sent[-1][14:16]==crc16(m.flash[BASE:BASE+8192]).to_bytes(2,'little')
        a=AccessMachine();a.flash[:]=m.flash;a.write(a.addr('id_cells_end'),1,2)
        a.u.mem_write(0x21000000,cell(1)[2:12])
        assert a.checked('user_findID',0x21000000,0x21000100)==0
        assert a.checked('user_getNumbersCount',budget=2000000)==0
    return {'body_cuts_and_retries':count,'marker_cuts':10,'programs_per_30':60,
            'incoming_FF_marker_journal_recount_cases':2,
            'NOR_program_floor_ms':60*0.7,'target_20ms_feasible_in_existing_model':False}
