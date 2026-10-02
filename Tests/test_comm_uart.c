/* Доработка: Кряжев А., 2026. Модель регистров USART3, без платы. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define COMM_UART_STANDALONE 1
/* HEADER */
typedef struct { volatile uint32_t SR, DR, BRR, CR1, CR2, CR3; } USART_TypeDef;
static USART_TypeDef regs;
#define USART3 (&regs)
#define USART_SR_RXNE (1u<<5)
#define USART_SR_TXE (1u<<7)
#define USART_SR_TC (1u<<6)
#define USART_SR_ORE (1u<<3)
#define USART_SR_FE (1u<<1)
#define USART_SR_NE (1u<<2)
#define USART_CR1_TXEIE (1u<<7)
#define USART_CR1_TCIE (1u<<6)
#define USART_CR1_RXNEIE (1u<<5)
#define USART_CR3_EIE 1u
#define USART_HardwareFlowControl_None 0u
#define USART_HardwareFlowControl_CTS 0x200u
#define USART_Mode_Rx 4u
#define USART_Mode_Tx 8u
#define USART_IT_RXNE USART_CR1_RXNEIE
#define USART_IT_ERR USART_CR3_EIE
#define USART_FLAG_TC USART_SR_TC
#define GPIO_Pin_10 (1u<<10)
#define GPIO_Pin_11 (1u<<11)
#define GPIO_Pin_13 (1u<<13)
#define GPIO_Pin_14 (1u<<14)
#define GPIO_Mode_IPU 1u
#define GPIO_Mode_AF_PP 2u
#define GPIO_Mode_Out_PP 3u
#define GPIO_Speed_50MHz 50u
#define GPIOB 2u
#define RCC_APB2Periph_AFIO 1u
#define RCC_APB2Periph_GPIOB 2u
#define RCC_APB1Periph_USART3 4u
#define USART3_IRQn 39u
#define ENABLE 1u
#define DISABLE 0u
#ifndef DOMOVOI_APP_HW
#define DOMOVOI_APP_HW 139
#endif
typedef struct { uint32_t GPIO_Pin, GPIO_Mode, GPIO_Speed; } GPIO_InitTypeDef;
typedef struct { uint32_t USART_BaudRate, USART_Mode, USART_HardwareFlowControl; } USART_InitTypeDef;
typedef struct { uint8_t NVIC_IRQChannel, NVIC_IRQChannelPreemptionPriority, NVIC_IRQChannelSubPriority, NVIC_IRQChannelCmd; } NVIC_InitTypeDef;
static uint32_t primask, input_pins, output_pins, gpio_pins, apb1, apb2, flow;
static bool rts_stop;
static unsigned rts_writes;
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
static void set_rts(bool stop) { rts_stop=stop; rts_writes++; }
#define RTSG_HIGH set_rts(true)
#define RTSG_LOW set_rts(false)
#endif
/* Статические буферы будущих владельцев: движок AT и канал. */
static uint8_t at_command[128], frame[1040];
static unsigned reads, writes, barriers, edge_at;
static uint32_t pending_sr;
static uint8_t pending_byte;
static uint8_t wire[2048];
void USART3_IRQHandler(void);
static void interrupt_byte(uint8_t b, uint32_t flags) {
  regs.SR=flags; regs.DR=b; USART3_IRQHandler();
}
static uint32_t __get_PRIMASK(void) { return primask; }
static void __disable_irq(void) { primask=1; }
static void __set_PRIMASK(uint32_t m) {
  primask=m;
  if (!m && pending_sr) { uint32_t sr=pending_sr; pending_sr=0; interrupt_byte(pending_byte,sr); }
}
static void __DMB(void) {
  barriers++;
  if (edge_at && barriers==edge_at) {
    edge_at=0;
    if (primask) pending_sr=USART_SR_RXNE|USART_SR_TXE|USART_SR_TC;
    else interrupt_byte(pending_byte,USART_SR_RXNE|USART_SR_TXE|USART_SR_TC);
  }
}
static void GPIO_StructInit(GPIO_InitTypeDef *p) { memset(p,0,sizeof(*p)); }
static void GPIO_Init(uint32_t port,GPIO_InitTypeDef *p) { CHECK(port==GPIOB); if (p->GPIO_Mode==GPIO_Mode_IPU) input_pins|=p->GPIO_Pin; else { if (p->GPIO_Mode==GPIO_Mode_Out_PP) gpio_pins|=p->GPIO_Pin; else { CHECK(p->GPIO_Mode==GPIO_Mode_AF_PP); output_pins|=p->GPIO_Pin; } } }
static void USART_StructInit(USART_InitTypeDef *p) { memset(p,0,sizeof(*p)); }
static void USART_DeInit(USART_TypeDef *p) { memset(p,0,sizeof(*p)); }
static void USART_Init(USART_TypeDef *p,USART_InitTypeDef *i) { CHECK(i->USART_BaudRate==115200 && i->USART_Mode==(USART_Mode_Rx|USART_Mode_Tx)); flow=i->USART_HardwareFlowControl; p->CR3=flow; }
static void USART_ITConfig(USART_TypeDef *p,uint32_t bit,uint32_t enable) { CHECK(enable==ENABLE); if (bit==USART_IT_RXNE) p->CR1|=bit; else { CHECK(bit==USART_IT_ERR); p->CR3|=bit; } }
static void USART_Cmd(USART_TypeDef *p,uint32_t e) { (void)p; CHECK(e==ENABLE); }
static uint16_t USART_ReceiveData(USART_TypeDef *p) { reads++; uint16_t b=(uint16_t)p->DR; p->SR&=~(USART_SR_RXNE|USART_SR_ORE|USART_SR_FE|USART_SR_NE); return b; }
static void USART_SendData(USART_TypeDef *p,uint16_t b) { CHECK(writes<sizeof(wire)); wire[writes++]=(uint8_t)b; p->DR=b; p->SR&=~(USART_SR_TXE|USART_SR_TC); }
static void USART_ClearFlag(USART_TypeDef *p,uint16_t f) { p->SR&=~f; }
static void RCC_APB2PeriphClockCmd(uint32_t p,uint32_t e) { CHECK(e==ENABLE); apb2|=p; }
static void RCC_APB1PeriphClockCmd(uint32_t p,uint32_t e) { CHECK(e==ENABLE); apb1|=p; }
static void NVIC_Init(NVIC_InitTypeDef *p) { CHECK(p->NVIC_IRQChannel==USART3_IRQn && p->NVIC_IRQChannelCmd==ENABLE); }
/* PRODUCTION */
static void reset(void) {
  primask=0; pending_sr=0; edge_at=0; barriers=0; writes=reads=0;
  input_pins=output_pins=gpio_pins=apb1=apb2=0; rts_stop=true; rts_writes=0; comm_uart_init(115200); reads=0;
  CHECK(primask==0 && apb1==RCC_APB1Periph_USART3 && apb2==(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB));
  CHECK((regs.CR1&USART_CR1_RXNEIE) && (regs.CR3&USART_CR3_EIE));
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  CHECK(flow==USART_HardwareFlowControl_CTS && input_pins==(GPIO_Pin_11|GPIO_Pin_13));
  CHECK(output_pins==GPIO_Pin_10 && gpio_pins==GPIO_Pin_14 && !rts_stop);
#else
  CHECK(flow==USART_HardwareFlowControl_None && input_pins==GPIO_Pin_11 && output_pins==GPIO_Pin_10);
  CHECK(gpio_pins==0 && rts_writes==0);
#endif
}
static void rx_tests(void) {
  const uint8_t *p=NULL; comm_uart_stats_t s;
  reset(); CHECK(comm_uart_peek(&p)==0 && p==NULL); CHECK(comm_uart_peek(NULL)==0);
  CHECK(comm_uart_consume(0)); CHECK(!comm_uart_consume(1));
  for (unsigned i=0;i<511;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(comm_uart_peek(&p)==511); for (unsigned i=0;i<511;i++) CHECK(p[i]==(uint8_t)i);
  interrupt_byte(0xee,USART_SR_RXNE); comm_uart_stats(&s); CHECK(s.overflow==1 && (s.errors&COMM_UART_OVERFLOW));
  CHECK(!comm_uart_consume(512)); CHECK(comm_uart_consume(500));
  for (unsigned i=511;i<531;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(comm_uart_peek(&p)==12); for (unsigned i=0;i<12;i++) CHECK(p[i]==(uint8_t)(500+i));
  CHECK(comm_uart_consume(12)); CHECK(comm_uart_peek(&p)==19);
  for (unsigned i=0;i<19;i++) CHECK(p[i]==(uint8_t)(512+i));
  CHECK(comm_uart_consume(19)); CHECK(comm_uart_peek(&p)==0);
  for (unsigned i=0;i<70000;i++) { interrupt_byte((uint8_t)i,USART_SR_RXNE); CHECK(comm_uart_peek(&p)==1 && *p==(uint8_t)i); CHECK(comm_uart_consume(1)); }
  /* Прерывание после чтения границы consume: новый хвост не теряется. */
  interrupt_byte('A',USART_SR_RXNE); interrupt_byte('B',USART_SR_RXNE); CHECK(comm_uart_peek(&p)==2);
  pending_byte='C'; edge_at=barriers+1; CHECK(comm_uart_consume(2));
  CHECK(comm_uart_peek(&p)==1 && *p=='C'); CHECK(comm_uart_consume(1));
  /* Нельзя освободить байты, которых ещё не было в выданном участке. */
  interrupt_byte('D',USART_SR_RXNE); CHECK(comm_uart_peek(&p)==1);
  interrupt_byte('E',USART_SR_RXNE); CHECK(!comm_uart_consume(2)); CHECK(comm_uart_consume(1));
  CHECK(comm_uart_peek(&p)==1 && *p=='E');
  comm_uart_abort(); CHECK(comm_uart_peek(&p)==0); comm_uart_stats(&s); CHECK(s.overflow==1);
  reset(); interrupt_byte(0,USART_SR_ORE); interrupt_byte(0,USART_SR_FE); interrupt_byte(0,USART_SR_NE);
  interrupt_byte('X',USART_SR_RXNE|USART_SR_ORE|USART_SR_FE|USART_SR_NE);
  CHECK(reads==4 && comm_uart_peek(&p)==0); comm_uart_stats(&s);
  CHECK(s.ore==2 && s.fe==2 && s.ne==2 && s.errors==(COMM_UART_ORE|COMM_UART_FE|COMM_UART_NE));
  primask=1; CHECK(comm_uart_errors()==(COMM_UART_ORE|COMM_UART_FE|COMM_UART_NE) && primask==1); __set_PRIMASK(0);
  CHECK(comm_uart_errors()==0); comm_uart_stats(&s); CHECK(s.ore==2 && s.fe==2 && s.ne==2 && s.errors==0);
  for (unsigned i=0;i<70000;i++) interrupt_byte(0,USART_SR_ORE);
  comm_uart_stats(&s); CHECK(s.ore==UINT16_MAX); comm_uart_stats(NULL);
  interrupt_byte('Z',USART_SR_RXNE); CHECK(comm_uart_peek(&p)==1 && *p=='Z');
  primask=1; comm_uart_abort(); CHECK(primask==1); __set_PRIMASK(0);
}
/* В UART уходят только эти два статических буфера, без стека/heap. */
static bool owned_tx(const uint8_t *data, uint16_t count) {
  CHECK((data==at_command && count<=sizeof(at_command)) || (data==frame && count<=sizeof(frame)));
  return comm_uart_tx(data,count);
}
static void tx_finish(unsigned count) {
  for (unsigned i=0;i<count;i++) { regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(!comm_uart_tx_done()); }
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(comm_uart_tx_done());
}
static void tx_tests(void) {
  const uint8_t *rxp; reset(); CHECK(!comm_uart_tx_done());
  CHECK(!comm_uart_tx(NULL,1)); CHECK(!comm_uart_tx(NULL,0)); CHECK(!comm_uart_tx(frame,1041));
  at_command[0]='A'; at_command[1]='B'; at_command[2]='C';
  pending_byte='R'; edge_at=barriers+1; CHECK(owned_tx(at_command,3));
  CHECK(primask==0 && writes==1 && wire[0]=='A' && !comm_uart_tx_done());
  CHECK(tx_data==at_command); /* Публикуется указатель, копии команды нет. */
  CHECK(comm_uart_peek(&rxp)==1 && *rxp=='R'); CHECK(comm_uart_consume(1));
  CHECK(!owned_tx(frame,1)); CHECK(!owned_tx(at_command,0)); CHECK(tx_data==at_command);
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(writes==1 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(writes==2 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(writes==3 && !comm_uart_tx_done());
  CHECK(!(regs.CR1&USART_CR1_TXEIE) && (regs.CR1&USART_CR1_TCIE));
  CHECK(!owned_tx(frame,1)); /* Владелец занят и между последним TXE и TC. */
  regs.SR=USART_SR_TXE; USART3_IRQHandler(); CHECK(writes==3 && !comm_uart_tx_done());
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(comm_uart_tx_done() && tx_data==NULL);
  CHECK(!(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)) && !memcmp(wire,"ABC",3));
  /* Ноль завершён без DR и IRQ; отказ не снимает предыдущий успех. */
  CHECK(owned_tx(at_command,0)); CHECK(comm_uart_tx_done() && writes==3 && tx_data==NULL);
  CHECK(!comm_uart_tx(NULL,1)); CHECK(comm_uart_tx_done());
  /* Кадр уже построен: команда QISEND его не затирает, затем передаётся кадр. */
  reset(); for (unsigned i=0;i<sizeof(frame);i++) frame[i]=(uint8_t)i;
  static const char command[]="AT+QISEND=0,1040\r";
  memcpy(at_command,command,sizeof(command)-1);
  CHECK(owned_tx(at_command,sizeof(command)-1)); tx_finish(sizeof(command)-1);
  CHECK(!memcmp(wire,command,sizeof(command)-1));
  for (unsigned i=0;i<sizeof(frame);i++) CHECK(frame[i]==(uint8_t)i);
  primask=1; CHECK(owned_tx(frame,sizeof(frame))); CHECK(primask==1 && tx_data==frame); __set_PRIMASK(0);
  tx_finish(sizeof(frame)); CHECK(writes==sizeof(command)-1+sizeof(frame));
  CHECK(!memcmp(wire+sizeof(command)-1,frame,sizeof(frame)));
  /* Указатель жив до TC/abort; abort не разрешает устаревшему IRQ читать его. */
  for (unsigned stage=0;stage<3;stage++) {
    CHECK(owned_tx(frame,2)); CHECK(!comm_uart_tx_done());
    for (unsigned i=0;i<stage;i++) { regs.SR=USART_SR_TXE; USART3_IRQHandler(); }
    unsigned sent=writes; primask=1; comm_uart_abort(); CHECK(primask==1 && tx_data==NULL); __set_PRIMASK(0);
    regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler();
    CHECK(writes==sent && !comm_uart_tx_done() && !(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)));
  }
  memset(at_command,0xa5,sizeof(at_command)); CHECK(owned_tx(at_command,sizeof(at_command)));
  tx_finish(sizeof(at_command)); comm_uart_init(115200); CHECK(tx_data==NULL && !comm_uart_tx_done());
}
static void rts_tests(void) {
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  const uint8_t *p; comm_uart_stats_t stats; reset();
  /* Полезная ёмкость 511: при 383 свободно 128, при 384 свободно 127. */
  for (unsigned i=0;i<383;i++) { interrupt_byte((uint8_t)i,USART_SR_RXNE); CHECK(!rts_stop); }
  interrupt_byte(127,USART_SR_RXNE); CHECK(rts_stop);
  CHECK(comm_uart_peek(&p)==384); CHECK(!comm_uart_consume(385) && rts_stop);
  CHECK(comm_uart_consume(256) && rts_stop); CHECK(comm_uart_consume(0) && rts_stop);
  primask=1; CHECK(comm_uart_consume(1) && !rts_stop && primask==1); __set_PRIMASK(0);
  CHECK(comm_uart_peek(&p)==127); CHECK(comm_uart_consume(127));
  /* Модем учитывает RTS, но ещё 127 байт могут уже находиться в пути. */
  reset(); unsigned sent=0, received=0;
  for (unsigned cycle=0;cycle<10;cycle++) {
    unsigned in_flight=127;
    while (!rts_stop || in_flight) {
      if (rts_stop) in_flight--;
      interrupt_byte((uint8_t)sent++,USART_SR_RXNE);
    }
    CHECK(sent-received==511); comm_uart_stats(&stats); CHECK(stats.errors==0 && stats.overflow==0);
    while (received<sent) {
      uint16_t n=comm_uart_peek(&p); CHECK(n);
      for (unsigned i=0;i<n;i++) CHECK(p[i]==(uint8_t)(received+i));
      CHECK(comm_uart_consume(n)); received+=n;
    }
    CHECK(!rts_stop && received==sent);
  }
  /* IRQ при consume: старое заполнение не возвращает разрешение после стопа. */
  reset(); for (unsigned i=0;i<511;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(rts_stop && comm_uart_peek(&p)==511); pending_byte=0xee; edge_at=barriers+1;
  CHECK(comm_uart_consume(384)); CHECK(!rts_stop && comm_uart_peek(&p)==128);
  CHECK(p[127]==0xee); comm_uart_stats(&stats); CHECK(stats.errors==0);
  for (unsigned i=0;i<256;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(rts_stop); comm_uart_abort(); CHECK(!rts_stop && comm_uart_peek(&p)==0);
#endif
}
int main(void) { rx_tests(); tx_tests(); rts_tests(); puts("commuart: RX/errors/races, static owner TX/TXE-TC/abort, RTS hysteresis/stream, CTS profiles PASS"); return 0; }
