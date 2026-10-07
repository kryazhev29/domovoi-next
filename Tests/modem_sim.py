"""Доработка: Кряжев А., 2026. Имитатор: только записи обмена UART, без USB/отладки."""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / 'Tests/fixtures/modem'
LOG_NAMES = (
    '2026-10-01_плата1_SLM336_журнал_модема_без_SIM.txt',
    '2026-10-01_плата3_M66_журнал_модема_отказ_сервера.txt',
)


def extract(path):
    raw = path.read_bytes()
    text = raw.decode('cp1251').replace('\r\r\n', '\n').replace('\r\n', '\n').replace('\r', '\n')
    # Префикс USB до первой метки GSM не является ответом модема.
    pattern = re.compile(r'>>> GSM: Cmd ([^\n]*)|(?:<<< GSM: Answer |>>> GSM: Recv )([\s\S]*?)(?:-----)', re.M)
    events, omitted = [], []
    last_answer = None
    duplicates = 0
    for match in pattern.finditer(text):
        if match[1] is not None:
            last_answer = None
            command = match[1].strip()
            if command.startswith('AT'):
                events.append({'tx': command})
            else:
                omitted.append('Отладочная запись отправки двоичного кадра без байтов')
            continue
        answer = match[0].startswith('<<<')
        payload = match[2]
        # В журнале prompt и разделитель стоят в одной строке.
        if payload.strip() == '>':
            payload = '> '
        else:
            payload = payload.replace('\n', '\r\n')
        if 'IPD16:' in payload:
            payload = payload[:payload.index('IPD16:')]
            omitted.append('Обрезанное тело direct push +RECEIVE 16: IPD16/SLDN — отладочное представление, не UART-байты')
        if not answer and last_answer == payload:
            duplicates += 1
            last_answer = None
            continue
        last_answer = payload if answer else None
        if payload:
            assert '\x00' not in payload and 'GSM:' not in payload and 'SLDN' not in payload
            events.append({'rx': payload})
    return {'origin': 'capture', 'source': path.name, 'sha256': hashlib.sha256(raw).hexdigest(),
            'normalization': 'cp1251; CRCRLF журнала → CRLF; Cmd/Answer/Recv выделены; соседние копии Answer/Recv сняты',
            'duplicates_removed': duplicates, 'omitted': sorted(set(omitted)), 'events': events}


def c_header():
    """Статические сценарии для C-модели IRQ; JSON не нужен в прошивке."""
    output = ['typedef struct { const unsigned char *data; unsigned short count; unsigned char tx; } sim_step_t;']
    fixtures = [FIXTURES / name for name in ('m66-server-reject.json', 'slm336-no-sim.json')]
    variants = []
    for path in fixtures:
        fixture = json.loads(path.read_text(encoding='utf-8'))
        variants.append(fixture)
        joined = []
        for entry in fixture['events']:
            if 'rx' in entry and joined and 'rx' in joined[-1]:
                joined[-1]['rx'] += entry['rx']
            else:
                joined.append(entry.copy())
        variants.append({'events': joined})
    for i, fixture in enumerate(variants):
        for j, entry in enumerate(fixture['events']):
            data = next(iter(entry.values())).encode('ascii')
            output.append(f'static const unsigned char sim_{i}_{j}[]={{' + ','.join(str(b) for b in data) + '};')
        output.append(f'static const sim_step_t scenario_{i}[]={{')
        for j, entry in enumerate(fixture['events']):
            output.append(f'{{sim_{i}_{j},sizeof(sim_{i}_{j}),{int("tx" in entry)}}},')
        output.append('};')
    output += ['static const sim_step_t *const scenarios[]={' + ','.join(f'scenario_{i}' for i in range(len(variants))) + '};',
               'static const unsigned short scenario_counts[]={' + ','.join(f'sizeof(scenario_{i})/sizeof(scenario_{i}[0])' for i in range(len(variants))) + '};']
    return '\n'.join(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--logs', type=Path, required=True)
    args = parser.parse_args()
    FIXTURES.mkdir(parents=True, exist_ok=True)
    for name, short in zip(LOG_NAMES, ('slm336-no-sim', 'm66-server-reject')):
        fixture = extract(args.logs / name)
        (FIXTURES / (short + '.json')).write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(short, len(fixture['events']), 'steps;', fixture['duplicates_removed'], 'duplicate RX removed')
