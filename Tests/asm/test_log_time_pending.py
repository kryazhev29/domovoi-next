# Доработка: Кряжев А., 2026
"""Событие до времени сервера: ждёт в очереди и получает настоящее время.

Найдено на плате 07.10.2026: событие «подключён к серверу» после старта
записывалось сразу с временем от включения (01.01 00:00:19), минуя очередь.
"""
import struct

from storage_machine import StorageMachine, fields

EVENT_CONNECT = 14


def u32(m, address):
    return struct.unpack('<I', bytes(m.u.mem_read(address, 4)))[0]


def main():
    m = StorageMachine()
    d = fields('device_s'); log = fields('cellLog_s')
    pending = m.addr('log_pending'); size = 32
    m.write(m.addr('log_timeKnown'), 0, 1)
    m.write(m.addr('log_pendingCount'), 0, 1)
    m.write(m.device + d['worktime'], 19, 4)
    m.write(m.device + d['datetime'], 19, 4)

    # До времени сервера событие только встаёт в очередь — во флеш ничего не пишется.
    assert m.checked('log_addSystemEvent', EVENT_CONNECT) == 1
    assert bytes(m.u.mem_read(m.addr('log_pendingCount'), 1)) == b'\x01'
    assert m.counts['start'] == 0, 'событие записано во флеш до времени сервера'
    assert u32(m, pending + log['datetime']) == 19

    # Пришло время сервера: время события пересчитывается от момента события.
    now = 1791337000
    m.write(m.device + d['worktime'], 25, 4)
    m.checked('log_setTime', now)
    assert u32(m, pending + log['datetime']) == now - (25 - 19), u32(m, pending + log['datetime'])
    assert m.counts['start'] == 0, 'очередь пишет только владелец журнала'
    print('PASS событие до времени сервера ждёт в очереди и получает настоящее время')


if __name__ == '__main__':
    main()
