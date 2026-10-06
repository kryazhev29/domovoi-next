"""Current ELF adapters for older host scenarios; no output files on import."""
import importlib
import sys
from iar_image import image_paths

def event_append():
    from test_iar_event_async import measure
    return [measure(image_paths()[0],True,n) for n in (0,3071)]

def fw_blocks():
    from test_iar_fw_async import measure
    return [measure(image_paths()[0],True,a,b) for a,b in ((0,2),(3,5),(95,96))]

def fw_bridge():
    before=sys.argv;sys.argv=[before[0]]
    try:m=importlib.import_module('verification_fw_bridge')
    finally:sys.argv=before
    return {n:getattr(m,n)() for n in ('ack_order','config_failure','quiet_cases','abort_cases')}

def usb_queue():
    from iar_irq_support import Machine
    if 'USB_Tx_SourceBusy' in Machine().symbols:
        from test_usb_source import run
        return run()
    from verification_usb_try import UsbTryMachine
    class PortableQueue(UsbTryMachine):
        expected_stack=None
        instruction_limit=None
    m=PortableQueue();cases=0;maximum=0
    for mask in (0,1):
        for inp in (0,1,m.size//2,m.size-2,m.size-1):
            for out in (0,1,m.size//2,m.size-2,m.size-1,m.size):
                for length in (0,1,64,m.size-2,m.size-1,m.size,65535):
                    result,steps=m.run(inp,out,length,mask)
                    free=((0 if out==m.size else out)+(m.size-1)-inp)%m.size
                    assert result==int(length<m.size and length<=free)
                    maximum=max(maximum,steps);cases+=1
    for options in (dict(enabled=0),dict(configured=0),dict(configured=4),dict(epoch=8),dict(stored_epoch=6),dict(connected=False),dict(null=True),dict(inp=m.size),dict(out=(m.size+1))):
        for mask in (0,1):assert m.run(mask=mask,**options)[0]==0
    for mask in (0,1):assert m.run(length=0,null=True,mask=mask)[0]==1
    points=m.run((m.size-3),(m.size-3),8)[1]
    for point in range(points+1):m.run((m.size-3),(m.size-3),8,irq=(point,'reset'))
    # Maximum framed configurator reply: refuse one byte short, then retry
    # after the host consumes queued bytes. Publication and payload stay atomic.
    assert m.size==1040
    assert m.run(0,m.size-2,1038)[0]==0
    assert m.run(0,m.size-1,1038)[0]==1
    return {'backpressure_retry_bytes':1038,'queue_cases':cases,'irq_cases':points+1,'maximum_instructions':maximum,'stack':m.top-m.minimum}


def owner_lifetimes():
    before=sys.argv;sys.argv=[before[0]]
    try:
        upload=importlib.import_module('verification_upload_lifetime')
        photo=importlib.import_module('verification_photo_lifetime')
    finally:sys.argv=before
    return {'upload_ack':upload.reachable_ack(),'photo_tx':photo.run(),
            'empty_cleanup':upload.durable_empty(),'recovery_gate':upload.recovery_gate()}
