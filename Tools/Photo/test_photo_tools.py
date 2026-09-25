# Доработка: Кряжев А., 2026
"""Локальные проверки сервера, GUI и эмулятора. COM и внешняя сеть не используются."""
import importlib.util
import io
import os
import queue
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path
import camera_emulator as camera
import photo_server_gui as gui

HERE = Path(__file__).resolve().parent
JPEG = (HERE / 'sample_640x480.jpg').read_bytes()


class Board(threading.Thread):
    daemon = True
    def __init__(self, port, imei='123456789012345', reset=False, status=8192, param_once=False, repeat_final=False):
        super().__init__(); self.port, self.imei, self.reset, self.status = port, imei, reset, status
        self.stop_event = threading.Event(); self.requests = []; self.commands = []; self.connections = 0
        self.index = self.offset = 0; self.error = None; self.reset_done = False
        self.images = [(1789990001, JPEG), (1789990002, JPEG)]
        self.expected = list(self.images)
        self.param_once, self.repeat_final = param_once, repeat_final
        self.param_done = self.final_timeout_done = False
        self.busy_once = self.busy_done = False; self.request_times = []

    def stream(self, args):
        self.requests.append(args)
        asked, offset = struct.unpack('<IH', args)
        if not self.images:
            yield b'\0'; return
        self.index = next((i for i, (n, _) in enumerate(self.images) if n == asked), 0) if asked else 0
        number, data = self.images[self.index]
        self.offset = min(offset, len(data))
        if self.offset == len(data):
            self.index += 1; self.offset = 0
            if self.index == len(self.images):
                self.images.clear(); yield b'\0'; return
        sent = 0
        while self.index < len(self.images):
            number, data = self.images[self.index]
            while self.offset < len(data):
                if self.reset and not self.reset_done and ((self.reset is True and sent == 15) or (self.reset == 'second' and self.index == 1 and self.offset >= 1024)):
                    self.reset_done = True
                    raise ConnectionResetError('модель сброса посреди потока')
                block = data[self.offset:self.offset + 512]
                yield struct.pack('<BIIH', len(self.images) - self.index, number, len(data), self.offset) + block
                self.offset += len(block); sent += 1
            self.index += 1; self.offset = 0
        yield b'\0'

    def run(self):
        try:
            while not self.stop_event.is_set():
                try:
                    with socket.create_connection(('127.0.0.1',self.port),timeout=1) as sock:
                        self.connections += 1; sock.settimeout(.1); pending=b''
                        sock.sendall(gui.build(1,self.imei.encode()+bytes([1,139]),1))
                        while not self.stop_event.is_set():
                            try: data=sock.recv(4096)
                            except socket.timeout: continue
                            if not data: break
                            frames,pending=gui.take_frames(pending+data)
                            for uniq,cmd,args in frames:
                                self.commands.append((cmd,args))
                                if cmd==1:
                                    sock.sendall(gui.build(2,struct.pack('<IH',int(time.time()),self.status),2))
                                elif cmd==21:
                                    asked,offset=struct.unpack('<IH',args)
                                    final=bool(self.images) and asked==self.images[-1][0] and offset==len(self.images[-1][1])
                                    self.request_times.append(time.monotonic())
                                    if self.busy_once and not self.busy_done:
                                        # Код 2: плата занята съёмкой или восстановлением сектора.
                                        self.busy_done=True
                                        sock.sendall(gui.build(11,bytes([21,2]),uniq))
                                        continue
                                    if final and self.param_once and not self.param_done:
                                        self.param_done=True
                                        self.requests.append(args)
                                        sock.sendall(gui.build(11,bytes([21,1]),uniq))
                                        continue
                                    # В настоящей прошивке ответ 11 всегда идёт первым.
                                    sock.sendall(gui.build(11,bytes([21,0]),uniq))
                                    blocks=self.stream(args)
                                    if final and self.repeat_final and not self.final_timeout_done:
                                        self.final_timeout_done=True
                                        for _ in blocks: pass
                                        continue
                                    for block in blocks:
                                        sock.sendall(gui.build(14,block,uniq))
                except OSError:
                    if not self.stop_event.is_set(): time.sleep(.02)
        except Exception as exc: self.error=repr(exc)


class Tests(unittest.TestCase):
    def test_camera(self):
        cam=camera.C328(JPEG)
        self.assertEqual(cam.feed(b'\xaa\x0d\0'),[])
        self.assertEqual(cam.feed(b'\0\0\0'),[b'\xaa\x0e\x0d\0\0\0\xaa\x0d\0\0\0\0'])
        cam.feed(b'\xaa\x01\0\x07\0\x07'); self.assertEqual(cam.resolution,7)
        for size in (64,512):
            cam.feed(bytes([0xAA,6,8,size&255,size>>8,0]))
            out=bytearray()
            for number in range((len(JPEG)+size-7)//(size-6)):
                p=cam.feed(b'\xaa\x0e\0\0'+struct.pack('<H',number))[0]
                ident,n=struct.unpack_from('<HH',p)
                self.assertEqual(ident,number);self.assertEqual(sum(p[:-2])&255,p[-2]);self.assertEqual(len(p),n+6)
                out.extend(p[4:-2])
            self.assertEqual(out,JPEG)
        bad=camera.C328(JPEG,error_packet=0).feed(b'\xaa\x0e\0\0\0\0')[0]
        self.assertNotEqual(sum(bad[:-2])&255,bad[-2])
        self.assertEqual(camera.C328(JPEG,silent=True).feed(b'\xaa\x0d\0\0\0\0'),[])
        gui.valid_jpeg(JPEG)
        with self.assertRaises(ValueError):gui.valid_jpeg(JPEG[:-2])

    def test_frames_and_fw_compatibility(self):
        frame=gui.build(21,struct.pack('<IH',10,512),7)
        for split in range(len(frame)):
            a,p=gui.take_frames(frame[:split]);b,p=gui.take_frames(p+frame[split:]);self.assertEqual(a+b,[(7,21,struct.pack('<IH',10,512))]);self.assertEqual(p,b'')
        bad=frame[:-1]+bytes([frame[-1]^1]);self.assertEqual(gui.take_frames(bad+frame)[0],[(7,21,struct.pack('<IH',10,512))])
        # Исходный fw_server.py только импортируется; его обновление FW не менялось.
        source=HERE.parents[5]/'Стенд_Domovoi'/'Скрипты'/'fw_server.py'
        if source.exists():
            spec=importlib.util.spec_from_file_location('existing_fw_server',source);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
            self.assertEqual(mod.take_frames(frame)[0],gui.take_frames(frame)[0])

    def run_server(self, faults, reset=False, count=1, param_once=False, repeat_final=False, busy_once=False):
        with tempfile.TemporaryDirectory() as td:
            events=queue.Queue();server=gui.Server(0,'127.0.0.1',td,events,retry=.12);server.start();self.assertTrue(server.ready.wait(3));self.assertIsNone(server.error)
            boards=[Board(server.port,f'{123456789012345+i:015d}',reset,param_once=param_once,repeat_final=repeat_final) for i in range(count)]
            for board in boards:board.busy_once=busy_once;board.start()
            started=set();finished=set();messages=[];deadline=time.monotonic()+35
            try:
                while time.monotonic()<deadline and len(finished)<count:
                    try: kind,value=events.get(timeout=5)
                    except queue.Empty:
                        self.fail('\n'.join(messages[-20:]) + '\n' + repr([b.error for b in boards]))
                    if kind=='clients':
                        for p in value:
                            if p['imei'] and p['status'] and p['imei'] not in started:
                                started.add(p['imei']);server.command(p['key'],'get',faults)
                    elif kind=='log':
                        messages.append(value)
                        if 'очередь пуста' in value:
                            for b in boards:
                                if b.imei in value:finished.add(b.imei)
                self.assertEqual(len(finished),count, '\n'.join(messages[-12:]))
                for b in boards:
                    self.assertIsNone(b.error)
                    for number,_ in b.expected:self.assertEqual((Path(td)/'Фото_с_камеры'/f'{b.imei}_{number}.jpg').read_bytes(),JPEG)
                    self.assertEqual(b.requests[0],struct.pack('<IH',0,0))
                    if not faults and not reset and not param_once and not repeat_final:
                        self.assertEqual(b.requests, [struct.pack('<IH',0,0),
                                                      struct.pack('<IH',b.expected[-1][0],len(JPEG))])
                    if param_once:
                        self.assertTrue(b.param_done)
                        self.assertEqual(b.requests[2],struct.pack('<IH',0,0))
                    if repeat_final:
                        self.assertTrue(b.final_timeout_done)
                        self.assertEqual(b.requests[-1],b.requests[-2])
                    if faults.get('disconnect'):self.assertGreaterEqual(b.connections,2)
                    if busy_once:
                        self.assertTrue(b.busy_done)
                        self.assertGreaterEqual(b.request_times[1]-b.request_times[0],.1)
                    self.assertFalse(list((Path(td)/'Фото_с_камеры').glob('*.partial')))
                self.assertTrue(list((Path(td)/'Логи').glob('*_photo_server.log')))
                self.assertIsNone(server.error)
            finally:
                for b in boards:b.stop_event.set()
                server.stop_event.set();server.join(3)
                for b in boards:b.join(2)
            return messages

    def test_tcp_two_controllers(self):self.run_server({},count=2)
    def test_tcp_param_fallback_and_repeat_final(self):self.run_server({},param_once=True,repeat_final=True)
    def test_tcp_loss_repeat_delay_reset(self):self.run_server(dict(drop=3,repeat=True,delay=.002),reset=True)
    def test_tcp_reset_during_second_photo(self):self.run_server({},reset='second')
    def test_tcp_busy_waits_before_repeat(self):self.run_server({},busy_once=True)

    def test_restart_resumes_from_confirmed_offset(self):
        # Подтверждённое начало плата хранит во Flash и повторно не отдаёт.
        number=1789990001
        def packet(offset):
            return struct.pack('<BIIH',1,number,len(JPEG),offset)+JPEG[offset:offset+1024]
        with tempfile.TemporaryDirectory() as td:
            logs=[];emit=lambda kind,value:logs.append(value) if kind=='log' else None
            first=gui.Transfer('123456789012345',td,emit)
            for offset in range(0,3072,1024):self.assertEqual(first.receive(packet(offset)),'block')
            self.assertEqual(first.request,struct.pack('<IH',number,3072))
            # Перезапуск сервера после подтверждения 2048: хвост 2048–3072 придёт заново.
            second=gui.Transfer('123456789012345',td,emit)
            for offset in range(2048,len(JPEG),1024):self.assertEqual(second.receive(packet(offset)),'block')
            self.assertEqual((Path(td)/f'123456789012345_{number}.jpg').read_bytes(),JPEG)
            self.assertFalse((Path(td)/f'123456789012345_{number}.partial').exists())
            # Без сохранённого начала выгрузка останавливается, а не просит 0 по кругу.
            third=gui.Transfer('123456789012346',td,emit)
            self.assertEqual(third.receive(packet(2048)),'resume')
            self.assertEqual(third.request,struct.pack('<IH',number,0))
            third.new_stream()
            self.assertEqual(third.receive(packet(2048)),'abort')
            self.assertFalse(third.active)
            self.assertIn('подтверждены платой',logs[-1])

    def test_optional_pillow(self):
        original=gui.Image
        try:
            gui.Image=None;gui.valid_jpeg(JPEG)
            with self.assertRaises(ValueError):gui.valid_jpeg(JPEG[:-1])
        finally:gui.Image=original

    def test_tcp_disconnect_resume(self):self.run_server(dict(disconnect=True))

    def test_stop_and_update_mode(self):
        for status in (2048,8192):
            with tempfile.TemporaryDirectory() as td:
                events=queue.Queue();server=gui.Server(0,'127.0.0.1',td,events);server.start();self.assertTrue(server.ready.wait(3))
                board=Board(server.port,status=status);board.start()
                try:
                    key=None;deadline=time.monotonic()+3
                    while key is None and time.monotonic()<deadline:
                        kind,value=events.get(timeout=3)
                        if kind=='clients':
                            key=next((p['key'] for p in value if p['status']==status),None)
                    self.assertIsNotNone(key)
                    server.command(key,'get',{'delay':1})
                    if status==2048:server.command(key,'capture',{'address':17})
                    else:server.command(key,'stop')
                    time.sleep(1.2)
                    self.assertFalse(any(cmd in (7,21) for cmd,args in board.commands))
                finally:
                    board.stop_event.set();server.stop_event.set();server.join(2);board.join(2)

    def test_gui(self):
        # Программная проверка окна; физического визуального осмотра не имитирует.
        root=gui.tk.Tk();root.withdraw()
        with tempfile.TemporaryDirectory() as td:
            app=gui.App(root,td)
            app.open_image=lambda:None
            app.events.put(('clients',[dict(key='1',imei='123456789012345',ip='127.0.0.1',last='12:00',status=8192)]))
            app.events.put(('image',str(HERE/'sample_640x480.jpg')))
            app.pump();root.update();app.tree.selection_set('1');app.show_bits()
            self.assertIn('13=1',app.bits['text']);self.assertEqual(len(app.tree.get_children()),1)
            app.close()


def bridge():
    repo=HERE.parents[1]
    proc=subprocess.Popen([sys.executable,str(repo/'Tests/run_host.py'),'work','photo','bridge',str(HERE/'sample_640x480.jpg')],
                          stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,cwd=repo)
    cam=camera.C328(JPEG);done=False
    for line in proc.stdout:
        if line.startswith('TX '):
            replies=cam.feed(bytes.fromhex(line[3:]));proc.stdin.write(b''.join(replies).hex()+'\n');proc.stdin.flush()
        elif line.startswith('BRIDGE_OK'):
            print(line.strip());done=True
        else:print(line.rstrip())
    result=proc.wait(timeout=5)
    if result or not done:raise AssertionError(proc.stderr.read())
    print(f'Python C328 <-> production photo.c: {len(cam.requests)} commands, JPEG byte-for-byte OK')


if __name__=='__main__':
    if '--bridge' in sys.argv:bridge()
    else:unittest.main(verbosity=2)
