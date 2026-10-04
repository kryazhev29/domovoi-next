"""Доработка: Кряжев А., 2026. Полный прогон проверок без платы: python Tests/run_all.py [--fast].
--fast - без мутаций (они пересобирают модель десятки раз)."""
import subprocess, sys, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PY = sys.executable
GROUPS = ('access', 'bootjournal', 'cfgstrings', 'cmdlengths', 'commands', 'commat', 'commserver', 'commslm', 'commuart', 'countends', 'crc', 'crcblocks', 'crcff', 'flashcmds', 'fwblockbounds', 'fwstart', 'idexpiry', 'idindex', 'idsync', 'idtext', 'log', 'logcommit', 'logupload', 'outs', 'persistence', 'photo', 'recovery', 'rs485cfg', 'rules', 'rx', 'sectorjournal', 'serverrx', 'sms', 'storage', 'sync', 'tempidmeta', 'timersirq', 'uarttimer', 'uploadretry', 'usbdesc', 'usbresetwait', 'usbresume', 'usbsetupbounds', 'utcday', 'w25', 'wiegandirq')
TX = ('idle', 'reset', 'esof', 'busy', 'large')


def run(title, args, timeout=900):
    t = time.monotonic()
    r = subprocess.run([PY, *args], cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout)
    ok = r.returncode == 0 and "FAIL" not in r.stdout
    tail = (r.stdout.strip().splitlines() or [""])[-1][:110]
    print(("PASS " if ok else "FAIL ") + title.ljust(22) + " %5.1fs  " % (time.monotonic() - t) + tail, flush=True)
    if not ok:
        print((r.stdout + r.stderr)[-1500:])
    return ok


def main():
    fast = "--fast" in sys.argv
    results = [run("sources", ["Tests/check_comm_sources.py"])]
    results += [run(g, ["Tests/run_host.py", "work", g]) for g in GROUPS]
    results += [run("tx " + s, ["Tests/run_host.py", "work", "tx", s]) for s in TX]
    results += [run("async config", ["Tests/test_store_async.py"]), run("async callers", ["Tests/test_store_callers.py"])]
    results.append(run("modem_sim", ["-m", "unittest", "Tests/modem_sim.py"]))
    results.append(run("comm_review", ["Tests/comm_review_host.py"]))
    results.append(run("uart arm", ["Tests/build_comm_uart.py"]))
    if not fast:
        for script in ("comm_mutations.py", "recover_mutations.py", "idindex_mutations.py", "store_async_mutations.py"):
            results.append(run(script, ["Tests/" + script], timeout=3600))
    print("TOTAL %d/%d passed" % (sum(results), len(results)))
    sys.exit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
