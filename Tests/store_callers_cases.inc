
static void setup(void){
 memset(&log_job,0,sizeof(log_job)); log_timeKnown=1; host_boot(); memset(&cfg_job,0,sizeof(cfg_job)); held=0; memset(&store,0,sizeof(store));
 cfg_legacy_checked=0; memset(&config,0,sizeof(config)); memset(&device,0,sizeof(device));
 memset(flash,255,sizeof(flash));memset(mem,0,sizeof(mem));
 mem[MB_CONFIG].start=MEM_CONFIG_LEGACY;mem[MB_CONFIG].size=4096;
 mem[MB_LOG].start=1897472;mem[MB_LOG].size=98304;
 mem[MB_TEMPID].start=1999872;mem[MB_TEMPID].size=49152;
 config.crc=crc16_calc((uint8_t*)&config,510);device.crc=crc16_calc((uint8_t*)&device,510);
 memcpy(flash+MEM_CONFIG_LEGACY,&config,512);memcpy(flash+MEM_CONFIG_LEGACY+512,&device,512);
 CHECK(mem_read(MEM_CONFIG_LEGACY,got,1024));operations=0;
 upload_reset(9);upload_reset(10);store_runtime=1;async_fail=0;store_save_requested=store_reset_requested=0;
 usb_bus_resets=now=frames=empty_frames=acks=stamps=syncs=resets=0;log_pendingCount=0;txbusy=false;
 for(unsigned i=0;i<40;i++){cellLog_s c;memset(&c,0,sizeof(c));c.index=i+1;c.logType=33;memcpy(flash+mem[MB_LOG].start+i*32,&c,32);}
 device.logsCount=40;device.newAddressLog=mem[MB_LOG].start+40*32;device.status.readyLogData=1;
}
static void drain(void){unsigned n=0;while(store.job||mem_config_result()||store_save_requested||store_reset_requested){user_storage_poll();now++;CHECK(++n<20000);}}
int main(void){
 setup();CHECK(store_command_allowed(12));store.job=US_SAVE;CHECK(store_command_allowed(24)&&store_command_allowed(27)&&!store_command_allowed(12));store.job=0;user_send_logRecords(CH_SERVER,7);CHECK(frames==1&&log_unacked[1]==32&&!store.job);
 user_send_logRecords(CH_SERVER,7);CHECK(frames==2&&!store.job&&device.readLogIDIndex==0); // retransmission
 user_send_logRecords(CH_SERVER,8);CHECK(store.job==US_ACK&&device.readLogIDIndex==0&&frames==2);
 for(unsigned i=0;i<12;i++){user_storage_poll();now++;CHECK(device.readLogIDIndex==0&&frames==2);}
 drain();CHECK(frames==3&&device.readLogIDIndex==32&&log_unacked[1]==40);
 user_send_logRecords(CH_SERVER,9);CHECK(!empty_frames);drain();CHECK(empty_frames==1&&!store.clear&&!device.logsCount);
 setup();user_send_logRecords(CH_SERVER,7);user_comm_session(false);user_send_logRecords(CH_SERVER,8);CHECK(!store.job&&device.readLogIDIndex==0&&log_unacked[1]==32);
 user_send_logRecords(CH_SERVER,9);CHECK(store.job==US_ACK);user_comm_session(false);unsigned sent=frames;drain();CHECK(frames==sent&&device.readLogIDIndex==32);
 setup();store_save_requested=1;user_storage_poll();CHECK(store.job==US_SAVE);device.FWblockIdx=99;drain();device_s saved_device;CHECK(mem_read(MEM_CONFIG_LEGACY+512,(uint8_t*)&saved_device,512));CHECK(saved_device.FWblockIdx==0); // owned snapshot
 setup();store_reset_requested=1;user_storage_poll();CHECK(store.job==US_RESET&&!resets);drain();CHECK(resets==1);
 setup();store_reset_requested=1;user_storage_poll();
 memset(&log_pending[0],0,sizeof(log_pending[0]));log_pending[0].index=6789;log_pendingCount=1;
 while(mem_config_result()!=2){user_storage_poll();now++;}user_storage_poll();CHECK(!resets&&store_reset_requested&&log_pendingCount==1);
 drain();CHECK(resets==1&&!log_pendingCount&&log_getRecordsCount()==41);
 setup();config.datetime=42;device.datetimeConfig=42;store.cmd=12;store.channel=CH_SERVER;CHECK(store_config_start());CHECK(!stamps&&!syncs);txbusy=true;
 while(store.job!=US_REPLY){user_storage_poll();now++;}CHECK(syncs==1&&!stamps);user_storage_poll();CHECK(!stamps);txbusy=false;drain();CHECK(stamps==1);
 // Reboot after a verified cleanup intent, with a partially erased journal.
 setup();user_send_logRecords(CH_SERVER,1);user_send_logRecords(CH_SERVER,2);drain();user_send_logRecords(CH_SERVER,3);
 while(store.job!=US_ERASE){user_storage_poll();now++;}CHECK(!empty_frames&&store.clear==10);
 CHECK(mem_read(MEM_CONFIG_LEGACY+512,(uint8_t*)&saved_device,512));
 memset(flash+(mem[MB_LOG].start&~4095u),255,1300);
 memset(&store,0,sizeof(store));held=0;store_restore(&saved_device);CHECK(store.job==US_RECOVER&&store.quiet);
 cellLog_s pending;memset(&pending,0,sizeof(pending));CHECK(!log_addEvent(&pending));drain();CHECK(!store.clear&&!empty_frames);CHECK(log_addEvent(&pending));

 // A failed checkpoint cannot acknowledge or advance; same packet can be retried.
 setup();user_send_logRecords(CH_SERVER,1);user_send_logRecords(CH_SERVER,2);async_fail=1;drain();
 CHECK(device.readLogIDIndex==0&&frames==1&&log_unacked[1]==32);user_send_logRecords(CH_SERVER,2);drain();CHECK(device.readLogIDIndex==32&&frames==2);
 // Neither a failed intent nor failed erase admits new journal writes or an empty reply.
 setup();user_send_logRecords(CH_SERVER,1);user_send_logRecords(CH_SERVER,2);drain();user_send_logRecords(CH_SERVER,3);
 while(store.job!=US_MARK){user_storage_poll();now++;}async_fail=1;
 while(mem_config_result()!=3){user_storage_poll();now++;}user_storage_poll();now++;
 CHECK(store.clear==10&&!empty_frames);CHECK(!log_addEvent(&pending));
 while(store.job!=US_ERASE){user_storage_poll();now++;}async_fail=1;
 while(!store.waiting){user_storage_poll();now++;}
 while(store.waiting){user_storage_poll();now++;}CHECK(!empty_frames&&store.clear==10);drain();CHECK(empty_frames==1);
 // Every cleanup sector can be interrupted, without making remaining data look like a new queue.
 for(unsigned sector=0;sector<25;sector++){
   setup();device.newAddressLog=mem[MB_LOG].start+mem[MB_LOG].size;device.logsCount=3072;store.cmd=10;store.channel=CH_SERVER;store.uniq=7;
   CHECK(!store_finish(10));while(store.job!=US_ERASE){user_storage_poll();now++;}
   CHECK(mem_read(MEM_CONFIG_LEGACY+512,(uint8_t*)&saved_device,512));
   memset(flash+(mem[MB_LOG].start&~4095u),255,sector*4096+127);
   memset(&store,0,sizeof(store));held=0;store_restore(&saved_device);drain();CHECK(!store.clear&&!empty_frames);
 }

 // The prior application can append after rollback while ignoring the intent.
 // The new application preserves that first sector and requires fresh ACKs.
 setup();store.cmd=10;store.channel=CH_SERVER;CHECK(!store_finish(10));
 while(store.job!=US_ERASE){user_storage_poll();now++;}
 CHECK(mem_read(MEM_CONFIG_LEGACY+512,(uint8_t*)&saved_device,512));
 memset(flash+(mem[MB_LOG].start&~4095u),255,4096);memset(&pending,0,sizeof(pending));pending.index=12345;
 memcpy(flash+mem[MB_LOG].start,&pending,sizeof(pending));memset(&store,0,sizeof(store));held=0;
 store_restore(&saved_device);device.logsCount=log_getRecordsCount();drain();
 CHECK(!memcmp(flash+mem[MB_LOG].start,&pending,sizeof(pending))&&!store.clear&&!empty_frames);
 CHECK(device.readLogIDIndex==0);

 setup();user_send_logRecords(CH_USB,7);CHECK(log_unacked[0]==32);
 usb_bus_resets++;CHECK(user_storage_command_ready());CHECK(!log_unacked[0]);
 user_send_logRecords(CH_USB,8);CHECK(!store.job&&upload_index(10,CH_USB)==0&&log_unacked[0]==32);

 setup();user_send_logRecords(CH_SERVER,1);void *held_heap=malloc(0x1400);CHECK(held_heap);
 user_send_logRecords(CH_SERVER,2);CHECK(!store.job&&!device.readLogIDIndex&&frames==1);free(held_heap);
 user_send_logRecords(CH_SERVER,2);CHECK(store.job==US_ACK);drain();CHECK(device.readLogIDIndex==32&&frames==2);
 setup();memset(&log_pending[0],0,sizeof(log_pending[0]));log_pending[0].index=999;log_pendingCount=1;
 user_storage_poll();CHECK(log_async_active()&&!user_storage_command_ready()&&!store_snapshot());
 CHECK(store_command_allowed(24)&&store_command_allowed(27));
 while(log_pendingCount){user_storage_poll();now++;}CHECK(log_getRecordsCount()==41);
 puts("PASS storage callers: delayed ACK/config/reset, duplicate, reconnect, busy slot, immutable checkpoint, partial-clear reboot/resume");return 0;
}
