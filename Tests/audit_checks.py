"""Enumerate guards and verification operations; fail on unaudited new sites.

This is a conservative inventory, not a proof that guards may be removed.
Source predicates are kept verbatim. The only deletion decision is recorded
separately with its caller invariant and linked mutation test.
"""
import argparse
from collections import Counter
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    'User/c/user_fun_runtime.c', 'User/c/table_runtime.inc',
    'User/c/photo_runtime.c', 'User/c/photo_runtime.inc',
    'User/c/memory_runtime.c', 'User/c/fw_service_runtime.c',
    'User/c/service_runtime.c', 'User/c/service_ff.inc', 'User/c/upload_runtime.c',
    'Modules/comm_at.c', 'Modules/comm_modem.c', 'Modules/comm_channel.c',
    'Modules/comm_sms.c',
)


def mask(source):
    def blank(match):
        return ''.join('\n' if c == '\n' else ' ' for c in match[0])
    return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  blank, source, flags=re.S)


def closing(text, position, left='(', right=')'):
    depth = 1
    i = position + 1
    while depth:
        depth += (text[i] == left) - (text[i] == right)
        i += 1
    return i


def inventory():
    result = []
    for filename in FILES:
        source = (ROOT / filename).read_bytes().decode('cp1251')
        text = mask(source)
        functions = []
        pattern = r'(?m)^[A-Za-z_][\w\s*]*?\b(\w+)\s*\([^;{}]*\)\s*\{'
        for match in re.finditer(pattern, text):
            start = text.index('{', match.start())
            functions.append((match.start(), closing(text, start, '{', '}'), match[1]))

        def add(start, end, kind):
            function = next((n for a, b, n in functions if a <= start < b), 'file scope')
            predicate = ' '.join(source[start:end].split())
            line = source.count('\n', 0, start) + 1
            last = source.count('\n', 0, end) + 1
            value = predicate.lower()
            if kind == 'retained duplicate':
                label, reason = 'дубль', ('Только !bytes: все четыре вызова из user_sync требуют ptr[9]!=0; '
                    'User/c/user_fun_runtime.c:3679,3695,3711 — sync_cells_valid для 13/14/15; '
                    ':3729 — явная проверка для 16. Размеры 32/32/16/4, '
                    'uint8_t count: произведение ненулевое и не переполняет uint16_t. '
                    '16 CRC-valid zero-count входов и три bypass-мутации sync_cells_valid: '
                    'Tests/asm/test_stage3_entry_guards.py. Проверка сохранена: оба шага −8 Б '
                    'отменены после красных host-проверок; см. отчёт.')
            elif kind == 'ABI assert':
                label, reason = 'защита', 'ABI, размер объекта или смещение поля C/ASM'
            elif kind == 'read/compare/CRC':
                if re.search(r'crc|read|memcmp|confirm', value):
                    label, reason = 'защита', 'данные NOR/пакета либо сохранённый курсор; другой способ проверки не гарантирует тот же факт'
                else:
                    label, reason = 'нужна', 'результат операции используется последующим кодом'
            elif filename.startswith('Modules/') and re.search(r'number|scan|parse|prefix|take|frame|data|count|limit|pos|code|token|capacity|command|reason|profile|side|req|urc', value):
                label, reason = 'защита', 'границы входа модема/SMS, целостность потока или владение транспортом'
            elif re.search(r'\b(len|size|bytes|index|count|crc|address|adr|end|offset|capacity|ptr|data|cell)\b|mem_|w25_|sj_|flash_|work_|alloc|epoch|fault|reboot|trial|timeout|available|busy|active|pending|primask|cancel|!p\b|!photo\b', value):
                label, reason = 'защита', 'граница/целостность данных, RAM/NOR, состояние владельца, отмена, таймаут или гонка IRQ'
            else:
                label, reason = 'нужна', 'выбирает действие автомата, ответ/журнал, ограничение порции либо изменение состояния'
            result.append({'file': filename, 'line': line, 'last': last, 'function': function,
                           'kind': kind, 'predicate': predicate, 'label': label, 'reason': reason})

        # A superset: ordinary branches and bounded loops are included too.
        for match in re.finditer(r'\b(if|while|for|switch)\s*\(', text):
            start = text.index('(', match.start())
            add(match.start(), closing(text, start), match[1])
        for match in re.finditer(r'\b(?:RT_ASSERT|assert|memcmp|strcmp|strncmp|crc16_(?:calc(?:Init)?|ffInit)|crc32_update|CRC32|CRC_CalcBlockCRC|w25_read_data|mem_read|photo_confirmed_offset)\s*\(', text):
            start = text.index('(', match.start())
            add(match.start(), closing(text, start), 'ABI assert' if match[0].strip().startswith(('RT_ASSERT', 'assert')) else 'read/compare/CRC')
        for match in re.finditer(r'\breturn\s+([^;{}]+);', text):
            if re.search(r'\?|&&|\|\||==|!=|[<>]|!\w', match[1]):
                add(match.start(), match.end(), 'conditional return')
        for match in re.finditer(r'\?', text):
            start = max(text.rfind('\n',0,match.start()),text.rfind(';',0,match.start())) + 1
            end = text.find(';',match.end())
            if end >= 0:
                add(start, end + 1, 'conditional expression')
        for match in re.finditer(r'\bdefault\s*:', text):
            add(match.start(), match.end(), 'default state')
        if filename == 'User/c/table_runtime.inc':
            start = text.index('!bytes')
            add(start, start + len('!bytes'), 'retained duplicate')
    return sorted(result, key=lambda x: (FILES.index(x['file']), x['line'], x['kind']))


def document(rows):
    counts = Counter(r['label'] for r in rows)
    lines = [
        '# Опись проверок — этап 3', '',
        'База `46be7f9`, `compare/c-asm`. Инвентаризация относится к текущим строкам исходников; воспроизводится `python Tests/audit_checks.py --check`.', '',
        'Опись намеренно шире проверок отказа: включены все `if`, `for/while`, `switch/default`, условные выражения и возвраты, ABI-assert, чтения NOR, сравнения памяти/строк и расчёты CRC. Повторная строка означает отдельную операцию внутри условия, а не вторую проверку в прошивке. Декларации CRC тоже видны в этой описи исходника; это не подсчёт вызовов или байтов. Сохранены исходное условие и функция: это позволяет проверить классификацию без пересказа большого файла.', '',
        'Метки консервативные: совпадение текста или CRC не доказывает дубль. Все строки «защита» и «нужна» сохранены. Ввод проходит проверки на границе своего слоя: UART/кадр/CRC, затем аргументы команды; это разные факты. CRC NOR не заменяет проверку результата записи, valid-header не заменяет чтение после обрыва питания. Граница после await/poll, USB epoch и Flash ownership проверяются заново, поскольку состояние может измениться.', '',
        'Мёртвые ветки ASM_RUNTIME удалены отдельно, без удаления исполняемых проверок: 50 препроцессированных IAR translation units дали одинаковые токены до/после во всех шести Release/ReleaseCustomer проектах. Для ASM использованы фактические карты и перечни команд сборки; C-зеркала не являются его входами.', '',
        'Кольцо `comm_modem_diag()->failure_step/failure_reason/failure_next` сохранено: `Modules/comm_recover.c:rc_early` использует его для восстановления. `fault_capture`, `fault_record`, stack paint/canary и остановка до IWDG сохранены в обеих сборках.', '',
        'Кандидаты, оставленные: `service_crc_poll` предварительно проверяет занятый TX, а `comm_tx_acquire` проверяет его повторно. Предварительная проверка экономит вызовы в занятом канале; проверка acquire защищает владение. Повторные `mem_tables_ready` защищают Flash/NOR, поэтому не удалены. `cfg_equal_flash` сверяет именно зеркальную копию, CRC — валидность выбранного слота: это разные гарантии.', '',
        'Исполняемые проверки не удалены. Единственный доказанный кандидат «дубль» — часть `!bytes` в `table_runtime_begin`; он сохранён после двух отменённых шагов по правилу красной проверки. Остальные части того же условия защищают владение/границы Flash и остались. Ни «недостижимо», ни «повтор» не присвоены без достаточного доказательства. Отключение диагностического кода описано отдельно в разделе «Этап 3» отчёта.', '',
        f'Всего записей: {len(rows)}; ' + ', '.join(f'{k}: {v}' for k, v in sorted(counts.items())) + '.', '',
    ]
    for filename in FILES:
        subset = [r for r in rows if r['file'] == filename]
        lines += ['## ' + filename, '', '| Место / функция | Что проверяет | Метка | Основание |',
                  '|---|---|---|---|']
        for row in subset:
            predicate = row['predicate'].replace('|', '\\|').replace('`', "'")
            lines.append(f"| `{filename}:{row['line']}` / `{row['function']}` | `{predicate}` | **{row['label']}** | {row['reason']} |")
        lines.append('')
    return '\n'.join(lines)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    content = document(inventory())
    path = ROOT / 'docs/checks_audit.md'
    if args.check:
        assert path.read_text(encoding='utf-8') == content, 'audit is stale'
    else:
        path.write_text(content, encoding='utf-8')
    print('PASS checks audit', len(inventory()), 'sites')
