"""Доработка: Кряжев А., 2026. Побайтовое сравнение провода до/после чистки."""
import argparse,json,os,struct
from pathlib import Path
from c_vs_asm_suite import select_image
from test_photo_runtime import PhotoMachine

class BeforePhoto(PhotoMachine):
    def capture(self,data):
        self.begin_camera(len(data));a=self.pget('address',4);stamp=self.pget('stamp',4)
        for off in range(0,len(data),58):self.data(data[off:off+58])
        assert self.flash[a:a+8]==struct.pack('<II',stamp,len(data))
        assert self.flash[a+1033:a+1033+len(data)]==data
        return a,stamp

def image(before,variant):
    base=Path(os.environ['DOMOVOI_COMPARE_BUILD'])
    select_image(variant,base/'before' if before else base)

def scenario(before,variant,channel,queue):
    image(before,variant);m=(BeforePhoto if before else PhotoMachine)();m.boot_media()
    stamps=[]
    for data in queue:stamps.append(m.capture(data)[1])
    m.packet(21,b'',channel);m.drain()
    for _ in range(20):
        m.port();m.drain()
        if m.sent and m.sent[-1][9]==0:break
    first=(list(m.commands),list(m.events),list(m.answers),list(m.sent))
    m.sent=[];m.packet(21,struct.pack('<IH',stamps[-1],17),channel);m.drain()
    for _ in range(20):
        m.port();m.drain()
        if m.sent and m.sent[-1][9]==0:break
    resumed=(list(m.answers),list(m.sent))
    m.sent=[];m.packet(21,struct.pack('<IH',stamps[-1],len(queue[-1])),channel);m.drain()
    m.port();m.drain()
    return first,resumed,(list(m.answers),list(m.sent))

def run(variant):
    count=0
    for channel in (0,1):
        for queue in ([b'x'*58],[bytes(range(256))*3],[b'a'*59,b'b'*700]):
            old=scenario(True,variant,channel,queue);new=scenario(False,variant,channel,queue)
            assert old==new,(variant,channel,list(map(len,queue)),'wire changed')
            count+=1
    return count

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--variant',choices=('C','HYBRID'),default='HYBRID');args=p.parse_args()
    print('PASS wire before/after',args.variant,run(args.variant),'scenarios')
