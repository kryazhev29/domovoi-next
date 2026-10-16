"""Доработка: Кряжев А., 2026. Порт ARM и сравнение бюджета с HEAD, без IAR и платы."""
from pathlib import Path
import argparse, os, subprocess
from comm_uart_budget import arm_sections

p=argparse.ArgumentParser()
p.add_argument('--cc', default=__import__('tools_path').zig())
p.add_argument('--out', type=Path, default=Path('Release/comm-stage4_6/final'))
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
out=root/a.out
out.mkdir(parents=True,exist_ok=True)
os.environ['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global')
os.environ['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-local')
flags=['-target','thumb-freestanding-eabi','-mcpu=cortex_m3','-mthumb','-Oz','-std=c99',
       '-Wall','-Wextra','-Werror','-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER',
       '-DDOMOVOI_DIAG='+os.environ.get('DOMOVOI_TEST_DIAG','1'),'-IUser','-I.','-IModules','-ICMSIS/Include','-ICMSIS/STM32F10x/Include',
       '-ISTM32F10x_StdPeriph_Driver/inc','-c']
profiles={'v5':[]}  # только v5 (решение владельца 03.10.2026)
# Сравниваем тем же компилятором, профилем и флагами, все выходы — в final.
def historical_source(path):
    data=subprocess.check_output(['git','show','HEAD:'+path],cwd=root)
    # В прежних коммитах исходники cp1251, после перехода — UTF-8.
    try:
        return data.decode('utf-8')
    except UnicodeDecodeError:
        return data.decode('cp1251')

baseline=out/'comm_uart-before.c'
baseline.write_text(historical_source('Modules/comm_uart.c'),encoding='utf-8',newline='')
# Исторический драйвер собирается со своим заголовком, а не с новым контрактом.
(out/'comm_uart.h').write_text(historical_source('Modules/comm_uart.h'),encoding='utf-8',newline='')
for name,defines in profiles.items():
    before=out/('comm_uart-before-'+name+'.o')
    after=out/('comm_uart-'+name+'.o')
    for source,obj in ((str(baseline),before),('Modules/comm_uart.c',after)):
        subprocess.run([a.cc,'cc',*flags,*defines,source,
                        '-o',str(obj)],cwd=root,check=True)
    old,new=arm_sections(before),arm_sections(after)
    assert new['arm_flash_estimate'] <= old['arm_flash_estimate']+150, name
    assert new['arm_ram'] <= old['arm_ram'], name
    print(name, 'Flash',old['arm_flash_estimate'],'->',new['arm_flash_estimate'],
          'RAM',old['arm_ram'],'->',new['arm_ram'])
# USART3 принадлежит только comm_uart (старый GSM удалён) - проверка двух владельцев не нужна.
print('comm_uart ARM: v5, warnings=0, Flash <= HEAD+150, RAM <= HEAD PASS')
