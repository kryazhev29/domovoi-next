#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
typedef struct { uint8_t Current_Configuration; } DEVICE_INFO;
static DEVICE_INFO Device_Info;
static struct { uint32_t VTOR; } scb;
#define SCB (&scb)
enum { ATTACHED = 1, CONFIGURED = 5 };
static uint8_t bDeviceState;
static unsigned clock_restores;
static void SystemInit(void) { clock_restores++; SCB->VTOR = 0x08000000u; }
/* PRODUCTION */
int main(void)
{
    static const uint32_t vectors[] = {0x08008200u, 0x08000000u};
    unsigned i, configured;
    for (i = 0; i < 2; i++) {
        for (configured = 0; configured < 2; configured++) {
            SCB->VTOR = vectors[i];
            Device_Info.Current_Configuration = (uint8_t)configured;
            Leave_LowPowerMode();
            CHECK(SCB->VTOR == vectors[i]);
            CHECK(bDeviceState == (configured ? CONFIGURED : ATTACHED));
        }
    }
    CHECK(clock_restores == 4);
    puts("usbresume: application and bootloader vector tables preserved passed");
    return 0;
}
