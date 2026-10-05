"""Power cuts at NOR byte boundaries; recovery executes the final ASM machine."""
import hashlib,json,struct
from test_photo_runtime import *

def cut_image(flash,kind,a,b,n):
    cut=bytearray(flash)
    for i,x in enumerate(b[:n]):cut[a+i]=x if kind=='erase' else cut[a+i]&x
    return cut

class Rebooter:
    def __init__(self):
        self.m=PhotoMachine();self.ram=bytes(self.m.u.mem_read(0x20000000,0x5000))
    def boot(self,flash):
        m=self.m;m.u.mem_write(0x20000000,self.ram);m.flash[:]=flash;m.io=None;m.fault=None
        m.costs=[];m.counts={'read':0,'start':0,'poll':0};m.answers=[];m.sent=[];m.events=[];m.commands=[];m.rx_count=0;m.now=0
        m.boot_media();return m

def capture_cuts():
    initial=prefix_flash();m=PhotoMachine(initial);m.boot_media();m.record_changes=True;m.capture(b'z'*129)
    flash=bytearray(initial);count=0;reboot=Rebooter();old=bytes(initial[0x200000:0x200ff7])
    max_resume=0
    for kind,a,b in m.changes:
        points=set((0,1,len(b)//2,len(b)-1,len(b)))
        if a in (0x2ff000,0x2ff00c,0x200ff7,0x200ffb):points.update(range(min(len(b),16)+1))
        for n in sorted(points):
            r=reboot.boot(cut_image(flash,kind,a,b,n));max_resume=max(max_resume,max(r.costs))
            assert r.flash[0x200000:0x200ff7]==old,(kind,hex(a),n,'prefix lost')
            stamp,size=struct.unpack('<II',r.flash[0x200ff7:0x200fff])
            number=int.from_bytes(r.u.mem_read(r.device+r.d['countPhoto'],2),'little')
            if stamp!=0xffffffff:
                assert stamp>=51 and size==129 and number==2,(kind,hex(a),n,stamp,size,number)
                assert r.flash[0x200ff7+1033:0x200ff7+1033+129]==b'z'*129
            else:assert number==1
            assert r.flash[0x2ff000:0x300000]==b'\xff'*4096,(kind,hex(a),n,'metadata not cleaned')
            count+=1
        flash=cut_image(flash,kind,a,b,len(b))
    print('capture/prefix',count,'cuts PASS',flush=True)
    return {'cuts':count,'max_resume_instructions':max_resume}

def acknowledgement_cuts():
    m=PhotoMachine();m.boot_media();address,stamp=m.capture(b'a'*700);initial=bytes(m.flash)
    m.record_changes=True;m.packet(21,struct.pack('<IH',stamp,512));m.drain()
    flash=bytearray(initial);reboot=Rebooter();count=0
    for kind,a,b in m.changes:
        for n in range(len(b)+1):
            r=reboot.boot(cut_image(flash,kind,a,b,n))
            done=r.checked('photo_confirmed_offset',address,700)
            assert done in (0,512),(hex(a),n,done)
            assert r.flash[address+1033:address+1733]==b'a'*700
            count+=1
        flash=cut_image(flash,kind,a,b,len(b))
    print('ack',count,'byte cuts PASS',flush=True);return count

def clean(m):
    for _ in range(1800):
        m.port();m.drain()
        if m.rget('initialized') and not m.rget('cleanup') and not m.active() and int.from_bytes(m.u.mem_read(m.device+m.d['newAddressPhoto'],4),'little')==0x200000:return
        assert not m.rget('fault')
    raise AssertionError('cleanup did not terminate')

def cleanup_cuts():
    initial=prefix_flash(dirty=False);initial[0x200000+1032]=0
    initial[0x200ff7:0x200fff]=struct.pack('<II',51,129)
    initial[0x200ff7+1032]=0;initial[0x200ff7+1033:0x200ff7+1033+129]=b'x'*129
    m=PhotoMachine(initial);m.boot_media();m.record_changes=True;clean(m)
    flash=bytearray(initial);reboot=Rebooter();count=0
    for kind,a,b in m.changes:
        points=set((0,1,len(b)//2,len(b)-1,len(b)))
        if a==0x2ff000:points.update((4,12,16,28,32,33,36,40,44,45,46,64,128))
        for n in sorted(points):
            r=reboot.boot(cut_image(flash,kind,a,b,n));clean(r)
            assert r.flash[0x200000:0x202000]==b'\xff'*8192,(kind,hex(a),n)
            assert r.flash[0x2ff000:0x300000]==b'\xff'*4096
            count+=1
        flash=cut_image(flash,kind,a,b,len(b))
    print('cleanup',count,'cuts PASS',flush=True);return count

if __name__=='__main__':
    result={'capture_prefix':capture_cuts(),'acknowledgements':acknowledgement_cuts(),'cleanup':cleanup_cuts()}
    result['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/photo_power_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS photo power recovery, committed offsets, prefix and cleanup')
