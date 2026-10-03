"""Доработка: Кряжев А., 2026. Канал/сервер на настоящих UART, AT и SLM."""
from pathlib import Path
import argparse, os, subprocess, tempfile
from comm_slm_host import unit as slm_unit, memory, ROOT, ZIG, HEADERS, MODULES

NEW = ('Modules/comm_channel.c', 'Modules/comm_server.c', 'Modules/comm_recover.c', 'Modules/comm_info.c', 'Modules/comm_call.c')
NEW_HEADERS = ('Modules/comm_channel.h', 'Modules/comm_server.h', 'Modules/comm_recover.h', 'Modules/comm_info.h', 'Modules/comm_call.h')

def source(name):
    return (ROOT / name).read_bytes().decode('cp1251')

def function(text, name):
    import re
    m = re.search(r'(?m)^(?:static )?(?:void|bool|uint16_t|unsigned short)\s+' + name + r'\s*\([^;]*?\)\s*\{', text)
    assert m, name
    depth = 1; i = m.end()
    while depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[m.start():i]

def unit():
    old = '#define COMM_CHANNEL_MODEL 1\n' + slm_unit().replace('int main(void)', 'static int slm_regressions(void)')
    old = old.replace('static void sim_tx(void)', 'static void legacy_sim_tx(void)')
    old = old.replace('  sim_tx();', '  legacy_sim_tx();')
    headers = '\n'.join(line for n in NEW_HEADERS for line in source(n).splitlines() if not line.startswith('#include "'))
    code = '\n'.join(line for n in NEW for line in source(n).splitlines() if not line.startswith('#include'))
    user = source('User/user_fun.c')
    rx = user[user.index('#define SERVER_RX_CAPACITY'):user.index('// принято СМС')]
    # Сохранённый сборщик и его стык QIRD; настоящие CRC/границы/отложенное исполнение.
    helpers = user[user.index('/* Назначение кадра серверу'):user.index('void user_send_anyData(')]
    producers = ('user_send_registrationData', 'user_send_statusPack', 'user_send_anyData', 'user_send_smsToServer')
    legacy = '\n'.join(function(user, n).replace(n, 'legacy_' + n).replace('user_comm_acquire(', 'legacy_acquire(').replace('comm_tx_acquire(', 'legacy_tx_acquire(')
                       .replace('comm_tx_commit(', 'legacy_commit(').replace('user_comm_free(', 'legacy_free(') for n in producers)
    current = '\n'.join(function(user, n) for n in producers)
    producer_code = helpers + legacy + '\n' + current
    return old + '\n' + headers + '\n' + code + '\n' + source('Tests/test_comm_channel.c') + '\n' + source('Tests/test_comm_server.c').replace('/* CRC */', function(source('User/crc.c'), 'crc16_calc')).replace('/* RX */', rx).replace('/* PRODUCERS */', producer_code).replace('/* RECOVER */', source('Tests/test_comm_recover.c')).replace('/* CALL */', source('Tests/test_comm_call.c'))

def arm(out):
    names = (*HEADERS, *NEW_HEADERS, *MODULES, *NEW, 'Modules/comm_at.c', 'User/user_comm.h', 'User/user_comm.c')
    rows = []
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for name in names:
            (tmp / Path(name).name).write_text(source(name).replace('\r\n','\n').replace('../Modules/',''), encoding='utf-8')
        (tmp/'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void*,const void*,size_t);\nvoid *memset(void*,int,size_t);\nint memcmp(const void*,const void*,size_t);\nsize_t strlen(const char*);\nint strcmp(const char*,const char*);\nchar *strncpy(char*,const char*,size_t);\n', encoding='ascii')
        (tmp/'stdio.h').write_text('#include <stddef.h>\nint snprintf(char*,size_t,const char*,...);\n', encoding='ascii')
        (tmp/'user_fun.h').write_text('#include <stdint.h>\n#include <stdbool.h>\n#define CH_SERVER 1\ntypedef uint8_t logSystem_e;\nvoid user_send_registrationData(unsigned);\nvoid user_send_statusPack(unsigned);\nbool log_addSystemEvent(logSystem_e);\nuint32_t user_comm_local_time(void);\nbool user_comm_busy(void);\n#define CT_PHONE 1\nbool user_incomingcall(char*,uint8_t);\n',encoding='ascii')
        for name in (*MODULES, *NEW, 'Modules/comm_at.c', 'User/user_comm.c'):
            stem = Path(name).stem; obj = out/(stem+'.o')
            subprocess.run([ZIG,'cc','-target','thumb-freestanding-eabi','-mcpu=cortex_m3','-mthumb','-Oz','-std=c99','-Wall','-Wextra','-Werror','-fstack-usage','-I'+str(tmp),'-c',str(tmp/(stem+'.c')),'-o',str(obj)],check=True)
            ram, flash = memory(obj); rows.append(f'{stem}: RAM={ram} Flash={flash} heap=0')
    (out/'memory.txt').write_text('\n'.join(rows)+'\n', encoding='utf-8')
    print('\n'.join(rows))

def run(sanitize=False):
    out = ROOT/'Release/comm-stage4'; out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out/'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out/'zig-local')
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp=Path(tmp); (tmp/'test.c').write_text(unit().replace('\r\n','\n'),encoding='utf-8')
        subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',*(['-fsanitize=undefined','-fno-sanitize-recover=all'] if sanitize else []),str(tmp/'test.c'),'-o',str(tmp/'test.exe')],check=True)
        subprocess.run([str(tmp/'test.exe')],check=True,timeout=300)

def mutations():
    out=ROOT/'Release/comm-stage4'; out.mkdir(parents=True,exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global'); os.environ['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-local')
    code=unit().replace('\r\n','\n')
    faults=(
        ('registration-limit','sv.registrations < 2','sv.registrations < 3'),
        ('ping-zero-replies','&& v[1] > 0 && v[1] <= v[0]','&& v[1] <= v[0]'),
        ('unacked-deadline','now_ms - ch.tx_since > comm_timeouts[COMM_T_UNACKED]','now_ms - ch.tx_since > comm_timeouts[COMM_T_STABLE]'),
        ('close-before-open','ch.state = CH_CLOSE; ch.reopen_pause','ch.state = CH_PAUSE; ch.reopen_pause'),
        ('second-command','if (comm_tx_busy()) return;','if (false) return;'),
        ('tcp-ack','if (ch.answer == 1) { ch.tx_state = TX_FREE;','if (ch.answer != 0) { ch.tx_state = TX_FREE;'),
        ('dns-header-only','v[0] == 0 && v[1] > 0 ? 2 : 3','v[0] == 0 && v[1] > 0 ? 1 : 3'),
        ('event-repeat','!sv.flags.traffic_event && sv.handlers.no_traffic','sv.handlers.no_traffic'),
    )
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp=Path(tmp)
        for label,old,new in faults:
            assert old in code, (label,old)
            (tmp/'test.c').write_text(code.replace(old,new,1),encoding='utf-8')
            subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(tmp/'test.c'),'-o',str(tmp/'test.exe')],check=True)
            r=subprocess.run([str(tmp/'test.exe')],capture_output=True,timeout=300)
            (out/('mutation-'+label+'.log')).write_bytes(r.stdout+r.stderr)
            assert r.returncode!=0, ('undetected',label)
            print(label,'detected')
    print('comm_server mutations: 8/8 detected')

if __name__ == '__main__':
    p=argparse.ArgumentParser(); p.add_argument('--arm',action='store_true'); p.add_argument('--sanitize',action='store_true'); p.add_argument('--mutations',action='store_true'); a=p.parse_args()
    if a.arm:
        out=ROOT/'Release/comm-stage4'; out.mkdir(parents=True,exist_ok=True)
        os.environ['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global'); os.environ['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-local')
        arm(out)
    elif a.mutations: mutations()
    else: run(a.sanitize)
