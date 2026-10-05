"""Real storage + upload continuations, NOR model; no device/network I/O."""
import pathlib, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
ns={"__file__":str(ROOT/"Tests/run_host.py")}
code=(ROOT/"Tests/run_host.py").read_text(encoding="utf-8")
exec(code[:code.index("with tempfile.TemporaryDirectory")],ns)
src,fun,types=ns["source"],ns["function"],ns["typedefs"]
u=src("User/user_fun.c")
st=src("structures.h")
base=src("Tests/test_storage.c").split("static void legacy_user_setdefault")[0]
base=base.replace("static saved_s config,device;", "")
a=base.index("#define MB_CONFIG"); b=base.index("static jmp_buf",a)
base=base[:a]+base[b:]
base=base.replace("oplength[256]","oplength[1024]")
layouts=types(st,"releMode_s","rs485Periphery_s","config_s","status_s","triggers_s","device_s","cellLog_s","cellID_s","dataChanel_e","cmdCode_e","shtamp_e","memBlock_e")
base+=fun(src("User/crc.c"),"crc16_calcInit")+"\n"+layouts+src("Tests/store_callers_model.inc")
header="\n".join(x for x in src("User/memory.h").splitlines() if x.startswith(("#define MEM","typedef uint8_t (*mem_page_reader","bool mem_patch","uint8_t mem_config_result")))
shared=src("User/sector_journal.h")+src("User/sector_journal.c").replace('#include "sector_journal.h"','')
mem=src("User/memory.c").replace('#include "memory.h"','').replace('#include "sector_journal.h"','')
decls=u[u.index("/* >>> Cooperative storage callers */"):u.index("/* <<< Cooperative storage callers */")]
upload=u[u.index("/* >>> подтверждение выгрузки */"):u.index("/* <<< подтверждение выгрузки */")]
impl=u[u.index("/* >>> Cooperative storage implementation */"):]
names=("log_blank","log_valid","log_uniqueIndex","log_getRecordsCount","log_readRecords","user_getTempIDRecordsCount","log_addEvent","log_flushPending","user_send_logRecords","user_send_tempIDs","user_comm_session","user_writeConfigAndDevice")
unit=base+src("Tests/store_async_model.inc")+header+shared+mem+decls+upload+"\n".join(fun(u,n) for n in names)+impl+src("Tests/store_callers_cases.inc")
unit=src("User/fast_kernels.h")+"\n"+unit.replace('#include "fast_kernels.h"','')
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d); (p/"test.c").write_text(unit,encoding="utf-8")
 subprocess.run([ns["ZIG"],"cc","-std=c99","-O2","-Wall","-Wextra","-Wno-unused-function","-Wno-unused-variable",str(p/"test.c"),"-o",str(p/"test.exe")],check=True)
 subprocess.run([str(p/"test.exe")],check=True,timeout=300)
