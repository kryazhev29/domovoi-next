#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* TYPES */
#define DEBUG_USER
#define OUT1 0
#define OUT2 1
#define OUT_ON 1
static char tempStrBuffer[128];
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
static void user_debug(char *p) { (void)p; }
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
/* PRODUCTION */
static void init(void)
{
    memset(tempStrBuffer, 0, sizeof(tempStrBuffer)); memset(&config, 0, sizeof(config));
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
int main(void)
{
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
    return 0;
}
