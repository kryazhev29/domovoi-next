"""Счётчик NOR: решения кольца/ACK в ELF, ускорено копирование payload камеры."""
import argparse,json,sys,time,math
from pathlib import Path
from collections import Counter
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from firmware_suite import select_image
from test_flash_wear import fast
from test_flash_wear2 import RingPhoto

def run(root,events,size,active=0,baseline=False):
    select_image(root)
    counts=Counter();started=time.monotonic()
    def attach(m):
        original=m.change
        def change(kind,address,data):
            if kind=='erase':counts[address]+=1
            original(kind,address,data)
        m.change=change
        return fast(m)
    m=attach(RingPhoto())
    if baseline: m.boot_media();m.port();m.drain()
    else: m.boot_ring()
    if active:
        from test_counter_wear import CounterMachine
        from test_id_sync_fast import cell,BASE,SIZE
        m.write(m.addr('mem')+20,SIZE)
        for k in range(1,active+1):m.flash[BASE+32*(k-1):BASE+32*k]=cell(k)
        m.write(m.addr('id_used_bytes'),active*32);m.write(m.addr('id_cells_end'),active,2)
        if not baseline: assert m.checked('counter_boot',budget=10000000)

    for i in range(events):
        if active:
            import struct
            # После заполнения один горячий ID + 1023 холодных состояния: копирование при GC.
            k=i+1 if i<active else 1
            value=bytearray(cell(k));value[21:29]=struct.pack('<Q',i+1)
            m.logical=CounterMachine.logical.__get__(m);m.flash_ready=CounterMachine.flash_ready.__get__(m)
            CounterMachine.update(m,k,bytes(value))
        address,number=m.capture_policy(size)
        m.ack(number,size)
        if baseline:
            for _ in range(100):
                m.port();m.drain()
                if not m.rget('cleanup'):break
            assert not m.rget('cleanup')
        assert m.getd('countPhoto',2)==0
        if (i+1)%128==0:
            # Новый Unicorn с тем же RAM/NOR: избегаем накопления native-cache;
            # это НЕ перезапуск прошивки и не реконструкция её курсоров моделью.
            ram=bytes(m.u.mem_read(0x20000000,0x5000));now=m.now
            n=attach(RingPhoto(m.flash));n.u.mem_write(0x20000000,ram);n.now=now;m=n
    maximum=max(counts.values(),default=0)
    rate=maximum*1000/events
    return {'baseline':baseline,'events':events,'photo_bytes':size,'counter_records':active,'erases':{hex(k):v for k,v in sorted(counts.items())},
            'max_sector_erases':maximum,'max_per_1000':rate,
            'years_300_per_day':100000/(rate*.3)/365 if rate else None,
            'years_1000_per_day':100000/rate/365 if rate else None,
            'seconds':round(time.monotonic()-started,3),
            'payload_acceleration':True,'policy_and_ack':'IAR ELF'}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build-root',type=Path,required=True)
    p.add_argument('--events',type=int,default=10000);p.add_argument('--size',type=int,default=13266)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--active',type=int,default=0);p.add_argument('--baseline',action='store_true');a=p.parse_args()
    result=run(a.build_root,a.events,a.size,a.active,a.baseline)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print({k:v for k,v in result.items() if k!='erases'},flush=True)
    assert a.baseline or result['max_sector_erases']<=math.ceil(a.events/(256//math.ceil((a.size+13)/4096)))+2,'wear threshold exceeded'
