"""Narrow linked Thumb instruction mutations; no source/binary file changes."""
import hashlib
import json
from test_service_runtime import *


def patch(m, mode):
    if mode == 'commit-retry':
        # The commit result lives in reply_send (shared since 859c443): a failed
        # commit reported as sent loses the reply instead of retrying it.
        start = m.addr('reply_send'); code = bytes(m.u.mem_read(start, 64))
        pattern = b'\x00\x20\x70\xbd' # movs r0,#0; pop {r4-r6,pc} after release
        assert code.count(pattern) == 1
        off = code.index(pattern); replacement = b'\x01\x20'
        original = code[off:off+2]; m.u.mem_write(start+off, replacement)
        return {'symbol': 'reply_send', 'offset': off, 'original': original.hex(), 'replacement': replacement.hex()}
    name = 'service_crc_quiet' if mode == 'session-cancel' else 'service_crc_poll'
    start = m.addr(name)
    code = bytes(m.u.mem_read(start, 24 if mode == 'session-cancel' else 600))
    if mode == 'read-budget':
        pattern = b'\x40\x2d\x88\xbf\x40\x25' # cmp r5,64; it hi; movhi r5,64
        assert code.count(pattern) == 1
        off = code.index(pattern)+4; replacement = b'\x41\x25'
    elif mode == 'read-error':
        pattern = b'\x20\x68\x40\x19\x20\x60' # advance address only after successful read
        assert code.count(pattern) == 1
        off = code.index(pattern)-2
        assert int.from_bytes(code[off:off+2], 'little') & 0xff00 == 0xd000
        replacement = b'\x00\xbf' # remove failed-read branch
    elif mode == 'session-cancel':
        pattern = b'\x82\x42' # cmp r2,r0; beq cancel
        assert code.count(pattern) == 1
        off = code.index(pattern)+2
        word = int.from_bytes(code[off:off+2], 'little')
        assert word & 0xff00 == 0xd000
        replacement = (word ^ 0x100).to_bytes(2, 'little')
    elif mode == 'commit-retry':
        pattern = b'\x00\x2d' # cmp r5,0 after preserving commit result
        places = [i for i in range(0,len(code)-4,2) if code[i:i+2] == pattern and
                  int.from_bytes(code[i+2:i+4], 'little') & 0xff00 == 0xd000]
        assert len(places) == 3, places # real bytes, FF bytes, commit result
        off = places[-1]+2; replacement = b'\x00\xbf'
    else:
        pattern = b'\x31\x8c\x48\x40\x20\x82' # affine constant XOR then store CRC
        assert code.count(pattern) == 1
        off = code.index(pattern)+2; replacement = b'\x00\xbf'
    original = code[off:off+len(replacement)]
    assert original != replacement
    m.u.mem_write(start+off, replacement)
    return {'symbol': name, 'offset': off, 'original': original.hex(), 'replacement': replacement.hex()}


def run():
    detected = {}
    for mode in ('read-budget', 'read-error', 'session-cancel', 'commit-retry', 'ff-affine'):
        m = ServiceMachine(129 if mode != 'ff-affine' else 0)
        detail = patch(m, mode) # setup assertions never count as detection
        try:
            m.packet(17, channel=int(mode == 'commit-retry'))
            if mode == 'read-error': m.fault = ('read', 1)
            if mode == 'session-cancel': m.checked('service_crc_quiet', 0)
            if mode == 'commit-retry':
                m.commit_fail = True
                for _ in range(200):
                    if not m.active(): break
                    m.tick()
                assert m.active(), 'lost pending response'
            else:
                m.drain()
                expected = [] if mode == 'session-cancel' else [m.expected_error(17)] if mode == 'read-error' else [m.expected(17)]
                assert m.sent == expected, 'wire/cancel mismatch'
        except AssertionError as error:
            detail['detected_by'] = str(error)
        else:
            raise AssertionError((mode, 'mutation survived'))
        detected[mode] = detail
        print(mode, 'DETECTED in linked Thumb', flush=True)
    return {'elf_sha256': hashlib.sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest(),
            'mutations': detected, 'detected': len(detected)}


if __name__ == '__main__':
    result = run()
    (ROOT/'Tests/asm/service_mutation_results.json').write_text(json.dumps(result, indent=2)+'\n')
