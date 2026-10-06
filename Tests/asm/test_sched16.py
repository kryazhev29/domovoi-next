"""Command-14 wire/storage/access/migration oracles against the selected IAR ELF."""
import hashlib
import struct

from storage_machine import StorageMachine, crc16, fields
from test_access_reads import AccessMachine, TODAY
from test_service_runtime import ServiceMachine

BASE = 0x1ca800
MARK = struct.pack('<II', 0x36314853, 0xc9ceb7ac)


def record(index, start=(8, 30), stop=(18, 15), blocked=0, days=bytes([1])*7):
    return bytes([index, *start, *stop, blocked]) + days + b'\x91\x92\x93'


def tail(scenes=bytes(range(1, 9)), holiday=1):
    return bytes(scenes) + bytes([holiday]) + bytes(range(0xa1, 0xa8))


def parse(m, command, payload, channel=0, uniq=37):
    body = struct.pack('<HBBB', len(payload)+3, 0, uniq, command) + payload
    packet = body + struct.pack('<H', crc16(body))
    m.u.mem_write(0x21000000, packet)
    m.checked('user_parse_comand', 0x21000000, channel)


def protocol():
    cases = 0
    for count in (1, 25, 64):
        for channel in (0, 1):
            m = StorageMachine()
            neighbor = bytes((i*7+31)&255 for i in range(2048))
            m.flash[BASE-2048:BASE] = neighbor
            indices = list(range(65-count, 65))[::-1]
            payload = b''.join(record(i) for i in indices) + tail()
            expected = bytearray(b'\xff'*2048)
            for i in indices:
                expected[(i-1)*16:i*16] = record(i)
            expected[1024:1040] = tail()
            for repeat in range(2):
                m.command(14, payload, count, channel, 37, stamp=0x12345678)
                assert m.active() and m.checked('work_capacity', 14) >= 1040
                m.drain()
                assert not m.answers and m.timestamps[-1] == (3, 0x12345678, crc16(expected[:1040]), channel, 37)
                assert m.flash[BASE:BASE+2048] == expected
                assert m.flash[BASE-2048:BASE] == neighbor
                cases += 1
            # The packet carries every schedule: cells it omits are erased
            # (a schedule deleted on the server must not survive in flash).
            payload = record(32, blocked=1) + record(1) + tail(bytes([64])*8, 3)
            m.command(14, payload, 2, channel, 38)
            m.drain()
            expected[:1024] = b'\xff'*1024
            expected[31*16:32*16] = record(32, blocked=1)
            expected[:16] = record(1)
            expected[1024:1040] = tail(bytes([64])*8, 3)
            assert m.flash[BASE:BASE+2048] == expected
            assert m.timestamps[-1][2] == crc16(expected[:1040])
            cases += 1
    # Strict count/length/index validation, rejected before any flash change.
    invalid = []
    for count in (1,25,64):
        good=b''.join(record(i) for i in range(1,count+1))+tail()
        invalid.extend(((count,good[:-1]),(count,good+b'\0')))
    invalid += [(0, tail()), (65, record(1)*65+tail()), (1, record(0)+tail()),
               (1, record(65)+tail()), (2, record(1)+record(65)+tail())]
    for count, payload in invalid:
        m = StorageMachine(); before = bytes(m.flash)
        m.command(14, payload, count)
        assert m.answers == [(14, 1, 0, 7)] and not m.active() and not m.timestamps
        assert bytes(m.flash) == before and not any(m.counts.values())
        cases += 1
    # Duplicates retain the old last-record-wins semantics.
    m = StorageMachine()
    m.command(14, record(1)+record(1, blocked=1)+tail(), 2); m.drain()
    assert m.flash[BASE:BASE+16] == record(1, blocked=1)
    return {'wire_cases': cases+1, 'maximum_payload': 1045, 'crc_bytes': 1040}


def crc_paths():
    cases = 0
    for channel in (0, 1):
        m = ServiceMachine()
        m.active = lambda: m.call('table_runtime_active') != 0
        m.tick = lambda: StorageMachine.tick(m)
        del m.stubs[m.addr('user_send_timeshtamp')]
        m.command(14,record(64)+tail(),1,channel,37,stamp=0x12345678)
        m.drain()
        body=struct.pack('<HBBBBIH',10,255,37,12,3,0x12345678,crc16(m.flash[BASE:BASE+1040]))
        assert m.sent == [b'SLDN'+body+struct.pack('<H',crc16(body))]
        cases += 1
    for empty in (False, True):
        for runtime in (False, True):
            for channel in (0, 1):
                m = ServiceMachine()
                if empty:
                    m.flash[BASE:BASE+2048] = b'\xff'*2048
                else:
                    m.flash[BASE:BASE+2048] = b'\xff'*2048
                    m.flash[BASE+63*16:BASE+64*16] = record(64)
                    m.flash[BASE+1024:BASE+1040] = tail()
                    m.flash[BASE+1040:BASE+2048] = b'\x51'*1008
                m.write(m.addr('store_runtime'), int(runtime), 1)
                # Synchronous CRC scans use the same real NOR contents.
                if not runtime:
                    def read():
                        a,p,n,_=m.args()
                        m.u.mem_write(p,bytes(m.flash[a:a+n])); m.ret(1)
                    m.stubs[m.addr('w25_read_data')] = read
                    m.checked('user_send_crcPack', channel, 73, budget=2000000)
                else:
                    m.packet(17, channel=channel); m.drain()
                assert m.sent == [m.expected(17)]
                assert struct.unpack_from('<H', m.sent[0], 13)[0] == crc16(m.flash[BASE:BASE+1040])
                cases += 1
    return {'crc12_and_crc17_paths': cases}


def access():
    cases = 0
    for holiday in (1, 2, 3):
        for today_holiday in (False, True):
            for day in (0, 3, 6):
                for minute in (8*60+29, 8*60+30, 12*60, 18*60+14, 18*60+15):
                    m = AccessMachine()
                    m.flash[BASE:BASE+16] = record(1)
                    m.flash[BASE+1024:BASE+1040] = tail(holiday=holiday)
                    m.write(m.device+m.d['day'], day, 1)
                    m.write(m.device+m.d['hour'], minute//60, 1)
                    m.write(m.device+m.d['minute'], minute%60, 1)
                    if today_holiday:
                        m.flash[0x1cc000:0x1cc004] = struct.pack('<I', TODAY)
                    allowed = 8*60+30 <= minute <= 18*60+15 and (
                        holiday==1 or (holiday==2 and today_holiday) or (holiday==3 and not today_holiday))
                    assert bool(m.checked('user_checkShedule', 1)) == allowed, (holiday, today_holiday, day, minute)
                    # Access through a group's schedule uses the same global holiday policy.
                    m.put_group(1); m.put_id(1,b'1234567890')
                    m.flash[0x1c9000+fields('cellGroup_s')['sheduleNumber']] = 1
                    m.u.mem_write(0x21000600,b'1234567890')
                    m.checked('user_checkIdAndControl',0x21000600,2)
                    assert bool(m.loads) == allowed
                    cases += 1
    m = AccessMachine(); buf=0x21000300
    m.flash[BASE:BASE+16] = record(1)
    for scenes in (bytes(8), bytes(range(57,65))):
        m.flash[BASE+1024:BASE+1040] = tail(scenes,3)
        assert m.checked('user_getShedule',1,buf)
        assembled=bytes(m.u.mem_read(buf,32)); offsets=fields('cellShedule_s')
        assert assembled[offsets['scene']:offsets['scene']+8] == scenes
        assert assembled[offsets['onHoliday']] == 3
    m.fail_read(2)
    assert not m.checked('user_getShedule',1,buf), 'tail read failure must deny access'
    # Trigger 8: command 5 selects a schedule for opening to any ID.
    m = AccessMachine(); m.flash[BASE:BASE+16] = record(1)
    m.flash[BASE+1024:BASE+1040] = tail()
    m.write(m.device+m.d['hour'],12,1)
    m.stubs[m.addr('store_command_start')] = lambda:m.ret(1)
    m.stubs[m.addr('log_addSystemEvent')] = lambda:m.ret(1)
    triggers=bytearray(16); triggers[fields('triggers_s')['openAllByShedule']]=1
    parse(m,5,triggers)
    m.u.mem_write(0x21000600,b'+79999999999\0')
    m.checked('user_incomingcall',0x21000600,1)
    assert m.loads
    m.loads.clear(); m.flash[BASE+1024+8]=2
    m.checked('user_incomingcall',0x21000600,1)
    assert not m.loads
    # Blocked days/records and overnight windows retain their established behavior.
    for rec, hour, expected in ((record(1,blocked=1),12,False),
                                (record(1,days=bytes(7)),12,False),
                                (record(1,start=(22,0),stop=(6,0)),23,True),
                                (record(1,start=(22,0),stop=(6,0)),7,False)):
        m.flash[BASE:BASE+16]=rec; m.flash[BASE+1024:BASE+1040]=tail()
        m.write(m.device+m.d['hour'],hour,1)
        assert bool(m.checked('user_checkShedule',1)) == expected
    return {'access_cases':cases+9, 'scene_source':'shared tail; existing scene execution unchanged'}


def migration_machine(flash=None):
    m = StorageMachine(flash)
    m.write(m.addr('store_runtime'),0,1)
    m.u.mem_write(m.addr('mem')+6*8,struct.pack('<II',0x1cd400,4096))
    m.stubs[m.addr('fw_resume_after_boot')] = m.ret
    m.stubs[m.addr('timers_pause')] = m.ret
    # Boot reads a complete device snapshot (512 B); runtime jobs remain bounded.
    def read():
        a,p,n,_=m.args()
        assert 0<n<=1024 and a+n<=len(m.flash)
        if m.failed('read'):m.ret(0); return
        m.u.mem_write(p,bytes(m.flash[a:a+n])); m.ret(1)
    m.stubs[m.addr('w25_read_data')] = read
    return m


def restore(m):
    assert m.checked('mem_recover',budget=3000000)
    m.checked('user_readsavedinformation',budget=3000000)
    return m.checked('sched16_boot',budget=3000000)


def legacy():
    m = migration_machine()
    m.flash[BASE-2048:BASE] = bytes((i*3+11)&255 for i in range(2048))
    for i in range(64):
        m.flash[BASE+i*32:BASE+(i+1)*32] = record(i+1)[:13]+tail()[:9]+bytes(10)
    m.write(m.device+m.d['datetimeShedule'],987654321)
    assert m.checked('user_writeConfigAndDevice',budget=3000000)
    return bytes(m.flash)


def migrated(m, neighbor):
    assert m.flash[BASE:BASE+2048] == b'\xff'*2048
    assert m.flash[BASE-2048:BASE] == neighbor
    assert bytes(m.u.mem_read(m.device+m.d['datetimeShedule'],4)) == bytes(4)
    assert bytes(m.u.mem_read(m.device+m.d['reserv']+32,8)) == MARK


def migration():
    initial = legacy(); neighbor=initial[BASE-2048:BASE]
    m=migration_machine(initial); assert restore(m); migrated(m,neighbor)
    # Registration reads the zeroed timestamp from the restored device.
    m=migration_machine(m.flash); assert restore(m); migrated(m,neighbor)
    sent=[]
    def send():
        p,n,_,_=m.args(); sent.append(bytes(m.u.mem_read(p,n))); m.ret(1)
    m.stubs[m.addr('usb_send')]=send
    m.checked('user_send_registrationData',0)
    assert len(sent)==1 and sent[0][8]==1 and sent[0][34:38]==bytes(4)
    assert crc16(sent[0][4:-2])==int.from_bytes(sent[0][-2:],'little')
    m.write(m.addr('store_runtime'),1,1)
    m.command(14,record(64)+tail(),1); m.drain()
    assert m.timestamps[-1][1]==123456
    # Persist the new sync stamp and verify that the next boot keeps new data.
    m.write(m.addr('store_runtime'),0,1)
    assert m.checked('user_writeConfigAndDevice',budget=3000000)
    saved=bytes(m.flash[BASE:BASE+2048])
    r=migration_machine(m.flash); assert restore(r)
    assert bytes(r.flash[BASE:BASE+2048])==saved
    assert int.from_bytes(r.u.mem_read(r.device+r.d['datetimeShedule'],4),'little')==123456
    assert r.checked('user_clearMemBlock',4,budget=3000000); migrated(r,neighbor)
    # Read failure blocks table access and preserves a zero timestamp.
    r=migration_machine(initial); r.fault=('read',1)
    assert not r.checked('sched16_boot',budget=3000000)
    assert not r.checked('mem_tables_ready')
    r=migration_machine(initial)
    r.stubs[r.addr('user_writeConfigAndDevice')]=lambda:r.ret(0)
    assert not restore(r) and not r.checked('mem_tables_ready')
    r=migration_machine(r.flash); assert restore(r); migrated(r,neighbor)
    # Exercise the real user_init call site; peripherals and unrelated startup
    # work are stubbed, device restoration itself was exercised above.
    r=migration_machine(initial)
    r.checked('user_readsavedinformation',budget=3000000)
    for name in ('user_initvariables','user_readconfig','app_boot_metadata','rs485_init','uart_setRecTimeout','WG_init'):
        r.stubs[r.addr(name)]=r.ret
    r.stubs[r.addr('w25_read_devID')]=lambda:r.ret(0x17c2)
    r.checked('user_init',budget=3000000); migrated(r,neighbor)
    return {'legacy_to_empty':True,'new_sync_survives_boot':True,'factory_reset':True,
            'registration_timestamp_zero':True,'actual_user_init_hook':True}


def migration_power():
    initial=legacy(); neighbor=initial[BASE-2048:BASE]
    count=0; operations=0; distinct=0
    for factory in (False,True):
        m=migration_machine(initial); assert restore(m)
        if factory:
            m.write(m.addr('store_runtime'),1,1)
            m.command(14,record(1)+tail(),1); m.drain()
            m.write(m.addr('store_runtime'),0,1)
            assert m.checked('user_writeConfigAndDevice',budget=3000000)
            image=bytes(m.flash)
        else:
            image=initial; m=migration_machine(image)
        m.record_changes=True
        assert (m.checked('user_clearMemBlock',4,budget=3000000) if factory else restore(m))
        state=bytearray(image); operations+=len(m.changes); seen=set()
        for kind,address,data in m.changes:
            for n in sorted({0,1,len(data)//2,len(data)-1,len(data)}):
                cut=bytearray(state)
                for i,value in enumerate(data[:n]):
                    cut[address+i]=value if kind=='erase' else cut[address+i]&value
                count+=1
                fingerprint=hashlib.sha256(cut).digest()
                # Erasing/programming FF can leave exactly the same persistent
                # state at several cut points. Reboot each distinct image once.
                if fingerprint in seen:continue
                seen.add(fingerprint); distinct+=1
                r=migration_machine(cut)
                assert restore(r),(factory,kind,hex(address),n)
                # Before a factory-reset intent commits, the old new-format
                # database may survive; after intent, it must finish clearing.
                if factory and r.flash[BASE:BASE+2048] != b'\xff'*2048:
                    assert r.flash[BASE:BASE+2048]==image[BASE:BASE+2048]
                else:
                    migrated(r,neighbor)
                assert r.flash[BASE-2048:BASE]==neighbor
            if kind=='erase':state[address:address+len(data)]=data
            else:
                for i,value in enumerate(data):state[address+i]&=value
    return {'power_cut_cases':count,'distinct_reboots':distinct,'flash_operations':operations}
