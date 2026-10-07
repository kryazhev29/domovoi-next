"""Доработка: Кряжев А., 2026.
Сравнение десятичного форматирования C с независимым эталоном.
Проверяются границы чисел, длина и прежний интерфейс сборки строк.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
C=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "user_format.h"
static void number(uint32_t value) {
  char got[32],expected[32];
  memset(got,0x5a,sizeof got);
  sprintf(expected,"00%08" PRIX32,value);
  if(format_id(got,0,value)!=10 || strcmp(got,expected) || got[11]!=0x5a) __builtin_trap();
  memset(got,0x5a,sizeof got);
  sprintf(expected,"%" PRIu32,value);
  if(format_u32(got,0,value)!=strlen(expected) || strcmp(got,expected) || got[strlen(expected)+1]!=0x5a) __builtin_trap();
  memset(got,0x5a,sizeof got);
  sprintf(expected,"%" PRId32,(int32_t)value);
  if(format_i32(got,0,(int32_t)value)!=strlen(expected) || strcmp(got,expected) || got[strlen(expected)+1]!=0x5a) __builtin_trap();
}
static const char *net[]={"Not network","gprs","edge","3G","4G","5G"};
static void status(unsigned type,unsigned signal,unsigned fw,unsigned connected,unsigned server,unsigned mask) {
  char got[128],expected[128];
  const char *network=net[type<6?type:0];
  memset(got,0x5a,sizeof got);
  sprintf(expected,"STATUS:\r\n-SIGNAL: %d\r\n-NET: %s\r\n-CLOUD: %d/%d\r\n-FW: %d\r\n-TRIG: %d/%d/%d/%d",
    signal,network,connected,server,fw,mask&1,(mask>>1)&1,(mask>>2)&1,(mask>>3)&1);
  if(format_status(got,0,signal,network,connected,server,fw,mask&1,(mask>>1)&1,(mask>>2)&1,(mask>>3)&1)!=strlen(expected)
     || strcmp(got,expected) || got[strlen(expected)+1]!=0x5a) __builtin_trap();
}
int main(void) {
  uint32_t x=1,p=1;unsigned cases=0;
  number(0);number(UINT32_MAX);number(INT32_MAX);number((uint32_t)INT32_MIN);
  for(unsigned i=0;i<32;i++) { number(x-1);number(x);number(x+1);x<<=1; }
  for(unsigned i=0;i<10;i++) { number(p-1);number(p);number(p+1);p*=10; }
  x=1234567;
  for(unsigned i=0;i<100000;i++) { x=x*1664525u+1013904223u;number(x); }
  const unsigned firmware[]={0,1,9,10,255,256,65534,65535};
  for(unsigned type=0;type<256;type++) for(unsigned signal=0;signal<256;signal++) for(unsigned mask=0;mask<16;mask++) {
    status(type,signal,firmware[(type+signal+mask)&7],(mask>>1)&1,1+(mask&1),mask);cases++;
  }
  for(unsigned fw=0;fw<65536;fw++) for(unsigned mask=0;mask<16;mask++) {
    status(fw%6,(fw&1)?0:255,fw,(mask>>1)&1,1+(mask&1),mask);cases++;
  }
  printf("PASS exact formats: %u STATUS cases; u32/i32/hex boundaries and 100000 samples\n",cases);
  return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix='domovoi-format-') as tmp:
        folder=Path(tmp);source=folder/'format_test.c';exe=folder/'format_test.exe'
        source.write_text(C,encoding='utf-8')
        zig=os.environ.get('DOMOVOI_ZIG',str(Path(__import__('tempfile').gettempdir())/'codex-zig/zig.exe'))
        command=[zig,'cc'] if Path(zig).name.lower()=='zig.exe' else [zig]
        subprocess.run([*command,'-std=c99','-O2','-Wall','-Wextra','-Werror',
                        '-I'+str(ROOT/'User'),str(source),str(ROOT/'User/user_format.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)


if __name__=='__main__':main()
