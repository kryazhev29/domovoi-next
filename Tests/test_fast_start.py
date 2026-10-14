"""SLM336: путь до настоящего разбора ответа регистрации сервера.

Плата №1: UART отвечает OK сразу после ключа; AT READY через 2500 мс,
SIM через 4100 мс, до этого CPIN даёт CME 14, GSN/QCFG до AT READY - пустой OK.
Main занят до 5000 мс, GPIO идут из тика. CS GSM через 6500 мс от PC13,
EPS LTE через 4000 мс, auto через 5000 мс. CGATT 400 мс,
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
import test_startup_ready
from iar_image import result_path

ROOT = Path(__file__).resolve().parents[1]


def model(root, check, mutation=''):
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
                        'bench_init(); for (unsigned i=0;!comm_server_online() && i<2500;++i) { transport_tick(10); bench_sample(); }')
    code = code.replace('sim_now<11000', 'sim_now<25000')
    code = code.replace('test.local', '192.0.2.1')
    code = code.replace('static void transport_tick(unsigned increment)\n{',
                        'static void transport_tick(unsigned increment)\n{\n  comm_power_v5_poll(sim_now);\n'
                        '  if (sim_now<5000) { sim_now+=increment; return; }\n'
                        '  bench_urc();\n'
                        '  if (bench_open_due && sim_now>=bench_open_due) { bench_open_due=0; sim_feed("+QIOPEN: 0,0\\r\\n"); }')
    code = code.replace('CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));',
                        'if (strstr(s,"+QIOPEN:")) { bench_open_due=sim_now+500; s="OK\\r\\n"; delay=30; }\n  CHECK(!sim_pending[0] && strlen(s) < sizeof(sim_pending));')
    if mutation == 'early_setup':
        code = code.replace('if (!slm.attempt.at_ready && slm.attempt.result == COMM_AT_OK)',
                            'if (0 && !slm.attempt.at_ready && slm.attempt.result == COMM_AT_OK)')
        code = code.replace('if (action != MODEM_READY && entry->action >> 4 == SLM_PHASE_SETUP',
                            'if (0 && action != MODEM_READY && entry->action >> 4 == SLM_PHASE_SETUP')
    elif mutation == 'missing_data':
        code = code.replace('if (action != MODEM_READY && entry->action >> 4 == SLM_PHASE_SETUP',
                            'if (0 && action != MODEM_READY && entry->action >> 4 == SLM_PHASE_SETUP')
    elif mutation == 'sim_fail':
        code = code.replace('if (action == MODEM_SIM) {\n    uint16_t code',
                            'if (0 && action == MODEM_SIM) {\n    uint16_t code')
    prefix = '''static unsigned bench_mode, bench_gsm, bench_net, bench_ps, bench_case;
static unsigned bench_online, bench_tcp, bench_first, bench_attach, bench_deact, bench_open_due;
static unsigned bench_at_urc, bench_sim_urc, bench_net_urc, bench_injected, bench_setup, bench_failed;
static void bench_tx(void);
static void bench_sample(void);
static void bench_urc(void);
static void bench_init(void);
'''
    suffix = r'''
static void bench_sample(void)
{
  if (comm_channel_ready() && !bench_tcp) bench_tcp=sim_now-10u;
  if (comm_server_online() && !bench_online) bench_online=sim_now-10u;
  if (slm.attempt.status.state==SLM_FAILED && !bench_failed) bench_failed=sim_now-10u;
  comm_recover_poll(sim_now-10u);
}
static void bench_init(void) { comm_recover_init(&rc_hooks,sim_now); }
static void bench_urc(void)
{
  if (power_events<5) return;
  unsigned age=sim_now-power_trace[4].time;
  if (!bench_at_urc && age>=2500 && bench_case!=1) {
    bench_at_urc=sim_now; sim_feed("AT READY\r\n");
  }
  if (!bench_sim_urc && age>=(bench_case==2 ? 2000u : 4100u) && bench_case!=3) {
    bench_sim_urc=sim_now; sim_feed(bench_case==4 ? "+CPIN: READY\r\n" : "+SIM1 Ready\r\n");
  }
  if (!bench_net_urc && age>=1800 && bench_case==2) {
    bench_net_urc=sim_now; sim_feed(bench_gsm ? "+CREG: 1\r\n" : "+CEREG: 1\r\n");
  }
}
static void bench_tx(void)
{
  if (!(regs.CR1 & USART_CR1_TXEIE) || tx_data!=command) { original_transport_tx(); return; }
  char cmd[129]; CHECK(tx_count<sizeof(cmd)); memcpy(cmd,tx_data,tx_count); cmd[tx_count-1u]=0;
  unsigned elapsed=sim_now-power_trace[2].time;
  unsigned age=sim_now-power_trace[4].time;
  if (!strcmp(cmd,"AT") && !bench_first) bench_first=sim_now;
  bool setup=slm.attempt.status.phase==SLM_PHASE_SETUP && strcmp(cmd,"AT") && strcmp(cmd,"AT+CMEE?");
  if (setup && !bench_setup) bench_setup=sim_now;
  const char *boot=NULL;
  if (!strcmp(cmd,"AT")) boot="OK\r\n";
  else if (setup && age<2500) boot="OK\r\n";
  else if (!strcmp(cmd,"AT+CPIN?") && age<(bench_case==2 ? 2000u : 4100u))
    boot=bench_case==4 ? "+CPIN: NOT READY\r\nOK\r\n" : bench_case==5 ? "OK\r\n" :
         bench_case==6 ? "+CME ERROR: 10\r\n" : bench_case==7 ? "+CME ERROR: 13\r\n" : "+CME ERROR: 14\r\n";
  else if (!bench_injected && bench_case>=8 && !strcmp(cmd,"AT+GSN")) {
    bench_injected=1; boot=bench_case==8 ? "OK\r\n" : bench_case==9 ? "ERROR\r\n" : "+CME ERROR: 14\r\n";
  }
  const char *tag=!strcmp(cmd,"AT+CREG?") ? "+CREG" : !strcmp(cmd,"AT+CGREG?") ? "+CGREG" : !strcmp(cmd,"AT+CEREG?") ? "+CEREG" : NULL;
  if (boot || tag || !strcmp(cmd,"AT+CGATT=1") || !strcmp(cmd,"AT+QIDEACT=1")) {
    writes=0;
    for (unsigned i=0;i<tx_count;++i) interrupt_byte(0,USART_SR_TXE);
    interrupt_byte(0,USART_SR_TC);
    if (boot) sim_reply(boot,30);
    else if (tag) {
      unsigned registered=!strcmp(tag,"+CEREG") ? !bench_gsm && elapsed>=bench_net
        : bench_gsm && elapsed>=(!strcmp(tag,"+CREG") ? bench_net : bench_ps);
      char response[48]; snprintf(response,sizeof(response),"%s: 1,%u\r\nOK\r\n",tag,registered ? 1 : 2);
      sim_reply(response,80);
    } else if (!strcmp(cmd,"AT+CGATT=1")) {
      CHECK(bench_gsm); ++bench_attach;
      bench_ps=elapsed+400; sim_reply("OK\r\n",400);
    } else if (!strcmp(cmd,"AT+QIDEACT=1")) { ++bench_deact; sim_reply("OK\r\n",300); }
    return;
  }
  original_transport_tx();
}
int main(void)
{
  const unsigned modes[]={1,3,0};
  for (bench_case=0;bench_case<(BENCH_CHECK ? 11u : 1u);++bench_case) for (unsigned i=0;i<3;++i) {
    bench_mode=modes[i]; bench_gsm=bench_mode==1; bench_net=bench_gsm ? 6500 : bench_mode==3 ? 4000 : 5000;
    bench_ps=11000; bench_online=bench_tcp=bench_first=bench_attach=bench_deact=bench_open_due=0;
    bench_at_urc=bench_sim_urc=bench_net_urc=bench_injected=bench_setup=bench_failed=0;
    server_reset_start(false);
    unsigned sum=0; for (unsigned p=0;p<5;++p) sum+=slm.diag.phases[p];
    CHECK(bench_tcp>=sum && bench_online>=bench_tcp && !dns_requests && registrations_sent==1);
    if (BENCH_CHECK) {
      CHECK(!bench_deact && bench_attach==bench_gsm);
      CHECK(bench_first<=5020 && !slm.diag.failure_count);
      CHECK(bench_setup-power_trace[4].time>=(bench_case==1 ? 3500u : 2500u));
      unsigned all=0; for (unsigned p=0;p<6;++p) all+=slm.diag.phases[p]; CHECK(all==bench_online);
      CHECK(bench_online-power_trace[2].time <= (bench_gsm ? 11000u : 10000u));
    }
    if (!bench_case) printf("BENCH {\"mode\":%u,\"power\":%u,\"AT\":%u,\"SIM\":%u,\"network\":%u,\"PDP\":%u,\"TCP\":%u,\"server_reply\":%u,\"total\":%u,\"from_PC13\":%u,\"first_AT\":%u,\"CGATT\":%u,\"QIDEACT\":%u,\"failed_at\":%u,\"attempt_start\":%u,\"tcp_ready\":%u}\n",
      bench_mode,slm.diag.phases[0],slm.diag.phases[1],slm.diag.phases[2],slm.diag.phases[3],slm.diag.phases[4],
      bench_tcp-slm.attempt.attempt_started-sum,bench_online-bench_tcp,bench_online,bench_online-power_trace[2].time,bench_first,bench_attach,bench_deact,
      bench_failed,slm.attempt.attempt_started,bench_tcp);
  }
  return 0;
}
'''
    return '#define BENCH_CHECK ' + str(int(check)) + '\n' + prefix + code + suffix


def run(root, check, folder, name, mutation=''):
    source = folder / (name + '.c')
    binary = folder / (name + '.exe')
    source.write_text(model(root, check, mutation), encoding='utf-8')
    compiled = subprocess.run([server.ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                              str(source), '-o', str(binary)], capture_output=True)
    if compiled.returncode: print(compiled.stderr.decode('utf-8', errors='replace')[-3000:])
    compiled.check_returncode()
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    if mutation:
        assert result.returncode and 'FAIL ' in result.stderr, (mutation, result.stdout, result.stderr)
        return mutation
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
        test_startup_ready.run(folder)
        rows['mutations_killed'] = [run(ROOT, True, folder, name, name)
                                    for name in ('early_setup', 'missing_data', 'sim_fail')]
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
