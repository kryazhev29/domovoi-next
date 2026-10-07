from test_flash_executor import finish, CANCELLED
# Доработка: Кряжев А., 2026
"""Кэш CRC сырых байтов: все писатели, сброс и обрывы каждой записи в связанном коде."""
from test_id_sync_fast import SyncMachine,fixture,cell,BASE
from test_access_reads import AccessMachine
from test_expiry_runtime import ExpiryMachine,record
from test_idclear_runtime import ClearMachine
from storage_machine import crc16
import struct

# Читает готовую CRC базы через её текущий интерфейс.
def get(m):
    p=0x21000800
    return int.from_bytes(m.u.mem_read(p,2),'little') if m.checked('idcrc_get',p) else None

# Считает результат по независимому эталону сырых данных.
def oracle(m):return crc16(m.flash[BASE:BASE+8192])

# Доводит кэш CRC до готовности перед проверкой записи.
def warm(m):
    for _ in range(1600):
        m.now+=1;m.crc_tick()
        value=get(m)
        if value is not None and not m.checked('idcrc_io_active'):return value
    raise AssertionError('CRC cache did not become known')

# Проверяет основные сценарии модуля и возвращает результаты.
def run():
    m=fixture(SyncMachine(),0,total=8192);assert warm(m)==oracle(m)
    for ids in ((1,2),(1,129),(2,)):
        m.command(13,b''.join(cell(i,len(ids)) for i in ids),len(ids));m.drain()
        assert get(m)==oracle(m);assert warm(m)==oracle(m)
        r=fixture(SyncMachine(m.flash),0,unknown=True,total=8192)
        assert warm(r)==oracle(r)
    a=AccessMachine();a.put_id(1,b'0000000001');assert warm(a)==oracle(a)
    row=bytearray(a.flash[BASE:BASE+32]);row[15]^=3
    a.u.mem_write(0x21000400,bytes(row))
    assert a.checked('user_updateID',0x21000400,budget=5000000)==1
    assert get(a)==oracle(a);assert warm(a)==oracle(a)
    e=ExpiryMachine();e.install([record(1)],(0,));assert warm(e)==oracle(e)
    e.start_expiry();e.expiry_drain();assert get(e)==oracle(e);assert warm(e)==oracle(e)
    c=ClearMachine();c.flash[BASE:BASE+32]=cell(1);assert warm(c)==oracle(c)
    c.parse();c.clear_drain();assert get(c)==oracle(c);assert warm(c)==oracle(c)
    return {'writers':['packet13 append','packet13 sector journal','user_updateID / cntPass / first-pass','expiry','clear'],'CRC':'raw full oracle'}

# Проверяет восстановление после обрывов изменяющих операций.
def power():
    m=fixture(SyncMachine(),0,total=8192);assert warm(m)==oracle(m)
    initial=bytes(m.flash);m.record_changes=True
    m.command(13,cell(1),1);m.drain();assert warm(m)==oracle(m)
    image=bytearray(initial);cuts=0
    for kind,a,b in m.changes:
        for n in sorted({0,1,len(b)//2,len(b)-1,len(b)}):
            cut=bytearray(image)
            for i,v in enumerate(b[:n]):cut[a+i]=v if kind=='erase' else cut[a+i]&v
            r=fixture(SyncMachine(cut),0,unknown=True,total=8192)
            assert r.checked('sj_recover',budget=5000000)
            expected=oracle(r)
            for _ in range(1600):
                r.now+=1;r.crc_tick();cached=get(r)
                assert cached is None or cached==expected,(kind,hex(a),n,cached,expected)
                if cached is not None and not r.checked('idcrc_io_active'):break
            else:raise AssertionError('recovery did not finish')
            cuts+=1
        for i,v in enumerate(b):image[a+i]=v if kind=='erase' else image[a+i]&v
    assert all(not 0x1fb000<=a<0x1fe000 for _,a,_ in m.changes)
    return {'cuts':cuts,'CRC_flash_writes':0}


# Проверяет фоновое построение CRC, приостановку и повтор после сброса.
def background():
    # Граница сырых байтов учитывает оборванные ячейки; готовая граница ограничивает чтение.
    counts=[]
    for used in (0,1,63,64,65,8192,0xffffffff):
        m=fixture(SyncMachine(),0,total=8192)
        n=min(used,8192);m.flash[BASE:BASE+n]=bytes((i*17)&255 for i in range(n))
        m.write(m.addr('id_used_bytes'),used);m.record_changes=True
        assert get(m) is None
        before=m.counts['read'];assert warm(m)==oracle(m)
        assert m.counts['read']-before==(n+63)//64 and not m.changes
        counts.append(used)
    # После каждого фрагмента сканирования имитируем сброс: CRC в ОЗУ снова неизвестна.
    m=fixture(SyncMachine(),0,unknown=True,total=8192)
    m.flash[BASE:BASE+8192]=bytes((i*13)&255 for i in range(8192))
    expected=oracle(m)
    for step in range(129):
        m.now+=1;m.crc_tick();assert m.read_budget<=64
        r=fixture(SyncMachine(m.flash),0,unknown=True,total=8192)
        assert get(r) is None and warm(r)==expected
    # Другой владелец флеша приостанавливает проход, сохраняя готовый префикс.
    m=fixture(SyncMachine(),0,unknown=True,total=8192);m.crc_tick()
    before=m.counts['read'];assert m.checked('service_crc_begin_config',0,73)
    assert m.exec_state() == (6,1,1)
    for _ in range(3):m.crc_tick()
    assert m.counts['read']==before
    m.checked('service_crc_cancel');finish(m,CANCELLED);assert warm(m)==oracle(m)
    assert m.counts['read']==128
    # Запись после частичного прохода и ошибка чтения требуют повторного сканирования.
    m=fixture(SyncMachine(),0,unknown=True,total=8192);m.crc_tick()
    m.command(13,cell(1),1);m.drain();assert get(m) is None and warm(m)==oracle(m)
    m=fixture(SyncMachine(),0,unknown=True,total=8192);m.fault=('read',1)
    m.crc_tick();assert get(m) is None
    m.fault=None;assert warm(m)==oracle(m)
    return {'occupied_boundaries':counts,'scan_power_cuts':129,'pause_resume':'prefix retained','writer_mid_scan':'restart','read_error':'retry','Flash_metadata':0}


# Проверяет запрет публикации CRC отменённой записи.
def cancellation():
    for point in ('old_bytes','verify'):
        m=fixture(SyncMachine(),1,total=8192);assert warm(m)==oracle(m)
        m.u.mem_write(0x21000400,bytes(32))
        assert m.checked('idcrc_io_start',BASE,0x21000400,32,m.now)
        if point=='verify':
            for _ in range(10):
                m.now+=1;assert m.checked('idcrc_io_poll',m.now)==1
                if m.io:break
            else:raise AssertionError('NOR start not reached')
        m.checked('idcrc_unknown');assert get(m) is None
        for _ in range(10):
            m.now+=1;r=m.checked('idcrc_io_poll',m.now)
            if r==2:break
            assert r==1
        else:raise AssertionError('cancel did not drain NOR')
        assert get(m) is None and warm(m)==oracle(m)
    m=fixture(SyncMachine(),0,total=8192);assert warm(m)==oracle(m)
    m.command(13,cell(1),1);m.checked('table_runtime_cancel');m.drain()
    assert get(m) is None and warm(m)==oracle(m)
    e=ExpiryMachine();e.install([record(1)],(0,));assert warm(e)==oracle(e)
    e.start_expiry();e.checked('expiry_runtime_cancel');assert get(e) is None
    e.expiry_drain();assert warm(e)==oracle(e)
    e=ExpiryMachine();e.install([record(1)],(0,));assert warm(e)==oracle(e)
    e.start_expiry();e.fault=('read',e.counts['read']+1);e.expiry_drain()
    assert get(e) is None;e.fault=None;assert warm(e)==oracle(e)
    c=ClearMachine();c.flash[BASE:BASE+32]=cell(1);assert warm(c)==oracle(c)
    c.parse();c.checked('idclear_runtime_cancel');assert get(c) is None
    c.clear_drain();assert warm(c)==oracle(c)
    a=AccessMachine();a.put_id(1,b'0000000001');assert warm(a)==oracle(a)
    a.u.mem_write(0x21000400,bytes(a.flash[BASE:BASE+32]))
    a.fault=('read',a.counts['read']+1)
    assert not a.checked('user_updateID',0x21000400,budget=5000000)
    assert get(a) is None;a.fault=None;assert warm(a)==oracle(a)
    a.u.mem_write(0x21000400,cell(2))
    assert not a.checked('user_updateID',0x21000400,budget=5000000)
    assert get(a) is None and warm(a)==oracle(a)
    return {'cancel_before_write_and_during_verify':2,'writer_cancels':['packet13','expiry','clear'],'unknown_old_read':'recompute','expiry_read_failure':'recompute','missing_update_ID':'recompute'}


# Проверяет учёт CRC через реальный драйвер записи и стирания W25.
def real_nor():
    from test_id_sync_fast import TimedMachine
    m=fixture(TimedMachine(),1,total=8192);m.install_nor();assert warm(m)==oracle(m)
    m.u.mem_write(0x21000400,bytes(32));peaks=[]
    for source,length in ((0x21000400,32),(0,0)):
        assert m.checked('idcrc_io_start',BASE,source,length,int(m.cycles//72000))
        for _ in range(1000):
            m.cycles+=m.gap;before=m.cycles
            r=m.checked('idcrc_io_poll',int(m.cycles//72000),budget=300000)
            peaks.append((m.cycles-before)/72000)
            if r==2:break
            assert r==1
        else:raise AssertionError('real NOR CRC operation stuck')
        assert get(m)==oracle(m)
    assert max(peaks)<1
    return {'native_NOR_job_ABI':'verified','write_and_erase':'oracle equal','maximum_CRC_poll_ms':round(max(peaks),6),'timing':'modeled SPI/NOR'}


# Проверяет восстановление журнала неизменённым загрузчиком.
def loader_recovery():
    m=fixture(SyncMachine(),1,total=8192);assert warm(m)==oracle(m)
    initial=bytes(m.flash);m.record_changes=True;m.command(13,cell(1,1),1);m.drain();warm(m)
    cut=bytearray(initial)
    for kind,a,b in m.changes:
        for i,v in enumerate(b):cut[a+i]=v if kind=='erase' else cut[a+i]&v
        if kind=='write' and a==0x1fa01c:break
    else:raise AssertionError('journal commit not reached')
    assert cut[BASE:BASE+8192]==initial[BASE:BASE+8192]
    # Неизменённый загрузчик восстанавливает сектор прямыми вызовами NOR;
    # он не знает о кэше CRC приложения в ОЗУ.
    loader=fixture(SyncMachine(cut),0,unknown=True,total=8192)
    loader.stubs[loader.addr('idcrc_write')]=loader.sync_write
    loader.stubs[loader.addr('idcrc_erase')]=loader.sync_erase
    assert loader.checked('sj_recover',budget=2000000)
    assert loader.flash[BASE:BASE+32]==cell(1,1)
    reboot=fixture(SyncMachine(loader.flash),0,unknown=True,total=8192)
    expected=oracle(reboot)
    for _ in range(1600):
        reboot.now+=1;reboot.crc_tick();cached=get(reboot)
        assert cached is None or cached==expected,('loader bypassed dirty',cached,expected)
        if cached is not None and not reboot.checked('idcrc_io_active'):break
    else:raise AssertionError('loader recovery CRC did not finish')
    return {'cut':'SJ commit before first MB_ID mutation','loader':'unmodified raw NOR boundary','CRC':expected}


# Проверяет восстановление после отказа фиксации журнала.
def failed_commit_recovery():
    m=fixture(SyncMachine(),1,total=8192);assert warm(m)==oracle(m)
    m.command(13,cell(1,1),1)
    for _ in range(4000):
        m.tick()
        if m.io and m.io[0]==0x1fa01c:break
    else:raise AssertionError('commit marker not reached')
    m.fault=('poll',m.counts['poll']+1);m.drain()
    assert m.answers and not m.sent
    # Публикация настроек после отказа не должна скрыть незавершённое
    # восстановление загрузчика за готовой CRC состояния до записи.
    loader=fixture(SyncMachine(m.flash),0,unknown=True,total=8192)
    loader.stubs[loader.addr('idcrc_write')]=loader.sync_write
    loader.stubs[loader.addr('idcrc_erase')]=loader.sync_erase
    assert loader.checked('sj_recover',budget=2000000)
    reboot=fixture(SyncMachine(loader.flash),0,unknown=True,total=8192);expected=oracle(reboot)
    for _ in range(1600):
        reboot.now+=1;reboot.crc_tick();cached=get(reboot)
        assert cached is None or cached==expected,('failed commit published old CRC',cached,expected)
        if cached is not None and not reboot.checked('idcrc_io_active'):break
    else:raise AssertionError('failed commit recovery did not finish')
    return {'failed_marker_verification':'dirty retained','CRC':expected}
