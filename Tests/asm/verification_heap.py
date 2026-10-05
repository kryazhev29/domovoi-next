"""Independent linked-IAR explicit ownership and lifetime checks.

Does not compile or mutate production. --root may select an integrated ELF tree.
All sizes are bytes; no board timing or global stack bound is claimed.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path


def run(root):
    sys.path.insert(0,str(root/'Tests/asm'))
    from iar_image import image_paths
    from test_work_memory import run as owners
    from storage_machine import StorageMachine
    m=StorageMachine();assert not m.heap_bytes
    assert all(n not in m.symbols for n in ('__basic_malloc','__basic_free','__iar_dlmalloc','__iar_dlfree','HEAP$$Base'))
    return {'elf_sha256':hashlib.sha256(image_paths()[0].read_bytes()).hexdigest(),
            'heap':0,'workspace':2224,'cstack':m.top-m.bottom,
            'static_gap':m.addr('fault_record')-m.top,'owner_contracts':owners(),
            'scope':'Linked explicit owner pairs; no allocator or fragmentation.'}


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = run(args.root.resolve())
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
