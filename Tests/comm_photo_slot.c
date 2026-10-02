/* Доработка: Кряжев А., 2026. Слот для прежних ожиданий модели фото.
   fail_alloc этой модели означает отказ выдачи занятого слота серверу. */
static uint8_t photo_tx_slot[1040];
static uint8_t *comm_tx_acquire(uint16_t *capacity)
{
  *capacity = sizeof(photo_tx_slot);
  return fail_alloc ? NULL : photo_tx_slot;
}
static bool comm_tx_commit(uint16_t count)
{ return count <= sizeof(photo_tx_slot) && connection_send(device.connectionhandle, photo_tx_slot, count) == CE_OK; }
static void comm_tx_release(void) {}
static bool comm_tx_busy(void) { return !connection_datasended(device.connectionhandle); }
static bool comm_channel_ready(void) { return connection_state(device.connectionhandle) == CE_CONNECTED; }
