"""Check final relocation against the independent C kernels, without mutations."""
import hashlib,json,tempfile
from test_asm_crc import ROOT,Path,build,Machine,linked_firmware
path=ROOT/'Release/Exe/DomovoiNext.out'
with tempfile.TemporaryDirectory(prefix='domovoi-linked-crc-') as directory:
    reference=Machine(build(Path(directory)))
    count=linked_firmware(path,reference)
result=ROOT/'Tests/asm/runtime_final_crc.json'
data=json.loads(result.read_text())
data['linked_firmware_cases']=count
data['final_elf_sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
result.write_text(json.dumps(data,indent=2)+'\n')
print('PASS final CRC relocation:',count,'C/ASM comparisons, ABI, exact read bounds')
