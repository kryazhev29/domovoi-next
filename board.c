// Доработка: Кряжев А., 2026
// Периферия платы v5: GPIO, контроль питания и резервные регистры.
// Начальные уровни выходов задаются до запуска пользовательской программы.
// Настройка через CMSIS сохраняет прежние режимы и приоритеты прерываний.

#include "board.h"

// Настроить фронт при закрытых масках, затем разрешить уже настроенную линию.
void board_exti_config(uint32_t lines, uint8_t falling)
{
  EXTI->IMR &= ~lines;
  EXTI->EMR &= ~lines;
  EXTI->RTSR &= ~lines;
  EXTI->FTSR &= ~lines;
  if (falling) EXTI->FTSR |= lines;
  else EXTI->RTSR |= lines;
  EXTI->IMR |= lines;
}

// Установить готовый приоритет и разрешить выбранное внутреннее прерывание.
void board_irq_enable(uint8_t irq, uint8_t priority)
{
  NVIC->IP[irq] = priority;
  NVIC->ISER[irq >> 5] = 1u << (irq & 31);
}

// Настроить входы, безопасные уровни выходов и детектор питания 2,5 В.
void board_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                  RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  // Режимы по 4 бита: 0 — аналог, 3 — выход 50 МГц, 7 — открытый сток, 8 — подтяжка.
  // Маски затрагивают только ножки платы, остальные настройки принадлежат драйверам.
  GPIOA->CRL = (GPIOA->CRL & ~0x000FFFFFu) | 0x00008883u;
  GPIOA->CRH = (GPIOA->CRH & ~0x00F0000Fu) | 0x00800003u;
  GPIOB->CRL = (GPIOB->CRL & ~0xFF00F0FFu) | 0x33003000u;
  GPIOB->CRH = (GPIOB->CRH & ~0xFF0F000Fu) | 0x43030003u;
  GPIOC->CRH = (GPIOC->CRH & ~0x0FF00000u) | 0x07300000u;
  GPIOA->BSRR = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_13;
  LED1_OFF;
  LED2_OFF;
  LED3_OFF;
  GSM_PWR_OFF;
  OUT1_OFF;
  OUT2_OFF;
  GSM_KEY_OFF;
  CSEL_HIGH;
  RTSG_LOW;

  RCC->AHBENR |= RCC_AHBENR_CRCEN;
  RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  PWR->CR = (PWR->CR & ~PWR_CR_PLS) | PWR_CR_DBP | 0x60u;
  EXTI->PR = EXTI_PR_PR16;
  board_exti_config(EXTI_IMR_MR16, 0);
  SCB->AIRCR = 0x05FA0700u;
  board_irq_enable(PVD_IRQn, 0);
  // Детектор включается последним: EXTI/NVIC уже готовы принять низкое питание при старте.
  PWR->CR |= PWR_CR_PVDE;
}

// Отключить и сбросить GPIO и AFIO перед переходом к загрузчику.
void board_deinit(void)
{
  const uint32_t ports = RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                         RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  RCC->APB2ENR &= ~ports;
  RCC->APB2RSTR |= ports;
  RCC->APB2RSTR &= ~ports;
}

// Запросить системный сброс с барьерами CMSIS.
void board_reset(void)
{
  NVIC_SystemReset();
}

// Ограниченно ждать восстановления питания, обслуживая аппаратный сторож.
void board_waitPowerStability(void)
{
  uint32_t timeout = SystemCoreClock;
  while ((PWR->CSR & PWR_CSR_PVDO) && timeout != 0) {
    IWDG->KR = 0xAAAAu;
    timeout--;
  }
}
