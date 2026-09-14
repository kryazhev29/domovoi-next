"""Package application images and upload over the new bootloader's USB CDC.
No device is accessed unless the explicit upload subcommand is used.
"""
import argparse, struct, time, re
from pathlib import Path
MAX_CODE=0x17e00
def crc32(data):
    if len(data)%4: raise ValueError('CRC input must contain complete little-endian words')
    crc=0xffffffff
    for (word,) in struct.iter_unpack('<I',data):
        crc^=word
        for _ in range(32): crc=((crc<<1)^(0x04c11db7 if crc&0x80000000 else 0))&0xffffffff
    return crc
def crc16(data):
    crc=0xffff
    for b in data:
        crc^=b
        for _ in range(8):crc=(crc>>1)^(0xa001 if crc&1 else 0)
    return crc
def application(path):
    match=re.fullmatch(r'(\d+)\.(\d+)\.(\d+)\.bin',path.name)
    if not match:raise ValueError('Expected HW.FW.UNIX_TIMESTAMP.bin')
    hw,fw,stamp=map(int,match.groups())
    if not 1<=hw<=255 or not 1<=fw<=65535 or not 0<=stamp<=0xffffffff:raise ValueError('Metadata outside wire format limits')
    data=path.read_bytes();data+=b'\xff'*((-len(data))%4)
    if not 8<=len(data)<=MAX_CODE:raise ValueError('Application exceeds existing Flash region')
    sp,pc=struct.unpack_from('<II',data)
    if not 0x20000008<=sp<=0x20005000 or sp%8 or not pc&1 or not 0x08008200<=pc&~1<0x08008200+len(data):raise ValueError('Invalid vectors: application must be linked at 0x08008200')
    return hw,fw,stamp,data
def frame(command,data=b''):
    body=bytes([0xaa,command])+data
    return b'SLGT'+struct.pack('<H',len(body))+body+struct.pack('<H',crc16(body))
def upload(port,path):
    import serial
    hw,fw,stamp,data=application(path)
    with serial.Serial(port,115200,timeout=0.2,write_timeout=5) as dev:
        dev.reset_input_buffer()
        def exchange(cmd,payload=b'',timeout=30):
            dev.write(frame(cmd,payload));dev.flush();pending=bytearray();end=time.monotonic()+timeout
            while time.monotonic()<end:
                pending.extend(dev.read(128))
                pos=pending.find(b'SLGT')
                if pos<0:
                    pending[:]=pending[-3:];continue
                del pending[:pos]
                if len(pending)<6:continue
                size=struct.unpack_from('<H',pending,4)[0]
                if size<2 or size>592:del pending[0];continue
                if len(pending)<size+8:continue
                body=bytes(pending[6:6+size]);got=struct.unpack_from('<H',pending,6+size)[0];del pending[:size+8]
                if got!=crc16(body) or body[:2]!=bytes([0x55,cmd]):continue
                if len(body)!=10:raise RuntimeError('Unsupported bootloader response')
                if body[2]==7:raise RuntimeError('Unsupported MCU Flash size: this build requires a factory-reported 128 KiB device')
                if body[2]:raise RuntimeError(f'Bootloader status {body[2]} for command {cmd:02x}')
                return body
            raise TimeoutError(f'No response to command {cmd:02x}; re-enter boot mode and restart upload')
        info=exchange(0)
        if info[3]<0x31 or info[4]!=hw:raise ValueError('Wrong bootloader version or hardware ID')
        exchange(0x20,struct.pack('<II',len(data),crc32(data))+path.name.encode('ascii')+b'\0')
        for offset in range(0,len(data),256):
            exchange(0x21,struct.pack('<I',offset)+data[offset:offset+256],5)
        exchange(0x22)
        print('Image accepted; installation follows. Verify application registration and firmware version on the device.')
def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('inspect');p.add_argument('image',type=Path)
    p=sub.add_parser('legacy');p.add_argument('image',type=Path);p.add_argument('output',type=Path)
    p=sub.add_parser('embed');p.add_argument('bootloader',type=Path);p.add_argument('output',type=Path)
    p=sub.add_parser('upload');p.add_argument('port');p.add_argument('image',type=Path)
    a=parser.parse_args()
    if a.command=='upload':upload(a.port,a.image);return
    if a.command=='embed':
        data=a.bootloader.read_bytes()
        if len(data)!=32768 or data[0x7ffc]!=0x31:raise ValueError('Expected padded 32 KiB new bootloader')
        text='#include <stdint.h>\n#ifdef __ICCARM__\n#pragma data_alignment=4\n#else\n__attribute__((aligned(4)))\n#endif\nstatic const uint8_t domovoi_boot_image[32768] = {\n'
        text+=''.join(','.join(f'0x{x:02x}' for x in data[i:i+16])+',\n' for i in range(0,len(data),16))
        text+='};\n#define DOMOVOI_BOOT_IMAGE_CRC 0x%08xu\n'%crc32(data)
        a.output.write_text(text);return
    hw,fw,stamp,data=application(a.image)
    if a.command=='inspect':print(f'HW={hw}, FW={fw}, build={stamp}, size={len(data)}, CRC=0x{crc32(data):08x}');return
    if fw>255:raise ValueError('Old loader supports only an 8-bit firmware version')
    header=bytearray(b'\xff'*512)
    struct.pack_into('<IBHBII',header,0,0x57464c53,hw,fw,0,len(data),crc32(data))
    struct.pack_into('<I',header,20,stamp)
    crypt=0x5a;encoded=bytearray()
    for b in data:
        encoded.append(((b-fw)&255)^crypt);crypt=(crypt+b)&255
    a.output.write_bytes(header+encoded)
    print('Legacy encrypted package written:',a.output)
if __name__=='__main__':main()
