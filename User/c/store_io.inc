// Подшаги STORE для ID и ручного образа; источник удержан до verify.
bool store_id_begin(uint32_t address, const cellID_s *cell)
{
  flash_exec_args_t args = { NULL, FS_ID };
  if (!flash_exec_begin(FX_STORE, &args)) return false;
  flash_job.data.storeio.address = address;
  memcpy(flash_job.data.storeio.data, cell, sizeof(*cell));
  return true;
}
void store_access_reply(uint8_t channel, uint8_t uniq)
{
  if (flash_job.kind != FX_STORE || flash_job.step != FS_ID) return;
  flash_job.data.storeio.reply = 1;
  flash_job.data.storeio.channel = channel;
  flash_job.data.storeio.uniq = uniq;
  flash_job.data.storeio.epoch = usb_bus_resets;
}
static uint8_t store_id_chunk(const void *context, uint16_t offset, uint8_t *data)
{
  const store_io_s *p = context;
  uint32_t address = (p->address & ~4095u) + offset;
  if (!w25_read_data(address, data, 64)) return 0;
  if (p->address >= address && p->address < address + 64)
    memcpy(data + p->address - address, p->data, 32);
  return 1;
}
bool store_manual_begin(const uint8_t *data, uint16_t length, uint8_t uniq)
{
  if (!flash_exec_idle()) return false;
  uint8_t mode = (uint8_t)boot_manual_prepare(data, length);
  if (!mode) return false;
  uint8_t *source = work_acquire(length, WORK_FW_DATA);
  if (!source) return false;
  memcpy(source, data, length);
  flash_exec_args_t args = { source, FS_MANUAL };
  if (!flash_exec_begin(FX_STORE, &args)) {
    work_release(source, WORK_FW_DATA);
    return false;
  }
  store_io_s *p = &flash_job.data.storeio;
  p->mode = mode;
  p->length = length;
  p->uniq = uniq;
  p->epoch = usb_bus_resets;
  p->address = mode == 1 ? BOOT_CANDIDATE : BOOT_CANDIDATE + rt_get32(source + 1);
  p->end = p->address + (mode == 1 ? MANUAL_BOOT_SIZE : length - 5u);
  return true;
}
static void store_io_done(bool ok)
{
  store_io_s *p = &flash_job.data.storeio;
  uint8_t step = flash_job.step;
  uint8_t outputs = p->outputs, type = p->type, count = p->count, started = p->started;
  uint8_t reply = p->reply, channel = p->channel, uniq = p->uniq;
  uint16_t event = p->event, length = p->length;
  uint32_t epoch = p->epoch;
  uint8_t *source = p->source;
  char id[11];
  memcpy(id, ((cellID_s *)p->data)->ID, 10);
  id[10] = 0;
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  if (step == FS_ID) {
    if (ok && outputs && !unblock_active) {
      server_out_event = (logControl_e)event;
      user_access_complete(outputs, (codeType_e)type, id, count, started);
    }
    if (reply && (channel != CH_USB || epoch == usb_bus_resets)) {
      store.cmd = 8;
      store.channel = channel;
      store.uniq = uniq;
      store.quiet = 0;
      store.code = ok ? CMD_SUCCESS : CMD_RUNERROR;
      store_phase_set(US_REPLY);
    }
    return;
  }
  uint8_t response[8];
  boot_manual_ready(source, length, ok, response);
  work_release(source, WORK_FW_DATA);
  if (epoch == usb_bus_resets)
    user_send_anyData(MANUAL_BOOT_COMMAND, response, sizeof(response), CH_USB, uniq);
}
// STORE владеет источником до verify; вложенный журнал не создаёт вторую работу.
static void store_io_poll(uint32_t now)
{
  store_io_s *p = &flash_job.data.storeio;
  uint8_t result;
  if (flash_job.step == FS_BOOT) {
    app_boot_request_poll(now);
    return;
  }
  if (flash_job.step == FS_ID) {
    if (!p->mode) {
      idindex_runtime_suspend();
      if (!sj_runtime_begin(p->address & ~4095u, store_id_chunk, p)) {
        store_io_done(false);
        return;
      }
      p->mode = 1;
      return;
    }
    result = sj_runtime_poll(now);
    if (result == 1) return;
    sj_runtime_reset();
    if (result != 2) idcrc_unknown();
    store_io_done(result == 2);
    return;
  }
  if (flash_job.step != FS_MANUAL) return;
  if (p->epoch != usb_bus_resets && !p->waiting) {
    store_io_done(false);
    return;
  }
  if (p->waiting) {
    result = w25_async_poll(now);
    if (result == 1) return;
    p->waiting = 0;
    if (result != 2) {
      store_io_done(false);
      return;
    }
    p->position += p->mode == 1 ? 4096 : p->count;
    return;
  }
  if (p->address + p->position >= p->end) {
    store_io_done(true);
    return;
  }
  uint32_t address = p->address + p->position;
  if (p->mode == 1) result = w25_async_start(address, NULL, 0, now);
  else {
    uint32_t n = p->end - address;
    if (n > 64) n = 64;
    if (n > 256u - (address & 255u)) n = 256u - (address & 255u);
    p->count = (uint8_t)n;
    result = w25_async_start(address, p->source + 5 + p->position, (uint16_t)n, now);
  }
  if (!result) {
    store_io_done(false);
    return;
  }
  p->waiting = 1;
}
