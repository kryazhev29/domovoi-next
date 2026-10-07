"""Доработка: Кряжев А., 2026. Бюджет замены по карте IAR и объекту ARM."""
from pathlib import Path
import argparse, json, re, struct

def arm_sections(path):
    data=path.read_bytes()
    assert data[:5] == b'\x7fELF\x01' and data[5] == 1
    head=struct.unpack_from('<16sHHIIIIIHHHHHH',data)
    assert head[2] == 40, 'ARM object required'
    offset,size,count,names=head[6],head[11],head[12],head[13]
    sections=[struct.unpack_from('<IIIIIIIIII',data,offset+i*size) for i in range(count)]
    string_section=sections[names]
    strings=data[string_section[4]:string_section[4]+string_section[5]]
    alloc={strings[s[0]:].split(b'\0',1)[0].decode():s[5] for s in sections if s[2]&2}
    return {'arm_sections':alloc,'arm_ram':sum(s[5] for s in sections if s[2]&3 == 3),
            'arm_flash_estimate':sum(s[5] for s in sections if s[2]&2 and s[1]!=8)}


if __name__ == '__main__':
    p=argparse.ArgumentParser()
    p.add_argument('map',type=Path,nargs='?')
    p.add_argument('--object',type=Path)
    p.add_argument('--before',type=Path)
    a=p.parse_args()
    if not (a.map or a.object): p.error('map or --object required')
    result={}
    if a.map:
        text = a.map.read_text(encoding='utf-8')
        summary = text.split('*** MODULE SUMMARY')[1].split('*** ENTRY LIST')[0]
        modules = {}
        for name in ('gsm.o', 'user_gsm.o', 'uart_stm32f10x.o', 'user_fun.o'):
            line = next(x for x in summary.splitlines() if x.strip().startswith(name+' '))
            # Ширины колонок задаёт заголовок карты, пустые поля означают ноль.
            fields = [line[29:37], line[38:45], line[46:53], line[54:]]
            modules[name] = dict(zip(('ro_code','rw_code','ro_data','rw_data'),
                                    (int(x.strip().replace("'",'') or '0') for x in fields)))
        entries = {}; pending = None
        for line in text.split('*** ENTRY LIST')[1].splitlines():
            m = re.match(r"^(.*?)\s+(0x[\da-fA-F']+)\s+(0x[\da-fA-F]+)\s+(Code|Data)\s+\S+\s+(\S+\.o)", line)
            if m:
                name = m[1].strip() or pending
                if name: entries[name] = (int(m[3],16), m[4], m[5])
                pending = None
            elif line.strip() and re.match(r'^\w+$', line.strip()):
                pending = line.strip()
        uart_names = ('USART3_IRQHandler','uart_sendstr','uart_rxview','uart_takerx',
                      'uart_matchresponse','uart_sendStrAndWaitResp')
        user_names = ('gsm_user_debug','usergsm_debug','usergsm_event','user_setconnections','user_incomingcall')
        def code_sizes(names, owner):
            result = {}
            for name in names:
                size,kind,module = entries[name]
                assert kind == 'Code' and module == owner, name
                result[name] = size
            return result
        uart = code_sizes(uart_names,'uart_stm32f10x.o')
        user = code_sizes(user_names,'user_fun.o')
        legacy = sum(modules[n]['ro_code']+modules[n]['ro_data'] for n in ('gsm.o','user_gsm.o'))
        result = {'modules':modules,'uart_exclusive_code':uart,'user_gsm_code':user,
                  'legacy_budget_lower_bound':legacy+sum(uart.values())+sum(user.values())}
    if a.object: result.update(arm_sections(a.object))
    if a.before:
        if not a.object: p.error('--before requires --object')
        old=arm_sections(a.before)
        result['before']=old
        result['flash_delta']=result['arm_flash_estimate']-old['arm_flash_estimate']
        result['ram_delta']=result['arm_ram']-old['arm_ram']
        assert result['flash_delta'] <= 150, result
        assert result['ram_delta'] <= 0, result
    print(json.dumps(result,ensure_ascii=False,indent=2))