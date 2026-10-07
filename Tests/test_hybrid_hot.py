"""Доработка: Кряжев А., 2026. Контракт перевода холодной логики гибрида на C."""
from pathlib import Path
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[1]
# Проверить состав обоих проектов: монолит и холодный ASM должны быть удалены.
def main():
    assert not (ROOT / 'DomovoiNext_ASM.ewp').exists()
    assert not (ROOT / 'User/hybrid').exists()
    assert not (ROOT / 'User/c/user_fun_runtime.c').exists()
    expected = {'user_main.c', 'user_commands.c', 'user_access.c', 'user_ids.c',
                'user_schedule.c', 'user_sms.c', 'user_log.c', 'user_upload.c',
                'user_protocol.c', 'user_fw.c', 'user_store.c', 'user_config.c',
                'user_modem.c', 'user_diag.c', 'user_rx.c'}
    for variant in ('C', 'HYBRID'):
        names = [node.text.replace('\\', '/').split('/')[-1]
                 for node in ET.parse(ROOT / ('DomovoiNext_' + variant + '.ewp')).findall('.//file/name')]
        assert expected <= set(names), variant
        if variant == 'HYBRID':
            kernels={Path(n.text.replace('\\','/')).name for n in ET.parse(ROOT/('DomovoiNext_'+variant+'.ewp')).findall('.//file/name') if '/User/asm/' in n.text.replace('\\','/')}
            assert kernels == {'crc_asm.s','crc_runtime.s','fast_kernels.s','spi_stm32f10x_runtime.s'}
            assert {p.name for p in (ROOT/'User/asm').glob('*.s')} == kernels
        assert not {'user_fun_runtime.c', 'user_fun_runtime.s', 'user_fun_hot.s'} & set(names), variant
    print('PASS холодная логика обоих проектов собирается из тематических C-модулей')
if __name__ == '__main__':
    main()
