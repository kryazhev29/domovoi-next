/**
  ******************************************************************************
  * @file    stm32_it.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "includes.h"

/* Fixed noinit RAM, above the legacy loader's low-RAM work area.
 * 12 LE words: magic, exception, EXC_RETURN, SP, PC, LR, xPSR,
 * CFSR, HFSR, MMFAR, BFAR, frame_valid. Magic is committed last.
 * No stack or watchdog reload; reject incomplete/out-of-RAM frames. */
#pragma location=".fault_record"
__root __no_init volatile uint32_t fault_record[12];

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

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                          */
/******************************************************************************/

/*******************************************************************************
* Function Name  : NMI_Handler
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void NMI_Handler(void)
{
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
__stackless void HardFault_Handler(void)
{
  __asm("B fault_capture");
}

/*******************************************************************************
* Function Name  : MemManage_Handler
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
__stackless void MemManage_Handler(void)
{
  __asm("B fault_capture");
}

/*******************************************************************************
* Function Name  : BusFault_Handler
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
__stackless void BusFault_Handler(void)
{
  __asm("B fault_capture");
}

/*******************************************************************************
* Function Name  : UsageFault_Handler
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
__stackless void UsageFault_Handler(void)
{
  __asm("B fault_capture");
}

/*******************************************************************************
* Function Name  : SVC_Handler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void SVC_Handler(void)
{
}

/*******************************************************************************
* Function Name  : DebugMon_Handler
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void DebugMon_Handler(void)
{
}

/*******************************************************************************
* Function Name  : PendSV_Handler
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void PendSV_Handler(void)
{
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void SysTick_Handler(void)
{
  timers_handle();
}

/*******************************************************************************
* Function Name  : USB_IRQHandler
* Description    : This function handles USB Low Priority interrupts
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

/*******************************************************************************
* Function Name  : USB_FS_WKUP_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void USBWakeUp_IRQHandler(void)
{
  EXTI_ClearITPendingBit(EXTI_Line18);
}

/*******************************************************************************
* Function Name  : ADC1_2_IRQHandler
* Description    : This function handles ADC1 and ADC2 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#pragma call_graph_root="interrupt"
void ADC1_2_IRQHandler(void)
{
  ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
}

/******************************************************************************/
/*                 STM32 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32xxx.s).                                            */
/******************************************************************************/

/*******************************************************************************
* Function Name  : PPP_IRQHandler
* Description    : This function handles PPP interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/*void PPP_IRQHandler(void)
{
}*/

// обработчик прерывания от PVD
#pragma call_graph_root="interrupt"
void PVD_IRQHandler(void)
{
  // сразу в перезагрузку
  NVIC_SystemReset();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

