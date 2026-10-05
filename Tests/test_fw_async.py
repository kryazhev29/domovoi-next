"""Real FW command20 continuation: ownership, faults, cuts, incremental CRC."""
import pathlib, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
ns={'__file__':str(ROOT/'Tests/run_host.py')}
code=(ROOT/'Tests/run_host.py').read_text(encoding='utf-8');exec(code[:code.index('with tempfile.TemporaryDirectory')],ns)
src,fun,types=ns['source'],ns['function'],ns['typedefs'];u=src('User/user_fun.c')
defs='\n'.join(x for x in u.splitlines() if x.startswith('#define FW_'))
jobs=u[u.index('/* >>> Cooperative firmware block writer */'):u.index('/* <<< Cooperative firmware block writer */')]
prod='\n'.join(fun(u,n) for n in ('CRC32','fw_abort','fw_blocks_done','fw_take_block','user_storage_command_ready','user_storage_flash_ready'))+'\n'+jobs
prod+='\nstatic void command(uint8_t *ptr,uint16_t len,dataChanel_e cch,uint8_t uniqId){switch(ptr[4]){'+ns['cases'](u)(20)+'default:CHECK(0);}}\n'
assert fun(u,'user_storage_poll').index('fw_storage_poll(now)')<fun(u,'user_storage_poll').index('log_storage_poll(now)')
layouts=types(src('structures.h'),'status_s','triggers_s','device_s','dataChanel_e','bootFWparam_s','logUpdate_e')
unit=(ROOT/'Tests/fw_async_model.inc').read_text(encoding='utf-8').replace('/* TYPES */',layouts).replace('/* DEFINES */',defs).replace('/* PRODUCTION */',prod)
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);(p/'test.c').write_text(unit,encoding='utf-8')
    subprocess.run([ns['ZIG'],'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
    subprocess.run([str(p/'test.exe')],check=True,timeout=300)
