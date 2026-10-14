"""SLM336: синтетический путь до настоящего разбора ответа регистрации сервера.

UART готов через 2800 мс от PC13; CS GSM через 6500 мс, PS через 11000 мс
без явного attach; EPS LTE через 4000 мс, auto через 5000 мс. CGATT 400 мс,
QIDEACT 300 мс, QIACT 700 мс, QIOPEN 500 мс, остальные ответы из общего стенда.
Это заданные задержки модели, не прогноз времени регистрации реальной сети.
"""
import argparse
import io
import json
import os
from pathlib import Path
import subprocess
import zipfile
from temp_cleanup import TemporaryDirectory
import comm_server_host as server
import comm_slm_host as modem
from iar_image import result_path

ROOT = Path(__file__).resolve().parents[1]


def model(root, check):
    old_root, old_defaults = server.ROOT, modem.unit.__defaults__
    try:
        server.ROOT = root
        modem.unit.__defaults__ = (root,)
        code = server.unit().replace('\r\n', '\n')
    finally:
        server.ROOT, modem.unit.__defaults__ = old_root, old_defaults
    code = code.replace('int main(void)', 'int original_main(void)')
    code = code.replace('static void transport_tx(void)', 'static void original_transport_tx(void)')
    code = code.replace('  transport_tx();', '  bench_tx();')
    code = code.replace('  comm_modem_transport(NULL); sim_reset();',
                        '  comm_modem_transport(NULL); sim_reset(); test_config.network_type=bench_mode; sim_rat_change=bench_mode;')
    code = code.replace('for (unsigned i=0;!comm_server_online() && i<1500;++i) transport_tick(10);',
                        'for (unsigned i=0;!comm_server_online() && i<2500;++i) { transport_tick(10); bench_sample(); }')
    code = code.replace('sim_now<11000', 'sim_now<25000')
    code = code.replace('test.local', '192.0.2.1')
    code = code.replace('static void transport_tick(unsigned increment)\n{',
                        'static void transport_tick(unsigned increment)\n{\n  if (bench_open_due && sim_now>=bench_open_due) { bench_open_due=0; sim_feed("+QIOPEN: 0,0\\r\\n"); }')
    code = code.replace('CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));',
                        'if (strstr(s,"+QIOPEN:")) { bench_open_due=sim_now+500; s="OK\\r\\n"; delay=30; }\n  CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));')
    prefix = '''static unsigned bench_mode, bench_gsm, bench_net, bench_ps;
static unsigned bench_online, bench_tcp, bench_first, bench_attach, bench_deact, bench_open_due;
static void bench_tx(void);
static void bench_sample(void);
'''
    suffix = r'''
static void bench_sample(void)
{
  if (comm_channel_ready() && !bench_tcp) bench_tcp=sim_now-10u;
  if (comm_server_online() && !bench_online) bench_online=sim_now-10u;
}
static void bench_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE) || tx_data!=command) { original_transport_tx(); return; }
  char cmd[129]; CHECK(tx_count<sizeof(cmd)); memcpy(cmd,tx_data,tx_count); cmd[tx_count-1u]=0;
  unsigned elapsed=sim_now-power_trace[2].time;
  if (!strcmp(cmd,"AT") && !bench_first) bench_first=sim_now;
  const char *tag=!strcmp(cmd,"AT+CREG?") ? "+CREG" : !strcmp(cmd,"AT+CGREG?") ? "+CGREG" : !strcmp(cmd,"AT+CEREG?") ? "+CEREG" : NULL;
  if ((!strcmp(cmd,"AT") && elapsed<2800) || tag || !strcmp(cmd,"AT+CGATT=1") || !strcmp(cmd,"AT+QIDEACT=1")) {
    writes=0;
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (tag) {
      unsigned registered=!strcmp(tag,"+CEREG") ? !bench_gsm && elapsed>=bench_net
        : bench_gsm && elapsed>=(!strcmp(tag,"+CREG") ? bench_net : bench_ps);
      char response[48]; snprintf(response,sizeof(response),"%s: 1,%u\r\nOK\r\n",tag,registered ? 1 : 2);
      sim_reply(response,80);
    } else if (!strcmp(cmd,"AT+CGATT=1")) {
      CHECK(bench_gsm && elapsed>=bench_net); ++bench_attach;
      bench_ps=elapsed+400; sim_reply("OK\r\n",400);
    } else if (!strcmp(cmd,"AT+QIDEACT=1")) { ++bench_deact; sim_reply("OK\r\n",300); }
    return;
  }
  original_transport_tx();
}
int main(void)
{
  const unsigned modes[]={1,3,0};
  for (unsigned i=0;i<3;++i) {
    bench_mode=modes[i]; bench_gsm=bench_mode==1; bench_net=bench_gsm ? 6500 : bench_mode==3 ? 4000 : 5000;
    bench_ps=11000; bench_online=bench_tcp=bench_first=bench_attach=bench_deact=bench_open_due=0;
    server_reset_start(true);
    unsigned sum=0; for (unsigned p=0;p<5;++p) sum+=slm.diag.phases[p];
    CHECK(bench_tcp>=sum && bench_online>=bench_tcp && !dns_requests && registrations_sent==1);
    if (BENCH_CHECK) {
      CHECK(!bench_deact && bench_attach==bench_gsm);
      CHECK(bench_first-power_trace[4].time<=20u);
      unsigned all=0; for (unsigned p=0;p<6;++p) all+=slm.diag.phases[p]; CHECK(all==bench_online);
      CHECK(bench_online-power_trace[2].time <= (bench_gsm ? 11000u : 10000u));
    }
    printf("BENCH {\"mode\":%u,\"power\":%u,\"AT\":%u,\"SIM\":%u,\"network\":%u,\"PDP\":%u,\"TCP\":%u,\"server_reply\":%u,\"total\":%u,\"from_PC13\":%u,\"first_AT\":%u,\"CGATT\":%u,\"QIDEACT\":%u}\n",
      bench_mode,slm.diag.phases[0],slm.diag.phases[1],slm.diag.phases[2],slm.diag.phases[3],slm.diag.phases[4],
      bench_tcp-sum,bench_online-bench_tcp,bench_online,bench_online-power_trace[2].time,bench_first,bench_attach,bench_deact);
  }
  return 0;
}
'''
    return '#define BENCH_CHECK ' + str(int(check)) + '\n' + prefix + code + suffix


def run(root, check, folder, name):
    source = folder / (name + '.c')
    binary = folder / (name + '.exe')
    source.write_text(model(root, check), encoding='utf-8')
    compiled = subprocess.run([server.ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                              str(source), '-o', str(binary)], capture_output=True)
    if compiled.returncode: print(compiled.stderr.decode('utf-8', errors='replace')[-3000:])
    compiled.check_returncode()
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    if result.returncode: print(result.stdout, result.stderr)
    result.check_returncode()
    return [json.loads(s[6:]) for s in result.stdout.splitlines() if s.startswith('BENCH ')]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline-ref')
    args = parser.parse_args()
    target = result_path('fast-start-timing.json')
    with TemporaryDirectory(dir=target.parent) as temp:
        folder = Path(temp)
        os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(folder / 'zig-global')
        os.environ['ZIG_LOCAL_CACHE_DIR'] = str(folder / 'zig-local')
        rows = {'model': __doc__, 'after': run(ROOT, True, folder, 'after')}
        if args.baseline_ref:
            archive = subprocess.check_output(['git', 'archive', '--format=zip', args.baseline_ref], cwd=ROOT)
            baseline = folder / 'baseline'
            with zipfile.ZipFile(io.BytesIO(archive)) as package:
                package.extractall(baseline)
            rows['before'] = run(baseline, False, folder, 'before')
            for old, new in zip(rows['before'], rows['after']):
                assert old['mode'] == new['mode'] and old['total'] > new['total']
        target.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('PASS fast start: GSM/LTE/auto, real registration reply, USB112 sum; ' + str(target))


if __name__ == '__main__':
    main()
