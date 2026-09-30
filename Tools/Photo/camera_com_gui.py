"""Оконная программа прямого опроса C328 через USB–UART, без контроллера."""
import ctypes
import io
import logging
from logging.handlers import RotatingFileHandler
import math
import os
from pathlib import Path
import queue
import re
import sys
import threading
from datetime import datetime
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from camera_com import BAUD_RATES, C328Client, CameraError, Cancelled, DemoPort, list_ports
from camera_emulator import WindowsSerial

try:
    from PIL import Image, ImageTk
except ImportError:
    Image = ImageTk = None

HERE = Path(__file__).resolve().parent
DEMO = 'Демо (без COM)'


def default_output():
    # Упакованный exe хранит результаты рядом с собой, исходник — на стенде.
    if getattr(sys, 'frozen', False):
        return Path(sys.executable).resolve().parent / 'Фото_камеры_COM'
    for parent in HERE.parents:
        stand = parent / 'Стенд_Domovoi'
        if stand.is_dir():
            return stand / 'Фото_камеры_COM'
    return HERE / 'Фото_камеры_COM'


def save_jpeg(data, directory, prefix):
    if Image is not None:
        with Image.open(io.BytesIO(data)) as picture:
            picture.load()
            if picture.format != 'JPEG':
                raise CameraError('Ответ камеры не является JPEG')
    path = directory / f'{prefix}_{datetime.now():%Y-%m-%d_%H-%M-%S_%f}.jpg'
    # Файл появляется с расширением .jpg только после завершения записи.
    temporary = path.with_suffix('.part')
    try:
        with temporary.open('xb') as stream:
            stream.write(data)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    return path


class CameraApp:
    def __init__(self, root, output=None, transport_factory=None):
        self.root = root
        self.transport_factory = transport_factory or WindowsSerial
        self.events = queue.Queue()
        self.stop_event = threading.Event()
        self.worker = None
        self.closing = False
        self.last_file = None
        self.preview_image = None
        self.port = tk.StringVar()
        self.baud = tk.StringVar(value='115200')
        self.interval = tk.StringVar(value='5')
        self.output = tk.StringVar(value=str(output or default_output()))
        self.status = tk.StringVar(value='Выберите COM-порт адаптера и нажмите «Проверить связь» или «Снимок».')
        self.counter = tk.StringVar(value='Снимков: 0')
        self.count = 0
        root.title('Камера COM — ДОМОВОЙ NEXT / C328')
        root.geometry('1120x800')
        root.minsize(920, 660)
        style = ttk.Style(root)
        if 'vista' in style.theme_names():
            style.theme_use('vista')
        root.columnconfigure(0, weight=1)
        root.rowconfigure(3, weight=1)
        settings = ttk.LabelFrame(root, text='Прямое подключение камеры • C328 / OV528 • 8N1', padding=12)
        settings.grid(row=0, column=0, sticky='ew', padx=12, pady=(12, 6))
        settings.columnconfigure(1, weight=1)
        ttk.Label(settings, text='COM-порт').grid(row=0, column=0, sticky='w')
        self.port_box = ttk.Combobox(settings, textvariable=self.port, width=22)
        self.port_box.grid(row=0, column=1, sticky='ew', padx=8)
        self.refresh_button = ttk.Button(settings, text='Обновить', command=self.refresh_ports)
        self.refresh_button.grid(row=0, column=2)
        ttk.Label(settings, text='Скорость').grid(row=0, column=3, padx=(18, 6))
        self.baud_box = ttk.Combobox(settings, textvariable=self.baud, values=BAUD_RATES, state='readonly', width=10)
        self.baud_box.grid(row=0, column=4)
        ttk.Label(settings, text='Пауза опроса, с').grid(row=0, column=5, padx=(18, 6))
        self.interval_box = ttk.Entry(settings, textvariable=self.interval, width=7)
        self.interval_box.grid(row=0, column=6)
        ttk.Label(settings, text='Папка снимков').grid(row=1, column=0, pady=(12, 0))
        self.output_box = ttk.Entry(settings, textvariable=self.output)
        self.output_box.grid(row=1, column=1, columnspan=5, sticky='ew', padx=8, pady=(12, 0))
        self.folder_button = ttk.Button(settings, text='Выбрать…', command=self.choose_folder)
        self.folder_button.grid(row=1, column=6, pady=(12, 0))
        buttons = ttk.Frame(root)
        buttons.grid(row=1, column=0, sticky='ew', padx=12, pady=6)
        self.start_buttons = []
        for label, mode in [('Проверить связь', 'sync'), ('Снимок', 'single'), ('Начать опрос', 'cycle')]:
            button = ttk.Button(buttons, text=label, command=lambda m=mode: self.start(m))
            button.pack(side='left', padx=(0, 8))
            self.start_buttons.append(button)
        self.stop_button = ttk.Button(buttons, text='Стоп', state='disabled', command=self.stop)
        self.stop_button.pack(side='left')
        ttk.Button(buttons, text='Открыть папку', command=self.open_folder).pack(side='right')
        ttk.Label(buttons, textvariable=self.counter).pack(side='right', padx=15)
        info = ttk.Frame(root)
        info.grid(row=2, column=0, sticky='ew', padx=12, pady=6)
        self.status_label = ttk.Label(info, textvariable=self.status, wraplength=1050)
        self.status_label.pack(anchor='w')
        self.progress = ttk.Progressbar(info, maximum=100)
        self.progress.pack(fill='x', pady=(6, 0))
        pane = ttk.Panedwindow(root, orient='vertical')
        pane.grid(row=3, column=0, sticky='nsew', padx=12, pady=6)
        preview = ttk.LabelFrame(pane, text='Последний снимок', padding=8)
        self.preview = ttk.Label(preview, text='JPEG 640×480 появится здесь после съёмки.', anchor='center')
        self.preview.pack(fill='both', expand=True)
        self.filename = ttk.Label(preview, text='', anchor='center', wraplength=1000)
        self.filename.pack(fill='x')
        pane.add(preview, weight=3)
        journal = ttk.LabelFrame(pane, text='Журнал обмена — TX запрос / RX ответ', padding=5)
        self.log = tk.Text(journal, height=9, wrap='word', font=('Consolas', 9), state='disabled')
        scroll = ttk.Scrollbar(journal, command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right', fill='y')
        self.log.pack(fill='both', expand=True)
        self.log.tag_configure('tx', foreground='#155da0')
        self.log.tag_configure('rx', foreground='#227544')
        self.log.tag_configure('error', foreground='#b42318')
        pane.add(journal, weight=1)
        ttk.Label(root, text='TX адаптера → RX камеры; RX → TX; GND → GND. Питание камеры — по её паспорту.\n'
                  'Пауза отсчитывается после снимка. «Демо» проверяет программу без COM-порта.',
                  wraplength=1050).grid(row=4, column=0, sticky='w', padx=12, pady=(2, 12))
        self.editable = [self.port_box, self.refresh_button, self.baud_box,
                         self.interval_box, self.output_box, self.folder_button]
        self.refresh_ports()
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.after_id = root.after(40, self.process_events)

    def refresh_ports(self):
        try:
            self.port_box['values'] = list_ports() + [DEMO]
        except OSError as error:
            self.status.set(f'Не удалось прочитать список портов: {error}. Можно ввести COM вручную.')

    def choose_folder(self):
        value = filedialog.askdirectory(parent=self.root, initialdir=self.output.get())
        if value:
            self.output.set(value)

    def open_folder(self):
        try:
            path = Path(self.output.get()).expanduser()
            path.mkdir(parents=True, exist_ok=True)
            os.startfile(path)
        except OSError as error:
            messagebox.showerror('Папка снимков', str(error), parent=self.root)

    def set_busy(self, busy):
        for widget in self.editable + self.start_buttons:
            widget.configure(state='disabled' if busy else 'normal')
        if not busy:
            self.baud_box.configure(state='readonly')
        self.stop_button.configure(state='normal' if busy else 'disabled')

    def start(self, mode):
        if self.worker is not None and self.worker.is_alive():
            return
        port = self.port.get().strip()
        try:
            if port != DEMO and not re.fullmatch(r'COM[1-9][0-9]*', port, re.IGNORECASE):
                raise ValueError('Выберите COM-порт USB–UART, например COM8, или «Демо (без COM)».')
            baud = int(self.baud.get())
            if baud not in BAUD_RATES:
                raise ValueError('Выберите скорость из списка.')
            interval = float(self.interval.get().replace(',', '.')) if mode == 'cycle' else 5
            if not math.isfinite(interval) or not 0.1 <= interval <= 86400:
                raise ValueError('Пауза опроса должна быть от 0,1 до 86400 секунд.')
            if not self.output.get().strip():
                raise ValueError('Выберите папку для снимков и журнала.')
            output = Path(self.output.get()).expanduser().resolve()
            output.mkdir(parents=True, exist_ok=True)
        except (ValueError, OSError) as error:
            messagebox.showerror('Настройки', str(error), parent=self.root)
            return
        self.stop_event.clear()
        self.progress['value'] = 0
        self.set_busy(True)
        self.status.set(f'Открытие {port}…')
        self.worker = threading.Thread(target=self.run, args=(mode, port, baud, interval, output), daemon=True)
        self.worker.start()

    def run(self, mode, name, baud, interval, output):
        port = None
        handler = None
        logger = logging.Logger(f'camera-com-{id(self)}')

        def emit(kind, value):
            if kind != 'progress':
                line = value.hex(' ').upper() if isinstance(value, bytes) else str(value)
                logger.info('%s %s', kind.upper(), line)
            self.events.put((kind, value))

        try:
            handler = RotatingFileHandler(output / 'camera_com.log', maxBytes=3_000_000,
                                          backupCount=3, encoding='utf-8')
            handler.setFormatter(logging.Formatter('%(asctime)s %(message)s'))
            logger.addHandler(handler)
            emit('info', f'{name}, {baud} бод, 8N1. Режим: {mode}')
            if name == DEMO:
                port = DemoPort((HERE / 'sample_640x480.jpg').read_bytes())
            else:
                port = self.transport_factory(name.upper(), baud)
            client = C328Client(port, self.stop_event, emit)
            while True:
                if mode == 'sync':
                    client.synchronize()
                    break
                self.events.put(('progress', (0, 1)))
                data = client.capture()
                if self.stop_event.is_set():
                    raise Cancelled('Остановлено пользователем')
                path = save_jpeg(data, output, 'DEMO' if name == DEMO else name.upper())
                emit('photo', path)
                if mode != 'cycle':
                    emit('stage', f'Готово: {len(data)} байт. COM-порт освобождён.')
                    break
                emit('stage', f'Снимок сохранён. Следующий опрос через {interval:g} с…')
                if self.stop_event.wait(interval):
                    raise Cancelled('Остановлено пользователем')
        except Cancelled as error:
            emit('stage', str(error))
        except Exception as error:
            emit('error', f'{error}. Опрос остановлен.')
        finally:
            if port is not None:
                try:
                    port.close()
                except OSError as error:
                    emit('error', f'Закрытие порта: {error}')
            if handler is not None:
                handler.close()
                logger.removeHandler(handler)
            self.events.put(('done', None))

    def stop(self):
        self.stop_event.set()
        self.status.set('Остановка…')
        self.stop_button.configure(state='disabled')

    def append_log(self, kind, value):
        line = value.hex(' ').upper() if isinstance(value, bytes) else str(value)
        self.log.configure(state='normal')
        self.log.insert('end', f'{datetime.now():%H:%M:%S.%f}'[:-3] + f' {kind.upper():5} {line}\n', kind)
        if int(self.log.index('end-1c').split('.')[0]) > 1800:
            self.log.delete('1.0', '301.0')
        self.log.see('end')
        self.log.configure(state='disabled')

    def show_photo(self, path):
        self.last_file = path
        self.count += 1
        self.counter.set(f'Снимков: {self.count}')
        self.filename.configure(text=str(path))
        if Image is None:
            self.preview.configure(text='JPEG сохранён. Для просмотра откройте папку снимков.\n'
                                   'В исходниках предпросмотр требует Pillow.')
            return
        try:
            with Image.open(path) as picture:
                picture.thumbnail((900, 390), Image.Resampling.LANCZOS)
                self.preview_image = ImageTk.PhotoImage(picture.copy(), master=self.root)
            self.preview.configure(image=self.preview_image, text='')
        except (OSError, ValueError) as error:
            self.status.set(f'Файл сохранён, ошибка предпросмотра: {error}')

    def process_events(self):
        for _ in range(180):
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == 'progress':
                received, total = value
                self.progress['value'] = received * 100 / total
            elif kind == 'done':
                self.set_busy(False)
            else:
                self.append_log(kind, value)
                if kind in ('stage', 'error'):
                    self.status.set(value)
                elif kind == 'photo':
                    self.show_photo(value)
        if self.closing and (self.worker is None or not self.worker.is_alive()):
            self.root.destroy()
            return
        self.after_id = self.root.after(40, self.process_events)

    def close(self):
        self.closing = True
        self.stop()


def main():
    if os.name == 'nt':
        try:
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except (AttributeError, OSError):
            pass
    root = tk.Tk()
    CameraApp(root)
    root.mainloop()


if __name__ == '__main__':
    main()
