// Заводская очистка: STORE до конца A/B, ответ — после verify.
// Ответ хранится отдельно от принятой работы и ждёт готовности транспорта.
static void factory_reply(void)
{
  if (factory.reply != 2) return;
  if (factory.channel == CH_USB && factory.epoch != usb_bus_resets) {
    factory.reply = 0;
    return;
  }
  if (factory.channel == 2) {
    if (!user_comm_sms(factory.number, factory.code ? "DEFAULT: Error" : "DEFAULT: Complete")) return;
  } else {
    if (factory.channel == CH_SERVER && comm_tx_busy()) return;
    user_send_answer_comand(3, (cmdCode_e)factory.code, (dataChanel_e)factory.channel, factory.uniq);
  }
  factory.reply = 0;
}
bool store_factory_request(uint8_t channel, uint8_t uniq, uint8_t delay, const char *number)
{
  if (factory.reply || (flash_job.kind == FX_STORE && flash_job.step == US_FACTORY)) return false;
  factory.channel = channel;
  factory.uniq = uniq;
  factory.epoch = usb_bus_resets;
  factory.reply = 1;
  if (number) {
    strncpy(factory.number, number, sizeof(factory.number) - 1);
    factory.number[sizeof(factory.number) - 1] = 0;
  }
  device.timeToDefault = delay ? delay : 1;
  return true;
}
bool store_factory_begin(void)
{
  if (!flash_exec_begin(FX_STORE, NULL)) return false;
  photo_storage_cancel();
  upload_reset(10);
  upload_reset(9);
  sched16_reset_mark(false);
  flash_job.data.factory.phase = 1;
  flash_job.data.factory.block = MB_ID;
  flash_job.data.factory.address = 0;
  flash_job.data.factory.pending = 0;
  store_phase_set(US_FACTORY);
  if (!store_snapshot()) flash_job.data.factory.phase = 5;
  return true;
}
// Неполный сектор очищается через журнал, соседние данные сохраняются.
static uint8_t factory_chunk(const void *context, uint16_t offset, uint8_t *data)
{
  (void)context;
  uint32_t address = flash_job.data.factory.address + offset;
  if (!w25_read_data(address, data, 64)) return 0;
  for (uint16_t i = 0; i < 64; ++i) {
    if (address + i >= mem[flash_job.data.factory.block].start && address + i < flash_job.data.factory.end) data[i] = 255;
  }
  return 1;
}
static void factory_done(bool ok)
{
  if (ok) user_factory_variables((device_s *)(mem_config_snapshot() + sizeof(config)));
  mem_config_reset();
  flash_job.data.factory.phase = 0;
  store_phase_set(0);
  if (!ok) mem_storage_ready = false;
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  if (ok) {
    id_cells_end = 0;
    id_used_bytes = 0;
    idindex_scanned(0, 0);
    photo_storage_reset();
  }
  factory.code = ok ? CMD_SUCCESS : CMD_RUNERROR;
  if (factory.reply) factory.reply = 2;
}
// Один квант A/B, журнала или стирания; успех только после последнего verify.
static void factory_poll(uint32_t now)
{
  uint8_t result;
  if (flash_job.data.factory.phase == 5) {
    factory_done(false);
    return;
  }
  if (flash_job.data.factory.phase == 1 || flash_job.data.factory.phase == 4) {
    result = mem_config_result();
    if (result == 1) return;
    if (result != 2) {
      factory_done(false);
      return;
    }
    if (flash_job.data.factory.phase == 4) {
      factory_done(true);
      return;
    }
    mem_config_reset();
    flash_job.data.factory.phase = 2;
  }
  if (flash_job.data.factory.pending) {
    result = flash_job.data.factory.pending == 1 ? w25_async_poll(now) : sj_runtime_poll(now);
    if (result == 1) return;
    if (flash_job.data.factory.pending == 2) sj_runtime_reset();
    flash_job.data.factory.pending = 0;
    if (result != 2) {
      factory_done(false);
      return;
    }
    flash_job.data.factory.address += 4096;
    return;
  }
  if (!flash_job.data.factory.address || flash_job.data.factory.address >= flash_job.data.factory.end) {
    if (flash_job.data.factory.address) ++flash_job.data.factory.block;
    if (flash_job.data.factory.block == MB_CONFIG) ++flash_job.data.factory.block;
    if (flash_job.data.factory.block > MB_PHOTO) {
      device.FWinProgress = 0;
      device.status.readyToUpdate = 0;
      device.updateTimeout = 0;
      device.numbersCount = 0;
      device.logsCount = 0;
      device.newAddressLog = mem[MB_LOG].start + 32;
      device.tempIDsCount = 0;
      device.newAddressTempID = mem[MB_TEMPID].start;
      device.countPhoto = 0;
      sched16_reset_mark(true);
      user_default_values();
      flash_job.data.factory.phase = store_snapshot() ? 4 : 5;
      return;
    }
    flash_job.data.factory.address = mem[flash_job.data.factory.block].start & ~4095u;
    flash_job.data.factory.end = mem[flash_job.data.factory.block].start + mem[flash_job.data.factory.block].size;
    if (!mem[flash_job.data.factory.block].size) {
      ++flash_job.data.factory.block;
      flash_job.data.factory.address = 0;
      return;
    }
  }
  bool partial = flash_job.data.factory.address < mem[flash_job.data.factory.block].start || flash_job.data.factory.address + 4096 > flash_job.data.factory.end;
  bool accepted = partial ? sj_runtime_begin(flash_job.data.factory.address, factory_chunk, NULL) :
    w25_async_start(flash_job.data.factory.address, NULL, 0, now);
  if (!accepted) {
    factory_done(false);
    return;
  }
  flash_job.data.factory.pending = partial ? 2 : 1;
}
