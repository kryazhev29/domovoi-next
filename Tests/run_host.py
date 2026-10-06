# Доработка: Кряжев А., 2026
import os, pathlib, subprocess, sys, tempfile, re
from source_select import source_path, source_text, host_adapters
# DOMOVOI_ROOT runs the tests against a copy, e.g. with a deliberate fault.
ROOT=pathlib.Path(os.environ.get('DOMOVOI_ROOT') or pathlib.Path(__file__).resolve().parents[1])
ZIG=__import__('os').environ.get('DOMOVOI_ZIG', r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe')
ref=sys.argv[1] if len(sys.argv)>1 and sys.argv[1]!='work' else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return subprocess.check_output(['git','show',ref+':'+name.removeprefix('Tests/asm/reference/')],cwd=ROOT).decode('cp1251') if ref else source_text(ROOT,name)
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:const char \*|char\s*\*\s*|uint8_t\s*\*\s*|(?:void|bool|int|RESULT|uint8_t|uint16_t|uint32_t|unsigned short|unsigned int|pInt8U|Int8U|Int16U|cmdCode_e|logControl_e)\s+)'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    body=text[m.start():i]
    if name=='crc16_ffInit' and 'sc_ff_powers' in body:
        body=source('User/c/service_ff.inc')+'\n'+body
    return body
def typedefs(text,*names):
    # Packed like structures.h, so the tests see the real layouts and numbers.
    out=[]
    for name in names:
        m=re.search(r'typedef\s+(?:enum|struct)\s*\{(?:[^{}]|\{[^{}]*\})*\}\s*'+name+r'\s*;',text)
        assert m,name
        out.append(m.group(0))
    return '#pragma pack(push,1)\n'+'\n'.join(out)+'\n#pragma pack(pop)\n'
def cases(user):
    parse=user[user.index('void user_parse_comand'):]
    return lambda n:parse[parse.index('\r\n    case %d:'%n):parse.index('\r\n    case %d:'%(n+1))]
def sync_end(user):
    return user[user.index('/* >>> Конец синхронизации'):user.index('/* <<< Конец синхронизации */')]
def sync_stub(user):
    return re.search(r'enum \{ SYNC_ID[^}]*\};',user).group(0)+'\n#define sync_packet(b) ((void)(b))\n'
def config_helpers(user):
    return ''.join(function(user,n)+'\n' for n in ('config_exchange','config_terminate_strings') if 'static void '+n+'(' in user)

with tempfile.TemporaryDirectory(prefix='domovoi-host-') as d:
    d=pathlib.Path(d)
    if kind=='storage':
        # Check physical erase-sector footprints, not just logical byte ranges.
        mapping=source('Tests/asm/reference/User/user_fun.c')
        ranges=[]
        for name,start,size in re.findall(r'mem\[(MB_\w+)\]\.start = (\d+);\s*mem\[\1\]\.size = (\d+);',mapping):
            start,size=int(start),int(size)
            if size:ranges.append((name,start&~4095,(start+size+4095)&~4095))
        def constant(text,name):
            value=re.search(r'^#define\s+'+name+r'\s+(0x[0-9A-Fa-f]+|[0-9]+)u?\s*$',text,re.M)
            assert value,name
            return int(value[1],0)
        header=source('User/memory.h'); boot=source('Bootloader/boot_core.h')
        occupied=[(constant(header,n),constant(header,n)+constant(header,'MEM_CONFIG_SLOT_SIZE')) for n in ('MEM_CONFIG_SLOT0','MEM_CONFIG_SLOT1')]
        if '#include "sector_journal.h"' in source('Tests/asm/reference/User/memory.c'):
            journal=source('User/sector_journal.h')
            occupied += [(constant(journal,n),constant(journal,n)+constant(journal,'SJ_SECTOR')) for n in ('SJ_DATA','SJ_META')]
        boot_end=constant(boot,'BOOT_BACKUP')+constant(boot,'BOOT_SLOT_SIZE')
        for i,(lo,hi) in enumerate(occupied):
            assert lo%4096==hi%4096==0 and lo>=boot_end and hi<=0x200000
            for name,a,b in ranges:assert hi<=a or lo>=b,(name,hex(a),hex(b))
            for a,b in occupied[i+1:]:assert hi<=a or lo>=b
        print('Flash map: config banks and scratch sectors outside all rounded mem[] and boot areas',flush=True)
        code=source('Tests/asm/reference/User/memory.c').replace('#include "memory.h"','').replace('#include "sector_journal.h"','')
        header=source('User/memory.h')
        defines='\n'.join(x for x in header.splitlines() if x.startswith(('#define MEM','typedef uint8_t (*mem_page_reader','bool mem_patch','uint8_t mem_config_result')))
        test=(ROOT/'Tests/test_storage.c').read_text()
        legacy=subprocess.check_output(['git','show','203c55b:User/user_fun.c'],cwd=ROOT).decode('cp1251')
        legacy_code='\n'.join(function(legacy,n) for n in ['user_writeConfigAndDevice','user_readconfig'])
        for n in ['user_writeConfigAndDevice','user_readconfig','user_setdefault','user_saveindex','mem_read','mem_write','mem_erase_sector']:
            legacy_code=re.sub(r'\b'+n+r'\b','legacy_'+n,legacy_code)
        reset='#define TEST_STORAGE_BOOT() (cfg_legacy_checked = 0)\n' if 'cfg_legacy_checked' in code else '#define TEST_STORAGE_BOOT() ((void)0)\n'
        shared=source('User/sector_journal.h')+'\n'+source('User/sector_journal.c').replace('#include "sector_journal.h"','')+'\n' if '#include "sector_journal.h"' in source('Tests/asm/reference/User/memory.c') else ''
        unit=test.replace('/* PRODUCTION */',defines+'\n'+shared+code+'\n'+reset+function(source('Tests/asm/reference/User/user_fun.c'),'user_writeConfigAndDevice')+'\n'+legacy_code)
    elif kind in ('sectorjournal','bootjournal'):
        code=source('Tests/asm/reference/User/memory.c').replace('#include "memory.h"','').replace('#include "sector_journal.h"','')
        header=source('User/memory.h')
        defines='\n'.join(x for x in header.splitlines() if x.startswith(('#define MEM','typedef uint8_t (*mem_page_reader','bool mem_patch','uint8_t mem_config_result')))
        shared=''
        if '#include "sector_journal.h"' in source('Tests/asm/reference/User/memory.c'):
            shared=source('User/sector_journal.h')+'\n'+source('User/sector_journal.c').replace('#include "sector_journal.h"','')+'\n'
            code=code.replace('#include "sector_journal.h"','')
        boot_test=''
        if kind=='bootjournal':
            platform=source('Bootloader/boot_platform.c')
            for name in ('sj_io_read','sj_io_write','sj_io_erase','sj_io_service'):
                code=re.sub(r'\b'+name+r'(?=\()', 'app_'+name, code)
            adapters='\n'.join(function(platform,n) for n in ('sj_io_read','sj_io_write','sj_io_erase','sj_io_service','boot_ext_erase','boot_ext_write'))
            defines+='\n#define BOOT_BACKUP 0x21000u\n#define BOOT_SLOT_SIZE 0x20000u\n'
            code+='\nstatic int storage_read(uint32_t a,void *p,uint32_t n) { return w25_read_data(a,p,(uint16_t)n); }\nstatic int storage_write(uint32_t a,const void *p,uint32_t n) { return w25_write_data(a,(uint8_t*)p,(uint16_t)n); }\nstatic int storage_erase(uint32_t a) { return w25_erase_sector(a); }\nstatic void boot_service(void) { services++; }\n'+adapters
            boot_test='CHECK(!boot_ext_erase(SJ_DATA) && !boot_ext_write(SJ_DATA,after,32) && !sj_io_erase(0x20000u) && !sj_io_write(0x21000u,after,32) && !sj_io_read(0xffffffffu,image,2));'
        expected=[]
        mapping=source('Tests/asm/reference/User/user_fun.c')
        for name,start,size in re.findall(r'mem\[(MB_\w+)\]\.start = (\d+);\s*mem\[\1\]\.size = (\d+);',mapping):
            if name in ('MB_ID','MB_GROUP','MB_SHEDULE','MB_HOLIDAY','MB_TEMPID'):
                start,size=int(start),int(size);expected.append('{'+str(start&~4095)+'u,'+str((start+size+4095)&~4095)+'u}')
        layout='static const uint32_t expected[][2] = {'+','.join(expected)+'};'
        unit=(ROOT/'Tests/test_sector_journal.c').read_text(encoding='utf8').replace('/* PRODUCTION */',defines+'\n'+shared+code).replace('/* EXPECTED DESTINATIONS */',layout).replace('/* BOOT_ADAPTER_TEST */',boot_test)

    elif kind in ('photo','rs485cfg'):
        user=source('Tests/asm/reference/User/user_fun.c'); case=cases(user)
        assert 'len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR' in case(7)
        assert 'if (len == 3 || len == 9)' in case(21)
        assert 'photo_request(cch, uniqId, NULL, 0)' in function(user,'user_send_Photos')
        assert 'photo_poll();' in function(user,'port_check_data')
        assert 'photo_connection_lost();' in function(user,'user_comm_session')
        assert '(cmd == 7 || cmd == 21) ? 5 : 255' in function(user,'user_send_answer_comand')
        code=source('Tests/asm/reference/User/photo.c').replace('#include "photo.h"','').replace('#include "sector_journal.h"','')
        code=function(source('User/sector_journal.c'),'crc32_update')+'\n'+code
        code=code.replace('#include "../Modules/comm_channel.h"','')
        code=source('Tests/comm_photo_slot.c')+'\n'+code
        code += '\n'+function(user,'user_send_Photos')+'\nstatic void command(uint8_t cmd,uint16_t len,uint8_t address,uint8_t uniqId){ uint8_t data[11]={0}; uint8_t *ptr=data; dataChanel_e cch=CH_SERVER; ptr[5]=address; switch(cmd){'+case(7)+case(21)+'default: break;}}'
        unit=source('Tests/test_photo.c').replace('/* PRODUCTION */',function(user,'user_getPhotoCount')+'\n'+code)
        if kind=='rs485cfg':
            types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s')
            unit=re.sub(r'typedef struct \{ uint8_t rs485Mode, reserv\[4\]; periphery_s rs485Periphery\[8\]; \} config_s;',types,unit)
            unit=unit.replace('uint8_t readyToUpdate, readyPhoto;', 'uint8_t readyToUpdate, readyPhoto, defaultConfig;').replace('uint32_t datetime, newAddressPhoto;', 'uint32_t datetime, datetimeConfig, newAddressPhoto; uint16_t crc;')
            extra=config_helpers(user)+sync_stub(user)+source('Tests/test_rs485cfg.c').replace('/* COMMAND12 */',case(12))
            unit=unit.replace('int main(int argc,char **argv) {',extra+'\nint main(int argc,char **argv) {\n test_rs485cfg();')
    elif kind=='fwstart':
        user=source('Tests/asm/reference/User/user_fun.c')
        case=cases(user)
        # Call sites: commands 19 and 20 go through the tested functions, command 4 carries their code.
        c19=case(19);c20=case(20)
        assert '(len - 3) == 16' in c19 and 'cmdCode_e started = fw_start(&ptr[5], cch);' in c19 and 'store_command_start(19, cch, uniqId)' in c19,'command 19'
        assert 'fw_take_block(&ptr[5], (uint16_t)(len - 3), cch);' in c20 and 'FW_BLOCK_SKIP' not in user and 'user_send_statusUpdateFW(ptr[4], code, cch, uniqId);' in c20,'command 20'
        # Command 2: a download is cancelled before the reset, a rollback or install is left alone;
        # reset now goes through timeToReset so answer 11 is sent first.
        c2=case(2)
        assert c2.index('if (device.rebootToUpdate){')<c2.index('fw_abort("CANCEL");')<c2.index('device.timeToReset = 2;'),'command 2 during an update'
        # A failed install or trial goes to the journal after registration, command 4 is not sent.
        c1=case(1)
        assert 'app_boot_error_text(reason);' in c1 and 'app_boot_error_reported();' in c1 and 'user_send_statusUpdateFW' not in c1,'boot error after registration'
        # Timing: one command 13 in flight for 20 s, one 10-minute limit for the whole download, saved every minute.
        assert '#define FW_BLOCK_RETRY_MS       20000u' in user and '#define FW_TIMEOUT_S            600u' in user,'timing constants'
        loop=function(user,'user_loop')
        assert '>= FW_BLOCK_RETRY_MS){' in loop and 'fw_end("TIMEOUT", uniqIDCnt++);' in loop and 'updateTimeout % 60u == 0' in loop and 'fw_end("INSTALL", uniqIDCnt++);' in loop,'loop'
        assert 'fw_idle' not in user and '15000' not in loop,'old limits'
        assert 'updateTimeout' not in function(user,'fw_take_block'),'a block must not touch the limit'
        saved=function(user,'user_readsavedinformation')
        assert 'fw_rollback_poll();' in loop and 'fw_status_poll();' in loop,'deferred firmware work is polled'
        assert 'device.FWblockIdx = DV.FWblockIdx;' in saved,'resume uses saved progress'
        assert 'device.FWinProgress = DV.FWinProgress;' in saved and 'device.updateTimeout = DV.updateTimeout;' in saved and saved.index('fw_resume_after_boot();')>saved.index('while (read_count--)'),'resume at boot'
        reset=function(user,'user_factoryReset')
        assert reset.index('device.FWinProgress = 0;')<reset.index('user_setdefault();'),'factory reset cancels a download'
        types=typedefs(source('structures.h'),'logSystem_e','logControl_e','logUpdate_e','cmdCode_e','dataChanel_e','bootFWparam_s')
        defines='\n'.join(x for x in user.splitlines() if x.startswith('#define FW_'))+'\n'
        helpers=user[user.index('/* Режим обновления, бит 11 статуса 1'):user.index('// Разбор данных')]
        # Доработка: Кряжев А., 2026. Исполняем реальные участки цикла, не модель таймеров.
        poll=loop[loop.index('    if (device.registredOnServer && device.FWinProgress){'):loop.index('  // каждые 100 мс')]
        poll=poll[:poll.rfind('  }')]
        tick=loop[loop.index('    if (device.updateTimeout){'):loop.index('    if (app_trial_expired()){')]
        audit='static void audit_poll(void){\n'+poll+'\n}\nstatic void audit_tick(void){\n'+tick+'\n}\n'
        unit=(ROOT/'Tests/test_fw_start.c').read_text(encoding='utf-8').replace('/* TYPES */',types).replace('/* PRODUCTION */',defines+helpers).replace('/* AUDIT LOOP */',audit).replace('/* FIRMWARE CRC */', function(user,'CRC32')+'\n'+function(user,'user_checkFirmware'))
    elif kind=='log':
        user=source('Tests/asm/reference/User/user_fun.c');st=source('structures.h')
        case=cases(user)
        # The controller writes no command or exchange events (answers of 16.09, sheet E525).
        assert 'log_addComandEvent' not in user and 'log_addTransferEvent' not in user and 'LOG_CMD_' not in st and 'LOG_TR_' not in st and 'LT_SYSTEM' not in st,'command and exchange events'
        # Command 8: the source picks the event before the outputs; the admin test is logged too.
        c8=case(8)
        assert c8.index('server_out_event = cmd8_event(ptr[5]);')<c8.index('user_activateOut((ptr[6] == 1)'),'command 8 source'
        # The source only picks the journal event: the admin test goes the same way as any source.
        assert 'LOG_CON_OUTONBYTEST' not in c8 and 'case 3: return LOG_CON_OUTONBYTEST;' in user,'admin test logged'
        # Protokol2 row 135: registration answers 4 and 5 reset to factory settings; row 537: no code 5 in answer 11.
        assert '#define REG_UNKNOWN_IMEI        4' in user and '#define REG_NOT_BOUND           5' in user and 'case REG_UNKNOWN_IMEI:' in case(1) and 'case REG_NOT_BOUND:' in case(1) and 'CMD_UNDEFIMEI' not in case(1),'registration answers'
        assert function(user,'user_parse_comand').count('CMD_RUNNING')==1 and 'CMD_RUNNING' not in case(2),'no code 5 for delayed reset'
        assert 'device.protocolVersion = DV.protocolVersion;' not in user,'protocol version is constant'
        # Protokol2 row 284: an expired ID does not open, 130 is gone (138 tells the end); a sync of the same card
        # keeps the first pass and the 138 mark; the end is checked every second and recounted on new server time.
        assert 'if (id_expired(&cellId)){' in user and 'LOG_CON_IDEXPIRED' not in user,'use time expired'
        sync_body=function(user,'sync_page') if 'static uint8_t sync_page(' in user else case(13)
        assert 'id_watch_end(id_time_end(old));' in sync_body and 'user_checkIdTimeEnd();' in user and 'id_endScan = true;' in function(user,'log_setTime'),'use time end checked'
        assert 'memcpy(old->reserve, first, 4); old->reserve[6] = done;' in sync_body and 'memcmp(old->ID,' in sync_body,'use time kept on sync'
        assert 'log_addControlEvent(server_out_event, id, cnt);' in function(user,'user_activateOut'),'server event from its source'
        # Journal sheet 131: after every pass of an ID with limited passes; 129: photo number when the photo is stored.
        assert 'if (cnt != 255) log_addControlEvent(LOG_CON_PASSCOUNT, id, cnt);' in function(user,'user_activateOut'),'pass counter event'
        assert 'log_addPhotoEvent(photo->stamp);' in source('Tests/asm/reference/User/photo.c'),'photo event'
        # Journal upload: the empty answer is 3 bytes (0 left, 0 in packet), sent before the erase; used sectors only.
        up=function(user,'user_send_logRecords')
        assert up.count('user_send_anyData(10, empty, sizeof(empty), cch, uniqId);')==1 and 'uint8_t empty[3] = {0, 0, 0};' in up and 'tmp16' not in up,'empty journal answer'
        assert up.index('upload_finish(10)')<up.index('user_send_anyData(10, empty'),'Q5: all records confirmed before erase/empty answer'
        clr=function(user,'log_clearLogBuffer')
        assert 'endAdr = device.newAddressLog;' in clr and 'mem[MB_LOG].start & ~(uint32_t)4095' in clr,'used sectors only'
        pend=user[user.index('/* >>> отложенные события журнала'):user.index('/* <<< отложенные события журнала */')]
        pend+=user[user.index('/* >>> номер записи журнала'):user.index('/* <<< номер записи журнала */')]
        # Q5, согласовано владельцем: одинаковое подтверждение и стирание по USB/серверу.
        assert 'cch == CH_USB ? log_usbReadIndex : device.readLogIDIndex' in function(user,'upload_index'),'separate USB journal position'
        assert 'if (!upload_ack(10, cch)) return;' in up and 'sent ? readIdx + cntId : 0' in up,'Q5: next request confirms, transport failure retains cursor'
        assert 'cmd == 10 ? log_clearLogBuffer() : user_clearTempIdBuffer()' in function(user,'upload_finish'),'Q5: both channels erase only after final ACK'
        # USB commands other than ident, status, PIN and USB diagnostics need the PIN.
        pc=function(user,'user_parse_comand')
        assert pc.index('CMD_CRCERROR')<pc.index('if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4])){')<pc.index('switch (ptr[4])'),'PIN gate'
        allowed = 'return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_DIAG_COMMAND'
        if '#define USB_COMM_DIAG_COMMAND ' in user: allowed += ' || cmd == USB_COMM_DIAG_COMMAND'
        assert allowed + ';' in user and 'device.userAutorized = 0; // кабель вынут' in user,'PIN scope'
        # Command 8: a requested output that did not switch answers 2, not success.
        assert 'res8 = CMD_RUNERROR;' in c8 and 'user_send_answer_comand(ptr[4], res8, cch, uniqId);' in c8 and 'CMD_SUCCESS, cch' not in c8,'command 8 answer'
        # Каждое событие листа (таблица test_log.c) где-то пишется. Вторые серверы и 7-10
        # получаются сдвигом от первого номера; 34 - функции нет в настройках и протоколе (вопрос заказчику).
        writers=user+source('Tests/asm/reference/User/user_comm.c')+source('Tests/asm/reference/User/photo.c')
        derived={'LOG_SYS_CONNECT2':'LOG_SYS_CONNECT1','LOG_SYS_ERRCONNECT2':'LOG_SYS_ERRCONNECT1','LOG_SYS_FAILCONNECT2':'LOG_SYS_FAILCONNECT1',
                 'LOG_SYS_ENDCONFUPDATE':'LOG_SYS_ENDIDUPDATE + i','LOG_SYS_ENDGROUPUPDATE':'LOG_SYS_ENDIDUPDATE + i',
                 'LOG_SYS_ENDSHEDULEUPDATE':'LOG_SYS_ENDIDUPDATE + i','LOG_SYS_ENDHOLLIDAYUPDATE':'LOG_SYS_ENDIDUPDATE + i'}
        not_written={'LOG_SYS_RESETONSHEDULE'}
        names=re.findall(r'\{(LOG_\w+), \d+,',(ROOT/'Tests/test_log.c').read_text())
        assert len(names)>=55,'journal table'
        missing=[n for n in names if n not in not_written and n not in writers and not (n in derived and derived[n] in writers)]
        assert not missing,('journal events never written',missing)
        assert 'event = (uint8_t)(event + user_comm_server(address));' in user,'second server by offset'
        code=function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')+'\n'+pend+'\n'.join(function(user,n) for n in ['log_addSystemEvent','log_addSystemEventID','log_addControlEvent','log_addPhotoEvent','log_addUpdateEvent'])
        unit=(ROOT/'Tests/test_log.c').read_text().replace('/* TYPES */',typedefs(st,'logSystem_e','logControl_e','logUpdate_e','cellLog_s')).replace('/* PRODUCTION */',code)
    elif kind=='logcommit':
        user=source('Tests/asm/reference/User/user_fun.c')
        names=['log_blank','log_valid','log_uniqueIndex','log_addEvent','log_getRecordsCount','log_readRecords']
        code='static uint32_t log_lastIndex;\n'+function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')+'\n'
        code+='\n'.join(function(user,n) for n in names if re.search(r'(?m)^(?:static )?(?:bool|void|uint16_t) '+n+r'\(',user))
        unit=(ROOT/'Tests/test_log_commit.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellLog_s')).replace('/* PRODUCTION */',code)
        if 'static bool log_readRecords(' not in user:
            unit=unit.replace('/* PRODUCTION */',code)
            unit=unit.replace('static void reset(void)', 'static bool log_readRecords(uint16_t first,uint8_t *p,uint16_t count){return mem_read(4096+first*32,p,count*32);}\nstatic void reset(void)')
    elif kind=='countends':
        user=source('Tests/asm/reference/User/user_fun.c')
        code='static uint32_t log_lastIndex;\n'+function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')+'\n'
        code+='\n'.join(function(user,n) for n in ['log_blank','log_valid','log_getRecordsCount','user_getTempIDRecordsCount'])
        unit=(ROOT/'Tests/test_count_ends.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellLog_s','cellID_s')).replace('/* PRODUCTION */',code)
    elif kind=='access':
        user=source('Tests/asm/reference/User/user_fun.c');st=source('structures.h')
        case=cases(user)
        # Relays (Д-07): permissions are worked out before any way of opening, both relays never forced.
        check=function(user,'user_checkIdAndControl')
        assert check.index('bool onOut[2]')<check.index('if (user_findTempID(id)){'),'relay permissions first'
        assert 'user_activateOut(0,' not in user and 'user_activateOut(1,' not in user and check.count('user_activateOut(onOut[0], onOut[1], tp, id);')==5 and check.count('user_activateOut(')==5,'requested relays only'
        c8=case(8)
        assert 'user_activateOut((ptr[6] == 1) && config.rele[0].onCmdServer,' in c8 and '(ptr[7] == 1) && config.rele[1].onCmdServer, CT_SERVER, tempStrBuffer))' in c8,'command 8 outputs'
        # Protokol2 rows 185-188: 0 switches an allowed output off; values above 1 are a parameter error.
        assert 'if (ptr[6] == 0 && config.rele[0].onCmdServer) out_load(OUT1, OUT_OFF, 0, 0, 0);' in c8 and 'if (ptr[7] == 0 && config.rele[1].onCmdServer) out_load(OUT2, OUT_OFF, 0, 0, 0);' in c8 and 'res8 = CMD_PARAMERROR;' in c8,'command 8 off'
        # Schedule time (Д-09) in both places.
        assert 'flag = shedule_time_ok(&shedule);' in check and 'flag = shedule_time_ok(&shedule);' in function(user,'user_checkShedule') and 'device.minute >= shedule.startMinute' not in user,'schedule window'
        # ID base end (Д-11): recounted at start, after a sync and after clearing.
        assert 'device.numbersCount = user_getNumbersCount();' in case(13) and 'device.numbersCount++' not in user,'count after sync'
        c103=user[user.index('    case 103:'):user.index('    case 105:')]
        assert c103.index('user_clearMemBlock(MB_ID)')<c103.index('device.numbersCount = user_getNumbersCount();'),'count after clearing'
        # Settings (Д-13): an empty server 2 is accepted.
        c12=case(12)
        assert 'if ((p[8] == 0) || (p[137] == 0)){' in c12 and 'p[40]' not in c12,'server 2 may be empty'
        # Factory network type automatic (Д-24), protocol version 1 (Д-06, Протокол2 E424).
        assert 'config.netType = 0;' in function(user,'user_setdefault') and 'NT_GSM' not in function(user,'user_setdefault'),'automatic network'
        init=function(user,'user_initvariables')
        assert '#define PROTOCOL_VERSION        (uint8_t)1' in user and init.index('user_readsavedinformation();')<init.index('device.protocolVersion = PROTOCOL_VERSION;'),'protocol version'
        # Status answers: three misses in a row, an answer to status 1 restarts the run.
        c24=user[user.index('    case 24:'):user.index('    case 27:')]
        assert '#define MAXSTATUSANSWERERROR    (uint8_t)3' in user and '++device.lmCount >= MAXSTATUSANSWERERROR' in user,'three misses'
        assert 'device.lmCount = 0;' in c24 and 'switch (ptr[5])' not in c24,'answer 24'
        # Settings sector: device saves take the settings from RAM; the modem type is saved on a change only.
        assert 'user_writeConfigAndDevice();' in function(user,'user_saveindex') and 'mem_rewrite' not in function(user,'user_saveindex'),'saveindex from RAM'
        assert 'device.moduleType = type;' in user,'module type from the modem'
        types=typedefs(st,'logControl_e','codeType_e','releMode_s','cellShedule_s','cellID_s')
        # Passes per day: the counter restores on the first pass of a new day; a record from the server is today's.
        sync_body=function(user,'sync_page') if 'static uint8_t sync_page(' in user else cases(user)(13)
        assert 'user_reloadPassCounter' not in user and 'id_set_pass_day(old, id_today());' in sync_body,'passes per day'
        code=re.search(r'(?m)^static uint16_t id_cells_end;',user).group(0)+'\n'
        code+=user[user.index('/* >>> время действия ID'):user.index('/* <<< проходы в сутки */')]
        code+='\n'.join(function(user,n) for n in ['shedule_time_ok','user_findID','user_getNumbersCount','user_checkIdTimeEnd','user_activateOut'])
        unit=(ROOT/'Tests/test_access.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='rules':
        user=source('Tests/asm/reference/User/user_fun.c')
        rules=user[user.index('/* Статус 1 вне очереди (таблица E92'):user.index('/* Режим обновления, бит 11 статуса 1')]
        defines=[x for x in user.splitlines() if x.startswith('#define TIMETOSTATUSRESPONSE')][0]+'\n'
        # Call sites: each rule must be applied where the table puts it.
        loop=function(user,'user_loop')
        assert loop.index('status_event_due(st, timers_getMillis())')>loop.index('device.status.stateOut2 = out_getstate(OUT2);'),'status out of turn after inputs and outputs are read'
        case=cases(user)
        assert 'status_event_sent(st, timers_getMillis());' in case(1),'registration does not trigger an extra status'
        c8=case(8);head=c8[:c8.index('if (ptr[5] != 0 &&')]
        assert 'if (fw_update_quiet()){' in head and 'CMD_RUNERROR' in head,'command 8 refused in update mode'
        sms=function(user,'user_smsreceived')
        gate=sms[sms.index('if (fw_update_quiet()'):sms.index('if (strcmp(cmd, "add") == 0){')]
        assert all('strcmp(cmd, "%s") == 0'%c in gate for c in ('add','reset','setout','default')) and '"imei"' not in gate and '"status"' not in gate,'SMS control commands refused in update mode'
        call=user[user.index('bool user_incomingcall (char* num, uint8_t typ)'):user.index('void user_comanderror')]
        assert 'fw_update_quiet' not in call,'calls keep working in update mode'
        # Events 6-10: settings count as a sync packet once saved; the quiet timer ticks every second.
        c12=case(12);assert 'store_config_start()' in c12 and 'sync_packet(SYNC_CONFIG);' in function(user,'store_config_complete') and 'if (result == 2)' in function(user,'user_storage_poll'),'event 7 after verified settings'
        assert 'sync_end_tick();' in loop,'sync end ticks in the main loop'
        # Block request (13): the retry clock starts only when the request left; a busy slot retries in 50 ms, not 20 s.
        assert 'if (user_getFWblockFromServer((dataChanel_e)device.cchFWupdate, device.FWblockIdx))' in loop,'block request not lost'
        # Update events of the journal sheet (system 1-5 struck out): 132 start, 133 failure with its
        # reason code, 134 time limit, 135 just before the install restart, 136 trial confirmed, 137 rollback.
        rest=user
        for n in ('ENDIDUPDATE','ENDCONFUPDATE','ENDGROUPUPDATE','ENDSHEDULEUPDATE','ENDHOLLIDAYUPDATE'): rest=rest.replace('LOG_SYS_'+n,'')
        assert 'UPDATE)' not in rest,'no system update events'
        assert user.count('fw_log(LOG_FW_START, NULL);')==1 and 'fw_log(LOG_FW_INSTALL, NULL);\r\n        user_savereset();' in loop,'events 132 and 135'
        assert 'app_trial_confirm();\r\n          if (!app_trial_active()) fw_log(LOG_FW_DONE, NULL);' in case(1),'event 136 on confirmation'
        assert 'fw_log(strcmp(reason, "TRIAL FAIL") == 0 ? LOG_FW_ROLLBACK : LOG_FW_FAIL, reason);' in case(1),'bootloader result: 137 after a failed trial'
        assert '0x48570000+DOMOVOI_APP_HW' in source('CMSIS/STM32F10x/Source/startup_stm32f10x_md.s'),'board marker in the vector table'
        assert source('DomovoiNext.ewp').count('<state>DOMOVOI_APP_HW=139</state>')==2 and 'DOMOVOI_APP_HW' not in source('Bootloader/DomovoiBoot.ewp'),'board marker only in the application build'
        # Только v5: сборок для других плат нет (решение владельца 03.10.2026).
        assert 'BOARD_IP_V3' not in source('DomovoiNext.ewp') and 'Release_140' not in source('DomovoiNext.ewp'),'v5 only'
        unit=(ROOT/'Tests/test_update_rules.c').read_text().replace('/* PRODUCTION */',defines+rules+function(user,'user_send_statusPack'))
    elif kind=='serverrx':
        user=source('Tests/asm/reference/User/user_fun.c')
        for name in ['user_comm_session','user_comm_rx_done']:
            assert 'server_rx_reset();' in function(user,name),name
        assert function(source('Tests/asm/reference/User/user_comm.c'),'user_comm_poll').count('server_rx_begin_pass();') == 1
        code=user[user.index('#define SERVER_RX_CAPACITY'):user.index('/* QIRD копирует прямо в прежний сборщик')]
        unit=(ROOT/'Tests/test_server_rx.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='fwblockbounds':
        user=source('Tests/asm/reference/User/user_fun.c');case=cases(user)
        defines='\n'.join(x for x in user.splitlines() if x.startswith('#define FW_'))+'\n#define FWBLOCK_OFFSET 0x200u\n'
        code=function(user,'fw_take_block')+'\nvoid command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){ switch(ptr[4]) {'+case(20)+'default: abort(); }}'
        types=typedefs(source('structures.h'),'dataChanel_e','logUpdate_e','bootFWparam_s')
        unit=(ROOT/'Tests/test_fw_block_bounds.c').read_text().replace('/* TYPES */',types).replace('/* DEFINES */',defines).replace('/* PRODUCTION */',code)
    elif kind=='cmdlengths':
        user=source('Tests/asm/reference/User/user_fun.c');parse=function(user,'user_parse_comand')
        types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s','status_s','triggers_s','device_s','dataChanel_e','cmdCode_e','logSystem_e','logUpdate_e')
        helper=function(user,'command_args_valid')+'\n' if 'static bool command_args_valid(' in user else ''
        labels=list(re.finditer(r'^    (?:case [^:]+:|default:)',parse,re.M))
        wanted=(1,2,3,6,18,23,102,105,108); selected=[]
        for i,m in enumerate(labels[:-1]):
            if m[0].strip().split()[1].rstrip(':').isdigit() and int(m[0].strip().split()[1].rstrip(':')) in wanted:
                selected.append(parse[m.start():labels[i+1].start()])
        assert len(selected)==len(wanted)
        defines='\n'.join(x for x in user.splitlines() if x.startswith(('#define REG_UNKNOWN_IMEI','#define REG_NOT_BOUND','#define USB_COMM_DIAG_COMMAND')))
        code=defines+'\n'+helper+function(user,'usb_cmd_without_pin')+'\n'+parse[:parse.index('  switch (ptr[4])')]+'  switch(ptr[4]){'+''.join(selected)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_command_lengths.c').read_text().replace('/* TYPES */',types).replace('/* CRC */',function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')).replace('/* PRODUCTION */',code)
    elif kind=='cfgstrings':
        user=source('Tests/asm/reference/User/user_fun.c');case=cases(user)
        types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s','status_s','triggers_s','device_s','memBlock_e','dataChanel_e','cmdCode_e','shtamp_e')
        restore=''
        if 'static void upload_restore(' in user:
            restore='static uint16_t log_usbReadIndex,temp_usbReadIndex;\n'+function(user,'upload_store')+'\n'+function(user,'upload_restore')+'\n'
        if 'log_ring_checkpoint(' in restore:
            restore='static void log_ring_checkpoint(void){}\nstatic void log_ring_restore(const device_s *p){(void)p;}\n'+restore
        if 'sched16_restore(&DV)' in user:
            schedule=source('User/table_runtime.c')
            restore+=re.search(r'^#define SCHED16_MARK_OFFSET[^\n]*',schedule,re.M).group(0)+'\n'+function(schedule,'sched16_restore')+'\n'
        code=restore+config_helpers(user)+'\n'.join(function(user,n) for n in ('user_readconfig','user_readsavedinformation'))
        code+='\n'+sync_stub(user)+'void command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){ switch(ptr[4]) {'+case(12)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_config_strings.c').read_text().replace('/* TYPES */',types).replace('/* CRC */',function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')).replace('/* PRODUCTION */',code)
    elif kind=='sms':
        user=source('Tests/asm/reference/User/user_fun.c');strings=source('User/user_str_fun.c')
        code='\n'.join(function(strings,n) for n in ('skipchar','user_strcpy'))+'\n'
        code+='\n'.join(function(user,n) for n in ('user_last10digitsnumber','user_sendsms','user_comm_server','user_smsreceived','log_addSystemValue','user_comm_link','user_comm_balance','user_comm_low_signal'))
        links=re.search(r'enum \{ COMM_LINK_UP[^}]*\};',source('Modules/comm_server.h')).group(0)
        types=typedefs(source('structures.h'),'logSystem_e','logControl_e','codeType_e')+'\n'+links
        unit=(ROOT/'Tests/test_sms.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='uarttimer':
        uart=source('Modules/uart_stm32f10x.c')
        types=typedefs(uart,'uart_t')+'\n'+re.search(r'^#define UARTS[^\n]*',uart,re.M).group(0)+'\n'
        code='\n'.join(function(uart.replace('inline bool uart_addbyte','bool uart_addbyte'),n) for n in ('uart_addbyte','uart_getstate','uart_isReceiving','uart_timer1ms'))
        unit=(ROOT/'Tests/test_uart_timer.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='idtext':
        user=source('Tests/asm/reference/User/user_fun.c')
        code='\n'.join(function(user,n) for n in ('reader_id','WG_readedID','RF433_readedID','RFID_readedID'))
        types='#include <stdbool.h>\n#include <stdint.h>\n'+typedefs(source('structures.h'),'codeType_e','logControl_e')+'\n'
        unit=types+(ROOT/'Tests/test_id_text.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='usbresume':
        unit=(ROOT/'Tests/test_usb_resume.c').read_text().replace('/* PRODUCTION */',function(source('USB-FS-CDC_User/Src/hw_config.c'),'Leave_LowPowerMode'))
    elif kind=='usbdesc':
        unit=(ROOT/'Tests/test_usb_descriptor.c').read_text().replace('/* PRODUCTION */',function(source('USB-FS-CDC_User/Src/usb_prop.c'),'Virtual_Com_Port_GetStringDescriptor'))
    elif kind=='logupload':
        unit=(ROOT/'Tests/test_log_upload.c').read_text().replace('/* PRODUCTION */',function(source('Tests/asm/reference/User/user_fun.c'),'user_send_logRecords'))
    elif kind=='uploadretry':
        user=source('Tests/asm/reference/User/user_fun.c')
        code=function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')+'\n'
        if '/* >>> подтверждение выгрузки */' in user:
            code+=user[user.index('/* >>> подтверждение выгрузки */'):user.index('/* <<< подтверждение выгрузки */')]+'\n'
        else:
            code+='static uint16_t log_usbReadIndex,temp_usbReadIndex,log_unacked[2],temp_unacked[2]; static void upload_reset(uint8_t cmd){(void)cmd;} static uint16_t upload_index(uint8_t cmd,dataChanel_e ch){return cmd==10?(ch==CH_USB?log_usbReadIndex:device.readLogIDIndex):device.readTempIDIndex;} static void upload_restore(const device_s *v){(void)v;} static void log_setReadIndex(dataChanel_e ch,uint16_t n){if(ch==CH_USB)log_usbReadIndex=n;else device.readLogIDIndex=n;}\n'
        code+='\n'.join(function(user,n) for n in ['log_blank','log_valid','log_readRecords','user_send_anyData','user_send_logRecords','user_send_tempIDs','user_factoryReset'])
        unit=(ROOT/'Tests/test_upload_retry.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellLog_s','cellID_s','dataChanel_e')).replace('/* PRODUCTION */',code)
    elif kind=='idindex':
        code=source('Tests/asm/reference/User/idindex.c').replace('#include "idindex.h"',source('User/idindex.h'))
        unit=(ROOT/'Tests/test_idindex.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='outs':
        import re as _re
        outs=_re.sub(r'(?m)^#include.*$','',source('User/outs.c'))
        # DWT is outside the host model; keep the real relay implementation.
        unit=(ROOT/'Tests/test_outs.c').read_text(encoding='utf-8').replace('/* PRODUCTION */','void prof_relay_on(void) {}\n'+outs)
    elif kind=='crcff':
        crc=source('Tests/asm/reference/User/crc.c')
        user=source('Tests/asm/reference/User/user_fun.c')
        code=chr(10).join(function(crc,n) for n in ['crc16_calcInit','gf2_apply','crc16_ffInit'])
        code+=chr(10)+'static uint32_t id_used_bytes = 0xFFFFFFFFu;'+chr(10)+function(user,'user_calcCRC16InFlash')+chr(10)+function(user,'id_crc16')
        unit=(ROOT/'Tests/test_crc_ff.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='crcblocks':
        user=source('Tests/asm/reference/User/user_fun.c');crc=source('Tests/asm/reference/User/crc.c')
        code=chr(10).join(function(crc,n) for n in ['crc16_calcInit','gf2_apply','crc16_ffInit'])
        code+=chr(10)+'static uint32_t id_used_bytes = 0xFFFFFFFFu;'+chr(10)+function(user,'id_crc16')+chr(10)+function(user,'user_send_crcMemoryID')
        unit=(ROOT/'Tests/test_crc_blocks.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='flashcmds':
        user=source('Tests/asm/reference/User/user_fun.c')
        start=user.index('    case 103:')
        end=user.index('    case 104:',start) if '    case 104:' in user[start:] else user.index('    case 105:',start)
        code=function(user,'user_clearMemBlock')+'\nstatic void command(uint8_t *ptr,dataChanel_e cch,uint8_t uniqId){switch(ptr[4]){'+user[start:end]+'default:abort();}}'
        unit=(ROOT/'Tests/test_flash_commands.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellMem_s','memBlock_e','dataChanel_e','cmdCode_e')).replace('/* PRODUCTION */',code)
    elif kind=='w25':
        driver=source('Tests/asm/reference/User/flash_w25.c')
        driver=driver.replace('__no_operation();','') # hardware CS delay, like the existing host NOP stub
        result='bool' if 'bool w25_write_data' in driver else 'void'
        protos='static uint8_t w25_busy(void); static inline void w25_delay_ON(void); static inline void w25_delay_OFF(void); static void w25_WREN(void); static void w25_WRDI(void); static '+result+' w25_page_program(Int32U,pInt8U,Int16U);\n'
        code=protos+re.sub(r'^#include[^\n]*','',driver,flags=re.M)
        unit='#define HAVE_RESULT '+str(int(result=='bool'))+'\n'+(ROOT/'Tests/test_w25.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='sync':
        user=source('Tests/asm/reference/User/user_fun.c');case=cases(user)
        helper=function(user,'sync_cells_valid') if 'static bool sync_cells_valid(' in user else ''
        if '/* >>> Сборка таблиц страницами:' in user:helper+='\n'+user[user.index('/* >>> Сборка таблиц страницами:'):user.index('/* <<< Сборка таблиц страницами */')]
        crcsrc=source('Tests/asm/reference/User/crc.c')
        if 'static uint16_t id_crc16(' in user:helper=chr(10).join(function(crcsrc,n) for n in ['crc16_calcInit','gf2_apply','crc16_ffInit'])+chr(10)+'static uint32_t id_used_bytes = 0xFFFFFFFFu;'+chr(10)+function(user,'id_crc16')+chr(10)+helper
        if '/* >>> Праздники целиком' in user:helper+='\n'+user[user.index('/* >>> Праздники целиком'):user.index('/* <<< Праздники целиком */')]
        code=helper+'\n'+sync_end(user)+'\nvoid command(uint8_t *ptr, uint16_t len, dataChanel_e cch, uint8_t uniqId) { uint32_t tmp32; switch(ptr[4]) {'+''.join(case(n) for n in range(13,17))+'default: abort(); }}'
        types=typedefs(source('structures.h'),'cellMem_s','cellID_s','cellShedule_s','cellGroup_s','cellHoliday_s','cmdCode_e','shtamp_e','memBlock_e','dataChanel_e','logSystem_e')
        unit=(ROOT/'Tests/test_sync.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='idsync':
        user=source('Tests/asm/reference/User/user_fun.c');case=cases(user);crcsrc=source('Tests/asm/reference/User/crc.c')
        code=source('Tests/asm/reference/User/idindex.c').replace('#include "idindex.h"',source('User/idindex.h'))+chr(10)
        code+=chr(10).join(function(crcsrc,n) for n in ['crc16_calcInit','gf2_apply','crc16_ffInit'])+chr(10)
        code+='static uint32_t id_used_bytes = 0xFFFFFFFFu;'+chr(10)+function(user,'user_calcCRC16InFlash')+chr(10)+function(user,'id_crc16')+chr(10)+function(user,'sync_cells_valid')+chr(10)
        code+=user[user.index('/* >>> Сборка таблиц страницами:'):user.index('/* <<< Сборка таблиц страницами */')]+chr(10)
        code+=function(user,'user_findID')+chr(10)+function(user,'user_getNumbersCount')+chr(10)
        code+=sync_stub(user)+'void command(uint8_t *ptr, uint16_t len, dataChanel_e cch, uint8_t uniqId) { uint32_t tmp32; switch(ptr[4]) {'+case(13)+'default: abort(); }}'
        types=typedefs(source('structures.h'),'cellMem_s','cellID_s','cmdCode_e','shtamp_e','memBlock_e','dataChanel_e')
        unit=(ROOT/'Tests/test_id_sync.c').read_text(encoding='utf-8').replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='persistence':
        user=source('Tests/asm/reference/User/user_fun.c')
        code='\n'.join(function(user,n) for n in ['user_writeConfigAndDevice','user_updateID'])
        unit='#define HAVE_RESULT '+str(int('bool user_writeConfigAndDevice' in user))+'\n'+(ROOT/'Tests/test_persistence.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellID_s')).replace('/* PRODUCTION */',code)
    elif kind=='commands':
        user=source('Tests/asm/reference/User/user_fun.c');case=cases(user)
        code=function(source('Tests/asm/reference/User/crc.c'),'crc16_calc')+'\n'+config_helpers(user)+sync_stub(user)+'void command(uint8_t *ptr, uint16_t len, dataChanel_e cch, uint8_t uniqId) { switch(ptr[4]) {'+case(11)+case(12)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_commands.c').read_text().replace('/* CONFIG TYPES */',typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s')).replace('/* PRODUCTION */',code)
    elif kind=='wiegandirq':
        wg=source('User/wiegand.c')
        code='\n'.join(x for x in wg.splitlines() if re.match(r'^(?:volatile )?(?:uint32_t|uint8_t|bool) wg',x))+'\n'
        code+='\n'.join(function(wg,n) for n in ('wg_parity_valid','EXTI2_IRQHandler','EXTI3_IRQHandler','WG_process'))
        prof='#ifndef WG_PARITY\n#define WG_PARITY 1\n#endif\nuint32_t prof_card_mark;\n#define PROF_CARD_MARK() (prof_card_mark = PROF_NOW() | 1u)\n#define PROF_NOW() (0u)\n#define PROF_STEP(i, call) do { call; } while (0)\n'
        unit=(ROOT/'Tests/test_wiegand_irq.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',prof+code)
    elif kind=='timersirq':
        timers=source('Modules/timers.c').replace('inline void timers_handle','void timers_handle')
        code='\n'.join(x for x in timers.splitlines() if x.startswith(('unsigned int milliSeconds','unsigned int timer[','volatile unsigned int milliSeconds','volatile unsigned int timer[','#define timer_cnt')))+'\n'
        if 'static unsigned int timers_snapshot(' in timers:code+=function(timers,'timers_snapshot')+'\n'
        code+='\n'.join(function(timers,n) for n in ('timers_set','timers_get','timers_handle','timers_pause','timers_getMillis'))
        unit=(ROOT/'Tests/test_timers_irq.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='crc':
        user=source('Tests/asm/reference/User/user_fun.c')
        # Compile the actual CRC gate; the command switch is unreachable here.
        code=user[user.index('void user_parse_comand'):]
        code=(function(user,'command_args_valid')+'\n' if 'static bool command_args_valid(' in user else '')+code[:code.index('  /* ПИН (команда 102)')]+'  (void)tmp32;\n}\n'
        unit=(ROOT/'Tests/test_crc.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='recovery':
        code=source('Tests/asm/reference/Modules/usb_stm32f10x.c').replace('#include "usb_stm32f10x.h"','').replace('#pragma data_alignment=4','')
        if 'uint8_t usb_try_send(' in code:code=code.replace(function(code,'usb_try_send'),'')
        code=re.sub(r'__weak void usb_data_reading[^}]+}', '', code)
        code+=function(source('USB-FS-CDC_User/Src/usb_istr.c'),'USB_Istr')
        unit=(ROOT/'Tests/test_usb_recovery.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='rx':
        code=source('Tests/asm/reference/Modules/usb_stm32f10x.c').replace('#include "usb_stm32f10x.h"','').replace('#pragma data_alignment=4','')
        if 'uint8_t usb_try_send(' in code:code=code.replace(function(code,'usb_try_send'),'')
        code=re.sub(r'__weak void usb_data_reading[^}]+}', '', code)
        user=function(source('Tests/asm/reference/User/user_fun.c'),'usb_data_reading').replace('void usb_data_reading','void app_usb_data_reading')
        unit=(ROOT/'Tests/test_rx.c').read_text().replace('/* PRODUCTION */',user+'\n'+code)
    elif kind=='usbresetwait':
        pwr=source('USB-FS-CDC_User/Src/usb_pwr.c');code=''
        if 'uint8_t USB_WaitReset(' in pwr:code+=function(pwr,'USB_WaitReset')+'\n'
        code+=function(pwr,'Suspend')
        unit=(ROOT/'Tests/test_usb_reset_wait.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='usbsetupbounds':
        usb=source('STM32_USB-FS-Device_Driver/src/usb_core.c')
        block=usb[usb.index('      Related_Endpoint = (pInformation->USBwIndex0 & 0x0f);'):usb.index('\r\n  /*GET CONFIGURATION*/')]
        block=block[:block.rfind('    }')]
        code='\n'.join(function(usb,n) for n in ('Standard_GetStatus','Standard_ClearFeature','Standard_SetEndPointFeature'))
        code+='\nstatic uint8_t *(*audit_getstatus(void))(uint16_t){\nuint32_t Related_Endpoint,Reserved,Status;\nuint8_t *(*CopyRoutine)(uint16_t)=NULL;\nif(pInformation->USBwIndex1)return NULL;\n'+block+'\nreturn CopyRoutine;\n}'
        unit=(ROOT/'Tests/test_usb_setup_bounds.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='utcday':
        user=source('Tests/asm/reference/User/user_fun.c')
        clock=user[user.index('    device.day = user_getweakday'):user.index('    device.connected = comm_channel_ready();')]
        code='\n'.join(function(user,n) for n in ('user_getweakday','user_dayIsHoliday'))+'\nstatic void audit_clock(void){\n'+clock+'\n}'
        unit=(ROOT/'Tests/test_utc_day.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellHoliday_s')).replace('/* PRODUCTION */',code)
    elif kind=='idexpiry':
        user=source('Tests/asm/reference/User/user_fun.c')
        unit=(ROOT/'Tests/test_id_expiry.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellID_s')).replace('/* PRODUCTION */','\n'.join(function(user,n) for n in ('id_first_pass','id_expired','id_time_end')))
    elif kind=='tempidmeta':
        unit=(ROOT/'Tests/test_tempid_meta.c').read_text(encoding='utf-8').replace('/* TYPES */',typedefs(source('structures.h'),'cellID_s','codeType_e')).replace('/* PRODUCTION */',function(source('Tests/asm/reference/User/user_fun.c'),'user_findTempID')+'\n'+function(source('Tests/asm/reference/User/user_fun.c'),'user_addIdToTempBuffer'))
    elif kind=='commuart':
        header=source('Modules/comm_uart.h')
        code=source('Modules/comm_uart.c')
        code='\n'.join(x for x in code.splitlines() if not x.startswith('#include'))
        unit=source('Tests/test_comm_uart.c').replace('/* HEADER */',header).replace('/* PRODUCTION */',code)
        unit=unit.replace('/* TICK */',function(source('stm32_it.c'),'SysTick_Handler'))
        profile=sys.argv[3] if len(sys.argv)>3 else 'v5'
        defines={'v5':''}
        unit=defines[profile]+unit
    elif kind=='commat':
        from comm_at_host import unit as comm_at_unit
        unit=comm_at_unit(ROOT)
    elif kind=='commslm':
        from comm_slm_host import unit as comm_slm_unit
        unit=comm_slm_unit(ROOT)
    elif kind=='commserver':
        from comm_server_host import unit as server_unit
        unit=server_unit()
    elif kind=='tx':
        hw=source('USB-FS-CDC_User/Src/hw_config.c')
        if 'usb_tx_source' in hw:
            # Test the actual source owner and IRQ handlers from the linked ELF.
            subprocess.run([sys.executable,str(ROOT/'Tests/asm/test_usb_source.py'),
                            '--scenario',sys.argv[3] if len(sys.argv)>3 else 'idle'],check=True)
            sys.exit(0)
        header=source('USB-FS-CDC_User/Inc/hw_config.h')
        globals=hw[hw.index('uint8_t  USART_Rx_Buffer'):hw.index('static void IntToUnicode')]
        # The first definition is the application's; the #else branch is the bootloader's.
        code=source('User/usb_ring_size.h')+'\n'+[x for x in header.splitlines() if x.startswith('#define USART_RX_DATA_SIZE')][0]+'\n'
        if 'uint8_t USART_To_USB_Send_Data' in hw:code+='#define HOST_FIXED 1\n'
        code+=globals+'\n'
        for name in ['USB_Tx_Reset','usb_tx_space','usb_tx_enqueue','USART_To_USB_Send_Data','USB_Send_Debug','Handle_USBAsynchXfer']:
            if re.search(r'(?m)^(?:static )?(?:void|uint8_t|uint16_t|uint32_t)\s+'+name+r'\s*\(',hw):code+=function(hw,name)+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_endp.c'),'EP1_IN_Callback')+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_prop.c'),'Virtual_Com_Port_Reset')+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_istr.c'),'USB_Istr')+'\n'
        unit=source('Tests/test_tx.c').replace('/* PRODUCTION */',code)
    else:
        raise ValueError(kind)
    if 'idindex_' in unit and 'int8_t idindex_find(' not in unit:
        # Индекс ID проверяется своей группой idindex; здесь он «не готов» - старый путь перебором.
        stub='__attribute__((unused)) static uint32_t idindex_hash(uint16_t i,const uint8_t *id){(void)i;(void)id;return 0;} __attribute__((unused)) static void idindex_scanned(uint32_t s,uint16_t e){(void)s;(void)e;} __attribute__((unused)) static int8_t idindex_find(uint32_t b,const uint8_t *id,uint8_t *c){(void)b;(void)id;(void)c;return -1;} __attribute__((unused)) static bool idindex_put(uint16_t i,const uint8_t *id){(void)i;(void)id;return true;} __attribute__((unused)) static void idindex_commit(uint32_t d,uint16_t e){(void)d;(void)e;} __attribute__((unused)) static void idindex_poll(uint32_t b){(void)b;}'
        lines=unit.split(chr(10));last=max(i for i,l in enumerate(lines) if l.startswith('#include'))
        lines.insert(last+1,'#include <stdbool.h>'+chr(10)+'#include <stdint.h>'+chr(10)+stub);unit=chr(10).join(lines)
    if 'id_used_bytes' in unit and 'static uint32_t id_used_bytes' not in unit:
        pos=unit.index('/* PRODUCTION */') if '/* PRODUCTION */' in unit else None
        lines=unit.split(chr(10));last=max(i for i,l in enumerate(lines) if l.startswith('#include'))
        lines.insert(last+1,'static uint32_t id_used_bytes = 0xFFFFFFFFu;');unit=chr(10).join(lines)
    if 'mem_tables_ready()' in unit and 'bool mem_tables_ready(void)' not in unit:
        unit='#include <stdbool.h>\nstatic bool table_storage_ready=true;\nstatic bool mem_tables_ready(void) { return table_storage_ready; }\n'+unit
    if kind!='commserver':
        helpers=source('Tests/asm/reference/User/user_fun.c')
        helpers=helpers[helpers.index('/* Назначение кадра серверу'):helpers.index('void user_send_anyData(')]
        # Отправка серверу - только слот канала: помощники и модель слота нужны, если тест их зовёт.
        if not re.search(r'user_comm_(acquire|free)\(', unit): helpers=''
        if re.search(r'comm_tx_(acquire|commit|release)\(', unit+helpers) and 'bool comm_tx_commit(' not in unit:
            proto='#include <stdint.h>\n#include <stdbool.h>\n#include <stdlib.h>\nstatic uint8_t *comm_tx_acquire(uint16_t *capacity);\nstatic bool comm_tx_commit(uint16_t count);\nstatic void comm_tx_release(void);\n'
            if 'static int8_t connection_send(' in unit:
                slot='#define COMM_SLOT_FAILURE '+('fail_alloc' if kind=='uploadretry' else '0')+'\n'+source('Tests/comm_upload_slot.c')+'\n'
                pos=unit.index('\n',unit.index('static int8_t connection_send('))+1
                unit=unit[:pos]+slot+unit[pos:]
            else:
                unit+='\nstatic uint8_t sink_slot[1040];\nstatic uint8_t *comm_tx_acquire(uint16_t *capacity){*capacity=sizeof(sink_slot);return sink_slot;}\nstatic bool comm_tx_commit(uint16_t count){return count<=sizeof(sink_slot);}\nstatic void comm_tx_release(void){}\n'
            unit=proto+helpers+unit
        else:
            unit=helpers+unit
    if 'Cooperative config publication' in unit:
        unit='#include <stdint.h>\n#include <stdbool.h>\nuint8_t mem_config_result(void);\nbool w25_async_active(void){return false;}\nbool w25_async_start(uint32_t a,const uint8_t*p,uint16_t n,uint32_t now){(void)a;(void)p;(void)n;(void)now;return false;}\nuint8_t w25_async_poll(uint32_t now){(void)now;return 3;}\n'+unit
    if kind == 'w25':
        unit='#include <stdint.h>\n#include <stdbool.h>\nbool w25_read_data(uint32_t,uint8_t*,uint16_t);\n'+unit
    # Existing isolated tests cover synchronous startup/compatibility paths.
    # Runtime continuations have their own real-code harness (test_store_callers.py).
    if ('store.' in unit or 'store_runtime' in unit or 'store_restore(' in unit or 'store_command_start(' in unit) and 'static uint8_t store_runtime' not in unit and 'unsigned job,channel,quiet,seen' not in unit:
        unit='#include <stdint.h>\n#include <stdbool.h>\nstatic uint8_t store_runtime __attribute__((unused)) = 0;\nstatic uint8_t store_save_requested __attribute__((unused)), store_reset_requested __attribute__((unused));\nstatic struct {uint8_t job,clear,channel,uniq,cmd,quiet,seen;} store __attribute__((unused));\n#define store_ack(a,b) false\n#define store_finish(a) false\n#define store_restore(a) ((void)0)\n#define store_upload_request(a,b,c) true\n#define store_command_start(a,b,c) user_send_answer_comand(a,CMD_SUCCESS,b,c)\n'+unit
    # Legacy queue tests remain synchronous; the real runtime writer is tested separately.
    if kind == 'log':
        unit=re.sub(r'/\* >>> Cooperative firmware block writer \*/.*?/\* <<< Cooperative firmware block writer \*/', '', unit, flags=re.S)
        unit=re.sub(r'/\* >>> Cooperative event writer \*/.*?/\* <<< Cooperative event writer \*/', '', unit, flags=re.S)
    if 'log_async_active(' in unit and 'bool log_async_active(void) {' not in unit:
        unit='#define log_async_active() false\n'+unit
    if 'fw_async_active(' in unit and 'bool fw_async_active(void) {' not in unit:
        unit='#define fw_async_active() false\n'+unit
    for helper,args,value in [('photo_storage_busy','','false'),('photo_storage_cancel','','((void)0)'),('table_runtime_active','','false'),('table_runtime_quiet','ch','((void)(ch))'),('table_runtime_cancel','','((void)0)'),('fw_async_begin','a,b,c','false'),('fw_async_reply','id','((void)(id))'),('fw_async_quiet','ch','((void)(ch))'),('fw_async_cancel','','((void)0)')]:
        if helper+'(' in unit and 'static void '+helper+'(' not in unit and 'static bool '+helper+'(' not in unit:
            unit='#define '+helper+'('+args+') '+value+'\n'+unit
    if 'server_rx_verified(' in unit and 'bool server_rx_verified(' not in unit:
        unit='#define server_rx_verified(p) 0\n'+unit
    if 'store_command_allowed(' in unit and 'bool store_command_allowed(' not in unit:
        unit='#define store_command_allowed(cmd) 1\n'+unit
    if 'user_storage_flash_ready()' in unit and 'bool user_storage_flash_ready(void)' not in unit:
        unit='#define user_storage_flash_ready() 1\n'+unit
    if 'USB_Tx_CommandReady()' in unit:
        helper=function(source('USB-FS-CDC_User/Src/hw_config.c'),'USB_Tx_CommandReady')
        helper=helper.replace('  extern bool user_storage_command_ready(void);','')
        at=unit.index('int main(')
        unit=unit[:at]+helper+'\n'+unit[at:]
        unit='#define USB_Tx_SourceBusy() 0\n'+unit
    if 'user_storage_command_ready()' in unit and 'bool user_storage_command_ready(void)' not in unit:
        unit='#define user_storage_command_ready() 1\n'+unit
    if kind in ('cfgstrings','commands'):
        unit=unit.replace('void command(', 'void command_step(')
        helper = '\nvoid command(uint8_t*,uint16_t,dataChanel_e,uint8_t);\nstatic config_s staged_config; static bool staged_ok;\nstatic bool store_config_start(void){staged_config=config; staged_ok=user_writeConfigAndDevice(); return staged_ok;}\nstatic const uint8_t *mem_config_snapshot(void){return (const uint8_t*)&staged_config;}\n'
        at=unit.index('void command_step(')
        unit=unit[:at]+helper+unit[at:]
        at=unit.index('int main(')
        unit=unit[:at]+function(source('Tests/asm/reference/User/user_fun.c'),'store_config_complete')+'\nvoid command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){staged_ok=false;command_step(ptr,len,cch,uniqId);if(staged_ok){store_config_complete();user_send_timeshtamp(SHTAMP_CONFIG,config.datetime,user_calcCRC16InFlash(mem[MB_CONFIG].start,mem[MB_CONFIG].size),cch,uniqId);}}\n'+unit[at:]
    if kind == 'rs485cfg':
        helper='static config_s staged_config; static bool staged_ok;\nstatic bool store_config_start(void){staged_config=config; staged_ok=user_writeConfigAndDevice(); return staged_ok;}\nstatic const uint8_t *mem_config_snapshot(void){return (const uint8_t*)&staged_config;}\n'+function(source('Tests/asm/reference/User/user_fun.c'),'store_config_complete')+'\n'
        unit=unit.replace('static void receive_config(',helper+'static void receive_config_step(')
        unit=unit.replace('static void test_rs485cfg(void)', 'static void receive_config(dataChanel_e cch,uint8_t code){staged_ok=false;receive_config_step(cch,code);if(staged_ok){store_config_complete();user_send_timeshtamp(SHTAMP_CONFIG,config.datetime,user_calcCRC16InFlash(mem[MB_CONFIG].start,mem[MB_CONFIG].size),cch,7);}}\nstatic void test_rs485cfg(void)')
    if 'fast_' in unit:
        (d/'fast_kernels.h').write_text(source('User/fast_kernels.h'), encoding='ascii')
        unit='#include "fast_kernels.h"\n'+unit.replace('#include "fast_kernels.h"','')
    unit=unit.replace('#include \"domovoi_diag.h\"','').replace('#include \"../domovoi_diag.h\"','')
    (d/'test.c').write_text(host_adapters(ROOT)+unit.replace('\r\n','\n'))
    flags=['-DWG_PARITY=0'] if kind=='wiegandirq' and sys.argv[3:]==['0'] else []
    subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',*flags,str(d/'test.c'),'-o',str(d/'test.exe')],check=True)
    result=subprocess.run([str(d/'test.exe')]+sys.argv[3:],timeout=3 if kind=='tx' else 300)
    sys.exit(result.returncode)
