"""Final IAR ELF collector, per-pass instructions (not cycles/time)."""
import argparse, hashlib, json
from pathlib import Path
from test_asm_crc import Machine, STACK, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_PC, UC_ARM_REG_LR
from test_perf_scan import crc16


def measure(path, data, bounded):
    m = Machine(path)
    count, low = [0], [STACK]
    timer = m.sym['timers_getMillis'] & ~1
    def hook(u, a, n, unused):
        count[0] += 1
        low[0] = min(low[0], u.reg_read(UC_ARM_REG_SP))
        if a == timer:
            from test_asm_crc import ARGS
            u.reg_write(ARGS[0], 0)
            u.reg_write(UC_ARM_REG_PC, u.reg_read(UC_ARM_REG_LR))
    m.uc.hook_add(UC_HOOK_CODE, hook)
    m.uc.mem_write(m.sym['server_rx_buffer'], data)
    m.uc.mem_write(m.sym['server_rx_count'], len(data).to_bytes(2,'little'))
    def get(name):
        return int.from_bytes(m.uc.mem_read(m.sym[name],2),'little')
    passes=[]
    for _ in range(20000):
        before=(get('server_rx_count'),get('server_rx_ready'))
        if bounded: before+=(get('server_rx_pos'),get('server_rx_offset'))
        count[0]=0
        if bounded: m.call('server_rx_begin_pass',check_abi=True)
        m.call('server_rx_scan',check_abi=True)
        passes.append(count[0])
        after=(get('server_rx_count'),get('server_rx_ready'))
        if bounded: after+=(get('server_rx_pos'),get('server_rx_offset'))
        if not bounded or after[1] or not after[0] or before==after: break
    else: raise AssertionError('collector did not finish')
    return dict(max_instructions=max(passes), total_instructions=sum(passes),
                passes=len(passes), stack_bytes=STACK-low[0], left=get('server_rx_count'),
                ready=get('server_rx_ready'), tail=bytes(m.uc.mem_read(m.sym['server_rx_buffer'],get('server_rx_count'))).hex())


def main():
    p=argparse.ArgumentParser();p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--firmware',type=Path,required=True);p.add_argument('--json',type=Path,required=True)
    a=p.parse_args()
    from test_fast_integration import scan_samples
    cases=scan_samples()
    cases['scan_boundary']=b'\xa5'*63+cases['valid']
    result={'units':'Unicorn instructions, not board cycles or time', 'cases':{}}
    for name,data in cases.items():
        old,new=measure(a.baseline,data,False),measure(a.firmware,data,True)
        assert (old['left'],old['ready'],old['tail'])==(new['left'],new['ready'],new['tail']),name
        assert new['max_instructions'] < 16000,(name,new)
        result['cases'][name]={'before':old,'after':new}
        print(name,old['max_instructions'],'->',new['max_instructions'],'passes',new['passes'])
    result['sha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (a.baseline,a.firmware)}
    a.json.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS bounded collector ELF: equivalent output, bounded passes, ABI')

if __name__=='__main__':main()
