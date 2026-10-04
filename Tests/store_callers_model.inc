
static config_s config;
static device_s device;
static struct {uint32_t start,size;} mem[10];
static uint32_t now, log_lastIndex, usb_bus_resets;
static uint8_t log_pendingCount, sync_pending, sync_quiet;
static cellLog_s log_pending[16];
static unsigned frames, empty_frames, resets, acks, stamps, syncs;
static uint8_t frame[1200];
static uint8_t comm_buffer[1200];
static bool txbusy;
#define _1B(x) ((uint8_t)(x))
#define _2B(x) ((uint8_t)((x)>>8))
#define CE_OK 0
#define SYNC_CONFIG 1
#define SLM_IDLE 1
#define USB_PULLUP_OFF ((void)0)
#define LOG_TIME_WAIT 600
static uint32_t timers_getMillis(void){return now;}
static bool w25_read_available(void){return true;}
static void comm_modem_off(void){}
static void NVIC_SystemReset(void){resets++;}
static unsigned comm_modem_state(void){return 0;}
static void user_comm_boot(void){}
static bool comm_tx_busy(void){return txbusy;}
static void server_rx_reset(void){}
static void photo_connection_lost(void){}
static void rs485_set_config(const uint8_t *p){memcpy(&config,p,sizeof(config));}
static void sync_packet(unsigned k){(void)k;syncs++;}
static void user_send_answer_comand(unsigned c,unsigned r,dataChanel_e ch,unsigned id){(void)c;(void)r;(void)ch;(void)id;acks++;}
static void user_send_timeshtamp(shtamp_e c,uint32_t t,uint16_t r,dataChanel_e ch,unsigned id){(void)c;(void)t;(void)r;(void)ch;(void)id;stamps++;}
static uint16_t user_calcCRC16InFlash(uint32_t a,uint32_t n){return crc16_calc(flash+a,n);}
static void user_send_anyData(unsigned cmd,uint8_t *p,uint16_t n,dataChanel_e ch,unsigned id){(void)cmd;(void)ch;(void)id;frames++;if(n<=3&&!p[0]&&!p[1])empty_frames++;}
static uint8_t *user_comm_acquire(uint16_t n){CHECK(n<=sizeof(comm_buffer));return txbusy?NULL:comm_buffer;}
static void user_comm_free(uint8_t*p,dataChanel_e ch){if(ch==CH_USB)free(p);}
static bool user_comm_send(uint8_t*p,uint16_t n,dataChanel_e ch){(void)ch;if(txbusy)return false;frames++;CHECK(n<=sizeof(frame));memcpy(frame,p,n);return true;}
static bool user_writeConfigAndDevice(void);
static bool log_clearLogBuffer(void){CHECK(0);return false;}
static bool user_clearTempIdBuffer(void){CHECK(0);return false;}
static void upload_reset(uint8_t cmd);
static bool log_readRecords(uint16_t first,uint8_t*data,uint16_t count);
void user_send_logRecords(dataChanel_e,uint8_t);
void user_send_tempIDs(dataChanel_e,uint8_t);

static uint8_t usb_send(uint8_t*p,uint16_t n){return user_comm_send(p,n,CH_USB);}
static bool comm_tx_commit(uint16_t n){return user_comm_send(comm_buffer,n,CH_SERVER);}
