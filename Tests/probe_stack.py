"""Reproduce IAR 9.50's server callback cycle with three stack-control rules.

Uses existing objects from build_iar.py; all probe artifacts stay in TEMP.
The minimal rule set reproduces the crash with real firmware objects. It does
not claim a standalone minimal C reproduction or a certified stack bound.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

RULES = '''possible calls channel_emit [comm_channel.o]: server_channel_event [comm_server.o];
possible calls server_link [comm_server.o]: user_comm_link;
possible calls server_channel_event [comm_server.o]: user_comm_session, comm_recover_no_internet;
'''


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build-root',type=Path,default=Path(tempfile.gettempdir())/'domovoi-c-asm')
    parser.add_argument('--project',choices=('ASM','C','HYBRID'),default='ASM')
    args=parser.parse_args()
    commands=json.loads((args.build_root/(args.project+'-Release')/'commands.json').read_text(encoding='utf-8-sig'))
    command=next(c['arguments'].copy() for c in commands if Path(c['arguments'][0]).stem.lower()=='ilinkarm')
    while '--stack_usage_control' in command:
        index=command.index('--stack_usage_control');del command[index:index+2]
    with tempfile.TemporaryDirectory(prefix='domovoi-stack-cycle-') as tmp:
        folder=Path(tmp);control=folder/'minimal.suc';control.write_text(RULES,encoding='ascii')
        command[command.index('-o')+1]=str(folder/'probe.out')
        command[command.index('--map')+1]=str(folder/'probe.map')
        result=subprocess.run([*command,'--stack_usage_control',str(control)],capture_output=True)
        log=result.stdout+result.stderr
        reproduced=b'Internal error' in log and b'0x8' in log
        print('PASS' if reproduced else 'FAIL','three-rule IAR AccessViolation',result.returncode)
        print(log.decode(errors='replace')[-900:])
        return 0 if reproduced else 1


if __name__=='__main__':
    raise SystemExit(main())
