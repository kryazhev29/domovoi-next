"""Collect current linked workspace, stack and allocator absence contracts."""
from pathlib import Path
import json
from verification_heap import run
if __name__=='__main__':
    print(json.dumps(run(Path(__file__).resolve().parents[2]),indent=2))
