"""Мутации износа: возврат головы фото и A/B/SJ на каждый ограниченный проход."""
from firmware_suite import ROOT
from test_flash_mutations import run as mutate

def counter_erases():
    from test_counter_wear import smoke
    smoke()

def photo_erases():
    from test_flash_wear import fast
    from test_flash_wear2 import RingPhoto
    from collections import Counter
    m=fast(RingPhoto());m.boot_ring();counts=Counter();original=m.change
    def change(kind,a,b):
        if kind=='erase':counts[a]+=1
        original(kind,a,b)
    m.change=change
    for _ in range(16):
        _,n=m.capture_policy(13266);m.ack(n,13266)
    assert max(counts.values())==1,dict(counts)

def run():
    counter=(ROOT/'User/c/counter_runtime.inc').read_text(encoding='utf-8')
    counter=counter.replace('if (!ct.initialized || app_trial_active()) return 0;','if (1) return 0;')
    photo=(ROOT/'User/c/photo_runtime.inc').read_text(encoding='utf-8')
    photo=photo.replace('pr.bytes=pj.queue_bytes;','pr.bytes=pj.queue_bytes;\n  if (pr.circular && !pj.count) device.newAddressPhoto=PHOTO_RING_START;')
    return mutate({
      'counter-per-pass-SJ':('counter_erases',[('User/c/user_store.c','#include "counter_runtime.inc"',counter)]),
      'photo-reset-head':('photo_erases',[('User/c/photo_runtime.c','#include "photo_runtime.inc"',photo)])
    },module='test_flash_wear2_mutations',suite='flash-wear2-mutations')
