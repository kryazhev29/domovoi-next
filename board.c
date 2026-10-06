// ƒоработка:  р€жев ј., 2026
// ѕерифери€ платы v5: GPIO, контроль питани€ и резервные регистры.
// Ќачальные уровни выходов задаютс€ до запуска пользовательской программы.
// Ќастройка через CMSIS сохран€ет прежние режимы и приоритеты прерываний.

#include "board.h"

// Ќастроить фронт при закрытых масках, затем разрешить уже настроенную линию.
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

// ”становить готовый приоритет и разрешить выбранное внутреннее прерывание.
void board_irq_enable(uint8_t irq, uint8_t priority)
{
  NVIC->IP[irq] = priority;
  NVIC->ISER[irq >> 5] = 1u << (irq & 31);
}

// Ќастроить входы, безопасные уровни выходов и детектор питани€ 2,5 ¬.
void board_init(void)
{
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                  RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  // –ежимы по 4 бита: 0 Ч аналог, 3 Ч выход 50 ћ√ц, 7 Ч открытый сток, 8 Ч подт€жка.
  // ћаски затрагивают только ножки платы, остальные настройки принадлежат драйверам.
  // PA0 Ч реле, PA2/PA3 Ч Wiegand; PA1/PA4 остаютс€ в прежнем состо€нии.
  GPIOA->CRL = (GPIOA->CRL & ~0x0000FF0Fu) | 0x00008803u;
  // PA8 Ч подт€жка USB, PA13 Ч подт€жка входа загрузчика.
  GPIOA->CRH = (GPIOA->CRH & ~0x00F0000Fu) | 0x00800003u;
  // PB0/PB1 Ч аналоговые IN1/IN2; PB3 Ч реле, PB6/PB7 Ч светодиоды.
  GPIOB->CRL = (GPIOB->CRL & ~0xFF00F0FFu) | 0x33003000u;
  // PB8 Ч светодиод, PB12 Ч CS, PB14 Ч RTS, PB15 Ч детект USB.
  GPIOB->CRH = (GPIOB->CRH & ~0xFF0F000Fu) | 0x43030003u;
  // PC13 Ч питание модема, PC14 Ч открытый сток ключа запуска.
  GPIOC->CRH = (GPIOC->CRH & ~0x0FF00000u) | 0x07300000u;
  GPIOA->BSRR = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_13;
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
  // ƒетектор включаетс€ последним: EXTI/NVIC уже готовы прин€ть низкое питание при старте.
  PWR->CR |= PWR_CR_PVDE;
}

// ќтключить и сбросить GPIO и AFIO перед переходом к загрузчику.
void board_deinit(void)
{
  const uint32_t ports = RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                         RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;
  RCC->APB2ENR &= ~ports;
  RCC->APB2RSTR |= ports;
  RCC->APB2RSTR &= ~ports;
}

// «апросить системный сброс с барьерами CMSIS.
void board_reset(void)
{
  NVIC_SystemReset();
}

// ќграниченно ждать восстановлени€ питани€, обслужива€ аппаратный сторож.
void board_waitPowerStability(void)
{
  uint32_t timeout = SystemCoreClock;
  while ((PWR->CSR & PWR_CSR_PVDO) && timeout != 0) {
    IWDG->KR = 0xAAAAu;
    timeout--;
  }
}
