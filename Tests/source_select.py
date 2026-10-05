"""Select C production inputs for source-extracting host checks."""
import os
import re
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


def source_text(root, name):
    """Select the ordinary C branch before extracting declaration fragments."""
    text=source_path(root,name).read_bytes().decode('cp1251')
    if '#include "../usb_ring_size.h"' in text:
        text=text.replace('#include "../usb_ring_size.h"',
                          (root/'User/usb_ring_size.h').read_bytes().decode('cp1251'))
    stack=[];active=True;out=[]
    for line in text.splitlines(True):
        m=re.match(r'\s*#(ifdef|ifndef|if|else|elif|endif)\b(.*)',line)
        if not m:
            if active:out.append(line)
            continue
        directive,argument=m.groups()
        if directive in ('ifdef','ifndef','if'):
            special=directive in ('ifdef','ifndef') and argument.strip()=='HYBRID_CARD'
            value=directive=='ifndef' if special else True
            stack.append([active,special,value]);active=active and value
            if not special and active:out.append(line)
        elif directive in ('else','elif'):
            if not stack:
                if active:out.append(line)
                continue
            parent,special,value=stack[-1]
            if special:stack[-1][2]=not value;active=parent and not value
            else:
                active=parent
                if active:out.append(line)
        else:
            if not stack:
                if active:out.append(line)
                continue
            parent,special,value=stack.pop();active=parent
            if not special and active:out.append(line)
    return ''.join(out)


def host_adapters(root):
    # Extracted host protocol fixtures supply allocation/error stubs. Workspace
    # exclusion, real storage lifetimes and cancellation run on the linked ELF.
    header=(root/'User/user_format.h').read_bytes().decode('cp1251')
    implementation=(root/'User/user_format.c').read_bytes().decode('cp1251').replace('#include "user_format.h"','')
    # Private production helpers share this extracted fixture translation unit
    # with host stubs (comm_at's text, for example). Keep their names local.
    implementation=implementation.replace('text(', 'domovoi_format_text(').replace('decimal(', 'domovoi_format_decimal(')
    return ('#ifndef DOMOVOI_HOST_ADAPTERS_DEFINED\n#define DOMOVOI_HOST_ADAPTERS_DEFINED\n'
            '#define work_acquire(size,slot) malloc(size)\n'
            '#define work_release(ptr,slot) free(ptr)\n'+header+'\n'+implementation+'\n#endif\n')
