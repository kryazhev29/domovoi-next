"""Время модели UART/NOR: полный поток 14 и следующий снимок после ACK 21."""
import argparse,json,sys,math,struct
from pathlib import Path
from flash_wear_migration import select

def run(size,new):
    from photo_latency_audit import TimedCamera
    from test_flash_wear import fast
    from test_flash_wear2 import RingPhoto
    m=TimedCamera(115200,60)
    if new:
        m.flash.extend(b'\xff'*(0x1000000-len(m.flash)));m.write(m.addr('trial'),0,1)
    fast(m)
    if new:RingPhoto.boot_ring(m)
    else:m.boot_media()
    data=bytes((i*19)&255 for i in range(size))
    def capture():
        start=m.now;m.begin_camera(size);address=m.pget('address',4)
        for off in range(0,size,m.packet_size-6):
            payload=data[off:off+m.packet_size-6];m.next_request()
            m.now+=math.ceil((len(payload)+6)*10000/115200)+2+60;m.data(payload)
        assert m.flash[address+13:address+13+size]==data
        return m.events[-1],m.now-start
    number,capture_ms=capture();start=m.now;m.packet(21,b'');m.drain()
    for _ in range(1000):
        m.port();m.drain()
        if m.sent and m.sent[-1][9]==0:break
    else:raise AssertionError('wire did not finish')
    assert b''.join(p[20:-2] for p in m.sent[:-1])==data
    upload_ms=m.now-start;start=m.now;ops=dict(m.operations)
    m.packet(21,struct.pack('<IH',number,size));m.drain();ack_response=m.now-start
    m.checked('photo_storage_cancel');m.drain()
    for _ in range(100):
        m.port();m.drain()
        if (new or not m.rget('cleanup')) and m.rget('initialized') and not m.active():break
    ack_ready=m.now-start;ack_erases=m.operations['erase']-ops['erase']
    number,next_ms=capture()
    return dict(bytes=size,new_ring=new,capture_ms=capture_ms,upload_ms=upload_ms,
                ACK_response_ms=ack_response,ACK_to_ready_ms=ack_ready,ACK_erases=ack_erases,
                next_capture_ms=next_ms,ACK_plus_next_ms=ack_ready+next_ms,
                assumptions={'baud':115200,'camera_reply_ms':60,'program_ms':1,'erase_busy_ms':40,'poll_ms':1,'erase_verify_polls':64},wire_exact=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build-root',required=True);p.add_argument('--new',action='store_true');p.add_argument('--output',type=Path,required=True);a=p.parse_args();select(a.build_root)
    result=[run(size,a.new) for size in (13266,51200)]
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(result)
