"""Доработка: Кряжев А., 2026. Частный TX фото живёт до завершения отправки.
Проверяется машинный код выбранного IAR ELF; плата и сервер не используются.
Результат относится к модели аппаратных границ.
"""
import hashlib
import json
from test_photo_runtime import *
from test_photo_edges import reach


# Проверить, что частный пакет защищён от чужих команд до принятия транспортом.
def run():
    m = PhotoMachine(); m.boot_media(); m.capture(b'x'*768)
    m.packet(21, b'', 1); m.drain(); m.port(); reach(m,PHASE['SEND_FILL'])
    m.tick() # Выделить пакет 534 байта и прочитать первую порцию до 64 байт.
    from test_work_memory import take,drop,owned,UP_PACKET,PHOTO,PHOTO_PACKET
    reserve=take(m,UP_PACKET);assert reserve
    tested = []
    for phase in (PHASE['SEND_FILL'],PHASE['SEND_COMMIT']):
        reach(m, phase)
        pointer = m.rget('tx',4); assert pointer
        assert owned(m)==(1<<(PHOTO-1))|(1<<(PHOTO_PACKET-1))|(1<<(UP_PACKET-1))
        assert m.exec_active(8)
        assert not m.checked('flash_exec_idle')
        assert not m.checked('flash_exec_idle')
        assert not m.checked('mem_config_begin',0x21000000,0x21000200)
        before = dict(m.counts); saved = bytes(m.u.mem_read(pointer,534))
        for cmd in (9,10,13):
            m.packet(cmd)
            assert m.answers[-1][0:2] == (cmd,2)
            assert m.rget('tx',4) == pointer and m.exec_state()[1] == phase
        assert m.counts == before and bytes(m.u.mem_read(pointer,534)) == saved
        tested.append({'phase':phase,'live_photo_tx_bytes':534,'owned_slots':owned(m),'new_storage_commands':'rejected before allocation/I/O'})
    m.tick()
    assert not m.exec_active(8) and m.rget('tx',4) == 0
    assert owned(m)==(1<<(PHOTO-1))|(1<<(UP_PACKET-1))
    drop(m,UP_PACKET,reserve)
    from iar_image import image_paths
    return {'elf_sha256':hashlib.sha256(image_paths()[0].read_bytes()).hexdigest(),
            'phases':tested,'phase_zero_photo_tx':0,'workspace_bytes':2224,
            'conclusion':'Photo TX is guarded from accepted command9/10/13 and config snapshots; idle photo context can coexist.'}

if __name__ == '__main__':
    out = run()
    (__import__('pathlib').Path(__import__('os').environ['DOMOVOI_COMPARE_BUILD'])/'verification_photo_lifetime_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS linked photo TX lifetime:',out['phases'],flush=True)
