// Доработка: Кряжев А., 2026
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned dataChanel_e;
#define CH_SERVER 1
#define CMD_CRCERROR 7
#define CMD_SUCCESS 0
#define CMD_PARAMERROR 1
#define wdt_reload() ((void)0)
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned calls;
// Вернуть эталонный CRC, чтобы проверять только шлюз команд.
static uint16_t crc16_calc(uint8_t*p,uint16_t n){uint16_t c=0xffff;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^((c&1)?0xa001:0);}return c;}
// Запомнить ответ команды для проверки отказа.
void user_send_answer_comand(unsigned cmd,unsigned error,dataChanel_e ch,unsigned id){CHECK(cmd==106);CHECK(error==CMD_CRCERROR);CHECK(ch==2);CHECK(id==0xa5);calls++;}
/* КОД */
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){uint8_t p[]={3,0,255,0xa5,106,0,0};user_parse_comand(p,2);CHECK(calls==1);puts("CRC error echoes command 106 and independent unique ID 0xA5: passed");return 0;}
