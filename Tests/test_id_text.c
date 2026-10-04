#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static struct { uint8_t sourceGetID, timeToGetID, addressGetID; } device;
static char tempStrBuffer[128], expected[11];
static unsigned replies, controls, debug_lines;
static int current_type;
static void user_debug(char *p) { CHECK(strstr(p, expected)); debug_lines++; }
static unsigned events78;
static void user_send_readedIDtoServer(uint8_t *p, uint8_t state) { CHECK(state == 1); CHECK(!memcmp(p, expected, 10)); CHECK(events78 == replies + 1); replies++; }
/* Event 78: ID received on server request (command 6), written before packet 8. */
static bool log_addControlEvent(logControl_e num, char *id, uint8_t cnt) { CHECK(num == LOG_CON_RECVIDONREADER && !memcmp(id, expected, 10) && cnt == 0); events78++; return true; }
static void user_checkIdAndControl(char *p, int type) { CHECK(!memcmp(p, expected, 11)); current_type = type; controls++; }
static int checked_sprintf(char *p, size_t capacity, const char *format, ...)
{
    va_list args, copy;
    va_start(args, format); va_copy(copy, args);
    int length = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    CHECK(length >= 0 && (size_t)length < capacity);
    int result = vsprintf(p, format, args);
    va_end(args);
    return result;
}
#define DEBUG_USER
#define sprintf(p, ...) checked_sprintf(p, __builtin_object_size(p, 0), __VA_ARGS__)
/* PRODUCTION */
#undef sprintf
int main(void)
{
    static const uint32_t ids[] = {0, 0x12345678u, 0xffffffffu};
    unsigned i;
    for (i = 0; i < 3; i++) {
        snprintf(expected, sizeof(expected), "00%08X", ids[i]);
        device.sourceGetID = 1; device.timeToGetID = 5;
        WG_readedID(ids[i]); CHECK(current_type == CT_WIEGAND);
        CHECK(!device.sourceGetID && !device.timeToGetID);
        device.sourceGetID = 2; device.timeToGetID = 5;
        RF433_readedID(ids[i]); CHECK(current_type == CT_RF433);
        CHECK(!device.sourceGetID && !device.timeToGetID);
        device.sourceGetID = 3; device.addressGetID = 7; device.timeToGetID = 5;
        RFID_readedID(ids[i], 7); CHECK(current_type == CT_RFID);
        CHECK(!device.sourceGetID && !device.timeToGetID);
    }
    CHECK(replies == 9 && controls == 9 && debug_lines == 9 && events78 == 9);
    /* No request, other source or other RFID address: no event, no packet 8. */
    snprintf(expected, sizeof(expected), "00%08X", 5u);
    device.sourceGetID = 0; WG_readedID(5);
    device.sourceGetID = 1; RF433_readedID(5); RFID_readedID(5, 7);
    device.sourceGetID = 3; device.addressGetID = 8; RFID_readedID(5, 7);
    CHECK(replies == 9 && events78 == 9 && controls == 13 && device.sourceGetID == 3);
    puts("idtext: ID formatting fits including NUL, wire ID unchanged passed");
    return 0;
}
