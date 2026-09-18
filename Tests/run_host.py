import os, pathlib, subprocess, sys, tempfile, re
# DOMOVOI_ROOT runs the tests against a copy, e.g. with a deliberate fault.
ROOT=pathlib.Path(os.environ.get('DOMOVOI_ROOT') or pathlib.Path(__file__).resolve().parents[1])
ZIG=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'
ref=sys.argv[1] if len(sys.argv)>1 and sys.argv[1]!='work' else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return (subprocess.check_output(['git','show',ref+':'+name],cwd=ROOT) if ref else (ROOT/name).read_bytes()).decode('cp1251')
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:const char \*|(?:void|bool|uint8_t|uint16_t|uint32_t|cmdCode_e|logControl_e)\s+)'+name+r'\s*\([^;]*?\)\s*\{',text)
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
        # Timing: one command 13 in flight for 20 s, one 10-minute limit saved every minute, install failure ends the update.
        assert '#define FW_BLOCK_RETRY_MS       20000u' in user and '#define FW_TIMEOUT_S            600u' in user,'timing constants'
        loop=function(user,'user_loop')
        assert '>= FW_BLOCK_RETRY_MS){' in loop and 'fw_end("TIMEOUT", uniqIDCnt++);' in loop and 'updateTimeout % 60u == 0' in loop and 'fw_end("INSTALL", uniqIDCnt++);' in loop,'loop'
        assert 'fw_idle' not in user and '15000' not in loop,'old limits'
        saved=function(user,'user_readsavedinformation')
        assert 'device.FWinProgress = DV.FWinProgress;' in saved and 'device.updateTimeout = DV.updateTimeout;' in saved and saved.index('fw_resume_after_boot();')>saved.index('while (read_count--)'),'resume at boot'
        reset=function(user,'user_factoryReset')
        assert reset.index('device.FWinProgress = 0;')<reset.index('user_setdefault();'),'factory reset cancels a download'
        types=typedefs(source('structures.h'),'logSystem_e','logControl_e','cmdCode_e','dataChanel_e','bootFWparam_s')
        defines='\n'.join(x for x in user.splitlines() if x.startswith('#define FW_'))+'\n'
        helpers=user[user.index('/* Update mode, status 1 bit 11'):user.index('// Разбор данных')]
        unit=(ROOT/'Tests/test_fw_start.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',defines+helpers)
    elif kind=='log':
        user=source('User/user_fun.c');st=source('structures.h')
        case=cases(user)
        # The controller writes no command or exchange events (answers of 16.09, sheet E525).
        assert 'log_addComandEvent' not in user and 'log_addTransferEvent' not in user and 'LOG_CMD_' not in st and 'LOG_TR_' not in st and 'LT_SYSTEM' not in st,'command and exchange events'
        # Command 8: the source picks the event before the outputs; the admin test is logged too.
        c8=case(8)
        assert c8.index('server_out_event = cmd8_event(ptr[5]);')<c8.index('user_activateOut((ptr[6] != 0)'),'command 8 source'
        assert 'log_addControlEvent(LOG_CON_OUTONBYTEST, tempStrBuffer, 0, 255);' in c8,'admin test logged'
        assert 'log_addControlEvent(server_out_event, id, 0, cnt);' in function(user,'user_activateOut'),'server event from its source'
        code='\n'.join(function(user,n) for n in ['log_addSystemEvent','log_addSystemEventID','log_addControlEvent'])
        unit=(ROOT/'Tests/test_log.c').read_text().replace('/* TYPES */',typedefs(st,'logSystem_e','logControl_e','cellLog_s')).replace('/* PRODUCTION */',code)
    elif kind=='access':
        user=source('User/user_fun.c');st=source('structures.h')
        case=cases(user)
        # Relays (Д-07): permissions are worked out before any way of opening, both relays never forced.
        check=function(user,'user_checkIdAndControl')
        assert check.index('bool onOut[2]')<check.index('if (user_findTempID(id)){'),'relay permissions first'
        assert 'user_activateOut(0,' not in user and 'user_activateOut(1,' not in user and check.count('user_activateOut(onOut[0], onOut[1], tp, id);')==5 and check.count('user_activateOut(')==5,'requested relays only'
        c8=case(8)
        assert 'user_activateOut((ptr[6] != 0) && config.rele[0].onCmdServer,' in c8 and '(ptr[7] != 0) && config.rele[1].onCmdServer, CT_SERVER, tempStrBuffer);' in c8,'command 8 outputs'
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
        code=re.search(r'(?m)^static uint16_t id_cells_end;',user).group(0)+'\n'
        code+='\n'.join(function(user,n) for n in ['shedule_time_ok','user_findID','user_getNumbersCount','user_activateOut'])
        unit=(ROOT/'Tests/test_access.c').read_text().replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='rules':
        user=source('User/user_fun.c')
        rules=user[user.index('/* Status 1 out of turn (table E92'):user.index('/* Update mode, status 1 bit 11')]
        defines=[x for x in user.splitlines() if x.startswith('#define TIMETOSTATUSRESPONSE')][0]+'\n'
        # Call sites: each rule must be applied where the table puts it.
        loop=function(user,'user_loop')
        assert loop.index('status_event_due(st, timers_getMillis())')>loop.index('device.status.stateOut2 = out_getstate(OUT2);'),'status out of turn after inputs and outputs are read'
        case=cases(user)
        assert 'status_event_sent(st, timers_getMillis());' in case(1),'registration does not trigger an extra status'
        c8=case(8);head=c8[:c8.index('if (ptr[5] != 0)')]
        assert 'if (fw_update_quiet()){' in head and 'CMD_RUNERROR' in head,'command 8 refused in update mode'
        sms=function(user,'user_smsreceived')
        gate=sms[sms.index('if (fw_update_quiet()'):sms.index('if (strcmp(v, "add") == 0){')]
        assert all('strcmp(v, "%s") == 0'%c in gate for c in ('add','reset','setout','default')) and '"imei"' not in gate and '"status"' not in gate,'SMS control commands refused in update mode'
        call=user[user.index('bool user_incomingcall (char* num, uint8_t typ)'):user.index('void user_comanderror')]
        assert 'fw_update_quiet' not in call,'calls keep working in update mode'
        # Update events of the journal sheet: 2 start (number or ROLLBACK), 3 just before the install
        # restart, 4 trial confirmed, 5 always with its reason, 6 foreign board.
        for ev,n in (('RUNUPDATE',1),('ENDUPDATE',1),('HWERRUPDATE',1),('GOTOUPDATE',0),('ERRUPDATE',0)):
            assert user.count('log_addSystemEvent(LOG_SYS_%s);'%ev)==n,(ev,user.count('log_addSystemEvent(LOG_SYS_%s);'%ev))
        assert user.count('fw_log(LOG_SYS_GOTOUPDATE, ')==2 and 'log_addSystemEvent(LOG_SYS_RUNUPDATE);\r\n        user_savereset();' in loop,'events 2 and 3'
        assert 'app_trial_confirm();\r\n          if (!app_trial_active()) log_addSystemEvent(LOG_SYS_ENDUPDATE);' in case(1),'event 4 on confirmation'
        assert '0x48570000+DOMOVOI_APP_HW' in source('CMSIS/STM32F10x/Source/startup_stm32f10x_md.s'),'board marker in the vector table'
        assert source('DomovoiNext.ewp').count('<state>DOMOVOI_APP_HW=139</state>')==2 and 'DOMOVOI_APP_HW' not in source('Bootloader/DomovoiBoot.ewp'),'board marker only in the application build'
        unit=(ROOT/'Tests/test_update_rules.c').read_text().replace('/* PRODUCTION */',defines+rules+function(user,'user_send_statusPack'))
    elif kind=='gsmreg':
        gsm=source('Modules/gsm.c')
        code='\n'.join(x for x in gsm.splitlines() if x.startswith('#define GSM_REG_'))+'\n'
        code+=function(gsm,'gsm_getregstate')
        unit=(ROOT/'Tests/test_gsm_reg.c').read_text().replace('/* PRODUCTION */',code)
    elif kind=='crc':
        user=source('User/user_fun.c')
        # Compile the actual CRC gate; the command switch is unreachable here.
        code=user[user.index('void user_parse_comand'):]
        code=code[:code.index('  switch (ptr[4])')]+'  (void)tmp32;\n}\n'
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
    (d/'test.c').write_text(unit)
    subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(d/'test.c'),'-o',str(d/'test.exe')],check=True)
    result=subprocess.run([str(d/'test.exe')]+sys.argv[3:],timeout=3 if kind=='tx' else 300)
    sys.exit(result.returncode)
