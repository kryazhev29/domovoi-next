/* Доработка: Кряжев А., 2026. Модель регистров USART3, без платы.
   Периферия и время моделируются; код проверяемого модуля берётся из проекта.
   Каждый сценарий проверяет наблюдаемое поведение без платы и сервера. */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
/* DMA продолжает работать при PRIMASK=1, как на STM32. */
typedef struct { volatile uint32_t ISR, IFCR; } DMA_TypeDef;
typedef struct { volatile uint32_t CCR, CNDTR, CPAR, CMAR; } DMA_Channel_TypeDef;
typedef struct { volatile uint32_t AHBENR, APB1ENR, APB2ENR, APB1RSTR, CFGR; } RCC_TypeDef;
typedef struct { volatile uint32_t CRH, BSRR; } GPIO_TypeDef;
static GPIO_TypeDef gpio;
static uint32_t SystemCoreClock = 72000000u;
static uint8_t irq_priorities[64];
static uint32_t irq_enabled[2];
static DMA_TypeDef dma;
static DMA_Channel_TypeDef dma_channel;
static RCC_TypeDef rcc;
static uint8_t rx[512];
#define DMA1 (&dma)
#define DMA1_Channel3 (&dma_channel)
#define RCC (&rcc)
#define RCC_AHBENR_DMA1EN 1u
#define DMA_ISR_TCIF3 (1u<<9)
#define DMA_ISR_HTIF3 (1u<<10)
#define DMA_ISR_TEIF3 (1u<<11)
#define DMA_IFCR_CGIF3 (1u<<8)
#define DMA_CCR3_EN 1u
#define DMA_CCR3_TCIE 2u
#define DMA_CCR3_HTIE 4u
#define DMA_CCR3_TEIE 8u
#define DMA_CCR3_CIRC 32u
#define DMA_CCR3_MINC 128u
#define DMA1_Channel3_IRQn 13u
#define USART_SR_IDLE (1u<<4)
#define USART_CR1_IDLEIE (1u<<4)
#define USART_CR3_DMAR (1u<<6)
/* Совместимые имена диагностики в моделях AT/модема/канала. */
#define tx_data uart_state.tx_ptr
#define tx_count uart_state.tx_length
#define tx_pos uart_state.tx_index
/* ЗАГОЛОВОК МОДУЛЯ */
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
#define USART_CR1_RE 4u
#define USART_CR1_TE 8u
#define USART_CR1_UE 0x2000u
#define USART_CR3_CTSE 0x200u
#define GPIO_Pin_11 (1u<<11)
#define GPIO_Pin_13 (1u<<13)
#define GPIOB (&gpio)
#define RCC_APB2ENR_AFIOEN 1u
#define RCC_APB2ENR_IOPBEN 8u
#define RCC_APB1ENR_USART3EN (1u<<18)
#define RCC_APB1RSTR_USART3RST (1u<<18)
#define RCC_CFGR_PPRE1 0x700u
#define USART3_IRQn 39u
#define ENABLE 1u
#define DISABLE 0u
#ifndef DOMOVOI_APP_HW
#define DOMOVOI_APP_HW 139
#endif
static uint32_t primask;
static bool rts_stop;
static unsigned rts_writes;
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
// Смоделировать линию RTS и записать её переход.
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
void DMA1_Channel3_IRQHandler(void);
static unsigned dma_irqs, uart_irqs;
// Очистить флаги DMA в модели регистров.
static void dma_clear_flags(void) {
  uint32_t flags=dma.IFCR;
  if (flags&DMA_IFCR_CGIF3) flags|=DMA_ISR_HTIF3|DMA_ISR_TCIF3|DMA_ISR_TEIF3;
  dma.ISR&=~flags; dma.IFCR=0;
}
// Передать один байт DMA в модель памяти.
static void dma_byte(uint8_t b, bool service) {
  dma_clear_flags(); CHECK(dma_channel.CCR&DMA_CCR3_EN);
  CHECK(dma_channel.CNDTR && dma_channel.CNDTR<=512);
  regs.DR=b; regs.SR&=~USART_SR_RXNE;
  rx[512-dma_channel.CNDTR]=b;
  if (--dma_channel.CNDTR==256) dma.ISR|=DMA_ISR_HTIF3;
  if (!dma_channel.CNDTR) { dma.ISR|=DMA_ISR_TCIF3; dma_channel.CNDTR=512; }
  if (service && !primask && (dma.ISR&(DMA_ISR_HTIF3|DMA_ISR_TCIF3|DMA_ISR_TEIF3))) { dma_irqs++; DMA1_Channel3_IRQHandler(); }
}
/* Старые границы проверяем при наблюдении каждого байта; новые потоки —
   с настоящими HT/TC и тиком раз в 12 байт, без RXNE IRQ. */
static void interrupt_byte(uint8_t b, uint32_t flags);
// Выдать маску прерываний модели процессора.
static uint32_t __get_PRIMASK(void) { dma_clear_flags(); return primask; }
// Запретить прерывания в модели процессора.
static void __disable_irq(void) { primask=1; }
// Восстановить маску прерываний модели процессора.
static void __set_PRIMASK(uint32_t m) {
  primask=m;
  if (!m && pending_sr) { uint32_t sr=pending_sr; pending_sr=0; interrupt_byte(pending_byte,sr); comm_uart_tick(); }
}
// Обозначить барьер памяти в однопоточной модели.
static void __DMB(void) {
  dma_clear_flags(); barriers++;
  if (edge_at && barriers==edge_at) {
    edge_at=0; dma_byte(pending_byte,false);
    if (primask) pending_sr=USART_SR_IDLE|USART_SR_TXE|USART_SR_TC;
    else { interrupt_byte(pending_byte,USART_SR_IDLE|USART_SR_TXE|USART_SR_TC); comm_uart_tick(); }
  }
}
/* Настройку NVIC сверяем по конкретным номерам и значениям регистров. */
static void board_irq_enable(uint8_t irq,uint8_t priority)
{
  CHECK(irq==USART3_IRQn || irq==DMA1_Channel3_IRQn);
  irq_priorities[irq]=priority; irq_enabled[irq/32] |= 1u<<(irq%32);
}
/* КОД МОДУЛЯ */
/* Учесть аппаратные эффекты записи DR и чтения SR/DR после IRQ. */
static void model_irq(uint32_t flags, uint8_t b)
{
  uint16_t before=uart_state.tx_index;
  regs.SR=flags; regs.DR=b; uart_irqs++; USART3_IRQHandler();
  if (uart_state.tx_index != before) {
    CHECK(writes<sizeof(wire)); wire[writes++]=(uint8_t)regs.DR;
    regs.SR&=~(USART_SR_TXE|USART_SR_TC);
  }
  if (flags&(USART_SR_ORE|USART_SR_FE|USART_SR_NE)) {
    ++reads; regs.SR&=~(USART_SR_IDLE|USART_SR_RXNE|USART_SR_ORE|USART_SR_FE|USART_SR_NE);
  }
}
/* DMA принимает RXNE; обработчик USART обслуживает только ошибки и передачу. */
static void interrupt_byte(uint8_t b, uint32_t flags)
{
  if (flags&USART_SR_RXNE) {
    dma_byte(b,false); comm_uart_tick(); flags&=~USART_SR_RXNE;
  }
  model_irq(flags,b);
}

// Сбросить регистры и наблюдения модели UART.
static void reset(void) {
  primask=0; pending_sr=0; edge_at=0; barriers=0; writes=reads=0;
  memset(&dma,0,sizeof(dma)); memset(&dma_channel,0,sizeof(dma_channel)); dma_irqs=uart_irqs=0;
  memset(&rcc,0,sizeof(rcc)); memset(&gpio,0,sizeof(gpio)); memset(&regs,0,sizeof(regs));
  memset(irq_enabled,0,sizeof(irq_enabled)); rcc.CFGR=0x400u;
  rts_stop=true; rts_writes=0; comm_uart_init(115200); reads=0;
  CHECK(primask==0 && rcc.APB1ENR==RCC_APB1ENR_USART3EN && rcc.APB2ENR==(RCC_APB2ENR_AFIOEN|RCC_APB2ENR_IOPBEN));
  CHECK(!(regs.CR1&(USART_CR1_RXNEIE|USART_CR1_IDLEIE)) && (regs.CR3&USART_CR3_EIE) && (regs.CR3&USART_CR3_DMAR));
  CHECK(dma_channel.CNDTR==512 && dma_channel.CCR==(DMA_CCR3_EN|DMA_CCR3_CIRC|DMA_CCR3_MINC|DMA_CCR3_HTIE|DMA_CCR3_TCIE|DMA_CCR3_TEIE));
  CHECK(dma_channel.CMAR==(uint32_t)(uintptr_t)rx && dma_channel.CPAR==(uint32_t)(uintptr_t)&regs.DR);
  CHECK(rcc.AHBENR&RCC_AHBENR_DMA1EN);
  CHECK(regs.CR3==(USART_CR3_CTSE|USART_CR3_DMAR|USART_CR3_EIE));
  CHECK(gpio.CRH==0x03808B00u && gpio.BSRR==(GPIO_Pin_11|GPIO_Pin_13) && !rts_stop);
  CHECK(regs.BRR==312u && regs.CR1==(USART_CR1_RE|USART_CR1_TE|USART_CR1_UE));
  CHECK(irq_priorities[USART3_IRQn]==0xF0 && irq_priorities[DMA1_Channel3_IRQn]==0xF0);
  CHECK(irq_enabled[USART3_IRQn/32]&(1u<<(USART3_IRQn%32)));
  CHECK(irq_enabled[DMA1_Channel3_IRQn/32]&(1u<<(DMA1_Channel3_IRQn%32)));
}
// Проверить приём UART и переполнение кольца.
static void rx_tests(void) {
  const uint8_t *p=NULL; comm_uart_stats_t s;
  reset(); CHECK(comm_uart_peek(&p)==0 && p==NULL);
  CHECK(comm_uart_consume(0)); CHECK(!comm_uart_consume(1));
  for (unsigned i=0;i<511;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(comm_uart_peek(&p)==511); for (unsigned i=0;i<511;i++) CHECK(p[i]==(uint8_t)i);
  interrupt_byte(0xee,USART_SR_RXNE); comm_uart_stats(&s); CHECK((s.errors&COMM_UART_OVERFLOW)); DIAG_CHECK(s.overflow==1);
  CHECK(!comm_uart_consume(512));
  /* DMA не умеет отбрасывать новый байт: после обгона только abort. */
  CHECK(comm_uart_peek(&p)==0); comm_uart_abort();
  for (unsigned i=0;i<500;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(comm_uart_peek(&p)==500 && comm_uart_consume(500));
  for (unsigned i=500;i<531;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
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
  comm_uart_abort(); CHECK(comm_uart_peek(&p)==0); comm_uart_stats(&s); DIAG_CHECK(s.overflow==1);
  reset(); interrupt_byte(0,USART_SR_ORE); interrupt_byte(0,USART_SR_FE); interrupt_byte(0,USART_SR_NE);
  interrupt_byte('X',USART_SR_RXNE|USART_SR_ORE|USART_SR_FE|USART_SR_NE);
  CHECK(reads==4 && dma_channel.CNDTR==511 && comm_uart_peek(&p)==0); comm_uart_stats(&s);
  CHECK(s.errors==(COMM_UART_ORE|COMM_UART_FE|COMM_UART_NE)); DIAG_CHECK(s.ore==2 && s.fe==2 && s.ne==2);
  primask=1; CHECK(comm_uart_errors()==(COMM_UART_ORE|COMM_UART_FE|COMM_UART_NE) && primask==1); __set_PRIMASK(0);
  CHECK(comm_uart_errors()==0); comm_uart_stats(&s); CHECK(s.errors==0); DIAG_CHECK(s.ore==2 && s.fe==2 && s.ne==2);
  for (unsigned i=0;i<70000;i++) interrupt_byte(0,USART_SR_ORE);
  comm_uart_stats(&s); DIAG_CHECK(s.ore==UINT16_MAX);
  comm_uart_abort(); /* После ошибки линии DMA-данные недостоверны. */
  interrupt_byte('Z',USART_SR_RXNE); CHECK(comm_uart_peek(&p)==1 && *p=='Z');
  primask=1; comm_uart_abort(); CHECK(primask==1); __set_PRIMASK(0);
}
/* В UART уходят только эти два статических буфера, без стека/heap. */
static bool owned_tx(const uint8_t *data, uint16_t count) {
  CHECK((data==at_command && count<=sizeof(at_command)) || (data==frame && count<=sizeof(frame)));
  return comm_uart_tx(data,count);
}
// Завершить DMA и физическую передачу UART.
static void tx_finish(unsigned count) {
  for (unsigned i=0;i<count;i++) { regs.SR=USART_SR_TXE|USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(!comm_uart_tx_done()); }
  regs.SR=USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(comm_uart_tx_done());
}
// Проверить очередь и завершение передачи UART.
static void tx_tests(void) {
  const uint8_t *rxp; reset(); CHECK(!comm_uart_tx_done());
  at_command[0]='A'; at_command[1]='B'; at_command[2]='C';
  pending_byte='R'; edge_at=barriers+1; CHECK(owned_tx(at_command,3));
  CHECK(primask==0 && writes==1 && wire[0]=='A' && !comm_uart_tx_done());
  CHECK(uart_state.tx_ptr==at_command); /* Публикуется указатель, копии команды нет. */
  CHECK(comm_uart_peek(&rxp)==1 && *rxp=='R'); CHECK(comm_uart_consume(1));
  CHECK(!owned_tx(frame,1)); CHECK(!owned_tx(at_command,0)); CHECK(uart_state.tx_ptr==at_command);
  regs.SR=USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(writes==1 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(writes==2 && !comm_uart_tx_done());
  regs.SR=USART_SR_TXE|USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(writes==3 && !comm_uart_tx_done());
  CHECK(!(regs.CR1&USART_CR1_TXEIE) && (regs.CR1&USART_CR1_TCIE));
  CHECK(!owned_tx(frame,1)); /* Владелец занят и между последним TXE и TC. */
  regs.SR=USART_SR_TXE; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(writes==3 && !comm_uart_tx_done());
  regs.SR=USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR); CHECK(comm_uart_tx_done() && uart_state.tx_ptr==NULL);
  CHECK(!(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)) && !memcmp(wire,"ABC",3));
  /* Кадр уже построен: команда QISEND его не затирает, затем передаётся кадр. */
  reset(); for (unsigned i=0;i<sizeof(frame);i++) frame[i]=(uint8_t)i;
  static const char command[]="AT+QISEND=0,1040\r";
  memcpy(at_command,command,sizeof(command)-1);
  CHECK(owned_tx(at_command,sizeof(command)-1)); tx_finish(sizeof(command)-1);
  CHECK(!memcmp(wire,command,sizeof(command)-1));
  for (unsigned i=0;i<sizeof(frame);i++) CHECK(frame[i]==(uint8_t)i);
  primask=1; CHECK(owned_tx(frame,sizeof(frame))); CHECK(primask==1 && uart_state.tx_ptr==frame); __set_PRIMASK(0);
  tx_finish(sizeof(frame)); CHECK(writes==sizeof(command)-1+sizeof(frame));
  CHECK(!memcmp(wire+sizeof(command)-1,frame,sizeof(frame)));
  /* Указатель жив до TC/abort; abort не разрешает устаревшему IRQ читать его. */
  for (unsigned stage=0;stage<3;stage++) {
    CHECK(owned_tx(frame,2)); CHECK(!comm_uart_tx_done());
    for (unsigned i=0;i<stage;i++) { regs.SR=USART_SR_TXE; model_irq(regs.SR,(uint8_t)regs.DR); }
    unsigned sent=writes; primask=1; comm_uart_abort(); CHECK(primask==1 && uart_state.tx_ptr==NULL); __set_PRIMASK(0);
    regs.SR=USART_SR_TXE|USART_SR_TC; model_irq(regs.SR,(uint8_t)regs.DR);
    CHECK(writes==sent && !comm_uart_tx_done() && !(regs.CR1&(USART_CR1_TXEIE|USART_CR1_TCIE)));
  }
  memset(at_command,0xa5,sizeof(at_command)); CHECK(owned_tx(at_command,sizeof(at_command)));
  tx_finish(sizeof(at_command)); comm_uart_init(115200); CHECK(uart_state.tx_ptr==NULL && !comm_uart_tx_done());
}
// Проверить управление потоком RTS/CTS.
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
    CHECK(sent-received==511); comm_uart_stats(&stats); CHECK(stats.errors==0); DIAG_CHECK(stats.overflow==0);
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
  CHECK(comm_uart_consume(384)); CHECK(rts_stop && comm_uart_peek(&p)==128);
  CHECK(p[127]==0xee); comm_uart_stats(&stats); CHECK(stats.errors==0);
  CHECK(comm_uart_consume(1) && !rts_stop);
  for (unsigned i=0;i<257;i++) interrupt_byte((uint8_t)i,USART_SR_RXNE);
  CHECK(rts_stop); comm_uart_abort(); CHECK(!rts_stop && comm_uart_peek(&p)==0);
#endif
}
/* Берём настоящий обработчик тика из stm32_it.c при сборке модели. */
static void timers_handle(void) {}
/* ТИК */
static unsigned drain(unsigned expected) {
  const uint8_t *p; uint16_t n;
  while ((n=comm_uart_peek(&p))!=0) {
    for (unsigned j=0;j<n;j++) CHECK(p[j]==(uint8_t)(expected+j));
    CHECK(comm_uart_consume(n)); expected+=n;
  }
  return expected;
}
// Проверить DMA, ошибки UART и сброс передачи.
static void dma_tests(void) {
  const uint8_t *p; comm_uart_stats_t stats;
  /* Длинный непрерывный поток: два оборота счётчиков 16 бит,
     аппаратные HT/TC, тик 1 мс (12 байт), чтение порциями. */
  reset(); unsigned received=0;
  for (unsigned i=0;i<131072;i++) {
    dma_byte((uint8_t)i,true);
    CHECK(dma_channel.CNDTR==512-((i+1)&511));
    if ((i+1)%12==0) SysTick_Handler();
    if ((i+1)%96==0) received=drain(received);
  }
  received=drain(received); CHECK(received==131072 && !uart_irqs && dma_irqs==512);
  comm_uart_stats(&stats); CHECK(!stats.errors); DIAG_CHECK(!stats.overflow);
  /* IDLE и новый RXNE при другом IRQ не должны читать DR вместо DMA. */
  reset(); unsigned sent=0; received=0;
  for (unsigned block=1;block<=73;block++) {
    for (unsigned j=0;j<block;j++) dma_byte((uint8_t)sent++,true);
    regs.SR=USART_SR_IDLE|USART_SR_RXNE; regs.DR=0xa5; model_irq(regs.SR,(uint8_t)regs.DR);
    CHECK((regs.SR&USART_SR_RXNE) && regs.DR==0xa5); received=drain(received);
  }
  CHECK(received==sent && reads==0); comm_uart_stats(&stats); CHECK(!stats.errors);
  /* DMA пишет ещё один байт посреди публикации участка по тику. */
  reset(); dma_byte('A',true); pending_byte='B'; edge_at=barriers+1;
  regs.SR=USART_SR_IDLE; SysTick_Handler();
  CHECK(comm_uart_peek(&p)==2 && p[0]=='A' && p[1]=='B');
  CHECK(comm_uart_consume(2) && comm_uart_peek(&p)==0);
  /* HT/TC публикуют RX без помощи peek и тика. */
  reset(); for (unsigned i=0;i<256;i++) dma_byte((uint8_t)i,true);
  CHECK(uart_state.rx_write==256 && dma_irqs==1 && !(dma.ISR&DMA_ISR_HTIF3));
  CHECK(drain(0)==256);
  for (unsigned i=256;i<512;i++) dma_byte((uint8_t)i,true);
  CHECK(uart_state.rx_write==512 && dma_irqs==2 && !(dma.ISR&DMA_ISR_TCIF3));
  CHECK(drain(256)==512);
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  /* Тик нужен между HT=256 и TC=512, когда нет IDLE и consume. */
  reset(); for (unsigned i=0;i<383;i++) dma_byte((uint8_t)i,true);
  SysTick_Handler(); CHECK(!rts_stop && uart_state.rx_write==383);
  dma_byte(127,true); SysTick_Handler(); CHECK(rts_stop && uart_state.rx_write==384);
  /* Медленный читатель: модем останавливается после ещё 32 байт.
     Каждая задержка тика <=12 байт, побайтовых IRQ нет. */
  reset(); sent=received=0;
  for (unsigned cycle=0;cycle<80;cycle++) {
    unsigned tail=32;
    while (!rts_stop || tail) {
      if (rts_stop) tail--;
      dma_byte((uint8_t)sent++,true);
      if (sent%12==0) SysTick_Handler();
    }
    comm_uart_stats(&stats); CHECK(!stats.errors && sent-received<=428);
    received=drain(received); CHECK(received==sent && !rts_stop);
  }
#endif
  /* Полный круг с той же позицией CNDTR нельзя принять за пустоту. */
  reset(); for (unsigned i=0;i<100;i++) dma_byte((uint8_t)i,true);
  CHECK(drain(0)==100);
  for (unsigned i=0;i<512;i++) dma_byte((uint8_t)i,false);
  SysTick_Handler(); comm_uart_stats(&stats);
  CHECK((comm_uart_errors()&COMM_UART_OVERFLOW)); DIAG_CHECK(stats.overflow==1);
  CHECK(comm_uart_peek(&p)==0 && p==NULL && !comm_uart_consume(1));
  /* После снятия ошибки и даже оборота счётчика старый поток не выдаём. */
  for (unsigned i=0;i<65536;i++) { dma_byte((uint8_t)i,true); if (i%12==0) SysTick_Handler(); }
  CHECK(comm_uart_peek(&p)==0 && !comm_uart_consume(0));
  comm_uart_abort(); CHECK(comm_uart_peek(&p)==0);
  dma_byte('Z',true); CHECK(comm_uart_peek(&p)==1 && *p=='Z'); CHECK(comm_uart_consume(1));
  reset(); for (unsigned i=0;i<600;i++) dma_byte((uint8_t)i,false);
  SysTick_Handler(); CHECK(comm_uart_errors()&COMM_UART_OVERFLOW);
  CHECK(comm_uart_peek(&p)==0);
  /* Abort на неполном DMA; чужой флаг канала 1 не сбрасывается. */
  reset(); for (unsigned i=0;i<89;i++) dma_byte((uint8_t)i,false);
  dma.ISR|=DMA_ISR_HTIF3|DMA_ISR_TCIF3|DMA_ISR_TEIF3|2u;
  primask=1; comm_uart_abort(); CHECK(primask==1); __set_PRIMASK(0);
  CHECK(dma_channel.CNDTR==512 && dma.ISR==2u && !uart_state.rx_write && !uart_state.rx_read);
  DMA1_Channel3_IRQHandler(); CHECK(comm_uart_peek(&p)==0 && !comm_uart_errors());
  unsigned old_irqs=dma_irqs; dma_byte('A',true); CHECK(dma_irqs==old_irqs && dma.ISR==2u);
  CHECK(comm_uart_peek(&p)==1 && *p=='A');
  CHECK(comm_uart_consume(1)); comm_uart_init(115200);
  CHECK(comm_uart_peek(&p)==0 && !comm_uart_errors() && dma_channel.CNDTR==512);
  /* Ошибка DMA — потеря потока; ошибки линии сохраняют счётчики. */
  dma.ISR=DMA_ISR_TEIF3; DMA1_Channel3_IRQHandler();
  CHECK(comm_uart_errors()&COMM_UART_OVERFLOW); CHECK(comm_uart_peek(&p)==0);
}
// Запустить все сценарии этого стенда.
int main(void) { rcc.AHBENR=0; dma_channel.CNDTR=0; SysTick_Handler(); CHECK(!uart_state.rx_write && !dma.IFCR && !rts_writes); rx_tests(); tx_tests(); rts_tests(); dma_tests(); puts("commuart: RX DMA/CNDTR/HT/TC/IDLE/tick, wrap/overrun/abort, TX/RTS/CTS PASS"); return 0; }
