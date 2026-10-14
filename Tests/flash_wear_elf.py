"""Износ и питание: настоящий IAR ELF, модель NOR; без платы и сервера."""
import argparse, collections, json, os, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT/'Tests'), str(ROOT/'Tests/asm')]


def endurance(events, temporary=False):
    from test_upload_runtime import UploadMachine, log, wire
    from test_stage5_log import append
    m = UploadMachine(); m.fixture(9 if temporary else 10, [])
    if temporary:m.write(m.device+m.d["newAddressTempID"],m.base)
    def instrumentation(machine):
        # Код ELF исполняется целиком; AAPCS/бюджеты проверяются на границах NOR.
        from unicorn import UC_HOOK_CODE
        machine.u.hook_del(machine.code_hook)
        for address in machine.stubs:
            machine.u.hook_add(UC_HOOK_CODE,machine.hook,begin=address,end=address)
        return machine
    instrumentation(m)
    m.record_changes = True
    erase = collections.Counter(); writes = collections.Counter()
    passes = []; ack_passes = []; started = time.monotonic()
    for i in range(1, events+1):
        if temporary:
            ident=f'{i:010d}'.encode();m.u.mem_write(0x21000400,ident+b'\0')
            assert m.checked('user_addIdToTempBuffer',0x21000400,0),i
            cell=bytes(2)+ident+bytes(20)
        else:
            append(m, i);cell=log(i)
        before = m.now; uniq = (i*2) & 255
        m.request(1, uniq)
        for _ in range(16000):
            m.tick_upload()
            if m.sent: break
        else: raise AssertionError(('send stuck', i))
        assert m.sent == [wire(m.cmd, uniq, 0, [cell])], (i, m.sent)
        m.sent.clear(); ack_before=m.now; m.request(1, (uniq+1)&255)
        for _ in range(16000):
            m.tick_upload()
            if m.sent: break
        else: raise AssertionError(('ACK stuck', i))
        assert m.sent == [wire(m.cmd, (uniq+1)&255, 0, [])], (i, m.sent)
        m.sent.clear(); passes.append(m.now-before); ack_passes.append(m.now-ack_before)
        for kind,a,b in m.changes:
            (erase if kind=='erase' else writes)[a&~4095] += 1
        m.changes.clear(); m.costs.clear()
        if i % 128 == 0:
            # Обход сбоя долгоживущего Unicorn: это перенос ОЗУ, а не сброс платы.
            # Все ARM-адреса, NOR, время, курсоры и состояния сохраняются побайтно.
            ram=bytes(m.u.mem_read(0x20000000,0x5000));media=m.flash;now=m.now
            m=instrumentation(UploadMachine());m.cmd=9 if temporary else 10;m.base=0x1e8400 if temporary else 0x1d0000
            m.u.mem_write(0x20000000,ram);m.flash=media;m.now=now;m.record_changes=True
        if i % 1000 == 0: print('events', i, 'seconds', round(time.monotonic()-started,1), flush=True)
    return dict(events=events, erases={hex(a):n for a,n in sorted(erase.items())},
                page_programs={hex(a):n for a,n in sorted(writes.items())},
                model_polls={'min':min(passes),'max':max(passes),'mean':sum(passes)/len(passes)},
                ack_model_polls={'min':min(ack_passes),'max':max(ack_passes),'mean':sum(ack_passes)/len(ack_passes)},
                seconds=round(time.monotonic()-started,2))


def main():
    p=argparse.ArgumentParser();p.add_argument('--build-root',required=True);p.add_argument('--events',type=int,default=10000);p.add_argument('--output',required=True);p.add_argument('--temporary',action='store_true')
    a=p.parse_args();base=Path(a.build_root)/'ReleaseCustomer'
    for k,sub,ext in [('ELF','ExePath','out'),('BIN','ExePath','bin'),('MAP','ListPath','map')]:
        os.environ['DOMOVOI_'+k]=str(base/sub/('DomovoiNext.'+ext))
    result=endurance(a.events,a.temporary)
    Path(a.output).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps(result),flush=True)

if __name__=='__main__':main()
