"""Runtime event writer: real C, NOR faults/power cuts/queue ownership."""
import re, pathlib, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
ns={'__file__':str(ROOT/'Tests/run_host.py')}
code=(ROOT/'Tests/run_host.py').read_text(encoding='utf-8')
exec(code[:code.index('with tempfile.TemporaryDirectory')],ns)
src,fun,types=ns['source'],ns['function'],ns['typedefs']
u=src('User/user_fun.c')
queue=u[u.index('#define LOG_PENDING_MAX'):u.index('/* <<< отложенные события журнала */')]
queue=re.sub(r'/\* >>> Cooperative firmware block writer \*/.*?/\* <<< Cooperative firmware block writer \*/', '', queue, flags=re.S)
production=fun(src('User/crc.c'),'crc16_calc')+'\n'
production+='\n'.join(fun(u,n) for n in ('log_blank','log_valid','log_getRecordsCount'))
unit=(ROOT/'Tests/event_async_model.inc').read_text(encoding='utf-8').replace('/* TYPES */',types(src('structures.h'),'cellLog_s'))
unit=unit.replace('/* PRODUCTION */',production+'\n'+queue)
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d);(p/'test.c').write_text(unit,encoding='utf-8')
    subprocess.run([ns['ZIG'],'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
    subprocess.run([str(p/'test.exe')],check=True,timeout=300)
