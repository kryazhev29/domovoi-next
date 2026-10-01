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
{ CHECK(type == CT_OTHER && !strcmp(id, "0123456789")); added++; }
static bool log_addControlEvent(logControl_e event, char *id, uint8_t cnt)
{ CHECK(event == LOG_CON_IDADDBYSMS && !strcmp(id, "0123456789") && cnt == 0); controls++; return true; }
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
#define sprintf(p, ...) checked_sprintf(p, __builtin_object_size(p, 1), __VA_ARGS__)
/* PRODUCTION */
#undef sprintf
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
int main(int argc, char **argv)
{
    if (argc > 1) { CHECK(!strcmp(argv[1], "debug")); test_debug(); return 0; }
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
    return 0;
}
