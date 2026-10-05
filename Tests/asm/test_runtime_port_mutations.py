"""Sequential mutations of actual linked Thumb instructions, never C references."""
import contextlib,io,runpy,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'Tests'))
import iar_irq_support as support
Original=support.Machine
for label,mask,want,change in [('busy',0xff00,0xd500,0x0100),
                              ('timeout',0xff00,0xd200,0x0100),
                              ('verified-success',0xffff,0x2002,0x0003),
                              ('runtime-read',0xffff,0x7800,0x5800)]:
    applied=[]
    class Mutant(Original):
        def __init__(self,*a,**kw):
            super().__init__(*a,**kw)
            start=self.addr('w25_read_data' if label=='runtime-read' else 'w25_async_poll')
            code=bytes(self.u.mem_read(start,40 if label=='runtime-read' else 220))
            locations=[i for i in range(0,len(code)-1,2) if int.from_bytes(code[i:i+2],'little')&mask==want]
            assert len(locations)==1,(label,locations)
            off=locations[0]; word=int.from_bytes(code[off:off+2],'little')^change
            self.u.mem_write(start+off,word.to_bytes(2,'little'))
            applied.append(start+off)
    support.Machine=Mutant
    try:
        with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(str(ROOT/'Tests/test_iar_store_async.py'))
    except AssertionError: print(label,'DETECTED in linked ASM')
    else:raise AssertionError((label,'survived'))
    finally:support.Machine=Original
    assert len(applied)==1,(label,'mutation was not applied')
print('PASS 4/4 linked-ASM mutations')
