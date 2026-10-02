/* Доработка: Кряжев А., 2026. CRC-16 хвоста из 0xFF без чтения совпадает с побайтовым расчётом. */
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
#include <stdint.h>
#include <stdbool.h>
#define MB_ID 0
typedef struct {uint8_t b[32];} cellID_s;
static struct {uint32_t start,size;} mem[1]={{4096,1601536}};
static uint8_t flash[4096+1601536];
static unsigned long read_bytes;
static bool mem_read(uint32_t a,uint8_t *p,uint16_t n){memcpy(p,flash+a,n);read_bytes+=n;return true;}
/* PRODUCTION */
int main(void){
  static unsigned char ff[70000];
  unsigned long ns[]={0,1,2,3,7,8,255,256,1000,1536,16000,65535,65536,69999,1601536};
  unsigned short inits[]={0xFFFF,0,1,0x1234,0x8000};
  memset(ff,0xFF,sizeof ff);
  for(unsigned i=0;i<sizeof ns/sizeof *ns;i++)for(unsigned k=0;k<5;k++){
    unsigned short ref=inits[k]; unsigned long n=ns[i];
    while(n){unsigned short c=n>60000?60000:(unsigned short)n; ref=crc16_calcInit(ff,c,ref); n-=c;}
    CHECK(ref==crc16_ffInit(ns[i],inits[k]));
  }
  /* Вся область базы против записанной части + хвоста 0xFF: тот же CRC, читается только записанное. */
  {static const uint32_t used[]={0,32,33*32,500*32,16000,16032,123457,1600000,1601536};
   static const uint32_t win[][2]={{0,1601536},{0,16000},{16000,16000},{1584000,16000},{16000*7,16000}};
   for(unsigned u=0;u<sizeof used/sizeof *used;u++){
    memset(flash,0xFF,sizeof flash);
    for(uint32_t i=0;i<used[u];i++)flash[4096+i]=(uint8_t)(i*131+7);
    id_used_bytes=used[u];
    for(unsigned w=0;w<sizeof win/sizeof *win;w++){
     uint32_t off=win[w][0],n=win[w][1];
     unsigned short ref=crc16_calcInit(flash+4096+off,(unsigned short)(n>60000?0:n),0xFFFF);
     if(n>60000){ref=0xFFFF;for(uint32_t k=0;k<n;){uint32_t c=n-k>60000?60000:n-k;ref=crc16_calcInit(flash+4096+off+k,(unsigned short)c,ref);k+=c;}}
     read_bytes=0;
     CHECK(id_crc16(off,n)==ref);
     CHECK(read_bytes==(off>=used[u]?0:(used[u]-off<n?used[u]-off:n)));
    }}
   id_used_bytes=0xFFFFFFFFu;read_bytes=0;CHECK(id_crc16(0,16000)==crc16_calcInit(flash+4096,16000,0xFFFF)&&read_bytes==16000);
  }
  puts("crcff: base CRC with unread erased tail equals full read for 9 fill levels; 0xFF tail CRC matches bytewise CRC for 15 lengths and 5 initial values passed");return 0;}
