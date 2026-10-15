"""Образ NOR, записанный старым ELF: частичные ACK 9/10 и старт нового ELF."""
import argparse, hashlib, json, os, subprocess, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'Tests'),str(ROOT/'Tests/asm')]


def select(root):
    for name,folder,ext in (('ELF','ExePath','out'),('BIN','ExePath','bin'),('MAP','ListPath','map')):
        os.environ['DOMOVOI_'+name]=str(Path(root)/'ReleaseCustomer'/folder/('DomovoiNext.'+ext))


def capture(path):
    from test_upload_runtime import UploadMachine, log, wire
    from test_stage5_log import append
    from test_flash_wear import fast
    m=fast(UploadMachine());m.fixture(10,[])
    for i in range(1,61):append(m,i)
    m.request(1,10);m.drain_upload()
    assert m.sent==[wire(10,10,0,[log(i) for i in range(1,33)],60)]
    m.sent.clear();m.request(1,11)
    for _ in range(16000):
        m.tick_upload()
        if m.sent:break
    assert m.read_word(m.addr('log_cursor32')+4)==33
    m.fixture(9,[]);m.sent.clear()
    m.write(m.device+m.d['newAddressTempID'],m.base)
    for i in range(1,102):
        m.u.mem_write(0x21000400,f'{i:010d}'.encode()+b'\0')
        assert m.checked('user_addIdToTempBuffer',0x21000400,0)
    m.request(1,20);m.drain_upload();assert m.sent[0][11]==100
    m.sent.clear();m.request(1,21)
    for _ in range(16000):
        m.tick_upload()
        if m.sent:break
    assert m.read_word(m.device+m.d['readTempIDIndex'],2)==100
    path.write_bytes(m.flash)


def verify(path):
    from test_flash_wear import restored
    from test_upload_runtime import log,wire
    image=path.read_bytes();m=restored(image,True)
    assert m.cursor(1)==100 and m.read_word(m.addr('log_cursor32')+4)==33
    m.request(1,30);m.drain_upload()
    assert m.sent[0][12:-2]==b'0000000101'
    m.sent.clear();m.cmd=10;m.base=0x1d0000
    m.request(1,31);m.drain_upload()
    assert m.sent==[wire(10,31,0,[log(i) for i in range(33,61)])]
    assert bytes(m.flash)==image,'migration changed old media before an ACK'
    return {'status':'PASS','log_unacked':28,'temporary_unacked':1,
            'flash_bytes':len(image),
            'old_log_cursor':33,'old_temp_cursor':100,'migration_erases':0,
            'old_nor_sha256':hashlib.sha256(image).hexdigest()}


def main():
    p=argparse.ArgumentParser();p.add_argument('--before');p.add_argument('--after')
    p.add_argument('--flash-mib',type=int,choices=(2,4,16))
    p.add_argument('--output',type=Path,required=True);p.add_argument('--capture',action='store_true')
    a=p.parse_args()
    if a.flash_mib:os.environ['DOMOVOI_FLASH_BYTES']=str(a.flash_mib*1024*1024)
    if a.capture:
        select(a.before);capture(a.output);return
    a.output.mkdir(parents=True,exist_ok=True);image=a.output/'a5b776c-partial-acks.nor'
    subprocess.run([sys.executable,__file__,'--capture','--before',a.before,'--output',str(image)],check=True)
    select(a.after);result=verify(image)
    (a.output/'migration.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))


if __name__=='__main__':main()
