"""Настоящие исходники для host-проверок фрагментов кода."""
import os
import re
from pathlib import Path


def source_path(root, name):
    """Все стенды читают исходники рабочего проекта независимо от варианта ELF."""
    return root / name


def source_text(root, name):
    """Выделить обычную C-ветку перед извлечением фрагментов."""
    if name == 'User/c/user_*.c':
        # Общие объявления и тематические модули берутся из живой прошивки.
        # Команды идут последними: выделение case ограничивается их функцией.
        paths = [root/'User/c/user_logic_internal.h', root/'User/c/runtime_internal.h']
        paths += sorted(p for p in (root/'User/c').glob('user_*.c') if p.name != 'user_commands.c')
        paths += [root/'User/c/user_commands.c']
        text = '\r\n'.join(p.read_bytes().decode('utf-8') for p in paths)
    else:
        text=source_path(root,name).read_bytes().decode('utf-8')
    # Фрагменты используют локальную видимость обычного C-проекта.
    text=re.sub(r'^\s*#include [\"<][^\n]*domovoi_diag\.h[\">]\s*$', '', text, flags=re.M)
    if '#include "../usb_ring_size.h"' in text:
        text=text.replace('#include "../usb_ring_size.h"',
                          (root/'User/usb_ring_size.h').read_bytes().decode('utf-8'))
    return text



def diagnostic_header(root):
    return '#define DOMOVOI_HOST_TEST 1\n#define DOMOVOI_DIAG ' + os.environ.get('DOMOVOI_TEST_DIAG','1') + '\n' + (root/'User/domovoi_diag.h').read_text(encoding='ascii') + '\n#if DOMOVOI_DIAG\n#define DIAG_CHECK(x) CHECK(x)\n#else\n#define DIAG_CHECK(x) ((void)0)\n#endif\n'


def host_adapters(root):
    # Стенды протокола задают отказы выделения памяти. Владение рабочей памятью
    # и отмена операций проверяются на связанном ELF.
    header=(root/'User/user_format.h').read_bytes().decode('utf-8')
    implementation=(root/'User/user_format.c').read_bytes().decode('utf-8').replace('#include "user_format.h"','')
    # Имена внутренних помощников форматирования не пересекаются с заглушками стенда.
    implementation=implementation.replace('text(', 'domovoi_format_text(').replace('decimal(', 'domovoi_format_decimal(')
    return (diagnostic_header(root)+'#ifndef DOMOVOI_HOST_ADAPTERS_DEFINED\n#define DOMOVOI_HOST_ADAPTERS_DEFINED\n'
            '#define work_acquire(size,slot) malloc(size)\n'
            '#define work_release(ptr,slot) free(ptr)\n'+header+'\n'+implementation+'\n#endif\n')
