/* Доработка: Кряжев А., 2026. Обратное давление в прежней модели выгрузки. */
#ifndef CE_OK
#define CE_OK 0
#endif
#ifndef CE_BUSY
#define CE_BUSY 5
#endif
static uint8_t upload_tx_slot[1040];
static uint8_t *comm_tx_acquire(uint16_t *capacity)
{
  *capacity = sizeof(upload_tx_slot);
  return COMM_SLOT_FAILURE ? NULL : upload_tx_slot;
}
static bool comm_tx_commit(uint16_t count)
{ return count <= sizeof(upload_tx_slot) && connection_send(device.connectionhandle, upload_tx_slot, count) == CE_OK; }
static void comm_tx_release(void) {}
