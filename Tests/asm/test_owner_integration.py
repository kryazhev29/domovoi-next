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


def clear_scheduler():
    from test_idclear_runtime import ClearMachine, wire
    cases = []
    for action in ('complete', 'reset_before', 'reset_pending', 'usb_epoch'):
        m = ClearMachine(); m.parse(); assert m.clear_active()
        costs = []
        for i in range(32):
            for gate in ('user_storage_command_ready', 'user_storage_flash_ready',
                         'user_storage_background_ready', 'mem_tables_ready'):
                assert not m.checked(gate), (action, gate)
            before = dict(m.counts)
            assert not m.checked('mem_config_begin', m.addr('config'), m.addr('device'))
            assert not m.checked('service_crc_begin_pack', 0, 7)
            m.checked('expiry_runtime_begin')
            assert not m.checked('expiry_runtime_active')
            m.checked('upload_runtime_begin', 10, 0, 7)
            assert not m.checked('upload_runtime_active')
            assert m.counts == before
            if (action == 'reset_before' and i == 0) or (action == 'reset_pending' and i == 1):
                m.checked('user_savereset')
            if action == 'usb_epoch' and i == 1:
                m.write(m.addr('usb_bus_resets'), 1)
            m.now += 1; m.checked('user_storage_poll'); costs.append(m.steps)
            assert m.io_budget <= 1 and m.read_budget == 0
            if not m.clear_active(): break
        else: raise AssertionError(('clear scheduler stuck', action))
        assert m.io is None and heap_stats(m)['largest_request'] == 4088
        assert m.wires == ([wire()] if action == 'complete' else [])
        assert m.counters() == ((0, 0, 0) if action in ('complete', 'usb_epoch') else (7, 129, 8192))
        cases.append({'action': action, 'passes': len(costs), 'maximum_instructions': max(costs),
                      'total_instructions': sum(costs), 'stack': m.stack_peak})
    return cases


if __name__ == '__main__':
    from storage_machine import ROOT
    out = {'expiry_scheduler': expiry_scheduler(), 'clear_scheduler': clear_scheduler(), 'status': 'PASS',
           'elf_sha256': hashlib.sha256(
               (ROOT / 'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}
    (ROOT / 'Tests/asm/owner_integration_results.json').write_text(
        json.dumps(out, indent=2) + '\n')
    print('PASS production expiry/clear scheduler, reset drain and cross-owner exclusion')
