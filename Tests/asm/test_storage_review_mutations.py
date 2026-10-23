"""Возврат каждого исправленного дефекта хранения (отчёт владельца, блок 4) в копии исходников.

Собираются только затронутые объекты; рабочее дерево и исходный ELF не меняются.
Каждый возврат должен быть обнаружен своим стендом. Файлы .inc не являются
единицами сборки: их мутированная копия подключается из копии включающего .c.
"""
import os
from pathlib import Path
from firmware_suite import ROOT
from test_flash_mutations import run as mutate

INCLUDERS = {'User/c/table_runtime.inc': ('User/table_runtime.c', 'c/table_runtime.inc'),
             'User/c/photo_runtime.inc': ('User/c/photo_runtime.c', 'photo_runtime.inc'),
             'User/c/counter_runtime.inc': ('User/c/memory_runtime.c', 'counter_runtime.inc')}


# Мутирует .inc в копии и возвращает замену строки #include во включающем .c.
def inc(tag, name, old, new):
    base = Path(os.environ['DOMOVOI_COMPARE_BUILD']) / 'storage-review-mutations' / os.environ.get('DOMOVOI_COMPARE_CONFIG', 'Release') / 'inc'
    base.mkdir(parents=True, exist_ok=True)
    text = (ROOT / name).read_text(encoding='utf-8')
    assert text.count(old) == 1, (tag, name, 'место мутации изменилось')
    copy = base / (tag + '-' + Path(name).name)
    copy.write_bytes(text.replace(old, new).replace('\n', '\r\n').encode('utf-8'))
    includer, include = INCLUDERS[name]
    return (includer, f'#include "{include}"', f'#include "{copy.as_posix()}"')


def groups():
    return [
        ('test_table_runtime', {
            'table-cancel-loses-metadata': ('test_cancel_after_write', [inc('table-cancel-loses-metadata', 'User/c/table_runtime.inc',
                '(table_job.scan_full == 2 || (!table_job.scan_full && !table_job.failed))',
                '(table_job.scan_full == 2 || result == FX_OK)')]),
            'table-crc-after-cancel': ('test_cancel_after_write', [inc('table-crc-after-cancel', 'User/c/table_runtime.inc',
                '    if (table_job.cancelled && !table_job.scan_full) {\n      flash_job.step=TABLE_REPLY;\n      return;\n    }\n',
                '')]),
        }),
        ('test_photo_edges', {
            'photo-wrap-skips-oldest': ('wrap_keeps_oldest', [inc('photo-wrap-skips-oldest', 'User/c/photo_runtime.inc',
                '    if (pr.oldest>=photo->address) return 0; // Хвост занят самым старым снимком: заворот его сотрёт.\n',
                '')]),
        }),
        ('test_counter_wear', {
            'counter-replaced-drops-crc': ('replace_keeps_crc', [inc('counter-replaced-drops-crc', 'User/c/counter_runtime.inc',
                '  uint16_t end=ct.used;\n  ct_row row,last;\n',
                '  idcrc_unknown();\n  uint16_t end=ct.used;\n  ct_row row,last;\n')]),
        }),
        ('test_expiry_runtime', {
            'expiry-journal-only': ('layer', [('User/c/expiry_runtime.c',
                '    int written=counter_write(p->target,(const cellID_s *)p->record);\n',
                '    int written=0;\n')]),
        }),
        ('test_memory_runtime', {
            'cursor-marker-forced': ('cursor_without_marker', [('User/c/memory_runtime.c',
                '    if (magic!=LOG_CURSOR_MAGIC) memset(saved->reserv+28,0,4); // Образ без маркера: номер журнала не хранил.\n',
                '')]),
        }),
        # Блок 5: писатель загрузчика, праздники, служебные USB-команды.
        ('test_iar_boot_review', {
            'ram-writer-single-read': ('ram_writer', [('Bootloader/app_update.c',
                '  for(tries=8;tries;tries--) {\n   *(volatile uint32_t*)0x40003000u=0xaaaau;',
                '  for(tries=1;tries;tries--) {\n   *(volatile uint32_t*)0x40003000u=0xaaaau;')]),
            'ram-writer-single-program': ('ram_writer', [('Bootloader/app_update.c',
                '  for(tries=4;tries;tries--) {\n   *sr=0x34u;*cr=2u;*ar=page;*cr=0x42u;',
                '  for(tries=1;tries;tries--) {\n   *sr=0x34u;*cr=2u;*ar=page;*cr=0x42u;')]),
        }),
        ('test_access_reads', {
            'holiday-no-early-exit': ('holidays', [('User/c/user_schedule.c',
                '      if(value==0xffffffffu) return 0; // Команда 16 пишет праздники подряд: дальше стёрто.\n',
                '')]),
        }),
        ('test_usb_service_commands', {
            'command-107-any-channel': ('run', [('User/c/user_commands.c',
                '    case 107: // режим прозрачной передачи gsm\n      if (cch == CH_USB)\n        user_send_anyData(107, NULL, 0, cch, uniqId);',
                '    case 107: // режим прозрачной передачи gsm\n      user_send_anyData(107, NULL, 0, cch, uniqId);')]),
        }),
    ]


def run():
    results = {}
    for module, mutations in groups():
        results.update(mutate(mutations, module, 'storage-review-mutations'))
    return results


if __name__ == '__main__':
    print(run())
