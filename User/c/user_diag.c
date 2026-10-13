// Доработка: Кряжев А., 2026
// Диагностические ответы USB 111 и 112; состав полей задаёт протокол.
// Снимок снимается на запрос; CRC и заголовок формирует общий модуль packet.
// Команда 112 доступна в рабочих сборках, внутренние счётчики — только в 111 под DIAG.
#include "user_logic_internal.h"
#include "prof.h"
#include "runtime_internal.h"
#include "../../Modules/comm_info.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_recover.h"
#include "../../Modules/comm_uart.h"
#include "../../Modules/comm_modem.h"
#include "../../Modules/comm_sms.h"
#if DOMOVOI_DIAG
// Прочитать окрашенный свободный стек; реализация принадлежит main.c.
uint32_t stack_free_bytes(void);

// Снять диагностические поля USB для ответа 111.
void user_send_usbDiag(uint8_t uniqId)
{
  uint32_t v[USB_DIAG_VALUES];
  uint8_t send_buffer[8 + 3 + sizeof(v)];
  uint16_t len = 3 + sizeof(v);

  v[0] = 7;
  v[1] = device.worktime;
  v[2] = timers_getMillis();
  usb_diag_fill(&v[3]);
  v[26] = loop_max_ms;
  v[27] = loop_stalls;
  v[28] = mem_config_errors;
  v[29] = 0;
  v[30] = 0;
  v[31] = 0;
  v[32] = fault_record[0] == FAULT_RECORD_MAGIC;
  usb_diag_fill_regs(&v[33]);
  usb_diag_fill_errors(&v[51]);
  usb_diag_fill_addr(&v[53]);
  v[57] = stack_free_bytes();
  comm_uart_stats_t u;
  comm_uart_stats(&u);
  v[58] = u.errors;
  v[59] = u.overflow;
  v[60] = u.ore;
  v[61] = u.fe;
  v[62] = u.ne;
  v[63] = comm_channel_diag();
  v[64] = comm_server_diag();
  v[65] = comm_recover_diag(timers_getMillis(), &v[66]);
  comm_sms_diag(&v[67]);

  send_buffer[6] = 0;
  send_buffer[7] = uniqId;
  send_buffer[8] = USB_DIAG_COMMAND;
  memcpy(&send_buffer[9], (uint8_t*)v, sizeof(v));
  packet_frame(send_buffer, len);
  prof_send_comm(send_buffer, len + 8);
}

#endif
// Формат сервисного ответа: docs/protocol/usb_link_diag.md.
void user_send_commDiag(uint8_t uniqId)
{
  uint8_t b[96] = {0};
  const slm_diag_t *d = comm_modem_diag();
  const comm_info_t *info = comm_info();
  uint32_t wait;
  b[7] = uniqId;
  b[8] = USB_COMM_DIAG_COMMAND;
  b[9] = 4;
  b[91] = b[92] = b[93] = 255;
  b[10] = comm_recover_link(timers_getMillis(), &wait);
  b[12] = comm_modem_ready() ? info->csq : 99u;
  if (comm_modem_ready()) {
    b[13] = info->net;
    b[91] = info->rsrp; b[92] = info->rsrq; b[93] = info->sinr;
    memcpy(b + 15, info->operator_code, 7);
  }
  b[14] = comm_channel_probe_result();
  rt_put32(b + 22, wait);
  for (uint8_t i = 0; i < SLM_PHASE_COUNT; ++i) rt_put32(b + 26 + 4u * i, d->phases[i]);
  b[50] = d->failure_count;
  uint8_t slot = d->failure_count == 10u ? d->failure_next : 0u;
  for (uint8_t i = 0; i < d->failure_count; ++i) {
    rt_put32(b + 51 + 4u * i, d->failures[slot]);
    slot = slot == 9u ? 0 : (uint8_t)(slot + 1u);
  }
  packet_send(b, sizeof(b) - 8u, CH_USB);
}
