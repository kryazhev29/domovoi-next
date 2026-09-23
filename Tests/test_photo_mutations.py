# Доработка: Кряжев А., 2026
# Каждая мутация должна компилироваться и падать на проверке поведения.
import os, pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
mutations=[
 ('duplicate_number','previous.datetime == photo->stamp','0'),
 ('lost_packet','if (!photo->hasOffset || *index >= count) return;', 'if (!photo->hasOffset) { *offset += 512; return; } if (*index >= count) return;'),
 ('repeat_request','else *offset = photo->requestOffset;', 'else *offset = photo->requestOffset + 512;'),
 ('foreign_photo','photo->requestNumber != h.datetime', '0'),
 ('out_of_range','photo->requestOffset > h.size','photo->requestOffset > 65535u'),
 ('reset_resume','if (!photo->hasOffset || *index >= count)', 'if (!photo->valid || !photo->hasOffset || *index >= count)'),
 ('two_photos','(*index)++;','(*index) += 2;'),
 ('malloc_state','p = malloc(PHOTO_BLOCK + 22u);','photo->offset = photo->requestOffset; p = malloc(PHOTO_BLOCK + 22u);'),
 ('heap_limit','#define PHOTO_BLOCK 512u','#define PHOTO_BLOCK 1024u'),
 ('camera_speed','photo->stage == P_DATA || (uint32_t)(now - photo->last)', '(uint32_t)(now - photo->last)'),
 ('flash_data','photo->address + 8u + photo->written, p + 4','photo->address + 9u + photo->written, p + 4'),
 ('empty_count',': 0;\n  if (n) {',': 1;\n  if (n) {'),
 ('ready_bit','device.countPhoto = 0; device.status.readyPhoto = 0;','device.countPhoto = 0; device.status.readyPhoto = 1;'),
 ('early_header','photo->size = size;','photoParam_s early = {photo->stamp, size}; mem_write(photo->address, (uint8_t*)&early, 8);\n  photo->size = size;'),
 ('oversize','size > PHOTO_LIMIT','size > 0xFFFFFFu'),
 ('space','size > end - photo->address - 8u','size > end - photo->address'),
 ('update_start','device.status.readyToUpdate || !photo_alloc()','!photo_alloc()'),
 ('update_upload','device.status.readyToUpdate || channel == CH_SMS','channel == CH_SMS'),
 ('address','config.rs485Periphery[i].address == address','config.rs485Periphery[i].address != 0'),
 ('checksum','p[n-2] != sum','(p[n-2] != sum && 0)'),
 ('timeout','P_SYNC ? 30 : 3','P_SYNC ? 31 : 3'),
 ('previous_photo','if (prefix) a += MEMSECTORSIZE;','if (prefix) a += 0;'),
]

subprocess.run([sys.executable,str(ROOT/"Tests/run_host.py"),"work","photo"],check=True)
reports=[]
with tempfile.TemporaryDirectory(prefix='photo-mutations-') as tmp:
 copy=pathlib.Path(tmp)
 for name in ['Tests/run_host.py','Tests/test_photo.c','User/user_fun.c','User/photo.c']:
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
