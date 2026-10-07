"""Доработка: Кряжев А., 2026. Один проект и только измеренное ASM-ядро."""
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]

def main():
    projects=set(p.name for p in ROOT.glob('*.ewp'))
    assert projects=={'DomovoiNext.ewp'},projects
    assert set(p.name for p in ROOT.glob('*.eww'))=={'DomovoiNext.eww'}
    tree=ET.parse(ROOT/'DomovoiNext.ewp')
    assert {n.findtext('name') for n in tree.findall('configuration')}=={'Release','ReleaseCustomer'}
    names={Path(n.text.replace('\\','/')).name for n in tree.findall('.//file/name')}
    expected={'user_main.c','user_commands.c','user_access.c','user_ids.c','user_schedule.c',
              'user_sms.c','user_log.c','user_upload.c','user_protocol.c','user_fw.c',
              'user_store.c','user_config.c','user_modem.c','user_diag.c','user_rx.c'}
    assert expected<=names
    assert {p.name for p in (ROOT/'User/asm').glob('*.s')}=={'crc_asm.s'}
    assert {'crc_asm.s','crc_runtime.c','fast_kernels.c','spi_stm32f10x_runtime.c'}<=names
    assert 'crc_asm.c' not in names
    for project in (ROOT/'DomovoiNext.ewp',ROOT/'Bootloader/DomovoiBoot.ewp'):
        for kind,check,option in [('ICCARM','IExtraOptionsCheck','IExtraOptions'),('AARM','AExtraOptionsCheckV2','AExtraOptionsV2')]:
            for setting in ET.parse(project).findall(f'.//settings[name="{kind}"]'):
                options={o.findtext('name'):o.findtext('state') for o in setting.findall('data/option')}
                assert options[check]=='1' and '--source_encoding utf8' in options[option]
    print('PASS единый проект, состав ядер и UTF-8 IAR')

if __name__=='__main__':main()
