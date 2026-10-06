"""Mutate linked Thumb instructions in Unicorn; never change build inputs."""
import hashlib
import json
from pathlib import Path
from test_idindex_runtime import IndexMachine


def run():
    detected = []
    for name, pattern, replacement, span in (
        ('pending-is-not-success', b'\x01\x28', b'\x00\x28', 64),
        ('require-verified-result', b'\x02\x28', b'\x03\x28', 64),
        ('read-budget-64', b'\x40\x22', b'\x41\x22', 900),
    ):
        m = IndexMachine()
        expected, _, _ = m.setup((1, 2))
        at = m.addr('idindex_impl_poll' if 'idindex_impl_poll' in m.symbols else 'idindex_poll')
        code = bytes(m.u.mem_read(at, span))
        locations = [i for i in range(0, len(code) - 1, 2)
                     if code[i:i + 2] == pattern]
        assert len(locations) == 1, (name, locations)
        m.u.mem_write(at + locations[0], replacement)
        try:
            m.drain_index()
            assert m.state() == 3
            assert m.flash[0x300000:0x340000] == expected
        except AssertionError:
            detected.append(name)
        else:
            raise AssertionError((name, 'mutant survived'))
    return detected


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[2]
    result = {'detected': run(), 'status': 'PASS',
              'elf_sha256': hashlib.sha256(
                  (root / 'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}
    (root / 'Tests/asm/idindex_mutation_results.json').write_text(
        json.dumps(result, indent=2) + '\n')
    print('PASS', len(result['detected']), '/3 linked index mutations')
