"""Counterexamples for rejected storage designs; NOT completed feature tests.

Run separately against each linked image. These preserve concrete reasons why
an address wrap, a two-byte ID commit or a separately committed CRC is unsafe.
The production storage paths remain unchanged.
"""
import argparse
import struct

from c_vs_asm_suite import select_image
from storage_machine import StorageMachine, crc16
from test_upload_runtime import UploadMachine, log


def unrestricted_reads(m):
    def read():
        address, target, count, _ = m.args()
        assert address + count <= len(m.flash)
        m.u.mem_write(target, bytes(m.flash[address:address + count]))
        m.ret(1)
    m.stubs[m.addr('mem_read')] = read


def run():
    findings = {}
    m = UploadMachine()
    m.fixture(10, [log(i) for i in range(1, 3073)])
    # A naive wrap erases one sector, then commits the next record at its start.
    m.flash[m.base:m.base + 4096] = b'\xff' * 4096
    m.flash[m.base:m.base + 32] = log(3073)
    unrestricted_reads(m)
    observed = m.call('log_getRecordsCount')
    assert observed == 1
    findings['ring_wrap_linear_scan'] = {'observed_live': observed, 'required_live': 2945}

    m = UploadMachine()
    # Even a correct logical ordering cannot retain the old ordinal after loss.
    m.fixture(10, [log(i) for i in range(129, 401)])
    m.write(m.addr('log_usbReadIndex'), 128, 2)
    m.request(); m.drain_upload()
    first = struct.unpack_from('<I', m.sent[0], 12)[0]
    assert first == 257
    findings['ring_stale_ordinal'] = {'observed_first': first, 'required_first': 129}

    m = UploadMachine()
    m.fixture(10, [log(i) for i in range(3073, 3201)] + [log(i) for i in range(129, 3073)])
    # Power loss can make the first record unusable while later records survive.
    m.flash[m.base:m.base + 32] = b'\xff' * 32
    first_indices = []
    for offset in range(0, 98304, 4096):
        record = bytes(m.flash[m.base + offset:m.base + offset + 32])
        m.u.mem_write(0x21000000, record)
        if m.call('log_valid', 0x21000000):
            first_indices.append(struct.unpack_from('<I', record)[0])
    assert max(first_indices) < 3200
    findings['ring_first_record_only'] = {'observed_max': max(first_indices), 'required_max': 3200}

    # Both byte orders for index-last have a non-FFFF intermediate state.
    # Index-first likewise exposes a cell before its body is written.
    for label, raw in (
        ('id_index_first', b'\x01\x00' + b'\xff' * 30),
        ('id_index_last_low_first', b'\x01\xff' + bytes(30)),
        ('id_index_last_high_first', b'\xff\x00' + bytes(30)),
    ):
        m = StorageMachine()
        m.flash[0x41000:0x41020] = raw
        unrestricted_reads(m)
        occupied = m.call('user_getNumbersCount', budget=2000000)
        assert occupied == 1
        findings[label] = {'observed_occupied': occupied, 'required_occupied': 0}

    before, after = crc16(b'\xff' * 32), crc16(b'\x01\x00' + bytes(30))
    assert before != after
    findings['crc_metadata_first_cut'] = {'cached': after, 'oracle': before}
    findings['crc_data_first_cut'] = {'cached': before, 'oracle': after}
    m = StorageMachine()
    m.write(m.addr('cfg_legacy_checked'), 1, 1)
    assert m.call('flash_claim', 5)
    accepted = m.call('mem_config_begin', m.addr('config'), m.addr('device'))
    assert not accepted
    findings['crc_existing_config_transaction'] = {'accepted_while_table_owned': bool(accepted)}
    return findings


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--variant', required=True, choices=('ASM', 'C', 'HYBRID'))
    p.add_argument('--build-root')
    args = p.parse_args()
    select_image(args.variant, args.build_root)
    for label, result in run().items():
        print('REJECTED DESIGN', label, result)
