# Доработка: Кряжев А., 2026
import os, pathlib, subprocess, sys, tempfile, re
from temp_cleanup import TemporaryDirectory
from source_select import source_text, host_adapters
# DOMOVOI_ROOT направляет проверки в копию, например с намеренно внесённой ошибкой.
ROOT=pathlib.Path(os.environ.get('DOMOVOI_ROOT') or pathlib.Path(__file__).resolve().parents[1])
ZIG=__import__('os').environ.get('DOMOVOI_ZIG', str(pathlib.Path(tempfile.gettempdir())/'codex-zig/zig.exe'))
if len(sys.argv) > 1 and sys.argv[1] != 'work':
    sys.exit('Проверяется рабочий код: используйте run_host.py work <группа>.')
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return source_text(ROOT,name)
# Прочитать фикстуру блока в UTF-8; русские маркеры остаются служебными точками вставки.
def fixture(name):
    text=(ROOT/name).read_bytes().decode('utf-8')
    return text.replace('/* КОД */','/* PRODUCTION */').replace('/* ТИПЫ */','/* TYPES */')

def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:const char \*|char\s*\*\s*|uint8_t\s*\*\s*|(?:void|bool|int|RESULT|uint8_t|uint16_t|uint32_t|unsigned short|unsigned int|pInt8U|Int8U|Int16U|cmdCode_e|logControl_e)\s+)'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    body=text[m.start():i]
    return body
def typedefs(text,*names):
    # Упаковка соответствует structures.h: стенд использует настоящие размеры и смещения.
    out=[]
    for name in names:
        m=re.search(r'typedef\s+(?:enum|struct)\s*\{(?:[^{}]|\{[^{}]*\})*\}\s*'+name+r'\s*;',text)
        assert m,name
        out.append(m.group(0))
    return '#pragma pack(push,1)\n'+'\n'.join(out)+'\n#pragma pack(pop)\n'
from contract_mapping import REPLACEMENTS
if kind in REPLACEMENTS:
    sys.path.insert(0, str(ROOT/'Tests/asm'))
    from firmware_suite import select_image, run_cases
    select_image()
    target = REPLACEMENTS[kind]
    if target == 'commserver':
        from comm_server_host import run
        run()
    elif target == 'logic ELF':
        from test_logic_cleanup import reader_contracts
        reader_contracts()
    else:
        result = run_cases([target])
        sys.exit(0 if all(r['status']=='PASS' for r in result.values()) else 1)
    sys.exit(0)

if kind == 'tx':
    sys.path.insert(0, str(ROOT/'Tests/asm'))
    if 'DOMOVOI_ELF' not in os.environ:
        from firmware_suite import select_image
        select_image()
    import test_usb_source
    scenario = sys.argv[3] if len(sys.argv) > 3 else None
    result = getattr(test_usb_source, scenario)() if scenario else test_usb_source.run()
    print('PASS USB source', result)
    sys.exit(0)

with TemporaryDirectory(prefix='domovoi-host-') as d:
    d=pathlib.Path(d)
    if kind=='sms':
        user=source('User/c/user_*.c');strings=source('User/user_str_fun.c')
        code='\n'.join(function(strings,n) for n in ('skipchar','user_strcpy','user_parse_pin'))+'\n'
        code+=re.search(r'enum \{ UNIVERSAL_PIN_BASE[^}]*\};',user).group(0)+'\n'
        code+='\n'.join(function(user,n) for n in ('user_pin_valid','sms_command','user_last10digitsnumber','user_sendsms','user_comm_server','user_smsreceived','log_addSystemValue','user_comm_link','user_comm_balance','user_comm_low_signal'))
        links=re.search(r'enum \{ COMM_LINK_UP[^}]*\};',source('Modules/comm_server.h')).group(0)
        code=re.search(r'enum \{ SMS_UNKNOWN[^}]*\};',user).group(0)+'\n'+code
        types=typedefs(source('structures.h'),'logSystem_e','logControl_e','codeType_e')+'\n'+links
        unit=source('Tests/test_sms.c').replace('/* ТИПЫ ПРОТОКОЛА */',types).replace('/* КОД МОДУЛЯ */',code)
    elif kind=='uarttimer':
        uart=source('Modules/uart_stm32f10x.c')
        types=typedefs(uart,'uart_t')+'\n'
        code='\n'.join(function(uart.replace('inline bool uart_addbyte','bool uart_addbyte'),n) for n in ('uart_addbyte','uart_getstate','uart_isReceiving','uart_timer1ms'))
        unit=fixture('Tests/test_uart_timer.c').replace('/* TYPES */',types).replace('/* PRODUCTION */',code)
    elif kind=='usbresume':
        subprocess.run([sys.executable,str(ROOT/'Tests/test_iar_usb_idle_events.py')],check=True)
        sys.exit(0)
    elif kind=='usbdesc':
        unit=fixture('Tests/test_usb_descriptor.c').replace('/* PRODUCTION */',function(source('USB-FS-CDC_User/Src/usb_prop.c'),'Virtual_Com_Port_GetStringDescriptor'))
    elif kind=='outs':
        import re as _re
        outs=_re.sub(r'(?m)^#include.*$','',source('User/outs.c'))
        # DWT вне модели; используется настоящая реализация реле.
        unit=fixture('Tests/test_outs.c').replace('/* PRODUCTION */','void prof_relay_on(void) {}\n'+outs)
    elif kind=='wiegandirq':
        wg=source('User/wiegand.c')
        code='\n'.join(x for x in wg.splitlines() if x.endswith(';') and re.match(r'^(?:(?:static|volatile) )?(?:(?:uint32_t|uint8_t|bool) wg|struct .* wg_frames)',x))+'\n'
        code+='\n'.join(function(wg,n) for n in ('wg_parity_valid','EXTI2_IRQHandler','EXTI3_IRQHandler','WG_process'))
        prof='#ifndef WG_PARITY\n#define WG_PARITY 1\n#endif\nuint32_t prof_card_mark;\n#define PROF_CARD_MARK() (prof_card_mark = PROF_NOW() | 1u)\n#define PROF_NOW() (0u)\n#define PROF_STEP(i, call) do { call; } while (0)\n'
        unit=fixture('Tests/test_wiegand_irq.c').replace('/* PRODUCTION */',prof+code)
    elif kind=='timersirq':
        timers=source('Modules/timers.c').replace('inline void timers_handle','void timers_handle')
        code='\n'.join(x for x in timers.splitlines() if x.startswith(('unsigned int milliSeconds','unsigned int timer[','volatile unsigned int milliSeconds','volatile unsigned int timer[','#define timer_cnt')))+'\n'
        if 'static unsigned int timers_snapshot(' in timers:code+=function(timers,'timers_snapshot')+'\n'
        code+='\n'.join(function(timers,n) for n in ('timers_set','timers_get','timers_handle','timers_pause','timers_take','timers_getMillis'))
        unit=fixture('Tests/test_timers_irq.c').replace('/* PRODUCTION */',code)
    elif kind=='usbresetwait':
        pwr=source('USB-FS-CDC_User/Src/usb_pwr.c');code=''
        if 'uint8_t USB_WaitReset(' in pwr:code+=function(pwr,'USB_WaitReset')+'\n'
        unit=fixture('Tests/test_usb_reset_wait.c').replace('/* PRODUCTION */',code)
    elif kind=='usbsetupbounds':
        usb=source('STM32_USB-FS-Device_Driver/src/usb_core.c')
        block=usb[usb.index('      Related_Endpoint = (pInformation->USBwIndex0 & 0x0f);'):usb.index('  else if (Request_No == GET_CONFIGURATION)')]
        block=block[:block.rfind('    }')]
        code='\n'.join(function(usb,n) for n in ('Standard_GetStatus','Standard_ClearFeature','Standard_SetEndPointFeature'))
        code+='\nstatic uint8_t *(*audit_getstatus(void))(uint16_t){\nuint32_t Related_Endpoint,Reserved,Status;\nuint8_t *(*CopyRoutine)(uint16_t)=NULL;\nif(pInformation->USBwIndex1)return NULL;\n'+block+'\nreturn CopyRoutine;\n}'
        unit=fixture('Tests/test_usb_setup_bounds.c').replace('/* PRODUCTION */',code)
    elif kind=='idexpiry':
        user=source('User/c/user_*.c')
        unit=(ROOT/'Tests/test_id_expiry.c').read_bytes().decode('utf-8').replace('/* ТИПЫ */',typedefs(source('structures.h'),'cellID_s')).replace('/* ИСХОДНИК */','\n'.join(function(user,n) for n in ('id_first_pass','id_expired','id_time_end')))
    elif kind=='tempidmeta':
        ids=source('User/c/user_ids.c');begin=ids.index('// 5 бит поколения');end=ids.index('void user_ids_boot(',begin)
        unit=source('Tests/test_tempid_meta.c').replace('/* ТИПЫ ПРОТОКОЛА */',typedefs(source('structures.h'),'cellID_s','codeType_e')).replace('/* КОД МОДУЛЯ */',function(source('User/c/user_*.c').replace('static inline bool id_equal','static bool id_equal'),'id_equal')+'\n'+ids[begin:end])
    elif kind=='commuart':
        header=source('Modules/comm_uart.h')
        code=source('Modules/comm_uart.c')
        code='\n'.join(x for x in code.splitlines() if not x.startswith('#include'))
        unit=source('Tests/test_comm_uart.c').replace('/* ЗАГОЛОВОК МОДУЛЯ */',header).replace('/* КОД МОДУЛЯ */',code)
        unit=unit.replace('/* ТИК */',function(source('stm32_it.c'),'SysTick_Handler'))
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
    else:
        raise ValueError(kind)
    if 'fast_' in unit:
        (d/'fast_kernels.h').write_text(source('User/fast_kernels.h'), encoding='utf-8')
        unit='#include "fast_kernels.h"\n'+unit.replace('#include "fast_kernels.h"','')
    unit=unit.replace('#include \"domovoi_diag.h\"','').replace('#include \"../domovoi_diag.h\"','')
    (d/'test.c').write_text(host_adapters(ROOT)+unit.replace('\r\n','\n'))
    flags=['-DWG_PARITY=0'] if kind=='wiegandirq' and sys.argv[3:]==['0'] else []
    subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',*flags,str(d/'test.c'),'-o',str(d/'test.exe')],check=True)
    result=subprocess.run([str(d/'test.exe')]+sys.argv[3:],timeout=3 if kind=='tx' else 300)
    sys.exit(result.returncode)
