# Доработка: Кряжев А., 2026
# Модель прошивки для ELF-стендов без IAR: clang+lld под thumbv7m, те же имена символов и DWARF.
# Образ больше IAR (-Oz без inline, libc модели), поэтому ROM модели 224 КиБ; размеры по нему не мерить.
# Выход: DOMOVOI_COMPARE_BUILD/Release/ExePath/DomovoiNext.{out,bin}, ListPath/DomovoiNext.map.
import os, re, subprocess, sys, pathlib
S = pathlib.Path(__file__).resolve().parent
R = pathlib.Path(os.environ.get('DOMOVOI_ROOT', S.parents[1]))
B = pathlib.Path(os.environ.get('DOMOVOI_COMPARE_BUILD', str(S/'build')))
exe = B/'Release'/'ExePath'; lst = B/'Release'/'ListPath'; obj = B/'obj'
for d in (exe, lst, obj): d.mkdir(parents=True, exist_ok=True)
ewp = (R/'DomovoiNext.ewp').read_text(encoding='utf-8')
sources = sorted({m.replace('\\', '/') for m in re.findall(r'<name>\$PROJ_DIR\$\\([^<]+\.c)</name>', ewp)})
sources += ['Tests/reference/crc_asm.c']
extra = [str(S/'startup_clang.c'), str(S/'libc_min.c')]
flags = ['--target=thumbv7m-none-eabi', '-mcpu=cortex-m3', '-mthumb', '-std=gnu99', '-Oz', '-g', '-ffunction-sections', '-fdata-sections',
  '-fno-builtin', '-fshort-enums', '-fgnu89-inline', '-fno-common', '-fno-inline', '-U__arm', '-D__CORE__=8',
  '-DASM_CRC', '-DSTM32F10X_MD', '-DUSE_STDPERIPH_DRIVER', '-DRELEASE_BUILD', '-DDOMOVOI_DIAG=' + os.environ.get('DOMOVOI_TEST_DIAG', '1'),
  '-DDOMOVOI_APP_HW=139', '-include', str(S/'shim'/'iar_shim.h'), '-I' + str(S/'shim')]
flags += ['-I' + str(R/p) for p in ('.', 'Bootloader', 'CMSIS/Include', 'CMSIS/STM32F10x/Include', 'Modules',
  'STM32F10x_StdPeriph_Driver/inc', 'STM32_USB-FS-Device_Driver/inc', 'USB-FS-CDC_User/Inc', 'User', 'User/c')]
flags += ['-Wno-unknown-pragmas', '-Wno-pragma-pack', '-w']
objects = []
failed = 0
# Правки модели: IAR-прагмы без аналога у clang заменяются атрибутами в копии файла.
PATCHES = {'User/work_memory.c': [('#pragma data_alignment=8\nstatic work_memory_s work_memory;',
                                   'static work_memory_s work_memory __attribute__((aligned(8)));')]}
for src in sources + extra:
    path = R/src if not src.startswith('/') else pathlib.Path(src)
    if src in PATCHES:
        text = path.read_text(encoding='utf-8')
        for a, b in PATCHES[src]:
            assert a in text, (src, a); text = text.replace(a, b)
        patched = obj/('patched_' + path.name); patched.write_text(text, encoding='utf-8')
        import shutil; shutil.copystat(path, patched); path = patched
    o = obj/(path.stem + '_' + str(abs(hash(str(path))) % 10000) + '.o')
    if o.exists() and o.stat().st_mtime >= path.stat().st_mtime and not os.environ.get('DOMOVOI_REBUILD'):
        objects.append(str(o)); continue
    r = subprocess.run(['clang', *flags, '-c', str(path), '-o', str(o)], capture_output=True, text=True)
    if r.returncode:
        failed += 1; print('ОШИБКА', src); print(r.stderr[:2000])
    else: objects.append(str(o))
if failed: sys.exit('не собрано: %d' % failed)
out = exe/'DomovoiNext.out'
r = subprocess.run(['ld.lld', '-T', str(S/'link.ld'), '-o', str(out), *objects], capture_output=True, text=True)
if r.returncode: sys.exit('компоновка: ' + r.stderr[:3000])
subprocess.run(['llvm-objcopy', '-O', 'binary', str(out), str(exe/'DomovoiNext.bin')], check=True)
# Карта в духе IAR: адрес и размер данных для map_addr/map_size.
from elftools.elf.elffile import ELFFile
lines = []
with out.open('rb') as f:
    for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols():
        if not s.name or s['st_info']['type'] in ('STT_SECTION', 'STT_FILE'): continue
        kind = 'Data' if s['st_info']['type'] == 'STT_OBJECT' else ('Code' if s['st_info']['type'] == 'STT_FUNC' else 'Abs')
        lines.append("%-40s 0x%08x  0x%x  %s" % (s.name, s['st_value'], s['st_size'], kind))
(lst/'DomovoiNext.map').write_text('\n'.join(lines) + '\n', encoding='utf-8')
size = subprocess.check_output(['llvm-size', str(out)], text=True)
print(size)
print('bin', (exe/'DomovoiNext.bin').stat().st_size, 'Б; модель:', B)
