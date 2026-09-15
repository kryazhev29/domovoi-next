import pathlib, subprocess, sys, tempfile, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
ZIG=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'
ref=sys.argv[1] if len(sys.argv)>1 and sys.argv[1]!='work' else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return (subprocess.check_output(['git','show',ref+':'+name],cwd=ROOT) if ref else (ROOT/name).read_bytes()).decode('cp1251')
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:void|uint8_t|uint16_t|uint32_t|cmdCode_e)\s+'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    return text[m.start():i]
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
        case19=user[user.index('    case 19:'):user.index('    case 20:')]
        # The rollback branch must be reached through the new gate, not the old field checks.
        assert 'fw_start_rejected(' in case19 and case19.index('fw_start_rejected(')<case19.index('if (firmwareVersion == 1)'),'command 19 gate'
        assert '(firmwareSize & 3u)' not in case19,'old field checks still in command 19'
        # Answer codes: rollback 253, update accepted through fw_start_accepted; the last block ends the update either way.
        assert 'CMD_ROLLBACKRUN, cch' in case19 and 'fw_start_accepted(cch), cch' in case19 and 'CMD_SUCCESS, cch' not in case19,'command 19 answers'
        case20=user[user.index('    case 20:'):user.index('    case 21:')]
        assert case20.count('fw_blocks_done(')==2 and 'firmwarePercent = 252' not in case20,'command 20 end of update'
        # Blocks are programmed in place, the download flag survives a reset and every end of an update is saved.
        assert 'mem_write(adr, &ptr[7], 1024)' in case20 and 'mem_rewrite(adr' not in case20 and 'malloc' not in case20,'command 20 write'
        saved=function(user,'user_readsavedinformation')
        assert 'device.FWinProgress = DV.FWinProgress;' in saved and saved.index('fw_resume_after_boot();')>saved.index('while (read_count--)'),'resume at boot'
        assert 'if (cch != CH_USB) user_saveindex();' in case19,'command 19 saves the download'
        loop=user[user.index('    if (device.rebootToUpdate){'):user.index('    if (app_trial_expired()){')]
        assert loop.count('user_saveindex();')==3 and 'updateTimeout % 600u == 0' in loop,'loop saves boot failure, timeout and remaining time'
        assert 'device.updateTimeout = DV.updateTimeout;' in saved,'remaining timeout restored'
        reset=function(user,'user_factoryReset')
        assert reset.index('device.FWinProgress = 0;')<reset.index('user_setdefault();'),'factory reset cancels a download'
        unit=(ROOT/'Tests/test_fw_start.c').read_text().replace('/* PRODUCTION */','\n'.join(function(user,n) for n in ['fw_start_rejected','fw_start_accepted','fw_blocks_done','fw_resume_block','fw_resume_after_boot']))
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
