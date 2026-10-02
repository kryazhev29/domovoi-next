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
#define USART_HardwareFlowControl_RTS_CTS 0x300u
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
static uint32_t primask, input_pins, output_pins, apb1, apb2, flow;
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
static void GPIO_Init(uint32_t port,GPIO_InitTypeDef *p) { CHECK(port==GPIOB); if (p->GPIO_Mode==GPIO_Mode_IPU) input_pins|=p->GPIO_Pin; else { CHECK(p->GPIO_Mode==GPIO_Mode_AF_PP); output_pins|=p->GPIO_Pin; } }
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
  input_pins=output_pins=apb1=apb2=0; comm_uart_init(115200); reads=0;
  CHECK(primask==0 && apb1==RCC_APB1Periph_USART3 && apb2==(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB));
  CHECK((regs.CR1&USART_CR1_RXNEIE) && (regs.CR3&USART_CR3_EIE));
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  CHECK(flow==USART_HardwareFlowControl_RTS_CTS && input_pins==(GPIO_Pin_11|GPIO_Pin_13) && output_pins==(GPIO_Pin_10|GPIO_Pin_14));
#else
  CHECK(flow==USART_HardwareFlowControl_None && input_pins==GPIO_Pin_11 && output_pins==GPIO_Pin_10);
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
static void tx_tests(void) {
  uint16_t capacity=0; const uint8_t *rxp; reset(); CHECK(!comm_uart_tx_done());
  CHECK(comm_uart_tx_acquire(NULL)==NULL); CHECK(!comm_uart_tx_commit(1));
  uint8_t *p=comm_uart_tx_acquire(&capacity); CHECK(p && capacity==1040);
  CHECK(comm_uart_tx_acquire(&capacity)==NULL && capacity==0);
  CHECK(!comm_uart_tx_commit(1041)); p[0]='A'; p[1]='B'; p[2]='C';
  pending_byte='R'; edge_at=barriers+1; CHECK(comm_uart_tx_commit(3));
  CHECK(primask==0 && writes==1 && wire[0]=='A' && !comm_uart_tx_done());
  CHECK(comm_uart_peek(&rxp)==1 && *rxp=='R'); CHECK(comm_uart_consume(1));
  CHECK(comm_uart_tx_acquire(&capacity)==NULL); CHECK(!comm_uart_tx_commit(1));
  comm_uart_tx_release(); CHECK(!comm_uart_tx_done());
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(writes==1 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(writes==2 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(writes==3 && !comm_uart_tx_done());
  CHECK(!(regs.CR1&USART_CR1_TXEIE) && (regs.CR1&USART_CR1_TCIE));
  regs.SR=USART_SR_TXE; USART3_IRQHandler(); CHECK(writes==3 && !comm_uart_tx_done());
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(comm_uart_tx_done() && !(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)));
  CHECK(!memcmp(wire,"ABC",3));
  /* Ноль завершён без DR и IRQ; release не отменяет принятую передачу. */
  p=comm_uart_tx_acquire(&capacity); CHECK(p && capacity==1040); CHECK(comm_uart_tx_commit(0)); CHECK(comm_uart_tx_done() && writes==3);
  p=comm_uart_tx_acquire(&capacity); CHECK(p); comm_uart_tx_release(); CHECK(!comm_uart_tx_done()); CHECK(!comm_uart_tx_commit(1));
  p=comm_uart_tx_acquire(&capacity); CHECK(p); for (unsigned i=0;i<capacity;i++) p[i]=(uint8_t)i;
  primask=1; CHECK(comm_uart_tx_commit(capacity)); CHECK(primask==1); __set_PRIMASK(0);
  for (unsigned i=0;i<1040;i++) { regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler(); CHECK(!comm_uart_tx_done()); }
  CHECK(writes==1043); for (unsigned i=0;i<1040;i++) CHECK(wire[3+i]==(uint8_t)i);
  regs.SR=USART_SR_TC; USART3_IRQHandler(); CHECK(comm_uart_tx_done());
  for (unsigned stage=0;stage<3;stage++) {
    p=comm_uart_tx_acquire(&capacity); CHECK(p); p[0]=1; CHECK(comm_uart_tx_commit(1));
    if (stage) { regs.SR=USART_SR_TXE; USART3_IRQHandler(); }
    unsigned sent=writes; comm_uart_abort(); regs.SR=USART_SR_TXE|USART_SR_TC; USART3_IRQHandler();
    CHECK(writes==sent && !comm_uart_tx_done() && !(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)));
    p=comm_uart_tx_acquire(&capacity); CHECK(p); comm_uart_tx_release();
  }
  p=comm_uart_tx_acquire(&capacity); CHECK(p); comm_uart_abort(); CHECK(!comm_uart_tx_commit(1));
}
int main(void) { rx_tests(); tx_tests(); puts("commuart: RX wrap/full/errors/races, owned TX/zero/abort/TXE-TC, GPIO profiles PASS"); return 0; }
