"""Старая → пробная новая → старая → новая: фото и ограниченный ID без ACK."""
import argparse,sys,subprocess,json,struct
from pathlib import Path
from flash_wear_migration import select

def phase(name,path):
    from test_photo_runtime import PhotoMachine
    from test_flash_wear import fast
    from test_id_sync_fast import cell,BASE
    image=bytearray(b'\xff'*0x1000000) if name=='old' else bytearray(path.read_bytes())
    m=fast(PhotoMachine(image));m.boot_media()
    if name=='old':
        m.flash[BASE:BASE+32]=cell(1)
        a,n=m.capture(b'A'*13266);assert a==0x200000
    elif name=='trial':
        from test_counter_wear import CounterMachine
        c=CounterMachine(m.flash);c.boot();c.write(c.addr('trial'),1,1)
        value=bytearray(cell(1));value[21:29]=struct.pack('<Q',1790000000)
        c.update(1,bytes(value));assert c.flash[BASE:BASE+32]==cell(1)
        assert c.logical(1)==value
        m.flash[:]=c.flash
        a,n=m.capture(b'B'*51200);assert 0x200000<a<0x300000
        assert m.flash[0x400000:0x800000]==b'\xff'*0x400000
    elif name in ('rollback','return'):
        count=int.from_bytes(m.u.mem_read(m.device+m.d['countPhoto'],2),'little');assert count==2,count
        assert m.flash[BASE:BASE+32]==cell(1)
        assert m.flash[0x20000d:0x20000d+13266]==b'A'*13266
        second=0x204000
        assert m.flash[second+13:second+13+51200]==b'B'*51200
        # Чтение реального кадра фото 14 в старом и новом ELF.
        m.packet(21,b'');m.drain();m.port();m.drain()
        assert m.sent[-1][20:-2]==b'A'*512
        m.checked('photo_storage_cancel');m.drain()
        if name=='return':
            from test_counter_wear import CounterMachine
            c=CounterMachine(m.flash);c.boot();assert c.logical(1)[21:29]==struct.pack('<Q',1790000000)
    path.write_bytes(m.flash)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--before',required=True);p.add_argument('--after',required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--phase');a=p.parse_args()
    if a.phase:
        select(a.before if a.phase in ('old','rollback') else a.after);phase(a.phase,a.output)
    else:
        a.output.mkdir(parents=True,exist_ok=True);path=a.output/'photo-id.nor'
        for name in ('old','trial','rollback','return'):
            subprocess.run([sys.executable,__file__,'--before',a.before,'--after',a.after,'--output',str(path),'--phase',name],check=True)
        result={'old_new_trial_old_new':'PASS','photos':[13266,51200],'limited_ID':'overlay; old sees last materialized base','wire_photo_unchanged':True}
        (a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(result)
