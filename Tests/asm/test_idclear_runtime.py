# Доработка: Кряжев А., 2026
from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Очистка команды 103 в связанном IAR ELF; полное стирание не атомарно."""
import hashlib,json,struct
from storage_machine import *
from test_memory_runtime import heap_stats

BASE=0x41000

class ClearMachine(StorageMachine):
    # Готовит связанный образ, модель NOR и состояние сценария.
    def __init__(self,flash=None):
        super().__init__(flash);self.wires=[];self.busy=0;self.passes=[];self.legacy=False
        self.stubs[self.addr('usb_try_send' if 'usb_try_send' in self.symbols else 'usb_send')]=self.send
        if flash is None:self.flash[BASE:BASE+8192]=b'\x5a'*8192
        self.write(self.device+81,7,2);self.write(self.addr('id_cells_end'),129,2)
        self.write(self.addr('id_used_bytes'),8192)
        self.write(self.device+47,1700000400);self.write(self.device+165,987,2)
    # Читает модельную NOR с контролем диапазона и заданным отказом.
    def read(self):
        assert self.legacy or ('idcrc_io_poll' in self.symbols and self.args()[2]<=64), 'CRC delta reads are bounded to 64 bytes'
        a,p,n,_=self.args();assert n<=1024
        self.read_budget+=n;self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    # Моделирует проверенное синхронное стирание NOR.
    def sync_erase(self):
        assert self.legacy,'synchronous runtime erase'
        super().sync_erase()
    # Моделирует передачу буфера по границе SPI.
    def send(self):
        p,n,_,_=self.args();self.wires.append(bytes(self.u.mem_read(p,n)))
        if self.busy:self.busy-=1;self.ret(0)
        else:self.ret(1)
    # Начинает очистку через настоящий вход команды 103.
    def start_clear(self,channel=0,uniq=7):return self.checked('idclear_runtime_begin',channel,uniq)
    # Читает признак незавершённой очистки базы.
    def clear_active(self):return self.checked('idclear_runtime_active')!=0
    # Выполняет шаг очистки и проверяет бюджет обмена.
    def clear_tick(self):
        self.now+=1;self.checked('idclear_runtime_poll',self.now)
        self.passes.append(self.steps);assert self.io_budget<=1 and self.read_budget<=(64 if 'idcrc_io_poll' in self.symbols else 0)
    # Доводит очистку до конца с ограничением числа проходов.
    def clear_drain(self):
        for _ in range(10000):
            if not self.clear_active():return
            self.clear_tick()
        raise AssertionError('clear stuck')
    # Читает счётчики и границу занятой базы из связанного устройства.
    def counters(self):
        return (int.from_bytes(self.u.mem_read(self.device+81,2),'little'),
                int.from_bytes(self.u.mem_read(self.addr('id_cells_end'),2),'little'),
                int.from_bytes(self.u.mem_read(self.addr('id_used_bytes'),4),'little'))
    # Запускает настоящий разбор входной команды.
    def parse(self,channel=0,uniq=7):
        b=struct.pack('<HBBB',3,0,uniq,103);b+=struct.pack('<H',crc16(b))
        self.u.mem_write(0x21000000,b);self.checked('user_parse_comand',0x21000000,channel)
        self.u.mem_write(0x21000000,b'\xa5'*len(b))

# Собирает ожидаемые байты кадра независимо от кода продукта.
def wire(error=False,uniq=7):
    b=struct.pack('<HBBB',11 if error else 3,255,uniq,11 if error else 103)
    if error:b+=struct.pack('<BBIH',103,2,1700000400,987)
    return b'SLDN'+b+struct.pack('<H',crc16(b))

# Проверяет успешную очистку, неизменный ответ и возврат памяти.
def success_and_heap():
    m=ClearMachine();live=[(slot,take(m,slot)) for slot in (PHOTO,PHOTO_PACKET)]
    m.parse();assert m.clear_active() and not m.answers and not m.wires
    assert m.counts=={'read':0,'start':0,'poll':0}
    p=int.from_bytes(m.u.mem_read(m.addr('idclear_job'),4),'little')
    assert owned(m)&(1<<6)
    peak=heap_stats(m);m.busy=3
    while m.clear_active():
        m.clear_tick()
        if m.counts['poll']<6:assert m.counters()==(7,129,8192)
    assert m.counters()==(0,0,0) and m.flash[BASE:BASE+8192]==b'\xff'*8192
    assert m.wires==[wire()]*4 and m.io is None
    for slot,p in live:drop(m,slot,p)
    assert owned(m)==0
    small={'passes':len(m.passes),'maximum_instructions':max(m.passes),
           'total_instructions':sum(m.passes),'stack':m.stack_peak,'context_bytes':40,
           'live_workspace_payload':180+534+40,'free':peak}
    m=ClearMachine();m.write(m.addr('mem')+20,1601536)
    m.flash[BASE:BASE+1601536]=b'\x5a'*1601536
    m.record_changes=True;assert m.start_clear();m.clear_drain()
    assert m.flash[BASE:BASE+1601536]==b'\xff'*1601536 and m.wires==[wire()]
    assert [a for k,a,b in m.changes if k=='erase' and BASE<=a<BASE+1601536]==list(range(BASE,BASE+1601536,4096))
    assert m.counts['poll']==3*m.counts['start']
    small['full_partition']={'bytes':1601536,'passes':len(m.passes),
        'maximum_instructions':max(m.passes),'total_instructions':sum(m.passes),
        'stack':m.stack_peak,'async_starts':m.counts['start'],'async_polls':m.counts['poll']}
    return small

# Проверяет отмену очистки, смену сеансов и ошибки W25.
def sessions_cancel_faults():
    count=0;max_instructions=0;max_stack=0
    for ticks in range(80):
        m=ClearMachine();assert m.start_clear()
        for _ in range(ticks):m.clear_tick()
        before_wires=list(m.wires);m.checked('idclear_runtime_cancel');m.clear_drain()
        assert m.wires==before_wires and m.io is None
        if not ticks:assert m.counters()==(7,129,8192) and m.u.mem_read(m.addr('mem_storage_ready'),1)==b'\1'
        elif m.counters()==(0,0,0):
            assert m.flash[BASE:BASE+8192]==b'\xff'*8192
        else:
            assert m.counters()==(7,129,8192) and m.u.mem_read(m.addr('mem_storage_ready'),1)==b'\0'
        max_instructions=max(max_instructions,max(m.passes));max_stack=max(max_stack,m.stack_peak)
        count+=1
    for event in ('epoch','quiet_usb','quiet_server','savereset'):
        m=ClearMachine();assert m.start_clear();m.clear_tick()
        if event=='epoch':m.write(m.addr('usb_bus_resets'),1)
        elif event=='savereset':m.checked('user_savereset')
        else:m.checked('idclear_runtime_quiet',int(event=='quiet_server'))
        m.clear_drain();assert m.io is None
        assert m.wires==([wire()] if event=='quiet_server' else [])
        assert m.counters()==((7,129,8192) if event=='savereset' else (0,0,0));count+=1
        max_instructions=max(max_instructions,max(m.passes));max_stack=max(max_stack,m.stack_peak)
    for kind,points in (('start',(1,2)),('poll',(1,2,3,4,5,6))):
        for point in points:
            m=ClearMachine();m.fault=(kind,point);m.busy=3;assert m.start_clear()
            while not m.wires:m.clear_tick()
            m.write(m.device+47,100);m.write(m.device+165,0,2);m.clear_drain()
            assert m.wires==[wire(True)]*4 and m.counters()==(7,129,8192)
            assert m.u.mem_read(m.addr('mem_storage_ready'),1)==b'\0' and m.io is None
            assert not m.start_clear();count+=1
            max_instructions=max(max_instructions,max(m.passes));max_stack=max(max_stack,m.stack_peak)
    return {'cases':count,'maximum_instructions':max_instructions,'stack':max_stack}

# Проверяет отказ на запрещённых входах и занятых ресурсах.
def guards():
    count=0
    # Адреса mem[] задаются при user_init: проверяем профиль один раз в тесте.
    m=ClearMachine()
    base=int.from_bytes(m.u.mem_read(m.addr('mem')+16,4),'little');size=int.from_bytes(m.u.mem_read(m.addr('mem')+20,4),'little')
    assert base==BASE and size==8192 and not (base|size)&4095
    assert 0x41000<=base<base+size<=0x1c8000;count+=1
    m=ClearMachine();p=take(m,REPLY);assert p
    old=bytes(m.u.mem_read(m.addr('ix'),16));m.parse()
    assert not m.clear_active() and not any(m.counts.values()) and m.answers[-1]==(103,2,0,7)
    assert bytes(m.u.mem_read(m.addr('ix'),16))==old;count+=1
    m=ClearMachine();assert not m.start_clear(1);m.parse(1);assert not m.clear_active();count+=1
    m=ClearMachine();assert m.start_clear();assert not m.start_clear();count+=1
    m=ClearMachine();m.write(m.addr('mem')+20,0);m.record_changes=True;assert m.start_clear();m.clear_drain()
    assert m.counters()==(0,0,0) and not any(m.counts.values()) and m.wires==[wire()];count+=1
    return count

# Обрывает каждое стирание базы и проверяет сохранившиеся данные.
def bulk_power():
    # Обрыв стирания может сохранить любой префикс; атомарность журнала здесь
    # не обещается: нетронутые записи остаются, а успешного ответа не было.
    m=ClearMachine();m.record_changes=True;assert m.start_clear();m.clear_drain()
    state=bytearray(b'\x5a'*8192);count=0
    for kind,addr,data in m.changes:
        if not BASE<=addr<BASE+8192:continue  # Обрывы публикации CRC проверяет отдельная группа.
        assert kind=='erase'
        offset=addr-BASE
        for n in (0,1,2048,4095,4096):
            cut=bytearray(state);cut[offset:offset+n]=b'\xff'*n
            assert cut[:offset]==b'\xff'*offset
            assert cut[offset+n:]==b'\x5a'*(8192-offset-n)
            image=bytearray(m.flash);image[BASE:BASE+8192]=cut
            reboot=ClearMachine(image);reboot.legacy=True
            reboot.checked('idindex_runtime_suspend')
            expected=sum(cut[i:i+2]!=b'\xff\xff' for i in range(0,8192,32))
            assert reboot.checked('user_getNumbersCount',budget=1000000)==expected
            assert not reboot.clear_active() and not reboot.wires
            count+=1
        state[offset:offset+4096]=b'\xff'*4096
    return {'partial_erase_cuts_with_real_startup_scan':count,'atomic':False}

if __name__=='__main__':
    out={'success_heap':success_and_heap()};print('parser/immutable ACK/busy/full ID/heap PASS',flush=True)
    out['cancel_sessions_io']=sessions_cancel_faults();out['guards_oom']=guards()
    print('cancel/reset/USB reconnect/I/O/bounds/OOM PASS',flush=True)
    out['power']=bulk_power()
    out['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/idclear_runtime_results.json').write_text(json.dumps(out,indent=2)+'\n')
