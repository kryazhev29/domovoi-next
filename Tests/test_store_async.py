"""Bounded config publication: real memory.c with a NOR/power-cut model."""
import pathlib, subprocess, tempfile, re
from source_select import source_path
ROOT = pathlib.Path(__file__).resolve().parents[1]
ZIG = r"C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe"
def src(p): return source_path(ROOT,p).read_bytes().decode("cp1251")
base = src("Tests/test_storage.c").split("/* PRODUCTION */")[0]
# Original harness supplies NOR 1->0, partial operations, legacy CRC, heap checks.
base = base.replace("oplength[256]", "oplength[1024]")
base = base.replace("bool w25_read_data(", "static unsigned read_fault, read_calls;\nbool w25_read_data(")
base = base.replace("{CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);return true;}", "{if(read_fault && ++read_calls==read_fault)return false;CHECK(a+n<=sizeof(flash));memcpy(p,flash+a,n);return true;}")
mem = src("Tests/asm/reference/User/memory.c")
mem = re.sub(r'^#include[^\n]*', '', mem, flags=re.M)
shared = src("User/sector_journal.h") + "\n" + src("User/sector_journal.c").replace('#include "sector_journal.h"', '')
header = "\n".join(x for x in src("User/memory.h").splitlines() if x.startswith(("#define MEM", "typedef uint8_t (*mem_page_reader", "bool mem_patch", "uint8_t mem_config_result")))
with tempfile.TemporaryDirectory() as d:
    p = pathlib.Path(d)
    unit = base + "\n" + src("Tests/store_async_model.inc") + "\n" + header + "\n" + shared + mem + "\n" + src("Tests/store_async_cases.inc")
    unit = src("User/fast_kernels.h") + "\n" + unit
    (p/"test.c").write_text(unit, encoding="utf-8")
    subprocess.run([ZIG,"cc","-std=c99","-O2","-Wno-unused-function","-Wno-unused-variable",str(p/"test.c"),"-o",str(p/"test.exe")],check=True)
    subprocess.run([str(p/"test.exe")],check=True,timeout=300)
