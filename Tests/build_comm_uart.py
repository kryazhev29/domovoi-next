"""Доработка: Кряжев А., 2026. Отдельная компиляция порта ARM, без IAR и платы."""
from pathlib import Path
import argparse, os, subprocess

p=argparse.ArgumentParser()
p.add_argument('--cc', default=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe')
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
out=root/'Release/comm-stage1'
out.mkdir(parents=True,exist_ok=True)
os.environ['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-global')
os.environ['ZIG_LOCAL_CACHE_DIR']=str(out/'zig-local')
flags=['-target','thumb-freestanding-eabi','-mcpu=cortex_m3','-mthumb','-Oz','-std=c99',
       '-Wall','-Wextra','-Werror','-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER',
       '-I.','-IModules','-ICMSIS/Include','-ICMSIS/STM32F10x/Include',
       '-ISTM32F10x_StdPeriph_Driver/inc','-c','Modules/comm_uart.c']
profiles={'v5':[], 'ipv3':['-DBOARD_IP_V3'], 'mini':['-DDOMOVOI_APP_HW=140'],
          'mini-ipv3':['-DDOMOVOI_APP_HW=140','-DBOARD_IP_V3']}
for name,defines in profiles.items():
    subprocess.run([a.cc,'cc',*flags,'-DCOMM_UART_STANDALONE',*defines,
                    '-o',str(out/('comm_uart-'+name+'.o'))],cwd=root,check=True)
guard=subprocess.run([a.cc,'cc',*flags,'-o',str(out/'disabled.o')],cwd=root,capture_output=True)
assert guard.returncode and b'select standalone USART3 owner' in guard.stderr
(out/'owner-guard.log').write_bytes(guard.stderr)
print('comm_uart ARM: 4 profiles, no warnings, explicit owner guard PASS')
