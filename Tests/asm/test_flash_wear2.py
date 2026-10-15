"""Кольцо фото: настоящий ELF, модель границ NOR/UART."""
import struct
from test_photo_runtime import PhotoMachine

class RingPhoto(PhotoMachine):
    def __init__(self, flash=None, trial=False):
        super().__init__(flash if flash is not None else bytes([255])*0x1000000)
        self.write(self.addr('trial'),int(trial),1)

    def getd(self,name,size=4):
        return int.from_bytes(self.u.mem_read(self.device+self.d[name],size),"little")

    def boot_ring(self):
        self.boot_media()
        self.port(); self.drain()
        assert self.rget('circular')==1 and not self.rget('fault')

    def capture(self,data):
        self.begin_camera(len(data)); start=self.pget('address',4)
        for off in range(0,len(data),self.packet_size-6):
            self.data(data[off:off+self.packet_size-6])
        assert self.flash[start+32:start+32+len(data)]==data
        assert self.flash[start+8:start+12]==struct.pack('<I',0x33504850)
        return start,self.events[-1]

    def capture_policy(self,size):
        # Камера и все payload-chunks проверяются capture/power. Здесь ускорен лишь
        # перенос payload: решения стирания, намерение, заголовок и ACK исполняет ELF.
        from test_photo_runtime import pf
        self.begin_camera(size)
        start=self.pget('address',4)
        self.write(self.photo_ptr()+pf()['written'],size,2)
        while not self.checked('pr_data_ready'):
            self.drain()
        head=32 if 'circular' in self.runtime_fields and self.rget('circular') else 13
        self.change('write',start+head,b'X'*size)
        self.write(self.photo_ptr()+pf()['written'],0,2)
        assert self.checked('pr_begin',5)
        self.write(self.job+self.job_fields['io_len'],size,2)
        self.drain()
        assert self.getd('countPhoto',2)==1
        return start,self.events[-1]

    def ack(self,number,size):
        self.packet(21,struct.pack('<IH',number,size)); self.drain()
        assert self.answers[-1][1]==0,self.answers
        self.checked('photo_storage_cancel'); self.drain()
        self.port();self.drain()

def photo_smoke():
    m=RingPhoto();m.boot_ring()
    a,num=m.capture(bytes([65])*13266)
    head=m.getd('newAddressPhoto')
    m.ack(num,13266)
    assert m.getd('countPhoto',2)==0
    assert m.getd('newAddressPhoto')==head and head>a
    b,num=m.capture(bytes([66])*51200)
    assert b==head
    q=RingPhoto(m.flash);q.boot_ring()
    assert q.getd('countPhoto',2)==1
    assert q.rget('oldest',4)==b
    q.ack(num,51200)
    r=RingPhoto(q.flash);r.boot_ring()
    assert r.getd('countPhoto',2)==0
    assert r.getd('newAddressPhoto')==q.getd('newAddressPhoto')
    return {'first':a,'second':b,'head':r.getd('newAddressPhoto')}

def photo_queue():
    from test_flash_wear import fast
    m=fast(RingPhoto());m.boot_ring()
    # Последовательная запись у конца кольца и переход через физическую границу.
    m.write(m.device+m.d['newAddressPhoto'],0x7ef000)
    a,n=m.capture(b'A'*13266)
    b,n2=m.capture(b'B'*51200)
    assert a==0x7ef000 and b==0x400000
    assert m.rget('turns',4)==1
    q=fast(RingPhoto(m.flash));q.boot_ring()
    assert q.getd('countPhoto',2)==2 and q.rget('oldest',4)==a
    q.packet(21,b'');q.drain();q.port();q.drain()
    assert q.sent[-1][10:14]==struct.pack('<I',n)
    q.checked('photo_storage_cancel');q.drain()
    q.ack(n2,51200)
    assert q.getd('countPhoto',2)==0
    r=fast(RingPhoto(q.flash));r.boot_ring()
    assert r.getd('countPhoto',2)==0 and r.rget('turns',4)==1
    # Очередь ограничена протоколом, хотя физический запас больше.
    m=fast(RingPhoto());m.boot_ring()
    for i in range(20):m.capture(bytes([i])*51200)
    before=bytes(m.flash)
    m.begin_camera(51200,drain=False)
    assert (not m.photo_ptr() or m.pget('stage')==0) and bytes(m.flash)==before
    assert m.getd('countPhoto',2)==20
    return {'wrap':True,'queue_limit':20,'unacknowledged_preserved':True}

def photo_erase_intent():
    from test_flash_wear import fast
    from storage_machine import crc16
    f=bytearray([255])*0x1000000
    def record(address,number,ack,intent=0xffffffff):
        body=struct.pack('<II',number,100)
        crc=crc16(body+struct.pack('<I',0))
        h=struct.pack('<IIIIIIII',number,100,0x33504850,
                      0xffffff00|ack,0,crc|((crc^65535)<<16),number^0xffffffff,intent)
        f[address:address+32]=h
        f[address+32:address+132]=b'x'*100
    record(0x400000,10,255) # Был ACK=0; прерванное стирание восстановило только байт ACK.
    record(0x7fc000,20,255,0xffff0000) # Долговечное намерение освободить 17 секторов от нуля.
    m=fast(RingPhoto(f));m.boot_ring()
    assert m.getd('countPhoto',2)==1 and m.rget('oldest',4)==0x7fc000
    assert m.rget('erase_intent',4)==0x400000
    return {'torn_erase_ack_bit_only':'not resurrected'}


def photo_repeat_ack():
    from test_flash_wear import fast
    m=fast(RingPhoto());m.boot_ring();a,n=m.capture(b'a'*700);b,n2=m.capture(b'b'*800)
    m.ack(n,700);assert m.getd('countPhoto',2)==1
    image=bytes(m.flash);m.ack(n,700)
    assert bytes(m.flash)==image and m.getd('countPhoto',2)==1
    q=fast(RingPhoto(m.flash));q.boot_ring();q.ack(n,700)
    assert q.getd('countPhoto',2)==1 and q.rget('oldest',4)==b
    return {'repeat_after_reboot':True,'NOR_unchanged':True}

class RingRebooter:
    def __init__(self):
        from test_flash_wear import fast
        self.m=fast(RingPhoto());self.ram=bytes(self.m.u.mem_read(0x20000000,0x5000))
    def boot(self,image):
        m=self.m;m.u.mem_write(0x20000000,self.ram);m.flash[:]=image;m.io=None;m.fault=None
        m.counts={'read':0,'start':0,'poll':0};m.answers=[];m.sent=[];m.events=[];m.commands=[];m.rx_count=0;m.now=0
        m.boot_ring();return m

def photo_power():
    from test_flash_wear import fast
    from test_photo_power import cut_image
    m=fast(RingPhoto());m.boot_ring();a,n=m.capture(b'a'*100);m.ack(n,100)
    b,n2=m.capture(b'b'*129);before=bytes(m.flash);m.record_changes=True
    c,n3=m.capture(b'c'*71);image=bytearray(before);reboot=RingRebooter();cuts=0
    for kind,address,data in m.changes:
        points=range(len(data)+1) if len(data)<=64 else (0,1,len(data)//2,len(data)-1,len(data))
        for nbytes in points:
            r=reboot.boot(cut_image(image,kind,address,data,nbytes))
            committed=r.flash[c+8:c+12]==struct.pack('<I',0x33504850)
            assert r.getd('countPhoto',2)==1+int(committed),(kind,hex(address),nbytes)
            assert r.rget('oldest',4)==b
            assert r.flash[b+32:b+32+129]==b'b'*129
            cuts+=1
        image=cut_image(image,kind,address,data,len(data))
    # Полный ACK публикуется только точным нулём; обрыв не возвращает прежний ACK a.
    for ack in range(256):
        f=bytearray(before);f[b+12]=ack;r=reboot.boot(f)
        assert r.getd('countPhoto',2)==int(ack!=0)
    return {'capture_cuts':cuts,'ACK_values':256,'previous_ACK_not_resurrected':True}


def photo_rotation_power():
    from test_flash_wear import fast
    from test_photo_power import cut_image
    m=fast(RingPhoto());m.boot_ring();a,n=m.capture(b'a'*13266);m.ack(n,13266)
    m.write(m.device+m.d['newAddressPhoto'],0x7ef000)
    b,n2=m.capture(b'b'*13266)
    before=bytes(m.flash);m.record_changes=True;c,n3=m.capture(b'c'*71)
    assert c==a and any(k=='erase' and x==a for k,x,_ in m.changes)
    image=bytearray(before);reboot=RingRebooter();cuts=0
    for kind,address,data in m.changes:
        points=range(len(data)+1) if len(data)<=32 else (0,1,len(data)//2,len(data)-1,len(data))
        for nbytes in points:
            r=reboot.boot(cut_image(image,kind,address,data,nbytes))
            committed=r.flash[c+8:c+12]==struct.pack('<I',0x33504850) and r.flash[c:c+4]==struct.pack('<I',n3)
            assert r.getd('countPhoto',2)==1+int(committed),(kind,hex(address),nbytes,r.getd('countPhoto',2))
            assert r.rget('oldest',4)==b and r.flash[b+32:b+32+13266]==b'b'*13266
            cuts+=1
        image=cut_image(image,kind,address,data,len(data))
    return {'wrap_write_erase_cuts':cuts,'old_ACK_not_resurrected':True,'unacked_preserved':True}

if __name__=='__main__':
    print(photo_smoke())
    print(photo_queue())
    print(photo_erase_intent())
