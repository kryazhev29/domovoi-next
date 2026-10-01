"""Этап 7: размеры/размещение только по IAR map, функциональные проверки ELF."""
import sys
from iar_irq_support import Machine

kind=sys.argv[1] if len(sys.argv)>1 else 'tables'
if kind=='tables':
    for boot in (False,True):
        m=Machine(boot=boot);m.init()
        names=('Device_Table','Device_Property','User_Standard_Requests','Device_Descriptor','Config_Descriptor','String_Descriptor')
        sizes={n:m.map_size(n) for n in names}
        for n in names:assert 0x08000000<=m.map_addr(n)<0x08020000, ('USB table still in RAM',n)
        m.u.mem_map(0x42000000,0x600000);m.u.mem_map(0x1ffff000,0x1000)
        m.call('USB_Init')
        for fn,desc,length in (('Virtual_Com_Port_GetDeviceDescriptor','Virtual_Com_Port_DeviceDescriptor',18),('Virtual_Com_Port_GetConfigDescriptor','Virtual_Com_Port_ConfigDescriptor',67)):
            p=m.call(fn,length);assert p==m.addr(desc)
            assert m.u.mem_read(p,1)[0] in (18,9)
        print('RAM tables', 'boot' if boot else 'app',sum(sizes.values()),'bytes moved to ROM; USB init/descriptors PASS')
elif kind=='mem':
    m=Machine();m.init();assert m.map_size('mem')==80, ('mem table not reduced',m.map_size('mem'))
    print('RAM mem[10]: map 80 bytes PASS')
elif kind=='log':
    m=Machine();assert 'log_pendingAt' not in m.symbols,'duplicate log uptime array still allocated'
    assert m.map_size('log_pending')==512
    print('RAM log: duplicate uptime array removed, pending map 512 bytes PASS')
