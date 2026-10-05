"""Build unchanged IAR configurations in TEMP; preserve a real capacity failure.

python Tests/asm/c_vs_asm_build.py --variant C --configuration ReleaseSpeed
Only --diagnostic expands addresses, in a temporary ICF. Never flash that image.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[2]

def build(variant,configuration,base,iar,diagnostic=False):
    stem='DomovoiNext' if variant=='baseline' else 'DomovoiNext_'+variant
    out=base/('baseline/Release' if variant=='baseline' else variant+'-'+configuration)
    out.mkdir(parents=True,exist_ok=True)
    tree=ET.parse(ROOT/(stem+'.ewp'))
    for node in [*tree.findall('.//state'),*tree.findall('.//file/name')]:
        if node.text:node.text=node.text.replace('$PROJ_DIR$',str(ROOT))
    for cfg in tree.findall('configuration'):
        for option in cfg.findall('.//option'):
            name=option.findtext('name');state=option.find('state')
            if name in ('ExePath','ObjPath','ListPath'):state.text=str(out/name)
            if diagnostic and name=='IlinkIcfFile':
                icf=Path(state.text).read_text()
                icf=icf.replace('0x0801FFFF','0x0803FFFF').replace('0x20004FBF','0x2000FFBF').replace('0x20004FC0','0x2000FFC0')
                target=out/'diagnostic.icf';target.write_text(icf);state.text=str(target)
    project=out/(stem+'.ewp');tree.write(project,encoding='utf-8',xml_declaration=True)
    command=[str(iar),str(project),'-build',configuration,'-parallel','4','-log','all']
    result=subprocess.run(command,cwd=ROOT,capture_output=True)
    log=result.stdout+result.stderr
    (out/('diagnostic-build.log' if diagnostic else 'build.log')).write_bytes(log)
    for line in log.decode(errors='replace').splitlines():
        if any(s in line for s in ('Error[','Warning[','Total number','Build succeeded')):print(line)
    return result.returncode

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--variant',choices=('baseline','ASM','C'),required=True)
    p.add_argument('--configuration',choices=('Release','ReleaseSpeed'),default='Release')
    p.add_argument('--build-root',type=Path,default=Path(tempfile.gettempdir())/'domovoi-c-asm')
    p.add_argument('--iar',type=Path,default=Path(os.environ.get('IAR_BUILD',r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\common\bin\IarBuild.exe')))
    p.add_argument('--diagnostic',action='store_true');a=p.parse_args()
    sys.exit(build(a.variant,a.configuration,a.build_root,a.iar,a.diagnostic))
