"""Доработка: Кряжев А., 2026. Обрывы NOR при записи, ACK, очистке и переходе.
Проверяется машинный код выбранного IAR ELF; плата и сервер не используются.
Результат относится к модели аппаратных границ.
"""
import json,struct
from test_photo_runtime import *

# Смоделировать обрыв посреди программирования или стирания NOR.
def cut_image(flash,kind,a,b,n):
    cut=bytearray(flash)
    for i,x in enumerate(b[:n]):cut[a+i]=x if kind=='erase' else cut[a+i]&x
    return cut

class Rebooter:
    # Подготовить модель и перехватить аппаратные границы машинного кода.
    def __init__(self):
        self.m=PhotoMachine();self.ram=bytes(self.m.u.mem_read(0x20000000,0x5000))
    # Перезапустить машинный код с исходной ОЗУ и заданным содержимым NOR.
    def boot(self,flash):
        m=self.m;m.u.mem_write(0x20000000,self.ram);m.flash[:]=flash;m.io=None;m.fault=None
        m.costs=[];m.counts={'read':0,'start':0,'poll':0};m.answers=[];m.sent=[];m.events=[];m.commands=[];m.rx_count=0;m.now=0
        m.boot_media();return m

# Перезапустить модель в точках обрыва записи и сохранить предыдущее фото.
def capture_cuts():
    initial=prefix_flash();initial[0x201000:0x202000]=b'\x55'*4096
    m=PhotoMachine(initial);m.boot_media();m.record_changes=True;m.capture(b'z'*129)
    flash=bytearray(initial);reboot=Rebooter();old=bytes(initial[0x200000:0x201000]);count=0
    for kind,a,b in m.changes:
        points=range(len(b)+1) if len(b)<=64 else (0,1,4,8,12,13,64,4095,4096)
        for n in points:
            r=reboot.boot(cut_image(flash,kind,a,b,n))
            assert r.flash[0x200000:0x201000]==old,(kind,hex(a),n)
            committed=r.flash[0x201008:0x20100c]==struct.pack('<I',FORMAT)
            number=int.from_bytes(r.u.mem_read(r.device+r.d['countPhoto'],2),'little')
            assert number==1+int(committed),(kind,hex(a),n,number)
            if committed:assert r.flash[0x201000+HEAD:0x201000+HEAD+129]==b'z'*129
            # После обрыва недописанный сектор можно занять следующим снимком.
            if n in (0,len(b)):
                r.capture(b'q'*59)
                assert r.flash[0x200000:0x201000]==old
            count+=1
        flash=cut_image(flash,kind,a,b,len(b))
    print('capture',count,'cuts PASS',flush=True);return {'cuts':count}

# Проверить все значения повреждённого ACK и потерю частичного курсора при сбросе.
def acknowledgement_cuts():
    m=PhotoMachine();m.boot_media();address,stamp=m.capture(b'a'*700);initial=bytes(m.flash)
    m.record_changes=True;m.packet(21,struct.pack('<IH',stamp,512));m.drain()
    assert not m.changes,'partial ACK must not write flash'
    r=Rebooter().boot(m.flash);r.packet(21,struct.pack('<IH',0,0));r.drain()
    assert r.pget('offset',2)==0
    r.packet(21,struct.pack('<IH',stamp,512));r.drain()
    assert r.pget('offset',2)==512
    m.changes=[];m.packet(21,struct.pack('<IH',stamp,700));m.drain()
    assert len(m.changes)==1 and m.changes[0][1]==address+HEAD-1
    reboot=Rebooter();count=0
    # Сбой битов единственного байта ACK: только точный ноль подтверждает фото.
    for ack in range(256):
        f=bytearray(initial);f[address+HEAD-1]=ack;r=reboot.boot(f)
        assert int.from_bytes(r.u.mem_read(r.device+r.d['countPhoto'],2),'little')==int(ack!=0)
        assert r.flash[address+HEAD:address+HEAD+700]==b'a'*700
        count+=1
    print('ACK',count,'bit cuts PASS',flush=True);return count

# Довести отложенную очистку до устойчивого пустого состояния.
def clean(m):
    m.port();m.drain()
    assert m.rget('initialized') and not m.rget('fault')

def cleanup_cuts():
    # Пустая очередь сохраняет курсор и байты ACK без единой записи.
    initial=prefix_flash(dirty=False);initial[0x200000+HEAD-1]=0
    m=PhotoMachine(initial);m.boot_media();m.record_changes=True;clean(m)
    assert not m.changes and m.flash==initial
    head=int.from_bytes(m.u.mem_read(m.device+m.d['newAddressPhoto'],4),'little')
    assert head==0x201000
    from test_flash_wear2 import photo_rotation_power
    return photo_rotation_power()

if __name__=='__main__':
    out={fn.__name__:fn() for fn in (capture_cuts,acknowledgement_cuts,cleanup_cuts)}
    print(json.dumps(out));print('PASS photo power')
