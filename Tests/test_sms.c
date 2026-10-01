#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* TYPES */
#define DEBUG_USER
#define OUT1 0
#define OUT2 1
#define OUT_ON 1
static struct { uint8_t before[16]; char bytes[128]; uint8_t after[16]; } scratch;
#define tempStrBuffer scratch.bytes
static char debug_text[128];
static unsigned recv_debug_lines;
static struct { uint32_t pinCode; struct { bool onCmdServer; } rele[2]; } config;
static struct {
    char IMEIGSM[16]; unsigned csq, connected, firmwareVersion, timeToReset;
    struct { bool smsToAdmin; } triggers;
} device;
static bool quiet;
static unsigned sent, received_events, sent_events, added, controls, resets, outputs, forwarded;
static unsigned last_output;
static char expected_added_id[11] = "0123456789";
static uintptr_t input_begin, input_end;
static char sent_number[128], sent_message[128], recv_id[128], send_id[128], forwarded_text[128];
static bool fw_update_quiet(void) { return quiet; }
static void user_debug(char *p)
{
    if (!strncmp(p, ">>U SMS recv: ", 14)) {
        CHECK(strlen(p) < sizeof(debug_text)); strcpy(debug_text, p); recv_debug_lines++;
    }
}
static void gsm_sendsms(char *num, char *mes)
{
    CHECK(strlen(num) < sizeof(sent_number)); CHECK(strlen(mes) < sizeof(sent_message));
    strcpy(sent_number, num); strcpy(sent_message, mes); sent++;
}
static bool log_addSystemEventID(logSystem_e event, char *id)
{
    if (event == LOG_SYS_RECVSMS) { strcpy(recv_id, id); received_events++; }
    else { CHECK(event == LOG_SYS_SENDSMS); strcpy(send_id, id); sent_events++; }
    return true;
}
static void user_addIdToTempBuffer(char *id, codeType_e type)
{
    uintptr_t address = (uintptr_t)id;
    if (address >= input_begin && address < input_end) CHECK(input_end - address >= 10);
    CHECK(type == CT_OTHER && !memcmp(id, expected_added_id, 10)); added++;
}
static bool log_addControlEvent(logControl_e event, char *id, uint8_t cnt)
{ CHECK(event == LOG_CON_IDADDBYSMS && !memcmp(id, expected_added_id, 10) && cnt == 0); controls++; return true; }
static void user_factoryReset(void) { resets++; }
static void out_load(uint8_t out, uint8_t mode, uint16_t activ, uint16_t pause, uint8_t period)
{ CHECK(mode == OUT_ON && activ == 0 && pause == 0 && period == 0); last_output = out; outputs++; }
static void user_send_smsToServer(char *data)
{ CHECK(strlen(data) < sizeof(forwarded_text)); strcpy(forwarded_text, data); forwarded++; }
static int checked_sprintf(char *dest, size_t capacity, const char *format, ...)
{
    va_list args, copy;
    va_start(args, format); va_copy(copy, args);
    int required = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    CHECK(required >= 0 && (size_t)required < capacity);
    int result = vsprintf(dest, format, args);
    va_end(args);
    return result;
}
static size_t bounded_length(const char *p, size_t capacity)
{ size_t n = 0; while (n < capacity && p[n]) n++; return n; }
static char *checked_strcat(char *dest, size_t capacity, const char *source, size_t source_capacity)
{
    size_t source_len = bounded_length(source, source_capacity);
    CHECK(source_len < source_capacity);
    size_t used = bounded_length(dest, capacity);
    CHECK(used < capacity && source_len < capacity - used);
    memcpy(dest + used, source, source_len + 1);
    return dest;
}
#define sprintf(p, ...) checked_sprintf(p, __builtin_object_size(p, 1), __VA_ARGS__)
#define strcat(p, s) checked_strcat(p, __builtin_object_size(p, 1), s, __builtin_object_size(s, 1))
/* PRODUCTION */
#undef sprintf
#undef strcat
static void init(void)
{
    memset(tempStrBuffer, 0, sizeof(tempStrBuffer)); memset(&config, 0, sizeof(config));
    memset(scratch.before, 0xa7, sizeof(scratch.before)); memset(scratch.after, 0xa7, sizeof(scratch.after));
    recv_debug_lines = 0; debug_text[0] = 0;
    memset(&device, 0, sizeof(device)); memcpy(&config.pinCode, "1234", 4);
    strcpy(device.IMEIGSM, "123456789012345"); device.csq = 17; device.connected = 1;
    device.firmwareVersion = 71; config.rele[0].onCmdServer = config.rele[1].onCmdServer = true;
    sent = received_events = sent_events = added = controls = resets = outputs = forwarded = 0;
    quiet = false; last_output = 255;
    strcpy(expected_added_id, "0123456789"); input_begin = input_end = 0;
}
static void run_case(const char *command, const char *reply, unsigned mode)
{
    char number[] = "+79031234567", message[100];
    init(); CHECK(strlen(command) < sizeof(message)); strcpy(message, command);
    quiet = mode == 1; if (mode == 2) config.rele[0].onCmdServer = false;
    device.triggers.smsToAdmin = true;
    user_smsreceived(number, message);
    CHECK(received_events == 1 && !strcmp(recv_id, "9031234567"));
    CHECK(forwarded == 1 && !strcmp(forwarded_text, message));
    CHECK(sent == (reply != NULL) && sent_events == sent);
    if (reply) {
        CHECK(!strcmp(sent_number, number)); CHECK(!strcmp(sent_message, reply));
        CHECK(!strcmp(send_id, "9031234567"));
    }
}
static void test_debug(void)
{
    static const unsigned lengths[] = {0, 1, 110, 111, 112, 160, 2048};
    static const char *starts[] = {"", "#imei 1234,", "#imei 9999,"};
    char number[] = "+79031234567", message[2049], expected[128];
    for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        for (unsigned mode = 0; mode < sizeof(starts) / sizeof(starts[0]); mode++) {
            unsigned length = lengths[i];
            if (mode && length < strlen(starts[mode])) continue;
            init(); memset(message, 'X', length); memcpy(message, starts[mode], strlen(starts[mode]));
            message[length] = 0;
            unsigned shown = length < 111 ? length : 111;
            snprintf(expected, sizeof(expected), ">>U SMS recv: %.*s\r\n", (int)shown, message);
            user_smsreceived(number, message);
            CHECK(recv_debug_lines == 1 && !strcmp(debug_text, expected));
            for (unsigned k = 0; k < sizeof(scratch.before); k++) CHECK(scratch.before[k] == 0xa7);
            for (unsigned k = 0; k < sizeof(scratch.after); k++) CHECK(scratch.after[k] == 0xa7);
            CHECK(received_events == 1 && !strcmp(recv_id, "9031234567"));
            CHECK(!forwarded && sent == (mode != 0) && sent_events == sent);
            if (mode) {
                CHECK(!strcmp(sent_number, number) && !strcmp(send_id, "9031234567"));
                CHECK(!strcmp(sent_message, mode == 1 ? "imei: 123456789012345" : "imei: Error \"Pin not valid\""));
            }
        }
    }
    puts("smsdebug: 0..2048-byte SMS, 111-byte boundary, guards, CRLF and PIN behavior passed");
}
static void test_add_bounds(bool short_only)
{
    for (unsigned k = 0; k <= 10; k++) {
        unsigned length = k == 0 ? 10 : k - 1;
        if (short_only && length == 10) continue;
        init(); memset(expected_added_id, 0, sizeof(expected_added_id));
        memcpy(expected_added_id, "0123456789", length);
        size_t prefix = strlen("#add 1234,");
        char *message = malloc(prefix + length + 1); CHECK(message != NULL);
        memcpy(message, "#add 1234,", prefix); memcpy(message + prefix, expected_added_id, length + 1);
        input_begin = (uintptr_t)message; input_end = input_begin + prefix + length + 1;
        memset(&tempStrBuffer[106], 'Q', sizeof(tempStrBuffer) - 106);
        char number[] = "+79031234567", expected[64];
        snprintf(expected, sizeof(expected), "add: %s - Ok", expected_added_id);
        user_smsreceived(number, message);
        CHECK(added == 1 && controls == 1 && sent == 1);
        CHECK(!strcmp(sent_message, expected) && !strcmp(sent_number, number));
        CHECK(!strcmp(send_id, "9031234567"));
        for (unsigned j = 0; j < sizeof(scratch.after); j++) CHECK(scratch.after[j] == 0xa7);
        free(message); input_begin = input_end = 0;
    }
    puts("smsadd: 10-byte ID terminates; short arguments are copied and zero-padded within bounds passed");
}
int main(int argc, char **argv)
{
    if (argc > 1) {
        if (!strcmp(argv[1], "debug")) test_debug();
        else { CHECK(!strcmp(argv[1], "addbounds") || !strcmp(argv[1], "addshort")); test_add_bounds(!strcmp(argv[1], "addshort")); }
        return 0;
    }
    run_case("#add 1234,0123456789", "add: Error \"Update in progress\"", 1); CHECK(!added && !controls);
    run_case("#reset 1234,", "reset: Error \"Update in progress\"", 1); CHECK(!device.timeToReset);
    run_case("#setout 1234,1", "setout: Error \"Update in progress\"", 1); CHECK(!outputs);
    run_case("#default 1234,", "default: Error \"Update in progress\"", 1); CHECK(!resets);
    run_case("#add 1234,0123456789", "add: 0123456789 - Ok", 0); CHECK(added == 1 && controls == 1);
    run_case("#add 1234", "add: Error \"Format not correct\"", 0); CHECK(!added && !controls);
    run_case("#imei 1234,", "imei: 123456789012345", 0);
    run_case("#status 1234,", "status:\r\n-signal: 17\r\n-net: \r\n-cloud: 1\r\n-fw: 71\r\n-trig: ", 0);
    run_case("#reset 1234,", "reset: Go to reset", 0); CHECK(device.timeToReset == 5);
    run_case("#setout 1234,1", "setout: Out 1 enabled", 0); CHECK(outputs == 1 && last_output == OUT1);
    run_case("#setout 1234,2", "setout: Out 2 enabled", 0); CHECK(outputs == 1 && last_output == OUT2);
    run_case("#setout 1234,1", "setout: Error, Out 1 is blocked", 2); CHECK(!outputs);
    run_case("#setout 1234", "setout: Error \"Format not correct\"", 0); CHECK(!outputs);
    run_case("#default 1234,", "default: Complete", 0); CHECK(resets == 1);
    run_case("#imei 9999,", "imei: Error \"Pin not valid\"", 0);
    run_case("#unknown 1234,", NULL, 0);
    run_case("#update 1234,", NULL, 0);
    run_case("plain text", NULL, 0);
    puts("sms: all reply sites preserve recipient, reply text and journal number passed");
    test_debug();
    test_add_bounds(false);
    return 0;
}
