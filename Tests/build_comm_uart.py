"""Доработка: Кряжев А., 2026. Порт ARM и сравнение бюджета с HEAD, без IAR и платы."""
from pathlib import Path
import argparse, os, subprocess
from comm_uart_budget import arm_sections

p=argparse.ArgumentParser()
p.add_argument('--cc', default=__import__('os').environ.get('DOMOVOI_ZIG', r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'))
p.add_argument('--out', type=Path, default=Path('Release/comm-stage4_6/final'))
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
out=root/a.out
out.mkdir(parents=True,exist_ok=True)
os.environ['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global')
os.environ['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-local')
flags=['-target','thumb-freestanding-eabi','-mcpu=cortex_m3','-mthumb','-Oz','-std=c99',
       '-Wall','-Wextra','-Werror','-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER',
       '-I.','-IModules','-ICMSIS/Include','-ICMSIS/STM32F10x/Include',
       '-ISTM32F10x_StdPeriph_Driver/inc','-c']
profiles={'v5':[]}  # только v5 (решение владельца 03.10.2026)
# Сравниваем тем же компилятором, профилем и флагами, все выходы — в final.
baseline=out/'comm_uart-before.c'
baseline.write_text(subprocess.check_output(['git','show','HEAD:Modules/comm_uart.c'],cwd=root).decode('cp1251'),encoding='utf-8',newline='')
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