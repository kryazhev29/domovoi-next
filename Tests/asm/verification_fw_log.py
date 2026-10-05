"""Late destination CRC preserves the legacy reason8 and owned status reply."""
from verification_fw_bridge import *


def run():
    elf=arg.project/'Release/Exe/DomovoiNext.out'
    sha=hashlib.sha256(elf.read_bytes()).hexdigest()
    m=FirmwareMachine();m.fixture();m.begin(1,0,True);m.until(lambda:m.phase()==13)
    m.flash[0x1200]&=0xfe;m.until(lambda:m.phase()==16)
    assert m.logs[-1]==(133,8),m.logs
    assert not m.read_word(m.device+171,1) and not m.read_word(m.addr('fw_rollback_end'))
    m.drain()
    assert len(m.sent)==2 and m.sent[-1][8:11]==bytes((4,0,253)) and largest_free(m)==4088
    assert hashlib.sha256(elf.read_bytes()).hexdigest()==sha
    return dict(status='PASS',elf_sha256=sha,late_crc_log=m.logs[-1],
        reply=m.sent[-1].hex(),rebootToUpdate=0,largest_free=4088)


if __name__=='__main__':
    result=run()
    Path(__file__).with_name('verification_fw_log_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
