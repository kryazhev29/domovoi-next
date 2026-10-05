"""Prove in linked code that private photo TX cannot overlap new storage jobs."""
import hashlib
import json
from test_photo_runtime import *
from test_photo_edges import reach


def free_chunks(m):
    p = int.from_bytes(m.u.mem_read(m.addr('Aldata')+4,4), 'little')
    out = []; visited = set()
    while p:
        assert p not in visited; visited.add(p)
        size, following = struct.unpack('<II', m.u.mem_read(p,8))
        assert m.addr('HEAP$$Base') <= p < p+size <= m.addr('HEAP$$Limit')
        out.append(size); p = following
    return out


def run():
    m = PhotoMachine(); m.boot_media(); m.capture(b'x'*768)
    m.packet(21, b'', 1); m.drain(); m.port(); reach(m,39)
    m.tick() # allocate TX534 and fill the first <=64 bytes
    usb = m.checked('__basic_malloc',1040); assert usb
    tested = []
    for phase in (39,40,41):
        reach(m, phase)
        pointer = m.rget('tx',4); assert pointer
        assert int.from_bytes(m.u.mem_read(pointer-8,4),'little') == 544
        assert m.checked('photo_storage_busy')
        assert not m.checked('user_storage_command_ready')
        assert not m.checked('user_storage_background_ready')
        assert not m.checked('mem_config_begin',0x21000000,0x21000200)
        before = dict(m.counts); saved = bytes(m.u.mem_read(pointer,534))
        for cmd in (9,10,13):
            m.packet(cmd)
            assert m.answers[-1][0:2] == (cmd,2)
            assert m.rget('tx',4) == pointer and m.rget('phase') == phase
        assert m.counts == before and bytes(m.u.mem_read(pointer,534)) == saved
        chunks = free_chunks(m)
        tested.append({'phase':phase,'live_photo_tx_chunk':544,
                       'heap_used_with_usb':m.heap_bytes-sum(chunks),
                       'free_chunks':chunks,'new_storage_commands':'rejected before allocation/I/O'})
    m.tick()
    assert not m.checked('photo_storage_busy') and m.rget('tx',4) == 0
    chunks = free_chunks(m)
    assert m.heap_bytes-sum(chunks) == 192+1048
    m.checked('__basic_free',usb)
    return {'elf_sha256':hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
            'phases':tested,'phase_zero_photo_tx':0,
            'idle_with_usb_heap_used':192+1048,
            'conclusion':'Photo TX544 is mutually exclusive with accepted command9/10/13 and config snapshots; photo_s192 can coexist.'}


if __name__ == '__main__':
    out = run()
    (ROOT/'Tests/asm/verification_photo_lifetime_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS linked photo TX lifetime:',out['phases'],flush=True)
