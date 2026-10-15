"""Контракты прежних cfgstrings/cmdlengths/commands на настоящем ELF."""
import struct
from storage_machine import StorageMachine, fields, crc16
from test_service_runtime import ServiceMachine


def packet(command, payload=b'', length=None):
    length = 3 + len(payload) if length is None else length
    body = struct.pack('<HBBB', length, 255, 42, command) + payload
    body = (body + bytes(2048))[:length + 2]
    return body + struct.pack('<H', crc16(body))


def execute(m, command, payload=b'', channel=1, length=None):
    data = packet(command, payload, length)
    m.u.mem_write(0x21000000, data)
    m.receive_command(0x21000000, channel)
    return data


def lengths():
    count = 0
    minima = {1: 8, 2: 4, 3: 4, 6: 6, 18: 4, 23: 7, 102: 7, 105: 4, 108: 4}
    for command, minimum in minima.items():
        for channel in (0, 1):
            for length in range(3, minimum):
                m = ServiceMachine()
                before = bytes(m.u.mem_read(m.device, 512)), bytes(m.flash)
                payload = bytes([0]) + bytes([0x5a]) * 32
                data = execute(m, command, payload, channel, length)
                assert m.answers == [(command, 1, channel, 42)], (command, length, m.answers)
                assert (bytes(m.u.mem_read(m.device, 512)), bytes(m.flash)) == before
                assert bytes(m.u.mem_read(0x21000000, len(data))) == data
                count += 1
    for command, minimum in minima.items():
        for extra in ((0, 1) if command == 102 else (0, 2)):
            m = ServiceMachine()
            calls = []
            # Сохранённые действия команд проверяются через реальные поля device.
            for name in ('app_trial_confirm', 'app_boot_error_reported', 'user_saveindex',
                         'comm_server_registered'):
                if name in m.symbols:
                    m.stubs[m.addr(name)] = m.ret
            if 'app_trial_active' in m.symbols: m.stubs[m.addr('app_trial_active')] = lambda: m.ret(1)
            if 'app_boot_error_unreported' in m.symbols: m.stubs[m.addr('app_boot_error_unreported')] = lambda: m.ret(0)
            m.stubs[m.addr('user_send_crcMemoryID')] = lambda: (calls.append(m.args()[0]), m.ret())
            payload = bytearray([2] * 32)
            if command == 1: payload[:5] = bytes([0]) + struct.pack('<I', 0x10203040)
            if command == 6: payload[:3] = bytes([5, 3, 7])
            if command == 23: payload[:4] = struct.pack('<I', 0x10203040)
            if command == 102:
                m.write(m.addr('config') + fields('config_s')['pinCode'], 4321)
                payload[:4+extra] = b'04321' if extra else b'4321'
            execute(m, command, payload, 0 if command >= 100 else 1, minimum + extra)
            expected = {1: ('datetime', 0x10203040, 4), 2: ('timeToReset', 2, 1),
                        3: ('timeToDefault', 2, 1), 6: ('timeToGetID', 5, 1),
                        23: ('datetime', 0x10203040, 4), 102: ('userAutorized', 1, 1),
                        105: ('moduleType', 2, 1), 108: ('logBits', 2, 1)}
            if command == 18: assert calls == [2]
            else:
                field, value, size = expected[command]
                assert int.from_bytes(m.u.mem_read(m.device + m.d[field], size), 'little') == value
            if command == 1:
                assert m.u.mem_read(m.device+m.d['registredOnServer'],1) == b'\x01'
            if command == 6:
                assert m.u.mem_read(m.device+m.d['sourceGetID'],1) == b'\x03'
                assert m.u.mem_read(m.device+m.d['addressGetID'],1) == b'\x07'
            count += 1
    for code in range(1,6):
        m=ServiceMachine();acks=[];reopened=[]
        m.stubs[m.addr('comm_server_registered')]=lambda:(acks.append(m.args()[0]),m.ret())
        m.stubs[m.addr('comm_channel_reopen')]=lambda:(reopened.append(m.args()[0]),m.ret())
        m.stubs[m.addr('app_trial_active')]=lambda:m.ret(1)
        execute(m,1,bytes([code]))
        assert not m.answers and acks == [0] and reopened == [0]
        assert not m.u.mem_read(m.device+m.d['registredOnServer'],1)[0]
        count += 1
    m=ServiceMachine();m.write(m.addr('config')+fields('config_s')['pinCode'],4321)
    execute(m,102,b'5321',0)
    assert not m.u.mem_read(m.device+m.d['userAutorized'],1)[0]
    assert m.sent[-1][8:10] == bytes([102,0])
    # CRC, авторизация и минимальная длина проверяются отдельными отказами.
    m = ServiceMachine(); data = bytearray(packet(2, b'\x02')); data[-1] ^= 1
    m.u.mem_write(0x21000000, bytes(data)); m.receive_command(0x21000000, 1)
    assert not m.answers and not m.u.mem_read(m.device+m.d["timeToReset"],1)[0]
    m = ServiceMachine(); m.write(m.device + m.d['userAutorized'], 0, 1)
    execute(m, 2, b'\x02', 0); assert m.answers == [(2, 4, 0, 42)]
    for length in range(3):
        m = ServiceMachine(); execute(m, 1, length=length); assert not m.answers
    # Мутация настоящего шлюза должна разрушать проверку короткого пакета.
    m = ServiceMachine(); m.stubs[m.addr('command_args_valid')] = lambda: m.ret(1)
    execute(m, 2, b'\x02', length=3)
    assert m.answers != [(2, 1, 1, 42)]
    return {'length_cases': count, 'guard_mutants': 1}


def config_strings():
    def machine():
        m = ServiceMachine()
        def read():
            address, pointer, size, *_ = m.args()
            m.u.mem_write(pointer, bytes(m.flash[address:address+size])); m.ret(1)
        m.stubs[m.addr('mem_read')] = read
        return m
    c = fields('config_s'); d = fields('device_s')
    names = {'server1': 32, 'server2': 32, 'NTPserver': 32, 'netOperator': 32,
             'countryCode': 8, 'simPhone': 16, 'ussdBalance': 8}
    for terminated in (False, True):
        m = machine(); saved = bytearray(b'Q' * 512)
        struct.pack_into('<I', saved, c['pinCode'], 4321)
        for name, size in names.items():
            if terminated: saved[c[name]:c[name]+size] = b'ok\0' + bytes(size-3)
        saved[-2:] = struct.pack('<H', crc16(saved[:-2]))
        m.flash[0x1cd400:0x1cd600] = saved
        m.checked('user_readconfig')
        current = bytes(m.u.mem_read(m.addr('config'), 512))
        for name, size in names.items():
            assert current[c[name]+size-1] == 0
            assert current[c[name]:c[name]+size-1] == saved[c[name]:c[name]+size-1]
        assert crc16(current[:-2]) == int.from_bytes(current[-2:], 'little')
        assert m.flash[0x1cd400:0x1cd600] == saved
    m = machine(); saved = bytearray(b'Q' * 512); saved[-1] ^= 1
    m.flash[0x1cd400:0x1cd600] = saved
    calls = []
    for name in ('user_setdefault', 'user_saveindex'):
        m.stubs[m.addr(name)] = lambda name=name: (calls.append(name), m.ret())
    m.checked('user_readconfig'); assert calls == ['user_setdefault', 'user_saveindex']
    for valid,terminated in ((True,False),(True,True),(False,False)):
        m = machine(); saved = bytearray(b'Q' * 512)
        if terminated: saved[d['IMEIGSM']:d['IMEIGSM']+16] = b'123456789012345\0'
        saved[-2:] = struct.pack('<H', crc16(saved[:-2]))
        if not valid: saved[-1] ^= 1
        m.u.mem_write(m.device+d['IMEIGSM'],b'old\0'+bytes(12))
        m.flash[0x1cd600:0x1cd800] = saved
        m.stubs[m.addr('user_comm_boot')] = m.ret
        m.stubs[m.addr('sched16_restore')] = m.ret
        m.checked('user_readsavedinformation')
        imei = bytes(m.u.mem_read(m.device + d['IMEIGSM'], 16))
        expected = b'123456789012345\0' if terminated else b'Q'*15+b'\0'
        assert imei == (expected if valid else b'old\0'+bytes(12))
    m=machine();saved=bytearray(b'Q'*512);struct.pack_into('<I',saved,c['pinCode'],4321)
    saved[-2:]=struct.pack('<H',crc16(saved[:-2]))
    m.flash[0x1cd400:0x1cd600]=saved
    m.stubs[m.addr('config_terminate_strings')]=m.ret
    m.checked('user_readconfig')
    assert bytes(m.u.mem_read(m.addr('config')+c['server1']+31,1)) != b'\0'
    return {'config_cases': 3, 'device_cases': 3, 'termination_mutants': 1}


def text_commands():
    calls = []
    def machine():
        m = ServiceMachine()
        m.u.mem_write(m.addr('config') + fields('config_s')['countryCode'], b'+7\0' + bytes(5))
        def send():
            text, number, *_ = m.args()
            def string(address):
                data = bytearray()
                for offset in range(2048):
                    byte = m.u.mem_read(address + offset, 1)[0]
                    if not byte:return bytes(data)
                    data.append(byte)
                raise AssertionError('unterminated SMS string')
            calls.append((string(text), string(number))); m.ret()
        m.stubs[m.addr('user_sendsms')] = send
        return m
    for length in range(3, 14):
        m = machine(); execute(m, 11, b'A' * 32, length=length)
        assert not calls and m.answers == [(11, 1, 1, 42)]
    for text, expected in ((b'hello', b'hello'), (b'h\0llo', b'h'), (b'', None), (b'A'*2027, b'A'*2027)):
        m = machine(); calls.clear()
        execute(m, 11, b'9001234567' + text)
        if expected is None: assert not calls and m.answers[-1][1] == 1
        else: assert calls == [(expected, b'+79001234567')]
    m = machine(); calls.clear()
    m.u.mem_write(m.addr('config') + fields('config_s')['countryCode'], b'7' * 8)
    execute(m, 11, b'9001234567hello'); assert not calls and m.answers[-1][1] == 1
    m = machine(); m.stubs[m.addr('work_acquire')] = lambda: m.ret(0)
    execute(m, 11, b'9001234567hello'); assert not calls and m.answers[-1][1] == 2
    for number in (b'9\0' + b'X'*8, b'9'*10):
        m = machine(); calls.clear()
        execute(m, 11, number + b'hello')
        assert calls == [(b'hello', b'+7' + number.split(b'\0')[0])]
    # Номер звонящего ограничен полем; хвост после раннего NUL очищен.
    for number in (b'', b'9', b'9'*40):
        m = machine()
        m.stubs[m.addr('user_checkIdAndControl')] = lambda: m.ret(0)
        address = m.device + m.d['callingnumber']
        capacity = m.d['csq'] - m.d['callingnumber']
        m.u.mem_write(address, b'X'*(capacity+1))
        m.u.mem_write(0x21000400, number + b'\0')
        m.checked('user_incomingcall', 0x21000400, 1)
        assert bytes(m.u.mem_read(address,capacity+1)) == number[:capacity-1].ljust(capacity,b'\0') + b'X'
    return {'sms_cases': 19, 'caller_bounds': 3}


def pin_config():
    # Реальное чтение, A/B-запись и повторная загрузка: переход выполняется один раз.
    def machine():
        m = ServiceMachine()
        # Стартовое синхронное чтение config допускает 512 байт за один вызов NOR.
        def read():
            address, pointer, size, *_ = m.args()
            assert 0 < size <= 512 and address + size <= len(m.flash)
            m.u.mem_write(pointer, bytes(m.flash[address:address+size])); m.ret(1)
        m.stubs[m.addr('w25_read_data')] = read
        m.stubs[m.addr('w25_write_data')] = lambda: StorageMachine.sync_write(m)
        m.stubs[m.addr('w25_erase_sector')] = lambda: StorageMachine.sync_erase(m)
        m.write(m.addr('store_runtime'), 0, 1)
        m.record_changes = True
        return m
    c = fields('config_s'); d = fields('device_s'); cases = 0
    for old, expected in ((0x31323334, 4321), (0x34333231, 1234),
                          (0x41423334, 4321), (100000, 4321),
                          (0, 0), (20140, 20140), (99999, 99999)):
        m = machine(); saved = bytearray(512)
        struct.pack_into('<I', saved, c['pinCode'], old)
        saved[-2:] = struct.pack('<H', crc16(saved[:-2]))
        m.flash[0x1cd400:0x1cd600] = saved
        saved_device = bytearray(512)
        saved_device[d['IMEIGSM']:d['IMEIGSM']+16] = b'867442070217472\0'
        saved_device[-2:] = struct.pack('<H', crc16(saved_device[:-2]))
        m.flash[0x1cd600:0x1cd800] = saved_device
        m.checked('user_readconfig', budget=3000000)
        current = bytes(m.u.mem_read(m.addr('config'), 512))
        assert struct.unpack_from('<I', current, c['pinCode'])[0] == expected, (old, expected)
        assert crc16(current[:-2]) == int.from_bytes(current[-2:], 'little')
        assert current[:c['pinCode']] == saved[:c['pinCode']]
        assert current[c['pinCode']+4:-2] == saved[c['pinCode']+4:-2]
        active = m.select_config()
        assert m.flash[active:active+512] == saved
        assert m.flash[active+512:active+1024] == saved_device
        assert int.from_bytes(m.u.mem_read(m.addr('store_save_requested'),1),'little') == (old > 99999)
        for name in ('user_comm_boot', 'sched16_restore'):
            m.stubs[m.addr(name)] = m.ret
        m.checked('user_readsavedinformation')
        assert bytes(m.u.mem_read(m.device+d['IMEIGSM'], 16)) == b'867442070217472\0'
        if old > 99999:
            for _ in range(1500):
                m.now+=1;m.checked('user_storage_poll')
                if not int.from_bytes(m.u.mem_read(m.addr('store_save_requested'),1),'little') and m.checked('flash_exec_idle'):break
            else:raise AssertionError('deferred PIN save stuck')
        active=m.select_config()
        assert m.flash[active:active+512] == current
        assert (active != 0x1cd400) == (old > 99999)
        assert m.flash[active+512+d['IMEIGSM']:active+512+d['IMEIGSM']+16] == b'867442070217472\0'
        before = bytes(m.flash); writes = len(m.changes)
        m.checked('user_readconfig', budget=3000000)
        assert bytes(m.flash) == before and len(m.changes) == writes
        cases += 1
    # Неверная CRC не разрешает переход ASCII даже при правдоподобном ПИНе.
    m = machine(); saved = bytearray(512)
    struct.pack_into('<I', saved, c['pinCode'], 0x34333231)
    saved[-2:] = struct.pack('<H', crc16(saved[:-2]) ^ 1)
    m.flash[0x1cd400:0x1cd600] = saved
    m.checked('user_readconfig', budget=3000000)
    assert int.from_bytes(m.u.mem_read(m.addr('config')+c['pinCode'], 4), 'little') == 4321
    return {'pin_config_cases': cases + 1}


def pin_commands():
    c = fields('config_s'); count = 0
    cases = ((4321, b'4321', True), (4321, b'04321', True),
             (20140, b'20140', True), (99999, b'99999', True),
             (0, b'0000', True), (0, b'00000', True),
             (432, b'432', False), (4321, b'004321', False),
             (4321, b'43a1', False), (0, b'', False),
             (4321, b'48074', True), (4321, b'48075', False))

    def machine(pin, imei=b'867442070217472'):
        m = ServiceMachine()
        m.write(m.addr('config') + c['pinCode'], pin)
        m.u.mem_write(m.device + m.d['IMEIGSM'], (imei + bytes(16))[:16])
        m.stubs[m.addr('log_addSystemEventID')] = lambda: m.ret(1)
        return m

    def sms(m, text, valid):
        replies = []
        def send():
            reply, number, *_ = m.args()
            replies.append((bytes(m.u.mem_read(reply, 128)).split(b'\0')[0],
                            bytes(m.u.mem_read(number, 16)).split(b'\0')[0])); m.ret()
        m.stubs[m.addr('user_sendsms')] = send
        m.u.mem_write(0x21000000, b'#imei ' + text + b',\0')
        m.u.mem_write(0x21000400, b'+79031234567\0')
        m.u.mem_write(0x21000800, b'\x01')
        assert m.checked('user_smsreceived', 0x21000400, 0x21000000, 0x21000800)
        imei = bytes(m.u.mem_read(m.device + m.d['IMEIGSM'], 16)).split(b'\0')[0]
        expected = b'IMEI: ' + imei if valid else b'IMEI: Error "Pin not valid"'
        assert replies == [(expected, b'+79031234567')], (text, replies)

    for pin, text, valid in cases:
        m = machine(pin); m.write(m.device + m.d['userAutorized'], 0, 1)
        execute(m, 102, text, 0)
        assert bool(m.u.mem_read(m.device+m.d['userAutorized'], 1)[0]) == valid, text
        if len(text) >= 4: assert m.sent[-1][8:10] == bytes([102, int(valid)])
        else: assert m.answers == [(102, 1, 0, 42)]
        sms(m, text, valid); count += 2
    for imei in (b'', b'86744207021747', b'8674420702174721',
                 b'867442a70217472', b'8674420a0217472'):
        for text in (b'32768', b'48074'):
            m = machine(4321, imei); execute(m, 102, text, 0)
            assert m.sent[-1][8:10] == b'\x66\0'
            sms(m, text, False); count += 2
    # Формула даёт uint16; нулевой IMEI из 15 цифр допустим по контракту заказчика.
    for imei, universal in ((b'000000000000000', 32768), (b'999999999999999', 49769),
                            (b'123456789012345', 34422)):
        m = machine(4321, imei)
        assert m.checked('user_pin_valid', universal)
        assert not m.checked('user_pin_valid', universal + 1)
        count += 2
    for pin in (4321, 20140, 0):
        m = machine(pin); m.checked('user_send_identPackToConfigurator', 0)
        assert m.sent[-1][24] == (pin == 4321); count += 1
    # Команда 12 принимает байты числа, сохраняет их, затем новый ПИН работает в SMS/USB.
    m = machine(4321); received = bytearray(512)
    struct.pack_into('<I', received, c['pinCode'], 20140)
    server = b'gate.gsmgate.ru:15010\0'
    received[c['server1']:c['server1']+len(server)] = server
    received[c['countryCode']:c['countryCode']+3] = b'+7\0'
    received[-2:] = struct.pack('<H', crc16(received[:-2]))
    m.flash[0x1cd400:0x1cd600] = bytes(m.u.mem_read(m.addr('config'), 512))
    m.checked('user_comm_boot')
    execute(m, 12, received, 1)
    job = m.addr('store') + fields('store_state_s')['intent']
    assert m.exec_active(12) and not m.answers
    m.limit_reads = 256
    for _ in range(2000):
        if not m.u.mem_read(job, 1)[0] and not m.exec_active(12) and not m.active(): break
        StorageMachine.tick(m)
    else: raise AssertionError('command 12 did not complete')
    assert int.from_bytes(m.u.mem_read(m.addr('config')+c['pinCode'], 4), 'little') == 20140
    assert m.flash[m.select_config()+c['pinCode']:m.select_config()+c['pinCode']+4] == struct.pack('<I', 20140)
    sms(m, b'20140', True); sms(m, b'04321', False); sms(m, b'48074', True)
    execute(m, 102, b'20140', 0); assert m.sent[-1][8:10] == b'\x66\x01'
    # Сервер СофтЭксперт шлёт ПИН текстом «4321»: принимается числом.
    struct.pack_into('<I', received, c['pinCode'], 0x31323334)
    received[-2:] = struct.pack('<H', crc16(received[:-2]))
    execute(m, 12, received, 1)
    for _ in range(2000):
        if not m.u.mem_read(job, 1)[0] and not m.exec_active(12) and not m.active(): break
        StorageMachine.tick(m)
    else: raise AssertionError('command 12 did not complete')
    assert int.from_bytes(m.u.mem_read(m.addr('config')+c['pinCode'], 4), 'little') == 4321
    before = bytes(m.u.mem_read(m.addr('config'), 512)), bytes(m.flash)
    for invalid in (100000, 0x41333231, 0xffffffff):
        struct.pack_into('<I', received, c['pinCode'], invalid)
        execute(m, 12, received, 1)
        assert m.answers[-1] == (12, 1, 1, 42)
        assert (bytes(m.u.mem_read(m.addr('config'), 512)), bytes(m.flash)) == before
    return {'pin_command_cases': count + 7}


def rx_capacity():
    for channel in (0, 1):
        m = ServiceMachine()
        for used in (0, 1, 100, 2048, 0):
            m.write(m.addr('server_rx_count'), used, 2)
            m.write(m.device + m.d['freeBufferSize'], 777, 2)
            assert m.checked('user_comm_rx_capacity') == 2048 - used
            m.write(m.addr('uniqIDCnt'), 42, 1)
            m.checked('user_send_statusPack', channel)
            body = struct.pack('<HBBBIHH', 18, 15, 42, 2, 0x78563412, 0, 2048 - used) + bytes(7)
            assert m.sent[-1] == b'SLDN' + body + struct.pack('<H', crc16(body)), (channel, used, m.sent[-1].hex())
            m.checked('answer11', 0x21000300, 42, 8, 0)
            body = struct.pack('<HBBBBBIH', 11, 255, 42, 11, 8, 0, 0x78563412, 2048 - used)
            assert bytes(m.u.mem_read(0x21000300, 19)) == b'SLDN' + body + struct.pack('<H', crc16(body))
    return {'status1_answer11_dynamic_free_and_full_frames': 'PASS'}


def fw_request_ttl():
    for channel in (0, 1):
        m = ServiceMachine()
        for block in (0, 7, 65535):
            m.write(m.addr('uniqIDCnt'), 42, 1)
            assert m.checked('user_getFWblockFromServer', channel, block) == 1
            body = struct.pack('<HBBBH', 5, 20, 42, 13, block)
            assert m.sent[-1] == b'SLDN' + body + struct.pack('<H', crc16(body))
    return {'request13_TTL20_and_unchanged_frame': 'PASS'}
