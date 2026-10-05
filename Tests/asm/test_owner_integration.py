"""Production scheduler and cross-owner contracts in the current linked ELF."""
import hashlib
import json
from test_expiry_runtime import ExpiryMachine, record
from test_memory_runtime import heap_stats


def expiry_scheduler():
    cases = []
    for cancel_at in (None, 2, 36, 400):
        m = ExpiryMachine()
        m.install([record()])
        m.checked('user_checkIdTimeEnd')
        assert m.expiry_active()
        costs = []
        for i in range(2500):
            assert not m.checked('user_storage_command_ready')
            assert not m.checked('user_storage_flash_ready')
            assert not m.checked('user_storage_background_ready')
            assert not m.checked('mem_tables_ready')
            previous = dict(m.counts)
            assert not m.checked('mem_config_begin', m.addr('config'), m.addr('device'))
            assert m.counts == previous, 'competing config touched NOR'
            assert not m.checked('service_crc_begin_pack', 0, 7)
            assert not m.checked('service_crc_active')
            if i == cancel_at:
                m.checked('user_savereset')
            m.now += 1
            m.checked('user_storage_poll')
            costs.append(m.steps)
            assert m.read_budget <= 128 and m.io_budget <= 1
            if not m.expiry_active():
                break
        else:
            raise AssertionError('scheduler did not drain expiry')
        if cancel_at is None:
            assert m.expiry_events == [(138, b'abcdefghij'.hex(), 255)]
            assert m.flash[0x41000 + 28] == 56
        assert m.io is None
        assert heap_stats(m)['largest_request'] == 4088
        cases.append({'cancel_at': cancel_at, 'passes': len(costs),
                      'maximum_instructions': max(costs),
                      'total_instructions': sum(costs), 'stack': m.stack_peak})
    return cases


if __name__ == '__main__':
    from storage_machine import ROOT
    out = {'expiry_scheduler': expiry_scheduler(), 'status': 'PASS',
           'elf_sha256': hashlib.sha256(
               (ROOT / 'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}
    (ROOT / 'Tests/asm/owner_integration_results.json').write_text(
        json.dumps(out, indent=2) + '\n')
    print('PASS production owner scheduler, reset drain and config exclusion')
