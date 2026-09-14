import unittest, tempfile, struct, subprocess, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import boot_tool
class PackageTests(unittest.TestCase):
 def test_crc_vectors(self):
  self.assertEqual(boot_tool.crc16(b'123456789'),0x4b37)
  self.assertEqual(boot_tool.crc32(bytes(4)),0xc704dd7b)
 def test_legacy_roundtrip(self):
  with tempfile.TemporaryDirectory() as d:
   image=Path(d)/'139.14.1726300000.bin';out=Path(d)/'legacy.bin'
   original=struct.pack('<II',0x20005000,0x08008209)+bytes(range(255));image.write_bytes(original)
   subprocess.run([sys.executable,str(Path(boot_tool.__file__)),'legacy',str(image),str(out)],check=True,capture_output=True)
   package=out.read_bytes();magic,hw,fw,res,size,crc=struct.unpack_from('<IBHBII',package)
   self.assertEqual((magic,hw,fw),(0x57464c53,139,14));crypt=0x5a;decoded=bytearray()
   for c in package[512:]:
    b=((c^crypt)+fw)&255;decoded.append(b);crypt=(crypt+b)&255
   self.assertEqual(decoded[:len(original)],original);self.assertEqual(size,len(decoded));self.assertEqual(crc,boot_tool.crc32(decoded))
 def test_wrong_link_address(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'139.14.1.bin';p.write_bytes(struct.pack('<II',0x20005000,0x08000009))
   with self.assertRaises(ValueError):boot_tool.application(p)
 def test_embed_matches(self):
  import re
  with tempfile.TemporaryDirectory() as d:
   image=Path(d)/'boot.bin';header=Path(d)/'boot.h'
   data=bytearray(b'\xff'*32768);data[0x7ffc:]=bytes([0x31,139,0,255]);image.write_bytes(data)
   subprocess.run([sys.executable,str(Path(boot_tool.__file__)),'embed',str(image),str(header)],check=True,capture_output=True)
   text=header.read_text()
   packed=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})(?=,|\n)',text))
   expected=int(re.search(r'CRC 0x([0-9a-f]+)',text)[1],16)
   self.assertEqual(packed,bytes(data));self.assertEqual(boot_tool.crc32(packed),expected)
if __name__=='__main__':unittest.main()
