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
    # Проверяем точный дифф фото, затем сравниваем прочие инструкции с ASM.
    text = '\n'.join(hot)
    for added, original_photo in (
        (
            'BEQ.N    ??store_command_allowed_0\n'
            'CMP      R0,#+21\n'
            'BNE.N    ??store_command_allowed_photo_ready\n'
            'BL       photo_empty_cleanup\n'
            'CBNZ.N   R0,??store_command_allowed_0\n'
            '??store_command_allowed_photo_ready:\n'
            'BL       user_storage_command_ready'
            ,
            'BEQ.N    ??store_command_allowed_0\n'
            'BL       user_storage_command_ready'
            ,
        ),
        (
            '??user_initvariables_1:\n'
            'MOVS     R0,#+0\n'
            'STRH     R0,[R4, #+155]\n'
            'LDRH     R0,[R4, #+129]\n'
            'BIC      R0,R0,#0x2000\n'
            'STRH     R0,[R4, #+129]\n'
            '??user_initvariables_2:'
            ,
            '??user_initvariables_1:\n'
            'BL       user_getPhotoCount\n'
            'STRH     R0,[R4, #+155]\n'
            'CBZ.N    R0,??user_initvariables_2\n'
            'LDRH     R0,[R4, #+129]\n'
            'ORR      R0,R0,#0x2000\n'
            'STRH     R0,[R4, #+129]\n'
            '??user_initvariables_2:'
            ,
        ),
    ):
        assert text.count(added) == 1, 'unexpected photo ASM change'
        text = text.replace(added, original_photo, 1)
    hot = text.splitlines()
    position=0
    for line in hot:
        while position<len(original) and original[position]!=line:position+=1
        assert position<len(original), ('changed ASM instruction/directive',line)
        position+=1
    print('PASS HYBRID retained ASM instructions and literal pools',len(hot))


if __name__=='__main__':main()
