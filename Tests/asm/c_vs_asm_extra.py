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
    from verification_usb_try import UsbTryMachine
    class PortableQueue(UsbTryMachine):
        expected_stack=None
        instruction_limit=None
    m=PortableQueue();cases=0;maximum=0
    for mask in (0,1):
        for inp in (0,1,639,1278,1279):
            for out in (0,1,639,1278,1279,1280):
                for length in (0,1,64,1278,1279,1280,65535):
                    result,steps=m.run(inp,out,length,mask)
                    free=((0 if out==1280 else out)+1279-inp)%1280
                    assert result==int(length<1280 and length<=free)
                    maximum=max(maximum,steps);cases+=1
    for options in (dict(enabled=0),dict(configured=0),dict(configured=4),dict(epoch=8),dict(stored_epoch=6),dict(connected=False),dict(null=True),dict(inp=1280),dict(out=1281)):
        for mask in (0,1):assert m.run(mask=mask,**options)[0]==0
    for mask in (0,1):assert m.run(length=0,null=True,mask=mask)[0]==1
    points=m.run(1277,1277,8)[1]
    for point in range(points+1):m.run(1277,1277,8,irq=(point,'reset'))
    return {'queue_cases':cases,'irq_cases':points+1,'maximum_instructions':maximum,'stack':m.top-m.minimum}
