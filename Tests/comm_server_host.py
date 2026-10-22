import re
"""Доработка: Кряжев А., 2026. Канал/сервер на настоящих UART, AT и SLM."""
from pathlib import Path
from source_select import source_path, source_text, host_adapters
import argparse, os, subprocess, json
from temp_cleanup import TemporaryDirectory
from comm_slm_host import unit as slm_unit, memory, ROOT, ZIG, HEADERS, MODULES

NEW = ('Modules/comm_channel.c', 'Modules/comm_wifi.inc', 'Modules/comm_server.c', 'Modules/comm_recover.c', 'Modules/comm_info.c', 'Modules/comm_call.c', 'Modules/comm_sms.c')
NEW_HEADERS = ('Modules/comm_channel.h', 'Modules/comm_server.h', 'Modules/comm_recover.h', 'Modules/comm_info.h', 'Modules/comm_call.h', 'Modules/comm_sms.h')

def source(name):
    return source_text(ROOT,name)

def function(text, name):
    import re
    m = re.search(r'(?m)^(?:static )?(?:(?:void|bool|uint8_t|uint16_t|unsigned short)\s+|(?:const char|uint8_t)\s*\*\s*)' + name + r'\s*\([^;]*?\)\s*\{', text)
    assert m, name
    depth = 1; i = m.end()
    while depth:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    return text[m.start():i]

def unit():
    rows = json.loads(source('Tests/fixtures/modem/sms-slm336.json'))['cases']
    sms_cases = ',\n'.join('{' + ','.join(json.dumps(r[k]) for k in ('number','text','pdu')) + '}' for r in rows)
    old = host_adapters(ROOT)+source('User/fast_kernels.h')+'\n'+source('User/c/fast_kernels.c').replace('#include "fast_kernels.h"','')+'\n#define COMM_CHANNEL_MODEL 1\n' + slm_unit().replace('int main(void)', 'static int slm_regressions(void)')
    old = old.replace('static void sim_tx(void)', 'static void legacy_sim_tx(void)')
    old = old.replace('  sim_tx();', '  legacy_sim_tx();')
    old = old.replace('#define usb_log_on(bit) 0', 'static unsigned net_log_mask;\n#undef usb_log_on\n#define usb_log_on(bit) (net_log_mask & (bit))')
    headers = '\n'.join(line for n in NEW_HEADERS for line in source(n).splitlines() if not line.startswith('#include "'))
    code = '\n'.join(line for n in NEW for line in source(n).splitlines() if not line.startswith('#include'))
    user = source('User/c/user_*.c')
    rx = 'void server_rx_process(void);\n' + function(source('User/c/runtime_internal.h').replace('static inline ','static '),'rt_get16') + '\n' + re.sub(r'^#include[^\n]*', '', source('User/c/user_rx.c'), flags=re.M)
    # Сохранённый сборщик и его стык QIRD; настоящие CRC/границы/отложенное исполнение.
    helpers = '\n'.join(function(user,n) for n in ('user_comm_acquire','user_comm_free','frame_begin'))
    producers = ('user_send_registrationData', 'user_send_statusPack', 'user_send_anyData', 'user_send_smsToServer')
    # Заголовок кадра общий: эталонная копия берёт слот через свой legacy_acquire.
    legacy = function(user, 'frame_begin').replace('frame_begin', 'legacy_frame_begin').replace('user_comm_acquire(', 'legacy_acquire(') + '\n'
    legacy += '\n'.join(function(user, n).replace(n, 'legacy_' + n).replace('frame_begin(', 'legacy_frame_begin(').replace('user_comm_acquire(', 'legacy_acquire(').replace('comm_tx_acquire(', 'legacy_tx_acquire(')
                       .replace('comm_tx_commit(', 'legacy_commit(').replace('packet_send(', 'legacy_packet_send(').replace('user_comm_free(', 'legacy_free(') for n in producers)
    current = '\n'.join(function(user, n) for n in producers)
    status = re.search(r'enum \{ STATUS_EVENT_MASK[^}]*\};', user).group(0)
    status += '\n'+re.search(r'static struct \{ uint8_t due, percent, code, id; \} fw_status_pending;', user).group(0)
    status += '\n'+re.search(r'static uint16_t status_reported;\s*static uint32_t status_reported_ms;', user).group(0)
    status += '\n'+re.search(r'static struct \{ char id\[10\]; uint8_t due; \} unreg_sms_pending;', user).group(0)
    status += '\n'+ '\n'.join(function(user,n) for n in ('status_event_due','status_event_sent'))
    status = 'uint8_t status_event_due(uint16_t,uint32_t);\nvoid status_event_sent(uint16_t,uint32_t);\n'+status

    status += '\n'+re.search(r'static struct \{ uint8_t state; char id\[10\]; \} reader_out;', user).group(0)
    status += '\n'+'\n'.join(function(user,n) for n in ('user_send_readedIDtoServer','reader_reply_poll'))
    status += '\n'+'\n'.join(function(source('User/c/runtime_internal.h').replace('static inline ','static '),n) for n in ('rt_put16','rt_put32'))
    packet = function(source('User/packet.c'),'packet_frame')+'\n'+function(source('User/packet.c'),'packet_send')
    # USB-резерв проверяет ELF-стенд; модель модема не передаёт через USB.
    producer_code = 'static uint8_t *work_usb_reply(uint16_t n) { (void)n; return NULL; }\nuint8_t *user_comm_acquire(uint16_t);\nbool packet_send(uint8_t*,uint16_t,uint8_t);\n' + status + helpers + packet + '\n' + packet.replace('packet_frame(', 'legacy_packet_frame(').replace('packet_send(', 'legacy_packet_send(').replace('comm_tx_commit(', 'legacy_commit(') + '\n' + legacy + '\n' + current
    producer_code += '\n' + '\n'.join(function(user,n) for n in ('user_send_statusUpdateFW','fw_status_poll','unreg_sms_note','unreg_sms_poll'))
    producer_code += '\n' + function(source('User/c/user_diag.c'), 'user_send_commDiag').replace('USB_COMM_DIAG_COMMAND', '112')
    if '/* >>> ход обновления */' in user:
        producer_code = user[user.index('/* >>> ход обновления */'):user.index('/* <<< ход обновления */')] + producer_code

    if '/* >>> уведомление об ID */' in user:
        producer_code += user[user.index('/* >>> уведомление об ID */'):user.index('/* <<< уведомление об ID */')]
    if '/* >>> ответ считывателя' in user:
        producer_code += '\n' + user[user.index('/* >>> ответ считывателя'):user.index('/* <<< ответ считывателя */')]
    old += '\n#define table_runtime_quiet(ch) ((void)(ch))\n#define fw_async_quiet(ch) ((void)(ch))\n#define flash_exec_idle() 1\n#define user_command_ready(ptr,ch) 1\n#define user_send_answer_comand(cmd,code,ch,id) CHECK(0)\nstatic struct {unsigned intent,channel,quiet,seen;} store;\n'
    # Автоматы хранения проверяются на связанном ELF; здесь модель модема
    # проверяет уведомление владельцев при смене сеанса.
    old += '\n#define flash_exec_quiet(ch) do { if(store.channel == (unsigned)(ch)) store.quiet = 1; } while(0)\n#define service_crc_quiet(ch) ((void)(ch))\n#define upload_runtime_quiet(ch) ((void)(ch))\n#define fw_service_quiet(ch) ((void)(ch))\n'
    return old + '\n' + headers + '\n' + code + '\n' + source('Tests/test_comm_channel.c') + '\n' + source('Tests/test_comm_server.c').replace('/* КОНТРОЛЬНАЯ СУММА */', function(source('Tests/reference/crc_runtime.c'), 'crc16_calcInit')+'\n'+function(source('Tests/reference/crc_runtime.c'), 'crc16_calc')).replace('/* ПРИЁМ */', rx).replace('/* ПРОИЗВОДИТЕЛИ ПАКЕТОВ */', producer_code).replace('/* ВОССТАНОВЛЕНИЕ */', source('Tests/test_comm_recover.c') + '\n' + source('Tests/test_comm_fallback.c')).replace('/* ОТКАЗ МОДЕМА ПРИ WI-FI */', source('Tests/test_comm_modem_stall.c')).replace('/* ВЫЗОВ */', source('Tests/test_link_diag.c') + source('Tests/test_comm_call.c').replace('/* ОБРАБОТЧИК ЗВОНКА ПРИЛОЖЕНИЯ */', '\n'.join(function(user, n) for n in ('user_incomingcall', 'user_comm_call')).replace('device.status.readyToUpdate', '(device.status & (1u << 11))')) + source('Tests/test_comm_sms.c').replace('/* СЦЕНАРИИ SMS SLM336 */',sms_cases) + source('Tests/test_comm_net.c') + source('Tests/test_comm_volte.c'))

def arm(out):
    names = (*HEADERS, *NEW_HEADERS, *MODULES, *NEW, 'Modules/comm_at.c', 'User/user_comm.h', 'User/c/user_comm_runtime.c')
    rows = []
    with TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for name in names:
            (tmp / Path(name).name).write_text(source(name).replace('\r\n','\n').replace('../Modules/',''), encoding='utf-8')
        (tmp/'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void*,const void*,size_t);\nvoid *memset(void*,int,size_t);\nvoid *memmove(void*,const void*,size_t);\nint memcmp(const void*,const void*,size_t);\nsize_t strlen(const char*);\nint strcmp(const char*,const char*);\nchar *strcpy(char*,const char*);\nchar *strncpy(char*,const char*,size_t);\nchar *strstr(const char*,const char*);\n', encoding='ascii')
        (tmp/'stdio.h').write_text('#include <stddef.h>\nint snprintf(char*,size_t,const char*,...);\n', encoding='ascii')
        (tmp/'user_fun.h').write_text('#include <stdint.h>\n#include <stdbool.h>\n#define CH_SERVER 1\ntypedef uint8_t logSystem_e;\nvoid user_send_registrationData(unsigned);\nvoid user_send_statusPack(unsigned);\nbool log_addSystemEvent(logSystem_e);\nuint32_t user_comm_local_time(void);\nbool user_comm_busy(void);\n#define CT_PHONE 1\ntypedef struct { struct { unsigned readyToUpdate; } status; } device_s;\nbool user_incomingcall(char*,uint8_t);\nbool user_smsreceived(char*,char*,bool*);\nbool flash_exec_read_allowed(void);\nbool user_access_ready(const char*);\nchar *user_last10digitsnumber(char*);\n',encoding='ascii')
        for name in (*MODULES, *NEW, 'Modules/comm_at.c', 'User/c/user_comm_runtime.c'):
            stem = Path(name).stem; obj = out/(stem+'.o')
            subprocess.run([ZIG,'cc','-target','thumb-freestanding-eabi','-mcpu=cortex_m3','-mthumb','-Oz','-std=c99','-Wall','-Wextra','-Werror','-fstack-usage','-I'+str(tmp),'-c',str(tmp/(stem+'.c')),'-o',str(obj)],check=True)
            ram, flash = memory(obj); rows.append(f'{stem}: RAM={ram} Flash={flash} heap=0')
    (out/'memory.txt').write_text('\n'.join(rows)+'\n', encoding='utf-8')
    print('\n'.join(rows))

def run(sanitize=False):
    out = ROOT/'Release/comm-stage4'; out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out/'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out/'zig-local')
    with TemporaryDirectory(dir=out) as tmp:
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
        ('second-command','!server_rx_ready || comm_tx_busy())','!server_rx_ready || false)'),
        ('tcp-ack','ch.sent != ch.acked || ch.tx_state != TX_FREE','ch.tx_state != TX_FREE'),
        ('dns-header-only','v[0] == 0 && v[1] > 0 ? 2 : 3','v[0] == 0 && v[1] > 0 ? 1 : 3'),
        ('event-repeat','if (!sv.flags.traffic_event) sv.flags.traffic_event = sv.handlers.no_traffic();','sv.flags.traffic_event = sv.handlers.no_traffic();'),
    )
    with TemporaryDirectory(dir=out) as tmp:
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
