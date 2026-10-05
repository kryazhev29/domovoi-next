"""Independent old C-linked 3450335 vs new ASM: camera, NOR format and wire frames."""
import shutil,json,hashlib
from test_photo_runtime import *
import iar_irq_support as support

class LegacyPhoto(PhotoMachine):
    def active(self):return False
    def port(self):self.now+=1;self.checked('port_check_data',budget=1000000)
    def read(self):
        a,p,n,_=self.args();assert 0<n<=2048 and a+n<=len(self.flash)
        self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    def boot_media(self):self.checked('user_getPhotoCount',budget=1000000)
    def begin_camera(self,size):
        self.packet(7,b'\5');assert self.answers[-1][1]==0 and self.pget('stage')==1
        self.port();self.feed(bytes([0xaa,0x0e,0x0d,0,0,0]));self.feed(bytes([0xaa,0x0d,0,0,0,0]))
        for ack in (1,6,5):
            self.now+=50;self.port();self.feed(bytes([0xaa,0x0e,ack,0,0,0]))
        self.now+=50;self.port();self.feed(bytes([0xaa,0x0a,1,size&255,(size>>8)&255,(size>>16)&255]))
        for _ in range(300):
            if self.pget('stage')==6:return
            self.port()
        raise AssertionError('legacy preparation did not terminate')
    def capture(self,data):
        self.begin_camera(len(data));a=self.pget('address',4);stamp=self.pget('stamp',4)
        for off in range(0,len(data),58):
            self.next_request();assert self.pget('packet',2)==off//58
            body=data[off:off+58];p=struct.pack('<HH',off//58,len(body))+body
            self.feed(p+bytes([sum(p)&255,0]))
        assert self.events[-1]==stamp and self.flash[a:a+8]==struct.pack('<II',stamp,len(data))
        assert self.flash[a+1033:a+1033+len(data)]==data
        return a,stamp

def main():
    saved=ROOT/'Release/task-asm-storage';baseline=saved/'baseline-root'
    for folder,ext in (('Exe','out'),('Exe','bin'),('List','map')):
        target=baseline/'Release'/folder/f'DomovoiNext.{ext}';target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(saved/f'DomovoiNext.{ext}',target)
    result={}
    for name,initial,data in (('empty',None,bytes(range(256))*3),('prefix4087',prefix_flash(),b'z'*129)):
        support.ROOT=baseline;old=LegacyPhoto(initial)
        support.ROOT=ROOT;new=PhotoMachine(initial)
        for m in (old,new):m.boot_media();a,stamp=m.capture(data)
        assert old.flash==new.flash and old.events==new.events,(name,'capture format')
        for m in (old,new):
            m.packet(21,b'');m.drain()
            for _ in range(20):
                m.port();m.drain()
                if m.sent and m.sent[-1][9]==0:break
        assert old.sent==new.sent,(name,'wire frames')
        for m in (old,new):
            m.packet(21,struct.pack('<IH',stamp,min(100,len(data))));m.drain()
        assert old.flash==new.flash,(name,'confirmed offsets')
        result[name]={'frames':len(new.sent),'bytes':len(data),'flash_equal':True}
    result['baseline_elf_sha256']=hashlib.sha256((saved/'DomovoiNext.out').read_bytes()).hexdigest()
    result['final_elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/photo_differential_results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS old C-linked ELF / new ASM: exact complete NOR image, numbering, wire data/CRC, persistent ACKs')

if __name__=='__main__':main()
