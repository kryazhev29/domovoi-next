from test_work_memory import owned
"""Independent command19 ownership bridge, reply order and abort audit in ELF."""
import argparse,hashlib,json,sys
from pathlib import Path
ap=argparse.ArgumentParser();ap.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[2]);arg=ap.parse_args()
sys.path.insert(0,str(arg.project/'Tests/asm'))
from test_fw_service_runtime import *


def assert_no_early_fetch(m):
    acquires=m.acquires;sent=list(m.sent)
    assert not m.checked('user_getFWblockFromServer',1,0)
    assert m.acquires==acquires and m.sent==sent,'request13 preceded owned command19 reply'


def ack_order():
    out=[]
    for number in (1,4):
        for ch in (0,1):
            m=FirmwareMachine();m.fixture();m.begin(number,ch,True)
            phases=set();m.busy=ch==0;m.commit_fail=ch==1
            for i in range(1500):
                phase=m.phase();phases.add(phase)
                assert_no_early_fetch(m)
                if phase==8 and m.read_word(m.addr('store')+fields('store_state_s')['intent'],1):
                    from test_flash_executor import state, CONFIG, FWSERVICE, PENDING, OK
                    kind, step, result = state(m)
                    config_result = m.checked('mem_config_result')
                    if config_result:
                        assert (kind, result) == (CONFIG, PENDING if config_result == 1 else OK)
                    else:
                        assert (kind, result) == (FWSERVICE, PENDING)
                        assert m.read_word(m.addr('store')+fields('store_state_s')['intent'],1) == 8
                    assert m.checked('fw_service_busy')
                if phase==16:break
                m.tick_fw()
            else:raise AssertionError(('ACK never ready',number,ch))
            pointer=m.ptr();wire=bytes(m.u.mem_read(pointer,18));assert wire[8:10]==bytes((11,0))
            for _ in range(4):
                m.tick_fw();assert_no_early_fetch(m)
                assert m.ptr()==pointer and bytes(m.u.mem_read(pointer,18))==wire and not m.sent
                if number==1:assert m.flash[0x1000:0x1020]==b'\x5a'*32
            m.busy=m.commit_fail=False;m.drain()
            assert m.sent==[wire] and not m.ptr() and owned(m)==0
            out.append(dict(number=number,channel=ch,setup_phases=sorted(phases),config_and_service_kind=True))
    return out


def config_failure():
    out=[]
    for ch in (0,1):
        m=FirmwareMachine();m.fixture();m.begin(4,ch,True);m.until(lambda:m.phase()==8)
        assert_no_early_fetch(m)
        m.fault=('poll',m.counts['poll']+1);m.until(lambda:m.phase()==16)
        assert bytes(m.u.mem_read(m.ptr()+9,2))==bytes((19,2))
        for offset,size in ((164,1),(148,2),(172,2),(174,4),(179,2)):
            assert not m.read_word(m.device+offset,size),(ch,offset)
        assert not m.read_word(m.addr('fw_write_errors'),1)
        assert not m.read_word(m.addr('fw_rollback_end'))
        assert_no_early_fetch(m);m.drain()
        assert len(m.sent)==1 and m.sent[0][8:10]==bytes((11,2))
        assert owned(m)==0
        out.append(dict(channel=ch,FWinProgress=0,blocks=0,index=0,timeout=0,answer='RUNERROR'))
    return out


def quiet_cases():
    count=0
    for number in (1,4):
        for ch in (0,1):
            for phase in (8,16):
                m=FirmwareMachine();m.fixture();m.begin(number,ch,True);m.until(lambda:m.phase()==phase)
                m.checked('fw_service_quiet',1-ch)
                assert not m.read_word(m.ptr()+fields('fw_service_s')['quiet'],1),'other session muted this reply'
                if ch:m.checked('user_comm_session',0)
                else:m.write(m.addr('usb_bus_resets'),1);m.tick_fw()
                m.drain();assert not m.sent and not m.ptr() and m.io is None
                if number==1:
                    assert m.flash[0x1200:0x1200+len(m.image)]==m.image
                    assert m.read_word(m.device+171,1)==1
                assert owned(m)==0;count+=1
    return count


def abort_cases():
    count=0
    for number,phases in ((4,(4,5,6,8,16)),(1,(1,2,8,16,10,11,12,13,14))):
        for phase in phases:
            m=FirmwareMachine();m.fixture();m.begin(number,1,True);m.until(lambda:m.phase()==phase)
            if phase in (4,5,10,12):m.tick_fw();assert m.io is not None
            was_pending=m.io is not None
            m.checked('fw_cancel',0)
            if was_pending:assert m.ptr(),'pending source freed before driver drained'
            m.until(lambda:not m.ptr())
            assert m.io is None and not m.checked('fw_service_busy')
            assert not m.read_word(m.device+164,1) and not m.read_word(m.addr('fw_rollback_end'))
            if phase<10 or phase==16:assert not m.sent

            if m.checked('mem_config_result')==1:
                m.until(lambda:m.checked('mem_config_result') in (2,3))
            count+=1
    for phase in (8,12):
        m=FirmwareMachine();m.fixture();m.begin(1,0,True);m.until(lambda:m.phase()==phase)
        if phase==12:m.tick_fw();assert m.io
        m.checked('user_savereset');m.until(lambda:not m.ptr())
        assert m.io is None and m.read_word(m.addr('store_reset_requested'),1)
        if phase==8:m.until(lambda:m.checked('mem_config_result') in (2,3))
        count+=1
    return count


if __name__=='__main__':
    result=dict(status='PASS',ack_order=ack_order(),config_failure=config_failure(),
        quiet_reconnect_usb=quiet_cases(),abort_reset=abort_cases(),
        elf_sha256=hashlib.sha256((__import__('iar_image').image_paths()[0]).read_bytes()).hexdigest(),
        scope='Real linked callers and storage scheduler; modeled NOR/transport. No board timing.')
    __import__('iar_image').result_path('verification_fw_bridge_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
