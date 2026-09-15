import pathlib, subprocess, sys, tempfile, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
ZIG=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'
ref=sys.argv[1] if len(sys.argv)>1 and sys.argv[1]!='work' else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return (subprocess.check_output(['git','show',ref+':'+name],cwd=ROOT) if ref else (ROOT/name).read_bytes()).decode('cp1251')
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:void|uint8_t|uint16_t|uint32_t)\s+'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    return text[m.start():i]
with tempfile.TemporaryDirectory(prefix='domovoi-host-') as d:
    d=pathlib.Path(d)
    if kind=='rx':
        code=source('Modules/usb_stm32f10x.c').replace('#include "usb_stm32f10x.h"','').replace('#pragma data_alignment=4','')
        code=re.sub(r'__weak void usb_data_reading[^}]+}', '', code)
        user=function(source('User/user_fun.c'),'usb_data_reading').replace('void usb_data_reading','void app_usb_data_reading')
        unit=(ROOT/'Tests/test_rx.c').read_text().replace('/* PRODUCTION */',user+'\n'+code)
    elif kind=='tx':
        hw=source('USB-FS-CDC_User/Src/hw_config.c')
        header=source('USB-FS-CDC_User/Inc/hw_config.h')
        globals=hw[hw.index('uint8_t  USART_Rx_Buffer'):hw.index('static void IntToUnicode')]
        code='\n'.join(x for x in header.splitlines() if x.startswith('#define USART_RX_DATA_SIZE'))+'\n'
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
