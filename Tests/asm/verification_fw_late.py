# Доработка: Кряжев А., 2026
# Поздние отказы прошивки и восстановление её сохранённых полей.
# Выполняется связанный IAR-код; NOR и транспорт задаёт модель без платы.
from test_work_memory import owned
"""Поздние ошибки прошивки и восстановление состояния после сброса."""
from verification_fw_bridge import *


# Проверяет поздний отказ CRC, удержание ответа и подавление закрытого канала.
def late_case(ch,registered=True,quiet=False):
    m=FirmwareMachine();m.fixture();m.begin(1,ch,True);m.until(lambda:m.phase()==13)
    assert len(m.sent)==1 and not m.read_word(m.device+171,1)
    m.flash[0x1200]&=0xfe # Назначение изменилось после проверки всех записей копии.
    m.write(m.device+133,int(registered),1)
    m.busy=ch==0;m.commit_fail=ch==1
    if not registered and ch:
        m.drain();assert len(m.sent)==1
        assert not m.read_word(m.device+171,1) and not m.read_word(m.addr('fw_rollback_end'))
        assert owned(m)==0
        return dict(channel=ch,registered=False,status_reply='suppressed')
    m.until(lambda:m.phase()==16)
    assert not m.read_word(m.device+171,1) and not m.read_word(m.addr('fw_rollback_end'))
    assert not m.read_word(m.device+164,1)
    assert m.read_word(m.ptr()+124,2)==13
    frame=bytes(m.u.mem_read(m.ptr(),13));assert frame[8]==4 and frame[10]==253
    # Прежний отправитель служит независимым эталоном байтов на отдельной модели
    # с тем же процентом: Python не повторяет разбор полей пакета.
    old=FirmwareMachine();old.write(old.device+150,m.read_word(m.device+150,1),1)
    old.checked('user_send_statusUpdateFW',20,253,ch,0)
    assert old.sent==[frame],(ch,old.sent,frame)
    pointer=m.ptr()
    for _ in range(4):
        m.tick_fw();assert m.ptr()==pointer and bytes(m.u.mem_read(pointer,13))==frame and len(m.sent)==1
        assert not m.lease
    if quiet:
        m.checked('user_comm_session',0);m.drain();assert len(m.sent)==1
    else:
        m.busy=m.commit_fail=False;m.drain();assert m.sent[-1]==frame and len(m.sent)==2
    assert not m.ptr() and not m.io and owned(m)==0
    return dict(channel=ch,registered=registered,quiet=quiet,crc_mismatch='no reboot',
        wire_matches_legacy_helper=True,owned_reply_bytes=13,busy_passes=4)


# Проверяет копирование заголовка и сброс до окончательной проверки образа.
def copy_header_and_power():
    m=FirmwareMachine();m.fixture(132)
    m.flash[0x1a000:0x1a004]=b'GOOD'
    m.begin(1,0,True);m.until(lambda:m.phase()==13)
    assert m.flash[0x1000:0x1004]==b'\xff'*4
    assert m.flash[0x1004:0x1020]==m.head[4:]
    assert not m.read_word(m.device+171,1)
    boot=FirmwareMachine();boot.flash=bytearray(m.flash)
    selected=boot.select_config()
    boot.u.mem_write(boot.device,bytes(boot.flash[selected+512:selected+1024]))
    boot.checked('fw_resume_after_boot')
    assert not boot.read_word(boot.device+164,1) and not boot.read_word(boot.device+171,1)
    assert not boot.ptr() and not boot.read_word(boot.addr('fw_rollback_end'))
    m.drain();assert m.read_word(m.device+171,1)==1
    assert m.flash[0x1200:0x1284]==m.image
    return dict(header_marker='FFFFFFFF before final verification',rollback_power_cut='current app remains; explicit rollback retry required, as before',verified_copy_reboots=True)


# Проверяет продолжение загрузки по долговечно сохранённому состоянию.
def download_resume():
    m=FirmwareMachine();m.fixture(1028);m.begin(4,1,True);m.busy=True;m.until(lambda:m.phase()==16)
    assert not m.sent and m.checked('mem_config_result')==0
    boot=FirmwareMachine();boot.flash=bytearray(m.flash)
    selected=boot.select_config()
    boot.u.mem_write(boot.device,bytes(boot.flash[selected+512:selected+1024]))
    boot.checked('fw_resume_after_boot')
    assert boot.read_word(boot.device+164,1)==1 and boot.read_word(boot.device+148,2)==2
    assert boot.read_word(boot.device+172,2)==0 and boot.read_word(boot.device+179,2)==600
    assert not boot.read_word(boot.device+171,1) and not boot.ptr()
    return dict(config_verified_before_ack=True,power_cut='actual cfg_select + fw_resume_after_boot',
        download_resumes=True,block_index=0,blocks=2,no_automatic_apply=True)


if __name__=='__main__':
    result=dict(status='PASS',late_errors=[late_case(0),late_case(1),late_case(1,False),late_case(1,True,True)],
        copy_header=copy_header_and_power(),download_resume=download_resume(),
        elf_sha256=hashlib.sha256((arg.project/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
        scope='Linked final firmware code, modeled NOR and transport; no board timing')
    Path(__file__).with_name('verification_fw_late_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
