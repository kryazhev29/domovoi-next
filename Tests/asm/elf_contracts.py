"""Проверка прежних сценариев на текущем ELF без записи результатов при импорте."""
import importlib
import sys
from iar_image import image_paths

def event_append():
    from test_stage5_log import run
    return run()

def fw_blocks():
    from test_iar_fw_async import measure
    return [measure(image_paths()[0],True,a,b) for a,b in ((0,2),(3,5),(95,96))]

def fw_bridge():
    before=sys.argv;sys.argv=[before[0]]
    try:m=importlib.import_module('verification_fw_bridge')
    finally:sys.argv=before
    return {n:getattr(m,n)() for n in ('ack_order','config_failure','quiet_cases','abort_cases')}

def usb_queue():
    from test_usb_source import run
    return run()


def owner_lifetimes():
    before=sys.argv;sys.argv=[before[0]]
    try:
        upload=importlib.import_module('verification_upload_lifetime')
        photo=importlib.import_module('verification_photo_lifetime')
    finally:sys.argv=before
    return {'upload_ack':upload.reachable_ack(),'photo_tx':photo.run(),
            'empty_cleanup':upload.durable_empty(),'recovery_gate':upload.recovery_gate()}
