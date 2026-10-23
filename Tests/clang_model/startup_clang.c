/* Старт модели clang: таблица векторов по образцу startup_stm32f10x_md.s и инициализация данных. */
#include <stdint.h>
extern uint32_t __data_load[], __data_start[], __data_end[], __bss_start[], __bss_end[];
extern uint32_t __ramfunc_load[], __ramfunc_start[], __ramfunc_end[];
extern char domovoi_cstack_limit[] __asm__("CSTACK$$Limit");
void SystemInit(void); int main(void);
void __iar_data_init3(void)
{
  uint32_t *s = __data_load, *d = __data_start;
  while (d < __data_end) *d++ = *s++;
  s = __ramfunc_load; d = __ramfunc_start;
  while (d < __ramfunc_end) *d++ = *s++;
  for (d = __bss_start; d < __bss_end; ) *d++ = 0;
}
void Reset_Handler(void) { SystemInit(); __iar_data_init3(); main(); for (;;) ; }
void Default_Handler(void) { for (;;) ; }
#define H(name) void name(void) __attribute__((weak, alias("Default_Handler")))
H(NMI_Handler); H(HardFault_Handler); H(MemManage_Handler); H(BusFault_Handler); H(UsageFault_Handler);
H(SVC_Handler); H(DebugMon_Handler); H(PendSV_Handler); H(SysTick_Handler);
H(WWDG_IRQHandler); H(PVD_IRQHandler); H(TAMPER_IRQHandler); H(RTC_IRQHandler); H(FLASH_IRQHandler); H(RCC_IRQHandler);
H(EXTI0_IRQHandler); H(EXTI1_IRQHandler); H(EXTI2_IRQHandler); H(EXTI3_IRQHandler); H(EXTI4_IRQHandler);
H(DMA1_Channel1_IRQHandler); H(DMA1_Channel2_IRQHandler); H(DMA1_Channel3_IRQHandler); H(DMA1_Channel4_IRQHandler);
H(DMA1_Channel5_IRQHandler); H(DMA1_Channel6_IRQHandler); H(DMA1_Channel7_IRQHandler); H(ADC1_2_IRQHandler);
H(USB_HP_CAN1_TX_IRQHandler); H(USB_LP_CAN1_RX0_IRQHandler); H(CAN1_RX1_IRQHandler); H(CAN1_SCE_IRQHandler);
H(EXTI9_5_IRQHandler); H(TIM1_BRK_IRQHandler); H(TIM1_UP_IRQHandler); H(TIM1_TRG_COM_IRQHandler); H(TIM1_CC_IRQHandler);
H(TIM2_IRQHandler); H(TIM3_IRQHandler); H(TIM4_IRQHandler); H(I2C1_EV_IRQHandler); H(I2C1_ER_IRQHandler);
H(I2C2_EV_IRQHandler); H(I2C2_ER_IRQHandler); H(SPI1_IRQHandler); H(SPI2_IRQHandler); H(USART1_IRQHandler);
H(USART2_IRQHandler); H(USART3_IRQHandler); H(EXTI15_10_IRQHandler); H(RTCAlarm_IRQHandler); H(USBWakeUp_IRQHandler);
typedef void (*vector_t)(void);
__attribute__((section(".intvec"), used)) const vector_t __vector_table[] = {
  (vector_t)domovoi_cstack_limit, Reset_Handler, NMI_Handler, HardFault_Handler, MemManage_Handler, BusFault_Handler, UsageFault_Handler,
  (vector_t)0x4C495254u, (vector_t)(0x48570000u + DOMOVOI_APP_HW), 0, 0, SVC_Handler, DebugMon_Handler, 0, PendSV_Handler, SysTick_Handler,
  WWDG_IRQHandler, PVD_IRQHandler, TAMPER_IRQHandler, RTC_IRQHandler, FLASH_IRQHandler, RCC_IRQHandler,
  EXTI0_IRQHandler, EXTI1_IRQHandler, EXTI2_IRQHandler, EXTI3_IRQHandler, EXTI4_IRQHandler,
  DMA1_Channel1_IRQHandler, DMA1_Channel2_IRQHandler, DMA1_Channel3_IRQHandler, DMA1_Channel4_IRQHandler,
  DMA1_Channel5_IRQHandler, DMA1_Channel6_IRQHandler, DMA1_Channel7_IRQHandler, ADC1_2_IRQHandler,
  USB_HP_CAN1_TX_IRQHandler, USB_LP_CAN1_RX0_IRQHandler, CAN1_RX1_IRQHandler, CAN1_SCE_IRQHandler,
  EXTI9_5_IRQHandler, TIM1_BRK_IRQHandler, TIM1_UP_IRQHandler, TIM1_TRG_COM_IRQHandler, TIM1_CC_IRQHandler,
  TIM2_IRQHandler, TIM3_IRQHandler, TIM4_IRQHandler, I2C1_EV_IRQHandler, I2C1_ER_IRQHandler,
  I2C2_EV_IRQHandler, I2C2_ER_IRQHandler, SPI1_IRQHandler, SPI2_IRQHandler, USART1_IRQHandler,
  USART2_IRQHandler, USART3_IRQHandler, EXTI15_10_IRQHandler, RTCAlarm_IRQHandler, USBWakeUp_IRQHandler
};
