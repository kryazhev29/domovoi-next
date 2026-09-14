"""Domovoi Service desktop UI. No device queries until the operator requests one."""
import datetime as dt
import os
from pathlib import Path
import queue
import struct
import sys
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

from boot_tool import application, crc32
from desktop_transport import identify, transfer, STATUS, Cancelled, InstallUnconfirmed

VERSION = '1.0.0'
BG, INK, MUTED, BLUE = '#edf2f7', '#15283f', '#56677b', '#1967d2'


def explain(exc):
    text = str(exc)
    translations = {
        'Expected HW.FW.UNIX_TIMESTAMP.bin': 'Имя файла должно быть HW.FW.TIMESTAMP.bin, например 139.14.1726300000.bin.',
        'Metadata outside wire format limits': 'Недопустимые номера HW, FW или время сборки в имени файла.',
        'Application exceeds existing Flash region': 'Размер приложения должен быть от 8 до 97792 байт.',
        'Invalid vectors: application must be linked at 0x08008200':
            'Это не подходящий образ приложения: требуются векторы по 0x08008200. '
            'Образ загрузчика и зашифрованный пакет сюда не подходят.',
    }
    return translations.get(text, text)


class App:
    def __init__(self, root, demo=False):
        self.root, self.demo = root, demo
        root.tk.call('tk', 'scaling', min(float(root.tk.call('tk', 'scaling')), 1.4))
        self.busy = False
        self.committing = False
        self.image = None
        self.devices = {}
        self.events = queue.Queue()
        self.cancel = threading.Event()
        self.log_lines = []
        self.port = tk.StringVar()
        self.device_text = tk.StringVar(value='Контроллер пока не определён')
        self.file_text = tk.StringVar(value='Выберите открытый BIN приложения')
        self.metadata = tk.StringVar(value='Файл будет проверен до начала передачи')
        self.status = tk.StringVar(value='Начните с подключения контроллера')
        self.detail = tk.StringVar(value='Подключите USB, войдите в загрузчик и нажмите «Найти контроллер».')
        self.stage = tk.StringVar(value='01  Подключение     →     02  Проверка файла     →     03  Передача     →     04  Результат')
        root.title(f'Domovoi Service  ·  {VERSION}' + ('  ·  ДЕМО' if demo else ''))
        width, height = min(1080, root.winfo_screenwidth()-80), min(850, root.winfo_screenheight()-110)
        root.geometry(f'{width}x{height}+30+25')
        root.minsize(min(900, width), min(700, height))
        root.configure(bg=BG)
        root.protocol('WM_DELETE_WINDOW', self.close)
        style = ttk.Style(root)
        style.theme_use('clam')
        style.configure('.', font=('Segoe UI', 10))
        style.configure('TFrame', background=BG)
        style.configure('TNotebook', background=BG, borderwidth=0)
        style.configure('TNotebook.Tab', padding=(20, 10))
        style.configure('TButton', padding=(13, 9), background='#f5f8fc', foreground=INK, borderwidth=1)
        style.map('TButton', background=[('active', '#e3edfb')])
        style.map('TNotebook.Tab', background=[('selected', 'white')])
        style.configure('Primary.TButton', background=BLUE, foreground='white', font=('Segoe UI', 11, 'bold'))
        style.map('Primary.TButton', background=[('disabled', '#c5d0de'), ('active', '#1556b0')])
        style.configure('TProgressbar', background=BLUE, troughcolor='#dfe7f1', borderwidth=0)
        header = tk.Frame(root, bg=INK, padx=26, pady=18)
        header.pack(fill='x')
        tk.Label(header, text='DOMOVOI  /  SERVICE', bg=INK, fg='white', font=('Segoe UI', 21, 'bold')).pack(anchor='w')
        tk.Label(header, text='Обновление контроллера по USB  •  проверка совместимости  •  диагностика',
                 bg=INK, fg='#b9cce2', font=('Segoe UI', 10)).pack(anchor='w', pady=(5, 0))
        tabs = ttk.Notebook(root)
        tabs.pack(fill='both', expand=True, padx=20, pady=14)
        main_page = ttk.Frame(tabs)
        canvas = tk.Canvas(main_page, bg=BG, highlightthickness=0)
        main_scroll = ttk.Scrollbar(main_page, orient='vertical', command=canvas.yview)
        main_scroll.pack(side='right', fill='y')
        canvas.pack(side='left', fill='both', expand=True)
        canvas.configure(yscrollcommand=main_scroll.set)
        main = ttk.Frame(canvas)
        canvas_window = canvas.create_window((0, 0), window=main, anchor='nw')
        main.bind('<Configure>', lambda e: canvas.configure(scrollregion=canvas.bbox('all')))
        canvas.bind('<Configure>', lambda e: canvas.itemconfigure(canvas_window, width=e.width))
        def wheel(event):
            if str(tabs.select()) == str(main_page):
                canvas.yview_scroll(-int(event.delta/120), 'units')
        root.bind('<MouseWheel>', wheel)
        help_page = ttk.Frame(tabs)
        dev = ttk.Frame(tabs)
        tabs.add(main_page, text='Обновление')
        tabs.add(help_page, text='Подключение и помощь')
        tabs.add(dev, text='Для разработчика')
        card = self.card(main, '01  Контроллер')
        row = tk.Frame(card, bg='white'); row.pack(fill='x', pady=5)
        self.ports = ttk.Combobox(row, textvariable=self.port, width=14, state='readonly')
        self.ports.pack(side='left', padx=(0, 8))
        self.ports.bind('<<ComboboxSelected>>', lambda _: self.selected())
        self.refresh_button = ttk.Button(row, text='Обновить порты', command=self.refresh)
        self.refresh_button.pack(side='left', padx=4)
        self.scan_button = ttk.Button(row, text='Найти контроллер', command=self.scan)
        self.scan_button.pack(side='left', padx=4)
        self.check_button = ttk.Button(row, text='Проверить выбранный', command=self.check)
        self.check_button.pack(side='left', padx=4)
        self.label(card, self.device_text)
        card = self.card(main, '02  Прошивка приложения')
        self.pick_button = ttk.Button(card, text='Выбрать файл .bin', command=self.pick)
        self.pick_button.pack(anchor='w', pady=(3, 6))
        self.label(card, self.file_text, bold=True)
        self.label(card, self.metadata)
        card = self.card(main, '03  Обновление')
        self.label(card, self.status, bold=True)
        self.label(card, self.detail)
        self.progress = ttk.Progressbar(card, maximum=100)
        self.progress.pack(fill='x', pady=(10, 6))
        self.label(card, self.stage)
        actions = tk.Frame(card, bg='white'); actions.pack(fill='x', pady=(12, 2))
        self.update_button = ttk.Button(actions, text='Обновить контроллер', style='Primary.TButton', command=self.start_update)
        self.update_button.pack(side='left')
        self.stop_button = ttk.Button(actions, text='Остановить передачу', command=self.stop, state='disabled')
        self.stop_button.pack(side='left', padx=10)
        log_header = ttk.Frame(main); log_header.pack(fill='x', pady=(5, 4))
        ttk.Label(log_header, text='Журнал сеанса', background=BG, font=('Segoe UI', 10, 'bold')).pack(side='left')
        ttk.Button(log_header, text='Сохранить журнал', command=self.save_log).pack(side='right')
        log_frame = ttk.Frame(main); log_frame.pack(fill='both', expand=True)
        self.log = tk.Text(log_frame, height=5, font=('Consolas', 9), bg='#f8fafc', fg=INK,
                           relief='flat', wrap='word', state='disabled', padx=10, pady=8)
        scroll = ttk.Scrollbar(log_frame, command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right', fill='y'); self.log.pack(fill='both', expand=True)
        self.help_text(help_page, '''Подключение к контроллеру

1. Подключите питание контроллера и USB-кабель с линиями данных.
2. Войдите в загрузчик: удерживайте BOOT при включении питания.
   Для v5/MODEM это PA13, для MINI — PB1, замыкаемые на GND.
3. Нажмите «Найти контроллер». Утилита опросит перечисленные COM-порты.
   Если подключено другое оборудование, выберите порт и нажмите
   «Проверить выбранный», чтобы обращаться только к нему.
4. Выберите открытый файл HW.FW.TIMESTAMP.bin и нажмите «Обновить».

Что означает результат

«Образ принят» означает подтверждение приёма, а не успешного запуска.
Проверьте новую версию после регистрации контроллера на сервере.
Утилита не подключается к серверу и не обновляет через мобильную сеть.

Если что-то пошло не так

• Нет порта: проверьте кабель, питание и драйвер USB CDC в Windows.
• Порт занят: закройте терминал и другое сервисное ПО.
• HW не совпадает: получите файл именно для вашей аппаратной версии.
• Загрузчик старее 0x31: требуется переходная прошивка или запись через SWD.
• Ошибка 7: установленный MCU не подтверждает необходимые 128 КиБ Flash.
• Передача оборвалась: войдите в загрузчик и начните передачу заново.
  Продолжения после отключения кабеля нет; повторы отдельных блоков есть.
• Ответ на установку потерян: сначала проверьте фактическую версию приложения.

Особенности плат

MINI не мигает светодиодами: эти выводы совмещены с CAN.
Для v5/MODEM используется индикация PWR/DATA.
HW 139 сам по себе не различает v5 и старый MODEM.
Не отключайте питание после запроса установки. При замене самого
загрузчика потеря питания может потребовать восстановления через SWD.

О программе

Domovoi Service 1.0.0. Поддерживается протокол загрузчика 0x31 и выше
с текущим форматом ответа. На реальном контроллере эта версия утилиты
ещё не проверена. Журнал автоматически сохраняется в LOCALAPPDATA/
DomovoiService/logs; его также можно сохранить отдельной кнопкой.
''')
        self.help_text(dev, '''Подготовка файлов для старой системы

Выберите проверенный файл приложения на вкладке «Обновление», затем
создайте пакет для старого загрузчика кнопкой ниже. Пакет содержит
512-байтный заголовок и старое шифрование. Версия FW ограничена 255.

Эта операция только создаёт файл и ничего не отправляет на плату.
Для нового загрузчика используется исходный открытый BIN.

Переходная прошивка

Сначала старый конфигуратор устанавливает новую рабочую FW.
Затем загрузчик заменяется отдельным файлом через штатное сервисное ПО.
Файл выбирает пользователь по фотографии платы. Автозамены нет.
Полного BIN новой рабочей FW пока нет; требуется сборка IAR.

Инструменты разработчика

Команда boot_tool.py embed остаётся в комплекте для разработчика.
Прошивка области загрузчика по SWD из этого окна не выполняется.
''', height=21)
        self.legacy_button = ttk.Button(dev, text='Создать пакет для старого загрузчика…', command=self.legacy)
        self.legacy_button.pack(anchor='w', padx=20, pady=12)
        self.controls = [self.refresh_button, self.scan_button, self.check_button, self.pick_button, self.legacy_button]
        self.logpath = Path(os.environ.get('LOCALAPPDATA', str(Path.home()))) / 'DomovoiService/logs' / (dt.datetime.now().strftime('%Y%m%d-%H%M%S')+'.log')
        self.append(f'Domovoi Service {VERSION}. ' + ('ДЕМО: обращений к портам нет.' if demo else 'Порты ещё не опрашивались.'))
        self.refresh()
        self.ready()
        root.after(80, self.drain)

    def card(self, parent, title):
        frame = tk.Frame(parent, bg='white', padx=18, pady=10)
        frame.pack(fill='x', pady=(0, 8))
        tk.Label(frame, text=title, bg='white', fg=MUTED, font=('Segoe UI', 10, 'bold')).pack(anchor='w', pady=(0, 4))
        return frame

    def label(self, parent, variable, bold=False):
        tk.Label(parent, textvariable=variable, bg='white', fg=INK if bold else MUTED,
                 font=('Segoe UI', 10, 'bold' if bold else 'normal'), anchor='w', justify='left',
                 wraplength=820).pack(anchor='w', fill='x', pady=2)

    def help_text(self, parent, text, height=30):
        box = tk.Text(parent, bg='white', fg=INK, font=('Segoe UI', 11), wrap='word',
                      relief='flat', padx=22, pady=18, height=height)
        box.insert('1.0', text); box.configure(state='disabled')
        scrollbar = ttk.Scrollbar(parent, command=box.yview)
        box.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side='right', fill='y'); box.pack(fill='both', expand=True)

    def append(self, text):
        line = dt.datetime.now().strftime('%H:%M:%S')+'  '+text
        self.log_lines.append(line)
        self.log.configure(state='normal'); self.log.insert('end', line+'\n'); self.log.see('end'); self.log.configure(state='disabled')
        try:
            self.logpath.parent.mkdir(parents=True, exist_ok=True)
            with self.logpath.open('a', encoding='utf-8') as file:
                file.write(line+'\n')
        except OSError:
            pass

    def ready(self):
        info = self.devices.get(self.port.get())
        ok = self.image and info and info['hw'] == self.image[1][0] and info['version'] >= 0x31 and info['status'] == 0
        self.update_button.configure(state='normal' if ok and not self.busy and not self.demo else 'disabled')
        if not self.busy and info:
            if info['status']:
                self.detail.set(STATUS.get(info['status'], f"Ошибка {info['status']}"))
            elif info['version'] < 0x31:
                self.detail.set('Требуется загрузчик от 0x31. Для старого загрузчика нужен отдельный переходный пакет.')
            elif self.image and info['hw'] != self.image[1][0]:
                self.detail.set(f"Обновление заблокировано: HW контроллера {info['hw']}, HW файла {self.image[1][0]}.")

    def refresh(self):
        if self.demo:
            ports = ['COM5']
        else:
            from serial.tools.list_ports import comports
            ports = [p.device for p in comports()]
        self.ports['values'] = ports
        if self.port.get() not in ports:
            self.port.set(ports[0] if ports else '')
        self.selected()

    def selected(self):
        info = self.devices.get(self.port.get())
        if info:
            error = STATUS.get(info['error'], str(info['error'])) if info['error'] else 'нет'
            self.device_text.set(f"{info['port']}  •  HW {info['hw']}  •  загрузчик 0x{info['version']:02X}  •  сохранённая ошибка: {error}")
        else:
            self.device_text.set('Порт выбран, контроллер не проверен' if self.port.get() else 'COM-порты не найдены. Проверьте кабель и драйвер.')
        self.ready()

    def run(self, worker):
        if self.busy:
            return
        self.busy = True; self.cancel.clear(); self.committing = False
        for button in self.controls: button.configure(state='disabled')
        self.ports.configure(state='disabled'); self.ready()
        def task():
            try: worker()
            except Cancelled as exc: self.events.put(('cancelled', str(exc)))
            except InstallUnconfirmed as exc: self.events.put(('uncertain', str(exc)))
            except Exception as exc: self.events.put(('error', explain(exc)))
            finally: self.events.put(('done', None))
        threading.Thread(target=task, daemon=True).start()

    def scan(self):
        if self.demo: return
        ports = list(self.ports['values'])
        self.devices.clear(); self.selected()
        self.status.set('Поиск контроллера'); self.detail.set('Проверяем COM-порты. Прошивка не записывается.')
        def worker():
            found = 0
            for port in ports:
                if self.cancel.is_set(): break
                try:
                    info = identify(port, cancel=self.cancel)
                    self.events.put(('device', info)); found += 1
                except Exception as exc: self.events.put(('log', f'{port}: {explain(exc)}'))
            self.events.put(('stage', f'Найдено контроллеров: {found}'))
        self.run(worker)
        self.stop_button.configure(state='normal', text='Остановить поиск')

    def check(self):
        if self.demo or not self.port.get(): return
        port = self.port.get(); self.devices.pop(port, None); self.selected()
        self.run(lambda: self.events.put(('device', identify(port, cancel=self.cancel))))

    def pick(self):
        path = filedialog.askopenfilename(title='Открытый BIN приложения', filetypes=[('Приложение', '*.bin'), ('Все файлы', '*.*')])
        if path: self.load_image(Path(path))

    def load_image(self, path):
        self.image = None; self.ready()
        try:
            info = application(path)
            hw, fw, stamp, data = info
            self.image = (path, info)
            self.file_text.set(path.name)
            date = dt.datetime.fromtimestamp(stamp, dt.timezone.utc).strftime('%d.%m.%Y %H:%M UTC')
            self.metadata.set(f'HW {hw}   •   FW {fw}   •   {date}\n{len(data):,} байт   •   CRC32 {crc32(data):08X}   •   адрес 0x08008200')
            self.status.set('Файл проверен'); self.detail.set('Для обновления нужен контроллер с совпадающим HW и загрузчиком от 0x31.')
            self.append(f'Файл: {path}; HW={hw}, FW={fw}, размер={len(data)}, CRC={crc32(data):08X}')
        except Exception as exc:
            self.file_text.set('Файл не подходит'); self.metadata.set(explain(exc)); self.append(explain(exc))
        self.ready()

    def start_update(self):
        if self.demo or self.busy or not self.image: return
        port, path = self.port.get(), self.image[0]
        info = self.devices.get(port)
        if not info or info['hw'] != self.image[1][0] or info['version'] < 0x31 or info['status']:
            return
        # The worker reads the file again and re-identifies the actual device.
        self.progress['value'] = 0
        self.status.set('Проверка контроллера и подготовка передачи')
        snapshot = self.image[1]
        self.run(lambda: transfer(port, path, lambda k, v: self.events.put((k, v)), self.cancel, expected_image=snapshot))
        self.stop_button.configure(state='normal', text='Остановить передачу')

    def stop(self):
        if not self.committing:
            self.cancel.set(); self.stop_button.configure(state='disabled')
            self.detail.set('Останавливаем передачу до запроса установки…')

    def drain(self):
        try:
            while True:
                kind, value = self.events.get_nowait()
                if kind == 'device':
                    self.devices[value['port']] = value; self.port.set(value['port']); self.selected()
                    self.status.set('Контроллер определён')
                    self.detail.set(STATUS.get(value['status'], 'Выберите совместимый файл приложения.'))
                    self.append(self.device_text.get())
                elif kind == 'progress':
                    count, total, elapsed = value
                    self.progress['value'] = count * 100 / total
                    rate = count / max(elapsed, .01)
                    self.status.set(f'Передача прошивки  ·  {count*100/total:.0f}%')
                    self.stage.set('Подключение ✓     →     Файл проверен ✓     →     Передача…     →     Результат')
                    self.detail.set(f'{count:,} / {total:,} байт   •   {rate/1024:.1f} КиБ/с   •   осталось ≈ {(total-count)/rate:.0f} с')
                elif kind == 'commit':
                    self.committing = True; self.stop_button.configure(state='disabled')
                    self.status.set(value); self.detail.set('Запрос установки отправляется один раз. Дождитесь ответа.'); self.append(value)
                    self.stage.set('Подключение ✓     →     Файл проверен ✓     →     Передано ✓     →     Ожидание ответа…')
                elif kind in ('error', 'uncertain', 'cancelled', 'accepted'):
                    titles = {'error':'Операция не завершена', 'uncertain':'Результат установки неизвестен',
                              'cancelled':'Передача остановлена', 'accepted':'Образ принят — запуск не подтверждён'}
                    self.status.set(titles[kind]); self.detail.set(value); self.append(value)
                    self.stage.set('Результат: '+titles[kind])
                    if kind in ('accepted', 'uncertain', 'error'):
                        self.devices.pop(self.port.get(), None)
                        self.device_text.set('Для следующей операции заново определите контроллер')
                elif kind == 'stage': self.status.set(value); self.append(value)
                elif kind == 'log': self.append(value)
                elif kind == 'done':
                    self.busy = False; self.committing = False
                    for button in self.controls: button.configure(state='normal')
                    self.ports.configure(state='readonly'); self.stop_button.configure(state='disabled'); self.ready()
        except queue.Empty: pass
        self.root.after(80, self.drain)

    def save_log(self):
        target = filedialog.asksaveasfilename(defaultextension='.log', initialfile=self.logpath.name,
                                            filetypes=[('Журнал', '*.log')])
        if target:
            try: Path(target).write_text('\n'.join(self.log_lines)+'\n', encoding='utf-8-sig')
            except OSError as exc: messagebox.showerror('Журнал не сохранён', str(exc))

    def legacy(self):
        if not self.image:
            messagebox.showinfo('Выберите приложение', 'Сначала выберите BIN на вкладке «Обновление».'); return
        target = filedialog.asksaveasfilename(defaultextension='.legacy', initialfile=self.image[0].stem+'.legacy', filetypes=[('Старый пакет', '*.legacy')])
        if not target: return
        try:
            if Path(target).resolve() == self.image[0].resolve(): raise ValueError('Нельзя перезаписывать исходный BIN.')
            from boot_tool import legacy_package
            legacy_package(self.image[0], Path(target))
            self.append('Создан пакет для старого загрузчика: '+target)
            messagebox.showinfo('Пакет создан', 'Файл сохранён. Контроллер не прошивался.')
        except Exception as exc: messagebox.showerror('Пакет не создан', explain(exc))

    def close(self):
        if self.busy:
            messagebox.showinfo('Операция выполняется', 'Дождитесь завершения операции. До запроса установки можно остановить передачу.'); return
        self.root.destroy()


if __name__ == '__main__':
    if sys.platform == 'win32':
        import ctypes
        try: ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception: pass
    window = tk.Tk()
    app = App(window, demo='--demo' in sys.argv)
    if '--smoke-test' in sys.argv:
        window.after(700, window.destroy)
    window.mainloop()
