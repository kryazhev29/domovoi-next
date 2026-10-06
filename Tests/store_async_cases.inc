/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */

static void boot_async(void) { held=0; host_boot(); memset(&cfg_job,0,sizeof(cfg_job)); mem_config_reset(); cfg_legacy_checked=0; }
static void seed_async(void) {
 memset(flash,255,sizeof(flash));
 for(unsigned i=0;i<4096;i++) olddata[i]=(uint8_t)(i*13+7);
 *(uint16_t*)(olddata+510)=crc16_calc(olddata,510);
 *(uint16_t*)(olddata+1022)=crc16_calc(olddata+512,510);
 memcpy(flash+LEGACY,olddata,4096); boot_async();
 CHECK(mem_read(LEGACY,got,4096));
 memcpy(newdata,olddata,4096); newdata[15]^=0x5a; newdata[620]^=0x37;
 *(uint16_t*)(newdata+510)=crc16_calc(newdata,510);
 *(uint16_t*)(newdata+1022)=crc16_calc(newdata+512,510);
 operations=0;
}
static void start_async(void) { CHECK(mem_config_begin(newdata,newdata+512)); }
static void drain_async(void) {
 unsigned steps=0; while(mem_config_result()==1) { unsigned before=operations; mem_config_poll(ticks++); CHECK(operations-before<=1); CHECK(++steps<2000); }
 CHECK(mem_config_result()==2);
}
int main(void) {
 seed_async(); heap_used=0x1400;CHECK(!mem_config_begin(newdata,newdata+512)&&!operations);heap_used=0;
 start_async(); CHECK(mem_config_result()==1&&!operations);
 newdata[15]^=1; CHECK(mem_config_snapshot()[15]!=(uint8_t)newdata[15]);newdata[15]^=1;
 CHECK(!mem_config_begin(olddata,olddata+512)); mem_config_reset(); CHECK(mem_config_result()==1);
 drain_async(); CHECK(!memcmp(flash+LEGACY,newdata,4096));
 boot_async(); CHECK(mem_read(LEGACY,got,4096)); CHECK(!memcmp(got,newdata,4096));
 seed_async(); memcpy(snapshot,flash,sizeof(flash)); start_async(); drain_async();
 unsigned total=operations, lens[1024], cases=0; memcpy(lens,oplength,sizeof(lens));
 for(unsigned k=1;k<=total;k++) for(unsigned p=0;p<=lens[k-1];p++) {
   memcpy(flash,snapshot,sizeof(flash)); boot_async(); CHECK(mem_read(LEGACY,got,4096));
   operations=0; cut=k; partial=p;
   if(!setjmp(poweroff)) { start_async(); drain_async(); }
   cut=0; boot_async(); CHECK(mem_read(LEGACY,got,4096));
   CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));
   start_async(); drain_async(); boot_async(); CHECK(mem_read(LEGACY,got,4096)); CHECK(!memcmp(got,newdata,4096)); cases++;
 }
 seed_async(); start_async(); async_fail=1;
 while(mem_config_result()==1) mem_config_poll(ticks++);
 CHECK(mem_config_result()==3); mem_config_reset(); CHECK(!heap_used);
 boot_async();CHECK(mem_read(LEGACY,got,4096));CHECK(!memcmp(got,olddata,4096));
 /* Если при старте нет целого банка, публикация нового банка предшествует уничтожению исходного зеркала. */
 seed_async();memset(flash+MEM_CONFIG_SLOT0,255,MEM_CONFIG_SLOT_SIZE);memset(flash+MEM_CONFIG_SLOT1,255,MEM_CONFIG_SLOT_SIZE);
 cfg_cached_active=MEM_CONFIG_LEGACY;cfg_cached_generation=0;
 start_async();drain_async();CHECK(!memcmp(flash+LEGACY,newdata,4096));

 unsigned read_faults=0;
 for(unsigned r=1;r<=384;r++) {
   seed_async();read_calls=0;read_fault=r;start_async();
   while(mem_config_result()==1) mem_config_poll(ticks++);
   CHECK(mem_config_result()==3 || read_calls<r);if(mem_config_result()==3)read_faults++;read_fault=0;
   boot_async();CHECK(mem_read(LEGACY,got,4096));CHECK(!memcmp(got,olddata,4096)||!memcmp(got,newdata,4096));
 }
 printf("PASS async config: busy, bounded mutation steps, immutable input, reboot/resume; %u cut points, %u read faults\n",cases,read_faults);
 return 0;
}
