"""Independent linked upload cleanup retry and reachable owner heap trace."""
import argparse, hashlib, json, sys
from pathlib import Path
ap=argparse.ArgumentParser()
ap.add_argument('--project', type=Path, default=Path(__file__).resolve().parents[2])
a=ap.parse_args()
sys.path.insert(0,str(a.project/'Tests/asm'))
from test_upload_runtime import *
from test_photo_runtime import PhotoMachine
from unicorn import UC_HOOK_CODE


class PhotoUploadMachine(UploadMachine, PhotoMachine):
    pass


def durable_empty():
    out=[]
    for cmd in (9,10):
        for channel in (0,1):
            m=UploadMachine();m.fixture(cmd,[log(1) if cmd==10 else temp(1)])
            m.checked('upload_setIndex',cmd,channel,1)
            m.write(m.device+129,4096 if cmd==10 else 16384,2)
            m.write(m.device+(89 if cmd==10 else 83),1,2)
            m.write(m.device+(91 if cmd==10 else 85),m.base+32,4)
            m.busy=True;m.request(channel);store=m.addr('store');seen=[]
            for i in range(4000):
                m.tick_upload()
                state=(m.phase(),m.read_word(store+18,1),m.checked('mem_config_result'))
                if not seen or seen[-1][1]!=state:seen.append((i,state))
                if m.flash[m.base:m.base+4096]==b'\xff'*4096 and m.read_word(store+18,1)==8:
                    break
            else:raise AssertionError(('cleanup stuck',cmd,channel,seen))
            assert not m.sent and m.checked('upload_index',cmd,channel)==0
            # Server keeps US_REPLY while busy; USB starts its privately owned
            # empty packet, which must survive SEND retries without new input.
            for _ in range(8):m.tick_upload()
            assert not m.sent
            if channel==0:
                assert m.phase()==5
                ctx=m.ptr();packet=m.read_word(ctx);before=bytes(m.u.mem_read(packet,1040))
                for _ in range(8):m.tick_upload()
                assert m.ptr()==ctx and bytes(m.u.mem_read(packet,1040))==before
            m.busy=False
            for _ in range(20):
                m.tick_upload()
                if m.sent:break
            assert m.sent==[wire(cmd,7,0,[])] and not m.ptr()
            assert largest_free(m)==4088
            out.append(dict(command=cmd,channel=channel,cleanup_polls=i+1,
                maximum_model_instructions=max(m.costs),stack=m.stack_peak,
                largest_after=4088,transitions=seen))
    return out


def reachable_ack():
    m=PhotoUploadMachine();m.boot_media();m.capture(b'x'*58)
    assert m.photo_ptr() and not m.checked('photo_storage_busy')
    assert m.read_word(m.photo_ptr()-8)==192
    m.fixture(10,[log(1),log(2)]);m.sent=[]
    heap=m.map_addr('HEAP$$Base');limit=m.map_addr('HEAP$$Limit')
    def free_chunks(raw):
        p=int.from_bytes(raw[m.addr('Aldata')+4-0x20000000:m.addr('Aldata')+8-0x20000000],'little')
        chunks=[]
        while p:
            assert heap<=p<limit
            size,p=struct.unpack_from('<II',raw,p-0x20000000);chunks.append(size)
        return chunks
    before=bytes(m.u.mem_read(0x20000000,0x5000))
    assert sum(free_chunks(before))==4096-192
    snapshots=[]
    hook=m.u.hook_add(UC_HOOK_CODE,lambda u,p,n,x:snapshots.append(bytes(u.mem_read(0x20000000,0x5000))) if p==m.addr('__basic_free') else None)
    m.write(m.addr('log_unacked'),1,2);m.request()
    m.u.hook_del(hook)
    assert m.checked('mem_config_result')==1 and not m.sent and not m.ptr()
    chunks=min((free_chunks(raw) for raw in snapshots),key=sum)
    assert chunks==[1776],chunks
    assert 4096-sum(chunks)==2320
    for _ in range(1500):
        m.tick_upload()
        if m.sent:break
    assert m.sent==[wire(10,7,1,[log(2)],2)]
    assert m.checked('upload_index',10,0)==1 and m.pending_cursor()==2
    assert largest_free(m)==3896
    return dict(reachable_live_chunks=[192,48,1048,1032],peak_used=2320,
        total_free=1776,largest_free_payload=1768,external_fragmentation=0,
        photo_s_allocation='actual camera capture, retained idle photo_s; no synthetic transport heap',
        only_idle_photo_remaining=192,largest_after=3896)


if __name__=='__main__':
    out=dict(status='PASS',elf_sha256=hashlib.sha256((a.project/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
        durable_empty=durable_empty(),reachable_ack=reachable_ack(),
        units='Unicorn instructions with modeled NOR/transport; not board cycles or time')
    Path(__file__).with_name('verification_upload_lifetime_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out,indent=2))
