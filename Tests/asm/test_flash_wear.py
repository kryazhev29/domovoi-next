"""Дописанные ACK и миграция текущего формата: IAR ELF + NOR."""
import struct
from test_upload_runtime import UploadMachine, log, wire, temp
from test_memory_runtime import run_config
from test_stage5_log import boot, append, records


def fast(m):
    from unicorn import UC_HOOK_CODE
    m.u.hook_del(m.code_hook)
    for address in m.stubs:m.u.hook_add(UC_HOOK_CODE,m.hook,begin=address,end=address)
    return m


def seed():
    m=fast(UploadMachine());m.fixture(10,[log(1),log(2),log(3)]);boot(m)
    assert m.checked('store_snapshot')
    run_config(m);assert m.checked('mem_config_result')==2
    m.checked('mem_config_reset');m.checked('flash_exec_finish',2);m.checked('flash_exec_take_result')
    return m


def restored(image,temporary=False):
    m=fast(UploadMachine());m.flash[:]=image;m.cmd=10;m.base=0x1d0000
    if temporary:m.fixture(9,[])
    active=m.select_config();assert active in (0x1f5000,0x1f7000)
    m.u.mem_write(0x21000000,bytes(m.flash[active+512:active+1024]))
    assert m.checked('mem_cursor_restore',0x21000000,budget=2000000)
    m.checked('upload_restore',0x21000000);boot(m)
    if temporary:m.checked('user_getTempIDRecordsCount',budget=2000000)
    return m


def ack_trace(m,next_index=3):
    before=bytes(m.flash);m.record_changes=True;m.changes.clear()
    m.checked('log_ring_sent',1,40,next_index)
    assert not m.checked('log_ring_ack',1,41)
    run_config(m);assert m.checked('mem_config_result')==2
    return before,list(m.changes)


def cursor(m):return m.read_word(m.addr('log_cursor32')+4)


def ack_power():
    result={}
    for rollover in (False,True):
        m=seed()
        if rollover:m.write(m.addr('cfg_cursor_offset'),4096,2)
        before,changes=ack_trace(m);image=bytearray(before);cuts=0
        for kind,a,data in changes:
            positions=range(len(data)+1) if len(data)<=16 else sorted({0,1,len(data)//2,len(data)-1,len(data)})
            for n in positions:
                torn=bytearray(image)
                for i,value in enumerate(data[:n]):torn[a+i]=value if kind=='erase' else torn[a+i]&value
                r=restored(torn);got=cursor(r)
                assert got in (0,3),(rollover,kind,hex(a),n,got)
                assert records(r)==[1,2,3], 'ACK must not erase events'
                # После устойчивой фиксации подтверждённое больше не выдаётся.
                r.request(1,90);r.drain_upload()
                expected=[log(i) for i in (1,2,3) if i>=got]
                assert r.sent==[wire(10,90,0,expected)],(got,r.sent)
                cuts+=1
            for i,value in enumerate(data):image[a+i]=value if kind=='erase' else image[a+i]&value
        r=restored(image);assert cursor(r)==3
        assert sum(k=='erase' for k,_,_ in changes)==(2 if rollover else 0)
        result['rollover' if rollover else 'append']=dict(cuts=cuts,commands=len(changes))
    return result


def migration():
    m=seed();m.write(m.addr('log_cursor32')+4,2);m.checked('upload_store')
    assert m.checked('store_snapshot');run_config(m);m.checked('mem_config_reset')
    m.checked('flash_exec_finish',2);m.checked('flash_exec_take_result')
    image=bytes(m.flash);r=restored(image)
    assert bytes(r.flash)==image and cursor(r)==2 and records(r)==[1,2,3]
    r.request(1,7);r.drain_upload();assert r.sent==[wire(10,7,0,[log(2),log(3)])]
    # Старое намерение полной очистки журнала не стирает остатки после обновления.
    active=r.select_config();saved=bytearray(r.flash[active+512:active+1024]);off=r.d['reserv']
    # Значение маркера из публичного формата STORE.
    from pathlib import Path
    import re
    header=(Path(__file__).resolve().parents[2]/'User/c/user_logic_internal.h').read_text(encoding='utf-8')
    mark=int(re.search(r'#define STORE_CLEAR_MARK\s+(0x[0-9A-Fa-f]+)',header)[1],16)
    struct.pack_into('<II',saved,off+8,mark|10,0x1e8000)
    r.u.mem_write(0x21000000,bytes(saved));r.checked('store_restore',0x21000000)
    assert bytes(r.flash)==image and records(r)==[1,2,3]
    return {'current_AB_and_ring':'preserved','legacy_clear_intent':'no erase','unacked':[2,3]}


def wear():
    # Малый регрессионный прогон выполняется также в мутированных ELF.
    m=seed();m.fixture(10,[log(i) for i in range(1,21)]);boot(m);m.record_changes=True
    for i in range(8):
        m.checked('log_ring_sent',1,2*i,i+2)
        m.checked('log_ring_ack',1,2*i+1);run_config(m)
        # Запускаем штатное продолжение и освобождаем ответ.
        m.checked('log_ring_ack_poll')
        for _ in range(300):
            m.tick_upload()
            if not m.ptr() and not m.checked('mem_config_result'):break
        m.checked('store_finish',10)
        for _ in range(700):
            m.tick_upload()
            if m.exec_state()==(0,0,0) and not m.checked('mem_config_result'):break
    assert not any(k=='erase' for k,_,_ in m.changes),'wear: ACK/completion caused erase'
    return {'ACKs':8,'erases':0}


def mutations():
    from test_flash_mutations import run
    return run({
        'ACK_AB':('wear',[('User/c/user_store.c','bool store_cursor_snapshot(void) { return store_save(true); }','bool store_cursor_snapshot(void) { upload_store(); return store_save(false); }')]),
        'BULK_CLEAR':('wear',[('User/c/user_store.c','  if (cmd == 10) device.status.readyLogData = 0;','  if (cmd == 10) { for(uint32_t a=mem[MB_LOG].start;a<mem[MB_LOG].start+mem[MB_LOG].size;a+=4096) mem_erase_sector(a); device.status.readyLogData = 0; }')]),
    },'test_flash_wear','wear-mutations')


def temp_power():
    from storage_machine import StorageMachine
    m=fast(UploadMachine());m.fixture(9,[])
    m.write(m.device+m.d['newAddressTempID'],m.base)
    m.u.mem_write(0x21000000,b'1234567890\0')
    before=bytes(m.flash);m.record_changes=True
    assert m.checked('user_addIdToTempBuffer',0x21000000,0,budget=2000000)
    changes=[c for c in m.changes if c[0]=="write"];assert [len(b) for _,_,b in changes]==[30,2]
    image=bytearray(before);cuts=0
    for kind,a,data in changes:
        for n in range(len(data)+1):
            torn=bytearray(image)
            for i,value in enumerate(data[:n]):torn[a+i]&=value
            r=fast(UploadMachine());r.fixture(9,[]);r.flash[:]=torn
            count=r.checked('user_getTempIDRecordsCount',budget=2000000)
            assert count==(1 if len(data)==2 and n==2 else 0),(len(data),n,count)
            r.u.mem_write(0x21000000,b'abcdefghij\0')
            assert r.checked('user_addIdToTempBuffer',0x21000000,0)
            r.request(1,7);r.drain_upload()
            payload=r.sent[0][12:-2]
            assert payload==(b'1234567890' if count else b'')+b'abcdefghij'
            cuts+=1
        for i,value in enumerate(data):image[a+i]&=value
    return {'cuts':cuts,'legacy_IDs':'same wire','torn_slots':'skipped'}


def both_channels():
    m=seed();m.checked('log_ring_sent',1,40,4);m.checked('log_ring_ack',1,41)
    run_config(m);m.checked('log_ring_ack_poll')
    for _ in range(300):
        m.tick_upload()
        if not m.ptr():break
    assert m.read_word(m.addr('log_cursor32'))==4
    r=restored(m.flash);r.request(0,7);r.drain_upload();assert r.sent==[wire(10,7,0,[])]
    return {'complete_upload':'both channels retired, as with old erase'}


def temp_ring():
    # Полный старый линейный образ: без ACK отказ, после ACK ленивый первый сектор.
    m=fast(UploadMachine());m.fixture(9,[temp(i+1) for i in range(1536)])
    m.u.mem_write(0x21000000,b'abcdefghij\0');before=bytes(m.flash)
    assert not m.checked('user_addIdToTempBuffer',0x21000000,0,budget=2000000)
    assert bytes(m.flash)==before,'unacknowledged IDs overwritten'
    m.checked('temp_set_index',1,1536);m.checked('temp_set_index',0,1536)
    m.record_changes=True
    assert m.checked('user_addIdToTempBuffer',0x21000000,0,budget=2000000)
    assert [a for k,a,_ in m.changes if k=='erase']==[0x1e8000]
    m.request(1,7);m.drain_upload();assert m.sent[0][12:-2]==b'abcdefghij'
    assert m.pending_cursor(1)==2049
    # Переход 5-битного поколения через 31 -> 0, включая отказ полного кольца.
    r=fast(UploadMachine());r.fixture(9,[])
    r.write(r.addr('temp_next'),0xfe00,2)
    r.checked('temp_set_index',0,0xfe00);r.checked('temp_set_index',1,0xfe00)
    r.u.mem_write(0x21000000,b'0123456789\0');assert r.checked('user_addIdToTempBuffer',0x21000000,0)
    r.request(1,8);r.drain_upload();assert r.sent[0][12:-2]==b'0123456789' and r.pending_cursor(1)==1
    return {'legacy_full':1536,'unacked_full':'refused unchanged','first_reclaim_erases':1,'epoch_31_to_0':'PASS'}


def rotation_power():
    result={}
    for temporary in (False,True):
        m=seed()
        if temporary:
            m.fixture(9,[temp(i+1) for i in range(1536)])
            m.checked('temp_set_index',0,96);m.checked('temp_set_index',1,96)
        else:
            m.fixture(10,[log(i+1) for i in range(3048)]);boot(m)
            m.write(m.addr('log_cursor32'),128);m.write(m.addr('log_cursor32')+4,128)
        m.checked('upload_store');assert m.checked('store_snapshot');run_config(m)
        m.checked('mem_config_reset');m.checked('flash_exec_finish',2);m.checked('flash_exec_take_result')
        before=bytes(m.flash);m.record_changes=True;m.changes.clear()
        if temporary:
            m.u.mem_write(0x21000000,b'abcdefghij\0');assert m.checked('user_addIdToTempBuffer',0x21000000,0,budget=2000000)
        else:append(m,3049)
        image=bytearray(before);cuts=0
        for kind,a,data in m.changes:
            points=range(len(data)+1) if len(data)<=32 else sorted(set(range(0,len(data)+1,64))|{1,4095})
            for n in points:
                torn=bytearray(image)
                for i,v in enumerate(data[:n]):torn[a+i]=v if kind=='erase' else torn[a+i]&v
                r=restored(torn,temporary)
                if temporary:
                    r.request(1,80);r.drain_upload()
                    assert r.cursor(1)==96
                    assert r.sent[0][12:-2]==b''.join(temp(i)[2:12] for i in range(97,197)), 'unacked legacy ID lost or confirmed ID returned'
                    assert torn[0x1e9000:0x1f4400]==before[0x1e9000:0x1f4400], 'unacked sectors changed'
                else:
                    assert cursor(r)==128
                    for sector in range(1,24):
                        start=0x1d0000+sector*4096+32
                        assert torn[start:start+4064]==before[start:start+4064]
                    live=r.read_word(r.addr('lr'));assert live&0xfffffe==0xfffffe,'unacked sector hidden'
                cuts+=1
            for i,v in enumerate(data):image[a+i]=v if kind=='erase' else image[a+i]&v
        result['temporary' if temporary else 'log']={'cuts':cuts,'protected_unacked':'preserved','ACK':'preserved'}
    return result


def temp_ack_power():
    result={}
    for complete in (False,True):
        m=seed();m.fixture(9,[temp(1),temp(2),temp(3)])
        next_index=3 if complete else 2
        m.write(m.addr('temp_unacked')+2,next_index,2)
        before=bytes(m.flash);m.record_changes=True;m.changes.clear()
        m.request(1,81);run_config(m)
        assert m.checked('mem_config_result')==2
        image=bytearray(before);cuts=0
        for kind,a,data in m.changes:
            assert kind=='write'
            for n in range(len(data)+1):
                torn=bytearray(image)
                for i,value in enumerate(data[:n]):torn[a+i]&=value
                r=restored(torn,True);got=r.cursor(1)
                assert got in (0,next_index),(complete,n,got)
                r.request(1,91);r.drain_upload()
                assert r.sent==[wire(9,91,0,[temp(i) for i in range(got+1,4)])]
                if complete and got==3:assert r.cursor(0)==3
                cuts+=1
            for i,value in enumerate(data):image[a+i]&=value
        r=restored(image,True);assert r.cursor(1)==next_index
        result['complete' if complete else 'partial']=cuts
    # Отстающий USB не выходит из окна сравнения при непрерывных частичных ACK.
    m=seed();m.fixture(9,[])
    m.checked('temp_set_index',0,0x7000)
    for cursor in range(0x7801,0x10800,100):
        value=cursor&65535;m.write(m.addr('temp_next'),(value+1)&65535,2)
        m.checked('temp_ack_cursor',1,value)
        other=m.cursor(0)
        assert ((value-other)&65535)<=2048,(value,other)
        assert m.cursor(1)==value
    # Устойчивый сдвиг окна снимает старый кадр другого канала только после commit.
    m=seed();m.fixture(9,[])
    m.checked('temp_set_index',0,0x7000);m.checked('temp_set_index',1,0x7800)
    m.write(m.addr('temp_next'),0x7802,2)
    m.write(m.addr('temp_unacked'),0x7000,2);m.write(m.addr('temp_unacked')+2,0x7801,2)
    m.request(1,82);assert m.pending_cursor(0)==0x7000
    run_config(m);m.checked('user_storage_poll')
    assert m.cursor(0)==0x7001 and m.cursor(1)==0x7801 and m.pending_cursor(0)==0
    result['inactive_channel_across_uint16_wrap']='PASS'
    return result
