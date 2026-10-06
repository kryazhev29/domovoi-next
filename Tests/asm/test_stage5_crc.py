"""Linked raw-byte CRC cache: all writer classes, reboot and every write cut."""
from test_id_sync_fast import SyncMachine,fixture,cell,BASE
from test_access_reads import AccessMachine
from test_expiry_runtime import ExpiryMachine,record
from test_idclear_runtime import ClearMachine
from storage_machine import crc16
import struct
A,B=0x1fb000,0x1fc000

def get(m):
    p=0x21000800
    return int.from_bytes(m.u.mem_read(p,2),'little') if m.checked('idcrc_get',0,8192,p) else None

def oracle(m):return crc16(m.flash[BASE:BASE+8192])

def warm(m):
    for _ in range(1600):
        m.now+=1;m.checked('idcrc_background')
        value=get(m)
        if value is not None and not m.checked('idcrc_io_active'):return value
    raise AssertionError('CRC cache did not become durable')

def run():
    m=fixture(SyncMachine(),0,total=8192);assert warm(m)==oracle(m)
    for ids in ((1,2),(1,129),(2,)):
        m.command(13,b''.join(cell(i,len(ids)) for i in ids),len(ids));m.drain()
        assert get(m)==oracle(m);assert warm(m)==oracle(m)
        r=fixture(SyncMachine(m.flash),0,unknown=True,total=8192)
        assert warm(r)==oracle(r)
    a=AccessMachine();a.put_id(1,b'0000000001');assert warm(a)==oracle(a)
    row=bytearray(a.flash[BASE:BASE+32]);row[15]^=3
    a.u.mem_write(0x21000400,bytes(row))
    assert a.checked('user_updateID',0x21000400,budget=5000000)==1
    assert get(a)==oracle(a);assert warm(a)==oracle(a)
    e=ExpiryMachine();e.install([record(1)],(0,));assert warm(e)==oracle(e)
    e.start_expiry();e.expiry_drain();assert get(e)==oracle(e);assert warm(e)==oracle(e)
    c=ClearMachine();c.flash[BASE:BASE+32]=cell(1);assert warm(c)==oracle(c)
    c.parse();c.clear_drain();assert get(c)==oracle(c);assert warm(c)==oracle(c)
    return {'writers':['packet13 append','packet13 sector journal','user_updateID / cntPass / first-pass','expiry','clear'],'CRC':'raw full oracle'}

def power():
    m=fixture(SyncMachine(),0,total=8192);assert warm(m)==oracle(m)
    checksum=oracle(m)
    # Fill A so the following packet rotates through erase/header/carry/dirty.
    for i in range(510):struct.pack_into('<IHH',m.flash,A+16+8*i,i+1,checksum,checksum^65535)
    m=fixture(SyncMachine(m.flash),0,total=8192);assert warm(m)==oracle(m)
    initial=bytes(m.flash);m.record_changes=True
    m.command(13,cell(1),1);m.drain();assert warm(m)==oracle(m)
    image=bytearray(initial);cuts=0
    for kind,a,b in m.changes:
        for n in sorted({0,1,len(b)//2,len(b)-1,len(b)}):
            cut=bytearray(image)
            for i,v in enumerate(b[:n]):cut[a+i]=v if kind=='erase' else cut[a+i]&v
            r=fixture(SyncMachine(cut),0,unknown=True,total=8192)
            assert r.checked('sj_recover',budget=5000000)
            expected=oracle(r)
            for _ in range(1600):
                r.now+=1;r.checked('idcrc_background');cached=get(r)
                assert cached is None or cached==expected,(kind,hex(a),n,cached,expected)
                if cached is not None and not r.checked('idcrc_io_active'):break
            else:raise AssertionError('recovery did not finish')
            cuts+=1
        for i,v in enumerate(b):image[a+i]=v if kind=='erase' else image[a+i]&v
    # Erasure may set sequence bits in the old bank while leaving its CRC pairs.
    # The header complement prevents a false newer-bank selection.
    corrupt=bytearray(m.flash);corrupt[A+4]|=2
    r=fixture(SyncMachine(corrupt),1,total=8192);assert warm(r)==oracle(r)
    return {'cuts':cuts,'old_bank_torn_generation':'rejected'}


def loader_recovery():
    m=fixture(SyncMachine(),1,total=8192);assert warm(m)==oracle(m)
    initial=bytes(m.flash);m.record_changes=True;m.command(13,cell(1,1),1);m.drain();warm(m)
    cut=bytearray(initial)
    for kind,a,b in m.changes:
        for i,v in enumerate(b):cut[a+i]=v if kind=='erase' else cut[a+i]&v
        if kind=='write' and a==0x1fa01c:break
    else:raise AssertionError('journal commit not reached')
    assert cut[BASE:BASE+8192]==initial[BASE:BASE+8192]
    # The unchanged bootloader performs sector recovery with raw NOR calls;
    # it knows nothing about the application CRC journal or its RAM cache.
    loader=fixture(SyncMachine(cut),0,unknown=True,total=8192)
    loader.stubs[loader.addr('idcrc_write')]=loader.sync_write
    loader.stubs[loader.addr('idcrc_erase')]=loader.sync_erase
    assert loader.checked('sj_recover',budget=2000000)
    assert loader.flash[BASE:BASE+32]==cell(1,1)
    reboot=fixture(SyncMachine(loader.flash),0,unknown=True,total=8192)
    expected=oracle(reboot)
    for _ in range(1600):
        reboot.now+=1;reboot.checked('idcrc_background');cached=get(reboot)
        assert cached is None or cached==expected,('loader bypassed dirty',cached,expected)
        if cached is not None and not reboot.checked('idcrc_io_active'):break
    else:raise AssertionError('loader recovery CRC did not finish')
    return {'cut':'SJ commit before first MB_ID mutation','loader':'unmodified raw NOR boundary','CRC':expected}


def failed_commit_recovery():
    m=fixture(SyncMachine(),1,total=8192);assert warm(m)==oracle(m)
    m.command(13,cell(1,1),1)
    for _ in range(4000):
        m.tick()
        if m.io and m.io[0]==0x1fa01c:break
    else:raise AssertionError('commit marker not reached')
    m.fault=('poll',m.counts['poll']+1);m.drain()
    assert m.answers and not m.sent
    # Metadata publication after a failed transaction must not make the still
    # pending loader recovery disappear behind a valid pre-transaction CRC.
    loader=fixture(SyncMachine(m.flash),0,unknown=True,total=8192)
    loader.stubs[loader.addr('idcrc_write')]=loader.sync_write
    loader.stubs[loader.addr('idcrc_erase')]=loader.sync_erase
    assert loader.checked('sj_recover',budget=2000000)
    reboot=fixture(SyncMachine(loader.flash),0,unknown=True,total=8192);expected=oracle(reboot)
    for _ in range(1600):
        reboot.now+=1;reboot.checked('idcrc_background');cached=get(reboot)
        assert cached is None or cached==expected,('failed commit published old CRC',cached,expected)
        if cached is not None and not reboot.checked('idcrc_io_active'):break
    else:raise AssertionError('failed commit recovery did not finish')
    return {'failed_marker_verification':'dirty retained','CRC':expected}
