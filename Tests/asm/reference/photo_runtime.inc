/* Independent specification of the checked-in IAR ASM photo state machines.
 * All NOR writes are owned until verified. No BUSY loops. One I/O/64-byte read
 * per storage pass; send buffers remain private across communication polls. */
#include "photo_runtime.h"
#include "usb_istr.h"
static photo_runtime_s pr;
enum { PT_IDLE, PT_START, PT_PREP, PT_DATA, PT_REQUEST, PT_ACK, PT_SEND, PT_CLEAN };
enum {
  PR_IO=1, PR_META, PR_HEAD, PR_FULL, PR_ACK_READ, PR_RECORD, PR_CAMERA,
  PR_TAIL, PR_META_ERASE, PR_DATA_ERASE, PR_BACKUP, PR_BACKUP_ADVANCE,
  PR_BACKUP_CRC, PR_PREFIX_BODY, PR_PREFIX_MARK, PR_RESTORE_CRC,
  PR_RESTORE_ERASE, PR_RESTORE, PR_RESTORE_ADVANCE, PR_DEST_CRC,
  PR_PREFIX_CLEAN, PR_PREP_DONE, PR_LAZY_DONE, PR_DATA_DONE, PR_PHOTO_MARK,
  PR_PHOTO_DONE, PR_CLEAN_BODY, PR_CLEAN_MARK, PR_CLEAN_SCAN, PR_CLEAN_ERASE,
  PR_CLEAN_DONE, PR_CLEAN_COMMIT, PR_CLEAN_END, PR_ACK_HEAD, PR_ACK_WRITE,
  PR_ACK_COMMIT, PR_ACK_ADVANCE, PR_REQUEST_DONE, PR_SEND_FILL, PR_SEND_CRC, PR_SEND_COMMIT,
  PR_META_BLANK, PR_META_DONE
};
static void photo_stop(void);
static uint8_t photo_alloc(void);
static void photo_command(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t);
static void photo_issue(void);
static void photo_frame(uint8_t *,uint16_t);
static void photo_fail(const char *);
static uint8_t pr_data_ready(void);

bool photo_storage_busy(void) { return pr.phase!=0; }
void photo_storage_cancel(void)
{
  pr.cancelled=pr.quiet=1;
  if (pr.task==PT_ACK) pr.initialized=0;
  if (photo) { photo->pending=0; if (photo->stage) photo_stop(); }
}
static void pr_release(void)
{
  free(pr.tx); pr.tx=0; pr.phase=pr.waiting=pr.committed=0;
}
static void pr_fail(void)
{
  pr.fault=1; /* Finite error outcome; reset/recovery resolves uncertain media. */
  if (photo) {
    if (photo->stage) photo_stop();
    photo->pending=0;
    if ((pr.task==PT_REQUEST || pr.task==PT_ACK) && !pr.quiet)
      user_send_answer_comand(21,CMD_RUNERROR,photo->channel,photo->requestId);
  }
  pr.initialized=0; pr_release();
}
static void pr_write(uint32_t address,const uint8_t *p,uint16_t n,uint8_t after)
{
  if (!n || n>64) { pr_fail(); return; }
  if (p!=sjr.data) memcpy(sjr.data,p,n);
  pr.io_address=address; pr.io_len=n; pr.io_done=0; pr.erase=0; pr.after=after; pr.phase=PR_IO;
}
static void pr_erase(uint32_t address,uint8_t after)
{
  pr.io_address=address; pr.io_len=pr.io_done=0; pr.erase=1; pr.after=after; pr.phase=PR_IO;
}
static void pr_scan_begin(uint8_t task)
{
  pr.task=task; pr.scan=mem[MB_PHOTO].start; pr.count=0; pr.max_number=0;
  pr.first=0; pr.first_offset=0; pr.phase=PR_HEAD;
}
static void pr_scan_finish(void)
{
  if (pr.task==PT_SEND) { pr.send_size=0; pr.phase=PR_SEND_FILL; return; }
  device.newAddressPhoto=pr.scan; device.countPhoto=pr.count; device.status.readyPhoto=pr.count!=0;
  pr.last_number=pr.max_number; pr.initialized=1;
  if (pr.task==PT_START) { pr.phase=PR_CAMERA; return; }
  if (pr.task==PT_REQUEST && pr.requested_number!=0xffffffffu) {
    if (photo && !pr.quiet) user_send_answer_comand(21,CMD_PARAMERROR,photo->channel,photo->requestId);
    pr_release(); return;
  }
  if (pr.task==PT_REQUEST || pr.task==PT_ACK) { pr.phase=PR_REQUEST_DONE; return; }
  pr.phase=0;
}
static cmdCode_e pr_start(uint8_t address)
{
  uint8_t found=0;
  for (uint8_t i=0;i<8;i++) if (config.rs485Periphery[i].type==3 && config.rs485Periphery[i].address==address) found=1;
  if (!found || config.rs485Mode!=1) return CMD_PARAMERROR;
  if (!user_storage_command_ready() || !pr.initialized || pr.phase || pr.fault || pr.cleanup || device.status.readyToUpdate || !photo_alloc() || photo->stage) return CMD_RUNERROR;
  if ((!device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start) || device.newAddressPhoto>photo_storage_end() || photo_storage_end()-device.newAddressPhoto<=PHOTO_RECORD_HEAD) return CMD_RUNERROR;
  if (pr.last_number==0xfffffffeu || device.datetime==0xffffffffu) return CMD_RUNERROR;
  pr.address=address; pr.cancelled=pr.quiet=pr.committed=0; pr_scan_begin(PT_START);
  return CMD_SUCCESS;
}
static uint8_t pr_prepare(uint32_t size)
{
  uint32_t end=photo_storage_end();
  if (!size || size>65535 || photo->address>end || end-photo->address<PHOTO_RECORD_HEAD || size>end-photo->address-PHOTO_RECORD_HEAD) return 0;
  photo->size=(uint16_t)size; photo->stage=P_PREP;
  pr.task=PT_PREP; pr.prefix=(uint16_t)(photo->address&4095u); pr.offset=0;
  pr.scan=photo->address; pr.limit=(photo->address&~4095u)+4096; pr.phase=PR_TAIL;
  if (!pr.prefix) { photo_erased_to=photo->address; pr.phase=PR_PREP_DONE; }
  return 1;
}
static uint8_t pr_data_ready(void)
{
  uint32_t used=photo->written+PHOTO_DATA;
  if (used>photo->size) used=photo->size;
  uint32_t end=photo->address+PHOTO_RECORD_HEAD+used;
  if (used==photo->size && photo_storage_end()-end>=sizeof(photoParam_s)) end+=sizeof(photoParam_s);
  if (end<=photo_erased_to) return 1;
  if (photo_erased_to>=photo_storage_end()) { photo_stop(); return 0; }
  pr.task=PT_DATA; pr_erase(photo_erased_to,PR_LAZY_DONE); return 0;
}
static void pr_frame(uint8_t *p,uint16_t n)
{
  if (n<6 || n>64 || pr.phase) return;
  uint16_t id=p[0]|((uint16_t)p[1]<<8),len=p[2]|((uint16_t)p[3]<<8); uint8_t sum=0;
  for (uint16_t i=0;i<n-2;i++) sum+=p[i];
  if (id!=photo->packet || !len || len>58 || n!=len+6 || p[n-2]!=sum || p[n-1] || len>photo->size-photo->written) { photo->used=0; return; }
  pr.task=PT_DATA; pr.payload=len;
  pr_write(photo->address+PHOTO_RECORD_HEAD+photo->written,p+4,len,PR_DATA_DONE);
}
static cmdCode_e pr_request(dataChanel_e channel,uint8_t id,const uint8_t *args,uint8_t size)
{
  if (size && size!=6) return CMD_PARAMERROR;
  if (!user_storage_command_ready() || !pr.initialized || pr.phase || pr.fault || pr.cleanup || device.status.readyToUpdate || channel==CH_SMS || !photo_alloc() || photo->stage) return CMD_RUNERROR;
  pr.requested_number=0xffffffffu; pr.requested_offset=0;
  if (size) { memcpy(&pr.requested_number,args,4); memcpy(&pr.requested_offset,args+4,2); }
  pr.quiet=pr.cancelled=pr.committed=pr.reply_wait=0; pr.usb_epoch=usb_bus_resets;
  photo->channel=channel; photo->requestId=id; photo->pending=0;
  pr_scan_begin(PT_REQUEST); return (cmdCode_e)255;
}
void photo_request_answer(dataChanel_e channel,uint8_t id,const uint8_t *args,uint8_t size)
{
  cmdCode_e r=pr_request(channel,id,args,size);
  if ((uint8_t)r!=255) user_send_answer_comand(21,r,channel,id);
}
static void pr_send_begin(void)
{
  pr.task=PT_SEND; pr.quiet=pr.cancelled=0; pr.usb_epoch=usb_bus_resets;
  pr.scan=photo->readAddress; pr.phase=PR_HEAD; pr.reply_wait=0;
}
static void pr_cleanup_begin(void)
{
  uint32_t first=mem[MB_PHOTO].start,end=photo_storage_end(),last=device.newAddressPhoto;
  if (last<=first || last>end) return;
  if (end-last>=sizeof(photoParam_s)) last+=sizeof(photoParam_s);
  last=(last+4095u)&~4095u;
  if (last> end) return;
  pr.cancelled=pr.quiet=pr.committed=0;
  pr.task=PT_CLEAN; pr.h[0]=0; pr.h[1]=last; pr.h[2]=~last; pr.phase=PR_CLEAN_BODY;
}
void photo_storage_poll(uint32_t now)
{
  uint32_t scratch=photo_storage_end(); uint16_t n;
  if (!pr.phase) return;
  if (photo && photo->channel==CH_USB && pr.usb_epoch!=usb_bus_resets && (pr.task==PT_REQUEST || pr.task==PT_ACK || pr.task==PT_SEND)) { pr.quiet=1; photo->pending=0; }
  if (device.status.readyToUpdate) photo_storage_cancel();
  if (pr.waiting) {
    uint8_t r=w25_async_poll(now);
    if (r==1) return;
    pr.waiting=0;
    if (r!=2) { pr_fail(); return; }
    if (!pr.erase) pr.io_done+=pr.io_step;
    if (pr.erase || pr.io_done==pr.io_len) pr.phase=pr.after;
    return;
  }
  if (pr.cancelled && !pr.committed) { pr_release(); return; }
  switch (pr.phase) {
  case PR_IO:
    n=pr.io_len-pr.io_done;
    if (!pr.erase && n>256u-((pr.io_address+pr.io_done)&255u)) n=(uint16_t)(256u-((pr.io_address+pr.io_done)&255u));
    pr.io_step=n;
    if (!w25_async_start(pr.io_address+pr.io_done,pr.erase ? 0 : sjr.data+pr.io_done,n,now)) { pr_fail(); break; }
    pr.waiting=1; break;
  case PR_META:
    if (!w25_read_data(scratch+4096+32,(uint8_t*)pr.h,12)) { pr_fail(); break; }
    if (pr.h[0]==0) {
      /* A torn final metadata erase may leave its marker but erase its body.
       * Body was verified before publishing; destination was verified before
       * cleanup. Discard only this invalid intent, never a photo sector. */
      if (pr.h[1]!=~pr.h[2] || pr.h[1]>scratch || pr.h[1]<=mem[MB_PHOTO].start || (pr.h[1]&4095u)) { pr_erase(scratch+4096,PR_META_DONE); break; }
      pr.cleanup=1; pr.task=PT_CLEAN; pr.limit=pr.h[1]; pr.offset=0; pr.phase=PR_CLEAN_SCAN; break;
    }
    if (!w25_read_data(scratch+4096,(uint8_t*)pr.h,16)) { pr_fail(); break; }
    if (pr.h[3]==0x50484f54u) {
      if (pr.h[0]<mem[MB_PHOTO].start || pr.h[0]>=scratch || (pr.h[0]&4095u) || !pr.h[1] || pr.h[1]>=4096) { pr_erase(scratch+4096,PR_META_DONE); break; }
      pr.committed=1; pr.offset=0; pr.crc=0xffffffffu; pr.phase=PR_RESTORE_CRC;
    } else { pr.offset=0; pr.phase=PR_META_BLANK; }
    break;
  case PR_META_BLANK:
    if (!w25_read_data(scratch+4096+pr.offset,sjr.data,64)) { pr_fail(); break; }
    if (!fast_all_ff(sjr.data,64)) { pr_erase(scratch+4096,PR_META_DONE); break; }
    pr.offset+=64; if (pr.offset==4096) pr.phase=PR_META_DONE;
    break;
  case PR_META_DONE: pr_scan_begin(PT_IDLE); break;
  case PR_HEAD:
    if (pr.scan+sizeof(photoParam_s)>scratch || (pr.task==PT_SEND && pr.scan>=device.newAddressPhoto)) { pr_scan_finish(); break; }
    if (!w25_read_data(pr.scan,(uint8_t*)pr.h,8)) { pr_fail(); break; }
    if (pr.h[0]==0xffffffffu) { pr_scan_finish(); break; }
    if (!pr.h[1] || pr.h[1]>65535 || scratch-pr.scan<PHOTO_RECORD_HEAD || pr.h[1]>scratch-pr.scan-PHOTO_RECORD_HEAD) { pr.scan=scratch; pr_scan_finish(); break; }
    if (pr.h[0]>pr.max_number) pr.max_number=pr.h[0];
    pr.confirmed=0; pr.slot=0; pr.free_slot=0xffff; pr.phase=PR_FULL; break;
  case PR_FULL:
    if (!w25_read_data(pr.scan+PHOTO_RECORD_HEAD-1,sjr.data,1)) { pr_fail(); break; }
    if (!sjr.data[0]) { pr.confirmed=(uint16_t)pr.h[1]; pr.phase=PR_RECORD; }
    else pr.phase=PR_ACK_READ;
    break;
  case PR_ACK_READ:
    if (!w25_read_data(pr.scan+8+pr.slot,sjr.data,64)) { pr_fail(); break; }
    for (uint8_t i=0;i<64;i+=8) {
      const uint8_t *b=sjr.data+i;
      uint16_t value=(uint16_t)(b[0]|((uint16_t)b[1]<<8)),inverse=(uint16_t)(b[2]|((uint16_t)b[3]<<8));
      if (fast_all_ff(b,8)) { pr.free_slot=pr.slot+i; pr.phase=PR_RECORD; break; }
      if (!b[4] && !b[5] && !b[6] && !b[7] && (uint16_t)(value^inverse)==65535 && value<=pr.h[1] && value>pr.confirmed) pr.confirmed=value;
    }
    pr.slot+=64; if (pr.slot==1024) pr.phase=PR_RECORD; break;
  case PR_RECORD:
    if (pr.confirmed<pr.h[1]) { pr.count++; if (!pr.first) { pr.first=pr.scan; pr.first_offset=pr.confirmed; } }
    if (pr.task==PT_REQUEST && pr.requested_number!=0xffffffffu &&
        ((pr.requested_number && pr.requested_number==pr.h[0]) || (!pr.requested_number && pr.confirmed<pr.h[1]))) {
      if (pr.requested_offset>pr.h[1] || (pr.requested_offset>pr.confirmed && pr.requested_offset!=pr.h[1] && pr.free_slot==65535)) {
        if (!pr.quiet) user_send_answer_comand(21,CMD_PARAMERROR,photo->channel,photo->requestId);
        pr_release(); break;
      }
      pr.target=pr.scan; pr.target_slot=pr.scan+8+pr.free_slot; pr.target_done=pr.confirmed; pr.target_size=(uint16_t)pr.h[1];
      pr.scan=mem[MB_PHOTO].start; pr.task=PT_ACK; pr.phase=PR_ACK_HEAD; break;
    }
    if (pr.task==PT_SEND && pr.confirmed<pr.h[1]) {
      uint16_t off=pr.scan==photo->readAddress ? photo->offset : 0;
      if (off<pr.confirmed) off=pr.confirmed;
      if (off>=pr.h[1]) { pr.scan+=PHOTO_RECORD_HEAD+pr.h[1]; pr.phase=PR_HEAD; break; }
      pr.send_offset=off; pr.send_size=(uint16_t)(pr.h[1]-off); if (pr.send_size>512) pr.send_size=512;
      pr.phase=PR_SEND_FILL; break;
    }
    pr.scan+=PHOTO_RECORD_HEAD+pr.h[1]; pr.phase=PR_HEAD; break;
  case PR_CAMERA:
    if (pr.cleanup || !pr.initialized || (!device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start) ||
        device.newAddressPhoto>scratch || scratch-device.newAddressPhoto<=PHOTO_RECORD_HEAD) { pr_fail(); break; }
    photo->address=device.newAddressPhoto; photo->stamp=device.datetime ? device.datetime : 1;
    if (pr.last_number>=photo->stamp) photo->stamp=pr.last_number+1;
    if (photo->stamp==0xffffffffu || !photo->stamp) { pr_fail(); break; }
    photo->size=photo->written=photo->packet=photo->used=0; photo_erased_to=0;
    photo->retry=photo->syncAck=photo->syncSeen=0; photo->rxBytes=0; photo->stage=P_SYNC;
    photo->started=now; photo->last=now-50; photo->deadline=now;
    uart_init(PHOTO_PORT,PHOTO_BAUD,0,photo->rx,sizeof(photo->rx)); device.getPhoto=pr.address; pr.phase=0; break;
  case PR_TAIL:
    n=(uint16_t)(pr.limit-pr.scan); if (n>64) n=64;
    if (!w25_read_data(pr.scan,sjr.data,n)) { pr_fail(); break; }
    if (!fast_all_ff(sjr.data,n)) { pr.phase=PR_META_ERASE; break; }
    pr.scan+=n;
    if (pr.scan==pr.limit) { photo_erased_to=pr.limit; pr.phase=PR_PREP_DONE; }
    break;
  case PR_META_ERASE: pr_erase(scratch+4096,PR_DATA_ERASE); break;
  case PR_DATA_ERASE: pr.offset=0; pr.crc=0xffffffffu; pr_erase(scratch,PR_BACKUP); break;
  case PR_BACKUP:
    n=pr.prefix-pr.offset; if (n>64) n=64;
    if (!w25_read_data((photo->address&~4095u)+pr.offset,sjr.data,n)) { pr_fail(); break; }
    pr.crc=crc32_update(pr.crc,sjr.data,n); pr.payload=n;
    pr_write(scratch+pr.offset,sjr.data,n,PR_BACKUP_ADVANCE); break;
  case PR_BACKUP_ADVANCE:
    pr.offset+=pr.payload;
    if (pr.offset==pr.prefix) { pr.expected=pr.crc; pr.crc=0xffffffffu; pr.offset=0; pr.phase=PR_BACKUP_CRC; }
    else pr.phase=PR_BACKUP;
    break;
  case PR_BACKUP_CRC:
    n=pr.prefix-pr.offset; if (n>64) n=64;
    if (!w25_read_data(scratch+pr.offset,sjr.data,n)) { pr_fail(); break; }
    pr.crc=crc32_update(pr.crc,sjr.data,n); pr.offset+=n;
    if (pr.offset==pr.prefix) {
      if (pr.crc!=pr.expected) { pr_fail(); break; }
      pr.h[0]=photo->address&~4095u; pr.h[1]=pr.prefix; pr.h[2]=pr.crc; pr.h[3]=0x50484f54u; pr.phase=PR_PREFIX_BODY;
    }
    break;
  case PR_PREFIX_BODY: pr_write(scratch+4096,(uint8_t*)pr.h,12,PR_PREFIX_MARK); break;
  case PR_PREFIX_MARK:
    pr.committed=1; pr_write(scratch+4096+12,(uint8_t*)pr.h+12,4,PR_RESTORE_ERASE); break;
  case PR_RESTORE_CRC:
    n=(uint16_t)(pr.h[1]-pr.offset); if (n>64) n=64;
    if (!w25_read_data(scratch+pr.offset,sjr.data,n)) { pr_fail(); break; }
    pr.crc=crc32_update(pr.crc,sjr.data,n); pr.offset+=n;
    if (pr.offset==pr.h[1]) { if (pr.crc!=pr.h[2]) pr_fail(); else pr.phase=PR_RESTORE_ERASE; }
    break;
  case PR_RESTORE_ERASE: pr.offset=0; pr_erase(pr.h[0],PR_RESTORE); break;
  case PR_RESTORE:
    n=(uint16_t)(pr.h[1]-pr.offset); if (n>64) n=64;
    if (!w25_read_data(scratch+pr.offset,sjr.data,n)) { pr_fail(); break; }
    pr.payload=n; pr_write(pr.h[0]+pr.offset,sjr.data,n,PR_RESTORE_ADVANCE); break;
  case PR_RESTORE_ADVANCE:
    pr.offset+=pr.payload;
    if (pr.offset==pr.h[1]) { pr.offset=0; pr.crc=0xffffffffu; pr.phase=PR_DEST_CRC; }
    else pr.phase=PR_RESTORE;
    break;
  case PR_DEST_CRC:
    n=(uint16_t)(pr.h[1]-pr.offset); if (n>64) n=64;
    if (!w25_read_data(pr.h[0]+pr.offset,sjr.data,n)) { pr_fail(); break; }
    pr.crc=crc32_update(pr.crc,sjr.data,n); pr.offset+=n;
    if (pr.offset==pr.h[1]) { if (pr.crc!=pr.h[2]) pr_fail(); else pr.phase=PR_PREFIX_CLEAN; }
    break;
  case PR_PREFIX_CLEAN: pr_erase(scratch+4096,PR_PREP_DONE); break;
  case PR_PREP_DONE:
    if (pr.committed) photo_erased_to=pr.h[0]+4096;
    pr.committed=0;
    if (pr.task==PT_PREP && !pr.cancelled) { photo->stage=P_DATA; photo->deadline=now; pr.phase=0; }
    else if (pr.cancelled) pr_release(); else pr_scan_begin(PT_IDLE);
    break;
  case PR_LAZY_DONE: photo_erased_to+=4096; pr.phase=0; break;
  case PR_DATA_DONE:
    photo->written+=pr.payload; photo->packet++; photo->retry=0; photo->deadline=now;
    if (photo->written!=photo->size) { pr.phase=0; break; }
    pr.h[0]=photo->stamp; pr.h[1]=photo->size;
    pr_write(photo->address+4,(uint8_t*)pr.h+4,4,PR_PHOTO_MARK); break;
  case PR_PHOTO_MARK:
    pr.committed=1; pr_write(photo->address,(uint8_t*)pr.h,4,PR_PHOTO_DONE); break;
  case PR_PHOTO_DONE:
    device.countPhoto++; device.status.readyPhoto=1;
    device.newAddressPhoto=photo->address+PHOTO_RECORD_HEAD+photo->size; pr.last_number=photo->stamp;
    log_addPhotoEvent(photo->stamp); photo_command(0x0e,0,0,0xf0,0xf0); photo_stop(); pr_release(); break;
  case PR_CLEAN_BODY: pr_write(scratch+4096+36,(uint8_t*)pr.h+4,8,PR_CLEAN_MARK); break;
  case PR_CLEAN_MARK:
    pr.committed=1; pr_write(scratch+4096+32,(uint8_t*)pr.h,4,PR_CLEAN_COMMIT); break;
  case PR_CLEAN_COMMIT:
    pr.cleanup=1; pr.initialized=0;
    device.newAddressPhoto=mem[MB_PHOTO].start; device.countPhoto=0; device.status.readyPhoto=0; device.readPhotoIndex=0;
    if (photo) { photo->readAddress=mem[MB_PHOTO].start; photo->offset=0; }
    pr_release(); break;
  case PR_CLEAN_SCAN:
    n=(uint16_t)((pr.limit-mem[MB_PHOTO].start)/4096u-pr.offset); if (n>64) n=64;
    if (!n || pr.offset+n>256) { pr_fail(); break; }
    if (!w25_read_data(scratch+4096+44+pr.offset,sjr.data,n)) { pr_fail(); break; }
    for (uint16_t i=0;i<n;i++) if (sjr.data[i]!=0) { pr.offset+=i; pr.committed=1; pr.phase=PR_CLEAN_ERASE; return; }
    pr.offset+=n;
    if (mem[MB_PHOTO].start+(uint32_t)pr.offset*4096==pr.limit) pr_erase(scratch+4096,PR_CLEAN_END);
    break;
  case PR_CLEAN_ERASE: pr_erase(mem[MB_PHOTO].start+(uint32_t)pr.offset*4096,PR_CLEAN_DONE); break;
  case PR_CLEAN_DONE: sjr.data[0]=0; pr_write(scratch+4096+44+pr.offset,sjr.data,1,PR_CLEAN_COMMIT); break;
  case PR_CLEAN_END:
    pr.cleanup=0; pr.initialized=1; pr.last_number=0;
    device.newAddressPhoto=mem[MB_PHOTO].start; device.countPhoto=0; device.status.readyPhoto=0; pr_release(); break;
  case PR_ACK_HEAD:
    if (!w25_read_data(pr.scan,(uint8_t*)pr.h,8)) { pr_fail(); break; }
    if (!pr.h[1] || pr.h[1]>65535 || pr.scan>pr.target || pr.scan>scratch || scratch-pr.scan<PHOTO_RECORD_HEAD || pr.h[1]>scratch-pr.scan-PHOTO_RECORD_HEAD) { pr_fail(); break; }
    if (pr.scan==pr.target && pr.requested_offset<=pr.target_done) { pr_scan_begin(PT_ACK); break; }
    pr.phase=PR_ACK_WRITE; break;
  case PR_ACK_WRITE:
    if (pr.scan<pr.target || pr.requested_offset==pr.target_size) {
      sjr.data[0]=0; pr_write(pr.scan+PHOTO_RECORD_HEAD-1,sjr.data,1,PR_ACK_ADVANCE);
    } else {
      uint16_t value[2]={pr.requested_offset,(uint16_t)~pr.requested_offset};
      pr_write(pr.target_slot,(uint8_t*)value,4,PR_ACK_COMMIT);
    }
    break;
  case PR_ACK_COMMIT: memset(sjr.data,0,4); pr_write(pr.target_slot+4,sjr.data,4,PR_ACK_ADVANCE); break;
  case PR_ACK_ADVANCE:
    if (pr.scan==pr.target) pr_scan_begin(PT_ACK);
    else { pr.scan+=PHOTO_RECORD_HEAD+pr.h[1]; pr.phase=PR_ACK_HEAD; }
    break;
  case PR_REQUEST_DONE:
    if (!pr.quiet && photo->channel==CH_SERVER && comm_tx_busy()) {
      if (!pr.reply_wait) { pr.reply_wait=1; pr.reply_until=now+2000; }
      if ((int32_t)(now-pr.reply_until)<0) break;
      pr.quiet=1;
    }
    if (!pr.quiet) {
      user_send_answer_comand(21,CMD_SUCCESS,photo->channel,photo->requestId);
      photo->readAddress=pr.first ? pr.first : device.newAddressPhoto; photo->offset=pr.first ? pr.first_offset : 0;
      photo->pending=1; photo_pending_since=now;
    }
    pr_release(); break;
  case PR_SEND_FILL:
    if (!pr.tx) {
      pr.tx=malloc(PHOTO_BLOCK+22); if (!pr.tx) { pr_release(); break; }
      uint16_t len=pr.send_size ? pr.send_size+14 : 4;
      memcpy(pr.tx,"SLDN",4); pr.tx[4]=(uint8_t)len; pr.tx[5]=(uint8_t)(len>>8); pr.tx[6]=60; pr.tx[7]=photo->requestId; pr.tx[8]=14;
      pr.tx[9]=pr.send_size ? (device.countPhoto>255 ? 255 : (uint8_t)device.countPhoto) : 0;
      if (pr.send_size) { memcpy(pr.tx+10,pr.h,8); memcpy(pr.tx+18,&pr.send_offset,2); }
      pr.offset=0;
    }
    n=pr.send_size-pr.offset; if (n>64) n=64;
    if (n && !w25_read_data(pr.scan+PHOTO_RECORD_HEAD+pr.send_offset+pr.offset,pr.tx+20+pr.offset,n)) { pr_fail(); break; }
    pr.offset+=n;
    if (pr.offset==pr.send_size) { pr.offset=0; pr.send_crc=65535; pr.phase=PR_SEND_CRC; }
    break;
  case PR_SEND_CRC:
    n=(pr.send_size ? pr.send_size+16 : 6)-pr.offset; if (n>64) n=64;
    pr.send_crc=crc16_calcInit(pr.tx+4+pr.offset,n,pr.send_crc); pr.offset+=n;
    if (pr.offset==(pr.send_size ? pr.send_size+16 : 6)) { memcpy(pr.tx+4+pr.offset,&pr.send_crc,2); pr.phase=PR_SEND_COMMIT; }
    break;
  case PR_SEND_COMMIT:
    if (pr.quiet || !photo->pending) { pr_release(); break; }
    n=pr.send_size ? pr.send_size+22 : 12;
    uint8_t ok=0;
    if (photo->channel==CH_USB) ok=usb_send(pr.tx,n);
    else {
      uint16_t capacity; uint8_t *p=comm_tx_acquire(&capacity);
      if (p) { if (capacity>=n) { memcpy(p,pr.tx,n); ok=comm_tx_commit(n); } comm_tx_release(); }
    }
    if (!ok) {
      if (!pr.reply_wait) { pr.reply_wait=1; pr.reply_until=now+2000; }
      if ((int32_t)(now-pr.reply_until)<0) break;
      pr_release(); break;
    }
    if (pr.send_size) {
      photo->offset=pr.send_offset+pr.send_size; photo->readAddress=pr.scan;
      if (photo->offset==pr.h[1]) { photo->readAddress+=PHOTO_RECORD_HEAD+pr.h[1]; photo->offset=0; }
      photo_pending_since=now;
    } else photo->pending=0;
    pr_release(); break;
  default: pr_fail(); break;
  }
}
static void pr_poll(void)
{
  if (pr.phase || !user_storage_background_ready()) return;
  uint32_t now=timers_getMillis();
  if (!pr.initialized && !pr.fault) { pr.task=PT_IDLE; pr.cancelled=pr.quiet=0; pr.phase=PR_META; return; }
  if (photo) {
    if (photo->pending && photo->channel==CH_USB && pr.usb_epoch!=usb_bus_resets) photo->pending=0;
    if (device.status.readyToUpdate) photo_storage_cancel();
    else {
      if (photo->stage && uart_getstate(PHOTO_PORT) && !uart_isReceiving(PHOTO_PORT)) {
        uint16_t n=uart_received(PHOTO_PORT); if (n>64) { photo_stop(); n=0; }
        for (uint16_t i=0;i<n && photo->stage && !pr.phase;i++) {
          if (photo->used>=64) { photo_stop(); break; }
          if (photo->rxBytes<sizeof(photo->rxHead)) photo->rxHead[photo->rxBytes]=photo->rx[i];
          if (photo->rxBytes!=65535) photo->rxBytes++;
          photo->frame[photo->used++]=photo->rx[i]; uint16_t want=6;
          if (photo->stage==P_DATA && photo->used>=4) { want=6u+photo->frame[2]+((uint16_t)photo->frame[3]<<8); if (want>64) { photo_stop(); break; } }
          if (photo->used==want) { photo_frame(photo->frame,want); photo->used=0; }
        }
        uart_clear(PHOTO_PORT);
      }
      if (pr.phase) return;
      if (photo->stage && (uint32_t)(now-photo->started)>=60000u) photo_fail("60 s limit");
      if (photo->stage && photo->stage!=P_PREP && (int32_t)(now-photo->deadline)>=0 &&
          (photo->stage==P_DATA || (uint32_t)(now-photo->last)>=50u)) {
        if (photo->stage!=P_DATA || pr_data_ready()) {
          if (photo->retry++>=(photo->stage==P_SYNC ? 30 : 3)) photo_fail("no answer");
          else { photo->used=0; photo_issue(); }
        }
      }
      if (pr.phase) return;
      if (photo->pending) {
        if ((uint32_t)(now-photo_pending_since)>=60000 || (photo->channel==CH_SERVER && !comm_channel_ready())) photo->pending=0;
        else { pr_send_begin(); return; }
      }
    }
    if (!photo->stage && !photo->pending && !pr.phase) { free(photo); photo=0; }
  }
  if (!pr.phase && !pr.fault && !device.countPhoto && device.newAddressPhoto>mem[MB_PHOTO].start && (!photo || !photo->stage)) pr_cleanup_begin();
}
