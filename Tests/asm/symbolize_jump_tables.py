"""Числовые таблицы переходов TBB/TBH (наследие экспорта листинга) -> выражения от меток.

Число в DC8/DC16 молча ломается, когда внутри функции добавляют или убирают инструкцию:
сборка проходит, переход попадает не туда. Скрипт ассемблирует файл с листингом IAR,
находит для каждого элемента таблицы строку-цель, ставит перед ней метку ??jt<n>_<i>
и пишет элемент как (метка-таблица)/2. Код при этом не меняется: проверка — ELF/BIN
после преобразования побайтно равны прежним.
Запуск: python Tests/asm/symbolize_jump_tables.py User/asm/<file>.s [...]"""
import pathlib, re, subprocess, sys, tempfile

IASM = r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\arm\bin\iasmarm.exe'
ROW = re.compile(r'^\s*(\d+)\s+([0-9A-F]{8})\s')       # строка исходника, смещение в секции


def listing(src):
    with tempfile.TemporaryDirectory() as d:
        r = subprocess.run([IASM, str(src), '-O' + d, '-L' + d, '-s+', '-M<>', '-w+',
                            '-DDOMOVOI_APP_HW=139', '--cpu', 'Cortex-M3', '--fpu', 'None'],
                           capture_output=True, text=True)
        assert r.returncode == 0, r.stdout + r.stderr
        return (pathlib.Path(d) / (src.stem + '.lst')).read_text(encoding='latin-1').splitlines()


def symbolize(path):
    raw = path.read_bytes().decode('ascii')
    crlf = '\r\n' in raw
    lines = raw.replace('\r\n', '\n').split('\n')
    lst = listing(path)
    # Смещение каждой строки исходника с кодом; секция меняется на директиве SECTION.
    offset, section, sec_of = {}, 0, {}
    for row in lst:
        m = ROW.match(row)
        if m:
            n = int(m.group(1)); offset.setdefault(n, int(m.group(2), 16))
    for i, text in enumerate(lines, 1):
        if text.strip().startswith('SECTION '):
            section += 1
        sec_of[i] = section
    by_offset = {}
    for n, off in offset.items():
        if not lines[n - 1].strip().startswith(('DC', 'DATA')):
            by_offset.setdefault((sec_of[n], off), n)
    labels, rewrites, tables = {}, {}, 0
    for i, text in enumerate(lines, 1):
        op = text.split()
        if not op or op[0] not in ('TBB', 'TBH'):
            continue
        half = op[0] == 'TBH'
        base = offset[i] + 4
        j = i + 1
        while not lines[j - 1].strip().endswith(':'):          # DATA, затем метка таблицы
            j += 1
        table = lines[j - 1].strip()[:-1]
        k, index = j + 1, 0
        while lines[k - 1].strip().startswith(('DC8', 'DC16')):
            kind, values = lines[k - 1].split(None, 1)
            if '-' in values:                                   # уже метки
                k += 1; continue
            exprs = []
            for v in values.split(','):
                v = int(v.strip(), 0)
                if v == 0:                                      # байт выравнивания TBB, не переход
                    exprs.append('0'); continue
                target = by_offset[(sec_of[i], base + 2 * v)]
                name = labels.setdefault(target, f'??jt{tables}_{index}')
                exprs.append(f'({name}-{table})/2')
                index += 1
            indent = lines[k - 1][:len(lines[k - 1]) - len(lines[k - 1].lstrip())]
            rewrites[k] = f'{indent}{kind}     ' + ','.join(exprs)
            k += 1
        tables += 1
    out = []
    for n, text in enumerate(lines, 1):
        if n in labels:
            out.append(labels[n] + ':')
        out.append(rewrites.get(n, text))
    text = '\n'.join(out)
    path.write_bytes((text.replace('\n', '\r\n') if crlf else text).encode('ascii'))
    return tables, len(labels)


if __name__ == '__main__':
    for name in sys.argv[1:]:
        t, l = symbolize(pathlib.Path(name))
        print(f'{name}: таблиц {t}, меток {l}')
