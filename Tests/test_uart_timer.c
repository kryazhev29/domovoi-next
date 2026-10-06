// Доработка: Кряжев А., 2026. Один USART1: тишина, новый байт и переполнение.
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
/* TYPES */
static uart_t uart;
/* PRODUCTION */
static void check_delay(unsigned delay)
{
 uint8_t buffer[2]={0};memset(&uart,0,sizeof uart);
 uart.rxbuffer=buffer;uart.bfsize=sizeof buffer;uart.timetowait=delay;
 CHECK(uart_addbyte('A'));CHECK(uart_isReceiving(0));
 for(unsigned tick=1;tick<delay;tick++) {
  uart_timer1ms();CHECK(uart.twait==delay-tick);CHECK(uart_isReceiving(0));CHECK(!uart_getstate(0));
 }
 uart_timer1ms();CHECK(!uart_isReceiving(0)&&uart_getstate(0)==1);
 CHECK(uart_addbyte('B'));CHECK(uart_isReceiving(0)&&!uart_getstate(0));
 CHECK(!uart_addbyte('C')&&uart.rxoverflow&&uart.received==2);
 for(unsigned tick=0;tick<delay;tick++)uart_timer1ms();
 CHECK(!uart_isReceiving(0)&&uart_getstate(0)==1&&buffer[0]=='A'&&buffer[1]=='B');
}
int main(void){check_delay(1);check_delay(2);check_delay(10);puts("uarttimer: single-port idle gap, renewal and overflow PASS");return 0;}
