// Доработка: Кряжев А., 2026
// Обработчики системных исключений, USB и контроля падения питания.
// Аварийный снимок сохраняется без использования повреждённого стека.
// SysTick обслуживает связь и таймеры; PVD передаёт сброс согласованному механизму.

#include "includes.h"
#include "Modules/comm_uart.h"

/* Фиксированное ОЗУ без инициализации, выше рабочей области старого загрузчика в начале ОЗУ.
 * 12 слов LE: метка, исключение, EXC_RETURN, SP, PC, LR, xPSR,
 * CFSR, HFSR, MMFAR, BFAR, frame_valid. Метка записывается последней.
 * Без стека и без перезарядки сторожевого таймера; незаконченные кадры и кадры вне ОЗУ отвергаются. */
#pragma location=".fault_record"
__root __no_init volatile uint32_t fault_record[12];

// Сохранить аварийный снимок без использования повреждённого стека.
__stackless void fault_capture(void)
{
  __asm(
    "CPSID I\n"
    "TST LR, #4\n"
    "ITE EQ\n"
    "MRSEQ R0, MSP\n"
    "MRSNE R0, PSP\n"
    "LDR R1, =fault_record\n"
    "MOVS R2, #0\n"
    "STR R2, [R1, #0]\n"
    "STR R2, [R1, #16]\n"
    "STR R2, [R1, #20]\n"
    "STR R2, [R1, #24]\n"
    "STR R2, [R1, #44]\n"
    "MRS R2, IPSR\n"
    "STR R2, [R1, #4]\n"
    "STR LR, [R1, #8]\n"
    "STR R0, [R1, #12]\n"
    "LDR R3, =0xE000ED28\n"
    "LDR R2, [R3, #0]\n"
    "STR R2, [R1, #28]\n"
    "LDR R2, [R3, #4]\n"
    "STR R2, [R1, #32]\n"
    "LDR R2, [R3, #12]\n"
    "STR R2, [R1, #36]\n"
    "LDR R2, [R3, #16]\n"
    "STR R2, [R1, #40]\n"
    "LDR R2, [R1, #28]\n"
    "MOVW R3, #0x3838\n"
    "TST R2, R3\n"
    "BNE fault_commit\n"
    "TST R0, #3\n"
    "BNE fault_commit\n"
    "LDR R2, =0x20000000\n"
    "CMP R0, R2\n"
    "BLO fault_commit\n"
    "LDR R2, =0x20004FA0\n"
    "CMP R0, R2\n"
    "BHI fault_commit\n"
    "LDR R2, [R0, #24]\n"
    "STR R2, [R1, #16]\n"
    "LDR R2, [R0, #20]\n"
    "STR R2, [R1, #20]\n"
    "LDR R2, [R0, #28]\n"
    "STR R2, [R1, #24]\n"
    "MOVS R2, #1\n"
    "STR R2, [R1, #44]\n"
    "fault_commit:\n"
    "DSB\n"
    "LDR R2, =0x31544C46\n"
    "STR R2, [R1, #0]\n"
    "DSB\n"
    "fault_wait:\n"
    "B fault_wait\n"
  );
}

#pragma call_graph_root="interrupt"
// Вернуться из немаскируемого IRQ; CSS на плате не включён.
void NMI_Handler(void)
{
}

#pragma call_graph_root="interrupt"
// Передать аппаратный сбой в безстековый снимок.
__stackless void HardFault_Handler(void)
{
  __asm("B fault_capture");
}

#pragma call_graph_root="interrupt"
// Сохранить нарушение защиты памяти без использования повреждённого стека.
__stackless void MemManage_Handler(void)
{
  __asm("B fault_capture");
}

#pragma call_graph_root="interrupt"
// Сохранить сбой шины без использования повреждённого стека.
__stackless void BusFault_Handler(void)
{
  __asm("B fault_capture");
}

#pragma call_graph_root="interrupt"
// Сохранить недопустимую инструкцию без использования повреждённого стека.
__stackless void UsageFault_Handler(void)
{
  __asm("B fault_capture");
}

#pragma call_graph_root="interrupt"
// Завершить программное исключение; приложение не использует службы SVC.
void SVC_Handler(void)
{
}

#pragma call_graph_root="interrupt"
// Завершить событие отладочного монитора.
void DebugMon_Handler(void)
{
}

#pragma call_graph_root="interrupt"
// Завершить отложенное исключение; диспетчера задач в приложении нет.
void PendSV_Handler(void)
{
}

#pragma call_graph_root="interrupt"
// Обслужить миллисекундные таймеры связи и приложения.
void SysTick_Handler(void)
{
  comm_uart_tick(); /* DMA/RTS проверяются каждые 1 мс. */
  timers_handle();
}

#pragma call_graph_root="interrupt"
// Передать события USB библиотеке CDC.
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

#pragma call_graph_root="interrupt"
// Подтвердить внутреннюю линию пробуждения USB.
void USBWakeUp_IRQHandler(void)
{
  EXTI->PR = EXTI_PR_PR18;
}

#pragma call_graph_root="interrupt"
// Подтвердить завершение инжектированного измерения.
void ADC1_2_IRQHandler(void)
{
  ADC1->SR = ~ADC_SR_JEOC;
}

// обработчик прерывания от PVD
#pragma call_graph_root="interrupt"
// Сохранить причину сброса и перезапустить МК при низком питании.
void PVD_IRQHandler(void)
{
  // сброс только при подтверждённом падении питания
  if ((EXTI->PR & EXTI->IMR & EXTI_PR_PR16) != 0) {
    EXTI->PR = EXTI_PR_PR16;
    if ((PWR->CSR & PWR_CSR_PVDO) != 0) {
      // Причина переживает системный сброс; журнал заполняется после старта.
      BKP->DR1 = PVD_RESET_MARK;
      __DSB();
      NVIC_SystemReset();
    }
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
