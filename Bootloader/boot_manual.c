/* Разработчик: Кряжев А., 2026 */
#include "boot_manual.h"
#include "boot_core.h"
#include <string.h>
static uint8_t session,armed,target_hw;
static uint32_t received,expected_crc;
static uint32_t get32(const uint8_t *p) {
 return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t crc_word(uint32_t c,uint32_t w) {
 unsigned i;c^=w;for(i=0;i<32;i++)c=(c<<1)^((c&0x80000000u)?0x04c11db7u:0u);return c;
}
static int validate(void) {
 uint8_t b[256];uint32_t pos,i,sp,pc,c=0xffffffffu;
 if(!manual_platform_supported())return MANUAL_DENIED;
 for(pos=0;pos<MANUAL_BOOT_SIZE;pos+=sizeof(b)) {
  if(!manual_stage_read(pos,b,sizeof(b)))return MANUAL_IO;
  if(!pos) {
   sp=get32(b);pc=get32(b+4);
   if(sp<0x20000008u||sp>0x20005000u||(sp&7u)||!(pc&1u)||
      (pc&~1u)<0x08000000u||(pc&~1u)>=0x08007ffcu)return MANUAL_BAD;
  }
  if(pos+sizeof(b)==MANUAL_BOOT_SIZE &&
     (b[252]!=BOOT_VERSION||b[253]!=target_hw||b[254]!=0||b[255]!=0xff))return MANUAL_BAD;
  for(i=0;i<sizeof(b);i+=4)c=crc_word(c,get32(b+i));
  manual_service();
 }
 return c==expected_crc?MANUAL_OK:MANUAL_CRC;
}
int boot_manual_busy(void) {return session||armed;}
static void manual_command(int usb,const uint8_t *p,uint16_t n,int prepared,uint8_t reply[8]) {
 uint8_t op=n?p[0]:255,status=MANUAL_BAD,b[256];uint32_t off,i;uint16_t count;
 if(!usb){status=MANUAL_DENIED;goto done;}
 if(!n)goto done;
 if(op==4&&n==1){status=MANUAL_OK;goto done;}
 if(armed){status=MANUAL_BUSY;goto done;}
 if(op==3&&n==1){session=0;received=0;target_hw=0;status=MANUAL_OK;goto done;}
 if(op==0) {
  if(n!=10||get32(p+1)!=MANUAL_BOOT_SIZE||(p[9]!=11&&p[9]!=139&&p[9]!=140)||!manual_board_hw(p[9]))goto done;
  if(!manual_platform_supported()){status=MANUAL_DENIED;goto done;}
  session=0;received=0;target_hw=0;
  if(prepared < 0){status=MANUAL_IO;goto done;}
  for(off=0;!prepared && off<MANUAL_BOOT_SIZE;off+=4096) {
   if(!manual_stage_erase(off)){status=MANUAL_IO;goto done;}
   for(i=0;i<4096;i+=sizeof(b)) {
    unsigned j;if(!manual_stage_read(off+i,b,sizeof(b))){status=MANUAL_IO;goto done;}
    for(j=0;j<sizeof(b);j++)if(b[j]!=255){status=MANUAL_IO;goto done;}
   }
   manual_service();
  }
  expected_crc=get32(p+5);target_hw=p[9];session=1;status=MANUAL_OK;
 } else if(op==1) {
  if(!session||n<6||n>261)goto done;
  off=get32(p+1);count=(uint16_t)(n-5);
  if(off>MANUAL_BOOT_SIZE||count>MANUAL_BOOT_SIZE-off||off>received)goto done;
  if(off<received) {
   if(count>received-off)goto done;
   if(!manual_stage_read(off,b,count)){status=MANUAL_IO;goto done;}
   status=memcmp(b,p+5,count)?MANUAL_BAD:MANUAL_OK;goto done;
  }
  if(prepared < 0 || (!prepared && !manual_stage_write(off,p+5,count))||!manual_stage_read(off,b,count)||memcmp(b,p+5,count)) {
   session=0;status=MANUAL_IO;goto done;
  }
  received+=count;status=MANUAL_OK;
 } else if(op==2&&n==1) {
  if(!session||received!=MANUAL_BOOT_SIZE)goto done;
  status=(uint8_t)validate();if(status==MANUAL_OK){armed=1;session=0;}
 }
done:
 reply[0]=op;reply[1]=status;reply[2]=(uint8_t)received;reply[3]=(uint8_t)(received>>8);
 reply[4]=(uint8_t)(received>>16);reply[5]=(uint8_t)(received>>24);
 reply[6]=target_hw;reply[7]=armed;
}
int boot_manual_apply(void) {
 int result;
 if(!armed)return MANUAL_DENIED;
 armed=0;result=validate();if(result)return result;
 return manual_program(expected_crc)?MANUAL_OK:MANUAL_IO;
}

// Валидируем намерение до приёма памяти исполнителем; ошибочные команды идут прежним путём.
int boot_manual_prepare(const uint8_t *p, uint16_t n)
{
  if (!n || armed) return 0;
  if (p[0] == 0 && n == 10 && get32(p+1) == MANUAL_BOOT_SIZE &&
      (p[9] == 11 || p[9] == 139 || p[9] == 140) && manual_board_hw(p[9]) && manual_platform_supported()) return 1;
  if (p[0] == 1 && session && n >= 6 && n <= 261 && get32(p+1) == received &&
      get32(p+1) <= MANUAL_BOOT_SIZE && n-5u <= MANUAL_BOOT_SIZE-get32(p+1)) return 2;
  return 0;
}

void boot_manual_command(int usb, const uint8_t *p, uint16_t n, uint8_t reply[8])
{
  manual_command(usb, p, n, 0, reply);
}

void boot_manual_ready(const uint8_t *p, uint16_t n, int ok, uint8_t reply[8])
{
  manual_command(1, p, n, ok ? 1 : -1, reply);
}
