#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
/* рхош опнрнйнкю */
static cellMem_s mem[16];
static struct {uint16_t numbersCount;} device;
static unsigned fail_erase, erases, successes, errors, counts;
static bool mem_erase_sector(uint32_t a){(void)a;erases++;return erases!=fail_erase;}
static void wdt_reload(void){}
static uint16_t user_getNumbersCount(void){counts++;return 0;}
static void user_send_anyData(unsigned cmd,void *p,unsigned n,dataChanel_e ch,unsigned id){CHECK(cmd==103&&!p&&!n&&ch==CH_USB&&id==42);successes++;}
static void user_send_answer_comand(unsigned cmd,cmdCode_e code,dataChanel_e ch,unsigned id){CHECK(cmd==103&&code==CMD_RUNERROR&&ch==CH_USB&&id==42);errors++;}
/* йнд лндскъ */
int main(void){uint8_t p[5]={0,0,0,0,103};unsigned f;(void)user_send_answer_comand;mem[MB_ID]=(cellMem_s){4096,8192};
 for(f=1;f<=2;f++){erases=successes=errors=counts=0;fail_erase=f;device.numbersCount=7;
 command(p,CH_USB,42);CHECK(errors==1&&!successes&&!counts&&device.numbersCount==7);CHECK(erases==f);}
 erases=successes=errors=counts=0;fail_erase=0;command(p,CH_USB,42);CHECK(!errors&&successes==1&&counts==1&&erases==2);
 puts("flashcmds: command 103 stops on erase failure, no false success passed");return 0;}
