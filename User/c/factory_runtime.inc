// Заводская очистка: STORE до конца A/B, ответ — после verify.
// Ответ хранится отдельно от принятой работы и ждёт готовности транспорта.
enum { FACTORY_MARK = 1, FACTORY_ERASE, FACTORY_SAVE = 4, FACTORY_ERROR };
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
  factory_job_s *p = &flash_job.data.factory;
  if (!counter_flush() || !flash_exec_begin(FX_STORE, NULL)) return false;
  photo_storage_cancel();
  upload_reset(10);
  upload_reset(9);
  sched16_reset_mark(false);
  p->phase = FACTORY_MARK;
  p->block = MB_ID;
  store_phase_set(US_FACTORY);
  if (!store_snapshot()) p->phase = FACTORY_ERROR;
  return true;
}
// Неполный сектор очищается через журнал, соседние данные сохраняются.
static uint8_t factory_chunk(const void *context, uint16_t offset, uint8_t *data)
{
  factory_job_s *p = &flash_job.data.factory;
  (void)context;
  uint32_t address = p->address + offset;
  if (!w25_read_data(address, data, 64)) return 0;
  for (uint16_t i = 0; i < 64; ++i) {
    if (address + i >= mem[p->block].start && address + i < p->end) data[i] = 255;
  }
  return 1;
}
static void factory_done(bool ok)
{
  mem_config_reset();
  store_phase_set(0);
  if (!ok) mem_storage_ready = false;
  flash_exec_finish(ok ? FX_OK : FX_ERROR);
  flash_exec_take_result();
  if (ok) {
    user_comm_boot();
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
  factory_job_s *p = &flash_job.data.factory;
  uint8_t result;
  if (p->phase == FACTORY_ERROR) {
    factory_done(false);
    return;
  }
  if (p->phase == FACTORY_MARK || p->phase == FACTORY_SAVE) {
    result = mem_config_result();
    if (result == 1) return;
    if (result != 2) {
      factory_done(false);
      return;
    }
    if (p->phase == FACTORY_SAVE) {
      factory_done(true);
      return;
    }
    mem_config_reset();
    p->phase = FACTORY_ERASE;
  }
  if (p->pending) {
    result = p->pending == 1 ? w25_async_poll(now) : sj_runtime_poll(now);
    if (result == 1) return;
    if (p->pending == 2) sj_runtime_reset();
    p->pending = 0;
    if (result != 2) {
      factory_done(false);
      return;
    }
    p->address += 4096;
    return;
  }
  if (!p->address || p->address >= p->end) {
    if (p->address) ++p->block;
    if (p->block == MB_CONFIG) ++p->block;
    if (p->block > MB_PHOTO) {
      // Существующая WORK_CONFIG сохраняет метаданные до единственной очистки device.
      device_s *saved = work_acquire(sizeof(device), WORK_CONFIG);
      if (!saved) { p->phase = FACTORY_ERROR; return; }
      memcpy(saved, &device, sizeof(device));
      user_factory_variables(saved);
      work_release(saved, WORK_CONFIG);
      sched16_reset_mark(true);
      user_default_values();
      p->phase = store_snapshot() ? FACTORY_SAVE : FACTORY_ERROR;
      return;
    }
    p->address = mem[p->block].start & ~4095u;
    p->end = mem[p->block].start + mem[p->block].size;
    if (!mem[p->block].size) {
      ++p->block;
      p->address = 0;
      return;
    }
  }
  bool partial = p->address < mem[p->block].start || p->address + 4096 > p->end;
  bool accepted = partial ? sj_runtime_begin(p->address, factory_chunk, NULL) :
    w25_async_start(p->address, NULL, 0, now);
  if (!accepted) {
    factory_done(false);
    return;
  }
  p->pending = partial ? 2 : 1;
}
