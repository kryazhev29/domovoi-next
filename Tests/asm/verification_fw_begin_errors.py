"""Latest owned PARAM/REFUSED replies: no busy-spin or loss on TX retry."""
from verification_fw_bridge import *


def check(number,ch,status):
    m=FirmwareMachine();m.fixture()
    if status==2:
        for offset,value,size in [(164,1,1),(148,5,2),(172,2,2),(179,321,2),(174,999,4),(178,1,1)]:
            m.write(m.device+offset,value,size)
    before=bytes(m.flash);saved=bytes(m.u.mem_read(m.device,512))
    m.busy=ch==0;m.commit_fail=ch==1;m.begin(number,ch,True)
    assert m.phase()==18 and not m.sent and not any(m.counts.values())
    m.tick_fw();assert m.phase()==16
    ctx=m.ptr();wire=bytes(m.u.mem_read(ctx,19))
    assert wire[8:11]==bytes((11,19,status)) and wire[7]==7
    assert crc16(wire[4:-2])==int.from_bytes(wire[-2:],'little')
    for _ in range(5):
        m.tick_fw();assert m.ptr()==ctx and bytes(m.u.mem_read(ctx,19))==wire
        assert not m.sent and not m.lease
    m.busy=m.commit_fail=False;m.drain()
    assert m.sent==[wire] and bytes(m.flash)==before and not any(m.counts.values())
    assert bytes(m.u.mem_read(m.device,512))==saved,'reject altered an existing firmware session'
    assert largest_free(m)==4088
    return dict(number=number,channel=ch,error=status,busy_passes=5,wire_bytes=19,
                unchanged_media=True,existing_session_unchanged=True,largest_free=4088)


if __name__=='__main__':
    initial_sha=hashlib.sha256((arg.project/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    result=dict(status='PASS',cases=[check(0,0,1),check(4,1,2)],
        elf_sha256=hashlib.sha256((arg.project/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest())
    assert result['elf_sha256']==initial_sha,'ELF changed during verification'
    Path(__file__).with_name('verification_fw_begin_errors_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
