// Доработка: Кряжев А., 2026. Один USART1: тишина, новый байт и переполнение.
// Проверка контрактов периферии без физической платы.
// Зависимости моделируются, код проверяемого модуля вставляет стенд.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
/* ТИПЫ */
static uart_t uart;
/* КОД */
// Проверить окончание тишины, новый байт и насыщение буфера.
static void check_delay(unsigned delay)
{
 uint8_t buffer[2]={0};memset(&uart,0,sizeof uart);
 uart.rxbuffer=buffer;uart.bfsize=sizeof buffer;uart.timetowait=delay;
 CHECK(uart_addbyte('A'));CHECK(uart_isReceiving());
 for(unsigned tick=1;tick<delay;tick++) {
  uart_timer1ms();CHECK(uart.twait==delay-tick);CHECK(uart_isReceiving());CHECK(!uart_getstate());
 }
 uart_timer1ms();CHECK(!uart_isReceiving()&&uart_getstate()==1);
 CHECK(uart_addbyte('B'));CHECK(uart_isReceiving()&&!uart_getstate());
 CHECK(!uart_addbyte('C')&&uart.received==2);
 for(unsigned tick=0;tick<delay;tick++)uart_timer1ms();
 CHECK(!uart_isReceiving()&&uart_getstate()==1&&buffer[0]=='A'&&buffer[1]=='B');
}
// Проверить все сценарии модуля и вернуть результат прогона.
int main(void){check_delay(1);check_delay(2);check_delay(10);puts("uarttimer: single-port idle gap, renewal and overflow PASS");return 0;}
