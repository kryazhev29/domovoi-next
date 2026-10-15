"""Границы готовности SLM336: реальные UART/AT, перестановки URC и постоянные отказы."""
import subprocess
from pathlib import Path
import comm_slm_host as modem


def run(folder):
    code = modem.unit().replace('\r\n', '\n').replace('int main(void)', 'int old_main(void)')
    code = code.replace('  unsigned step = slm.attempt.status.step;', r'''
  unsigned step = slm.attempt.status.step;
  if (!strcmp(sim_sent,"AT")) {
    sim_reply("OK\r\n",30); return;
  }
  if (step == boot_step && sim_now < boot_until) {
    if (boot_last) CHECK(sim_now-boot_last >= 230);
    boot_last=sim_now; sim_reply(boot_reply,30); return;
  }
''')
    code = code.replace('static void sim_tick(void)\n{', r'''static void sim_tick(void)
{
  if (power_events>=5) {
    unsigned age=sim_now-power_trace[4].time;
    if (!boot_at_sent && age>=boot_at) { boot_at_sent=1; sim_feed("AT READY\r\n"); }
    if (!boot_sim_sent && age>=boot_sim) { boot_sim_sent=1; sim_feed("+SIM1 Ready\r\n"); }
  }
''')
    prefix = '''static unsigned boot_step, boot_until, boot_last, boot_at, boot_sim, boot_at_sent, boot_sim_sent;
static const char *boot_reply;
'''
    suffix = r'''
static void boot_reset(unsigned step,const char *reply,unsigned until)
{
  sim_reset(); boot_step=step; boot_reply=reply; boot_until=until; boot_last=0;
  boot_at=2500; boot_sim=4100; boot_at_sent=boot_sim_sent=0;
  test_config.modem_type=255; sim_port_change=sim_rat_change=1;
  sim_start();
}
int main(void)
{
  const unsigned setup[]={SLM_MODEL,SLM_IMEI,SLM_ECHO,SLM_CMEE,SLM_IFC,SLM_CLIP,
    SLM_CMER,SLM_CREG_URC,SLM_CGREG_URC,SLM_CEREG_URC,SLM_URC_PORT,SLM_URC_SET,SLM_RAT_QUERY,SLM_RAT_SET};
  const char *failures[]={"ERROR\r\n","+CME ERROR: 14\r\n","OK\r\n"};
  for (unsigned i=0;i<sizeof(setup)/sizeof(setup[0]);++i) for (unsigned j=0;j<3;++j) {
    boot_reset(setup[i],failures[j],7800); sim_until(20000);
    CHECK(comm_modem_ready() && !slm.diag.failure_count);
  }
  /* Сервисы не появились: неизменный общий срок от ключа, включая повторы. */
  for (unsigned j=0;j<3;++j) {
    boot_reset(SLM_IMEI,failures[j],UINT32_MAX); sim_until(20000);
    CHECK(slm.attempt.status.state==SLM_FAILED && slm.attempt.status.step==SLM_IMEI);
    CHECK(slm.attempt.now-power_trace[4].time==10000);
  }
  const char *sim_bad[]={"+CPIN: NOT READY\r\nOK\r\n","+CME ERROR: 10\r\n",
    "+CME ERROR: 13\r\n","+CME ERROR: 14\r\n","OK\r\n","ERROR\r\n",
    "+CME ERROR: SIM not inserted\r\n"};
  for (unsigned i=0;i<sizeof(sim_bad)/sizeof(sim_bad[0]);++i) {
    boot_reset(SLM_SIM,sim_bad[i],UINT32_MAX); boot_sim=UINT32_MAX; sim_until(20000);
    CHECK(slm.attempt.status.reason==SLM_ERR_NO_SIM);
    CHECK(slm.attempt.now-power_trace[4].time==7500);
  }
  const char *locked[]={"+CPIN: SIM PIN\r\nOK\r\n","+CPIN: SIM PUK\r\nOK\r\n",
    "+CME ERROR: 11\r\n","+CME ERROR: 12\r\n"};
  for (unsigned i=0;i<4;++i) {
    boot_reset(SLM_SIM,locked[i],UINT32_MAX); sim_until(20000);
    CHECK(slm.attempt.status.reason==(i%2 ? SLM_ERR_PUK : SLM_ERR_PIN));
    CHECK(slm.attempt.now-power_trace[4].time<7500);
  }
  /* URC потерян; UART OK не даёт выйти в GMM раньше 3,5 с. */
  boot_reset(SLM_IMEI,"OK\r\n",8200); boot_at=UINT32_MAX;
  while (slm.attempt.status.step!=SLM_MODEL) sim_tick();
  CHECK(sim_now-power_trace[4].time>=3500 && sim_now-power_trace[4].time<3700);
  sim_until(20000); CHECK(comm_modem_ready() && !slm.diag.failure_count);
  /* После позднего user_init срок не начинается заново. */
  boot_reset(SLM_IMEI,"OK\r\n",UINT32_MAX);
  for (sim_now=0;sim_now<16000;sim_now+=10) comm_power_v5_poll(sim_now);
  comm_modem_poll(sim_now); comm_modem_poll(sim_now+10);
  CHECK(slm.attempt.status.reason==SLM_ERR_TIMEOUT);
  puts("PASS startup ready: setup 42, deadlines 3, SIM absent 7, PIN/PUK 4, lost URC, late main");
  return 0;
}
'''
    source, binary = Path(folder) / 'ready.c', Path(folder) / 'ready.exe'
    source.write_text(prefix + code + suffix, encoding='utf-8')
    subprocess.run([modem.ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=120)
