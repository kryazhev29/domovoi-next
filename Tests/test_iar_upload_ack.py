"""B28/Q5: IAR, настоящие helper/save/CRC/allocator и восстановление device."""
from iar_irq_support import Machine,ROOT,ELFFile,DWARFExprParser,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2
m=Machine()
with (ROOT/'Release/Exe/DomovoiNext.out').open('rb') as f:
    dwarf=ELFFile(f).get_dwarf_info();offset={}
    for cu in dwarf.iter_CUs():
        for die in cu.iter_DIEs():
            a=die.attributes.get('DW_AT_name')
            if not a or a.value!=b'device_s' or die.tag!='DW_TAG_typedef':continue
            for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                if 'DW_AT_name' not in member.attributes:continue
                location=member.attributes['DW_AT_data_member_location'].value
                if not isinstance(location,int):location=DWARFExprParser(cu.structs).parse_expr(location)[0].args[0]
                offset[member.attributes['DW_AT_name'].value.decode()]=location
size=m.map_size('device');cfgsize=m.map_size('config');d=m.addr('device')
flash=bytearray(b'\xff'*2097152);base=0x1CD400;failure=0;current_cmd=0;erases=0;max_stack=0
def load():
    a=m.u.reg_read(UC_ARM_REG_R0);p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R2)
    if failure==2:m.ret(0);return
    m.u.mem_write(p,bytes(flash[a:a+n]));m.ret(1)
def save():
    a=m.u.reg_read(UC_ARM_REG_R0);p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R2)
    flash[a:a+n]=m.u.mem_read(p,n);m.ret(1)
def erase():
    global erases
    saved=flash[base+cfgsize:base+cfgsize+size]
    field='readLogIDIndex' if current_cmd==10 else 'readTempIDIndex'
    usb=offset['reserv']+(4 if current_cmd==10 else 6)
    assert saved[offset[field]:offset[field]+2]==b'\0\0' and saved[usb:usb+2]==b'\0\0','erase before durable cursor reset'
    a=m.u.reg_read(UC_ARM_REG_R0)&~4095;flash[a:a+4096]=b'\xff'*4096;erases+=1;m.ret(1)
m.stubs[m.addr('mem_read')]=load;m.stubs[m.addr('mem_write')]=save;m.stubs[m.addr('mem_erase_sector')]=erase
m.stubs[m.addr('wdt_reload')]=lambda:m.ret()
m.stubs[m.addr('fw_resume_after_boot')]=lambda:m.ret()
def setup(cmd,ch):
    global current_cmd,failure,erases
    current_cmd=cmd;failure=erases=0;m.init();mem=m.addr('mem')
    for index,address in [(6,base),(7,0),(8,8192)]:
        m.write(mem+8*index,address);m.write(mem+8*index+4,4096)
    m.write(d+offset['logsCount' if cmd==10 else 'tempIDsCount'],1,2)
    m.write(d+offset['status'],1<<(12 if cmd==10 else 14),2)
    m.write(d+offset['newAddressLog' if cmd==10 else 'newAddressTempID'],32 if cmd==10 else 8224)
    flash[base:base+4096]=bytes(4096)
    name='log_unacked' if cmd==10 else 'temp_unacked';assert m.map_size(name)==4
    pending=m.addr(name)+(0 if ch==0 else 2);m.write(pending,32,2)
    return pending
def largest():
    lo,hi=0,m.heap_bytes
    while lo<hi:
        mid=(lo+hi+1)//2;p=m.call('__basic_malloc',mid)
        if p:m.call('__basic_free',p);lo=mid
        else:hi=mid-1
    return lo
for cmd in (9,10):
    for ch in (0,1):
        pending=setup(cmd,ch);cold=largest()
        assert m.call('upload_ack',cmd,ch);max_stack=max(max_stack,m.top-m.minimum)
        assert not m.read(pending,2) and m.call('upload_index',cmd,ch)==32
        assert largest()==cold,'ACK leaks heap'
        # Незавершённая следующая посылка не попала в device и не переживает запуск.
        m.write(pending,64,2)
        m.init();mem=m.addr('mem');m.write(mem+6*8,base);m.write(mem+6*8+4,4096)
        m.call('user_readsavedinformation');assert m.call('upload_index',cmd,ch)==32 and not m.read(pending,2)
        # Отказ heap/чтения не фиксирует подтверждение и не стирает область.
        pending=setup(cmd,ch);held=m.call('__basic_malloc',largest());assert held
        assert not m.call('upload_ack',cmd,ch) and m.call('upload_index',cmd,ch)==0 and m.read(pending,2)==32 and not erases
        m.call('__basic_free',held);failure=2
        assert not m.call('upload_ack',cmd,ch) and m.call('upload_index',cmd,ch)==0 and m.read(pending,2)==32
        failure=0;assert m.call('upload_ack',cmd,ch)
        assert m.call('upload_finish',cmd);assert erases==1 and m.call('upload_index',cmd,ch)==0
print(f'IAR upload ACK PASS: real CRC/save/allocator, map HEAP {m.heap_bytes}, durable cursor/reboot/OOM/read error, zero persisted before erase; executed ACK stack {max_stack} bytes with flash callbacks stubbed')
