"""Splitting ASM may delete sections/CFI; it must not rewrite instructions/pools."""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]


def lines(path):
    out=[]
    for line in path.read_text(encoding='cp1251').splitlines():
        line=line.strip()
        if not line or line.startswith(';') or line.startswith('#include'): continue
        if re.match(r'(?:CFI|PUBLIC|EXTERN)\b',line): continue
        line=line.replace('hybrid_unused_cold_','')
        out.append(line)
    return out


def main():
    original=lines(ROOT/'User/asm/user_fun_runtime.s')
    hot=lines(ROOT/'User/hybrid/user_fun_hot.s')
    position=0
    for line in hot:
        while position<len(original) and original[position]!=line:position+=1
        assert position<len(original), ('changed ASM instruction/directive',line)
        position+=1
    print('PASS HYBRID retained ASM instructions and literal pools',len(hot))


if __name__=='__main__':main()
