"""2 MiB NOR profile: explicit FF reads / refused writes outside the chip."""
from storage_machine import struct
LIMIT=0x200000

def real_layout():
    """Run production partition initialization, not the model's fixture map."""
    from storage_machine import StorageMachine
    layouts=[]
    for chip in (0x14c2,0x17c2):
        m=StorageMachine();m.u.mem_write(m.addr('mem'),bytes(80))
        m.stubs[m.addr('w25_read_devID')]=lambda chip=chip:m.ret(chip)
        for name in ('mem_recover','user_readconfig','user_initvariables',
                     'app_boot_metadata','WG_init','rs485_init','uart_setRecTimeout'):
            m.stubs[m.addr(name)]=lambda:m.ret(1)
        m.checked('user_init')
        layout=list(struct.iter_unpack('<II',bytes(m.u.mem_read(m.addr('mem'),80))))
        assert layout[7]==(0x1d0000,98304),('real user_init log partition',hex(chip),layout[7])
        assert layout[2]==(0x41000,1601536) and layout[8]==(0x1e8400,49152)
        assert all(start+size<=LIMIT for start,size in layout[:9])
        assert layout[9]==((0,0) if chip==0x14c2 else (0x200000,0x100000))
        assert bool(m.checked('storage_has_index'))==(chip==0x17c2)
        layouts.append({'device_ID':hex(chip),'log':layout[7],'photo':layout[9]})
    return layouts

def profile(m):
    m.flash=m.flash[:LIMIT]
    m.write(m.device+m.d['memID'],0x14c2,2)
    m.u.mem_write(m.addr('mem')+9*8,bytes(8))
    m.outside=[]
    for name in ('w25_read_data','w25_write_data','w25_erase_sector','w25_async_start'):
        address=m.addr(name);original=m.stubs[address]
        def guard(original=original,name=name):
            a,p,n,_=m.args();size=4096 if name=='w25_erase_sector' or (name=='w25_async_start' and not p) else n
            if a+size>LIMIT:
                m.outside.append((name,a,size))
                if name=='w25_read_data':m.u.mem_write(p,b'\xff'*size);m.ret(1)
                else:m.ret(0)
            else:original()
        m.stubs[address]=guard
    return m

def run():
    layout=real_layout()
    from test_id_sync_fast import SyncMachine,fixture,cell,BASE
    from test_access_reads import AccessMachine
    from test_stage5_log import boot,append,records
    from test_upload_runtime import UploadMachine,log,wire
    from test_memory_runtime import config_machine,run_config,SLOT1
    from test_fw_service_runtime import FirmwareMachine
    from test_photo_runtime import PhotoMachine
    m=profile(fixture(SyncMachine(),0,total=8192));m.command(13,b''.join(cell(i) for i in range(1,31)),30);m.drain();m.verify(30,8192)
    assert not m.checked('storage_has_index') and not m.outside
    # All public entry points remain safe even with a stale READY state in RAM.
    m.write(m.addr('ix')+10,3,1);m.u.mem_write(0x21000400,cell(1)[2:12])
    for name,args in [('idindex_scanned',(0,30)),('idindex_poll',(BASE,)),('idindex_put',(1,0x21000400)),('idindex_commit',(0,30)),('idindex_table_commit',(0,30))]:
        if name in m.symbols:m.checked(name,*args)
    assert m.checked('idindex_find',BASE,0x21000400,0x21000600)&255==255 and not m.outside
    a=profile(AccessMachine());a.flash[:]=m.flash;a.write(a.addr('id_cells_end'),30,2)
    a.u.mem_write(0x21000000,cell(30)[2:12]);assert a.checked('user_findID',0x21000000,0x21000100)==1 and not a.outside
    u=profile(UploadMachine());u.fixture(10,[log(1)]);assert boot(u)[0]==1;append(u,2)
    u.request();u.drain_upload();assert u.sent==[wire(10,7,0,[log(1),log(2)])] and not u.outside
    c,old,new=config_machine();profile(c);assert c.checked('mem_config_begin',0x21000000,0x21000200);run_config(c)
    assert c.checked('mem_config_result')==2 and c.flash[SLOT1:SLOT1+1024]==new and not c.outside
    from iar_image import image_paths
    payload_size=(image_paths()[1].stat().st_size-512+3)&~3
    f=profile(FirmwareMachine());f.fixture(payload_size);f.begin();f.drain()
    assert f.sent[0][9:11]==bytes((19,0)) and f.flash[0x1200:0x1200+payload_size]==f.image and not f.outside
    p=profile(PhotoMachine());p.packet(7,b'\5');p.drain()
    assert p.answers and p.answers[-1][1]!=0 and not p.active() and not p.outside
    # The boundary itself is enforced (not a hidden 4 MiB backing store).
    m.u.mem_write(0x21000000,b'x');assert m.checked('w25_write_data',LIMIT,0x21000000,1)==0
    assert m.checked('w25_read_data',LIMIT,0x21000000,1)==1 and m.u.mem_read(0x21000000,1)==b'\xff'
    return {'bytes':LIMIT,'production_layout':layout,'base_functions':['ID sync','linear card search','journal','upload','configuration','firmware staging'],'firmware_payload_bytes':payload_size,'index':'disabled','photo':'error','outside_attempts_before_boundary_selftest':0}
