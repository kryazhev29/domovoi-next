"""Контракты прежних cfgstrings/cmdlengths/commands на настоящем ELF."""
import struct
from storage_machine import fields, crc16
from test_service_runtime import ServiceMachine


def packet(command, payload=b'', length=None):
    length = 3 + len(payload) if length is None else length
    body = struct.pack('<HBBB', length, 255, 42, command) + payload
    body = (body + bytes(2048))[:length + 2]
    return body + struct.pack('<H', crc16(body))


def execute(m, command, payload=b'', channel=1, length=None):
    data = packet(command, payload, length)
    m.u.mem_write(0x21000000, data)
    m.checked('user_parse_comand', 0x21000000, channel)
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
        for extra in (0, 2):
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
                m.write(m.addr('config') + fields('config_s')['pinCode'], 0x31323334)
                payload[:4] = struct.pack('<I', 0x31323334)
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
    m=ServiceMachine();m.write(m.addr('config')+fields('config_s')['pinCode'],0x31323334)
    execute(m,102,struct.pack('<I',0x31323335),0)
    assert not m.u.mem_read(m.device+m.d['userAutorized'],1)[0]
    assert m.sent[-1][8:10] == bytes([102,0])
    # CRC, авторизация и минимальная длина проверяются отдельными отказами.
    m = ServiceMachine(); data = bytearray(packet(2, b'\x02')); data[-1] ^= 1
    m.u.mem_write(0x21000000, bytes(data)); m.checked('user_parse_comand', 0x21000000, 1)
    assert m.answers == [(2, 254, 1, 42)]
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
        m.stubs[m.addr('store_restore')] = m.ret
        m.stubs[m.addr('sched16_restore')] = m.ret
        m.checked('user_readsavedinformation')
        imei = bytes(m.u.mem_read(m.device + d['IMEIGSM'], 16))
        expected = b'123456789012345\0' if terminated else b'Q'*15+b'\0'
        assert imei == (expected if valid else b'old\0'+bytes(12))
    m=machine();saved=bytearray(b'Q'*512);saved[-2:]=struct.pack('<H',crc16(saved[:-2]))
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
                data = bytes(m.u.mem_read(address, 2048)); return data.split(b'\0', 1)[0]
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
    return {'sms_cases': 17}
