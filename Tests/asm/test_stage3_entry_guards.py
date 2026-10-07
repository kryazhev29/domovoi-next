"""Кадры с верным CRC и нулевым числом записей проверяют защиту вызывающего кода."""
from storage_machine import StorageMachine


def run():
    attempts = 0
    for command in (13, 14, 15, 16):
        for channel in (0, 1):
            for records in (b'', bytes(32)):
                m = StorageMachine()
                before = bytes(m.flash)

                m.command(command, records, 0, channel, 29)
                assert m.answers == [(command, 1, channel, 29)], (command, m.answers)
                assert not m.active() and not m.changes and bytes(m.flash) == before
                attempts += 1


    from iar_layout import fields
    assert fields('cellID_s')['ID'] == 2
    for size in (32, 16, 16, 4):
        for count in range(1, 256):
            assert 0 < count * size < 65536
    killed = 0
    for command in (13, 14, 15):
        m = StorageMachine()
        validator='sched16_valid' if command==14 else 'sync_cells_valid'
        m.stubs[m.addr(validator)] = lambda: m.ret(1)
        m.command(command, b'', 0, 0, 29)

        assert m.answers != [(command, 1, 0, 29)]
        killed += 1
    return {'zero_count_crc_valid_mutations': attempts,
            'caller_validator_mutants_killed': killed,
            'channels': [0, 1], 'commands': [13, 14, 15, 16],
            'nor_unchanged': True, 'caller_rejection': 'CMD_PARAMERROR'}
