"""Проверка контролёра на намеренно добавленном мёртвом объекте IAR."""
import json
import os
from pathlib import Path
import shutil
import subprocess
from temp_cleanup import TemporaryDirectory
from check_dead_code import inspect
from iar_image import result_path

def main():
    base=result_path('.')
    with TemporaryDirectory(prefix='dead-code-',dir=base) as temporary:
        root=Path(temporary);sources=root/'source';sources.mkdir()
        source=sources/'probe.c'
        source.write_bytes(b'unsigned audit_unread;\r\nvoid audit_discarded(void) { audit_unread=7; }\r\n')
        obj=root/'probe.o'
        iar=Path(os.environ.get('DOMOVOI_IAR',r'C:\Program Files\IAR Systems\Embedded Workbench 9.2'))
        subprocess.run([str(iar/'arm/bin/iccarm.exe'),str(source),'--cpu','Cortex-M3','--thumb','-Ohz','--source_encoding','utf8','-o',str(obj)],capture_output=True,check=True)
        for folder,stem in [('Release','DomovoiNext'),('ReleaseCustomer','DomovoiNext'),('BOOT-Release','DomovoiBoot')]:
            for sub,extension in [('ExePath','out'),('ListPath','map')]:
                target=root/folder/sub;target.mkdir(parents=True,exist_ok=True)
                shutil.copy2(base/folder/sub/f'{stem}.{extension}',target/f'{stem}.{extension}')
            (root/folder/'commands.json').write_text(json.dumps([{'arguments':['iccarm.exe'],'output':str(obj)}]),encoding='utf-8')
        report=inspect(root,source_root=sources)
        assert any(d['name']=='audit_discarded' and d['emitted_object'] for d in report['functions'])
        assert 'audit_unread' in report['unread_data']
    print('PASS dead-code guard: discarded IAR function and unread global detected')

if __name__=='__main__':main()
