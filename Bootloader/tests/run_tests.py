from pathlib import Path
import argparse,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--cc',required=True);a=p.parse_args()
b=Path(__file__).resolve().parents[1];out=b/'build/host-tests';out.mkdir(parents=True,exist_ok=True)
cmd=[a.cc];flags=['-O2','-std=c11','-Wall','-Wextra','-Werror']
if Path(a.cc).stem=='zig':cmd+=['cc'];flags+=['-fno-sanitize=all','-mcpu=baseline']
for name in ('test_core','test_usb'):
 exe=out/(name+'.exe' if sys.platform=='win32' else name)
 subprocess.run([*cmd,*flags,'-I'+str(b),str(b/'boot_core.c'),str(b/'tests'/f'{name}.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
exe=out/('test_manual.exe' if sys.platform=='win32' else 'test_manual')
subprocess.run([*cmd,*flags,'-I'+str(b),str(b/'tests/test_manual.c'),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
subprocess.run([sys.executable,str(b/'tests/test_tool.py')],check=True)
