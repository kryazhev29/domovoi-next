"""Доработка: Кряжев А., 2026. Камера и команды в машинном коде IAR.
UART и NOR моделируются; кадры, владение буферами и бюджет шага проверяются.
Смещения контекста берутся из модуля фото, а не из старой копии структуры.
"""
from test_work_memory import take, drop, PHOTO, REPLY
from iar_image import image_paths
import json,struct
from storage_machine import *

HEAD=13
FORMAT=0x32504850
PHASE={name:i+1 for i,name in enumerate(("IO","HEAD","CAMERA","LAZY_DONE","DATA_DONE","PHOTO_MARK","PHOTO_DONE","CLEAN_END","ACK_HEAD","ACK_ADVANCE","REQUEST_DONE","SEND_FILL","SEND_COMMIT"))}
# Вычислить начало следующего снимка с выравниванием по сектору.
def next_record(a,size):return (a+HEAD+size+4095)&~4095

# Читаем фактический тип камеры: runtime_layout.c пока содержит старую копию.
def pf():
    image=image_paths()[0]
    return photo_fields(str(image), image.stat().st_mtime_ns)

# Кешируем DWARF отдельно для каждого образа при сравнении до и после.
@lru_cache(None)
def photo_fields(path, version):
    with open(path, 'rb') as source:
        for cu in ELFFile(source).get_dwarf_info().iter_CUs():
            if not cu.get_top_DIE().attributes['DW_AT_name'].value.replace(b'\\', b'/').endswith(b'/photo_runtime.c'):
                continue
            for die in cu.iter_DIEs():
                if die.tag=='DW_TAG_typedef' and die.attributes.get('DW_AT_name', None) and die.attributes['DW_AT_name'].value==b'photo_s':
                    result={}
                    for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                        if 'DW_AT_name' not in member.attributes:continue
                        offset=member.attributes['DW_AT_data_member_location'].value
                        if not isinstance(offset,int):offset=DWARFExprParser(cu.structs).parse_expr(offset)[0].args[0]
                        result[member.attributes['DW_AT_name'].value.decode()]=offset
                    return result
    raise AssertionError(('контекст фото не найден', path))
class PhotoMachine(StorageMachine):
    # Подготовить модель и перехватить аппаратные границы машинного кода.
    def __init__(self,flash=None):
        super().__init__(flash);self.runtime_fields=fields('photo_runtime_s');self.rx_count=0;self.commands=[];self.events=[];self.sent=[]
        self.transport_busy=False;self.capacity=2048;self.poll_costs=[];self.port_costs=[]
        config=fields('config_s');self.write(self.addr('config')+config['rs485Mode'],1,1)
        self.u.mem_write(self.addr('config')+config['rs485Periphery'],bytes([5,3]))
        for name,fn in [('uart_init',self.ret),('uart_setRecTimeout',self.ret),
          ('uart_send',self.uart_send),('uart_getstate',lambda:self.ret(self.rx_count!=0)),
          ('uart_isReceiving',lambda:self.ret(0)),('uart_received',lambda:self.ret(self.rx_count)),
          ('uart_clear',self.uart_clear),('log_addPhotoEvent',self.event),
          ('usb_send',self.usb_send),('comm_channel_ready',lambda:self.ret(1)),
          ('comm_tx_acquire',self.acquire),('comm_tx_release',self.ret),
          ('comm_tx_commit',self.commit),('comm_tx_busy',lambda:self.ret(self.transport_busy))]:
            self.stubs[self.addr(name)]=fn
        if 'usb_try_send' in self.symbols:
            self.stubs[self.addr('usb_try_send')]=self.usb_send
    # Записать команду камеры и проверить её максимальный размер.
    def uart_send(self):
        p,n,_,_=self.args();assert n<=6;self.commands.append(bytes(self.u.mem_read(p,n)));self.ret()
    # Освободить принятые байты модели UART.
    def uart_clear(self):self.rx_count=0;self.ret()
    # Запомнить номер фото из события журнала.
    def event(self):self.events.append(self.args()[0]);self.ret()
    # Записать принятый транспортом пакет и проверить его размер.
    def usb_send(self):
        p,n,_,_=self.args();assert n<=534
        self.sent.append(bytes(self.u.mem_read(p,n)));self.ret(1)
    # Выдать слот серверного транспорта, если он свободен.
    def acquire(self):
        p=self.args()[0];self.write(p,self.capacity,2);self.ret(0 if self.transport_busy else 0x21000800)
    # Запомнить пакет, принятый серверным транспортом.
    def commit(self):
        n=self.args()[0];self.sent.append(bytes(self.u.mem_read(0x21000800,n)));self.ret(1)
    # Получить адрес рабочего контекста камеры из ELF.
    def photo_ptr(self):return int.from_bytes(self.u.mem_read(self.addr('photo'),4),'little')
    # Прочитать поле фактического контекста камеры по DWARF.
    def pget(self,n,size=1):return int.from_bytes(self.u.mem_read(self.photo_ptr()+pf()[n],size),'little')
    # Прочитать поле автомата хранения по DWARF.
    def rget(self,n,size=1):return int.from_bytes(self.u.mem_read(self.addr('pr')+self.runtime_fields[n],size),'little')
    # Узнать, владеет ли автомат фото флеш.
    def active(self):return self.call('photo_storage_busy')!=0
    # Выполнить шаг хранения и проверить ограничение работы с NOR.
    def tick(self):
        self.now+=1;self.checked('user_storage_poll')
        self.poll_costs.append(self.steps)
        assert self.read_budget<=64,('photo read budget',self.read_budget)
        assert self.io_budget<=1
    # Выполнить обработку RS485 и проверить бюджет чтения.
    def port(self):
        self.now+=1;self.checked('port_check_data');assert self.read_budget<=64
        self.port_costs.append(self.steps)
    # Передать порцию UART в реальный обработчик камеры.
    def feed(self,data):
        assert len(data)<=64 and self.photo_ptr()
        self.u.mem_write(self.photo_ptr()+pf()['rx'],bytes(data));self.rx_count=len(data);self.port()
    # Передать команду с CRC и затереть исходный буфер после разбора.
    def packet(self,cmd,body=b'',channel=0,uniq=9):
        p=struct.pack('<HBBB',len(body)+3,0,uniq,cmd)+body;p+=struct.pack('<H',crc16(p))
        self.u.mem_write(0x21000000,p);self.checked('user_parse_comand',0x21000000,channel)
        self.u.mem_write(0x21000000,b'\xa5'*len(p))
    # Восстановить очередь и проверить завершение стартового скана.
    def boot_media(self):
        self.port();self.drain()
        # Стартовый скан либо восстановил очередь, либо запустил её очистку.
        assert (self.rget('initialized') or self.rget('cleanup')) and not self.rget('fault')
    # Пройти согласование камеры до готовности принимать снимок.
    def begin_camera(self,size,drain=True):
        self.packet(7,b'\5');assert self.answers[-1][1]==0 and self.active()
        self.drain();assert self.pget('stage')==1
        self.port();self.feed(bytes([0xaa,0x0e,0x0d,0,0,0]));self.feed(bytes([0xaa,0x0d,0,0,0,0]))
        for ack in (1,6,5):
            self.now+=50;self.port();self.feed(bytes([0xaa,0x0e,ack,0,0,0]))
        self.now+=50;self.port();self.feed(bytes([0xaa,0x0a,1,size&255,(size>>8)&255,(size>>16)&255]))
        if drain:self.drain();assert self.pget('stage')==6
    # Дождаться запроса следующего пакета без пропуска номера.
    def next_request(self):
        packet=self.pget('packet',2)
        if self.commands and self.commands[-1]==bytes([0xaa,0x0e,0,0,packet&255,packet>>8]) and self.pget('deadline',4)>self.now:return
        old=len(self.commands)
        for _ in range(100):
            self.port()
            if self.active():self.drain()
            if len(self.commands)>old:return
        raise AssertionError('camera request did not progress')
    # Записать пакет камеры, проверить срок жизни буфера и обработку дубля.
    def data(self,payload,duplicate=False):
        self.next_request();packet=self.pget('packet',2);written=self.pget('written',2)
        assert packet==written//58,'camera packet skipped or repeated'
        frame=struct.pack('<HH',packet,len(payload))+payload
        frame+=bytes([sum(frame)&255,0]);self.feed(frame)
        assert self.active() and self.pget('written',2)==written
        self.u.mem_write(self.photo_ptr()+pf()['frame'],b'\xa6'*64)
        before=len(self.commands);self.port();assert len(self.commands)==before
        self.drain();assert not self.rget('fault')
        if duplicate and self.pget('stage'):
            before=self.pget('written',2);self.feed(frame)
            assert self.pget('written',2)==before and not self.active()
    # Записать весь снимок и проверить заголовок, данные и выравнивание.
    def capture(self,data):
        self.begin_camera(len(data));start=self.pget('address',4);stamp=self.pget('stamp',4)
        for off in range(0,len(data),58):self.data(data[off:off+58])
        stamp=self.events[-1]
        assert self.pget('stage')==0 and stamp==self.pget('stamp',4)
        assert self.flash[start:start+8]==struct.pack('<II',stamp,len(data))
        assert not start%4096
        assert self.flash[start+HEAD:start+HEAD+len(data)]==data
        assert self.flash[start+8:start+HEAD]==struct.pack("<IB",FORMAT,255)
        return start,stamp

# Создать готовое фото и занятый хвост для проверки границ секторов.
def prefix_flash(prefix=4087,dirty=True):
    # Параметр — прежняя граница данных; новый снимок всегда в следующем секторе.
    f=bytearray(b'\xff'*0x400000);base=0x200000;size=prefix-HEAD
    f[base:base+HEAD]=struct.pack('<IIIB',50,size,FORMAT,255)
    f[base+HEAD:base+prefix]=bytes((i*19)&255 for i in range(size))
    if dirty:f[base+4095]=0
    return f


# Проверить размеры снимков, содержимое NOR и бюджет каждого шага.
def normal():
    results={}
    for size in (1,58,59,255,256,4095,4096,65535):
        m=PhotoMachine();m.boot_media();data=bytes((i*37)&255 for i in range(size));m.capture(data)
        assert m.u.mem_read(m.device+m.d['countPhoto'],2)==b'\1\0' and len(m.events)==1
        results[str(size)]={'max_instructions':max(m.costs),'max_passes':len(m.costs),
                            'max_storage_poll':max(m.poll_costs),'max_port_poll':max(m.port_costs),
                            'total_instructions':sum(m.costs),'stack':m.stack_peak}
        print('camera',size,'PASS',flush=True)
    m=PhotoMachine(prefix_flash());old=bytes(m.flash[0x200000:0x201000]);m.boot_media();m.capture(b'z'*129)
    assert m.flash[0x200000:0x201000]==old and m.events==[51]
    assert m.flash[0x2ff000:0x300000]==b'\xff'*4096
    results['sector_alignment']={'max_instructions':max(m.costs),'max_storage_poll':max(m.poll_costs),
                          'passes':len(m.costs),'total_instructions':sum(m.costs),'stack':m.stack_peak}
    print('sector alignment/lazy erase PASS',flush=True)
    return results

# Проверить пакеты 14, частичный курсор, полный ACK и повтор после очистки.
def uploads():
    m=PhotoMachine();m.boot_media();data=bytes(range(256))*3;address,stamp=m.capture(data)
    m.packet(21,b'');assert m.active();before=len(m.answers);m.drain();assert len(m.answers)==before+1 and m.answers[-1][1]==0
    for _ in range(20):
        m.port();m.drain()
        if len(m.sent)==3:break
    assert len(m.sent)==3 and m.sent[-1][9]==0
    assert b''.join(x[20:-2] for x in m.sent[:-1])==data
    for p in m.sent:assert crc16(p[4:-2])==int.from_bytes(p[-2:],'little')
    m.packet(21,struct.pack('<IH',stamp,512));m.drain()
    assert m.flash[address+8:address+HEAD]==struct.pack('<IB',FORMAT,255)
    assert m.rget('resume_offset',2)==512
    m.packet(21,struct.pack('<IH',stamp,512));m.drain()
    assert m.flash[address+8:address+HEAD]==struct.pack('<IB',FORMAT,255),'partial ACK wrote flash'
    m.packet(21,struct.pack('<IH',stamp,len(data)));m.drain();assert m.flash[address+HEAD-1]==0
    for _ in range(60):m.port();m.drain()
    assert m.flash[0x200000:0x201000]==b'\xff'*4096 and not m.rget('cleanup')
    assert not m.rget('fault') and m.rget('initialized')
    # Повтор итогового ACK после очистки и перезагрузки — ответ 11/0 и пустой 14.
    for machine in (m, PhotoMachine(m.flash)):
        if machine is not m:machine.boot_media()
        machine.sent=[];machine.packet(21,struct.pack('<IH',stamp,len(data)));machine.drain()
        assert machine.answers[-1][1]==0
        machine.port();machine.drain();assert len(machine.sent)==1 and machine.sent[0][9]==0
    # Номер 0 после перезапуска пропускает полностью подтверждённое фото.
    q=PhotoMachine();q.boot_media();a,s1=q.capture(b'a'*59);b,s2=q.capture(b'b'*60)
    q.packet(21,struct.pack('<IH',s2,17));q.drain()
    assert q.flash[a+HEAD-1]==0 and q.flash[b+HEAD-1]==255
    r=PhotoMachine(q.flash);r.boot_media();r.packet(21,struct.pack('<IH',0,0));r.drain()
    assert r.pget('readAddress',4)==b and r.pget('offset',2)==0
    r.packet(21,struct.pack('<IH',s2,17));r.drain()
    r.port();r.drain();assert r.sent[-1][20:-2]==b'b'*43
    # Во время первого стирания повтор ACK не отменяет очистку.
    r.packet(21,struct.pack('<IH',s2,60));r.drain();r.sent=[]
    r.port();r.drain();r.port()
    assert r.rget('cleanup')
    r.packet(21,struct.pack('<IH',s2,60));assert r.answers[-1][1]==0
    r.drain();r.port();r.drain();assert r.sent[-1][9]==0
    print('upload 512, CRC, RAM cursor/full ACK/retry, cleanup PASS',flush=True)
    return {'max_instructions':max(m.costs),'passes':len(m.costs),'stack':m.stack_peak}

# Проверить дубли кадров, отказ памяти, отмену, сбои NOR и разрыв канала.
def errors():
    m=PhotoMachine();m.boot_media();m.begin_camera(116);m.data(b'A'*58,True);m.data(b'B'*58)
    assert len(m.events)==1 and m.flash[0x200000+HEAD:0x200000+HEAD+116]==b'A'*58+b'B'*58
    m=PhotoMachine();m.boot_media();assert take(m,PHOTO)
    m.packet(7,b'\5');assert m.answers[-1][1]==2 and not m.active() and not m.counts['start']
    m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request()
    frame=struct.pack('<HH',0,58)+b'x'*58;frame+=bytes([sum(frame)&255,0]);m.feed(frame)
    m.tick();assert m.io is not None
    m.checked('photo_storage_cancel');m.drain();assert m.io is None and not m.events and m.flash[0x200000:0x200008]==b'\xff'*8
    m=PhotoMachine();m.boot_media();m.begin_camera(58);m.next_request();m.fault=('poll',m.counts['poll']+1);m.feed(frame);m.drain()
    assert m.rget('fault') and not m.events and m.io is None
    m=PhotoMachine();m.boot_media();m.capture(b'a'*100)
    m.packet(21,b'',1);m.checked('user_comm_session',0);m.drain();assert not m.sent and m.pget('pending')==0
    m.packet(21,b'');m.write(m.addr('usb_bus_resets'),1);m.drain();assert m.pget('pending')==0
    print('duplicates/heap/cancel/write-failure/reconnect/USB PASS',flush=True)
    return 6

if __name__=='__main__':
    out={'units':'Unicorn instructions with NOR/UART models; NOT cycles/time','normal':normal(),'upload':uploads(),'errors':errors()}
    out['elf_sha256']=hashlib.sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest()
    (__import__('iar_image').result_path('photo_runtime_results.json')).write_text(json.dumps(out,indent=2)+'\n')
    print('PASS actual photo ASM/camera/main-loop: verified I/O, ownership, ABI, bounded work')
