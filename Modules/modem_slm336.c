/* Доработка: Кряжев А., 2026.
   Описывает команды и шаги запуска единственного SLM336 платы v5.
   Каждая строка задаёт команду, срок и два успешных перехода.
   Открытие TCP и чтение данных выполняет канал после завершения запуска.
*/
#include "comm_modem.h"
#include "comm_timeouts.h"
const char slm_tcp_command[] = "QIOPEN=1,0,\"TCP\",\"\005\",\006,0,0";
const char slm_read_command[] = "QIRD=0,\007";
const comm_modem_step_t slm_steps[] = {
  {"", COMM_T_AT_SIMPLE, SLM_MODEL, SLM_IMEI, (SLM_PHASE_SETUP << 4) | MODEM_READY},
  {"GMM", COMM_T_AT_SIMPLE, SLM_IMEI, SLM_IMEI, (SLM_PHASE_SETUP << 4) | MODEM_MODEL},
  /* IMEI нужен пакету регистрации на сервере (Протокол2, пакет 1). */
  {"GSN", COMM_T_AT_SIMPLE, SLM_ECHO, SLM_ECHO, (SLM_PHASE_SETUP << 4) | MODEM_IMEI},
  {"E0", COMM_T_AT_SIMPLE, SLM_RAT_QUERY, SLM_RAT_QUERY, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CMEE=1", COMM_T_AT_SIMPLE, SLM_IFC, SLM_IFC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"IFC=2,2", COMM_T_AT_SIMPLE, SLM_CLIP, SLM_CLIP, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CLIP=1", COMM_T_AT_SIMPLE, SLM_CMER, SLM_CMER, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CMER=3,0,0,0", COMM_T_AT_SIMPLE, SLM_CREG_URC, SLM_CREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CREG=1", COMM_T_AT_SIMPLE, SLM_CGREG_URC, SLM_CGREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CGREG=1", COMM_T_AT_SIMPLE, SLM_CEREG_URC, SLM_CEREG_URC, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CEREG=1", COMM_T_AT_SIMPLE, SLM_URC_PORT, SLM_URC_PORT, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"QURCCFG=\"urcport\"", COMM_T_AT_SIMPLE, SLM_URC_SET, SLM_SIM, (SLM_PHASE_SETUP << 4) | MODEM_PORT},
  {"QURCCFG=\"urcport\",\"uart1\"", COMM_T_AT_SIMPLE, SLM_SIM, SLM_SIM, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"CPIN?", COMM_T_SIM, SLM_IMSI, SLM_IMSI, (SLM_PHASE_SIM << 4) | MODEM_SIM},
  {"QCFG=\"nwscanmode\"", COMM_T_AT_SIMPLE, SLM_RAT_SET, SLM_CMEE, (SLM_PHASE_SETUP << 4) | MODEM_RAT},
  {"QCFG=\"nwscanmode\",\001,1", COMM_T_RAT_SET, SLM_CMEE, SLM_CMEE, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {"\011", COMM_T_AT_SIMPLE, SLM_NET_APN, SLM_APN, (SLM_PHASE_NETWORK << 4) | MODEM_NETWORK},
  {"CGCONTRDP=1", COMM_T_AT_SIMPLE, SLM_APN, SLM_APN, (SLM_PHASE_PDP << 4) | MODEM_APN_QUERY},
  /* Сразу после CPIN READY SLM336 отвечает на CIMI до 2 с (плата Алексея 20.10): срок как у SIM. */
  {"CIMI", COMM_T_SIM, SLM_NETWORK, SLM_NETWORK, (SLM_PHASE_NETWORK << 4) | MODEM_IMSI},
  {"QICSGP=1,1,\"\002\",\"\003\",\"\004\",\010", COMM_T_AT_SIMPLE, SLM_NETWORK, SLM_NETWORK, (SLM_PHASE_NETWORK << 4) | MODEM_OK},
  /* Сбросить оставшийся активным контекст перед QIACT; неактивный также даёт OK. */
  {"QIDEACT=1", COMM_T_QIDEACT, SLM_ACTIVATE, SLM_ACTIVATE, (SLM_PHASE_PDP << 4) | MODEM_ANY},
  {"QIACT=1", COMM_T_QIACT, SLM_DATA_ECHO, SLM_DATA_ECHO, (SLM_PHASE_PDP << 4) | MODEM_PDP},
  /* ATE0 отключает эхо команд; бинарное эхо QISEND выключается отдельно. */
  {"QISDE=0", COMM_T_AT_SIMPLE, SLM_STEP_COUNT, SLM_STEP_COUNT, (SLM_PHASE_PDP << 4) | MODEM_OK},
  {"CGATT=1", COMM_T_ATTACH, SLM_APN, SLM_APN, (SLM_PHASE_PDP << 4) | MODEM_OK},
};
typedef char all_steps_present[(sizeof(slm_steps) / sizeof(slm_steps[0]) == SLM_ATTACH + 1u - SLM_AT_READY) ? 1 : -1];
