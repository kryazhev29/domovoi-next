// Доработка: Кряжев А., 2026
// Системный старт STM32F103CB платы v5: кварц 12 МГц, PLL 72 МГц.
// AHB/APB2 работают на 72 МГц, APB1 — на 36 МГц; флеш ждёт два такта.
// Отказ кварца или PLL приводит к сбросу, запуск с неверной частотой запрещён.
/**
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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

#include "stm32f10x.h"

uint32_t SystemCoreClock = 72000000u;

// Ограниченно ждать PLL/переключение частоты; отказ запрещает дальнейший старт.
static int clock_wait(volatile uint32_t *reg, uint32_t mask, uint32_t value)
{
  uint32_t budget = 100000u;
  while ((*reg & mask) != value) {
    if (--budget == 0) { NVIC_SystemReset(); return 0; }
  }
  return 1;
}

// Запустить кварц 12 МГц и переключить ядро на PLL с множителем 6.
static void SetSysClockTo72(void)
{
  uint32_t startup = 0;
  RCC->CR |= RCC_CR_HSEON;
  do {
    startup++;
  } while ((RCC->CR & RCC_CR_HSERDY) == 0 && startup != HSE_STARTUP_TIMEOUT);
  if ((RCC->CR & RCC_CR_HSERDY) == 0) {
    NVIC_SystemReset();
    return;
  }

  FLASH->ACR |= FLASH_ACR_PRFTBE;
  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_2;
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
  RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL)) |
              RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL6;
  RCC->CR |= RCC_CR_PLLON;
  if (!clock_wait(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY)) return;
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  (void)clock_wait(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL);
}

// Сбросить настройки RCC и запустить единственную конфигурацию платы.
#pragma call_graph_root="func"
void SystemInit(void)
{
  RCC->CR |= RCC_CR_HSION;
  RCC->CFGR &= 0xF8FF0000u;
  RCC->CR &= 0xFEF6FFFFu;
  RCC->CR &= ~RCC_CR_HSEBYP;
  RCC->CFGR &= 0xFF80FFFFu;
  RCC->CIR = 0x009F0000u;
  SetSysClockTo72();
  // Это исходное положение ST; приложение и загрузчик задают свой VTOR после старта.
  SCB->VTOR = FLASH_BASE;
}
