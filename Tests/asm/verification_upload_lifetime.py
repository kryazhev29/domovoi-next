from test_work_memory import owned
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
from iar_image import image_paths


class PhotoUploadMachine(UploadMachine, PhotoMachine):
    pass


def durable_empty():
    out=[]
    for cmd in (9,10):
        for channel in (0,1):
            m=UploadMachine();m.fixture(cmd,[log(1) if cmd==10 else temp(1)])
            m.set_cursor(channel,1)
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
            assert not m.sent and m.cursor(channel)==0



            for _ in range(400):
                m.tick_upload()
                if channel==0 and m.phase()==5:break
            assert not m.sent
            if channel==0:
                assert m.phase()==5
                ctx=m.ptr();packet=m.read_word(ctx);before=bytes(m.u.mem_read(packet,1040))
                for _ in range(8):m.tick_upload()
                assert m.ptr()==ctx and bytes(m.u.mem_read(packet,1040))==before
            m.busy=False
            for _ in range(400):
                m.tick_upload()
                if m.sent:break
            assert m.sent==[wire(cmd,7,0,[])] and not m.ptr(),(cmd,channel,m.phase(),m.sent)
            assert owned(m)==0
            out.append(dict(command=cmd,channel=channel,cleanup_polls=i+1,
                maximum_model_instructions=max(m.costs),stack=m.stack_peak,
                largest_after=2032,transitions=seen))
    return out


def recovery_gate():
    count=0
    for bad_state in ('not_recovered','table_fault','journal_fault'):
        for through_parser in (False,True):
            m=UploadMachine();m.fixture(10,[log(1)])
            if bad_state=='not_recovered':m.write(m.addr('mem_storage_ready'),0,1)
            if bad_state=='table_fault':m.write(m.addr('table_fault'),1,1)
            if bad_state=='journal_fault':m.write(m.addr('sjr')+59,1,1)
            assert not m.checked('flash_exec_read_allowed')
            m.seed_pending(1)
            if through_parser:
                packet=struct.pack('<HBBB',3,0,7,10)
                packet+=struct.pack('<H',crc16(packet));m.u.mem_write(0x21000000,packet)
                m.checked('user_parse_comand',0x21000000,0)
            else:m.request()
            assert not m.ptr() and not m.sent and not any(m.counts.values()),bad_state
            assert m.pending_cursor()==1 and m.cursor()==0
            assert owned(m)==0
            count+=1
    return count


def reachable_ack():
    from test_work_memory import owned,PHOTO,CONFIG,UP_CONTEXT,UP_PACKET
    m=PhotoUploadMachine();m.boot_media();m.capture(b'x'*58)
    assert m.photo_ptr() and not m.exec_active(8)
    assert owned(m)==1<<(PHOTO-1)
    m.fixture(10,[log(1),log(2)]);m.sent=[]
    released=[]
    hook=m.u.hook_add(UC_HOOK_CODE,lambda u,p,n,x:released.append(owned(m)) if p==m.addr('work_release') else None)
    m.seed_pending(1);m.request();m.u.hook_del(hook)
    assert m.checked('mem_config_result')==1 and not m.sent and not m.ptr()
    assert owned(m)==(1<<(PHOTO-1))|(1<<(CONFIG-1))
    assert any(mask&(1<<(UP_CONTEXT-1)) and mask&(1<<(UP_PACKET-1)) for mask in released)
    for _ in range(1500):
        m.tick_upload()
        if m.sent:break
    assert m.sent==[wire(10,7,1,[log(2)],2)]
    assert m.cursor()==1 and m.pending_cursor()==2
    assert owned(m)==1<<(PHOTO-1) and largest_free(m)==2032
    return dict(workspace_bytes=fields('work_memory_s')['owned']+8,retained_photo_bytes=m.checked('work_capacity',PHOTO),ack_handoff='upload reservation released before config snapshot',wire_retry='exact pending log reply')


if __name__=='__main__':
    out=dict(status='PASS',elf_sha256=hashlib.sha256(image_paths()[0].read_bytes()).hexdigest(),
        durable_empty=durable_empty(),reachable_ack=reachable_ack(),recovery_gate=recovery_gate(),
        units='Unicorn instructions with modeled NOR/transport; not board cycles or time')
    __import__('iar_image').result_path('verification_upload_lifetime_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print(json.dumps(out,indent=2))
