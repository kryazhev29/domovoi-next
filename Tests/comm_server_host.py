"""Доработка: Кряжев А., 2026. Канал/сервер на настоящих UART, AT и SLM."""
from pathlib import Path
from source_select import source_path, source_text, host_adapters
import argparse, os, subprocess, tempfile
from comm_slm_host import unit as slm_unit, memory, ROOT, ZIG, HEADERS, MODULES

NEW = ('Modules/comm_channel.c', 'Modules/comm_server.c', 'Modules/comm_recover.c', 'Modules/comm_info.c', 'Modules/comm_call.c', 'Modules/comm_sms.c', 'Modules/comm_balance.c')
NEW_HEADERS = ('Modules/comm_channel.h', 'Modules/comm_server.h', 'Modules/comm_recover.h', 'Modules/comm_info.h', 'Modules/comm_call.h', 'Modules/comm_sms.h', 'Modules/comm_balance.h')

def source(name):
    return source_text(ROOT,name)

def function(text, name):
    import re
    m = re.search(r'(?m)^(?:static )?(?:(?:void|bool|uint16_t|unsigned short)\s+|const char\s*\*\s*)' + name + r'\s*\([^;]*?\)\s*\{', text)
    assert m, name
    depth = 1; i = m.end()
    while depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[m.start():i]

def unit():
    old = host_adapters(ROOT)+source('User/fast_kernels.h').replace('static inline ', 'static inline __attribute__((unused)) ')+'\n#define COMM_CHANNEL_MODEL 1\n' + slm_unit().replace('int main(void)', 'static int slm_regressions(void)')
    old = old.replace('static void sim_tx(void)', 'static void legacy_sim_tx(void)')
    old = old.replace('  sim_tx();', '  legacy_sim_tx();')
    headers = '\n'.join(line for n in NEW_HEADERS for line in source(n).splitlines() if not line.startswith('#include "'))
    code = '\n'.join(line for n in NEW for line in source(n).splitlines() if not line.startswith('#include'))
    user = source('Tests/asm/reference/User/user_fun.c')
    rx = user[user.index('#define SERVER_RX_CAPACITY'):user.index('// принято СМС')]
    # Сохранённый сборщик и его стык QIRD; настоящие CRC/границы/отложенное исполнение.
    helpers = user[user.index('/* Назначение кадра серверу'):user.index('void user_send_anyData(')]
    producers = ('user_send_registrationData', 'user_send_statusPack', 'user_send_anyData', 'user_send_smsToServer')
    legacy = '\n'.join(function(user, n).replace(n, 'legacy_' + n).replace('user_comm_acquire(', 'legacy_acquire(').replace('comm_tx_acquire(', 'legacy_tx_acquire(')
                       .replace('comm_tx_commit(', 'legacy_commit(').replace('packet_send(', 'legacy_packet_send(').replace('user_comm_free(', 'legacy_free(') for n in producers)
    current = '\n'.join(function(user, n) for n in producers)
    status = user[user.index('#define STATUS_EVENT_MASK'):user.index('/* Режим обновления, бит 11 статуса 1')]
    packet = function(source('User/packet.c'),'packet_send') if 'packet_send(' in current else ''
    producer_code = status + helpers + packet + '\n' + packet.replace('packet_send(', 'legacy_packet_send(').replace('comm_tx_commit(', 'legacy_commit(') + '\n' + legacy + '\n' + current
    producer_code += '\n' + function(user, 'user_send_statusUpdateFW')
    if '/* >>> ход обновления */' in user:
        producer_code = user[user.index('/* >>> ход обновления */'):user.index('/* <<< ход обновления */')] + producer_code

    if '/* >>> уведомление об ID */' in user:
        producer_code += user[user.index('/* >>> уведомление об ID */'):user.index('/* <<< уведомление об ID */')]
    if '/* >>> ответ считывателя' in user:
        producer_code += '\n' + user[user.index('/* >>> ответ считывателя'):user.index('/* <<< ответ считывателя */')]
    old += '\n#define table_runtime_quiet(ch) ((void)(ch))\n#define fw_async_quiet(ch) ((void)(ch))\n#define user_storage_command_ready() 1\n#define store_command_allowed(cmd) 1\nstatic struct {unsigned job,channel,quiet,seen;} store;\n'
    # Автоматы хранения проверяются на связанном ELF; здесь модель модема
    # проверяет уведомление владельцев при смене сеанса.
    old += '\n#define service_crc_quiet(ch) ((void)(ch))\n#define upload_runtime_quiet(ch) ((void)(ch))\n#define fw_service_quiet(ch) ((void)(ch))\n'
    return old + '\n' + headers + '\n' + code + '\n' + source('Tests/test_comm_channel.c') + '\n' + source('Tests/test_comm_server.c').replace('/* КОНТРОЛЬНАЯ СУММА */', function(source('Tests/asm/reference/User/crc.c'), 'crc16_calc')+'\n'+function(source('Tests/asm/reference/User/crc.c'), 'crc16_calcInit')).replace('/* ПРИЁМ */', rx).replace('/* ПРОИЗВОДИТЕЛИ ПАКЕТОВ */', producer_code).replace('/* ВОССТАНОВЛЕНИЕ */', source('Tests/test_comm_recover.c')).replace('/* ВЫЗОВ */', source('Tests/test_comm_call.c') + source('Tests/test_comm_sms.c') + source('Tests/test_comm_balance.c') + source('Tests/test_comm_volte.c'))

def arm(out):
    names = (*HEADERS, *NEW_HEADERS, *MODULES, *NEW, 'Modules/comm_at.c', 'User/user_comm.h', 'Tests/asm/reference/User/user_comm.c')
    rows = []
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for name in names:
            (tmp / Path(name).name).write_text(source(name).replace('\r\n','\n').replace('../Modules/',''), encoding='utf-8')
        (tmp/'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void*,const void*,size_t);\nvoid *memset(void*,int,size_t);\nvoid *memmove(void*,const void*,size_t);\nint memcmp(const void*,const void*,size_t);\nsize_t strlen(const char*);\nint strcmp(const char*,const char*);\nchar *strncpy(char*,const char*,size_t);\nchar *strstr(const char*,const char*);\n', encoding='ascii')
        (tmp/'stdio.h').write_text('#include <stddef.h>\nint snprintf(char*,size_t,const char*,...);\n', encoding='ascii')
        (tmp/'user_fun.h').write_text('#include <stdint.h>\n#include <stdbool.h>\n#define CH_SERVER 1\ntypedef uint8_t logSystem_e;\nvoid user_send_registrationData(unsigned);\nvoid user_send_statusPack(unsigned);\nbool log_addSystemEvent(logSystem_e);\nuint32_t user_comm_local_time(void);\nbool user_comm_busy(void);\n#define CT_PHONE 1\nbool user_incomingcall(char*,uint8_t);\nbool user_smsreceived(char*,char*);\nbool user_storage_flash_ready(void);\n',encoding='ascii')
        for name in (*MODULES, *NEW, 'Modules/comm_at.c', 'Tests/asm/reference/User/user_comm.c'):
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
        ('second-command','|| comm_tx_busy()) return;','|| false) return;'),
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
