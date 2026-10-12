/* Доработка: Кряжев А., 2026. Сценарии логики на модели NOR.
   Данные и отказы задаются стендом; проверяется реальный код модуля.
   Проверки не используют физическую плату и сеть. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
/* ТИПЫ ПРОТОКОЛА */
#define DEBUG_USER
#define OUT1 0
#define OUT2 1
static struct { uint8_t before[16]; char bytes[128]; uint8_t after[16]; } scratch;
#define tempStrBuffer scratch.bytes
static char debug_text[128];
static unsigned recv_debug_lines;
static struct { uint32_t pinCode; struct { bool onCmdServer; } rele[2]; uint8_t useServer1, useServer2; } config;
static struct {
    char IMEIGSM[16]; unsigned csq, connected, firmwareVersion, timeToReset, netType;
    uint16_t balance;
    struct { uint8_t handUnBlock, sheduleUnBlock, autoregistration, openAll; bool smsToAdmin; } triggers;
} device;
/* Адрес канала связи (0 - основной, 1 - резервный) и события журнала без SMS. */
static uint8_t server_address;
static uint8_t comm_server_address(void) { return server_address; }
static unsigned value_events, plain_events; static logSystem_e last_event; static char value_id[11];
static bool log_refuse;
static bool log_addSystemEvent(logSystem_e event) { if (log_refuse) return false; last_event = event; plain_events++; return true; }
static bool quiet, add_ok, flash_busy;
static bool flash_exec_idle(void) { return !flash_busy; }
static unsigned sent, received_events, sent_events, added, controls, resets, outputs, forwarded;
static bool sms_log_full;
static unsigned last_output;
static char expected_added_id[11] = "0123456789";
static uintptr_t input_begin, input_end;
static char sent_number[128], sent_message[128], recv_id[128], send_id[128], forwarded_text[128];
static bool fw_update_quiet(void) { return quiet; }

static bool user_comm_sms(const char *num, const char *mes)
{
    CHECK(strlen(num) < sizeof(sent_number)); CHECK(strlen(mes) < sizeof(sent_message));
    strcpy(sent_number, num); strcpy(sent_message, mes); sent++;
    return true;
}
static bool log_addSystemEventID(logSystem_e event, char *id)
{
    if (event == LOG_SYS_RECVSMS) {
        if (sms_log_full) return false;
        memcpy(recv_id, id, 10); recv_id[10]=0; received_events++;
    }
    else if (event == LOG_SYS_LOWBALANCE || event == LOG_SYS_LOWSIGNAL) {
        /* Поле ID - 10 байт: число текстом, остаток нули. */
        last_event = event; memcpy(value_id, id, 10); value_id[10] = 0; value_events++;
        for (unsigned i = strlen(value_id); i < 10; i++) CHECK(id[i] == 0);
    }
    else { CHECK(event == LOG_SYS_SENDSMS); strcpy(send_id, id); sent_events++; }
    return true;
}
static bool user_addIdToTempBuffer(char *id, codeType_e type)
{
    uintptr_t address = (uintptr_t)id;
    if (address >= input_begin && address < input_end) CHECK(input_end - address >= 10);
    CHECK(type == CT_OTHER && !memcmp(id, expected_added_id, 10));
    if (!add_ok) return false;
    added++;
    return true;
}
static bool log_addControlEvent(logControl_e event, char *id, uint8_t cnt)
{ CHECK(event == LOG_CON_IDADDBYSMS && !memcmp(id, expected_added_id, 10) && cnt == 0); controls++; return true; }
static bool store_factory_request(uint8_t channel, uint8_t uniq, uint8_t delay, const char *number)
{ CHECK(channel == 2 && uniq == 0 && delay == 0 && strlen(number) > 0); resets++; return true; }
static bool user_activateOut(bool out1, bool out2, codeType_e type, char *id)
{
    CHECK(type == CT_SERVER && id == NULL && out1 != out2);
    unsigned out = out2 ? 1 : 0;
    if (!config.rele[out].onCmdServer) return false;
    last_output = out;
    outputs++;
    return true;
}
static bool sms_slot_busy;
static bool user_send_smsToServer(char *data)
{ if(sms_slot_busy)return false; CHECK(strlen(data) < sizeof(forwarded_text)); strcpy(forwarded_text, data); forwarded++; return true; }

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

#define strcat(p, s) checked_strcat(p, __builtin_object_size(p, 1), s, __builtin_object_size(s, 1))
/* КОД МОДУЛЯ */
#define sms_received_call(n,d,first) user_smsreceived(n,d,(bool[]){first})
#undef sprintf
#undef strcat
static void init(void)
{
    memset(tempStrBuffer, 0, sizeof(tempStrBuffer)); memset(&config, 0, sizeof(config));
    memset(scratch.before, 0xa7, sizeof(scratch.before)); memset(scratch.after, 0xa7, sizeof(scratch.after));
    recv_debug_lines = 0; debug_text[0] = 0;
    memset(&device, 0, sizeof(device)); config.pinCode = 1234u;
    strcpy(device.IMEIGSM, "123456789012345"); device.csq = 17; device.connected = 1;
    device.firmwareVersion = 71; device.netType = 4; config.rele[0].onCmdServer = config.rele[1].onCmdServer = true;
    config.useServer1 = config.useServer2 = 1; server_address = 0;
    value_events = plain_events = 0; log_refuse = false;
    sent = received_events = sent_events = added = controls = resets = outputs = forwarded = 0;
    sms_log_full=false;
    quiet = flash_busy = false; add_ok = true; last_output = 255;
    strcpy(expected_added_id, "0123456789"); input_begin = input_end = 0;
}
static void run_case(const char *command, const char *reply, unsigned mode)
{
    char number[] = "+79031234567", message[100];
    init(); CHECK(strlen(command) < sizeof(message)); strcpy(message, command);
    quiet = mode == 1; if (mode == 2) config.rele[0].onCmdServer = false;
    if (mode == 3) add_ok = false;
    device.triggers.smsToAdmin = true;
    sms_received_call(number, message,true);
    CHECK(received_events == 1 && !strcmp(recv_id, "9031234567"));
    /* Пакет 7 - только SMS, которые не команды (лист «SMS», таблица 03.10). */
    CHECK(forwarded == (command[0] != '#'));
    if (forwarded) CHECK(!strcmp(forwarded_text, message));
    CHECK(sent == (reply != NULL) && sent_events == sent);
    if (reply) {
        CHECK(!strcmp(sent_number, number)); CHECK(!strcmp(sent_message, reply));
        CHECK(!strcmp(send_id, "9031234567"));
    }
}
/* STATUS: тип сети словами, C/x - связь и номер сервера, триггеры E1-E4 - флаги команды 5 по порядку. */
static void status_case(unsigned net, unsigned connected, uint8_t address, uint8_t use1, const char *net_text, const char *cloud)
{
    char number[] = "+79031234567", message[] = "#status 1234,", expected[128];
    init(); device.netType = net; device.connected = connected; server_address = address; config.useServer1 = use1;
    device.triggers.handUnBlock = 1; device.triggers.autoregistration = 1; device.triggers.openAll = 0;
    sms_received_call(number, message,true);
    snprintf(expected, sizeof(expected), "STATUS:\r\n-SIGNAL: 17\r\n-NET: %s\r\n-CLOUD: %s\r\n-FW: 71\r\n-TRIG: 1/0/1/0", net_text, cloud);
    CHECK(sent == 1 && !strcmp(sent_message, expected));
}
static void test_status(void)
{
    status_case(0, 0, 0, 1, "Not network", "0/1");
    status_case(1, 1, 0, 1, "gprs", "1/1");
    status_case(3, 1, 1, 1, "3G", "1/2");
    status_case(4, 1, 0, 0, "4G", "1/2"); /* сервер 1 выключен: основной адрес - сервер 2 */
    status_case(9, 1, 0, 1, "Not network", "1/1");
}
/* Пользовательская часть журнала связи: 11, 12, 14-19 по виду и номеру сервера, 37 и 38 с числом. */
static void test_journal(void)
{
    static const struct { uint8_t kind, address, use1; logSystem_e event; } links[] = {
        {COMM_LINK_UP, 0, 1, 14}, {COMM_LINK_UP, 1, 1, 15}, {COMM_LINK_UP, 0, 0, 15},
        {COMM_LINK_FAIL, 0, 1, 16}, {COMM_LINK_FAIL, 1, 1, 17},
        {COMM_LINK_LOST, 0, 1, 18}, {COMM_LINK_LOST, 1, 1, 19},
        {COMM_LINK_NO_NET, 0, 1, 11}, {COMM_LINK_NO_NET, 1, 1, 11}, {COMM_LINK_NO_INTERNET, 0, 1, 12},
    };
    for (unsigned i = 0; i < sizeof(links) / sizeof(links[0]); i++) {
        init(); config.useServer1 = links[i].use1;
        CHECK(user_comm_link(links[i].kind, links[i].address) && plain_events == 1 && last_event == links[i].event);
    }
    init(); log_refuse = true; CHECK(!user_comm_link(COMM_LINK_FAIL, 0));
    /* 37: баланс ниже 100 - сумма текстом (с минусом), в статус 2 - 1..65000. */
    init(); user_comm_balance(-12); CHECK(value_events == 1 && last_event == LOG_SYS_LOWBALANCE && !strcmp(value_id, "-12") && device.balance == 1);
    init(); user_comm_balance(99); CHECK(value_events == 1 && !strcmp(value_id, "99") && device.balance == 99);
    init(); user_comm_balance(100); CHECK(!value_events && device.balance == 100);
    init(); user_comm_balance(70000); CHECK(!value_events && device.balance == 65000);
    init(); user_comm_balance(-9999999); CHECK(value_events == 1 && !strcmp(value_id, "-9999999"));
    init(); user_comm_low_signal(9); CHECK(value_events == 1 && last_event == LOG_SYS_LOWSIGNAL && !strcmp(value_id, "9"));
    puts("comm journal: events 11, 12, 14-19 by server number, 37 balance text, 38 signal text passed");
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
            sms_received_call(number, message,true);
            CHECK(recv_debug_lines == 0);
            for (unsigned k = 0; k < sizeof(scratch.before); k++) CHECK(scratch.before[k] == 0xa7);
            for (unsigned k = 0; k < sizeof(scratch.after); k++) CHECK(scratch.after[k] == 0xa7);
            CHECK(received_events == 1 && !strcmp(recv_id, "9031234567"));
            CHECK(!forwarded && sent == (mode != 0) && sent_events == sent);
            if (mode) {
                CHECK(!strcmp(sent_number, number) && !strcmp(send_id, "9031234567"));
                CHECK(!strcmp(sent_message, mode == 1 ? "IMEI: 123456789012345" : "IMEI: Error \"Pin not valid\""));
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
        char number[] = "+79031234567", expected[64];
        if (length == 10) snprintf(expected, sizeof(expected), "ADD: %s - Ok", expected_added_id);
        else strcpy(expected, "ADD: Error \"Format not correct\"");
        sms_received_call(number, message,true);
        CHECK(added == (length == 10) && controls == added && sent == 1);
        CHECK(!strcmp(sent_message, expected) && !strcmp(sent_number, number));
        CHECK(!strcmp(send_id, "9031234567"));
        for (unsigned j = 0; j < sizeof(scratch.after); j++) CHECK(scratch.after[j] == 0xa7);
        free(message); input_begin = input_end = 0;
    }
    puts("smsadd: exactly 10 digits accepted; short arguments rejected within bounds PASS");
}
// Те же разбор и проверка, что в прошивке: длина, ведущие нули и неверный IMEI.
static void test_pin(void)
{
    static const struct { uint32_t pin; const char *text; bool valid; } cases[] = {
        {4321, "4321", true}, {4321, "04321", true}, {20140, "20140", true},
        {99999, "99999", true}, {0, "0000", true}, {0, "00000", true},
        {432, "432", false}, {4321, "004321", false}, {4321, "43a1", false},
        {0, "", false}, {4321, "48074", true}, {4321, "48075", false}
    };
    char number[] = "+79031234567", message[32];
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        init(); config.pinCode = cases[i].pin;
        strcpy(device.IMEIGSM, "867442070217472");
        snprintf(message, sizeof(message), "#imei %s,", cases[i].text);
        CHECK(sms_received_call(number, message,true));
        CHECK(sent == 1 && !strcmp(sent_message, cases[i].valid ?
              "IMEI: 867442070217472" : "IMEI: Error \"Pin not valid\""));
    }
    init(); config.pinCode = 4321u; device.IMEIGSM[0] = 0;
    strcpy(message, "#imei 32768,"); CHECK(sms_received_call(number, message,true));
    CHECK(!strcmp(sent_message, "IMEI: Error \"Pin not valid\""));
    strcpy(message, "#imei 48074,"); CHECK(sms_received_call(number, message,true));
    CHECK(!strcmp(sent_message, "IMEI: Error \"Pin not valid\""));
}
int main(int argc, char **argv)
{
    if (argc > 1) {
        if (!strcmp(argv[1], "debug")) test_debug();
        else { CHECK(!strcmp(argv[1], "addbounds") || !strcmp(argv[1], "addshort")); test_add_bounds(!strcmp(argv[1], "addshort")); }
        return 0;
    }
    run_case("#add 1234,0123456789", "ADD: Error \"Update in progress\"", 1); CHECK(!added && !controls);
    run_case("#reset 1234,", "RESET: Error \"Update in progress\"", 1); CHECK(!device.timeToReset);
    run_case("#setout 1234,1", "SETOUT: Error \"Update in progress\"", 1); CHECK(!outputs);
    run_case("#default 1234,", "DEFAULT: Error \"Update in progress\"", 1); CHECK(!resets);
    run_case("#add 1234,0123456789", "ADD: 0123456789 - Ok", 0); CHECK(added == 1 && controls == 1);
    { char number[]="+79161234567",message[]="#add 4321,9990001234";
      init();config.pinCode=4321;strcpy(expected_added_id,"9990001234");
      CHECK(sms_received_call(number,message,true) && added==1 && controls==1 && received_events==1); }
    /* Пробел после запятой и в конце, CR перед LF - как с телефона. */
    { char number[]="+79161234567",message[]="#add 4321, 9990001234 \r\n";
      init();config.pinCode=4321;strcpy(expected_added_id,"9990001234");
      CHECK(sms_received_call(number,message,true) && added==1 && controls==1); }
    run_case("#add 1234,0123456789", "ADD: Error", 3); CHECK(!added && !controls);
    { char number[] = "+79031234567", message[] = "#add 1234,0123456789";
      init(); flash_busy = true;
      CHECK(!sms_received_call(number, message,true) && !added && !sent);
      flash_busy = false;
      CHECK(sms_received_call(number, message,false) && added == 1 && sent == 1 && received_events == 1); }
    run_case("#add 1234,01234567890", "ADD: Error \"Format not correct\"", 0); CHECK(!added && !controls);
    for (unsigned i = 0; i < 10; i++) {
        char text[] = "#add 1234,0123456789";
        text[10 + i] = 'a';
        run_case(text, "ADD: Error \"Format not correct\"", 0);
        CHECK(!added && !controls);
    }
    run_case("#add 1234", "ADD: Error \"Format not correct\"", 0); CHECK(!added && !controls);
    run_case("#imei 1234,", "IMEI: 123456789012345", 0);
    run_case("#status 1234,", "STATUS:\r\n-SIGNAL: 17\r\n-NET: 4G\r\n-CLOUD: 1/1\r\n-FW: 71\r\n-TRIG: 0/0/0/0", 0);
    run_case("#reset 1234,", "RESET: Go to reset", 0); CHECK(device.timeToReset == 5);
    run_case("#setout 1234,1", "SETOUT: Out 1 enabled", 0); CHECK(outputs == 1 && last_output == OUT1);
    run_case("#setout 1234,2", "SETOUT: Out 2 enabled", 0); CHECK(outputs == 1 && last_output == OUT2);
    run_case("#setout 1234,1", "SETOUT: Error, Out is blocked", 2); CHECK(!outputs);
    run_case("#setout 1234", "SETOUT: Error \"Format not correct\"", 0); CHECK(!outputs);
    run_case("#setout 1234,   2", "SETOUT: Out 2 enabled", 0); CHECK(outputs == 1 && last_output == OUT2);
    const char *bad_out[] = {"", "257", "1abc", "12", "01", "+1", "-255", "1,2", "1 2"};
    for (unsigned i = 0; i < sizeof(bad_out) / sizeof(bad_out[0]); i++) {
        char text[64];
        snprintf(text, sizeof(text), "#setout 1234,%s", bad_out[i]);
        run_case(text, "SETOUT: Error \"Format not correct\"", 0);
        CHECK(!outputs);
    }
    run_case("#setout 1234,3", "SETOUT: Error \"Format not correct\"", 0); CHECK(!outputs);
    run_case("#setout 1234,0", "SETOUT: Error \"Format not correct\"", 0); CHECK(!outputs);
    run_case("#default 1234,", NULL, 0); CHECK(resets == 1);
    run_case("#imei 9999,", "IMEI: Error \"Pin not valid\"", 0);
    run_case("#unknown 1234,", NULL, 0);
    run_case("#update 1234,", NULL, 0);
    run_case("plain text", NULL, 0);
    { char number[]="",message[]="";bool first=true;init();sms_log_full=true;
      CHECK(!user_smsreceived(number,message,&first) && first && !received_events);
      sms_log_full=false;CHECK(user_smsreceived(number,message,&first) && !first && received_events==1);
      for(unsigned i=0;i<10;++i)CHECK(!recv_id[i]); }
    {char number[]="+79031234567",message[]="text";unsigned before=forwarded,events=received_events;
     device.triggers.smsToAdmin=1;
     sms_slot_busy=true;CHECK(!sms_received_call(number,message,true)&&forwarded==before&&received_events==events+1);
     sms_slot_busy=false;CHECK(sms_received_call(number,message,false)&&forwarded==before+1&&received_events==events+1);}
    test_status();
    puts("sms: all reply sites preserve recipient, reply text and journal number passed");
    test_journal();
    test_debug();
    test_add_bounds(false);
    test_pin();
    return 0;
}
