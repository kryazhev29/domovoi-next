// Доработка: Кряжев А., 2026
// Приём SLDN: поиск сигнатуры, ожидание длины, одна CRC и выполнение команды.
// Плохие длина и CRC сдвигают поиск на байт; неполный кадр живёт не более 30 с.
// Завершение QIRD разрешает разбор; новая TCP-сессия полностью сбрасывает RAM-буфер.
#include "user_logic_internal.h"
#include "fast_kernels.h"
#include "../table_runtime.h"
#include "photo_runtime.h"
enum { SERVER_RX_CAPACITY = 2048u, SERVER_RX_TIMEOUT_MS = 30000u, SERVER_PHOTO_WAIT_MS = 1000u };
static uint8_t server_rx_buffer[SERVER_RX_CAPACITY];
static uint16_t server_rx_count, server_rx_ready;
static uint32_t server_rx_last, server_rx_epoch;
static uint8_t server_rx_busy, server_rx_pending;

// Отменить кадры старого поколения TCP и очистить буфер.
static void server_rx_reset(void)
{
  ++server_rx_epoch;
  server_rx_count = server_rx_ready = server_rx_pending = 0;
}

// Удалить незавершённый кадр по пределу ожидания 30 секунд.
void server_rx_begin_pass(void)
{
  if (!server_rx_busy && !server_rx_ready && server_rx_count &&
      (uint32_t)(timers_getMillis() - server_rx_last) >= SERVER_RX_TIMEOUT_MS)
    server_rx_reset();
}

// Убрать использованный префикс буфера одним сдвигом.
static void server_rx_consume(uint16_t n)
{
  server_rx_count -= n;
  memmove(server_rx_buffer, server_rx_buffer + n, server_rx_count);
}

// Найти целый корректный SLDN, сохранив неполный хвост.
static void server_rx_scan(void)
{
  uint16_t skipped = 0, body, total, crc;
  if (server_rx_ready || server_rx_busy || server_rx_pending) return;
  while (skipped < server_rx_count) {
    uint16_t left = server_rx_count - skipped;
    uint8_t *p = server_rx_buffer + skipped;
    uint16_t found = (uint16_t)fast_sldn(p, left);
    skipped += found;
    left -= found;
    p += found;
    if (left < 6u) break;
    body = rt_get16(p + 4);
    if (body < 3u || body > SERVER_RX_CAPACITY - 8u) { ++skipped; continue; }
    total = body + 8u;
    if (left < total) break;
    crc = rt_get16(p + total - 2u);
    if (crc16_calc(p + 4, body + 2u) != crc) { ++skipped; continue; }
    server_rx_ready = total;
    server_rx_last = timers_getMillis(); // Теперь срок ожидания полного кадра, не его сборки.
    break;
  }
  if (skipped) server_rx_consume(skipped);
}

// Выполнить один готовый кадр и сохранить хвост потока.
void server_rx_process(void)
{
  uint16_t total;
  uint32_t epoch;
  bool ready;
  if (server_rx_busy || server_rx_pending) return;
  server_rx_scan();
  if (!server_rx_ready || comm_tx_busy()) return;
  ready = user_command_ready(server_rx_buffer + 4, false);
  if (!ready && (server_rx_buffer[8] != 21 ||
      (uint32_t)(timers_getMillis() - server_rx_last) < SERVER_PHOTO_WAIT_MS)) return;
  total = server_rx_ready;
  epoch = server_rx_epoch;
  server_rx_busy = 1;
  if (ready) user_parse_comand(server_rx_buffer + 4, CH_SERVER);
  else user_send_answer_comand(21, CMD_RUNERROR, CH_SERVER, server_rx_buffer[7]);
  server_rx_busy = 0;
  if (epoch != server_rx_epoch) return;
  server_rx_ready = 0;
  server_rx_consume(total);
  server_rx_scan();
}

// Вернуть свободное место для QIRD-backpressure.
uint16_t user_comm_rx_capacity(void)
{
  return SERVER_RX_CAPACITY - server_rx_count;
}

// Принять очередную часть ещё не завершённого QIRD; разбор — после user_comm_rx_done.
uint16_t user_comm_rx(const uint8_t *data, uint16_t count)
{
  uint16_t capacity;
  server_rx_begin_pass();
  capacity = user_comm_rx_capacity();
  if (count > capacity) count = capacity;
  server_rx_pending = 1;
  if (count) {
    if (!server_rx_count) server_rx_last = timers_getMillis();
    memcpy(server_rx_buffer + server_rx_count, data, count);
    server_rx_count += count;
  }
  return count;
}

// Разрешить разбор законченного QIRD либо отбросить его при ошибке.
void user_comm_rx_done(uint16_t count, uint8_t reason)
{
  (void)count;
  server_rx_pending = 0;
  if (reason) server_rx_reset();
}

// Очистить буфер и неподтверждённые серверные ответы при смене TCP.
void user_comm_session(bool open)
{
  server_rx_reset();
  flash_exec_quiet(CH_SERVER);
  answer_reply_drop();
  device.connectionhandle = 0; device.registredOnServer = 0;
  /* Поколение TCP отменяет неподтверждённый пакет; сохранённые курсоры и USB не трогаем. */
  temp_unacked[1] = 0; store.seen &= 5u;
  if (!open) photo_connection_lost(); /* «подключён» пишет user_comm_link после регистрации */
}
