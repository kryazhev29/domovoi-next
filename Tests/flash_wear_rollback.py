"""Новый ELF пишет без ACK; тот же NOR читает старый ELF после отката пробы."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
from flash_wear_migration import select, capture


def start(path):
    from test_flash_wear import fast
    from test_upload_runtime import UploadMachine
    from test_stage5_log import boot
    import struct
    m=fast(UploadMachine());m.fixture(9,[]);m.flash[:]=path.read_bytes()
    m.u.mem_write(m.addr('mem')+6*8,struct.pack('<II',0x1cd400,4096))
    m.select_config()
    read=m.stubs[m.addr('w25_read_data')]
    def boot_read():
        a,p,n,_=m.args();assert 0<n<=512 and a+n<=len(m.flash)
        m.u.mem_write(p,bytes(m.flash[a:a+n]));m.ret(1)
    m.stubs[m.addr('w25_read_data')]=boot_read
    m.checked('user_readconfig',budget=2000000)
    m.checked('user_readsavedinformation',budget=2000000)
    m.stubs[m.addr('w25_read_data')]=read
    boot(m);m.write(m.device+m.d['tempIDsCount'],m.checked('user_getTempIDRecordsCount',budget=2000000),2)
    return m


def produce(path,partial=False):
    from test_stage5_log import append
    from test_flash_wear import run_config
    from storage_machine import fields
    from unicorn import UC_HOOK_CODE
    m=start(path);m.write(m.addr('trial'),1,1)
    # Хвост курсоров новой версии есть, но новые события ниже не подтверждаются.
    end=m.read_word(m.addr('log_lastIndex'))+1
    if m.read_word(m.addr('log_cursor32'))<end:
        m.checked('log_ring_sent',0,40,end);m.checked('log_ring_ack',0,41)
        run_config(m);m.checked('log_ring_ack_poll')
        for _ in range(300):m.tick_upload()
    else:
        assert m.checked('store_cursor_snapshot');run_config(m)
        m.checked('mem_config_reset');m.checked('flash_exec_finish',2);m.checked('flash_exec_take_result')
    assert m.read_word(m.addr('log_cursor32')+4)==end
    for i in range(61,66):append(m,i)
    # Реальные SMS #add и авторегистрация; перехвачены только SMS TX и выход реле.
    for name in ('user_sendsms','user_activateOut'):
        address=m.addr(name);m.stubs[address]=lambda:m.ret(1)
        m.u.hook_add(UC_HOOK_CODE,m.hook,begin=address,end=address)
    m.write(m.addr('config')+fields('config_s')['pinCode'],4321)
    m.u.mem_write(0x21000400,b'79990000000\0')
    m.u.mem_write(0x21000500,b'#add 4321,0000000102\0');m.u.mem_write(0x21000600,b'\0')
    assert m.checked('user_smsreceived',0x21000400,0x21000500,0x21000600)
    m.write(m.device+m.d['status'],1<<5,2)
    m.u.mem_write(0x21000400,b'0000000103\0')
    m.checked('user_checkIdAndControl',0x21000400,2,budget=2000000)
    for i in range(104,106):
        m.u.mem_write(0x21000400,f'{i:010d}'.encode()+b'\0')
        assert m.checked('user_addIdToTempBuffer',0x21000400,0)
    m.write(m.addr('store_runtime'),0,1);m.checked('log_flushPending',budget=2000000)
    assert m.read_word(m.addr('log_pendingCount'),1)==0
    assert m.checked('app_trial_active')
    if partial:
        # Тело проверено, оборвался первый бит финального commit последнего ID.
        m.flash[0x1e8400+104*32:0x1e8402+104*32]=b'\xfe\xff'
    path.write_bytes(m.flash)


def events(image,first):
    from log_ring_fixture import valid
    cells=[bytes(image[a:a+32]) for a in range(0x1d0000,0x1e8000,32)]
    return sorted((c for c in cells if valid(c) and int.from_bytes(c[:4],'little')>=first),key=lambda c:int.from_bytes(c[:4],'little'))


def verify(path):
    from test_upload_runtime import log, wire
    m=start(path);before=bytes(m.flash)
    assert bytes(m.u.mem_read(m.device+m.d['reserv']+32,8))==bytes.fromhex('53483136acb7cec9')
    assert bytes(m.u.mem_read(m.device+m.d['IMEIGSM'],16))==b'123456789012345\0'
    from storage_machine import fields
    intent=m.addr('store')+fields('store_state_s')['intent']
    if m.read_word(intent,1):
        for _ in range(16000):
            m.tick_upload()
            if not m.read_word(intent,1) and m.exec_state()==(0,0,0):break
        first_new=int.from_bytes(events(before,0)[-7][:4],'little')
        raise AssertionError(('rollback resumed legacy erase',len(events(m.flash,first_new)),
                              'new unacknowledged events remain'))
    # Старый A/B не учитывает хвост ACK: повтор допустим, новые записи обязательны.
    first_temp=m.read_word(m.device+m.d['readTempIDIndex'],2)
    m.request(1,70);m.drain_upload()
    assert m.sent==[wire(9,70,0,[b'\0\0'+f'{i:010d}'.encode()+bytes(20) for i in range(first_temp+1,106)])]
    m.sent.clear();m.cmd=10;m.base=0x1d0000
    m.request(1,71);m.drain_upload()
    first=m.read_word(m.addr('log_cursor32')+4)
    expected=events(before,first)
    assert m.sent==[wire(10,71,0,expected[:32],len(expected))]
    if len(expected)>32:
        m.sent.clear();m.request(1,72)
        for _ in range(16000):
            m.tick_upload()
            if m.sent:break
        assert m.sent==[wire(10,72,0,expected[32:])]
    # Первый ACK старого ELF может стереть только A/B, журнал ещё неподтверждён.
    assert m.flash[0x1d0000:0x1f5000]==before[0x1d0000:0x1f5000]
    assert m.flash[:0x1d0000]==before[:0x1d0000], 'base/firmware changed'
    active=m.select_config()
    assert m.flash[active:active+512]==bytes(m.u.mem_read(m.addr('config'),512))
    # Старая прошивка дописывает свои записи; следующий старт новой читает оба формата.
    from test_stage5_log import append
    append(m,68)
    m.u.mem_write(0x21000400,b'0000000106\0')
    assert m.checked('user_addIdToTempBuffer',0x21000400,0)
    path.with_name('return-new.nor').write_bytes(m.flash)
    return {'status':'PASS','new_log_events':[int.from_bytes(c[:4],'little') for c in events(before,0)[-7:]],
            'new_temp_ids':[102,103,104,105],'old_log_repeats':max(0,len(expected)-7),
            'pending_data_erase':False,'nor_sha256':hashlib.sha256(before).hexdigest()}


def return_new(path):
    from test_upload_runtime import log,wire
    m=start(path);before=bytes(m.flash)
    assert bytes(m.u.mem_read(m.device+m.d['reserv']+32,8))==bytes.fromhex('53483136acb7cec9')
    assert bytes(m.u.mem_read(m.device+m.d['IMEIGSM'],16))==b'123456789012345\0'
    m.request(1,80);m.drain_upload()
    assert m.sent[0][12:-2]==b''.join(f'{i:010d}'.encode() for i in range(m.cursor(1)+1,107))
    m.cmd=10;m.base=0x1d0000;m.sent.clear();m.request(1,81);m.drain_upload()
    assert m.sent==[wire(10,81,0,events(before,m.read_word(m.addr('log_cursor32')+4)))]
    assert bytes(m.flash)==before


def main():
    p=argparse.ArgumentParser();p.add_argument('--before',required=True);p.add_argument('--after',required=True)
    p.add_argument('--clear-intent',type=int,choices=(0,9,10),default=0);p.add_argument('--partial-commit',action='store_true')
    p.add_argument('--output',type=Path,required=True);p.add_argument('--phase',choices=('capture','produce','verify','return'))
    a=p.parse_args()
    if a.phase:
        select(a.after if a.phase in ('produce','return') else a.before)
        if a.phase=='capture':
            capture(a.output)
            from test_memory_runtime import run_config
            from storage_machine import fields,struct
            m=start(a.output)
            m.u.mem_write(m.device+m.d['reserv']+32,struct.pack('<II',0x36314853,0xc9ceb7ac))
            m.u.mem_write(m.device+m.d['IMEIGSM'],b'123456789012345\0')
            m.write(m.addr('config')+fields('config_s')['pinCode'],4321)
            if a.clear_intent:
                if a.clear_intent==10:
                    from test_stage5_log import append
                    for i in range(61,201):append(m,i)
                    m.write(m.addr('log_cursor32'),201);m.write(m.addr('log_cursor32')+4,201)
                    m.checked('upload_store')
                else:
                    m.write(m.device+m.d['readTempIDIndex'],101,2);m.write(m.addr('temp_usbReadIndex'),101,2)
                    m.checked('upload_store')
                m.checked('store_marker',a.clear_intent,0x1e8000 if a.clear_intent==10 else 0x1e8400+101*32)
            assert m.checked('store_snapshot');run_config(m)
            assert m.checked('mem_config_result')==2
            if a.clear_intent==10:m.flash[0x1d0000:0x1d1000]=b'\xff'*4096
            a.output.write_bytes(m.flash)
        elif a.phase=='produce':produce(a.output,a.partial_commit)
        elif a.phase=='return':return_new(a.output.with_name('return-new.nor'))
        else:
            result=verify(a.output)
            a.output.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        return
    a.output.mkdir(parents=True,exist_ok=True);image=a.output/'rollback.nor'
    for phase in ('capture','produce','verify','return'):
        subprocess.run([sys.executable,__file__,'--before',a.before,'--after',a.after,
                        '--output',str(image),'--phase',phase,'--clear-intent',str(a.clear_intent)]+(['--partial-commit'] if a.partial_commit else []),check=True)
    print(image.with_suffix('.json').read_text())


if __name__=='__main__':main()
