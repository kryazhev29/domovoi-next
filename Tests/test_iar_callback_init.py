"""B40: реальные структуры параметров IAR на заранее загрязнённом стеке."""
from iar_irq_support import Machine, ROOT, ELFFile, DWARFExprParser, UC_ARM_REG_R0

layout = {}
with (ROOT/'Release/Exe/DomovoiNext.out').open('rb') as f:
    dwarf = ELFFile(f).get_dwarf_info()
    for cu in dwarf.iter_CUs():
        for die in cu.iter_DIEs():
            a = die.attributes.get('DW_AT_name')
            if die.tag != 'DW_TAG_typedef' or not a or a.value not in (b'ConnectParam_t', b'GsmParam_t'):
                continue
            fields = {}
            for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                if 'DW_AT_name' not in member.attributes:
                    continue
                off = member.attributes['DW_AT_data_member_location'].value
                if not isinstance(off, int):
                    off = DWARFExprParser(cu.structs).parse_expr(off)[0].args[0]
                fields[member.attributes['DW_AT_name'].value.decode()] = off
            layout[a.value.decode()] = fields

m = Machine(); m.init()
captured = {}
def capture(typ, field):
    pointer = m.u.reg_read(UC_ARM_REG_R0)
    captured[field] = m.read(pointer + layout[typ][field])
    m.ret(0)
m.stubs[m.addr('connection_create')] = lambda: capture('ConnectParam_t', 'Error')
m.stubs[m.addr('connection_open')] = lambda: m.ret()
m.u.mem_write(m.addr('config'), b'\xff' * m.map_size('config'))
m.u.mem_write(m.bottom, b'\xa5' * (m.top-m.bottom))
m.call('user_setconnections')
assert captured['Error'] == 0, ('B40 uninitialized Error', hex(captured['Error']))
for name in ('w25_read_devID', 'user_readsavedinformation', 'user_getNumbersCount', 'log_getRecordsCount', 'user_getTempIDRecordsCount', 'user_getPhotoCount'):
    m.stubs[m.addr(name)] = lambda: m.ret(0)
m.stubs[m.addr('usergsm_init')] = lambda: capture('GsmParam_t', 'ComandError')
m.u.mem_write(m.bottom, b'\x5a' * (m.top-m.bottom))
m.call('user_initvariables')
assert captured['ComandError'] == 0, ('B40 uninitialized ComandError', hex(captured['ComandError']))
print('B40 IAR: Error/ComandError default NULL on poisoned stack PASS')
