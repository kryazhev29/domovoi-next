/* STM32F103CB / Domovoi v5, based on the supplied v5 board wiring. */
#include "includes.h"
#include "boot_platform.h"
#include "usb_istr.h"
#ifdef __ICCARM__
#include <intrinsics.h>
#else
#define __disable_interrupt __disable_irq
#define __enable_interrupt __enable_irq
#define __no_operation __NOP
#endif
static volatile uint32_t ticks;
static int spi_fault;
static uint16_t flash_kib;
int boot_platform_supported(void) { return flash_kib==BOOT_REQUIRED_FLASH_KIB; }
uint32_t boot_millis(void) { return ticks; }
void SysTick_Handler(void) { ++ticks; }
void USB_LP_CAN1_RX0_IRQHandler(void) { USB_Istr(); }
void USBWakeUp_IRQHandler(void) { EXTI_ClearITPendingBit(EXTI_Line18); }
void boot_service(void) {
 IWDG->KR=0xAAAA;
#ifndef BOOT_BOARD_MINI
 /* Dedicated PWR/DATA LEDs only. MINI LEDs are electrically shared with CAN. */
 if((ticks/250u)&1u) { GPIOB->BRR=BOOT_LED_PRIMARY_PIN;GPIOB->BSRR=BOOT_LED_SECONDARY_PIN; }
 else { GPIOB->BRR=BOOT_LED_SECONDARY_PIN;GPIOB->BSRR=BOOT_LED_PRIMARY_PIN; }
#endif
}
static uint8_t spi_byte(uint8_t data) {
 uint32_t timeout=100000;
 while(!(SPI1->SR&2u)) if(!--timeout) {spi_fault=1;return 0xff;}
 SPI1->DR=data;timeout=100000;
 while(!(SPI1->SR&1u)) if(!--timeout) {spi_fault=1;return 0xff;}
 return (uint8_t)SPI1->DR;
}
static void select_chip(int active) {
 if(active) BOOT_CS_PORT->BRR=BOOT_CS_PIN;else BOOT_CS_PORT->BSRR=BOOT_CS_PIN;
 __no_operation();__no_operation();__no_operation();__no_operation();
}
static void command(uint8_t cmd,uint32_t address) {
 select_chip(1);spi_byte(cmd);spi_byte((uint8_t)(address>>16));
 spi_byte((uint8_t)(address>>8));spi_byte((uint8_t)address);
}
static int wait_flash(void) {
 uint32_t timeout=2000000;uint8_t status;
 do {
  select_chip(1);spi_byte(5);status=spi_byte(0xff);select_chip(0);
  if(spi_fault||!--timeout) return 0;
  boot_service();
 } while(status&1u);
 return status!=0xff;
}
static int write_enable(void) {
 uint8_t status;
 if(!wait_flash()) return 0;
 select_chip(1);spi_byte(6);select_chip(0);
 select_chip(1);spi_byte(5);status=spi_byte(0xff);select_chip(0);
 return !spi_fault&&(status&2u);
}
int boot_read(int external,uint32_t address,void *data,uint32_t size) {
 uint8_t *p=data;
 if(!external) {
  if(!boot_platform_supported())return 0;
  if(address<BOOT_APP_BASE||address>BOOT_FLASH_END||size>BOOT_FLASH_END-address) return 0;
  memcpy(data,(const void*)address,size);return 1;
 }
 if(address>BOOT_BACKUP+BOOT_SLOT_SIZE||size>BOOT_BACKUP+BOOT_SLOT_SIZE-address) return 0;
 spi_fault=0;if(!wait_flash()) return 0;
 command(3,address);while(size--) *p++=spi_byte(0xff);select_chip(0);
 return !spi_fault;
}
int boot_ext_erase(uint32_t address) {
 if((address&4095u)||address>=BOOT_BACKUP+BOOT_SLOT_SIZE) return 0;
 spi_fault=0;if(!write_enable()) return 0;
 command(0x20,address);select_chip(0);
 if(!wait_flash()) return 0;
 /* Reject a protected/failed erase before attempting any journal transition. */
 command(3,address);
 {unsigned i;int ok=1;for(i=0;i<4096;i++) if(spi_byte(0xff)!=0xff) ok=0;
 select_chip(0);return ok&&!spi_fault;}
}
int boot_ext_write(uint32_t address,const void *data,uint32_t size) {
 const uint8_t *p=data;uint32_t n,i;
 if(address>BOOT_BACKUP+BOOT_SLOT_SIZE||size>BOOT_BACKUP+BOOT_SLOT_SIZE-address) return 0;
 spi_fault=0;
 while(size) {
  n=256-(address&255u);if(n>size)n=size;
  if(!write_enable())return 0;
  command(2,address);for(i=0;i<n;i++)spi_byte(p[i]);select_chip(0);
  if(!wait_flash())return 0;
  command(3,address);
  for(i=0;i<n;i++)if(spi_byte(0xff)!=p[i])spi_fault=1;
  select_chip(0);if(spi_fault)return 0;
  address+=n;p+=n;size-=n;
 }
 return 1;
}
int boot_int_erase(uint32_t address) {
 FLASH_Status status;uint32_t mask=__get_PRIMASK();
 if(!boot_platform_supported())return 0;
 if(address<BOOT_APP_BASE||address>=BOOT_FLASH_END||(address&1023u))return 0;
 boot_service();__disable_interrupt();FLASH_Unlock();
 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
 status=FLASH_ErasePage(address);FLASH_Lock();__set_PRIMASK(mask);
 return status==FLASH_COMPLETE;
}
int boot_int_write(uint32_t address,const void *data,uint32_t size) {
 uint32_t i,word,mask;FLASH_Status status=FLASH_COMPLETE;
 if(!boot_platform_supported())return 0;
 if(address<BOOT_APP_BASE||address>BOOT_FLASH_END||size>BOOT_FLASH_END-address||((address|size)&3u))return 0;
 mask=__get_PRIMASK();__disable_interrupt();FLASH_Unlock();
 FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
 for(i=0;i<size;i+=4) {
  memcpy(&word,(const uint8_t*)data+i,4);
  status=FLASH_ProgramWord(address+i,word);
  if(status!=FLASH_COMPLETE||*(volatile uint32_t*)(address+i)!=word)break;
  IWDG->KR=0xAAAA;
 }
 FLASH_Lock();__set_PRIMASK(mask);return i==size;
}
void boot_platform_init(void) {
 GPIO_InitTypeDef g;
 /* Cache the factory signature before enabling USB/peripherals. */
 flash_kib=*(const volatile uint16_t*)BOOT_FLASH_SIZE_ADDRESS;
 SCB->VTOR=0x08000000u;
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO|RCC_APB2Periph_SPI1,ENABLE);
 GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable,ENABLE);
 GPIO_StructInit(&g);g.GPIO_Speed=GPIO_Speed_50MHz;g.GPIO_Mode=GPIO_Mode_Out_PP;
 /* Set inactive levels before enabling output drivers. */
 BOOT_USB_PORT->BRR=BOOT_USB_PIN;BOOT_CS_PORT->BSRR=BOOT_CS_PIN;
#ifndef BOOT_BOARD_MINI
 GPIOB->BRR=BOOT_LED_PINS;
#endif
 g.GPIO_Pin=BOOT_USB_PIN;GPIO_Init(BOOT_USB_PORT,&g);
 g.GPIO_Pin=BOOT_CS_PIN;GPIO_Init(BOOT_CS_PORT,&g);
#ifndef BOOT_BOARD_MINI
 g.GPIO_Pin=BOOT_LED_PINS;GPIO_Init(GPIOB,&g);
#endif
#ifdef BOOT_BOARD_MINI
 /* RXD is an output of the external CAN chip; never drive it from the MCU.
    TXD high prevents a dominant bus level throughout bootloader operation. */
 g.GPIO_Pin=BOOT_CAN_RX_PIN;g.GPIO_Mode=GPIO_Mode_IN_FLOATING;GPIO_Init(GPIOB,&g);
 GPIOB->BSRR=BOOT_CAN_TX_PIN;
 g.GPIO_Pin=BOOT_CAN_TX_PIN;g.GPIO_Mode=GPIO_Mode_Out_PP;GPIO_Init(GPIOB,&g);
 GPIOB->BRR=GPIO_Pin_0;g.GPIO_Pin=GPIO_Pin_0;GPIO_Init(GPIOB,&g);
 GPIOC->BRR=GPIO_Pin_14|GPIO_Pin_15;g.GPIO_Pin=GPIO_Pin_14|GPIO_Pin_15;GPIO_Init(GPIOC,&g);
#else
 GPIOB->BRR=GPIO_Pin_3|GPIO_Pin_7;g.GPIO_Pin=GPIO_Pin_3|GPIO_Pin_7;GPIO_Init(GPIOB,&g);
 GPIOC->BRR=GPIO_Pin_13;g.GPIO_Pin=GPIO_Pin_13;GPIO_Init(GPIOC,&g);
#ifndef BOOT_BOARD_MODEM
 GPIOA->BRR=GPIO_Pin_0;g.GPIO_Pin=GPIO_Pin_0;GPIO_Init(GPIOA,&g);
#endif
#endif
 g.GPIO_Mode=GPIO_Mode_IPU;g.GPIO_Pin=BOOT_BUTTON_PIN;GPIO_Init(BOOT_BUTTON_PORT,&g);
 g.GPIO_Pin=GPIO_Pin_15;g.GPIO_Mode=GPIO_Mode_IN_FLOATING;GPIO_Init(GPIOB,&g);
 g.GPIO_Pin=GPIO_Pin_5|GPIO_Pin_7;g.GPIO_Mode=GPIO_Mode_AF_PP;GPIO_Init(GPIOA,&g);
 g.GPIO_Pin=GPIO_Pin_6;g.GPIO_Mode=GPIO_Mode_IPU;GPIO_Init(GPIOA,&g);
 SPI1->CR1=(1u<<2)|(2u<<3)|(1u<<8)|(1u<<9);SPI1->CR1|=1u<<6;
 SysTick_Config(SystemCoreClock/1000u);
 IWDG->KR=0x5555;IWDG->PR=6;IWDG->RLR=4095;IWDG->KR=0xAAAA;IWDG->KR=0xCCCC;
 __enable_interrupt();
}
int boot_button(void) {
 uint32_t start=ticks;
 while(ticks-start<50u) { if(BOOT_BUTTON_PORT->IDR&BOOT_BUTTON_PIN)return 0;boot_service(); }
 return 1;
}
void boot_usb_init(void) { Set_System();Set_USBClock();USB_Interrupts_Config();USB_Init();USB_PULLUP_ON; }
#ifdef __ICCARM__
__stackless static void jump(uint32_t sp,uint32_t pc) {
 __asm("MSR MSP, R0");__asm("CPSIE i");__asm("BX R1");
}
#else
__attribute__((naked)) static void jump(uint32_t sp,uint32_t pc) {
 __asm volatile("msr msp, r0\n cpsie i\n bx r1");
}
#endif

void boot_start_app(void) {
 BootHeader h;unsigned i;
 if(!boot_validate(0,BOOT_APP_BASE,&h))return;
 __disable_interrupt();USB_PULLUP_OFF;
 SysTick->CTRL=0;SysTick->LOAD=0;SysTick->VAL=0;
 for(i=0;i<8;i++){NVIC->ICER[i]=0xffffffffu;NVIC->ICPR[i]=0xffffffffu;}
 SCB->ICSR=(1u<<25)|(1u<<27);
 RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB,ENABLE);
 RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB,DISABLE);
 SPI1->CR1=0;SCB->VTOR=BOOT_APP_VECTOR;__DSB();__ISB();
 jump(*(uint32_t*)BOOT_APP_VECTOR,*(uint32_t*)(BOOT_APP_VECTOR+4));
}
