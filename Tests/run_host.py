# Доработка: Кряжев А., 2026
import os, pathlib, subprocess, sys, tempfile, re
# DOMOVOI_ROOT runs the tests against a copy, e.g. with a deliberate fault.
ROOT=pathlib.Path(os.environ.get('DOMOVOI_ROOT') or pathlib.Path(__file__).resolve().parents[1])
ZIG=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'
ref=sys.argv[1] if len(sys.argv)>1 and sys.argv[1]!='work' else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return (subprocess.check_output(['git','show',ref+':'+name],cwd=ROOT) if ref else (ROOT/name).read_bytes()).decode('cp1251')
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:const char \*|char\s*\*\s*|uint8_t\s*\*\s*|(?:void|bool|uint8_t|uint16_t|uint32_t|unsigned short|Int8U|Int16U|cmdCode_e|logControl_e)\s+)'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    return text[m.start():i]
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
def config_helpers(user):
    return function(user,'config_terminate_strings')+'\n' if 'static void config_terminate_strings(' in user else ''

with tempfile.TemporaryDirectory(prefix='domovoi-host-') as d:
    d=pathlib.Path(d)
    if kind=='storage':
        # Check physical erase-sector footprints, not just logical byte ranges.
        mapping=source('User/user_fun.c')
        ranges=[]
        for name,start,size in re.findall(r'mem\[(MB_\w+)\]\.start = (\d+);\s*mem\[\1\]\.size = (\d+);',mapping):
            start,size=int(start),int(size)
            if size:ranges.append((name,start&~4095,(start+size+4095)&~4095))
        for lo,hi in [(0x1f5000,0x1f7000),(0x1f7000,0x1f9000)]:
            assert lo>=0x41000 and hi<=0x200000
            for name,a,b in ranges:assert hi<=a or lo>=b,(name,hex(a),hex(b))
        print('Flash map: both slots outside all rounded mem[] ranges and 0x00000..0x40FFF',flush=True)
        code=source('User/memory.c').replace('#include "memory.h"','')
        header=source('User/memory.h')
        defines='\n'.join(x for x in header.splitlines() if x.startswith('#define MEM'))
        test=(ROOT/'Tests/test_storage.c').read_text()
        legacy=subprocess.check_output(['git','show','203c55b:User/user_fun.c'],cwd=ROOT).decode('cp1251')
        legacy_code='\n'.join(function(legacy,n) for n in ['user_writeConfigAndDevice','user_readconfig'])
        for n in ['user_writeConfigAndDevice','user_readconfig','user_setdefault','user_saveindex','mem_read','mem_write','mem_erase_sector']:
            legacy_code=re.sub(r'\b'+n+r'\b','legacy_'+n,legacy_code)
        reset='#define TEST_STORAGE_BOOT() (cfg_legacy_checked = 0)\n' if 'cfg_legacy_checked' in code else '#define TEST_STORAGE_BOOT() ((void)0)\n'
        unit=test.replace('/* PRODUCTION */',defines+'\n'+code+'\n'+reset+function(source('User/user_fun.c'),'user_writeConfigAndDevice')+'\n'+legacy_code)
    elif kind in ('photo','rs485cfg'):
        user=source('User/user_fun.c'); case=cases(user)
        assert 'len == 4 ? photo_start(ptr[5]) : CMD_PARAMERROR' in case(7)
        assert 'if (len == 3 || len == 9)' in case(21)
        assert 'photo_request(cch, uniqId, NULL, 0)' in function(user,'user_send_Photos')
        assert 'photo_poll();' in function(user,'port_check_data')
        assert 'photo_connection_lost();' in function(user,'user_disconnected')
        assert '(cmd == 7 || cmd == 21) ? 5 : 255' in function(user,'user_send_answer_comand')
        code=source('User/photo.c').replace('#include "photo.h"','')
        code += '\n'+function(user,'user_send_Photos')+'\nstatic void command(uint8_t cmd,uint16_t len,uint8_t address,uint8_t uniqId){ uint8_t data[11]={0}; uint8_t *ptr=data; dataChanel_e cch=CH_SERVER; ptr[5]=address; switch(cmd){'+case(7)+case(21)+'default: break;}}'
        unit=source('Tests/test_photo.c').replace('/* PRODUCTION */',function(user,'user_getPhotoCount')+'\n'+code)
        if kind=='rs485cfg':
            types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s')
            unit=re.sub(r'typedef struct \{ uint8_t rs485Mode, reserv\[4\]; periphery_s rs485Periphery\[8\]; \} config_s;',types,unit)
            unit=unit.replace('uint8_t readyToUpdate, readyPhoto;', 'uint8_t readyToUpdate, readyPhoto, defaultConfig;').replace('uint32_t datetime, newAddressPhoto;', 'uint32_t datetime, datetimeConfig, newAddressPhoto;')
            extra=config_helpers(user)+source('Tests/test_rs485cfg.c').replace('/* COMMAND12 */',case(12))
            unit=unit.replace('int main(int argc,char **argv) {',extra+'\nint main(int argc,char **argv) {\n test_rs485cfg();')
    elif kind=='fwstart':
        user=source('User/user_fun.c')
        case=cases(user)
        # Call sites: commands 19 and 20 go through the tested functions, command 4 carries their code.
        c19=case(19);c20=case(20)
        assert '(len - 3) == 16' in c19 and 'user_send_answer_comand(ptr[4], fw_start(&ptr[5], cch), cch, uniqId);' in c19,'command 19'
        assert 'fw_take_block(&ptr[5], (uint16_t)(len - 3), cch);' in c20 and 'FW_BLOCK_SKIP' not in user and 'user_send_statusUpdateFW(ptr[4], code, cch, uniqId);' in c20,'command 20'
        # Command 2: a download is cancelled before the reset, a rollback or install is left alone.
        c2=case(2)
        assert c2.index('if (device.rebootToUpdate){')<c2.index('fw_abort("CANCEL");')<c2.index('user_savereset();'),'command 2 during an update'
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
        unit=(ROOT/'Tests/test_fw_start.c').read_text(encoding='utf-8').replace('/* TYPES */',types).replace('/* PRODUCTION */',defines+helpers).replace('/* AUDIT LOOP */',audit)
    elif kind=='log':
        user=source('User/user_fun.c');st=source('structures.h')
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
        assert 'id_watch_end(id_time_end(old));' in case(13) and 'user_checkIdTimeEnd();' in user and 'id_endScan = true;' in function(user,'log_setTime'),'use time end checked'
        assert 'if (keep){ memcpy(old->reserve, first, 4); old->reserve[6] = done; }' in case(13) and 'memcmp(old->ID, &ptr[(i*sizeof(cellID_s)) + 12], 10) == 0' in case(13),'use time kept on sync'
        assert 'log_addControlEvent(server_out_event, id, cnt);' in function(user,'user_activateOut'),'server event from its source'
        # Journal sheet 131: after every pass of an ID with limited passes; 129: photo number when the photo is stored.
        assert 'if (cnt != 255) log_addControlEvent(LOG_CON_PASSCOUNT, id, cnt);' in function(user,'user_activateOut'),'pass counter event'
        assert 'log_addPhotoEvent(photo->stamp);' in source('User/photo.c'),'photo event'
        # Journal upload: the empty answer is 3 bytes (0 left, 0 in packet), sent before the erase; used sectors only.
        up=function(user,'user_send_logRecords')
        assert up.count('user_send_anyData(10, empty, sizeof(empty), cch, uniqId);')==2 and 'uint8_t empty[3] = {0, 0, 0};' in up and 'tmp16' not in up,'empty journal answer'
        assert up.index('user_send_anyData(10, empty')<up.index('log_clearLogBuffer();'),'answer before the erase'
        clr=function(user,'log_clearLogBuffer')
        assert 'endAdr = device.newAddressLog;' in clr and 'mem[MB_LOG].start & ~(uint32_t)4095' in clr,'used sectors only'
        pend=user[user.index('/* >>> отложенные события журнала'):user.index('/* <<< отложенные события журнала */')]
        pend+=user[user.index('/* >>> номер записи журнала'):user.index('/* <<< номер записи журнала */')]
        # USB reads the journal from its own position and never erases it; only the server upload erases.
        assert 'uint16_t readIdx = (cch == CH_USB) ? log_usbReadIndex : device.readLogIDIndex;' in up and up.count('device.readLogIDIndex')==1,'USB journal position'
        assert 'if (cch == CH_USB) log_setReadIndex(cch, 0);' in up and 'else log_clearLogBuffer();' in up and 'log_usbReadIndex = 0;' in clr,'USB does not erase'
        # USB commands other than ident, status, PIN and USB diagnostics need the PIN.
        pc=function(user,'user_parse_comand')
        assert pc.index('CMD_CRCERROR')<pc.index('if (cch == CH_USB && !device.userAutorized && !usb_cmd_without_pin(ptr[4])){')<pc.index('switch (ptr[4])'),'PIN gate'
        assert 'return cmd == 100 || cmd == 101 || cmd == 102 || cmd == USB_DIAG_COMMAND;' in user and 'device.userAutorized = 0; // кабель вынут' in user,'PIN scope'
        # Command 8: a requested output that did not switch answers 2, not success.
        assert 'res8 = CMD_RUNERROR;' in c8 and 'user_send_answer_comand(ptr[4], res8, cch, uniqId);' in c8 and 'CMD_SUCCESS, cch' not in c8,'command 8 answer'
        code=pend+'\n'.join(function(user,n) for n in ['log_addSystemEvent','log_addSystemEventID','log_addControlEvent','log_addPhotoEvent','log_addUpdateEvent'])
        unit=(ROOT/'Tests/test_log.c').read_text().replace('/* TYPES */',typedefs(st,'logSystem_e','logControl_e','logUpdate_e','cellLog_s')).replace('/* PRODUCTION */',code)
    elif kind=='access':
        user=source('User/user_fun.c');st=source('structures.h')
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
        assert c103.index('user_clearMemBlock(MB_ID);')<c103.index('device.numbersCount = user_getNumbersCount();'),'count after clearing'
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
        assert 'if (device.moduleType != gsm_getModuleType()){' in function(user,'usergsm_event'),'module type on change'
        types=typedefs(st,'logControl_e','codeType_e','releMode_s','cellShedule_s','cellID_s')
        # Passes per day: the counter restores on the first pass of a new day; a record from the server is today's.
        assert 'user_reloadPassCounter' not in user and 'id_set_pass_day(old, id_today());' in cases(user)(13),'passes per day'
        code=re.search(r'(?m)^static uint16_t id_cells_end;',user).group(0)+'\n'
        code+=user[user.index('/* >>> время действия ID'):user.index('/* <<< проходы в сутки */')]
        code+='\n'.join(function(user,n) for n in ['shedule_time_ok','user_findID','user_getNumbersCount','user_checkIdTimeEnd','user_activateOut'])
        unit=(ROOT/'Tests/test_access.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='rules':
        user=source('User/user_fun.c')
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
        gate=sms[sms.index('if (fw_update_quiet()'):sms.index('if (strcmp(v, "add") == 0){')]
        assert all('strcmp(v, "%s") == 0'%c in gate for c in ('add','reset','setout','default')) and '"imei"' not in gate and '"status"' not in gate,'SMS control commands refused in update mode'
        call=user[user.index('bool user_incomingcall (char* num, uint8_t typ)'):user.index('void user_comanderror')]
        assert 'fw_update_quiet' not in call,'calls keep working in update mode'
        # Update events of the journal sheet (system 1-5 struck out): 132 start, 133 failure with its
        # reason code, 134 time limit, 135 just before the install restart, 136 trial confirmed, 137 rollback.
        rest=user
        for n in ('ENDIDUPDATE','ENDCONFUPDATE','ENDGROUPUPDATE','ENDSHEDULEUPDATE','ENDHOLLIDAYUPDATE'): rest=rest.replace('LOG_SYS_'+n,'')
        assert 'UPDATE)' not in rest,'no system update events'
        assert user.count('fw_log(LOG_FW_START, NULL);')==1 and 'fw_log(LOG_FW_INSTALL, NULL);\r\n        user_savereset();' in loop,'events 132 and 135'
        assert 'app_trial_confirm();\r\n          if (!app_trial_active()) fw_log(LOG_FW_DONE, NULL);' in case(1),'event 136 on confirmation'
        assert 'fw_log(strcmp(reason, "TRIAL FAIL") == 0 ? LOG_FW_ROLLBACK : LOG_FW_FAIL, reason);' in case(1),'bootloader result: 137 after a failed trial'
        assert '0x48570000+DOMOVOI_APP_HW' in source('CMSIS/STM32F10x/Source/startup_stm32f10x_md.s'),'board marker in the vector table'
        assert source('DomovoiNext.ewp').count('<state>DOMOVOI_APP_HW=139</state>')==4 and 'DOMOVOI_APP_HW' not in source('Bootloader/DomovoiBoot.ewp'),'board marker only in the application build'
        # Release_IPv3 - old MODEM board (IP_v3, HW 139): PA0 stays an input; Release_140 - transition from loader 140
        assert source('DomovoiNext.ewp').count('<state>BOARD_IP_V3</state>')==2,'IP_v3 builds are marked for C'
        assert '#ifdef BOARD_IP_V3' in source('board.h') and '#ifdef BOARD_IP_V3' in source('board.c'),'IP_v3 keeps PA0 (RS232 CTS) an input'
        unit=(ROOT/'Tests/test_update_rules.c').read_text().replace('/* PRODUCTION */',defines+rules+function(user,'user_send_statusPack'))
    elif kind=='serverrx':
        user=source('User/user_fun.c')
        for name in ['user_connected','user_disconnected']:
            assert 'server_rx_reset();' in function(user,name),name
        code=user[user.index('#define SERVER_RX_CAPACITY'):user.index('// принято СМС')]
        unit=(ROOT/'Tests/test_server_rx.c').read_text(encoding='utf-8').replace('/* PRODUCTION */',code)
    elif kind=='fwblockbounds':
        user=source('User/user_fun.c');case=cases(user)
        defines='\n'.join(x for x in user.splitlines() if x.startswith('#define FW_'))+'\n#define FWBLOCK_OFFSET 0x200u\n'
        code=function(user,'fw_take_block')+'\nvoid command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){ switch(ptr[4]) {'+case(20)+'default: abort(); }}'
        types=typedefs(source('structures.h'),'dataChanel_e','logUpdate_e','bootFWparam_s')
        unit=(ROOT/'Tests/test_fw_block_bounds.c').read_text().replace('/* TYPES */',types).replace('/* DEFINES */',defines).replace('/* PRODUCTION */',code)
    elif kind=='cmdlengths':
        user=source('User/user_fun.c');parse=function(user,'user_parse_comand')
        types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s','status_s','triggers_s','device_s','dataChanel_e','cmdCode_e','logSystem_e','logUpdate_e')
        helper=function(user,'command_args_valid')+'\n' if 'static bool command_args_valid(' in user else ''
        labels=list(re.finditer(r'^    (?:case [^:]+:|default:)',parse,re.M))
        wanted=(1,2,3,6,18,23,102,105,108); selected=[]
        for i,m in enumerate(labels[:-1]):
            if m[0].strip().split()[1].rstrip(':').isdigit() and int(m[0].strip().split()[1].rstrip(':')) in wanted:
                selected.append(parse[m.start():labels[i+1].start()])
        assert len(selected)==len(wanted)
        defines='\n'.join(x for x in user.splitlines() if x.startswith(('#define REG_UNKNOWN_IMEI','#define REG_NOT_BOUND')))
        code=defines+'\n'+helper+function(user,'usb_cmd_without_pin')+'\n'+parse[:parse.index('  switch (ptr[4])')]+'  switch(ptr[4]){'+''.join(selected)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_command_lengths.c').read_text().replace('/* TYPES */',types).replace('/* CRC */',function(source('User/crc.c'),'crc16_calc')).replace('/* PRODUCTION */',code)
    elif kind=='cfgstrings':
        user=source('User/user_fun.c');case=cases(user)
        types=typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s','status_s','triggers_s','device_s','memBlock_e','dataChanel_e','cmdCode_e','shtamp_e')
        code=config_helpers(user)+'\n'.join(function(user,n) for n in ('user_readconfig','user_readsavedinformation'))
        code+='\nvoid command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){ switch(ptr[4]) {'+case(12)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_config_strings.c').read_text().replace('/* TYPES */',types).replace('/* CRC */',function(source('User/crc.c'),'crc16_calc')).replace('/* PRODUCTION */',code)
    elif kind=='sms':
        user=source('User/user_fun.c');strings=source('User/user_str_fun.c')
        code='\n'.join(function(strings,n) for n in ('skipchar','user_strcpy'))+'\n'
        code+='\n'.join(function(user,n) for n in ('user_last10digitsnumber','user_sendsms','user_smsreceived'))
        types=typedefs(source('structures.h'),'logSystem_e','logControl_e','codeType_e')
        unit=(ROOT/'Tests/test_sms.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='uarttimer':
        uart=source('Modules/uart_stm32f10x.c')
        types=typedefs(uart,'uart_t')+'\n'+re.search(r'^#define UARTS[^\n]*',uart,re.M).group(0)+'\n'
        code='\n'.join(function(uart.replace('inline bool uart_addbyte','bool uart_addbyte'),n) for n in ('uart_addbyte','uart_getstate','uart_isReceiving','uart_timer1ms'))
        unit=(ROOT/'Tests/test_uart_timer.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='idtext':
        user=source('User/user_fun.c')
        code=typedefs(source('structures.h'),'codeType_e')+'\n'+'\n'.join(function(user,n) for n in ('WG_readedID','RF433_readedID','RFID_readedID'))
        unit=(ROOT/'Tests/test_id_text.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='usbresume':
        unit=(ROOT/'Tests/test_usb_resume.c').read_text().replace('/* PRODUCTION */',function(source('USB-FS-CDC_User/Src/hw_config.c'),'Leave_LowPowerMode'))
    elif kind=='usbdesc':
        unit=(ROOT/'Tests/test_usb_descriptor.c').read_text().replace('/* PRODUCTION */',function(source('USB-FS-CDC_User/Src/usb_prop.c'),'Virtual_Com_Port_GetStringDescriptor'))
    elif kind=='logupload':
        unit=(ROOT/'Tests/test_log_upload.c').read_text().replace('/* PRODUCTION */',function(source('User/user_fun.c'),'user_send_logRecords'))
    elif kind=='crcblocks':
        unit=(ROOT/'Tests/test_crc_blocks.c').read_text().replace('/* PRODUCTION */',function(source('User/user_fun.c'),'user_send_crcMemoryID'))
    elif kind=='sync':
        user=source('User/user_fun.c');case=cases(user)
        helper=function(user,'sync_cells_valid') if 'static bool sync_cells_valid(' in user else ''
        code=helper+'\nvoid command(uint8_t *ptr, uint16_t len, dataChanel_e cch, uint8_t uniqId) { uint32_t tmp32; switch(ptr[4]) {'+''.join(case(n) for n in range(13,17))+'default: abort(); }}'
        types=typedefs(source('structures.h'),'cellMem_s','cellID_s','cellShedule_s','cellGroup_s','cellHoliday_s','cmdCode_e','shtamp_e','memBlock_e','dataChanel_e')
        unit=(ROOT/'Tests/test_sync.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='commands':
        user=source('User/user_fun.c');case=cases(user)
        code=function(source('User/crc.c'),'crc16_calc')+'\n'+config_helpers(user)+'void command(uint8_t *ptr, uint16_t len, dataChanel_e cch, uint8_t uniqId) { switch(ptr[4]) {'+case(11)+case(12)+'default: abort(); }}'
        unit=(ROOT/'Tests/test_commands.c').read_text().replace('/* CONFIG TYPES */',typedefs(source('structures.h'),'releMode_s','rs485Periphery_s','config_s')).replace('/* PRODUCTION */',code)
    elif kind=='uartresponse':
        uart=source('Modules/uart_stm32f10x.c')
        types=re.search(r'typedef struct\{.*?\}uart_t;',uart,re.S).group(0)+'\n#define UARTS 3\n'
        code=''
        if 'static bool uart_matchresponse(' in uart:code+=function(uart,'uart_matchresponse')+'\n'
        code+='\n'.join(function(uart,n) for n in ('uart_clear','uart_waitresp','uart_sendStrAndWaitResp'))
        unit=(ROOT/'Tests/test_uart_response.c').read_text(encoding='utf-8').replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='gsmparser':
        gsm=source('Modules/gsm.c');strings=source('User/user_str_fun.c')
        types=typedefs(source('Modules/gsm.h'),'gsmresp_e','moduleType_e','pwrOp_e','GsmStr')
        code='\n'.join(function(strings,n) for n in ('chkdigitchar','chkdigitstr','skipchar','user_strcpy'))+'\n'
        if 'static bool gsm_datalength(' in gsm:code+=function(gsm,'gsm_datalength')+'\n'
        if 'static bool gsm_regstate(' in gsm:code+=function(gsm,'gsm_regstate')+'\n'
        if '/* >>> Поток RX:' in gsm:
            code+='#define TEST_STREAM 1\nstatic void gsm_rxreset(void);\nstatic void gsm_rxfailed(void);\nstatic bool gsm_rxpending(void);\nvoid gsm_read(char*,Int16U);\n'
            code+=gsm[gsm.index('/* >>> URC'):gsm.index('/* <<< URC')]+'\n'
            code+=function(gsm,'gsm_readtext')+'\n'+gsm[gsm.index('/* >>> Поток RX:'):gsm.index('/* <<< Поток RX */')]
        else:code+=function(gsm,'gsm_read')
        unit=(ROOT/'Tests/test_gsm_parser.c').read_text(encoding='utf-8').replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
        if '/* >>> Поток RX:' in gsm:unit='#define TEST_STREAM 1\n'+unit
    elif kind=='gsmreg':
        gsm=source('Modules/gsm.c')
        code='\n'.join(x for x in gsm.splitlines() if x.startswith('#define GSM_REG_'))+'\n'
        code+=function(gsm,'gsm_getregstate')
        unit=(ROOT/'Tests/test_gsm_reg.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='gsmurc':
        gsm=source('Modules/gsm.c');uart=source('Modules/uart_stm32f10x.c')
        # The answer copy is bounded: a URC carrying server data is far longer than gsm_strbuf.
        assert 'strcpy(rec,' not in uart and 'memcpy(rec, &uart[num].rxbuffer[0], len);' in uart,'bounded answer copy'
        assert 'uart_sendStrAndWaitResp(GSM_PORT, a, b, sizeof(gsm_strbuf), d, c)' in gsm,'answer buffer size'
        # Wiring: the stash is taken before the command clears the port and parsed by the main loop.
        run=function(gsm,'gsm_runcmd')
        assert run.index('gsm_stashurc();')<run.index('gsm_sendStrAndWaitResp'),'stash before the command'
        assert 'gsm_takeurc();' in function(source('User/user_fun.c'),'gsm_check_data'),'main loop parses the stash'
        code=gsm[gsm.index('/* >>> URC'):gsm.index('/* <<< URC')]+'\n'+run
        if 'gsm_rxfailed();' in code:code='#include <stdbool.h>\nstatic void gsm_rxfailed(void) {}\nstatic bool gsm_rxpending(void) {return false;}\n'+code
        unit=(ROOT/'Tests/test_gsm_urc.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='crc':
        user=source('User/user_fun.c')
        # Compile the actual CRC gate; the command switch is unreachable here.
        code=user[user.index('void user_parse_comand'):]
        code=(function(user,'command_args_valid')+'\n' if 'static bool command_args_valid(' in user else '')+code[:code.index('  /* ПИН (команда 102)')]+'  (void)tmp32;\n}\n'
        unit=(ROOT/'Tests/test_crc.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='recovery':
        code=source('Modules/usb_stm32f10x.c').replace('#include "usb_stm32f10x.h"','').replace('#pragma data_alignment=4','')
        code=re.sub(r'__weak void usb_data_reading[^}]+}', '', code)
        code+=function(source('USB-FS-CDC_User/Src/usb_istr.c'),'USB_Istr')
        unit=(ROOT/'Tests/test_usb_recovery.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='rx':
        code=source('Modules/usb_stm32f10x.c').replace('#include "usb_stm32f10x.h"','').replace('#pragma data_alignment=4','')
        code=re.sub(r'__weak void usb_data_reading[^}]+}', '', code)
        user=function(source('User/user_fun.c'),'usb_data_reading').replace('void usb_data_reading','void app_usb_data_reading')
        unit=(ROOT/'Tests/test_rx.c').read_text().replace('/* PRODUCTION */',user+'\n'+code)
    elif kind=='tx':
        hw=source('USB-FS-CDC_User/Src/hw_config.c')
        header=source('USB-FS-CDC_User/Inc/hw_config.h')
        globals=hw[hw.index('uint8_t  USART_Rx_Buffer'):hw.index('static void IntToUnicode')]
        # The first definition is the application's; the #else branch is the bootloader's.
        code=[x for x in header.splitlines() if x.startswith('#define USART_RX_DATA_SIZE')][0]+'\n'
        if 'uint8_t USART_To_USB_Send_Data' in hw:code+='#define HOST_FIXED 1\n'
        code+=globals+'\n'
        for name in ['USB_Tx_Reset','usb_tx_space','usb_tx_enqueue','USART_To_USB_Send_Data','USB_Send_Debug','Handle_USBAsynchXfer']:
            if re.search(r'(?m)^(?:static )?(?:void|uint8_t|uint16_t|uint32_t)\s+'+name+r'\s*\(',hw):code+=function(hw,name)+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_endp.c'),'EP1_IN_Callback')+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_prop.c'),'Virtual_Com_Port_Reset')+'\n'
        code+=function(source('USB-FS-CDC_User/Src/usb_istr.c'),'USB_Istr')+'\n'
        unit=(ROOT/'Tests/test_tx.c').read_text().replace('/* PRODUCTION */',code)
    else:
        raise ValueError(kind)
    (d/'test.c').write_text(unit.replace('\r\n','\n'))
    subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(d/'test.c'),'-o',str(d/'test.exe')],check=True)
    result=subprocess.run([str(d/'test.exe')]+sys.argv[3:],timeout=3 if kind=='tx' else 300)
    sys.exit(result.returncode)
