# Доработка: Кряжев А., 2026
"""Стартовый подсчёт базы ID при отказе чтения флеш: никого не теряем.

Если флеш не прочиталась, подсчёт не должен публиковать частичный результат:
число абонентов 0, граница поиска — вся база, индекс не строится. Тогда
запасной перебор в user_findID просматривает всю базу.
"""
import struct

from storage_machine import StorageMachine

BASE = 0x41000      # начало базы ID в модели
CELL = 32


def cell(index, ident):
    """Записанная ячейка ID: номер, 10 байт ID, остальное — признаки записи."""
    body = struct.pack('<H', index) + ident.ljust(10, b'\0')
    return body + b'\0' * (CELL - 1 - len(body)) + b'\x00'


def main():
    # Без отказа: две записанные ячейки дают счёт 2 и границу по последней.
    m = StorageMachine()
    m.flash[BASE:BASE + CELL] = cell(1, b'1111111111')
    m.flash[BASE + 5 * CELL:BASE + 6 * CELL] = cell(6, b'2222222222')
    assert m.checked('user_getNumbersCount', budget=20_000_000) == 2
    assert m.u.mem_read(m.addr('id_cells_end'), 2) == struct.pack('<H', 6)

    # Отказ первого же чтения: счёт 0, поиск по всей базе, конец данных неизвестен.
    m = StorageMachine()
    m.flash[BASE:BASE + CELL] = cell(1, b'1111111111')
    m.fault = ('read', 1)
    assert m.checked('user_getNumbersCount', budget=20_000_000) == 0
    size = struct.unpack('<I', m.u.mem_read(m.addr('mem') + 2 * 8 + 4, 4))[0]
    end = struct.unpack('<H', m.u.mem_read(m.addr('id_cells_end'), 2))[0]
    assert end == size // CELL, ('граница поиска не на всю базу', end, size // CELL)
    assert m.u.mem_read(m.addr('id_used_bytes'), 4) == b'\xff\xff\xff\xff'
    print('PASS стартовый подсчёт ID: отказ чтения не сужает поиск и не публикует частичный счёт')


if __name__ == '__main__':
    main()
