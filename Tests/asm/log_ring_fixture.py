"""Physical sector fixtures; the oracle remains a list of logical records."""
import struct
BASE=0x1d0000
MAGIC=0xd74c4752
CAPACITY=24*127

def pair(n):return n|((n^65535)<<16)
def install(m,cells,first_seq=1):
    assert len(cells)<=CAPACITY
    m.flash[BASE:BASE+98304]=b'\xff'*98304
    previous=0
    for s in range((len(cells)+126)//127):
        part=cells[s*127:(s+1)*127];seq=first_seq+s
        count=sum(valid(c) for c in part)
        header=struct.pack('<8I',MAGIC,seq,seq^0xffffffff,previous,previous^0xffffffff,
            pair(count) if (s+1)*127<len(cells) else 0xffffffff,0xffffffff,0xffffffff)
        a=BASE+s*4096;m.flash[a:a+32]=header
        m.flash[a+32:a+32+32*len(part)]=b''.join(part)
        for c in part:
            if valid(c):previous=max(previous,int.from_bytes(c[:4],'little'))

def valid(c):
    from storage_machine import crc16
    if len(c)!=32 or c==b'\xff'*32:return False
    if c[24:]==bytes(8):return True
    crc=int.from_bytes(c[24:26],'little')
    return c[26:28]==(crc^65535).to_bytes(2,'little') and c[28:]==b'LOG2' and crc==crc16(c[:24])
