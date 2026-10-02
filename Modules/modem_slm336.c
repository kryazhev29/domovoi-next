/* Доработка: Кряжев А., 2026. Только таблица команд и ответов SLM336. */
#include "comm_modem.h"
#include "comm_timeouts.h"
static const char commands[] =
  "\0"
  "GMM\0"
  "E0\0"
  "CMEE=1\0"
  "IFC=2,2\0"
  "CLIP=1\0"
  "CMER=3,0,0,0\0"
  "CREG=0\0"
  "CGREG=0\0"
  "CEREG=0\0"
  "QURCCFG=\"urcport\"\0"
  "QURCCFG=\"urcport\",\"uart1\"\0"
  "CPIN?\0"
  "QCFG=\"nwscanmode\"\0"
  "QCFG=\"nwscanmode\",\001,1\0"
  "\011\0"
  "CIMI\0"
  "QICSGP=1,1,\"\002\",\"\003\",\"\004\",\010\0"
  "QIACT=1\0"
  "QIOPEN=1,0,\"TCP\",\"\005\",\006,0,0\0"
  "QIRD=0,\007\0";
static const comm_modem_step_t steps[] = {
  {0, COMM_T_AT_SIMPLE, 6, 7, (SLM_PHASE_SETUP << 4) | MODEM_READY},
  {1, COMM_T_AT_SIMPLE, 7, 7, (SLM_PHASE_SETUP << 4) | MODEM_MODEL},
  {5, COMM_T_AT_SIMPLE, 8, 8, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {8, COMM_T_AT_SIMPLE, 9, 9, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {15, COMM_T_AT_SIMPLE, 10, 10, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {23, COMM_T_AT_SIMPLE, 11, 11, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {30, COMM_T_AT_SIMPLE, 12, 12, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {43, COMM_T_AT_SIMPLE, 13, 13, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {50, COMM_T_AT_SIMPLE, 14, 14, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {58, COMM_T_AT_SIMPLE, 15, 15, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {66, COMM_T_AT_SIMPLE, 16, 17, (SLM_PHASE_SETUP << 4) | MODEM_PORT},
  {84, COMM_T_AT_SIMPLE, 17, 17, (SLM_PHASE_SETUP << 4) | MODEM_OK},
  {110, COMM_T_SIM, 18, 18, (SLM_PHASE_SIM << 4) | MODEM_SIM},
  {116, COMM_T_AT_SIMPLE, 19, 20, (SLM_PHASE_NETWORK << 4) | MODEM_RAT},
  {134, COMM_T_AT_SIMPLE, 20, 20, (SLM_PHASE_NETWORK << 4) | MODEM_OK},
  {156, COMM_T_AT_SIMPLE, 21, 22, (SLM_PHASE_NETWORK << 4) | MODEM_NETWORK},
  {158, COMM_T_AT_SIMPLE, 22, 22, (SLM_PHASE_PDP << 4) | MODEM_IMSI},
  {163, COMM_T_AT_SIMPLE, 23, 23, (SLM_PHASE_PDP << 4) | MODEM_OK},
  {188, COMM_T_QIACT, SLM_STEP_COUNT, SLM_STEP_COUNT, (SLM_PHASE_PDP << 4) | MODEM_PDP},
};
typedef char command_offsets_fit[(sizeof(commands) <= UINT8_MAX + 1u) ? 1 : -1];
typedef char all_steps_present[(sizeof(steps) / sizeof(steps[0]) == SLM_ACTIVATE + 1u - SLM_AT_READY) ? 1 : -1];
static const comm_modem_profile_t slm_profile = {
  commands, steps, "SLM336", "+QIURC: \"recv\",\005", "+QIURC: \"closed\",\005",
  "+QIURC: \"pdpdeact\",\005", "+QIOPEN:\005,\005", "+QURCCFG: \"urcport\",",
  "\"uart1\"", "+QCFG: \"nwscanmode\",\005",
  {"CREG?", "CEREG?"}, {"+CREG:\005,\005|", "+CEREG:\005,\005|"}, 2, COMM_AT_SLM_QIRD, 196, 223
};
bool modem_slm336_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  return comm_modem_start(&slm_profile, settings, handlers, now_ms);
}
