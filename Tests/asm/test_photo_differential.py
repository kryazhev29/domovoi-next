"""Доработка: Кряжев А., 2026. Побайтовое сравнение провода до/после чистки.
Проверяется машинный код выбранного IAR ELF; плата и сервер не используются.
Результат относится к модели аппаратных границ.
"""
import argparse,os,struct
from pathlib import Path
from c_vs_asm_suite import select_image
from test_photo_runtime import PhotoMachine

# Выбрать свежий или исходный ELF для сравнения поведения.
def image(before,variant):
    base=Path(os.environ['DOMOVOI_COMPARE_BUILD'])
    select_image(variant,base/'before' if before else base)

# Снять команды камеры, события, ответы и пакеты полного цикла выгрузки.
def scenario(before,variant,channel,queue):
    image(before,variant);m=PhotoMachine();m.boot_media()
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

# Сравнить команды 7 и 21 и весь поток пакетов 14 до и после второго прохода.
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
