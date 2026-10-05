"""Select C production inputs for source-extracting host checks."""
import os
from pathlib import Path


def source_path(root, name):
    path = Path(name)
    if os.environ.get('DOMOVOI_SOURCES', '').lower() == 'c' and str(path).replace('\\', '/').startswith('Tests/asm/reference/'):
        suffix = '_runtime' if path.suffix == '.c' else ''
        candidate = root / 'User/c' / (path.stem + suffix + path.suffix)
        if candidate.exists():
            return candidate
        raise FileNotFoundError('No production C counterpart: ' + name)
    return root / path
