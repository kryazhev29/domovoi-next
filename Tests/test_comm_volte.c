/* Доработка: Кряжев А., 2026. VoLTE по настройке на модели модема.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
static void volte_tests(void)
{
  /* Настройка 1, модем 0: запрос и запись один раз за запуск. */
  volte_flag=1; volte_modem=0; server_reset(); advance(1000);
  CHECK(volte_queries==1 && volte_sets==1 && volte_modem==1 && comm_server_online());
  advance(20000); CHECK(volte_queries==1 && volte_sets==1);
  /* Новый запуск модема: снова запрос, запись не нужна. */
  CHECK(comm_modem_restart(false,sim_now)); rc_session();
  for (unsigned i=0;volte_queries<2 && i<3000;++i) transport_tick(10);
  advance(1000); CHECK(volte_queries==2 && volte_sets==1);
  /* Настройку сменили с сервера: модем выключается без перезапуска. */
  volte_flag=0; advance(500); CHECK(volte_sets==2 && volte_modem==0);
  /* Совпадает - только запрос. */
  volte_flag=0; volte_modem=0; server_reset(); advance(1000); CHECK(volte_queries==1 && !volte_sets);
  /* Модем не знает команду: в этом запуске больше не спрашиваем. */
  volte_flag=1; server_reset(); volte_error=1; volte_queries=volte_sets=0; comm_modem_restart(false,sim_now); rc_session();
  advance(20000); CHECK(volte_queries==1 && !volte_sets);
  /* Во время вызова VoLTE не трогается. */
  volte_flag=0; volte_modem=0; server_reset(); advance(1000); volte_flag=1;
  sim_feed("RING\r\n"); advance(20); CHECK(comm_call_active()); unsigned sets=volte_sets;
  advance(500); CHECK(volte_sets==sets);
  /* Тихое завершение ещё принадлежит звонку, хотя AT уже свободен. */
  for (unsigned i=0;comm_channel_call_pending() && i<1000;++i) transport_tick(10);
  CHECK(!comm_channel_call_pending() && comm_call_active());
  advance(50); CHECK(volte_sets==sets);
  advance(9000); CHECK(!comm_call_active() && volte_sets==sets+1);
  volte_flag=0; volte_modem=0;
  puts("comm_volte: query once per start, set on mismatch, runtime change, unsupported, not during a call PASS");
}
