"""Загрузчик по пятому отчёту: повторы писателя из ОЗУ и запуск прошивки при отказе W25.

Выполняется машинный код IAR-образов (приложение и загрузчик) в Unicorn; плата не используется.
Пути образов — из окружения run_all: DOMOVOI_ELF, DOMOVOI_BIN, DOMOVOI_BOOT_ELF, DOMOVOI_BOOT_BIN.
"""
import os, struct, sys
from pathlib import Path
sys.path[:0] = [str(Path(__file__).resolve().parent)]
import tools_path  # noqa: F401  (unicorn и pyelftools в мк\Инструменты)
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_R0, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_XPSR, UC_ARM_REG_MSP

ROOT = Path(__file__).resolve().parents[1]
STOP = 0x0801FFF0


def symbols(path):
    elf = ELFFile(open(path, 'rb'))
    return {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}


def crc32(data):
    c = 0xFFFFFFFF
    for (w,) in struct.iter_unpack('<I', data):
        c ^= w
        for _ in range(32):
            c = ((c << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if c & 0x80000000 else (c << 1) & 0xFFFFFFFF
    return c


# --- Писатель загрузчика из ОЗУ (replace_ram приложения) ---------------------------------
def ram_after_init(sym, app):
    u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    u.mem_map(0x08000000, 0x20000); u.mem_write(0x08000000, b'\xa5' * 0x8200); u.mem_write(0x08008200, app)
    u.mem_map(0x20000000, 0x5000)
    u.reg_write(UC_ARM_REG_SP, 0x20005000); u.reg_write(UC_ARM_REG_LR, STOP | 1); u.reg_write(UC_ARM_REG_XPSR, 0x01000000)
    u.emu_start(sym['__iar_data_init3'] | 1, STOP, count=5000000)
    assert u.reg_read(UC_ARM_REG_PC) == STOP
    return bytes(u.mem_read(0x20000000, 0x5000)), bytes(u.mem_read(0x08000000, 0x20000))


def writer(sym, ram, flash, image, corrupt_page=None, corrupt_once=False, flash_error_page=None, cut=None):
    u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    u.mem_map(0x20000000, 0x5000); u.mem_write(0x20000000, ram)
    u.mem_map(0x08000000, 0x20000); u.mem_write(0x08000000, flash)
    u.mem_map(0x40000000, 0x30000); u.mem_map(0xE000E000, 0x2000)
    checks = b''.join(struct.pack('<I', crc32(image[i:i + 1024])) for i in range(0, 32768, 1024))
    u.mem_write(0x20004000, checks)
    st = dict(index=0, address=0, rx=0, flash_address=0, reset=False, mutations=0, erased=[],
              page_reads=[0] * 32, inject=False, injected=0)

    def write(uc, access, address, size, value, _):
        if address == 0x40010C14 and value & 4096:
            st['index'] = 0; st['address'] = 0
        elif address == 0x4001300C:
            i = st['index']; st['index'] += 1
            if i == 0: assert value == 3
            if 1 <= i <= 3: st['address'] = (st['address'] << 8) | (value & 255)
            if i >= 4:
                off = st['address']; assert 0 <= off < 32768
                page = off // 1024
                if i == 4: st['page_reads'][page] += 1
                damaged = page == corrupt_page and (not corrupt_once or st['page_reads'][page] == 1)
                st['rx'] = image[off] ^ (1 if damaged else 0); st['address'] += 1
            else:
                st['rx'] = 0
        elif address == 0x4002200C: st['inject'] = False  # Запись в SR снимает флаги ошибок, как в STM32.
        elif address == 0x40022014: st['flash_address'] = value
        elif address == 0x40022010 and value == 0x42:
            page = st['flash_address']; assert 0x08000000 <= page < 0x08008000
            st['erased'].append(page); uc.mem_write(page, b'\xff' * 1024); st['mutations'] += 1
        elif 0x08000000 <= address < 0x08020000:
            assert address + size <= 0x08008000, 'Application overwritten'
            st['mutations'] += 1
            # Одна ошибка программирования (PGERR) на заданной странице: писатель должен повторить её.
            if flash_error_page is not None and (address - 0x08000000) // 1024 == flash_error_page and not st['injected']:
                st['inject'] = True; st['injected'] = 1
        elif address == 0xE000ED0C:
            assert value == 0x05FA0004; st['reset'] = True; uc.emu_stop()
        if cut and st['mutations'] >= cut: uc.emu_stop()

    def read(uc, access, address, size, value, _):
        if address == 0x40013008: uc.mem_write(address, struct.pack('<I', 3))
        elif address == 0x4001300C: uc.mem_write(address, struct.pack('<I', st['rx']))
        elif address == 0x4002200C:
            uc.mem_write(address, struct.pack('<I', 0x10 if st['inject'] else 0))  # PGERR держится до сброса флага.

    def code(uc, address, size, _):
        assert 0x20000000 <= address < 0x20005000, f'Execution escaped RAM at 0x{address:08x}'

    u.hook_add(UC_HOOK_MEM_READ, read, begin=0x40000000, end=0x4002FFFF)
    u.hook_add(UC_HOOK_MEM_WRITE, write)
    u.hook_add(UC_HOOK_CODE, code)
    u.reg_write(UC_ARM_REG_SP, 0x20005000); u.reg_write(UC_ARM_REG_R0, 0x20004000); u.reg_write(UC_ARM_REG_XPSR, 0x01000000)
    u.emu_start(sym['replace_ram'] | 1, 0, count=40000000)
    assert bytes(u.mem_read(0x08008000, 0x18000)) == flash[0x8000:], 'application area changed'
    st['image'] = bytes(u.mem_read(0x08000000, 32768))
    return st


def ram_writer():
    sym = symbols(os.environ['DOMOVOI_ELF'])
    app = Path(os.environ['DOMOVOI_BIN']).read_bytes()
    image = Path(os.environ['DOMOVOI_BOOT_BIN']).read_bytes()
    assert len(image) == 32768
    ram, flash = ram_after_init(sym, app)
    fn = sym['replace_ram'] & ~1
    assert ram[fn - 0x20000000:fn - 0x20000000 + 8] != bytes(8), 'replace_ram was not copied to RAM'
    st = writer(sym, ram, flash, image)
    assert st['reset'] and st['image'] == image and len(st['erased']) == 32
    total = st['mutations']
    # Одиночная помеха SPI на странице 7 после стёртой страницы 0: повтор чтения, образ записан целиком.
    st = writer(sym, ram, flash, image, corrupt_page=7, corrupt_once=True)
    assert st['reset'] and st['image'] == image and len(st['erased']) == 32, (len(st['erased']), st['reset'])
    assert st['page_reads'][7] == 2 and st['page_reads'][6] == 1, st['page_reads']
    # Постоянная порча страницы 7: после ограниченного числа повторов писатель останавливается до её стирания.
    st = writer(sym, ram, flash, image, corrupt_page=7)
    assert st['reset'] and len(st['erased']) == 7 and st['page_reads'][7] == 8, (len(st['erased']), st['page_reads'][7])
    assert st['image'][:7 * 1024] == image[:7 * 1024]
    # Ошибка программирования один раз на странице 3: страница стёрта и записана заново, образ точный.
    st = writer(sym, ram, flash, image, flash_error_page=3)
    assert st['reset'] and st['image'] == image and len(st['erased']) == 33, (len(st['erased']), st['reset'])
    assert st['erased'].count(0x08000000 + 3 * 1024) == 2
    for cut in (1, 2, total - 1):
        st = writer(sym, ram, flash, image, cut=cut); assert st['mutations'] >= cut
    return {'flash_operations': total, 'spi_retries': 8, 'program_retries': 4}


# --- Запуск прошивки загрузчиком при отказе внешней флеш-памяти -----------------------------
LEGACY = 0x57464C53


def header(magic, hw, version, body, stamp):
    h = bytearray(b'\xff' * 512)
    struct.pack_into('<IBHBIIII', h, 0, magic, hw, version, 0xFF, len(body), crc32(body), 0xFFFFFFFF, stamp)
    return bytes(h)


def boot(boot_bin, boot_sym, app, dead):
    u = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    flash = bytearray(b'\xff' * 0x20000); flash[:32768] = boot_bin
    flash[0x8000:0x8000 + 512 + len(app)] = header(LEGACY, 139, 50, app, 1789400000) + app
    u.mem_map(0x08000000, 0x20000); u.mem_write(0x08000000, bytes(flash))
    u.mem_map(0x20000000, 0x5000); u.mem_map(0x40000000, 0x30000); u.mem_map(0xE0000000, 0x100000)
    u.mem_map(0x1FFFF000, 0x1000); u.mem_write(0x1FFFF7E0, struct.pack('<H', 128))
    ticks = boot_sym['ticks']
    st = dict(app=None, usb=False, spi_bytes=0, buf=[])

    def rd(uc, access, addr, size, value, _):
        if addr == 0x40021000:
            uc.mem_write(addr, struct.pack('<I', struct.unpack('<I', uc.mem_read(addr, 4))[0] | 0x02020002))
        elif addr == 0x40021004:
            v = struct.unpack('<I', uc.mem_read(addr, 4))[0]; uc.mem_write(addr, struct.pack('<I', (v & ~0xC) | ((v & 3) << 2)))
        elif addr == 0x40013008: uc.mem_write(addr, struct.pack('<I', 0 if dead else 3))  # Мёртвая шина: флаги SPI не поднимаются.
        elif addr == 0x4001300C:
            # Исправная W25: регистр состояния «не занят», данные стёрты (журнала и кандидата нет).
            s = st['buf']; uc.mem_write(addr, struct.pack('<I', 0x00 if s and s[0] == 5 and len(s) >= 2 else 0xFF))
        elif addr == 0x40010808: uc.mem_write(addr, struct.pack('<I', 1 << 13))
        elif addr == 0x4002200C: uc.mem_write(addr, struct.pack('<I', 0))
        elif addr == ticks:
            t = struct.unpack('<I', uc.mem_read(ticks, 4))[0]; uc.mem_write(ticks, struct.pack('<I', (t + 1) & 0xFFFFFFFF))

    def wr(uc, access, addr, size, value, _):
        if addr == 0x4001300C: st['spi_bytes'] += 1; st['buf'].append(value & 0xFF)
        elif addr == 0x40010C14 and value & (1 << 12): st['buf'] = []
        elif 0x08000000 <= addr < 0x08008000: raise AssertionError(f'write into bootloader area 0x{addr:08x}')

    def app_code(uc, addr, size, _):
        st['app'] = addr; uc.emu_stop()

    def usb_code(uc, addr, size, _):
        st['usb'] = True; uc.emu_stop()

    u.hook_add(UC_HOOK_MEM_READ, rd, begin=ticks, end=ticks + 3)
    u.hook_add(UC_HOOK_MEM_READ, rd, begin=0x40000000, end=0x4002FFFF)
    u.hook_add(UC_HOOK_MEM_WRITE, wr, begin=0x40000000, end=0x4002FFFF)
    u.hook_add(UC_HOOK_MEM_WRITE, wr, begin=0x08000000, end=0x08007FFF)
    u.hook_add(UC_HOOK_CODE, app_code, begin=0x08008000, end=0x0801FFFF)
    entry = boot_sym['boot_usb_init'] & ~1
    u.hook_add(UC_HOOK_CODE, usb_code, begin=entry, end=entry + 1)
    sp, pc = struct.unpack_from('<II', boot_bin)
    u.reg_write(UC_ARM_REG_SP, sp)
    try:
        u.emu_start(pc | 1, 0x0801FFFF, timeout=120 * 1000000, count=400000000)
    except UcError as e:
        raise AssertionError(f'emulation fault {e} at pc=0x{u.reg_read(UC_ARM_REG_PC):08x}')
    st['vtor'] = struct.unpack('<I', u.mem_read(0xE000ED08, 4))[0]
    st['msp'] = u.reg_read(UC_ARM_REG_MSP)
    st['reset'] = struct.unpack_from('<II', flash, 0x8200)
    return st


def dead_flash_start():
    boot_bin = Path(os.environ['DOMOVOI_BOOT_BIN']).read_bytes(); assert len(boot_bin) == 32768
    boot_sym = symbols(os.environ['DOMOVOI_BOOT_ELF'])
    app = Path(os.environ['DOMOVOI_BIN']).read_bytes(); app += b'\xff' * ((-len(app)) % 4)
    out = {}
    for dead in (False, True):
        st = boot(boot_bin, boot_sym, app, dead)
        sp, pc = st['reset']
        assert st['app'] == pc & ~1 and not st['usb'], ('dead' if dead else 'healthy', hex(st['app'] or 0), st['usb'])
        assert st['vtor'] == 0x08008200 and st['msp'] == sp
        out['dead' if dead else 'healthy'] = st['spi_bytes']
    # Прежний загрузчик (до правки) с мёртвой шиной оставался в USB-загрузчике: стенд это различает.
    old = ROOT / 'Binaries/StorageReview_20261022/BOOT-Release/DomovoiBoot.bin'
    if old.exists() and old.read_bytes() != boot_bin:
        old_sym = dict(boot_sym)
        st = boot(old.read_bytes(), old_sym, app, True)
        out['previous_loader_stays_in_usb'] = bool(st['usb'] and st['app'] is None)
    return out


def main():
    print('ram writer', ram_writer())
    print('dead flash start', dead_flash_start())
    print('PASS boot review: RAM writer retries SPI and Flash faults, loader starts the application without W25')


if __name__ == '__main__':
    main()
