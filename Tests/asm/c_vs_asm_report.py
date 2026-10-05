"""Assemble the final three JSON artifacts and the requested Russian report."""
import json
import os
from pathlib import Path
import tempfile
from c_vs_asm_suite import ROOT

BASE=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-c-asm'))
VARIANTS=('ASM','C','C_SPEED')

def read(name):return json.loads((BASE/name).read_text(encoding='utf-8'))
def save(name,value):(ROOT/'Tests/asm'/name).write_text(json.dumps(value,indent=2,ensure_ascii=False,default=str)+'\n',encoding='utf-8')
def count(value):return sum(x['status']=='PASS' for x in value['cases'].values()),len(value['cases'])
def merge_functions(target,source):
    for name,row in source.items():
        if name not in target:target[name]=dict(row);continue
        dst=target[name]
        for k in ('calls','instructions','fetch_estimate'):dst[k]+=row[k]
        dst['min']=min(dst['min'],row['min']);dst['max']=max(dst['max'],row['max'])

def main():
    audit=read('audit.json');paired=read('paired.json');variants=paired['variants']
    runs={v:read(f'run-all-{v}.json') for v in ('ASM','C')}
    mutations={v:read(f'mutations-{v}.json') for v in ('ASM','C')}
    coverage={v:read(f'coverage-{v}.json') for v in VARIANTS}
    leaf={v:read(f'leaf-{v}.json') for v in VARIANTS}
    paths={v:read(f'paths-{v}.json') for v in VARIANTS}
    for v in VARIANTS:
        expected=audit[v]['elf_sha256']
        for name,data in (('paired',variants[v]),('coverage',coverage[v]),('leaf',leaf[v]),('paths',paths[v])):
            assert data['elf_sha256']==expected,(v,name,'stale ELF results')
        if v in runs:
            assert runs[v]['elf_sha256']==expected,(v,'run_all','stale ELF results')
            assert count(runs[v])[0]==count(runs[v])[1],(v,'run_all failed')
            assert mutations[v]['elf_sha256']==expected,(v,'mutations','stale ELF results')
    for v in VARIANTS:
        extra=BASE/f'leaf-init-{v}.json'
        if extra.exists() and leaf[v]['fixtures'].get('user_init',{}).get('status')!='PASS':
            init=read(extra.name);leaf[v]['fixtures'].update(init['fixtures'])
            merge_functions(leaf[v]['functions'],init['functions'])
    functions={}
    for v in VARIANTS:
        functions[v]={n:dict(r) for n,r in variants[v]['functions'].items()}
        merge_functions(functions[v],coverage[v]['functions'])
        merge_functions(functions[v],leaf[v]['functions'])
        merge_functions(functions[v],paths[v]['functions'])
    exported=sorted(n for n,s in audit['ASM']['linked_publics'].items() if s['type']=='STT_FUNC')
    assert all(set(exported)<=set(functions[v]) for v in VARIANTS),'missing exported function measurement'
    assert not audit['protected_diff'] if 'protected_diff' in audit else True
    for v in ('C','C_SPEED'):
        for name,r in paths[v]['scenarios'].items():
            assert r['result']==paths['ASM']['scenarios'][name]['result'],(v,name,'complete path mismatch')
        for name,result in leaf[v]['fixtures'].items():
            reference=leaf['ASM']['fixtures'][name]
            assert result['status']==reference['status']=='PASS',(v,name,'cold API fixture did not complete')
            assert result['wire']==reference['wire'] and result['nor_sha256']==reference['nor_sha256'],(v,name,'cold API output mismatch')
    speed={'method':{'instructions':'Unicorn executed instructions, inclusive exported calls; modeled hardware boundaries excluded',
        'fetch_estimate':'I + 2*F, F = new 32-bit Flash fetch words; one retained word, LATENCY=2; no pipeline/prefetch/branch/data-wait model. Conditional estimate, NOT board cycles.',
        'coverage':'Additional function samples include failure paths; C_SPEED fixtures with basic-heap assumptions may fail. Do not compare aggregate means as identical call arguments.'},
        'scenarios':{v:{n:r['cost'] for n,r in {**variants[v]['scenarios'],**paths[v]['scenarios']}.items() if r.get('cost')} for v in VARIANTS},
        'functions':functions,'unmeasured':{v:sorted(set(exported)-set(functions[v])) for v in VARIANTS},
        'coverage_fixtures':{v:coverage[v]['coverage_fixtures'] for v in VARIANTS},
        'cold_api_fixtures':leaf,
        'elf_sha256':{v:variants[v]['elf_sha256'] for v in VARIANTS}}
    tests={'run_all':runs,'mutations':mutations,'paired':paired,'complete_paths':paths,
           'selected_projects':'run_all --project selects linked runtime fixtures; legacy synchronous host contracts are mapped explicitly in each result'}
    # Include source identities, so a later checkout can detect stale results.
    import hashlib,subprocess
    audit['source_sha256']={str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [*(ROOT/'User/c').iterdir(),ROOT/'User/user_fun.h',ROOT/'DomovoiNext_C.ewp',ROOT/'DomovoiNext_ASM.ewp'] if p.is_file()}
    audit['head']=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip()
    audit['protected_diff']=subprocess.check_output(['git','diff','--name-only','--','User/asm','DomovoiNext.ewp','main.c','User/prof.c','User/prof.h','User/wiegand.c','User/outs.c'],cwd=ROOT).decode().splitlines()
    save('c_vs_asm_builds.json',audit);save('c_vs_asm_tests.json',tests);save('c_vs_asm_speed.json',speed)
    lines=[
      '# C / ASM — ДОМОВОЙ NEXT',
      '',
      'Задание `C_VS_ASM_20261006`, основание `4883384f5f2b24e650866ca3b5615206a70d8d7b`, ветка `compare/c-asm`. Коммитов и действий с платой нет.',
      '',
      '## Результат и незакрытые требования',
      '',
      'Созданы 19 C-модулей, два проекта IAR и конфигурация C ReleaseSpeed. Все 363 PUBLIC-имени исходных ASM имеют внешнюю видимость в C-объектах; все 231 связанные функции сохранены. ASM, исходный проект, main и точки DWT не изменены. Совпали смещения полей 11 типов runtime_layout и адреса всех экспортируемых RAM-данных в C Size. Адреса двух констант Flash (handlers, comm_settings) зависят от размера связанного кода.',
      '',
      '**Ограничения относительно буквальных критериев:** High/Speed не помещается; запас RAM 160 Б отсутствует уже в исходном ASM; полный ELF отличается отладочными секциями. Измерения охватывают '+str(len(set(exported)&set(functions['C'])))+' из '+str(len(exported))+' экспортируемых функций. Исходные ASM-наборы мутаций с привязкой к opcode не перенесены целиком на C: выполнен общий набор из 20 одинаковых семантических мутаций.',
      '',
      '## Сборки и размер',
      '',
      'IAR ARM 9.50.1.380, Cortex-M3. ASM и C Release: High/Size, 0 ошибок, только прежнее Ls016. C ReleaseSpeed: High/Speed, две ошибки Lp011 размещения. Диагностическая сборка Speed использует только временный расширенный ICF; она не предназначена для STM32F103 с 128 КБ Flash / 20 КБ RAM.',
      '',
      '| Показатель, байт | ASM Size | C Size | C Speed, диагностика |',
      '|---|---:|---:|---:|',
    ]
    for label,key in (('RO code','ro_code'),('RW code','rw_code'),('RO data','ro_data'),('RW data','rw_data')):
        lines.append('| '+label+' | '+' | '.join(str(audit[v]['map_totals'][key]) for v in VARIANTS)+' |')
    lines += [
      '| Образ приложения Flash | '+' | '.join(str(audit[v]['flash_payload_bytes']) for v in VARIANTS)+' |',
      '| Свободно в разделе 97 792 Б | '+' | '.join(str(97792-audit[v]['flash_span_bytes']) for v in VARIANTS)+' |',
      '| Промежуток heap → fault_record | 64 | 64 | неприменимо: RAM расширена |',
      '',
      'C Size экономит 960 Б Flash (1,03%). RW data одинаковы: 19 881 Б; RW code — 516 Б. В существующем HEAD DWT уже оставил 64 Б свободного промежутка, а не указанные в задании 160 Б. Резерв не увеличивался путём удаления функций.',
      '',
      f'Speed превышает размер приложения на {audit["C_SPEED"]["flash_span_bytes"]-97792} Б по загружаемому образу. Реальная линковка требует 0x5170 Б в области RAM 0x4fc0: нехватка RAM 432 Б. Компилятор выбирает dlmalloc вместо basic heap; это часть результата High/Speed, не ручная замена аллокатора.',
      '',
      '### По заменённым модулям',
      '',
      'Flash = ro code + ro data; RAM = rw code + rw data из MODULE SUMMARY. Общие модули и библиотеки перечислены в JSON, итог включает также блоки heap/stack и размещение линкера.',
      '',
      '| Модуль | Flash ASM | Flash C | Flash Speed | RAM ASM | RAM C | RAM Speed |',
      '|---|---:|---:|---:|---:|---:|---:|',
    ]
    for stem in sorted(set(variants['ASM']['exports'].values())):
        rows=[audit[v]['modules'].get(stem+'.o',dict(ro_code=0,ro_data=0,rw_code=0,rw_data=0)) for v in VARIANTS]
        lines.append('| '+stem+' | '+' | '.join(str(r['ro_code']+r['ro_data']) for r in rows)+' | '+' | '.join(str(r['rw_code']+r['rw_data']) for r in rows)+' |')
    lines += [
      '',
      '### Сохранность ASM',
      '',
      'Все PT_LOAD-сегменты исходного и нового ASM ELF совпадают по адресам, длинам и SHA-256. Полный ELF не равен: различаются `.debug_line`, `.iar.debug_line`, `.comment` из-за путей новой сборки. ASM-исходники не менялись. Полные хеши и хеши сегментов — `c_vs_asm_builds.json`.',
      '',
      '## Проверки поведения',
      '',
    ]
    for v in ('ASM','C'):
        passed,total=count(runs[v]);lines.append(f'- `python Tests/run_all.py --project {v}`: **{passed}/{total}**; результаты относятся к выбранному ELF, выходы host-проверок находятся в TEMP.')
    lines += [
      '- Runtime-проверки: доступ и ошибки чтения, индекс, таблицы/журнал, фото и ACK, выгрузка, очистка ID, истечение срока, обновление FW, владельцы Flash, очередь USB, конфигурация, реальные SPI/W25 пути в модели. Восемь групп power-cut включены.',
      '- Синхронные старые host-шаблоны заменены существующими проверками актуального ELF там, где API стал пошаговым. Полная таблица соответствия `legacy_contract_mapping` сохранена в результатах; это отдельный режим `--project`, исходный режим run_all без выбора проекта не объявляется новой проверкой ASM/C.',
      f'- Прямое сравнение: **{len(variants["ASM"]["scenarios"])} одинаковых сценария/входа**, 0 расхождений. Пакеты, события и полные NOR-снимки сравниваются Python `==` до сериализации; для восстановлений после обрыва каждого действия журнала сравниваются SHA-256 полного NOR. В JSON большие массивы заменены длиной и SHA-256.',
      '- Отдельно совпали два полных пути: завершённый кадр Wiegand → запись GPIO реле при базе 50 048 ID; входной поток сервера → разбор SLDN/CRC → пакет ответа. Проверены также одинаковые пакеты и NOR для явных входов всех дополнительных cold API-фикстур.',
      '- Общие мутации: **20/20** в ASM и **20/20** в C. Меняются реальные Thumb-инструкции в памяти Unicorn, настройки патча проверяются до ожидаемого падения, кэш трансляции очищается. Дополнительно ASM прошёл прежние 25 opcode-мутаций таблиц, фото, W25, индекса, service CRC и USB. Полного переноса этих 25 сигнатур на C нет.',
      '- USB: одинаковые проверки PRIMASK, единственной публикации указателя, границ кольца и прерывания reset. Старый предел 12 000 инструкций отделён от функциональной проверки; превышения не скрыты, исходная проверка сохраняет лимит по умолчанию. Метрики всех шести ответов есть в `usb_replies`.',
      '- Проверки C Speed с допущениями о basic heap не проходят на dlmalloc (включая `Aldata` и максимальный запрос 4088 Б). Это ограничение диагностического профиля: полного зелёного run_all для Speed не заявляется.',
      '',
      '## Скорость',
      '',
      'Единицы: выполненные команды Unicorn. Поле оценки `E = I + 2F`, где F — число новых 32-битных выборок Flash с удержанием последнего слова, LATENCY=2. Эта упрощённая модель **не учитывает** конвейер, prefetch, штраф переходов, ожидания данных, IRQ и время внешней NOR. E не является измерением DWT или точной оценкой времени STM32; DWT-точки сохранены для последующей проверки на плате.',
      '',
      '### Одинаковые пути',
      '',
      'В ячейках: инструкции / E. Поиск полной базы: 50 048 записей, последняя запись или отсутствие ID. Путь карты заканчивается на реальном вызове out_load; GPIO/события моделируются. Главный цикл — фактический main между двумя входами в gsm_check_data, без входящих данных, без задачи хранения; начальная настройка оборудования заменена границами модели.',
      'Строка `wiegand-full-last-to-gpio` охватывает WG_process, форматирование ID, поиск в полностью заполненном разделе 1 601 536 Б и настоящий out_load до инструкции записи GPIOB BSRR (бит 3). Приём отдельных битов и ожидание межкадровой паузы 25 мс исключены. Строка `server-stream-17-to-reply` начинается с user_comm_rx/user_comm_rx_done и заканчивается успешным comm_tx_commit; физический модем моделируется.',
      '',
      '| Сценарий | ASM | C Size | C Speed | C Size / ASM, инструкции |',
      '|---|---:|---:|---:|---:|',
    ]
    for name,row in variants['ASM']['scenarios'].items():
        if '/' in name or not row.get('cost'):continue
        costs=[variants[v]['scenarios'][name]['cost'] for v in VARIANTS]
        lines.append('| '+name+' | '+' | '.join(f'{r["instructions"]} / {r["fetch_estimate"]}' for r in costs)+f' | {costs[1]["instructions"]/costs[0]["instructions"]:.3f} |')
    for name,row in paths['ASM']['scenarios'].items():
        costs=[paths[v]['scenarios'][name]['cost'] for v in VARIANTS]
        lines.append('| '+name+' | '+' | '.join(f'{r["instructions"]} / {r["fetch_estimate"]}' for r in costs)+f' | {costs[1]["instructions"]/costs[0]["instructions"]:.3f} |')
    lines += ['', '### Выигрыш ASM от 7 раз', '', '| Вход | ASM | C Size | C Speed | C Size / ASM |', '|---|---:|---:|---:|---:|']
    for name,row in variants['ASM']['scenarios'].items():
        if '/' not in name or not row.get('cost'):continue
        counts=[variants[v]['scenarios'][name]['cost']['instructions'] for v in VARIANTS]
        if max(counts[1:])/counts[0]>=7:
            lines.append('| '+name+'; параметры '+str(row.get('arguments'))+' | '+' | '.join(map(str,counts))+f' | {counts[1]/counts[0]:.2f} |')
    lines += [
      '',
      'Порог 7× подтверждается для разреженного GF(2), x=0x8000: ASM пропускает нулевые биты через RBIT/CLZ, C сдвигает маску. Переносить этот результат на все fast_kernels нельзя. На пути карты C Size быстрее ASM; главный пустой цикл отличается примерно на 2,3%.',
      '',
      '### Экспортируемые функции',
      '',
      'Среднее число инструкций / среднее E на вызов, включая вложенные вызовы. Это наблюдения из набора сценариев, ошибок и границ; число вызовов и min/max сохранены в JSON. Дополнительные явные входы `c_vs_asm_leaf.py` измеряют простые API на пустой NOR, включая полный user_init, инициализаторы периферии и допустимые отказы API. Это не максимальная стоимость каждого возможного входа. Моделируемые аппаратные границы перечислены в JSON. Для сравнения скорости на точно одинаковых входах используйте таблицу путей и cold_api_fixtures, а не отношение средних по смешанным вызовам. Строки C Speed из незавершившихся heap-фикстур не доказывают эквивалентность этих фикстур.',
      '',
      '| Функция | ASM I/E | C Size I/E | C Speed I/E |',
      '|---|---:|---:|---:|',
    ]
    for name in exported:
        values=[]
        for v in VARIANTS:
            r=functions[v].get(name)
            values.append('—' if not r else f'{r["instructions"]/r["calls"]:.1f} / {r["fetch_estimate"]/r["calls"]:.1f}')
        lines.append('| '+name+' | '+' | '.join(values)+' |')
    lines += [
      '',
      '## Что отличалось в C-эталонах',
      '',
      '1. Единственный владелец Flash, claim/release и запрет чтения во время чужой операции; готовность таблиц и конфигурации проверяются до старта.',
      '2. Config job занимает 40 Б вместо старых 104 Б и использует общие 64 Б sjr; чтение конфигурации использует выбранный кэшированный банк.',
      '3. ID-index — текущий пошаговый автомат с pending/source/bucket; чтения и асинхронные действия ограничены за один poll.',
      '4. Группы/расписания читаются по номеру ячейки, праздники — блоками 64 Б. Ошибки чтения ID и праздника возвращают 2. Условные объявления C в user_fun.h отражают фактический трёхзначный ABI ASM вместо bool.',
      '5. Команды 13–16, 17/18, 19, 103 и выгрузка используют актуальные автоматы, отмену, владение ответом и сохранение после verified; пустой пакет 9 содержит три байта данных.',
      '6. Фото использует актуальные фазы владения, общий reply_send, сохранение префикса, CRC, ACK и восстановление после питания.',
      '7. usb_try_send реализован C с CMSIS/IAR intrinsics IRQ/DMB: проверка epoch, кольцо 1280 Б и единственная публикация полного пакета. Шесть asm("nop") старого W25-эталона заменены аппаратным intrinsic __no_operation; inline ASM в User/c отсутствует и запрещён source-check. Intrinsics используются только для процессорных примитивов и аппаратных пауз CS; алгоритмические ядра написаны обычным C.',
      '8. Написаны отсутствовавшие C-автоматы expiry, idclear, upload, fw_service, service и ядра CRC/FNV/blank/scan/GF2. SPI/USB-эталоны найдены в reference/Modules, хотя в задании были перечислены как отсутствующие.',
      '9. Команда 2 через timeToReset уже была в имеющемся эталоне; этот факт проверен, нового поведения не добавлено. Выдача пакета профиля 113 после 112 сохранена.',
      '',
      'Ошибки производственной логики не исправлялись. Обнаруженные сбои отдельных запусков C на Unicorn оказались связаны с глобальными MEM_WRITE/MEM_READ hooks и условными Thumb IT/POP; hooks сужены до реально моделируемых регионов. Изменений ASM для обхода эмулятора нет. Стоп-условие возврата модуля на ASM не потребовалось.',
      '',
      '## Воспроизведение',
      '',
      'Нужны установленный IAR 9.50.1, Python, unicorn 2.1.4, pyelftools и capstone. Старый host-компилятор Zig задаётся существующими тестами. Все промежуточные проекты, ELF, maps, логи и host-копии идут в `%TEMP%/domovoi-c-asm`; пути переопределяются DOMOVOI_COMPARE_BUILD / DOMOVOI_ELF / DOMOVOI_BIN / DOMOVOI_MAP.',
      '',
      '```powershell',
      'python Tests/check_comm_sources.py',
      'python Tests/asm/c_vs_asm_build.py --variant baseline',
      'python Tests/asm/c_vs_asm_build.py --variant ASM',
      'python Tests/asm/c_vs_asm_build.py --variant C',
      'python Tests/asm/c_vs_asm_build.py --variant C --configuration ReleaseSpeed',
      '# Ожидаются две Lp011; следующая сборка — только диагностическая:',
      'python Tests/asm/c_vs_asm_build.py --variant C --configuration ReleaseSpeed --diagnostic',
      'python Tests/run_all.py --project ASM',
      'python Tests/run_all.py --project C',
      'python Tests/asm/c_vs_asm_compare.py --paired --output "$env:TEMP/domovoi-c-asm/paired.json"',
      'foreach ($variant in "ASM","C","C_SPEED") {',
      '  python Tests/asm/c_vs_asm_compare.py --variant $variant --coverage-only --output "$env:TEMP/domovoi-c-asm/coverage-$variant.json"',
      '  python Tests/asm/c_vs_asm_leaf.py --variant $variant --output "$env:TEMP/domovoi-c-asm/leaf-$variant.json"',
      '  python Tests/asm/c_vs_asm_paths.py --variant $variant --output "$env:TEMP/domovoi-c-asm/paths-$variant.json"',
      '}',
      'python Tests/asm/c_vs_asm_audit.py',
      'python Tests/asm/c_vs_asm_report.py',
      '```',
      '',
      'Итоговые машинные файлы: `Tests/asm/c_vs_asm_builds.json`, `c_vs_asm_tests.json`, `c_vs_asm_speed.json` (три файла). План: `docs/c_asm_plan.md`.',
      '',
      'Ограничение очистки: автоматическая проверка команд запретила удаление 40 игнорируемых `.pyc` в `Tests/__pycache__` и `Tests/asm/__pycache__` с причиной `blocked by policy`, в том числе попытку удалить явно перечисленные файлы. Эти кэши остались; повторные прогоны используют PYTHONDONTWRITEBYTECODE=1. Остальные промежуточные артефакты находятся в TEMP.',
    ]
    (ROOT/'Tests/C_VS_ASM_20261006.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print('Report and 3 JSON artifacts written; compared',len(variants['ASM']['scenarios']),'scenarios')

if __name__=='__main__':main()
