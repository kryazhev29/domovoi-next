"""Independent linked-IAR allocator budget, fragmentation and lifetime checks.

Does not compile or mutate production. --root may select an integrated ELF tree.
All sizes are bytes; no board timing or global stack bound is claimed.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path


def run(root):
    sys.path.insert(0, str(root / 'Tests' / 'asm'))
    from storage_machine import StorageMachine, UC_ARM_REG_PRIMASK

    m = StorageMachine()
    live = {}

    def check_live():
        for p, (n, pattern, _) in live.items():
            assert bytes(m.u.mem_read(p, n)) == bytes([pattern]) * n, ('corruption', p)

    def allocate(n, pattern):
        p = m.checked('__basic_malloc', n)
        assert p and not p & 7, ('allocation/alignment', n, p)
        chunk = int.from_bytes(m.u.mem_read(p - 8, 4), 'little')
        assert chunk >= n + 8 and not chunk & 7
        assert m.addr('HEAP$$Base') <= p - 8 < p + n <= m.addr('HEAP$$Limit')
        for q, (qn, _, _) in live.items():
            assert p + n <= q - 8 or q + qn <= p - 8
        m.u.mem_write(p, bytes([pattern]) * n)
        live[p] = (n, pattern, chunk)
        return p

    def free(p):
        check_live()
        m.checked('__basic_free', p)
        del live[p]
        check_live()

    def largest():
        low, high = 0, m.heap_bytes
        while low < high:
            n = (low + high + 1) // 2
            p = m.checked('__basic_malloc', n)
            if p:
                m.checked('__basic_free', p)
                low = n
            else:
                high = n - 1
            check_live()
        p = m.checked('__basic_malloc', low) if low else 0
        chunk = int.from_bytes(m.u.mem_read(p - 8, 4), 'little') if p else 0
        if p:
            m.checked('__basic_free', p)
        total = m.heap_bytes - sum(v[2] for v in live.values())
        return {'largest_payload': low, 'largest_chunk': chunk,
                'total_free_chunks': total,
                'external_fragmentation': 1 - chunk / total if total else 0}

    pristine = largest()
    assert pristine['largest_payload'] == m.heap_bytes - 8
    held = [allocate(n, 0x31 + i) for i, n in enumerate((180, 1040, 534, 2016))]
    chunks = [live[p][2] for p in held]
    concurrent = largest()
    # A failing allocation must preserve all live allocations and PRIMASK.
    for primask in (0, 1):
        m.u.reg_write(UC_ARM_REG_PRIMASK, primask)
        assert m.checked('__basic_malloc', m.heap_bytes) == 0
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == primask
        check_live()
    m.u.reg_write(UC_ARM_REG_PRIMASK, 0)
    for i in (1, 3, 0, 2):
        free(held[i])
    assert largest() == pristine

    holes = [allocate(n, 0x41 + i) for i, n in enumerate((128, 512, 128, 768, 128))]
    tail = allocate(largest()['largest_payload'], 0x71)
    assert largest()['largest_payload'] == 0
    free(holes[1]); free(holes[3])
    fragmented = largest()
    assert fragmented['largest_payload'] == 768
    assert fragmented['total_free_chunks'] == 520 + 776
    assert m.checked('__basic_malloc', 1024) == 0
    check_live()
    for p in (holes[2], tail, holes[0], holes[4]):
        free(p)
    assert largest() == pristine

    # Repeated mixed lifetimes exercise splitting, best-fit holes, coalescing.
    for cycle in range(32):
        a = allocate(180, cycle)
        b = allocate(534, 100 + cycle)
        c = allocate(2016, 150 + cycle)
        free(b)
        d = allocate(1040, 200 + cycle)
        free(a); free(c); free(d)
        assert largest() == pristine
    check_live()
    return {
        'elf_sha256': hashlib.sha256((root / 'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
        'units': 'bytes; modeled allocator execution, not physical board timing',
        'heap': m.heap_bytes, 'cstack': m.top - m.bottom,
        'static_gap': m.addr('fault_record') - m.addr('HEAP$$Limit'),
        'concurrent_chunks': chunks, 'concurrent_used': sum(chunks),
        'concurrent_free': concurrent, 'fragmented_holes': fragmented,
        'fully_coalesced': pristine, 'repeated_lifetimes': 32,
        'allocator_stack_peak': m.stack_peak,
        'abi_bounds_oom_live_bytes_primask': 'PASS',
    }


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = run(args.root.resolve())
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
