// Доработка: Кряжев А., 2026
// Диагностические ответы USB 111 и 112; состав полей задаёт протокол.
// Снимок снимается на запрос; CRC и заголовок формирует общий модуль packet.
// Диагностика не меняет постоянные данные и исключается из профиля заказчика.
#include "user_logic_internal.h"
#include "prof.h"
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_recover.h"
#include "../../Modules/comm_uart.h"
#include "../../Modules/comm_modem.h"
#if DOMOVOI_DIAG
// Прочитать окрашенный свободный стек; реализация принадлежит main.c.
uint32_t stack_free_bytes(void);

// Снять диагностические поля USB для ответа 111.
void user_send_usbDiag(uint8_t uniqId)
{
  uint32_t v[58];
  uint8_t send_buffer[8 + 3 + sizeof(v)];
  uint16_t len = 3 + sizeof(v);

  v[0] = 5;
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

  send_buffer[6] = 0;
  send_buffer[7] = uniqId;
  send_buffer[8] = USB_DIAG_COMMAND;
  memcpy(&send_buffer[9], (uint8_t*)v, sizeof(v));
  packet_send(send_buffer, len, CH_USB);
}

// Снять диагностические поля модема для ответа 112.
void user_send_commDiag(uint8_t uniqId)
{
  uint32_t v[25 + SLM_STEP_COUNT];
  uint8_t send_buffer[8 + 3 + sizeof(v)];
  uint16_t len = 3 + sizeof(v);
  const slm_status_t *s = comm_modem_status();
  const slm_diag_t *d = comm_modem_diag();
  comm_uart_stats_t u;
  uint32_t now = timers_getMillis();
  comm_uart_stats(&u);
  v[0] = 1; v[1] = now;
  v[2] = s->state | (uint32_t)s->step << 8 | (uint32_t)s->phase << 16 | (uint32_t)s->reason << 24;
  v[3] = s->modem_code | (uint32_t)s->at_fault << 16; v[4] = s->completed_steps;
  v[5] = comm_channel_diag(); v[6] = comm_server_diag(); v[7] = comm_recover_diag(now, &v[8]);
  v[9] = u.errors; v[10] = u.overflow; v[11] = u.ore; v[12] = u.fe; v[13] = u.ne;
  v[14] = d->failure_count | (uint32_t)d->failure_next << 8 | (uint32_t)d->last_code << 16;
  for (uint8_t i = 0; i < 10u; i++)
    v[15 + i] = d->failure_step[i] | (uint32_t)d->failure_reason[i] << 8 | (uint32_t)d->failure_seconds[i] << 16;
  for (uint8_t i = 0; i < SLM_STEP_COUNT; i++) v[25 + i] = d->steps[i];
  send_buffer[6] = 0; send_buffer[7] = uniqId; send_buffer[8] = USB_COMM_DIAG_COMMAND;
  memcpy(&send_buffer[9], (uint8_t*)v, sizeof(v));
  packet_frame(send_buffer, len);
  prof_send_comm(send_buffer, len + 8);
}
#endif
