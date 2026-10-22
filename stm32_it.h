// Доработка: Кряжев А., 2026

#ifndef __STM32_IT_H
#define __STM32_IT_H

#include "stm32f10x.h"

extern volatile uint32_t fault_record[12];
#define FAULT_RECORD_MAGIC 0x31544C46u

void fault_capture(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void USBWakeUp_IRQHandler(void);
void ADC1_2_IRQHandler(void);
void PVD_IRQHandler(void);
#endif
