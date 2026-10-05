"""Required async contracts: each deliberate production fault must be detected."""
import pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CASES=(
 ("early-cursor","Tests/asm/reference/User/user_fun.c","if (accepted) store.job = US_ACK;","if (accepted) { store.job = US_ACK; upload_setIndex(cmd,cch,store.cursor); }","test_store_callers.py"),
 ("duplicate-ack","Tests/asm/reference/User/user_fun.c","!pending[channel] || store.repeat","!pending[channel]","test_store_callers.py"),
 ("missing-clear-intent","Tests/asm/reference/User/user_fun.c","store_marker(cmd, end);\r\n  /* Keep","store_marker(0, 0);\r\n  /* Keep","test_store_callers.py"),
 ("append-during-clear","Tests/asm/reference/User/user_fun.c","store.clear == 10 || !user_storage_flash_ready()","!user_storage_flash_ready()","test_store_callers.py"),
 ("ignored-store-error","Tests/asm/reference/User/memory.c","mem_config_errors++; cfg_job.result = 3;","mem_config_errors++; cfg_job.result = 2;","test_store_async.py"),
 ("lost-device-snapshot","Tests/asm/reference/User/memory.c","memcpy(cfg_job.snapshot + 512, second, 512);","memset(cfg_job.snapshot + 512, 0, 512);","test_store_async.py"),
)
with tempfile.TemporaryDirectory() as d:
 dst=pathlib.Path(d)
 shutil.copytree(ROOT/"User",dst/"User")
 shutil.copytree(ROOT/"Tests",dst/"Tests",ignore=shutil.ignore_patterns("__pycache__"))
 shutil.copyfile(ROOT/"structures.h",dst/"structures.h")
 for label,file,old,new,runner in CASES:
  p=dst/file; original=p.read_bytes(); assert old.encode("cp1251") in original,label
  p.write_bytes(original.replace(old.encode("cp1251"),new.encode("cp1251"),1))
  r=subprocess.run([sys.executable,str(dst/"Tests"/runner)],cwd=dst,capture_output=True,timeout=60)
  p.write_bytes(original)
  assert r.returncode and b"FAIL" in r.stderr,(label,"not detected or did not reach an assertion",r.stderr[-600:])
  print(label,"detected",flush=True)
print(f"PASS async mutations: {len(CASES)}/{len(CASES)}")
