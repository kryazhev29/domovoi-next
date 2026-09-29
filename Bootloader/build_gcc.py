"""Independent ARM GCC build; no programmer or connected board is accessed."""
from pathlib import Path
import subprocess, sys, xml.etree.ElementTree as E, shutil
B=Path(__file__).resolve().parent
cc=sys.argv[1] if len(sys.argv)>1 else shutil.which('arm-none-eabi-gcc')
if not cc: raise SystemExit('Pass the full path to arm-none-eabi-gcc.exe')
profile=sys.argv[2] if len(sys.argv)>2 else 'v5'
if profile not in ('v5','mini','modem'):raise SystemExit('Unknown board profile')
out=B/'build'/('gcc-'+profile);out.mkdir(parents=True,exist_ok=True)
root=E.parse(B/'DomovoiBoot.ewp').getroot()
cfg=root.find('configuration')
includes=[]
for o in cfg.findall('.//option'):
 if o.findtext('name')=='CCIncludePath2':
  includes += ['-I'+str(Path((s.text or '').replace('$PROJ_DIR$',str(B)))) for s in o.findall('state')]
flags=['-mcpu=cortex-m3','-mthumb','-Os','-g','-std=c99','-ffunction-sections','-fdata-sections','-Wall','-Wextra','-Wno-unused-parameter','-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER','-DBOOTLOADER']
if profile=='mini':flags+=['-DBOOT_BOARD_MINI','-DBOOT_HW_VERSION=11']
# old MODEM board (IP_v3) has its own number 140: the server and NextConfig tell it from v5 (139)
if profile=='modem':flags+=['-DBOOT_BOARD_MODEM','-DBOOT_HW_VERSION=140']
objects=[]
for node in root.findall('file/name'):
 source=Path(node.text.replace('$PROJ_DIR$',str(B)))
 if source.suffix=='.s':source=B/'startup_gcc.s'
 obj=out/(source.stem+'.o');objects.append(str(obj))
 subprocess.run([cc,*flags,*includes,'-c',str(source),'-o',str(obj)],check=True)
elf=out/'DomovoiBoot.elf'
subprocess.run([cc,*flags,'-nostartfiles','--specs=nano.specs','--specs=nosys.specs',*objects,'-T',str(B/'DomovoiBoot.ld'),'-Wl,--gc-sections','-Wl,-Map='+str(out/'DomovoiBoot.map'),'-o',str(elf)],check=True)
prefix=str(Path(cc).parent/'arm-none-eabi-')
subprocess.run([prefix+'objcopy.exe','-O','binary','--gap-fill','0xff',str(elf),str(out/'DomovoiBoot.bin')],check=True)
subprocess.run([prefix+'size.exe',str(elf)],check=True)
binary=(out/'DomovoiBoot.bin').read_bytes()
assert len(binary)==32768 and binary[0x7ffc]==0x32
print('Verified 32768-byte image:',out/'DomovoiBoot.bin')
