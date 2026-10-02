# Доработка: Кряжев А., 2026
# Каждая мутация должна компилироваться и падать на проверке поведения.
import os, pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
mutations=[
 ('readback_error', 'if (!mem_read(address, check, step)) return 0;', 'mem_read(address, check, step);'),
 ('program_error', 'if (!mem_write(address, data, n)) return 0;', 'mem_write(address, data, n);'),
 ('backup_read_error', 'if (!mem_read(address, b, step)) return 0;', 'mem_read(address, b, step);'),
 ('cleanup_marker_error', 'if (!mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4)) return 2;', 'mem_read(scratch + MEMSECTORSIZE + 32u, (uint8_t*)&marker, 4);'),
 ('confirmed_read_error', 'if (!mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1)) return 0;', 'mem_read(address + PHOTO_RECORD_HEAD - 1u, &full, 1);'),
 ('cleanup_header_error', 'if (!mem_read(at, (uint8_t*)marker, sizeof(marker))) return;', 'mem_read(at, (uint8_t*)marker, sizeof(marker));'),
 ('cleanup_bitmap_error', 'if (!mem_read(at + sizeof(marker), done, sectors)) return;', 'mem_read(at + sizeof(marker), done, sectors);'),
 ('cleanup_erase_error', 'if (!mem_erase_sector(first + (uint32_t)i * MEMSECTORSIZE)) return;', 'mem_erase_sector(first + (uint32_t)i * MEMSECTORSIZE);'),
 ('restore_erase_error', 'if (!mem_erase_sector(h[0])) return 0;', 'mem_erase_sector(h[0]);'),
 ('restore_copy_error', 'if (!mem_read(scratch + photo_copy_off, b, step)) return 0;', 'mem_read(scratch + photo_copy_off, b, step);'),
 ('capture_erase_error', 'if (!mem_erase_sector(photo_erased_to)) { photo_stop(); return 0; }', 'mem_erase_sector(photo_erased_to);'),
 ('upload_read_error', 'if (!mem_read(address + PHOTO_RECORD_HEAD + offset, p + 20, n)) {', 'if (mem_read(address + PHOTO_RECORD_HEAD + offset, p + 20, n) && 0) {'),

 ('duplicate_number','previous.datetime >= photo->stamp','0'),
 ('stream_advance','offset += n;','offset += 0;'),
 ('no_early_erase','if (!device.countPhoto && device.newAddressPhoto != mem[MB_PHOTO].start) {','if (device.newAddressPhoto != mem[MB_PHOTO].start) {'),
 ('final_confirmation','if (!device.countPhoto && !photo_clear_confirmed())','if (0 && !photo_clear_confirmed())'),
 ('first_number','(!number && photo_confirmed_offset(a, (uint16_t)h.size) < h.size)','(0 && photo_confirmed_offset(a, (uint16_t)h.size) < h.size)'),
 ('foreign_photo','if (!found || offset > h.size)','if ((found && number == 0xDEADBEEFu) || offset > h.size)'),
 ('out_of_range','offset > h.size','offset > 65534u'),
 ('repeat_request','if (photo_first_pending(&address, &offset) == 2) return CMD_RUNERROR;','if (photo_first_pending(&address, &offset) == 2) return CMD_RUNERROR; offset = 0;'),
 ('two_photos','address += PHOTO_RECORD_HEAD + h.size; offset = 0;','address += 2u * (PHOTO_RECORD_HEAD + h.size); offset = 0;'),
 ('busy_modem','photo->channel != CH_SERVER || (','1 || ('),
 ('disconnect','if (photo && photo->channel == CH_SERVER) photo->pending = 0;','if (photo && photo->channel == CH_SERVER) photo->pending = 1;'),
 ('malloc_state','p = photo->channel == CH_SERVER ? comm_tx_acquire(&capacity) : malloc(PHOTO_BLOCK + 22u);','photo->offset = 0; p = photo->channel == CH_SERVER ? comm_tx_acquire(&capacity) : malloc(PHOTO_BLOCK + 22u);'),
 ('heap_limit','#define PHOTO_BLOCK 512u','#define PHOTO_BLOCK 1024u'),
 ('camera_speed','photo->stage == P_DATA || (uint32_t)(now - photo->last)', '(uint32_t)(now - photo->last)'),
 ('flash_data','photo->address + PHOTO_RECORD_HEAD + photo->written, p + 4','photo->address + PHOTO_RECORD_HEAD + 1u + photo->written, p + 4'),
 ('empty_count',': 0;\n  if (n) {',': 1;\n  if (n) {'),
 ('ready_bit','device.countPhoto = 0; device.status.readyPhoto = 0; device.readPhotoIndex = 0;','device.countPhoto = 0; device.status.readyPhoto = 1; device.readPhotoIndex = 0;'),
 ('early_header','photo->size = (uint16_t)size;','photoParam_s early = {photo->stamp, size}; mem_write(photo->address, (uint8_t*)&early, 8);\n  photo->size = (uint16_t)size;'),
 ('oversize','size > PHOTO_LIMIT','size > 0xFFFFFFu'),
 ('space','size > end - photo->address - PHOTO_RECORD_HEAD','size > end - photo->address'),
 ('update_start','device.status.readyToUpdate || !photo_alloc()','!photo_alloc()'),
 ('update_upload','device.status.readyToUpdate || channel == CH_SMS','channel == CH_SMS'),
 ('address','config.rs485Periphery[i].address == address','config.rs485Periphery[i].address != 0'),
 ('checksum','p[n-2] != sum','(p[n-2] != sum && 0)'),
 ('timeout','P_SYNC ? 30 : 3','P_SYNC ? 31 : 3'),
 ('previous_photo','photo_erased_to = prefix ? photo->address - prefix + MEMSECTORSIZE : photo->address;','photo_erased_to = photo->address - prefix;'),
 ('capture_pending','if (photo->stage) return CMD_RUNERROR;','if (photo->stage || photo->pending) return CMD_RUNERROR;'),
 ('capture_waiting_ack','if (photo->stage) return CMD_RUNERROR;','if (photo->stage || device.status.readyPhoto) return CMD_RUNERROR;'),
 ('ack_persistence','return done;','return 0;'),
 ('empty_success','if (size && device.countPhoto) {','if (size) {'),
 ('recover_gap','if (!photo_cleanup_active() && photo_recover() && !device.countPhoto &&','if (0 && !photo_cleanup_active() && photo_recover() && !device.countPhoto &&'),
 ('restore_stream','(photo_cleanup_active() == 1 || photo_recover()) &&','1 &&'),
 ('no_cleanup_marker','!photo_write(at, (uint8_t*)marker, 4)','0'),
 ('cleanup_batch','if (!erased && photo_cleanup_active()) photo_cleanup_step();','if (photo_cleanup_active()) { photo_cleanup_step(); photo_cleanup_step(); }'),
 ('lazy_batch','wdt_reload(); if (!mem_erase_sector(photo_erased_to)) { photo_stop(); return 0; }','wdt_reload(); if (!mem_erase_sector(photo_erased_to)) { photo_stop(); return 0; } mem_erase_sector(photo_erased_to + MEMSECTORSIZE);'),
 ('zero_time','device.datetime ? device.datetime : 1u','device.datetime'),
 ('pending_timeout','(uint32_t)(now - photo_pending_since) >= 60000u','(uint32_t)(now - photo_pending_since) >= 600000u'),
 ('pending_progress','photo_pending_since = timers_getMillis();\n  } else photo->pending = 0;','photo_pending_since = 0;\n  } else photo->pending = 0;'),
 ('empty_no_flash_write','if (size && device.countPhoto) {','if (!device.countPhoto) { uint8_t zero = 0; mem_write(photo_storage_end() + MEMSECTORSIZE + 64u, &zero, 1); }\n  if (size && device.countPhoto) {'),
]

subprocess.run([sys.executable,str(ROOT/"Tests/run_host.py"),"work","photo"],check=True)
reports=[]
with tempfile.TemporaryDirectory(prefix='photo-mutations-') as tmp:
 copy=pathlib.Path(tmp)
 for name in ['Tests/run_host.py','Tests/test_photo.c','Tests/comm_photo_slot.c','User/user_fun.c','User/photo.c']:
  dest=copy/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 original=(copy/'User/photo.c').read_text(encoding='cp1251')
 for name,old,new in mutations:
  assert original.count(old)==1,(name,original.count(old))
  (copy/'User/photo.c').write_bytes(original.replace(old,new).replace('\n','\r\n').encode('cp1251'))
  r=subprocess.run([sys.executable,str(ROOT/'Tests/run_host.py'),'work','photo'],env={**os.environ,'DOMOVOI_ROOT':str(copy)},capture_output=True,text=True)
  output=r.stdout+r.stderr
  assert r.returncode and 'error:' not in output and 'Assertion failed' in output,(name,r.returncode,output)
  reports.append(name+': DETECTED');print(reports[-1],flush=True)
print('photo mutations: %d/%d detected'%(len(reports),len(mutations)))
