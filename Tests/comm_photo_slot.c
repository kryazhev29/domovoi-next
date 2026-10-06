/* Доработка: Кряжев А., 2026. Слот для прежних ожиданий модели фото.
   Буфер имитирует серверный транспорт, физическое соединение не создаётся.
   fail_alloc этой модели означает отказ выдачи занятого слота серверу. */
static uint8_t photo_tx_slot[1040];
// Выдать серверный слот модели или вернуть отказ занятого слота.
static uint8_t *comm_tx_acquire(uint16_t *capacity)
{
  *capacity = sizeof(photo_tx_slot);
  return fail_alloc ? NULL : photo_tx_slot;
}
// Передать пакет из серверного слота эталонной модели.
static bool comm_tx_commit(uint16_t count)
{ return count <= sizeof(photo_tx_slot) && connection_send(device.connectionhandle, photo_tx_slot, count) == CE_OK; }
// Имитировать освобождение статического серверного слота модели.
static void comm_tx_release(void) {}
// Сообщить о занятости серверного транспорта модели.
static bool comm_tx_busy(void) { return !connection_datasended(device.connectionhandle); }
// Сообщить о готовности серверного соединения модели.
static bool comm_channel_ready(void) { return connection_state(device.connectionhandle) == CE_CONNECTED; }
