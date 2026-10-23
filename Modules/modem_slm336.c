/* Доработка: Кряжев А., 2026.
   Описывает команды и шаги запуска единственного SLM336 платы v5.
   Каждая строка задаёт команду, срок и два успешных перехода.
   Открытие TCP и чтение данных выполняет канал после завершения запуска.
*/
#include "comm_modem.h"
#include "comm_timeouts.h"
const char slm_tcp_command[] = "QIOPEN=1,0,\"TCP\",\"\005\",\006,0,0";
const char slm_read_command[] = "QIRD=0,\007";
/* Команды шагов подряд через NUL в порядке slm_steps; первая пустая — голый AT. */
const char slm_commands[] =
  "\0"
  "GMM\0"
  "GSN\0"
  "E0\0"
  "CMEE=1\0"
  "IFC=2,2\0"
  "CLIP=1\0"
  "CMER=3,0,0,0\0"
  "CREG=1\0"
  "CGREG=1\0"
  "CEREG=1\0"
  "QURCCFG=\"urcport\"\0"
  "QURCCFG=\"urcport\",\"uart1\"\0"
  "CPIN?\0"
  "QCFG=\"nwscanmode\"\0"
  "QCFG=\"nwscanmode\",\001,1\0"
  "\011\0"
  "CGCONTRDP=1\0"
  "CIMI\0"
  "QICSGP=1,1,\"\002\",\"\003\",\"\004\",\010\0"
  "QIDEACT=1\0"
  "QIACT=1\0"
  "QISDE=0\0"
  "CGATT=1\0";
const comm_modem_step_t slm_steps[] = {
  {COMM_T_AT_SIMPLE, SLM_MODEL, SLM_IMEI, (SLM_PHASE_SETUP << 4) | MODEM_READY},
  {COMM_T_AT_SIMPLE, SLM_IMEI, SLM_IMEI, (SLM_PHASE_SETUP << 4) | MODEM_MODEL},
  /* IMEI нужен пакету регистрации на сервере (Протокол2, пакет 1). */
  {COMM_T_AT_SIMPLE, SLM_ECHO, SLM_ECHO, (SLM_PHASE_SETUP << 4) | MODEM_IMEI},
  {COMM_T_AT_SIMPLE, SLM_RAT_QUERY, SLM_RAT_QUERY, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_IFC, SLM_IFC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_CLIP, SLM_CLIP, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_CMER, SLM_CMER, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_CREG_URC, SLM_CREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_CGREG_URC, SLM_CGREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_CEREG_URC, SLM_CEREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_URC_PORT, SLM_URC_PORT, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_URC_SET, SLM_SIM, (SLM_PHASE_SETUP << 4) | MODEM_PORT},
  {COMM_T_AT_SIMPLE, SLM_SIM, SLM_SIM, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_SIM, SLM_IMSI, SLM_IMSI, (SLM_PHASE_SIM << 4) | MODEM_SIM},
  {COMM_T_AT_SIMPLE, SLM_RAT_SET, SLM_CMEE, (SLM_PHASE_SETUP << 4) | MODEM_RAT},
  {COMM_T_RAT_SET, SLM_CMEE, SLM_CMEE, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {COMM_T_AT_SIMPLE, SLM_NET_APN, SLM_APN, (SLM_PHASE_NETWORK << 4) | MODEM_NETWORK},
  {COMM_T_AT_SIMPLE, SLM_APN, SLM_APN, (SLM_PHASE_PDP << 4) | MODEM_APN_QUERY},
  /* Сразу после CPIN READY SLM336 отвечает на CIMI до 2 с (плата Алексея 20.10): срок как у SIM. */
  {COMM_T_SIM, SLM_NETWORK, SLM_NETWORK, (SLM_PHASE_NETWORK << 4) | MODEM_IMSI},
  {COMM_T_AT_SIMPLE, SLM_NETWORK, SLM_NETWORK, (SLM_PHASE_NETWORK << 4) | MODEM_OK},
  /* Сбросить оставшийся активным контекст перед QIACT; неактивный также даёт OK. */
  {COMM_T_QIDEACT, SLM_ACTIVATE, SLM_ACTIVATE, (SLM_PHASE_PDP << 4) | MODEM_ANY},
  {COMM_T_QIACT, SLM_DATA_ECHO, SLM_DATA_ECHO, (SLM_PHASE_PDP << 4) | MODEM_PDP},
  /* ATE0 отключает эхо команд; бинарное эхо QISEND выключается отдельно. */
  {COMM_T_AT_SIMPLE, SLM_STEP_COUNT, SLM_STEP_COUNT, (SLM_PHASE_PDP << 4) | MODEM_OK},
  {COMM_T_ATTACH, SLM_APN, SLM_APN, (SLM_PHASE_PDP << 4) | MODEM_OK},
};
typedef char all_steps_present[(sizeof(slm_steps) / sizeof(slm_steps[0]) == SLM_ATTACH + 1u - SLM_AT_READY) ? 1 : -1];
