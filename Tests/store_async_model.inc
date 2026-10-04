
static unsigned ticks, waits, held, max_reads, step_reads, async_fail;
static uint32_t async_address;
static uint16_t async_length;
static const uint8_t *async_data;
bool w25_async_start(uint32_t a, const uint8_t *p, uint16_t n, uint32_t now) {
 (void)now; if(held) return false;
 async_address=a; async_data=p; async_length=n; held=1; waits=3; return true;
}
uint8_t w25_async_poll(uint32_t now) {
 (void)now; CHECK(held); if(waits) { waits--; return 1; }
 held=0;
 if(async_fail){async_fail=0;return 3;}
 return (async_data ? w25_write_data(async_address,(uint8_t*)async_data,async_length) : w25_erase_sector(async_address)) ? 2 : 3;
}
bool w25_async_active(void) {return held!=0;}
