// Доработка: Кряжев А., 2026
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
// Снимок таймера сохраняет PRIMASK и не теряет ожидающий тик.
// Проверяются перенос времени и остаток при извлечении полных периодов.
/* Таймеры передают main снимок под сохранённым PRIMASK. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t irqmask;
static bool pending;
void timers_handle(void);
unsigned int timers_getMillis(void);
// Прочитать маску прерываний модели.
uint32_t __get_PRIMASK(void){return irqmask;}
// Запретить прерывания в модели.
void __disable_irq(void){irqmask=1;}
// Восстановить маску и доставить ожидающий тик модели.
void __set_PRIMASK(uint32_t mask){irqmask=mask;if(pending && !mask){pending=false;timers_handle();}}
// Имитировать пользовательский обработчик миллисекунды.
void timers_user_handle(void){}
// Питание проверяется отдельно в ELF; здесь моделируется зависимость тика.
void comm_power_v5_poll(uint32_t now){(void)now;}
// Имитировать очередной тик во время задержки.
void wdt_reload(void){timers_handle();}
/* КОД */
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){
  timers_set(0,9);CHECK(timers_get(0)==9);timers_handle();CHECK(timers_get(0)==10 && timers_getMillis()==1);
  irqmask=1;timers_set(0,3);CHECK(irqmask==1 && timers_get(0)==3 && irqmask==1 && timers_getMillis()==1 && irqmask==1);irqmask=0;
  pending=true;CHECK(timers_take(0,2)==1 && timer[0]==2 && !pending);
  milliSeconds=0xfffffffeu;timers_pause(5);CHECK(timers_getMillis()==3);
  timers_set(4,2500);CHECK(timers_take(4,1000)==2 && timers_get(4)==500);
  puts("timersirq: snapshot, pending tick, saved PRIMASK and pause wrap PASS");return 0;
}
