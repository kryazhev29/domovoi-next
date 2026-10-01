#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
typedef uint8_t Int8U;
typedef uint16_t Int16U;
/* TYPES */
static uart_t uart[UARTS];
/* Let the old source reach the runtime assertion that reproduces B18. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmisleading-indentation"
/* PRODUCTION */
#pragma clang diagnostic pop
static void check_delay(unsigned port, unsigned delay)
{
    uint8_t buffer[16] = {0};
    memset(uart, 0, sizeof(uart));
    uart[port].rxbuffer = buffer; uart[port].bfsize = sizeof(buffer);
    uart[port].timetowait = delay;
    CHECK(uart_addbyte(port, 'A')); CHECK(uart_isReceiving(port));
    for (unsigned tick = 1; tick < delay; tick++) {
        uart_timer1ms(); CHECK(uart[port].twait == delay - tick);
        CHECK(uart_isReceiving(port)); CHECK(!uart_getstate(port));
        CHECK(uart[port].received == 1 && buffer[0] == 'A');
    }
    uart_timer1ms(); CHECK(!uart[port].twait);
    CHECK(!uart_isReceiving(port) && uart_getstate(port) == 1);
    uart_timer1ms(); CHECK(!uart_isReceiving(port) && uart_getstate(port) == 1);
}
int main(void)
{
    for (unsigned port = 0; port < UARTS; port++) {
        check_delay(port, 1); check_delay(port, 2); check_delay(port, 10);
    }
    uint8_t buffers[UARTS][16] = {{0}};
    memset(uart, 0, sizeof(uart));
    for (unsigned port = 0; port < UARTS; port++) {
        uart[port].rxbuffer = buffers[port]; uart[port].bfsize = sizeof(buffers[port]);
        uart[port].timetowait = port + 2; CHECK(uart_addbyte(port, 'A' + port));
    }
    uart_timer1ms(); CHECK(uart_isReceiving(0) && uart_isReceiving(1) && uart_isReceiving(2));
    CHECK(uart_addbyte(0, 'Z'));
    uart_timer1ms(); CHECK(uart_isReceiving(0) && uart_isReceiving(1) && uart_isReceiving(2));
    uart_timer1ms(); CHECK(!uart_isReceiving(0) && !uart_isReceiving(1) && uart_isReceiving(2));
    CHECK(uart_getstate(0) == 1 && uart_getstate(1) == 1 && !uart_getstate(2));
    CHECK(uart[0].received == 2 && buffers[0][1] == 'Z');
    uart_timer1ms(); CHECK(!uart_isReceiving(2) && uart_getstate(2) == 1);
    uart[0].twait = 0; uart[0].receiving = 1; uart[0].state = 0;
    uart_timer1ms(); CHECK(uart_isReceiving(0) && !uart_getstate(0));
    CHECK(!uart_isReceiving(UARTS) && !uart_getstate(UARTS));
    puts("uarttimer: receiving stays set until the configured idle gap, renewed by each byte passed");
    return 0;
}
