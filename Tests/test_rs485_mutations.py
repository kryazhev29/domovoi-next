# Доработка: Кряжев А., 2026
import os, pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
mutations=[
 # После 89ac32c порт/скорость камеры выбираются по профилю платы.
 # В host rs485cfg проверяется профиль RS485: все 256 кодов скорости.
 ('photo_start_rate','Tests/asm/reference/User/photo.c','uart_init(PHOTO_PORT, PHOTO_BAUD, 0, photo->rx','uart_init(PHOTO_PORT, 115200, 0, photo->rx'),
 ('photo_stop_rate','Tests/asm/reference/User/photo.c','  photo->stage = P_IDLE;','  uart_init(PORT1_PORT, 115200, 0, port_buffer, PORTBUFSIZE); photo->stage = P_IDLE;'),
 ('baud0','Tests/asm/reference/User/photo.c','default: return 115200;','default: return 9600;'),
 ('baud1','Tests/asm/reference/User/photo.c','case 1: return 57600;','case 1: return 9600;'),
 ('baud2','Tests/asm/reference/User/photo.c','case 2: return 38400;','case 2: return 9600;'),
 ('baud3','Tests/asm/reference/User/photo.c','case 3: return 19200;','case 3: return 9600;'),
 ('baud4','Tests/asm/reference/User/photo.c','case 4: return 9600;','case 4: return 115200;'),
 ('invalid','Tests/asm/reference/User/photo.c','config.reserv[0] = 0;','config.reserv[0] = 1;'),
 ('invalid_log','Tests/asm/reference/User/photo.c','user_debug("RS485: invalid baud code, using 115200\\r\\n");',';'),
 ('usb_receive','Tests/asm/reference/User/user_fun.c','rs485_set_config(&ptr[5]);','if(cch != CH_USB) rs485_set_config(&ptr[5]);'),
 ('server_receive','Tests/asm/reference/User/user_fun.c','rs485_set_config(&ptr[5]);','if(cch != CH_SERVER) rs485_set_config(&ptr[5]);'),
 ('cancel_capture','Tests/asm/reference/User/photo.c','if (changed && photo && photo->stage)','if (changed && photo && photo->stage && 0)'),
 ('composition','Tests/asm/reference/User/photo.c','memcmp(config.rs485Periphery, next->rs485Periphery, sizeof(config.rs485Periphery)) != 0','0'),
 ('upload','Tests/asm/reference/User/photo.c','memcpy(&config, data, sizeof(config));','if(photo) photo->pending=0; memcpy(&config, data, sizeof(config));'),
 ('mode','Tests/asm/reference/User/photo.c','if (config.rs485Mode > 1) config.rs485Mode = 0;',';'),
 ('reserved','Tests/asm/reference/User/photo.c','config.reserv[0] = 0;','memset(config.reserv,0,4);'),
 ('unknown','Tests/asm/reference/User/photo.c','else if (config.rs485Periphery[i].type) other = 1;','else if (config.rs485Periphery[i].type) {other = 1; config.rs485Periphery[i].type=0;}'),
 ('camera_type','Tests/asm/reference/User/photo.c','config.rs485Periphery[i].type == 3 && config.rs485Periphery[i].address == address','config.rs485Periphery[i].type != 0 && config.rs485Periphery[i].address == address'),
 ('address_zero','Tests/asm/reference/User/photo.c','uint8_t i, found = 0;','uint8_t i, found = 0; if(!address) return CMD_PARAMERROR;'),
]
with tempfile.TemporaryDirectory(prefix='rs485-mutations-') as tmp:
    copy=pathlib.Path(tmp)
    files=['Tests/run_host.py','Tests/test_photo.c','Tests/test_rs485cfg.c','Tests/asm/reference/User/user_fun.c','Tests/asm/reference/User/photo.c','structures.h']
    for name in files:
        dest=copy/name; dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
    for name,file,old,new in mutations:
        original=(ROOT/file).read_text('cp1251')
        assert original.count(old)==1,(name,original.count(old))
        (copy/file).write_bytes(original.replace(old,new).replace('\n','\r\n').encode('cp1251'))
        result=subprocess.run([sys.executable,str(ROOT/'Tests/run_host.py'),'work','rs485cfg'],env={**os.environ,'DOMOVOI_ROOT':str(copy)},capture_output=True,text=True)
        output=result.stdout+result.stderr
        assert result.returncode and 'error:' not in output and 'Assertion failed' in output,(name,output)
        shutil.copyfile(ROOT/file,copy/file)
        print(name+': DETECTED',flush=True)
print('rs485 mutations: %d/%d detected'%(len(mutations),len(mutations)))
