"""Narrow linked Thumb instruction mutations; no source/binary file changes."""
import hashlib
import json
from test_service_runtime import *


def patch_shared(m,mode):
    from capstone import Cs,CS_ARCH_ARM,CS_MODE_THUMB,CS_MODE_MCLASS
    name='reply_send' if mode=='commit-retry' else 'service_crc_quiet' if mode=='session-cancel' else 'service_crc_poll'
    start=m.addr(name);code=bytes(m.u.mem_read(start,512));off=0
    if mode=='commit-retry':replacement=b'\x01\x20\x70\x47'
    elif mode=='session-cancel':replacement=b'\x70\x47'
    else:
        md=Cs(CS_ARCH_ARM,CS_MODE_THUMB|CS_MODE_MCLASS);instructions=list(md.disasm(code,start));places=[]
        if mode=='read-budget':
            for i in instructions:
                if i.mnemonic.startswith('mov') and i.size==2 and i.op_str.endswith(', #0x40'):
                    places.append(i)
            assert len(places)==1,[(i.mnemonic,i.op_str) for i in places]
            i=places[0];off=i.address-start
            word=int.from_bytes(i.bytes,'little');assert word&0xf800==0x2000
            replacement=((word&0xff00)|65).to_bytes(2,'little')
            read=m.stubs[m.addr('w25_read_data')]
            def bounded_read():
                assert m.args()[2]<=64,'CRC service exceeded its 64-byte read contract'
                read()
            m.stubs[m.addr('w25_read_data')]=bounded_read
        else:
            target=m.addr('w25_read_data');after=False
            for i in instructions:
                if i.mnemonic=='bl' and int(i.op_str.lstrip('#'),0)==target:after=True;continue
                if after and i.mnemonic.startswith('beq'):
                    off=i.address-start;replacement=b'\x00\xbf'*(i.size//2);break
            else:raise AssertionError('failed-read branch not found')
    original=code[off:off+len(replacement)];assert original!=replacement
    m.u.mem_write(start+off,replacement);m.u.ctl_remove_cache(start+off,start+off+len(replacement))
    return dict(symbol=name,offset=off,original=original.hex(),replacement=replacement.hex())


def patch(m, mode):
    if 'idcrc_get' in m.symbols and mode!='ff-affine':return patch_shared(m,mode)
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
        name = 'crc16_ffInit' # shared affine helper now owns the FF transform
        start = m.addr(name); code = bytes(m.u.mem_read(start, 60))
        pattern = b'\x80\xea\x01\x05' # eor.w r5,r0,r1 (affine constant)
        assert code.count(pattern) == 1
        off = code.index(pattern); replacement = b'\x05\x46\x00\xbf' # mov r5,r0; nop
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
